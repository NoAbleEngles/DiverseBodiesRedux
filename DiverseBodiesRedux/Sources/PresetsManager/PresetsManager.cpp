#include "PresetsManager.h"
#include "Ini/ini.h"
#include <sstream>

namespace globals {
    extern ini::map* g_ini;
}


PresetsManager::PresetsManager() {
    // Парсим BODYMORPHS из папок, указанных в ini-файле 
    auto folders_string = globals::g_ini->at<std::string>("PATH/sBodymorphsFolders", "");
    if (folders_string.empty()) {
        logger::error("No bodymorphs folders defined in ini.");
        return;
    }
    m_bodymorphsFolders = utils::string::split(folders_string, ",");

    for (const auto& folder : m_bodymorphsFolders) {
        if (folder.empty()) {
            logger::error("Empty folder path in bodymorphs folders.");
            continue;
		}
        loadPresetsFromFolder<BodymorphsPreset>(folder);
    }

    // Парсим BODYHAIRS из папок, указанных в ini-файле
    folders_string = globals::g_ini->at<std::string>("PATH/sBodyhairsFolders", "");
    if (folders_string.empty()) {
        logger::error("No bodyhairs folders defined in ini.");
        return;
    }
    m_bodymorphsFolders = utils::string::split(folders_string, ",");

    for (const auto& folder : m_bodymorphsFolders) {
        if (folder.empty()) {
            logger::error("Empty folder path in bodyhairs folders.");
            continue;
        }
        loadPresetsFromFolder<BodyhairsPreset>(folder);
    }
}

PresetsManager& PresetsManager::get() {
    static PresetsManager instance;
    return instance;
}

void PresetsManager::validatePresets() {
    // Копируем пресеты под мьютексом
    std::vector<std::shared_ptr<Preset>> presetsCopy;
    {
        std::lock_guard lock(m_presetsMutex);
        presetsCopy.assign(m_presets.begin(), m_presets.end());
    }

    // Для каждого пресета запускаем асинхронную проверку
    std::vector<std::pair<std::shared_ptr<Preset>, std::future<bool>>> futures;
    for (const auto& preset : presetsCopy) {
        if (preset) {
            futures.emplace_back(preset, preset->isValidAsync());
        }
    }

    // Запускаем отдельный detached thread для сбора результатов и обновления коллекции
    std::thread([this, futures = std::move(futures)]() mutable {
        decltype(m_presets) validPresets;
        for (auto& [preset, fut] : futures) {
            if (fut.valid() && fut.get()) {
                validPresets.insert(preset);
            }
        }
        // Обновляем коллекцию под мьютексом
        {
            std::lock_guard lock(m_presetsMutex);
            m_presets.swap(validPresets);
            m_presetsValidated = true;
        }
        // Уведомляем всех подписчиков о завершении валидации
        {
            std::lock_guard lock(m_callbacksMutex);

            // Сначала вызываем все колбэки
            for (const auto& [id, cb, once] : m_validationCallbacks) {
                if (cb) cb();
            }

            // Затем удаляем одноразовые подписки
            m_validationCallbacks.erase(
                std::remove_if(
                    m_validationCallbacks.begin(), m_validationCallbacks.end(),
                    [](const CallbackEntry& entry) { return entry.oneShot; }
                ),
                m_validationCallbacks.end()
            );
        }
        // validPresets уничтожается вне lock'а
        }).detach();
}

bool PresetsManager::isReady() {
    return m_presetsValidated;
}

PresetsManager::CallbackId PresetsManager::subscribeOnValidated(const std::function<void()>& callback, bool oneTimeShot) {
    std::lock_guard lock(m_callbacksMutex);
    CallbackId id = m_nextCallbackId++;
    m_validationCallbacks.push_back({ id, callback , oneTimeShot });
    return id;
}

bool PresetsManager::unsubscribeFromValidated(CallbackId id) {
    std::lock_guard lock(m_callbacksMutex);
    m_validationCallbacks.erase(
        std::remove_if(
            m_validationCallbacks.begin(), m_validationCallbacks.end(),
            [id](const CallbackEntry& entry) { return entry.id == id; }
        ),
        m_validationCallbacks.end()
    );
}

inline std::shared_ptr<Preset> PresetsManager::getPreset(const std::string& id) const noexcept {
    auto it = std::find_if(m_presets.begin(), m_presets.end(),
        [&id](const std::shared_ptr<Preset>& preset) {
            return preset && preset->id() == id;
	});
	return (it != m_presets.end()) ? *it : nullptr;
}



std::shared_ptr<Preset> PresetsManager::operator[](const std::string& id) const noexcept {
    return getPreset(id);
}

std::vector<std::shared_ptr<Preset>> PresetsManager::getPresets(const RE::Actor* actor, const std::function<bool(const RE::Actor* actor, const Preset&)>& filter) const noexcept {
	std::vector<std::shared_ptr<Preset>> applicablePresets;
    
    logger::info("GET PRESETS:");
    for (const auto& el : m_presets) {
        logger::info("{} - {}", el->id(), static_cast<int>(el->type()));
    }

    if (filter == nullptr) {
        for (const auto& preset : m_presets) {
            if (preset && preset->check(actor) != CoincidenceLevel::NONE) {
                applicablePresets.push_back(preset);
            }
        }

    } else {
        for (const auto& preset : m_presets) {
            if (preset && filter(actor, *preset)) {
                applicablePresets.push_back(preset);
            }
		}
	}

    std::sort(applicablePresets.begin(), applicablePresets.end(),
        [](const std::shared_ptr<Preset>& a, const std::shared_ptr<Preset>& b) {
            return *a < *b;
    });

    return applicablePresets;
}

std::vector<std::shared_ptr<Preset>> PresetsManager::operator[](const RE::Actor* actor) const noexcept {
	return getPresets(actor);
}

std::string PresetsManager::print()
{
    std::ostringstream oss;
    std::lock_guard lock(m_presetsMutex);
    oss << "PresetsManager: " << m_presets.size() << " presets loaded\n";
    for (const auto& presetPtr : m_presets) {
        if (presetPtr) {
            oss << presetPtr->print() << "\n";
        }
    }
    return oss.str();
}