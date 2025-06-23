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

/**
 * @brief Абстрактный базовый класс для всех пресетов.
 * 
 * Определяет интерфейс для работы с пресетами (загрузка, сравнение, применение и т.д.).
 */
class Preset
{
public:
	/**
	 * @brief Конструктор по умолчанию.
	 */
	Preset() = default;

	/**
	 * @brief Конструктор с указанием типа пресета.
	 * @param type Тип пресета.
	 */
	Preset(std::string filepath, PresetType type);

	/**
	 * @brief Копирующий конструктор.
	 */
	Preset(const Preset& other) noexcept = default;

	/**
	 * @brief Перемещающий конструктор.
	 */
	Preset(Preset&& other) noexcept = default;

	/**
	 * @brief Виртуальный деструктор.
	 */
	virtual ~Preset() = 0;

	/**
	 * @brief Получить тип пресета.
	 * @return Тип пресета.
	 */
	PresetType type() const noexcept;

	/**
	 * @brief Сравнение на равенство с другим пресетом.
	 * @param other Другой пресет.
	 * @return true, если пресеты равны.
	 */
	virtual bool operator==(const Preset& other) const noexcept = 0;

	/**
	 * @brief Оператор присваивания.
	 * @param other Другой пресет.
	 * @return Ссылка на этот объект.
	 */
	virtual Preset& operator=(const Preset& other) noexcept = 0;

	/**
	 * @brief Оператор перемещающего присваивания.
	 * @param other Другой пресет.
	 * @return Ссылка на этот объект.
	 */
	virtual Preset& operator=(Preset&& other) noexcept = 0;

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
	virtual CoincidenceLevel check(const RE::Actor* actor) const noexcept;

	/**
	 * @brief Применить пресет к актеру.
	 * @param actor Указатель на актера.
	 * @return true, если успешно.
	 */
	virtual bool apply(RE::Actor* actor) const = 0;

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
	virtual void clear() noexcept;

	/**
	 * @brief Клонировать пресет.
	 * @return Указатель на новый пресет, созданный на основе текущего.
	 */
	virtual Preset* clone() const = 0;

	/**
	 * @brief Получить имя пресета.
	 * @return Имя пресета.
	 */
	virtual std::string id() const noexcept = 0;

	/**
	 * @brief Узнать является ли пресет валидным (не пустой, доступны ресурсы на которые пресет ссылается). Ассинхронный метод.
	 * @return булево значение, true если пресет валиден.
	 */
	virtual std::future<bool> isValidAsync() const noexcept;

protected:
	/**
	 * @brief Условия применения пресета (пол, раса и т.д.).
	 */
	ConditionSettings m_conditions{};

private:

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
	 * @brief Тип пресета.
	 */
	PresetType m_type{ PresetType::NONE };
};

/**
 * @brief Пресет морфов тела (BodyMorph).
 * 
 * Содержит морфы, имя файла, тип тела и методы для применения к актеру.
 */
class BodymorphsPreset : public Preset
{
public:
	/**
	 * @brief Конструктор по умолчанию.
	 */
	BodymorphsPreset();

	/**
	 * @brief Конструктор из пути к файлу.
	 * @param path Путь к файлу пресета.
	 */
	BodymorphsPreset(const std::filesystem::path& path);

	/**
	 * @brief Конструктор из строки пути.
	 * @param path Строка пути к файлу пресета.
	 */
	BodymorphsPreset(const std::string& path);

	/**
	 * @brief Копирующий конструктор.
	 */
	BodymorphsPreset(const BodymorphsPreset& other) noexcept = default;

	/**
	 * @brief Перемещающий конструктор.
	 */
	BodymorphsPreset(BodymorphsPreset&& other) noexcept = default;

	/**
	 * @brief Деструктор.
	 */
	~BodymorphsPreset() = default;

	/// @copydoc Preset::operator=
	Preset& operator=(const Preset& other) noexcept override;

	/// @copydoc Preset::operator=
	Preset& operator=(Preset&& other) noexcept override;

	/**
	 * @brief Оператор присваивания для BodymorphsPreset.
	 */
	BodymorphsPreset& operator=(const BodymorphsPreset& other) noexcept;

	/**
	 * @brief Оператор перемещающего присваивания для BodymorphsPreset.
	 */
	BodymorphsPreset& operator=(BodymorphsPreset&& other) noexcept;

	/// @copydoc Preset::operator==
	bool operator==(const Preset& other) const noexcept override;

	/**
	 * @brief Сравнение на равенство с другим BodymorphsPreset.
	 */
	bool operator==(const BodymorphsPreset& other) const noexcept;

	/// @copydoc Preset::operator<
	bool operator<(const Preset& other) const noexcept override;

	/**
	 * @brief Оператор "меньше" для сортировки BodymorphsPreset.
	 */
	bool operator<(const BodymorphsPreset& other) const noexcept;

	/// @copydoc Preset::check
	CoincidenceLevel check(const RE::Actor* actor) const noexcept override;

	/// @copydoc Preset::isCondtionsEmpty
	bool isCondtionsEmpty() const noexcept override;

	/// @copydoc Preset::apply
	bool apply(RE::Actor*) const override;

	/// @copydoc Preset::remove
	bool remove(RE::Actor*) const override;

	/// @copydoc Preset::empty
	bool empty() const noexcept override;

	/// @copydoc Preset::clear
	void clear() noexcept override;

	/// @copydoc Preset::clone
	BodymorphsPreset* clone() const override;

	/// @copydoc Preset::name
	std::string id() const noexcept override;

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;

private:

	/**
	 * @brief Тип тела, определяемый пресетом.
	 */
	BodyType m_bodytype;

