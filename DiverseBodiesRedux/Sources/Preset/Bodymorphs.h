#pragma once
#include "Preset.h"

/**
 * @brief Пресет морфов тела для актёров.
 *
 * Применяет морфы BodyGen к телу актёра через LooksMenu интерфейс.
 * Включает защиту от двойной обработки и автоматическую очистку предыдущих морфов.
 * Поддерживает условия применения по полу и другим параметрам актёра.
 */
class BodymorphsPreset final : public Preset
{
public:
	static constexpr PresetType PRESET_TYPE = PresetType::BODYMORPHS;

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
	 * @brief Деструктор.
	 */
	~BodymorphsPreset() = default;

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

	/**
	 * @brief Получить реальный тип класса.
	 * @return PresetType класса.
	 */
	PresetType type() const noexcept override;

	/// @copydoc Preset::check
	CoincidenceLevel check(const RE::Actor* actor, Filter filter = AllFilters) const noexcept override;

	/// @copydoc Preset::apply
	bool apply(RE::Actor*, bool reset3d = true) const override;

	/// @copydoc Preset::remove
	bool remove(RE::Actor*) const override;

	/// @copydoc Preset::empty
	bool empty() const noexcept override;

	/// @copydoc Preset::clear
	void clear() noexcept override;

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

	BodymorphsPreset(const BodymorphsPreset& other) = delete;
	BodymorphsPreset& operator=(const BodymorphsPreset& other) = delete;
	BodymorphsPreset(BodymorphsPreset&& other) = delete;
	BodymorphsPreset& operator=(BodymorphsPreset&& other) = delete;
};