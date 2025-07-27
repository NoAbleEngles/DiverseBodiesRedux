#pragma once
#include "Preset.h"

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
	BodyhairsPreset* clone() const override;

	/// @copydoc Preset::name
	const std::string& id() const noexcept override;

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;

	/// @copydoc Preset::print
	std::string print() const override;

	/** 
	* @brief Получить все возможные оверлеи данного типа для мужского актёра.
	* @return Множество строковых представлений идентификаторов оверлеев.
	**/
	static const std::set<std::string> getAllPossibleMaleOverlays() noexcept;

	/**
	* @brief Получить все возможные оверлеи данного типа для женского актёра.
	* @return Множество строковых представлений идентификаторов оверлеев.
	**/
	static const std::set<std::string> getAllPossibleFemaleOverlays() noexcept;

	static void revalidateAllPossibleOverlays(const std::set<std::string>& AllValidOverlays);
private:

	/**
	 * @brief Морфы тела (имя морфа -> значение).
	 */
	std::vector<Overlay> m_overlays;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу мужского актёра.
	static std::set<std::string> ALL_ITEMS_M;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу женского актёра.
	static std::set<std::string> ALL_ITEMS_F;

	/// @copydoc Preset::loadFromFile
	bool loadFromFile(const std::string& presetFile) override;

	/**
	 * @brief Валидация оверлеев.
	 */
	ValidateOverlay& validate = ValidateOverlay::validateOverlay();
};