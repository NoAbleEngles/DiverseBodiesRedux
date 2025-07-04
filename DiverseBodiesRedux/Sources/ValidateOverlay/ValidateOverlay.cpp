#include "ValidateOverlay.h"
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <boost/json.hpp>
#include <filesystem>
#include <stack>
#include <fstream>
#include <sstream>
#include <chrono>

namespace logger = F4SE::log;

using namespace std::literals;


ValidateOverlay& ValidateOverlay::validateOverlay() {
    static ValidateOverlay instance;
    return instance;
}

std::future<bool> ValidateOverlay::operator()(const std::string& id)
{
    using namespace std::chrono;
    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    std::shared_ptr<std::future<void>> updateFuture;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (now - m_lastCheckTime > 30) {
            updateFuture = update_async();
        }
        else if (m_updateInProgress && m_updateFuture) {
            updateFuture = m_updateFuture;
        }
    }

    if (updateFuture) {
        // Ждём завершения обновления, но не блокируем основной поток
        return std::async(std::launch::async, [this, id, updateFuture]() {
            updateFuture->wait();
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_validOverlays.contains(id);
            });
    }
    else {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::async(std::launch::deferred, [this, id]() {
            return m_validOverlays.contains(id);
            });
    }
}

std::shared_ptr<std::future<void>> ValidateOverlay::update_async()
{
    if (m_updateInProgress.exchange(true)) {
        return m_updateFuture;
    }
    auto fut = std::make_shared<std::future<void>>(std::async(std::launch::async, [this]() {
        this->update();
        m_updateInProgress = false;
        }));
    m_updateFuture = fut;
    return fut;
}

void ValidateOverlay::update()
{
    std::set<std::string> validOverlays;
    std::stack<std::string> fault;
    std::filesystem::path folder;

    try {
        folder = std::filesystem::current_path() / "Data" / "F4SE" / "Plugins" / "F4EE" / "Overlays";
    }
    catch (const std::exception& e) {
        logger::error("Failed open file {} : {}", folder.string(), e.what());
        return;
    }

    std::error_code error{};

    auto FileExists = [](const std::string& filename) -> bool {
        if (filename.empty())
            return false;
        std::filesystem::path filePath = std::filesystem::current_path() / "Data" / "materials" / filename;
        if (std::filesystem::exists(filePath))
            return true;

        RE::BSTSmartPointer<RE::BSResource::Stream, RE::BSTSmartPointerIntrusiveRefCount> a_result = nullptr;
        [[maybe_unused]] auto files = RE::BSResource::GetOrCreateStream(("materials/" + filename).c_str(), a_result);

        if (a_result) {
            a_result->DoClose();
            return true;
        }
        return false;
        };

    auto get_json = [](const std::filesystem::path& path) -> std::string {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Cannot open file");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
        };

    auto ParseOverlayJSON = [&](const std::filesystem::path& path) -> bool {
        if (path.extension() != ".json")
            return false;
        boost::json::value json_value;
        try {
            std::string json_string = get_json(path);
            json_value = boost::json::parse(json_string);
        }
        catch (const std::exception& e) {
            logger::error("Failed parse ValidOverlays {} : {}", path.string(), e.what());
            return false;
        }
        if (!json_value.is_array())
            return false;

        const auto& json_array = json_value.as_array();
        std::string id;
        int success_counter = 0;
        for (const auto& item : json_array) {
            if (!item.is_object())
                continue;
            const auto& obj = item.as_object();
            auto it_id = obj.find("id");
            auto it_slots = obj.find("slots");
            if (it_id == obj.end() || it_slots == obj.end())
                continue;
            id = obj.at("id").as_string().c_str();
            const auto& slots = obj.at("slots").as_array();
            for (const auto& slot : slots) {
                if (slot.is_object() && slot.as_object().find("material") != slot.as_object().end()) {
                    std::string material = slot.at("material").as_string().c_str();
                    if (FileExists(material))
                        ++success_counter;
                    else
                        logger::error("Material file not found: {}", material);
                }
            }
        }
        if (success_counter && !id.empty()) {
            validOverlays.emplace(id);
            return true;
        }
        return false;
        };

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.exists(error) && entry.is_directory(error)) {
            for (const auto& file : std::filesystem::directory_iterator(entry.path())) {
                if (file.exists(error)) {
                    if (file.path().filename() == "overlays.json") {
                        if (!ParseOverlayJSON(file.path()))
                            fault.push(file.path().string());
                    }
                    else {
                        fault.push(file.path().string());
                    }
                }
                else {
                    fault.emplace("Failed to iterate directory " + file.path().string() + " : " + error.message());
                }
            }
        }
        else {
            fault.emplace("Failed to iterate directory " + folder.string() + " : " + error.message());
        }
    }

    if (!fault.empty()) {
        logger::info("Parse Overlays failed : ");
        while (!fault.empty()) {
            logger::info("\t{}", fault.top());
            fault.pop();
        }
    }

    if (validOverlays.empty())
        logger::warn("Failed to parse valid overlays : no overlays in folder!");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_validOverlays.swap(validOverlays);
        m_lastCheckTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}