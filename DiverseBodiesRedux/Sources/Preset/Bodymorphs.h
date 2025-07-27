#pragma once
#include "Preset.h"

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
	CoincidenceLevel check(const RE::Actor* actor, Filter filter = AllFilters) const noexcept override;

	/// @copydoc Preset::isCondtionsEmpty
	bool isCondtionsEmpty() const noexcept override;

	/// @copydoc Preset::apply
	bool apply(RE::Actor*, bool reset3d = true) const override;

	/// @copydoc Preset::remove
	bool remove(RE::Actor*) const override;

	/// @copydoc Preset::empty
	bool empty() const noexcept override;

	/// @copydoc Preset::clear
	void clear() noexcept override;

	/// @copydoc Preset::clone
	BodymorphsPreset* clone() const override;

	/// @copydoc Preset::name
	const std::string& id() const noexcept override;

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;

	/// @copydoc Preset::print
	std::string print() const override;

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