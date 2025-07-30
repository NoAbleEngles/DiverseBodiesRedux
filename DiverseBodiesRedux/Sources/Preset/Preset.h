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
#include <future>
#include "Details/PresetEnums.h"
#include "ValidateOverlay/ValidateOverlay.h"
#include "LooksMenu/LooksMenuInterfaces.h"

class BodymorphsPreset;
class BodyhairsPreset;

/**
 * @brief Абстрактный базовый класс для всех пресетов.
 * 
 * Определяет интерфейс для работы с пресетами (загрузка, сравнение, применение и т.д.).
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
	 * @brief Проверить существуют ли условия.
	 * вернёт false даже если задан только пол.
	 */
	virtual bool isCondtionsEmpty() const noexcept;

	/**
	 * @brief Проверить валиден ли пресет для актёра.
	 * @param actor Указатель на актера.
	 * @return возвращает CoincidenceLevel - enum отражающий уровень совпадения. 0 (NONE) - не соответствует условиям, ABSOLUTE - совпадение по formId или editorId. Проваленная проверка пола всегда вернёт NONE, не продолжая других проверок. Совпадение по FormId или EditorID всегда вернут true, не доходя до прочих проверок. KEYWORDS и FACTIONS имеют по одному флагу для has и hasNot, но проваленная проверка hasNot всё равно вернёт NONE, даже если есть совпадение по has.
	 */
	virtual CoincidenceLevel check(const RE::Actor* actor, Filter filter = AllFilters) const noexcept;

	/**
	 * @brief Применить пресет к актеру.
	 * @param actor Указатель на актера.
	 * @param reset3d Если true, сбрасывает 3D актёра после применения пресетов.
	 * @return true, если успешно.
	 */
	virtual bool apply(RE::Actor* actor, bool reset3d = true) const = 0;

	/**
	 * @brief Удалить эффекты пресета с актера.
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

	Preset(const Preset& other) = delete;
	Preset& operator=(const Preset& other) = delete;
	Preset(Preset&& other) = delete;
	Preset& operator=(Preset&& other) = delete;
};

class OverlayPreset;
class BodymorphsPreset;
class BodyhairsPreset;

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

/**
 * @brief Обход всех оверлеев актёра и получение уникальных id по названиям оверлеев.
 * @tparam actor указатель на актёра.
 * @param preset вектор с id пресетов, уникальные id которых необходимо получить.
 * @return вектор с найденными id.
 */
template <typename Container>
inline std::vector<uint32_t> findOverlaysUid(RE::Actor* actor, const Container& overlayIds) {
	using ValueType = typename Container::value_type;
	static_assert(std::is_same_v<ValueType, std::string>,
		"Container must contain std::string.");

	std::vector<uint32_t> result{};

	if (!actor) {
		logger::error("Actor is null in findOverlayUid.");
		return result;
	}

	if (overlayIds.empty()) {
		logger::error("Overlay IDs are empty in findOverlayUid.");
		return result;
	}

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::critical("OverlayInterface is nullptr!");
		return result;
	}

	Interface->ForEachOverlay(
		actor,
		actor->GetSex() == RE::Actor::Female,
		[&result, &overlayIds](int32_t uid, const OverlayInterface::OverlayDataPtr& overlay) {
			if (overlay && overlay->templateName) {
				for (auto overlayId : overlayIds) {
					if (strcmp(overlay->templateName.get()->c_str(), overlayId.c_str()) == 0) {
						result.push_back(overlay->uid);
					}
				}
			}
		}
	);

	return result;
}