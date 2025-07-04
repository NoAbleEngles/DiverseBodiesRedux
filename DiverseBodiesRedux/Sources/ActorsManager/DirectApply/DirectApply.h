#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include <functional>
#include "../Details/ActorsPresetHashEquals.hpp"

enum class MENU_STATE : uint8_t {
    NONE = 0,       // Меню не активно
    ACTORS = 1,     // Меню выбора актёров
    OPTIONS = 2,    // Меню настроек фильтров
    PRESETS = 3     // Меню выбора пресетов
};

enum MENU_OBJECT {
    LABEL = 0,
    BUTTON = 1,
    CHECKBOX = 2,
    SWITCHER = 3
};

using ApplyPreset = std::function<bool(RE::Actor* actor, const std::shared_ptr<Preset>& preset)>;
using UpdateInStorage = std::function<bool(RE::Actor* actor, const std::shared_ptr<Preset>& preset)>;

/**
 * @brief Класс для прямого применения пресетов к телам персонажей через Scaleform-меню.
 *
 * Используется только внутри ActorsManager для управления выбором актёра, фильтрацией и применением пресетов.
 * Предоставляет интерфейс для взаимодействия с пользователем через меню, фильтрации пресетов и их применения.
 * Экземпляр класса должен создаваться только внутри ActorsManager.
 *
 * Основные возможности:
 * - Фильтрация пресетов по различным критериям.
 * - Применение выбранного пресета к телу персонажа.
 * - Сохранение пресета в m_presets класса ActorsManager.
 * - Взаимодействие с GUI через Scaleform.
 */
class DirectApply : public RE::Scaleform::GFx::FunctionHandler 
{
public:
    struct Options {
        RE::Actor* actor{ nullptr };           ///< Текущий выбранный актёр
        Filter filter{ static_cast<Filter>(1) }; ///< Активный фильтр для пресетов
        PresetType presetType{ PresetType::BODYMORPHS }; ///< Тип пресета
    };

    /**
     * @brief Конструктор. Инициализирует меню и связывает с внешними данными.
     * @param m_presets Ссылка на карту пресетов ActorsManager.
     * @param m_mutex Ссылка на мьютекс для синхронизации доступа.
     * @param applyPresetFunc Функция применения пресета к актёру.
     * @param updateInStorage Функция обновления пресета в хранилище.
     */
    explicit DirectApply(const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>* const m_presets,
        const std::mutex* m_mutex,
        const ApplyPreset& applyPresetFunc,
        const UpdateInStorage& updateInStorage);

    ~DirectApply();

    inline static void* m_performInputProcessingHook{ nullptr };       ///< Хук для обработки ввода от игрока
    inline static std::function<void(const RE::ButtonEvent*)> 
        m_buttonEventHandler = nullptr;                         ///< Обработчик событий кнопок

    DirectApply() = delete;
    DirectApply(const DirectApply&) = delete;
    DirectApply& operator=(const DirectApply&) = delete;
    DirectApply(DirectApply&&) = delete;
    DirectApply& operator=(DirectApply&&) = delete;

    /**
     * @brief Отображает меню выбора актёров.
     */
    void showActors();

    /**
     * @brief Отображает меню фильтров и опций для выбранного актёра.
     */
    void showOptions();

    /**
     * @brief Отображает меню выбора пресетов для текущего актёра.
     * @param active Индекс активного элемента меню.
     */
    void showPresets(int active = 0);

    /**
     * @brief Скрывает меню и сбрасывает состояние.
     */
    void hide();

    /**
     * @brief Проверяет, инициализировано ли меню.
     * @return true, если меню инициализировано, иначе false.
     */
    bool isMenuInitialized() const noexcept;

    /**
     * @brief Обрабатывает вызовы из Scaleform (AS3) интерфейса.
     * @param params Параметры вызова из Flash.
     */
    void Call(const RE::Scaleform::GFx::FunctionHandler::Params& params) override;

private:
    RE::Scaleform::GFx::MovieDef* m_movieDef{ nullptr }; ///< Определение SWF-меню
    RE::Scaleform::GFx::Movie* m_movie{ nullptr };       ///< Экземпляр SWF-меню
    MENU_STATE m_state{ MENU_STATE::NONE };              ///< Текущее состояние меню
    Options m_options{};                                 ///< Текущие опции выбора
    int m_maxActorsCount{ 10 };                          ///< Максимальное количество актёров в меню
    bool m_initialized{ false };                         ///< Флаг инициализации меню

    const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>* const m_presets; ///< Ссылка на карту пресетов
    const std::mutex* m_mutex;                          ///< Ссылка на мьютекс для синхронизации
    const ApplyPreset m_applyPresetFunc;                ///< Функция применения пресета
    const UpdateInStorage m_updateInStorageFunc;        ///< Функция обновления пресета в хранилище

    /**
     * @brief Обработка нажатия на элемент меню актёров.
     * @param params Параметры вызова из Scaleform.
     */
    void actorsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка нажатия на элемент меню опций.
     * @param params Параметры вызова из Scaleform.
     */
    void optionsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка нажатия на элемент меню пресетов.
     * @param params Параметры вызова из Scaleform.
     */
    void presetsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка возврата из меню актёров.
     * @param params Параметры вызова из Scaleform.
     */
    void actorsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка возврата из меню опций.
     * @param params Параметры вызова из Scaleform.
     */
    void optionsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка возврата из меню пресетов.
     * @param params Параметры вызова из Scaleform.
     */
    void presetsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Обработка возврата из меню пресетов.
     * @param params Параметры вызова из Scaleform.
     */
    void presetsHoverCall(const RE::Scaleform::GFx::FunctionHandler::Params& params);

    /**
     * @brief Получает тип пресета по строковому представлению.
     * @param str Строка с названием типа пресета.
     * @return Тип пресета или PresetType::NONE, если не найден.
     */
    static PresetType getPresetTypeFromString(const std::string& str);

    /**
     * @brief Получает список актёров в радиусе от указанного объекта.
     * @param a_ref Ссылка на объект-центр поиска (по умолчанию игрок).
     * @param a_maxDistance Максимальная дистанция поиска.
     * @param a_maxActorsCount Максимальное количество актёров.
     * @param a_includeDead Включать ли мёртвых актёров.
     * @return Вектор указателей на найденных актёров.
     */
    static std::vector<RE::Actor*> getActorsInRange(RE::TESObjectREFR* a_ref = RE::PlayerCharacter::GetSingleton(),
        std::uint32_t a_maxDistance = 5000, int a_maxActorsCount = 10,
        bool a_includeDead = false);

    inline static int m_counter{};
};