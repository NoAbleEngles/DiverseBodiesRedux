#pragma once
#include <string>
#include <optional>
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <boost/json.hpp>
#include <ranges>
#include <type_traits>
#include <algorithm>
#include <vector>

namespace logger = F4SE::log;

using TintSlot = RE::BGSCharacterTint::EntrySlot;

// Forward declarations
class ParseLMPreset;
class MaskTint;
class PaletteTint;
class TextureSetTint;

namespace tints_memory {
    static auto& memoryManager = RE::MemoryManager::GetSingleton();

    template<typename T>
    inline void* Allocate(size_t count = 1) {
        return memoryManager.Allocate(sizeof(T) * count, alignof(T), true);
    }

    inline void Deallocate(void* ptr) {
        memoryManager.Deallocate(ptr, true);
    }

    template<typename T>
    inline void* AllocateScrapHeap(size_t count = 1) {
        return memoryManager.GetThreadScrapHeap()->Allocate(sizeof(T) * count, alignof(T));
    }

    inline void DeallocateScrapHeap(void* ptr) {
        memoryManager.GetThreadScrapHeap()->Deallocate(ptr);
    }
}

class Tint {
public:
    using ColorData = RE::BGSCharacterTint::Template::Palette::ColorData; ///< Тип данных для хранения цвета тинта
    using Color = RE::BGSCharacterTint::PaletteEntry::Color; ///< Тип данных для хранения цвета тинта в палитре

    enum class Type : int32_t {
        kNone = -1,
        kMask = 0,
        kPalette = 1,
        kTextureSet = 2
    };

    virtual Type getType() const noexcept = 0; // Делаем чисто виртуальным для абстрактности

    static RE::BGSCharacterTint::Entry* AddTint(RE::TESNPC* npc, const Tint& tint);
    static bool RemoveTint(RE::TESNPC* npc, uint32_t tintUniqueID);

    Tint(uint32_t uniqueID, const boost::json::object& jsonObject);
    Tint(const Tint&) noexcept = default; // Разрешаем копирование
    Tint(Tint&&) noexcept = default; // Разрешаем перемещение
    Tint(uint32_t uniqueID = 0, uint8_t percent = 255);
    virtual ~Tint() noexcept = 0; // Чистый виртуальный деструктор для абстрактности

	virtual uint32_t id() const noexcept;
    virtual uint8_t percent() const noexcept;

    virtual void setId(uint32_t id) noexcept;
    virtual void setPercent(uint8_t percent) noexcept;

    virtual bool empty() const noexcept;
    virtual void clear() noexcept;

    virtual bool apply(RE::TESNPC* npc) const;

    template<typename Container>
    static inline bool applyTints(RE::TESNPC* npc, const Container& tints);

	virtual bool remove(RE::TESNPC* npc) const;

    virtual Tint& operator=(const Tint&) noexcept = default; // Разрешаем оператор присваивания
    virtual Tint& operator=(Tint&&) noexcept = default; // Разрешаем оператор перемещения
    virtual bool operator==(const Tint&) const noexcept;
    virtual bool operator<(const Tint& other) const noexcept;
    
protected:
    uint32_t m_id{};            ///< Уникальный идентификатор наложения
    uint8_t m_percent{255};     ///< Процент видимости наложения (0-255, где 255 — полное наложение)
	RE::BGSCharacterTint::Template::Entry* m_templateEntry{ nullptr }; ///< Указатель на шаблонную запись тинта

	static RE::BGSCharacterTint::Entry* CreateTintEntry(const Tint& tint, RE::BGSCharacterTint::Template::Entry* templateEntry); ///< Создает запись тинта на основе типа и шаблонной записи

	static RE::BGSCharacterTint::Template::Entry* tryToFindTemplateEntry(uint32_t id); ///< Пытается найти шаблонную запись тинта по уникальному ID

    virtual bool loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept; ///< Загружает данные из JSON-объекта от пресета LooksMenu

    static bool updateTint(RE::BGSCharacterTint::Entry* entry, const Tint& tint);
};

class MaskTint : public Tint {
public:
    static constexpr Type TYPE{ Type::kMask }; ///< Тип наложения — маска

    MaskTint(uint32_t uniqueID, const boost::json::object& jsonObject);
    MaskTint(uint32_t uniqueID = 0, uint8_t percent = 255);
    ~MaskTint() noexcept override;

    bool operator==(const Tint&) const noexcept override;
    bool operator<(const Tint& other) const noexcept override;

    Type getType() const noexcept override;

protected:
    bool loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept override; ///< Загружает данные из JSON-объекта от пресета LooksMenu
};

class PaletteTint final : public Tint {
public:
    static constexpr Type TYPE{ Type::kPalette }; ///< Тип наложения — палитра

