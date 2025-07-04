#pragma once
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <Preset/Preset.h>
#include <PresetsManager/PresetsManager.h>
#include <set>
#include <memory>
#include <filesystem>
#include <type_traits>
#include <typeinfo>
#include <mutex>
#include <vector>
#include <functional>

/**
 * @brief Менеджер пресетов. Отвечает за загрузку, хранение и валидацию пресетов.
 */
class PresetsManager {
public:
    /**
     * @brief Уникальный идентификатор для колбэков валидации пресетов.
     * Используется для отписки от событий.
	 */
    using CallbackId = size_t;

    /**
     * @brief Получить единственный экземпляр менеджера (Singleton).
     * @return Ссылка на экземпляр PresetsManager.
     */
    static PresetsManager& get();

    /**
     * @brief Валидирует все загруженные пресеты.
     */
    void validatePresets();

    /**
     * @brief Готов ли менеджер к использованию? Пресеты загружены и валидированы хотя бы один раз.
     * @return true если готово, иначе false.
     */
    bool isReady();

    /**
     * @brief Подписаться на событие завершения валидации пресетов.
     * @param callback Функция, вызываемая после завершения валидации.
	 * @return Уникальный идентификатор колбэка, который можно использовать для отписки.
     */
    CallbackId subscribeOnValidated(const std::function<void()>& callback, bool oneShot = false);

    /**
     * @brief Отписаться от события завершения валидации пресетов.
     * @param callback Функция, вызываемая после завершения валидации.
     */
	bool unsubscribeFromValidated(CallbackId id);

    /**
	 * @brief Получить пресет по уникальному идентификатору.
	 * @param id Уникальный идентификатор пресета (имя файла без расширения).
	 * @return Указатель на Preset, если найден, иначе nullptr.
     */
    std::shared_ptr<Preset> getPreset(const std::string& id) const noexcept;


    /// @copydoc PresetsManager::getPreset
    std::shared_ptr<Preset> operator[](const std::string& id) const noexcept;

    /**
     * @brief Получить вектор пресетов подходящих под актёра по условиям.
	 * @param actor Актёр для которого нужно получить пресеты.
     * @param Можно установить свой фильтр для получения только определённых пресетов.
	 * @return вектор уникальных указателей на Preset, подходящих под условия актёра, отсортированных по типу пресета.
     */
    std::vector<std::shared_ptr<Preset>> getPresets(const RE::Actor* actor, const std::function<bool(const RE::Actor* actor, const Preset&)>& filter = nullptr) const noexcept;

    /// @copydoc PresetsManager::getPresets
   std::vector<std::shared_ptr<Preset>> operator[](const RE::Actor* actor) const noexcept;

   /*
   * @brief Получить все пресеты, загруженные в менеджер.
   * @return Стринг с распечатанными пресетами.
   */
   std::string print();

private:
    /**
     * @brief Внутренний тип для хранения колбэков с уникальным идентификатором.
	 * Потокобезопасный контейнер для хранения колбэков валидации.
	 */
    struct CallbackEntry {
        CallbackId id;
        std::function<void()> callback;
		bool oneShot{ false }; // Флаг для одноразового вызова колбэка
    };

    std::vector<CallbackEntry> m_validationCallbacks;
    std::mutex m_callbacksMutex;
    std::atomic<CallbackId> m_nextCallbackId{ 1 };

    /**
     * @brief Приватный конструктор, чтобы нельзя было создать экземпляр извне.
     */
    PresetsManager();

    // Запрет копирования и перемещения
    PresetsManager(const PresetsManager&) = delete;
    PresetsManager& operator=(const PresetsManager&) = delete;
    PresetsManager(PresetsManager&&) = delete;
    PresetsManager& operator=(PresetsManager&&) = delete;

    /**
     * @brief Множество уникальных указателей на Preset.
     */
    std::set<std::shared_ptr<Preset>> m_presets;

    /**
     * @brief Пути к папкам с bodymorphs-пресетами.
     */
    std::vector<std::string> m_bodymorphsFolders{};

    /**
     * @brief Пути к папкам с bodyhairs-пресетами.
     */
    std::vector<std::string> m_bodyhairsFolders{};

    /**
     * @brief Мьютекс для потокобезопасного доступа к пресетам.
     */
    mutable std::mutex m_presetsMutex;

    /**
     * @brief Флаг того что хотя бы один раз прошла валидация пресетов.
     */
    bool m_presetsValidated{ false };

    /**
     * @brief Загружает пресеты из указанной папки.
     * @tparam T Тип пресета (должен быть унаследован от Preset).
     * @param folderPath Путь к папке.
     * @return true, если хотя бы один пресет был успешно загружен.
     */
    template<typename T>
    bool loadPresetsFromFolder(const std::string& folderPath);
};

template<typename T>
inline bool PresetsManager::loadPresetsFromFolder(const std::string& folderPath) {
    static_assert(std::is_base_of<Preset, T>::value, "T must be derived from Preset");

    namespace fs = std::filesystem;

    fs::path path(folderPath);
    if (!path.is_absolute()) {
        // Относительный путь — делаем его относительно текущей директории (обычно папка с exe)
        path = fs::current_path() / path;
    }

    if (fs::exists(path) && fs::is_directory(path)) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".json" || entry.path().extension() == ".xml")) {
                if (entry.path().filename().string().ends_with("_conds.json")) {
					// Пропускаем файлы с условиями
                    continue;
				}
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
            logger::info("Successfully loaded {} {} presets from folder: {}", count, typeid(T).name(), path.string());
            return true;
        }
        else {
            logger::error("No valid {} presets found in folder: {}", typeid(T).name(), path.string());
            return false;
        }
    }
    else {
        logger::error("Directory does not exist or is not a directory: {}", path.string());
        return false;
    }
}