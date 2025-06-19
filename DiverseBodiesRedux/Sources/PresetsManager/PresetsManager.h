#pragma once
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <Preset/Preset.h>
#include <set>
#include <memory>
#include <filesystem>
#include <type_traits>
#include <typeinfo>
#include <mutex>

/**
 * @brief �������� ��������. �������� �� ��������, �������� � ��������� ��������.
 */
class PresetsManager {
public:
    /**
     * @brief �������� ������������ ��������� ��������� (Singleton).
     * @return ������ �� ��������� PresetsManager.
     */
    static PresetsManager& get();

    /**
     * @brief ���������� ��� ����������� �������.
     */
    void validatePresets();
private:
    /**
     * @brief ��������� �����������, ����� ������ ���� ������� ��������� �����.
     */
    PresetsManager();

    // ������ ����������� � �����������
    PresetsManager(const PresetsManager&) = delete;
    PresetsManager& operator=(const PresetsManager&) = delete;
    PresetsManager(PresetsManager&&) = delete;
    PresetsManager& operator=(PresetsManager&&) = delete;

    /**
     * @brief ��������� ���������� ���������� �� Preset.
     */
    std::set<std::shared_ptr<Preset>> m_presets;

    /**
     * @brief ���� � ������ � bodymorphs-���������.
     */
    std::vector<std::string> m_bodymorphsFolders{};

    /**
     * @brief ���� � ������ � bodyhairs-���������.
     */
    std::vector<std::string> m_bodyhairsFolders{};

    /**
     * @brief ������� ��� ����������������� ������� � ��������.
     */
    mutable std::mutex m_presetsMutex;

    /**
     * @brief ��������� ������� �� ��������� �����.
     * @tparam T ��� ������� (������ ���� ����������� �� Preset).
     * @param folderPath ���� � �����.
     * @return true, ���� ���� �� ���� ������ ��� ������� ��������.
     */
    template<typename T>
    bool loadPresetsFromFolder(const std::string& folderPath);
};

template<typename T>
inline bool PresetsManager::loadPresetsFromFolder(const std::string& folderPath) {
    static_assert(std::is_base_of<Preset, T>::value, "T must be derived from Preset");

    namespace fs = std::filesystem;

    if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".json" || entry.path().extension() == ".xml")) {
                auto preset = std::make_shared<T>(entry.path());
                if (!preset->empty()) {
                    ++count;
                    m_presets.insert(preset);
                }
                else {
                    logger::error("Failed to load {} from file: {}", typeid(T).name(), entry.path().string());
                }
            }
        }
        if (count) {
            logger::info("Successfully loaded {} {} presets from folder: {}", count, typeid(T).name(), folderPath);
            return true;
        }
        else {
            logger::error("No valid {} presets found in folder: {}", typeid(T).name(), folderPath);
            return false;
        }
    }
    else {
        logger::error("Directory does not exist or is not a directory: {}", folderPath);
        return false;
    }
}