    PaletteTint(uint32_t uniqueID, const boost::json::object& jsonObject);
    PaletteTint(uint32_t uniqueID = 0, uint8_t percent = 255);
    ~PaletteTint() noexcept override;

    bool operator==(const Tint&) const noexcept override;
    bool operator<(const Tint& other) const noexcept override;

    Type getType() const noexcept override;

    Color getColor() const noexcept; ///< Получает цвет палитры
    void setColor(const Color& color) noexcept; ///< Устанавливает цвет палитры
    uint16_t colorID() const noexcept; ///< Получает ID цвета в палитре
    void setColorID(uint16_t colorID) noexcept; ///< Устанавливает ID цвета в палитре

protected:
    Color m_color{};
    uint16_t m_colorID{}; // ID Цвета в палитре

    bool loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept override; ///< Загружает данные из JSON-объекта от пресета LooksMenu
};

class TextureSetTint final : public Tint {
public:
    static constexpr Type TYPE{ Type::kTextureSet }; ///< Тип наложения — текстура

    TextureSetTint(uint32_t uniqueID, const boost::json::object& jsonObject);
    TextureSetTint(uint32_t uniqueID = 0, uint8_t percent = 255);
    ~TextureSetTint() noexcept override;

    Type getType() const noexcept override;

protected:

    bool loadFromJsonObject(uint32_t uniqueID, const boost::json::object& jsonObject) noexcept override; ///< Загружает данные из JSON-объекта от пресета LooksMenu
};

namespace std {
    template<>
    struct hash<Tint> {
        std::size_t operator()(const Tint& t) const noexcept {
            std::size_t h = std::hash<uint32_t>{}(t.id());
            return h;
        }
    };
}

