#pragma once
#include "Details/Conditions.h"
#include "Details/Overlay.h"
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include "Details/PresetEnums.h"
#include "LooksMenu/LooksMenuInterfaces.h"

class BodymorphsPreset;
class BodyhairsPreset;
class BodyTattoosPreset;
class NailsPreset;
class NPCPreset;

/**
 * @brief Абстрактный базовый класс для всех пресетов.
 * 
 * Определяет общий интерфейс для работы с пресетами различных типов: загрузка из файлов,
 * сравнение, применение к актёрам, проверка условий и валидация. Все конкретные типы 
 * пресетов (морфы тела, оверлеи, тинты лица) наследуются от этого класса.
 * Включает встроенную защиту от двойной обработки одного и того же актёра.
 */
class Preset
{
public:
	static constexpr PresetType PRESET_TYPE = PresetType::NONE;

	/**
	 * @brief Конструктор по умолчанию.
	 */
	Preset() = default;

	/**
	 * @brief Конструктор с указанием id пресета.
	 * @param id Уникальный идентификатор пресета, обычно соответствует имени файла без расширения.
	 */
	Preset(const std::string& id);

	/**
	 * @brief Виртуальный деструктор.
	 */
	virtual ~Preset() = 0;

	/**
	 * @brief Получить тип пресета.
	 * @return Тип пресета.
	 */
	virtual PresetType type() const noexcept = 0;

	/**
	 * @brief Сравнение на равенство с другим пресетом.
	 * @param other Другой пресет.
	 * @return true, если пресеты равны.
	 */
	 virtual bool operator==(const Preset& other) const noexcept = 0;

	/**
	 * @brief Оператор "меньше" для сортировки.
	 * @param other Другой пресет.
	 * @return true, если этот пресет меньше другого.
	 */
	virtual bool operator<(const Preset& other) const noexcept;

	/**
	 * @brief Проверить валиден ли пресет для актёра.
	 * @param actor Указатель на актера.
	 * @return возвращает CoincidenceLevel - enum отражающий уровень совпадения. 0 (NONE) - не соответствует условиям, ABSOLUTE - совпадение по formId или editorId. Проваленная проверка пола всегда вернёт NONE, не продолжая других проверок. Совпадение по FormId или EditorID всегда вернут true, не доходя до прочих проверок. KEYWORDS и FACTIONS имеют по одному флагу для has и hasNot, но проваленная проверка hasNot всё равно вернёт NONE, даже если есть совпадение по has.
	 */
	virtual CoincidenceLevel check(const RE::Actor* actor, Filter filter = AllFilters) const noexcept;

	/**
	 * @brief Применить пресет к актеру с защитой от двойной обработки.
	 * @param actor Указатель на актера.
	 * @param reset3d Если true, сбрасывает 3D актёра после применения пресетов.
	 * @return true, если успешно.
	 */
	virtual bool apply(RE::Actor* actor, bool reset3d = true) const = 0;

	/**
	 * @brief Удалить эффекты пресета с актера с защитой от двойной обработки.
	 * @param actor Указатель на актера.
	 * @return true, если успешно.
	 */
	virtual bool remove(RE::Actor* actor) const = 0;

	/**
	 * @brief Проверить, пустой ли пресет.
	 * @return true, если пресет пустой.
	 */
	virtual bool empty() const noexcept = 0;

	/**
	 * @brief Очистить содержимое пресета.
	 */
	virtual void clear() noexcept = 0;

	/**
	 * @brief Получить имя пресета.
	 * @return Имя пресета.
	 */
	virtual const std::string& id() const noexcept;

	/**
	 * @brief Узнать является ли пресет валидным (не пустой, доступны ресурсы на которые пресет ссылается). Ассинхронный метод.
	 * @return булево значение, true если пресет валиден.
	 */
	virtual std::future<bool> isValidAsync() const noexcept;

	/**
	 * @brief Печать информации о пресете в строку.
	 * @return Строка с информацией о пресете.
	 */
	virtual std::string print() const = 0;

protected:
	/**
	 * @brief Условия применения пресета (пол, раса и т.д.).
	 */
	ConditionSettings m_conditions{};

	/**
	 * @brief Загрузить пресет из файла. Сделан только для вызова из конструктора от m_id.
	 * @param presetFile Путь к файлу.
	 * @return true, если успешно.
	 */
	virtual bool loadFromFile(const std::string& presetFile) = 0;

	/**
	 * @brief Уникальный идентификатор пресета. Соответствует имени файла.
	 */
	std::string m_id{};

	/**
	 * @brief Загружает условия из JSON-файла.
	 */
	void LoadConditions(const std::string& path);

	/**
	 * @brief Загружает условия из JSON-файла.
	 */
	void LoadConditions(const std::filesystem::path& path);

	/**
	 * @brief Загружает условия из JSON-объекта.
	 */
	void LoadConditions(boost::json::object& jsonConditionObject);

	/**
	 * @brief Утилитарный класс для защиты от двойной обработки одного актёра.
	 */
	class ProcessingGuard {
	public:
		ProcessingGuard(RE::Actor* actor, std::unordered_set<RE::Actor*>& processingSet) 
			: m_actor(actor), m_processingSet(processingSet), m_isActive(false) {
			if (actor && !m_processingSet.contains(actor)) {
				m_processingSet.insert(actor);
				m_isActive = true;
			}
		}

		~ProcessingGuard() {
			if (m_isActive && m_actor) {
				m_processingSet.erase(m_actor);
			}
		}

		bool isActive() const { return m_isActive; }

		// Запрещаем копирование и перемещение
		ProcessingGuard(const ProcessingGuard&) = delete;
		ProcessingGuard& operator=(const ProcessingGuard&) = delete;
		ProcessingGuard(ProcessingGuard&&) = delete;
		ProcessingGuard& operator=(ProcessingGuard&&) = delete;

	private:
		RE::Actor* m_actor;
		std::unordered_set<RE::Actor*>& m_processingSet;
		bool m_isActive;
	};

	Preset(const Preset& other) = delete;
	Preset& operator=(const Preset& other) = delete;
	Preset(Preset&& other) = delete;
	Preset& operator=(Preset&& other) = delete;
};

/**
 * @brief Загружает содержимое JSON-файла в строку.
 * @param filepath Путь к JSON-файлу.
 * @return Строка с содержимым файла.
 */
std::string getJson(const std::filesystem::path& filepath);

// Концепт: P и T — наследники Preset, P — указатель или ссылка
template <typename T, typename P>
concept PresetCastable =
std::derived_from<std::remove_pointer_t<std::remove_reference_t<T>>, Preset>&&
std::derived_from<std::remove_pointer_t<std::remove_reference_t<P>>, Preset>;

/**
 * @brief Безопасное приведение Preset* к нужному типу-наследнику.
 * @tparam T Целевой тип (например, BodymorphsPreset).
 * @param preset Указатель на Preset.
 * @return Указатель на T или nullptr.
 */
template <typename T, typename P>
	requires PresetCastable<T, P>&& std::is_pointer_v<P>&& std::is_pointer_v<T>
inline T preset_cast(P preset)
{
	if (preset && preset->type() == std::remove_pointer_t<T>::PRESET_TYPE) {
		using BasePtr = std::conditional_t<
			std::is_const_v<std::remove_pointer_t<T>>,
			const Preset*,
			Preset*
		>;
		return static_cast<T>(const_cast<BasePtr>(preset));
	}
	else {
		return nullptr;
	}
}

template <typename T>
inline T preset_cast(auto* preset)
{
	return preset_cast<T, decltype(preset)>(preset);
}