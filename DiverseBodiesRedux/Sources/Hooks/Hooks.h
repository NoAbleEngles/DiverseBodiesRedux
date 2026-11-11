#pragma once
#include <string>
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <detourxs/detourxs.h>
#include <typeinfo>
#include <functional>

namespace logger = F4SE::log;

/**
 * @brief Универсальный шаблонный класс для создания хуков функций.
 * Позволяет быстро и удобно создавать хуки для методов классов и отдельных функций.
 * Использует DetourXS для подмены указателя на функцию.
 *
 * @tparam TFunc Тип функции (например, typedef для метода или функции).
 */
template <typename TFunc>
class Hooks
{
public:
    /**
     * @brief Конструктор по умолчанию.
     */
    Hooks();

    /**
     * @brief Конструктор.
     * @param targetAddress Абсолютный адрес оригинальной функции для перехвата.
     * @param hookFunction Адрес вашей функции-обработчика (хук).
     */
    Hooks(uintptr_t targetAddress, TFunc hookFunction);

    /**
     * @brief Конструктор.
     * @param basePlugin имя оригинального плагина.
     * @param offsetAddress Адрес оригинальной функции для перехвата относительно плагина.
     * @param hookFunction Адрес вашей функции-обработчика (хук).
     */
    Hooks(const std::string& basePlugin, uintptr_t offsetAddress, TFunc hookFunction);

    /**
     * @brief Конструктор.
     * @param basePlugin имя оригинального плагина.
	 * @param getAddress Функция которая вернёт адрес оригинальной функции для перехвата относительно плагина. Вызывается один раз, при установке хука.
     * @param hookFunction Адрес вашей функции-обработчика (хук).
     */
    Hooks(const std::function<uintptr_t()>& getAddress, TFunc hookFunction);

    Hooks(const Hooks&) = delete;
    Hooks& operator=(const Hooks&) = delete;
    Hooks(Hooks&& other) noexcept;
    Hooks& operator=(Hooks&& other) noexcept;

    /**
     * @brief Настроить существующий объект хуков без копирования/перемещения.
     * Используется когда объект объявлен как глобальный и нужно задать адрес позже.
     */
    void Set(uintptr_t targetAddress, TFunc hookFunction) {
        if (m_installed) {
            logger::warn("Hooks: Attempt to reconfigure already installed hook (address {:X})", m_targetAddress);
            return;
        }
        m_targetAddress = targetAddress;
        m_hookFunction = reinterpret_cast<void*>(hookFunction);
    }

    /**
     * @brief Устанавливает хук.
     * @return true, если хук успешно установлен, иначе false.
     */
    virtual bool Install();

    /**
     * @brief Удаляет хук.
     * @return true, если хук успешно удалён, иначе false.
     */
    virtual bool Uninstall();

    /**
     * @brief Проверяет, установлен ли хук.
     * @return true, если хук установлен, иначе false.
     */
    virtual bool IsInstalled() const noexcept { return m_installed; }

    /**
     * @brief Получить оригинальную функцию (до хука).
     * @return Указатель на оригинальную функцию.
     */
    TFunc GetOriginal() const
    {
        return m_originalFunction;
    }

private:
	std::string m_basePlugin{};         ///< Имя оригинального плагина, если используется относительный адрес
    uintptr_t m_offset = 0;             ///< Смещение относительно базового плагина, если используется относительный адрес
    uintptr_t m_targetAddress = 0;      ///< Адрес оригинальной функции
    std::function<uintptr_t()> 
        m_getAddress{ nullptr };        ///< Функция для получения адреса, если используется относительный адрес
    void* m_hookFunction = nullptr;     ///< Адрес функции-хука
    DetourXS m_detour;                  ///< Объект detour
    TFunc m_originalFunction = nullptr; ///< Оригинальная функция (trampoline)
    bool m_installed = false;           ///< Флаг: установлен ли хук
};

uintptr_t GetModule(const std::string& pluginName);

template<class TFunc>
inline Hooks<TFunc>::Hooks(uintptr_t targetAddress, TFunc hookFunction)
    : m_targetAddress(targetAddress), m_hookFunction(reinterpret_cast<void*>(hookFunction)), m_originalFunction(nullptr), m_installed(false)
{
	if (m_targetAddress)
        logger::info("Hooks: Creating hook for TFunc {} at address {:X}", typeid(TFunc).name(), targetAddress);
}

