#include "PresetsManager.h"
#include "Ini/ini.h"

extern ini::map g_ini;

PresetsManager::PresetsManager() {
    // Парсим BODYMORPHS из папок, указанных в ini-файле 
    auto folders_string = g_ini.at<std::string>("PATH/sBodymorphsFolders", "");
    if (folders_string.empty()) {
        logger::error("No bodymorphs folders defined in ini.");
        return;
    }
    m_bodymorphsFolders = utils::string::split(folders_string, ",");

    for (const auto& folder : m_bodymorphsFolders) {
        loadPresetsFromFolder<BodymorphsPreset>(folder);
    }

    // Парсим BODYHAIRS из папок, указанных в ini-файле
    folders_string = g_ini.at<std::string>("PATH/sBodyhairsFolders", "");
    if (folders_string.empty()) {
        logger::error("No bodyhairs folders defined in ini.");
        return;
        m_bodymorphsFolders = utils::string::split(folders_string, ",");

        for (const auto& folder : m_bodymorphsFolders) {
            loadPresetsFromFolder<BodymorphsPreset>(folder);
        }
    }
}

PresetsManager& PresetsManager::get() {
    static PresetsManager instance;
    return instance;
}

void PresetsManager::validatePresets() {
    std::set<std::shared_ptr<Preset>> validPresets;

    {
        std::lock_guard lock(m_presetsMutex);
        // Копируем валидные пресеты во временный сет
        for (const auto& preset : m_presets) {
            if (preset && preset->isValid()) {
                validPresets.insert(preset);
            }
        }
        // Обмениваем содержимое
        m_presets.swap(validPresets);
    }
    // validPresets теперь содержит "старые" (невалидные) пресеты, которые будут уничтожены вне lock'а
}