	/**
	 * @brief Морфы тела (имя морфа -> значение).
	 */
	std::unordered_map<std::string, float> m_morphs;

	/// @copydoc Preset::loadFromFile
	bool loadFromFile(const std::string& presetFile) override;
};

/**
 * @brief Пресет волос для тела (BodyMorph).
 */
class BodyhairsPreset : public Preset
{
public:
	/**
	 * @brief Конструктор по умолчанию.
	 */
	BodyhairsPreset();

	/**
	 * @brief Конструктор из пути к файлу.
	 * @param path Путь к файлу пресета.
	 */
	BodyhairsPreset(const std::filesystem::path& path);

	/**
	 * @brief Конструктор из строки пути.
	 * @param path Строка пути к файлу пресета.
	 */
	BodyhairsPreset(const std::string& path);

	/**
	 * @brief Копирующий конструктор.
	 */
	BodyhairsPreset(const BodyhairsPreset& other) noexcept = default;

	/**
	 * @brief Перемещающий конструктор.
	 */
	BodyhairsPreset(BodyhairsPreset&& other) noexcept = default;

	/**
	 * @brief Деструктор.
	 */
	~BodyhairsPreset() = default;

	/// @copydoc Preset::operator=
	Preset& operator=(const Preset& other) noexcept override;

	/// @copydoc Preset::operator=
	Preset& operator=(Preset&& other) noexcept override;

	/**
	 * @brief Оператор присваивания для BodyhairsPreset.
	 */
	BodyhairsPreset& operator=(const BodyhairsPreset& other) noexcept;

	/**
	 * @brief Оператор перемещающего присваивания для BodyhairsPreset.
	 */
	BodyhairsPreset& operator=(BodyhairsPreset&& other) noexcept;

	/// @copydoc Preset::operator==
	bool operator==(const Preset& other) const noexcept override;

	/**
	 * @brief Сравнение на равенство с другим BodyhairsPreset.
	 */
	bool operator==(const BodyhairsPreset& other) const noexcept;

	/// @copydoc Preset::operator<
	bool operator<(const Preset& other) const noexcept override;

	/**
	 * @brief Оператор "меньше" для сортировки BodyhairsPreset.
	 */
	bool operator<(const BodyhairsPreset& other) const noexcept;

	/// @copydoc Preset::check
	CoincidenceLevel check(const RE::Actor* actor) const noexcept override;

	/// @copydoc Preset::isCondtionsEmpty
	bool isCondtionsEmpty() const noexcept override;

	/// @copydoc Preset::apply
	bool apply(RE::Actor*) const override;

	/// @copydoc Preset::remove
	bool remove(RE::Actor*) const override;

	/// @copydoc Preset::empty
	bool empty() const noexcept override;

	/// @copydoc Preset::clear
	void clear() noexcept override;

	/// @copydoc Preset::clone
	BodyhairsPreset* clone() const override;

	/// @copydoc Preset::name
	std::string id() const noexcept override;

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;

private:

	/**
	 * @brief Морфы тела (имя морфа -> значение).
	 */
	std::vector<Overlay> m_overlays;
	std::vector<std::string> m_overlaysToRemove;

	/// @copydoc Preset::loadFromFile
	bool loadFromFile(const std::string& presetFile) override;

	/**
	 * @brief Валидация оверлеев.
	 */
	ValidateOverlay& validate = ValidateOverlay::validateOverlay();
};

/**
 * @brief Загружает содержимое JSON-файла в строку.
 * @param filepath Путь к JSON-файлу.
 * @return Строка с содержимым файла.
 */
std::string getJson(const std::filesystem::path& filepath);

/**
 * @brief Безопасное приведение Preset* к нужному типу-наследнику.
 * @tparam T Целевой тип (например, BodymorphsPreset).
 * @param preset Указатель на Preset.
 * @return Указатель на T или nullptr.
 */
template <typename T>
T* preset_cast(Preset* preset)
{
	if (!preset) {
		return nullptr;
	}
	if constexpr (std::is_same_v<T, BodymorphsPreset>) {
		if (preset->type() == PresetType::BODYMORPHS) {
			return static_cast<T*>(preset);
		}
	}
	// Добавьте другие типы при необходимости, например OverlayPreset
	// else if constexpr (std::is_same_v<T, OverlayPreset>) {
	//     if (preset->type() == PresetType::OVERLAY) {
	//         return static_cast<T*>(preset);
	//     }
	// }
	return nullptr;
}

/**
 * @brief Безопасное приведение Preset& к нужному типу-наследнику.
 * @tparam T Целевой тип (например, BodymorphsPreset).
 * @param preset Ссылка на Preset.
 * @return Ссылка на T.
 * @throws std::bad_cast если тип не совпадает.
 */
template <typename T>
T& preset_cast(Preset& preset)
{
	if constexpr (std::is_same_v<T, BodymorphsPreset>) {
		if (preset.type() == PresetType::BODYMORPHS) {
			return static_cast<T&>(preset);
		}
	}
	// Добавьте другие типы при необходимости, например OverlayPreset
	// else if constexpr (std::is_same_v<T, OverlayPreset>) {
	//     if (preset.type() == PresetType::OVERLAY) {
	//         return static_cast<T&>(preset);
	//     }
	// }
	throw std::bad_cast();
}

/**
 * @brief Обход всех оверлеев актёра и получение уникальных id по названиям оверлеев.
 * @tparam actor указатель на актёра.
 * @param preset вектор с id пресетов, уникальные id которых необходимо получить.
 * @return вектор с найденными id.
 */
std::vector<uint32_t> findOverlaysUid(RE::Actor* actor, const std::vector<std::string>& overlayIds);