template<typename Container>
static inline bool Tint::applyTints(RE::TESNPC* npc, const Container& tints) {
    // Проверяем, что Container является контейнером
    static_assert(std::ranges::range<Container>, "Container must be a valid range/container");

    if (!npc || tints.empty()) {
        logger::error("applyTints: Invalid NPC or empty tints container");
        return false;
    }

    if (!npc->tintingData) {
        logger::error("applyTints: NPC's tintingData is null");
        return false;
    }

    // Определяем тип элементов контейнера
    using ElementType = std::remove_cv_t<std::remove_reference_t<decltype(*std::begin(tints))>>;

    // Расширенная проверка совместимости типов включая полиморфные smart_ptr
    static_assert(
        std::is_base_of_v<Tint, ElementType> ||
        (std::is_pointer_v<ElementType> && std::is_base_of_v<Tint, std::remove_pointer_t<ElementType>>) ||
        // Поддержка std::unique_ptr<Tint> и производных
        (std::is_same_v<ElementType, std::unique_ptr<Tint>> ||
         std::is_same_v<ElementType, std::unique_ptr<MaskTint>> ||
         std::is_same_v<ElementType, std::unique_ptr<PaletteTint>> ||
         std::is_same_v<ElementType, std::unique_ptr<TextureSetTint>>) ||
        // Поддержка std::shared_ptr<Tint> и производных
        (std::is_same_v<ElementType, std::shared_ptr<Tint>> ||
         std::is_same_v<ElementType, std::shared_ptr<MaskTint>> ||
         std::is_same_v<ElementType, std::shared_ptr<PaletteTint>> ||
         std::is_same_v<ElementType, std::shared_ptr<TextureSetTint>>),
        "Container elements must be derived from Tint, be pointers to Tint, or be smart pointers to Tint");

    std::vector<RE::BGSCharacterTint::Entry*> entries{};
    entries.reserve(std::size(tints));

    // Улучшенная лямбда для извлечения указателя на Tint с проверками nullptr
    auto extractTintPointer = [](const auto& tintItem) -> const Tint* {
        using ItemType = std::remove_cv_t<std::remove_reference_t<decltype(tintItem)>>;
        
        if constexpr (std::is_pointer_v<ItemType>) {
            // Raw pointer - добавляем проверку на nullptr
            return tintItem; // nullptr check будет выполнена позже
        }
        else if constexpr (std::is_same_v<ItemType, std::unique_ptr<Tint>> ||
                          std::is_same_v<ItemType, std::unique_ptr<MaskTint>> ||
                          std::is_same_v<ItemType, std::unique_ptr<PaletteTint>> ||
                          std::is_same_v<ItemType, std::unique_ptr<TextureSetTint>>) {
            // unique_ptr variants
            return tintItem.get();
        }
        else if constexpr (std::is_same_v<ItemType, std::shared_ptr<Tint>> ||
                          std::is_same_v<ItemType, std::shared_ptr<MaskTint>> ||
                          std::is_same_v<ItemType, std::shared_ptr<PaletteTint>> ||
                          std::is_same_v<ItemType, std::shared_ptr<TextureSetTint>>) {
            // shared_ptr variants
            return tintItem.get();
        }
        else if constexpr (std::is_base_of_v<Tint, ItemType>) {
            // Direct Tint object (reference or value)
            return &tintItem;
        }
        else {
            static_assert(sizeof(ItemType) == 0, "Unsupported container element type");
            return nullptr;
        }
    };

    // Создаем новые entries
    for (const auto& tintItem : tints) {
        const Tint* tint = extractTintPointer(tintItem);
        
        // Проверяем на nullptr и пустоту
        if (!tint || tint->empty()) {
            continue;
        }

        auto templateEntry = tryToFindTemplateEntry(tint->id());
        if (!templateEntry) {
            logger::error("applyTints: Template entry not found for uniqueID {}", tint->id());
            continue;
        }
        
        auto* entry = CreateTintEntry(*tint, templateEntry);
        if (!entry) {
            logger::error("applyTints: Failed to create tint entry for uniqueID {}", tint->id());
            continue;
        }
        
        entries.emplace_back(entry);
    }

    // Проверяем, есть ли в пресете тинт скин-тона (slot == kSkinTone)
    bool hasSkinToneInPreset = false;
    for (auto* e : entries) {
        if (e && e->templateEntry && e->templateEntry->slot == RE::BGSCharacterTint::EntrySlot::kSkinTone) {
            hasSkinToneInPreset = true;
            break;
        }
    }

    // Если в пресете нет скин-тона — сохраняем существующий скин-тон NPC (чтобы не сбросился в серый цвет)
    RE::BGSCharacterTint::Entry* preservedSkinTone = nullptr;
    if (!hasSkinToneInPreset && npc->tintingData && npc->tintingData->data) {
        for (uint32_t i = 0; i < npc->tintingData->size; ++i) {
            auto* oldEntry = npc->tintingData->data[i];
            if (oldEntry && oldEntry->templateEntry &&
                oldEntry->templateEntry->slot == RE::BGSCharacterTint::EntrySlot::kSkinTone) {
                preservedSkinTone = oldEntry;
                break;
            }
        }
    }

    if (entries.empty() && !preservedSkinTone) {
        logger::warn("applyTints: No valid tints to apply");
        return false;
    }

    // Очищаем существующие tints
    auto oldSize = npc->tintingData->size;
    npc->tintingData->size = 0;
    
    // Освобождаем память для старых entries (кроме сохранённого скин-тона)
    if (npc->tintingData->data) {
        for (uint32_t i = 0; i < oldSize; ++i) {
            if (npc->tintingData->data[i]) {
                if (npc->tintingData->data[i] == preservedSkinTone) {
                    continue;
                }
                // Вызываем деструктор перед освобождением памяти
                npc->tintingData->data[i]->~Entry();
                tints_memory::DeallocateScrapHeap(npc->tintingData->data[i]);
            }
        }
    }

    if (preservedSkinTone) {
        entries.insert(entries.begin(), preservedSkinTone);
        logger::info("applyTints: Preserved existing skin tone entry for NPC {:#x}", npc->formID);
    }

    // Проверяем, нужна ли переаллокация массива указателей
    // ИСПРАВЛЕНО: сравниваем с размером entries, а не tints
    if (npc->tintingData->capacity < entries.size()) {
        npc->tintingData->capacity = 0;
        if (npc->tintingData->data) {
            tints_memory::DeallocateScrapHeap(npc->tintingData->data);
            npc->tintingData->data = nullptr;
        }
        
        // Выделяем новую память для массива указателей
        auto* newData = static_cast<RE::BGSCharacterTint::Entry**>(
            tints_memory::AllocateScrapHeap<RE::BGSCharacterTint::Entry*>(entries.size())
        );
        
        if (!newData) {
            logger::error("applyTints: Failed to allocate memory for {} tint entries", entries.size());
            // Очищаем созданные entries при ошибке
            for (auto* entry : entries) {
                if (entry) {
                    entry->~Entry();
                    tints_memory::DeallocateScrapHeap(entry); // ИСПРАВЛЕНО: CreateTintEntry использует AllocateScrapHeap
                }
            }
            return false;
        }
        
        npc->tintingData->data = newData;
        npc->tintingData->capacity = static_cast<uint32_t>(entries.size());
    }

    // ИСПРАВЛЕНО: более эффективное копирование entries
    for (uint32_t i = 0; i < entries.size(); ++i) {
        npc->tintingData->data[i] = entries[i];
    }
    npc->tintingData->size = static_cast<uint32_t>(entries.size());
    
    return true;
}