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
	 * @brief Морфы тела (имя морфа -> значение).
	 */
	std::unordered_map<std::string, float> m_morphs;
	RE::NiPoint3 m_morphWeight{};

	/// @copydoc Preset::loadFromFile
	bool loadFromFile(const std::string& presetFile) override;

	BodymorphsPreset(const BodymorphsPreset& other) = delete;
	BodymorphsPreset& operator=(const BodymorphsPreset& other) = delete;
	BodymorphsPreset(BodymorphsPreset&& other) = delete;
	BodymorphsPreset& operator=(BodymorphsPreset&& other) = delete;

	mutable std::set<uint32_t> m_processingActors;  ///< Защита от двойной обработки актёра.
	mutable std::shared_mutex m_processingActorsMtx;  ///< Мьютекс для потокобезопасного доступа к m_processingActors.
};

class ResetBodyMorphs : public F4SE::ITaskDelegate
{
public:
	ResetBodyMorphs(RE::Actor* actor, std::function<void()> doAfterAll,  bool doDetach, bool doReset) noexcept;
	virtual ~ResetBodyMorphs();
	virtual void Run() override;

protected:
	RE::Actor* m_actor{nullptr};
	bool m_doDetach;
	bool m_doReset;
	std::function<void()> m_doAfterAll{ nullptr };
};

/**
 * @brief F4SE-таск: выполняет Actor::Reset3D с заданными флагами.
 *
 * Используется для HEAD-reset после Detach оверлеев в ResetBodyMorphs::Run().
 * Постановка через AddTask гарантирует FIFO-порядок внутри F4SE-очереди:
 * сначала ResetBodyMorphs (Detach), затем ResetActor (Reset3D).
 */
class ResetActor : public F4SE::ITaskDelegate
{
public:
	/**
	 * @param actor         Актёр для сброса. Должен оставаться валидным к моменту Run().
	 * @param flags         additionalFlags для Reset3D (что перестроить).
	 * @param excludeFlags  excludeFlags для Reset3D (что не трогать).
	 * @param doAfterAll    Коллбэк после выполнения (освобождение гардов).
	 */
	ResetActor(RE::Actor* actor, RE::RESET_3D_FLAGS flags, RE::RESET_3D_FLAGS excludeFlags, std::function<void()> doAfterAll) noexcept;
	virtual ~ResetActor() = default;
	virtual void Run() override;

protected:
	RE::Actor* m_actor{ nullptr };
	RE::RESET_3D_FLAGS m_flags;
	RE::RESET_3D_FLAGS m_excludeFlags;
	std::function<void()> m_doAfterAll{ nullptr };
};