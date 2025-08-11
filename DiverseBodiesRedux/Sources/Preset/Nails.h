#pragma once
#include "OverlayPreset.h"

/**
 * @brief Пресет волос для ногтей (NailsPreset).
 */
class NailsPreset final : public OverlayPreset
{
public:
	static constexpr PresetType PRESET_TYPE = PresetType::NAILS;

	/**
	 * @copydoc OverlayPreset::OverlayPreset
	 */
	NailsPreset();

	/**
	 * @copydoc OverlayPreset::OverlayPreset
	 **/ 
	NailsPreset(const std::filesystem::path& path);

	/**
	 * @brief Конструктор из строки пути.
	 * @param path Строка пути к файлу пресета.
	 */
	NailsPreset(const std::string& path);

	/**
	 * @brief Деструктор.
	 */
	~NailsPreset() = default;

	bool operator==(const Preset& other) const noexcept override;

	bool operator<(const Preset& other) const noexcept override;
	
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

	/**
	 * @brief Получить реальный тип класса.
	 * @return PresetType класса.
	 */
	PresetType type() const noexcept override;

	/// @copydoc Preset::clear
	void clear() noexcept override;

	/// @copydoc OverlayPreset::clear
	bool remove(RE::Actor* actor) const override;

	/**
	* @brief Добавляет все оверлеи из этого пресета в ALL_ITEMS_M и ALL_ITEMS_F. После всех добавлений нужно ревалидировать ALL_ITEMS_M и ALL_ITEMS_F с помощью revalidateAllPossibleOverlays. Метод создан для использования в конструкторе при создании объекта.
	**/
	void addOverlaysFromThisToPossibleOverlays();

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;
private:
	NailsPreset(const NailsPreset& other) = delete;
	NailsPreset& operator=(const NailsPreset& other) = delete;
	NailsPreset(NailsPreset&& other) = delete;
	NailsPreset& operator=(NailsPreset&& other) = delete;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу мужского актёра.
	static std::set<std::string> ALL_ITEMS_M;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу женского актёра.
	static std::set<std::string> ALL_ITEMS_F;
};