template<class TFunc>
inline Hooks<TFunc>::Hooks(const std::string& basePlugin, uintptr_t offsetAddress, TFunc hookFunction)
	: m_hookFunction(reinterpret_cast<void*>(hookFunction)), m_originalFunction(nullptr), m_installed(false), m_basePlugin(basePlugin), m_offset(offsetAddress)
{
	logger::info("Hooks: Creating hook for TFunc {} in plugin {}", typeid(TFunc).name(), basePlugin);
	// Если хук задан как глобальная переменная - плагин ещё не будет загружен, поэтому адреса нет.
    if (auto basePluginAddress = GetModule(m_basePlugin); basePluginAddress)
        m_targetAddress = GetModule(m_basePlugin) + m_offset;
}

template<class TFunc>
inline Hooks<TFunc>::Hooks(const std::function<uintptr_t()>& getAddress, TFunc hookFunction) :
    m_getAddress(getAddress), m_hookFunction(reinterpret_cast<void*>(hookFunction)), 
    m_originalFunction(nullptr), m_installed(false)
{
    logger::info("Hooks: Creating hook for TFunc {}", typeid(TFunc).name());
    // Если хук задан как глобальная переменная - плагин ещё не будет загружен, поэтому адреса нет.
}

/**
 * @brief Конструктор по умолчанию.
 */
template<class TFunc>
inline Hooks<TFunc>::Hooks()
    : m_targetAddress(0), m_hookFunction(nullptr), m_originalFunction(nullptr), m_installed(false)
{
    // Можно добавить логирование, если нужно
}

template <class TFunc>
inline bool Hooks<TFunc>::Install()
{
    // Если хук задан как глобальная переменная - плагин ещё не будет загружен, поэтому адреса к этому моменту ещё нет.
    if (!m_targetAddress) {
        if (m_getAddress) {
            m_targetAddress = m_getAddress();
            if (!m_targetAddress) {
                logger::error("Hooks: Invalid TFunc {} address (nullptr) for detour!", typeid(TFunc).name());
                m_installed = false;
                return false;
            }
        }
        else {
            if (auto basePluginAddress = GetModule(m_basePlugin); basePluginAddress)
                m_targetAddress = GetModule(m_basePlugin) + m_offset;
            if (!m_targetAddress) {
                logger::error("Hooks: Invalid TFunc {} address (nullptr) for detour!", typeid(TFunc).name());
                m_installed = false;
                return false;
            }
        }
    }
    if (!m_hookFunction) {
        logger::error("Hooks: Invalid TFunc {} hook function pointer (nullptr)!", typeid(TFunc).name());
        m_installed = false;
        return false;
    }
    // Пытаемся создать detour для указанной функции
    if (!m_detour.Create(reinterpret_cast<LPVOID>(m_targetAddress), m_hookFunction)) {
        logger::error("Hooks: Failed to create detour at address {:X}", m_targetAddress);
        m_installed = false;
        return false;
    }
    // Сохраняем оригинальный указатель на функцию
    m_originalFunction = reinterpret_cast<TFunc>(m_detour.GetTrampoline());
    logger::info("Hooks: Detour {} installed at address {:X}", typeid(TFunc).name(), m_targetAddress);
    m_installed = true;
    return true; 
}
        
template <class TFunc>
bool Hooks<TFunc>::Uninstall()
{
    if (!m_installed || !m_targetAddress) {
        logger::warn("Hooks: Uninstall called but hook is not installed or address is invalid!");
        return false;
    }
    if (!m_detour.Destroy()) {
        logger::error("Hooks: Failed to remove detour at address {:X}", m_targetAddress);
        return false;
    }
    m_installed = false;
    m_originalFunction = nullptr;
    logger::info("Hooks: Detour uninstalled at address {:X}", m_targetAddress);
    return true;
}

template<class TFunc>
inline Hooks<TFunc>::Hooks(Hooks&& other) noexcept
    : m_basePlugin(std::move(other.m_basePlugin)),
      m_offset(other.m_offset),
      m_targetAddress(other.m_targetAddress),
      m_getAddress(std::move(other.m_getAddress)),
      m_hookFunction(other.m_hookFunction),
      m_detour(), // DetourXS не перемещаем, создаём новый
      m_originalFunction(other.m_originalFunction),
      m_installed(other.m_installed)
{
    other.m_installed = false;
    other.m_originalFunction = nullptr;
}

template<class TFunc>
inline Hooks<TFunc>& Hooks<TFunc>::operator=(Hooks&& other) noexcept
{
    if (this != &other) {
        m_basePlugin = std::move(other.m_basePlugin);
        m_offset = other.m_offset;
        m_targetAddress = other.m_targetAddress;
        m_getAddress = std::move(other.m_getAddress);
        m_hookFunction = other.m_hookFunction;
        m_detour = DetourXS();
        m_originalFunction = other.m_originalFunction;
        m_installed = other.m_installed;
        other.m_installed = false;
        other.m_originalFunction = nullptr;
    }
    return *this;
}