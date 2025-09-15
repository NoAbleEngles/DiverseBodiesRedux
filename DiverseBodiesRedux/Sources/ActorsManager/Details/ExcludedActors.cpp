#include "ExcludedActors.h"
#include <Utils/utility.h>
#include "LooksMenu/ParseLooksMenuPreset.h" // для find_ci

extern std::string getJson(const std::filesystem::path& filepath);

inline std::unordered_set<std::uint32_t> getActorAndTesNPCFormIDsFromJsonArray(boost::json::array& arr, const std::string& plugin = std::string{}) {
    std::unordered_set<std::uint32_t> set{};
    for (const auto& formID : arr) {
        if (formID.is_string()) {
            std::string str = formID.as_string().c_str();
            auto tesform = functions::getFormFromString(str, plugin);
            if (tesform->As<RE::Actor>() || tesform->As<RE::TESNPC>()) {
                set.insert(tesform->GetFormID());
            }
            else {
                logger::info("Invalid formID: {} in plugin: {}", str, plugin.empty() ? "Fallout4.esm" : plugin);
            }
        }
    }
    return set;
}

void ExcludedActors::refreshExclusionList() noexcept {
	static auto ini = globals::g_ini;
	ini->reload();
	auto foldersStr = ini->at("PATH/sExclusions", std::string{});
	if (foldersStr.empty()) {
		m_excludedForms.clear();
		return;
	}
	auto folders = utils::string::split(foldersStr, ",");
	
    if (folders.empty()) {
        logger::error("No folders specified in PATH/sExclusions");
        return;
    }

    for (const auto& folder : folders) {
        if (folder.empty()) {
            logger::error("Empty folder path in bodymorphs folders.");
            continue;
        }
        loadExclusionsFromFolder(folder);
    }
}

bool ExcludedActors::loadExclusionsFromFolder(const std::string& folderPath) noexcept {
    namespace fs = std::filesystem;
    fs::path path(folderPath);

    if (!path.is_absolute()) {
        // Относительный путь — делаем его относительно текущей директории (обычно папка с exe)
        path = fs::current_path() / path;
    }

    if (fs::exists(path) && fs::is_directory(path)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(path)) {
			if (loadExclusionsFromJsonFile(entry.path())) ++count;
        }
        if (count > 0) {
            return true;
        } else {
            logger::warn("No valid .json files found in directory: {}", path.string());
			return false;
        }
    } else {
        logger::error("Directory does not exist or is not a directory: {}", path.string());
        return false;
    }
}

bool ExcludedActors::loadExclusionsFromJsonFile(const std::filesystem::path& path) noexcept {
    namespace fs = std::filesystem;
    if (!fs::exists(path) || path.extension() != ".json") {
        logger::error("File does not exist or is not a .json: {}", path.string());
        return false;
    }

    auto jsonString = getJson(path);

    boost::json::value jv = boost::json::parse(jsonString);

    if (jv.is_array()) {
        auto& arr = jv.as_array();
        bool anyLoaded = false;
        for (auto& item : arr) {
            if (item.is_object()) {
                if (loadExclusionsFromJsonObject(item.as_object())) {
                    anyLoaded = true;
                }
            }
        }
        return anyLoaded;
    }
    else if (jv.is_object()) {
        return loadExclusionsFromJsonObject(jv.as_object());
    }
    else {
        logger::error("JSON root is not an object or array: {}", path.string());
        return false;
    }
}

bool ExcludedActors::loadExclusionsFromJsonObject(boost::json::object& obj) noexcept {
    if (obj.empty()) {
        logger::error("JSON object is empty, cannot load exclusions.");
        return false;
	}

    auto formIDsArr = find_ci(obj, "formid");
    if (formIDsArr == obj.end() || !formIDsArr->value().is_array()) {
        logger::error("No valid 'formid' array found in JSON object.");
        return false;
	}

	std::string plugin;
	auto it = find_ci(obj, "plugin");
    if (it == obj.end() || !it->value().is_string()) {
		plugin = std::string{};
    } else {
		plugin = it->value().as_string().c_str();
    }

	auto forms = getActorAndTesNPCFormIDsFromJsonArray(formIDsArr->value().as_array(), plugin);

    if (forms.empty()) {
        logger::error("No valid Actor or TESNPC formIDs found in JSON object.");
        return false;
    }


    std::shared_ptr<std::unordered_set<PresetType>> flags;
	it = find_ci(obj, "exclusionflags");
    if (it != obj.end() && it->value().is_array()) {
		auto& arr = it->value().as_array();
        if (!arr.empty() && std::find_if(arr.begin(), arr.end(), [](const boost::json::value& v) {
            if (!v.is_string()) return false;
            std::string s = v.as_string().c_str();
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s == "all";
            }) == arr.end())
        {
            for (auto& flag : arr) {
                if (flag.is_string()) {
                    std::string flagStr = flag.as_string().c_str();
                    auto f = GetPresetTypeFromString(flagStr);
                    if (f != PresetType::NONE) {
                        if (!flags) {
                            flags = std::make_shared<std::unordered_set<PresetType>>();
						}
                        flags->insert(f);
                    } else {
                        logger::warn("Unknown exclusion flag: {}", flagStr);
                    }
                }
                else {
					logger::warn("Exclusion flag is not a string, skipping.");
                }
            }
        }  
    }

    if (flags && !flags->empty() && flags->size() == static_cast<int>(PresetType::END) - 1) {
		flags.reset(); // все флаги - эквивалентно null
    }
    else if (flags && flags->empty()) {
        flags.reset();
    }

    for (auto& form : forms) {
        m_excludedForms[form] = flags;
	}
	return true;
}

std::optional<std::shared_ptr<std::unordered_set<PresetType>>> ExcludedActors::getExcludedTypes(const RE::Actor* actor) const noexcept
{
    if (!actor) {
        return std::nullopt;
    }

    auto it = m_excludedForms.find(actor->formID);
    if (it != m_excludedForms.end()) {
        return it->second;
    }

    auto* leveledForm = functions::getLeveledForm(actor);
    if (!leveledForm) {
        leveledForm = actor->GetNPC();
	}
    if (!leveledForm) {
        return std::nullopt;
	}

    it = m_excludedForms.find(leveledForm->GetFormID());
    if (it != m_excludedForms.end()) {
        return it->second;
    }

    return std::nullopt;
}