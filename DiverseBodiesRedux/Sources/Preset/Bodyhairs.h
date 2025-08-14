#pragma once
#include "OverlayPreset.h"
#include <set>

/**
 * @brief Пресет волос для тела актёров.
 * 
 * Наследуется от OverlayPreset и применяет текстурные оверлеи волос на тело актёров
 * через LooksMenu интерфейс. Включает защиту от двойной обработки и валидацию оверлеев.
 */
class BodyhairsPreset final : public OverlayPreset
{
public:
	static constexpr PresetType PRESET_TYPE = PresetType::BODYHAIRS;

	/**
	 * @copydoc OverlayPreset::OverlayPreset
	 */
	BodyhairsPreset();

	/**
	 * @copydoc OverlayPreset::OverlayPreset
	 **/ 
	BodyhairsPreset(const std::filesystem::path& path);

	/**
	 * @brief Конструктор из строки пути.
	 * @param path Строка пути к файлу пресета.
	 */
	BodyhairsPreset(const std::string& path);

	/**
	 * @brief Деструктор.
	 */
	~BodyhairsPreset() = default;

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

	/// @copydoc OverlayPreset::clear
	bool remove(RE::Actor* actor) const override;

	/// @copydoc Preset::apply
	bool apply(RE::Actor*, bool reset3d = true) const override;

	/**
	* @brief Добавляет все оверлеи из этого пресета в ALL_ITEMS_M и ALL_ITEMS_F. После всех добавлений нужно ревалидировать ALL_ITEMS_M и ALL_ITEMS_F с помощью revalidateAllPossibleOverlays. Метод создан для использования в конструкторе при создании объекта.
	**/
	void addOverlaysFromThisToPossibleOverlays();

	/// @copydoc Preset::isValidAsync
	std::future<bool> isValidAsync() const noexcept override;
private:
	BodyhairsPreset(const BodyhairsPreset& other) = delete;
	BodyhairsPreset& operator=(const BodyhairsPreset& other) = delete;
	BodyhairsPreset(BodyhairsPreset&& other) = delete;
	BodyhairsPreset& operator=(BodyhairsPreset&& other) = delete;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу мужского актёра.
	static std::set<std::string> ALL_ITEMS_M;

	/// @brief Переменная, которая хранит абсолютно все пресеты, которые могут быть применены к телу женского актёра.
	static std::set<std::string> ALL_ITEMS_F;
};