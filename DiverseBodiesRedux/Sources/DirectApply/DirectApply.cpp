#include "DirectApply.h"
#include "PCH.h"
#include <chrono>
#include <sstream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "globals.h"
#include "Utils/utility.h"
#include <typeinfo>


//void testDeleteMe() {
//    auto player = RE::PlayerCharacter::GetSingleton();
//    /*auto playerFaceNPC = player->GetNPC();
//    if (playerFaceNPC && playerFaceNPC->faceNPC) {
//        playerFaceNPC = playerFaceNPC->faceNPC;
//    }
//    if (!playerFaceNPC || !playerFaceNPC->tintingData) {
//        logger::error("Player face NPC or tinting data is null");
//        return;
//	}*/
//
//    //auto* race = player->GetVisualsRace();
//	auto* playerFaceNPC = player->GetNPC();
//    if (playerFaceNPC) {
//        playerFaceNPC = playerFaceNPC->GetRootFaceNPC();
//	}
//
//    /*if (race && race->faceRelatedData[0] && race->faceRelatedData[0]->tintingTemplate) {
//        const auto* groups = race->faceRelatedData[0]->tintingTemplate;
//        for (const auto* group : groups->groups) {
//            if (!group) {
//                logger::warn("Tinting group is null");
//                continue;
//            }
//            std::ostringstream oss;
//            oss << "Group: " << group->name.c_str()
//                << " (id: " << group->id
//                << ", chargenIndex: " << group->chargenIndex
//                << "), entries: " << group->entries.size() << "\n";
//            for (const auto* entry : group->entries) {
//                if (!entry) {
//                    oss << "  [null entry]\n";
//                    continue;
//                }
//                oss << "  Entry: " << entry->name.c_str()
//                    << " (slot: " << static_cast<int>(entry->slot.underlying())
//                    << ", uniqueID: " << entry->uniqueID
//                    << ", flags: " << static_cast<int>(entry->flags)
//                    << ", type: " << typeid(*entry).name()
//                    << ")\n";
//            }
//            logger::info("{}", oss.str());
//        }
//    }
//    else {
//        logger::warn("No tinting template found for player visuals race");
//    }*/
//
//    if (playerFaceNPC && playerFaceNPC->headParts && playerFaceNPC->numHeadParts > 0) {
//        std::ostringstream oss;
//
//        // Рекурсивная лямбда для вывода headPart и всех extraParts
//        std::function<void(const RE::BGSHeadPart*, int)> logHeadPart = [&](const RE::BGSHeadPart* hp, int indent) {
//            if (!hp) {
//                oss << std::string(indent, ' ') << "null\n";
//                return;
//            }
//            oss << std::string(indent, ' ')
//                << "formID=0x" << std::hex << hp->formID << std::dec
//                << ", name=\"" << (hp->GetFullName() ? hp->GetFullName() : "") << "\""
//                << ", formEditorID=\"" << (hp->formEditorID.c_str() ? hp->formEditorID.c_str() : "") << "\"";
//
//            // textureSet
//            oss << ", textureSet=" << (hp->textureSet ? hp->textureSet->materialName.c_str() : "no");
//
//            // colorForm
//            oss << ", colorForm=";
//            if (hp->colorForm) {
//                oss << "0x" << std::hex << hp->colorForm->formID << std::dec;
//                if (auto id = hp->colorForm->GetFormEditorID())
//                    oss << " \"" << id << "\"";
//            }
//            else {
//                oss << "no";
//            }
//
//            // flags и type
//            oss << ", flags=" << static_cast<int>(hp->flags.underlying())
//                << ", type=" << static_cast<int>(hp->type.underlying());
//
//            // extraParts
//            oss << ", extraParts=" << hp->extraParts.size();
//            if (!hp->extraParts.empty()) {
//                oss << " [\n";
//                for (RE::BSTArrayHeapAllocator::size_type j = 0; j < hp->extraParts.size(); ++j) {
//                    oss << std::string(indent + 2, ' ') << "[" << j << "]: ";
//                    logHeadPart(hp->extraParts[j], indent + 4);
//                }
//                oss << std::string(indent, ' ') << "]";
//            }
//            oss << "\n";
//            };
//
//        oss << "HeadParts (" << static_cast<int>(playerFaceNPC->numHeadParts) << "):\n";
//        for (int i = 0; i < playerFaceNPC->numHeadParts; ++i) {
//            oss << "  [" << i << "]: ";
//            logHeadPart(playerFaceNPC->headParts[i], 4);
//        }
//        logger::info("{}", oss.str());
//    }
//
//    if (playerFaceNPC && playerFaceNPC->headRelatedData) {
//        const auto* hrd = playerFaceNPC->headRelatedData;
//        std::ostringstream oss;
//        oss << "headRelatedData:\n";
//        oss << "  hairColor: ";
//        if (hrd->hairColor) {
//            oss << fmt::format("0x{:X}", hrd->hairColor->formID);
//            const char* name = hrd->hairColor->GetFullName();
//            if (name) {
//                oss << " - " << name;
//            }
//            if (auto id = hrd->hairColor->GetFormEditorID()) {
//                oss << fmt::format(" \"{}\"", id);
//            }
//        }
//        else {
//            oss << "null";
//        }
//        oss << "\n  facialHairColor: ";
//        if (hrd->facialHairColor) {
//            oss << fmt::format("0x{:X}", hrd->facialHairColor->formID);
//            const char* name = hrd->facialHairColor->GetFullName();
//            if (name) {
//                oss << " - " << name;
//            }
//            if (auto id = hrd->facialHairColor->GetFormEditorID()) {
//                oss << fmt::format(" \"{}\"", id);
//            }
//        }
//        else {
//            oss << "null";
//        }
//        oss << "\n  faceDetails: ";
//        if (hrd->faceDetails) {
//            oss << fmt::format("0x{:X}", hrd->faceDetails->formID);
//            // materialName может быть пустым, но не nullptr
//            oss << " - " << (hrd->faceDetails->materialName.c_str() ? hrd->faceDetails->materialName.c_str() : "");
//        }
//        else {
//            oss << "null";
//        }
//        logger::info("{}", oss.str());
//    }
//    else {
//        logger::warn("playerFaceNPC or headRelatedData is null");
//    }
//
//    if (player && player->tintingData) {
//        int idx = 0;
//        for (auto el : *player->tintingData) {
//            if (!el) {
//                logger::info("TintingData[{}]: nullptr", idx++);
//                continue;
//            }
//            std::ostringstream oss;
//            oss << "TintingData[" << idx << "]: type=" << typeid(*el).name();
//
//            // Безопасно выводим templateEntry
//            const auto* tmpl = el->templateEntry;
//            if (tmpl) {
//                oss << "\n  name: " << (tmpl->name.empty() ? "<empty>" : tmpl->name.c_str());
//                oss << "\n  slot: " << static_cast<int>(tmpl->slot.underlying());
//                oss << "\n  uniqueID: " << tmpl->uniqueID;
//                oss << "\n  flags: " << static_cast<int>(tmpl->flags);
//            }
//            else {
//                oss << "\n  templateEntry: nullptr";
//            }
//
//            // Определяем реальный тип через dynamic_cast
//            using namespace RE::BGSCharacterTint;
//
//            uint32_t type = el->GetType();
//            switch (type) {
//            case Entry::kTypePalette: {
//                auto palette = static_cast<PaletteEntry*>(el);
//                oss << "\n  [PaletteEntry]";
//
//                // PaletteEntry наследует MaskEntry, а не Template::Palette!
//                // Но templateEntry может быть Template::Palette
//                if (auto tpal = static_cast<const Template::Palette*>(tmpl)) {
//                    oss << "\n    texture: " << (tpal->texture.empty() ? "<empty>" : tpal->texture.c_str());
//                    oss << "\n    unk28: " << tpal->unk28;
//                    oss << "\n    unk2C: " << tpal->unk2C;
//                    oss << "\n    colors.size: " << tpal->colors.size();
//                    for (size_t c = 0; c < tpal->colors.size(); ++c) {
//                        const auto& color = tpal->colors[c];
//                        oss << "\n      Color[" << c << "]:";
//                        oss << " colorForm=0x" << std::hex << (color.colorForm ? color.colorForm->formID : 0) << std::dec;
//                        if (color.colorForm && color.colorForm->GetFullName())
//                            oss << " \"" << color.colorForm->GetFullName() << "\"";
//                        oss << ", alpha=" << color.alpha;
//                        oss << ", blendOp=" << color.blendOp;
//                        oss << ", colorID=" << color.colorID;
//                        oss << ", unk12=" << color.unk12;
//                        oss << ", unk14=" << color.unk14;
//                    }
//                }
//                // Собственные параметры PaletteEntry
//                oss << "\n    colorID: " << palette->colorID;
//                oss << "\n    color: 0x" << std::hex << palette->color.bgra << std::dec;
//                break;
//            }
//            case Entry::kTypeMask: {
//                auto mask = static_cast<MaskEntry*>(el);
//                oss << "\n  [MaskEntry]";
//                if (auto tmask = static_cast<const Template::Mask*>(tmpl)) {
//                    oss << "\n    texture: " << (tmask->texture.empty() ? "<empty>" : tmask->texture.c_str());
//                    oss << "\n    blendOp: " << tmask->blendOp;
//                    oss << "\n    unk2C: " << tmask->unk2C;
//                }
//                break;
//            }
//            case Entry::kTypeTexture: {
//                auto texset = static_cast<TextureSetEntry*>(el);
//                oss << "\n  [TextureSetEntry]";
//                if (auto tset = static_cast<const Template::TextureSet*>(tmpl)) {
//                    oss << "\n    diffuse: " << (tset->diffuse.empty() ? "<empty>" : tset->diffuse.c_str());
//                    oss << "\n    normal: " << (tset->normal.empty() ? "<empty>" : tset->normal.c_str());
//                    oss << "\n    specular: " << (tset->specular.empty() ? "<empty>" : tset->specular.c_str());
//                    oss << "\n    blendOp: " << tset->blendOp;
//                    oss << "\n    defaultValue: " << tset->defaultValue;
//                }
//
//                if (texset->templateEntry->uniqueID == 3795) {
//                    texset->percent = 255;
//                    player->Reset3D(true, RE::RESET_3D_FLAGS::kNone, true, RE::RESET_3D_FLAGS::kHead);
//				}
//                break;
//            }
//            default:
//                oss << "\n  [Unknown type: " << type << "]";
//                break;
//            }
//            logger::info("{}", oss.str());
//            ++idx;
//        }
//    }
//}

namespace logger = F4SE::log;

MenuButtonHandler& MenuButtonHandler::get() {
    static MenuButtonHandler instance;
    return instance;
}

bool MenuButtonHandler::ShouldHandleEvent(const RE::InputEvent* a_event) {
    if (a_event->eventType != RE::INPUT_EVENT_TYPE::kButton || !m_onButtonPressedCallback) {
        return false;
    }
    const auto* button = static_cast<const RE::ButtonEvent*>(a_event);
    if (button->idCode != static_cast<std::int32_t>(m_buttonCode)) {
        return false;
    }
    if (button->value == 0.0f) {
        m_fired = false;  // отпущена: следующее удержание снова откроет меню
        return false;
    }
    if (button->QJustPressed()) {
        m_fired = false;  // новое нажатие (отпускание могло не дойти, пока было открыто меню)
    }
    // Один раз за удержание: когда кнопку держат не меньше holdSeconds (0 - сразу при нажатии).
    if (m_fired || button->QHeldDownSecs() < m_holdSeconds) {
        return false;
    }
    m_fired = true;
    return true;
}

MenuButtonHandler::MenuButtonHandler() : PlayerInputHandler(m_controlsData) {
    RE::PlayerControls::GetSingleton()->RegisterHandler(this);
    m_buttonCode = RE::BS_BUTTON_CODE::kNumpad_Divide; // Инициализация кнопки по умолчанию
    logger::info("MenuButtonHandler: Initialized with button code {}", RE::ToString(m_buttonCode));
};

bool MenuButtonHandler::setButtonPressedCallback(const std::function<void()>& callbackFunction) {
    if (callbackFunction) {
        m_onButtonPressedCallback = callbackFunction;
        return true;
    }
    return false;
}

void MenuButtonHandler::HandleEvent(const RE::ButtonEvent* a_event) {
    m_onButtonPressedCallback();
}

RE::BS_BUTTON_CODE MenuButtonHandler::button() {
    return m_buttonCode;
}

void MenuButtonHandler::setHoldSeconds(float seconds) noexcept {
    // NaN и отрицательные значения - 0 (сразу), слишком большие - потолок
    m_holdSeconds = seconds >= 0.0f ? (std::min)(seconds, kMaxHoldSeconds) : 0.0f;
}

void MenuButtonHandler::loadFromIni() {
    auto& ini = *globals::g_ini;
    const int button = ini.at("MENU/iMenuKey", kDefaultButton);
    if (!setButton(static_cast<RE::BS_BUTTON_CODE>(button))) {
        logger::warn("MenuButtonHandler: [MENU] iMenuKey={} is not a usable button, using the default.", button);
        setButton(static_cast<RE::BS_BUTTON_CODE>(kDefaultButton));
    }
    setHoldSeconds(ini.at("MENU/fMenuHoldSeconds", kDefaultHoldSeconds));
    logger::info("MenuButtonHandler: button {} ({}), hold {:.2f} s", static_cast<int>(m_buttonCode), RE::ToString(m_buttonCode), m_holdSeconds);
}

bool MenuButtonHandler::saveToIni() const {
    auto& ini = *globals::g_ini;
    const bool keyOk = ini.set("MENU/iMenuKey", static_cast<int>(m_buttonCode));
    const bool holdOk = ini.set("MENU/fMenuHoldSeconds", m_holdSeconds);
    return keyOk && holdOk;
}

bool MenuButtonHandler::setButton(RE::BS_BUTTON_CODE buttonCode) {
    // Escape закрывает меню и сам открыть его не может
    if (buttonCode != RE::BS_BUTTON_CODE::kEscape && RE::IsButtonCodeValid(buttonCode)) {
        m_buttonCode = buttonCode;
        return true;
    }
    return false;
}

// ===== РЕАЛИЗАЦИЯ КЛАССА DirectApply =====

MenuItem::MenuItem(int t, const std::string& l) : type(t), label(l), state(0), hasState(false) {}
MenuItem::MenuItem(int t, const std::string& l, int s) : type(t), label(l), state(s), hasState(true) {}

// Статические переменные
void* DirectApply::s_inputHook = nullptr;
bool DirectApply::s_isMenuRegistered = false;

// Состояние меню
static MENU_STATE s_menuState = MENU_STATE::NONE;
static bool s_isMenuOpen = false;

DirectApply::DirectApply()
{
    logger::info("DirectApply: Constructor called");
    menuName = MENU_NAME; // Установка имени меню

    menuFlags.set(
        RE::UI_MENU_FLAGS::kUsesCursor,
        RE::UI_MENU_FLAGS::kDisablePauseMenu,
        RE::UI_MENU_FLAGS::kUsesMovementToDirection
    );

    depthPriority.set(RE::UI_DEPTH_PRIORITY::kTerminal);

    const auto ScaleformManager = RE::BSScaleformManager::GetSingleton();
    if (!ScaleformManager->LoadMovieEx(*this, SWF_PATH, "root", RE::Scaleform::GFx::Movie::ScaleModeType::kShowAll)) {
        RE::SendHUDMessage::ShowHUDMessage("Failed to load menu.", "UIMenuCancel", false, true);
        return;
    }

    if (!uiMovie) {
        logger::error("DirectApply: Failed to load SWF movie.");
        return;
    }

    filterHolder = RE::msvc::make_unique<RE::BSGFxShaderFXTarget>(*uiMovie, "root.scrollPane_mc");
    if (filterHolder) {
        filterHolder->CreateAndSetFiltersToHUD(RE::HUDColorTypes::kMenuNoColorBackground);
        shaderFXObjects.push_back(filterHolder.get());
    }

    // Принудительная инициализация AS3 объекта
    if (uiMovie && uiMovie->asMovieRoot) {
        uiMovie->asMovieRoot->GetVariable(&menuObj, "root");

        // Принудительная инициализация ScrollPane и его компонентов
        RE::Scaleform::GFx::Value result;
        uiMovie->asMovieRoot->Invoke("root.initializeComponents", &result, nullptr, 0);

        logger::info("DirectApply: Forced AS3 initialization called");
    }

    m_isInitialized = menuObj.IsObject();

    if (m_isInitialized) logger::info("DirectApply: Constructor completed success.");
    else logger::error("DirectApply: Constructor failed.");
}

DirectApply::~DirectApply()
{
    logger::info("DirectApply: Destructor called");

    m_isInitialized = false;
}

RE::IMenu* DirectApply::Create(const RE::UIMessage&) {
    logger::info("DirectApply: Creating menu instance");
    return new DirectApply();
}

void DirectApply::show() {
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::show: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.show", &result, nullptr, 0);
    logger::info("DirectApply: show() called via asMovieRoot");
}

void DirectApply::hide() {
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::hide: Menu not initialized");
        return;
    }

    // Вызов через menuObj (если доступен), иначе через asMovieRoot
    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.hide", &result, nullptr, 0);
    logger::info("DirectApply: hide() called via asMovieRoot");
}

void DirectApply::close()
{
    logger::info("DirectApply: Closing menu");

    //Временная заглушка для закрытия меню

    logger::info("DirectApply: Menu closed");
}

// ===== МЕТОДЫ ДЛЯ РАБОТЫ С dbMenu.swf =====

void DirectApply::clear()
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::clear: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.clear", &result, nullptr, 0);
    logger::info("DirectApply: clear() called via asMovieRoot");
}

void DirectApply::destroy()
{
    logger::info("DirectApply: Destroying menu");
}

void DirectApply::push(MENU_OBJECT type, const std::string_view& label, int state)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::Push: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[3];
    args[0] = static_cast<double>(type);
    args[1] = label.data();
    args[2] = static_cast<double>(state);

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.push", &result, args, 3);
    logger::info("DirectApply: push({}, '{}', {}) called via asMovieRoot", static_cast<int>(type), label, state);
}

void DirectApply::pushArray(const std::vector<MenuItem>& items)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::pushArray: Menu not initialized");
        return;
    }

    // Добавляем отладочный вывод
    logger::info("DirectApply: pushArray called with {} items", items.size());

    // Проверяем содержимое элементов
    for (size_t i = 0; i < items.size() && i < 5; ++i) { // Логируем до 5 элементов
        const auto& item = items[i];
        logger::info("  Item {}: type={}, label='{}', hasState={}, state={}",
            i, item.type, item.label, item.hasState ? "true" : "false",
            item.hasState ? item.state : 0);
    }

    // Создаем массив для передачи в Flash
    RE::Scaleform::GFx::Value pushArray;
    uiMovie->CreateArray(&pushArray);

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];

        // Создаем массив для элемента [type, label, state?]
        RE::Scaleform::GFx::Value itemVal;
        uiMovie->CreateArray(&itemVal);

        // Добавляем тип
        itemVal.PushBack(item.type);

        // Добавляем лейбл
        itemVal.PushBack(item.label.c_str());

        // Всегда добавляем состояние (даже если нет) для лучшей совместимости с AS3
        if (item.hasState) {
            itemVal.PushBack(item.state);
        }
        else {
            // Для не-state элементов добавляем 0 по умолчанию
            itemVal.PushBack(0);
        }

        // Добавляем элемент в основной массив
        pushArray.PushBack(itemVal);
    }

    RE::Scaleform::GFx::Value args[1];
    args[0] = std::move(pushArray);

    // Попытка вызова с отладочным выводом результата
    RE::Scaleform::GFx::Value result;
    bool success = false;

    success = uiMovie->asMovieRoot->Invoke("root.pushArray", &result, args, 1);
    logger::info("DirectApply: pushArray() called via asMovieRoot with {} items, success: {}",
        items.size(), success ? "true" : "false");

}

void DirectApply::insert(int index, int type, const std::string& label, int state)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::Insert: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[4];
    args[0] = index;
    args[1] = type;
    args[2] = label.c_str();
    args[3] = state;

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.insert", &result, args, 4);
    logger::info("DirectApply: Insert({}, {}, '{}', {}) called", index, type, label, state);
}

int DirectApply::find(const std::string& label)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::Find: Menu not initialized");
        return -1;
    }

    RE::Scaleform::GFx::Value args[1];
    args[0] = label.c_str();

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.find", &result, args, 1);

    if (result.IsInt()) {
        int index = result.GetInt();
        logger::info("DirectApply: Find('{}') returned {}", label, index);
        return index;
    }

    return -1;
}

void DirectApply::deleteComponent(const std::string& label)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::DeleteComponent: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[1];
    args[0] = label.c_str();

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.deleteComponent", &result, args, 1);
    logger::info("DirectApply: DeleteComponent('{}') called", label);
}

void DirectApply::updateMenuScale()
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::UpdateMenuScale: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.updateMenuScale", &result, nullptr, 0);
    logger::info("DirectApply: UpdateMenuScale() called");
}

void DirectApply::setMenuMargins(float right, float top, float bottom)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::SetMenuMargins: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[3];
    args[0] = static_cast<double>(right);
    args[1] = static_cast<double>(top);
    args[2] = static_cast<double>(bottom);

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setMenuMargins", &result, args, 3);
    logger::info("DirectApply: SetMenuMargins({}, {}, {}) called", right, top, bottom);
}

RE::Scaleform::GFx::Value DirectApply::getMenuScaleInfo()
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::GetMenuScaleInfo: Menu not initialized");
        return RE::Scaleform::GFx::Value();
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.getMenuScaleInfo", &result, nullptr, 0);
    logger::info("DirectApply: GetMenuScaleInfo() called");
    return result;
}

RE::Scaleform::GFx::Value DirectApply::getMenuMargins()
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::GetMenuMargins: Menu not initialized");
        return RE::Scaleform::GFx::Value();
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.getMenuMargins", &result, nullptr, 0);
    logger::info("DirectApply: GetMenuMargins() called");
    return result;
}

void DirectApply::setHoverHighlight(int index)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::SetHoverHighlight: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[1];
    args[0] = index;

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setHoverHighlight", &result, args, 1);
    logger::info("DirectApply: SetHoverHighlight({}) called", index);
}

void DirectApply::clearHoverHighlight()
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::ClearHoverHighlight: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.clearHoverHighlight", &result, nullptr, 0);
    logger::info("DirectApply: ClearHoverHighlight() called");
}

void DirectApply::showActors()
{
    logger::info("DirectApply: Showing actors menu");

    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::showActors: Menu not initialized");
        return;
    }

    clear(); // Очищаем текущее меню

    // Получаем список актёров в зоне
    auto actorsVector = functions::getActorsInRange(
        RE::PlayerCharacter::GetSingleton(), // a_ref
        10000,                               // a_maxDistance
        m_maxActorsCount,                    // a_maxActorsCount
        false,                               // a_includeDead
        [](const RE::Actor* actor) -> bool { // filter
            return actor && actor->GetFullyLoaded3D() && ActorsManager::isQualifiedRace(actor) && !ActorsManager::isExcluded(actor);
        }
    );

    std::sort(actorsVector.begin(), actorsVector.end(),
        [](const RE::Actor* a, const RE::Actor* b) {
            if (!a) return false;
            if (!b) return true;
            static auto player = RE::PlayerCharacter::GetSingleton();
            auto d_a = player->GetPosition().GetDistance(a->GetPosition());
            auto d_b = player->GetPosition().GetDistance(b->GetPosition());
            return d_a < d_b;
        });

    logger::info("DirectApply: Found {} actors in range", actorsVector.size());

    int actorIndex = -1;

    // Если нет актеров в зоне, добавляем информационное сообщение через push
    if (actorsVector.empty()) {
        push(MENU_OBJECT::LABEL, "No actors found nearby.");
    }
    else {
        logger::info("DirectApply: Adding {} actors to menu", actorsVector.size());

        // Сначала добавляем заголовок
        push(MENU_OBJECT::LABEL, "SELECT AN ACTOR:");

        int count = 0;
        for (const auto& actor : actorsVector) {
            if (actor) {
                ++count;
                // Добавляем актёра в меню с номером для лучшей читаемости
                std::string actorName = actor->GetDisplayFullName();
                if (actorName.empty()) actorName = "<Unnamed>";
                if (actorIndex < 0 && m_options.actor && actor->formID == m_options.actor->formID) {
                    actorIndex = count;
                }
                std::string label = fmt::format("{} [{:X}]", actorName, actor->formID);
                push(MENU_OBJECT::BUTTON, label);
            }
        }
    }

    if (actorIndex >= 0) {
        RE::Scaleform::GFx::Value args[1];
        args[0] = actorIndex;
        RE::Scaleform::GFx::Value result;
        uiMovie->asMovieRoot->Invoke("root.setActiveIndex", &result, args, 1);
        logger::info("DirectApply: Set active index to {}", actorIndex);
    }

    // AS3 самостоятельно управляет отложенным масштабированием
    // Нет необходимости в дополнительных вызовах updateMenuScale()

    m_state = MENU_STATE::ACTORS;

    logger::info("DirectApply: Actors menu population complete");
}

void DirectApply::showOptions()
{
    clear();
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::showOptions: Menu not initialized");
        return;
    }

    // Лейбл актёра
    std::string labelName = m_options.actor ?
        fmt::format("[{:X}] {}", m_options.actor->formID, m_options.actor->GetDisplayFullName())
        : "Unknown actor";
    push(MENU_OBJECT::LABEL, labelName);

    // Свитчер для выбора типа пресета (отображается как кнопка)
    std::string bodyTypeObjects = std::move(getPresetsSwitcherString());
    int presetTypeCalculated = static_cast<int>(m_options.presetType) - 1; //Нужно т.к. не передаётся NONE и END
    if (presetTypeCalculated + 1 <= static_cast<int>(PresetType::NONE))
        presetTypeCalculated = 1;
    else if (presetTypeCalculated + 1 >= static_cast<int>(PresetType::END)) {
        presetTypeCalculated = static_cast<int>(PresetType::END) - 2;
    }
    push(MENU_OBJECT::SWITCHER, bodyTypeObjects, presetTypeCalculated);

    // Чекбоксы
    auto pushCheckbox = [this](const char* label, Filter filterOption) {
        int selected = static_cast<bool>(m_options.filter & filterOption) ? 1 : 0;
        push(MENU_OBJECT::CHECKBOX, label, selected);
        };

    for_each_filter([&](Filter filter) {
        if (filter != Filter::Gender) // Пол не должен быть в чекбоксах, он всегда включён
            pushCheckbox(getFilterString(filter).data(), filter);
        });

    // Кнопка "Apply"
    push(MENU_OBJECT::BUTTON, "Apply");

    // AS3 самостоятельно управляет отложенным масштабированием
    // Нет необходимости в дополнительных вызовах updateMenuScale()

    m_state = MENU_STATE::OPTIONS;
    logger::info("showOptions: m_options.actor = {}", (void*)m_options.actor);
}

void DirectApply::showPresets(int active)
{
    clear();
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::showPresets: Menu not initialized");
        return;
    }

    if (!m_options.actor) {
        logger::error("Current actor is not set, cannot show presets menu.");
        push(MENU_OBJECT::LABEL, "Invalid/No actor was set");
        m_state = MENU_STATE::PRESETS;
        return;
    }

    if (static_cast<int>(m_options.filter & Filter::Gender) == 0) {
        m_options.filter |= Filter::Gender; // Пол должен быть всегда включён
    }

    auto filter = [this](const RE::Actor* actor, const std::shared_ptr<Preset> preset)->bool {
        bool result = false;
        result = preset->type() == m_options.presetType;
        if (result)
            result = (preset->check(actor, m_options.filter) != CoincidenceLevel::NONE);
        return result;
        //return preset->type() == m_options.presetType && (preset->check(actor, m_options.filter) != CoincidenceLevel::NONE);
        };

    auto coincidenceFilter = [this](const RE::Actor* actor, std::vector<std::shared_ptr<Preset>>& vec) -> std::vector<std::shared_ptr<Preset>> {
        std::vector<int> levels;
        levels.reserve(vec.size());
        int maxLevel = 0;

        // Первый проход: вычисляем уровни совпадения и максимальный уровень
        for (const auto& preset : vec) {
            int level = preset ? static_cast<int>(preset->check(actor, m_options.filter)) : 0;
            levels.push_back(level);
            if (level > 0 && level > maxLevel)
                maxLevel = level;
        }

        // Второй проход: собираем только пресеты с максимальным уровнем
        std::vector<std::shared_ptr<Preset>> filteredPresets;
        for (size_t i = 0; i < vec.size(); ++i) {
            if (levels[i] == maxLevel && vec[i])
                filteredPresets.push_back(vec[i]);
        }
        return filteredPresets;
        };

    auto& manager = PresetsManager::get();
    // Здесь не очень хорошо что приходится делать два вызова check() - в getPresets и coincidenceFilter, но т.к. это только для меню, то это не критично.
    auto presetsArray = std::move(manager.getPresets(m_options.actor, filter));
    presetsArray = std::move(coincidenceFilter(m_options.actor, presetsArray));

    if (presetsArray.empty()) {
        push(MENU_OBJECT::LABEL, "No valid presets");
    }

    for (const auto& el : presetsArray) {
        push(MENU_OBJECT::BUTTON, el->id());
    }

    // AS3 самостоятельно управляет отложенным масштабированием
    // Нет необходимости в дополнительных вызовах updateMenuScale()

    m_state = MENU_STATE::PRESETS;
}

int DirectApply::maxActorsCount() const noexcept {
    return m_maxActorsCount;
}

void DirectApply::setMaxActorsCount(int count) noexcept {
    m_maxActorsCount = count;
    if (m_maxActorsCount < 1) {
        m_maxActorsCount = 1; // Минимальное значение 1
    }
    else if (m_maxActorsCount > 128) {
        m_maxActorsCount = 128; // Максимальное значение 128
    }
    logger::info("DirectApply: Max actors count set to {}", m_maxActorsCount);
}

RE::UI_MESSAGE_RESULTS DirectApply::ProcessMessage(RE::UIMessage& a_message)
{
    //logger::info("DirectApply: ProcessMessage called with type: {} : {}", RE::UiMessageTypeToString(a_message.type.get()), a_message.menu.c_str());

    switch (*a_message.type) {
    case RE::UI_MESSAGE_TYPE::kShow:
    {
        //testDeleteMe();
        auto ControlMap = RE::ControlMap::GetSingleton();
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
        ControlMap->PushInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);
        RE::SendHUDMessage::PushHUDMode("SpecialMode");
        RE::PlayerControls::GetSingleton()->blockPlayerInput = true;

        // Инициализируем menuObj при первом показе, когда Flash уже точно загружен
        if (menuObj.IsObject() && uiMovie && uiMovie->asMovieRoot) {
            // Устанавливаем отступы меню для корректного позиционирования
            RE::Scaleform::GFx::Value result;
            //RE::Scaleform::GFx::Value marginArgs[3];
            //marginArgs[0] = 10.0; // right margin
            //marginArgs[1] = 10.0; // top margin
            //marginArgs[2] = 10.0; // bottom margin

            //uiMovie->asMovieRoot->Invoke("root.setMenuMargins", &result, marginArgs, 3);
            //logger::info("DirectApply: setMenuMargins({}, {}, {}) called for proper positioning", std::to_string(marginArgs[0].GetNumber()), std::to_string(marginArgs[1].GetNumber()), std::to_string(marginArgs[2].GetNumber()));

            // Запускаем одно обновление масштабирования - AS3 будет управлять отложенным масштабированием
            uiMovie->asMovieRoot->Invoke("root.updateMenuScale", &result, nullptr, 0);
            logger::info("DirectApply: updateMenuScale() called - AS3 will handle delayed scaling");
        }
        else {
            m_isInitialized = false;
            logger::error("DirectApply: Failed to initialize menuObj");
        }

        // Устанавливаем цветовую схему для меню
        ini::map colorsIni = *globals::g_ini;
        ColorsTheme theme = Glass; // По умолчанию Glass

        if (colorsIni.update()) {
            std::string themeStr = colorsIni.at("COLORS/sTheme", "Glass"s);

            // Выбираем тему
            if (themeStr == "Neon") {
                theme = Neon;
            }
            else if (themeStr == "Vintage") {
                theme = Vintage;
            }

            logger::info("DirectApply: Selected color theme: {}", themeStr);

            // Применяем базовую тему
            setColorsTheme(theme);

            // Применяем переопределения цветов поверх темы
            if (auto scrollPaneColors = colorsIni.at("COLORS/sScrollPane", ""s); !scrollPaneColors.empty()) {
                auto parsed = parseScrollPaneColors(scrollPaneColors);
                if (!parsed.isEmpty()) {
                    uint32_t borderColor = parsed.border.empty() ? theme.scrollPaneBorder : parseHexColor(parsed.border);
                    uint32_t bgColor = parsed.background.empty() ? theme.scrollPaneBackground : parseHexColor(parsed.background);
                    setScrollableMenuColorsRGBA(borderColor, bgColor);
                    logger::info("DirectApply: Applied ScrollPane color override");
                }
            }

            if (auto labelColors = colorsIni.at("COLORS/sLabel", ""s); !labelColors.empty()) {
                auto parsed = parseComponentColors(labelColors);
                applyComponentColorOverride(MENU_OBJECT::LABEL, parsed, theme);
            }

            if (auto buttonColors = colorsIni.at("COLORS/sButton", ""s); !buttonColors.empty()) {
                auto parsed = parseComponentColors(buttonColors);
                applyComponentColorOverride(MENU_OBJECT::BUTTON, parsed, theme);
            }

            if (auto checkboxColors = colorsIni.at("COLORS/sCheckbox", ""s); !checkboxColors.empty()) {
                auto parsed = parseComponentColors(checkboxColors);
                applyComponentColorOverride(MENU_OBJECT::CHECKBOX, parsed, theme);
            }

            if (auto switcherColors = colorsIni.at("COLORS/sSwitcher", ""s); !switcherColors.empty()) {
                auto parsed = parseComponentColors(switcherColors);
                applyComponentColorOverride(MENU_OBJECT::SWITCHER, parsed, theme);
            }
        }
        else {
            // Если INI не загружен, применяем тему по умолчанию
            setColorsTheme(theme);
        }

        // Показываем актёров
        showActors();

        return RE::UI_MESSAGE_RESULTS::kPassOn;
    }
    case RE::UI_MESSAGE_TYPE::kHide:
    {
        auto ControlMap = RE::ControlMap::GetSingleton();
        ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kBasicMenuNav);
        ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kThumbNav);
        ControlMap->PopInputContext(RE::UserEvents::INPUT_CONTEXT_ID::kVirtualController);

        if (!RE::UI::GetSingleton()->GetMenuOpen("NAFStudioMenu")) {
            RE::SendHUDMessage::PopHUDMode("SpecialMode");
            RE::PlayerControls::GetSingleton()->blockPlayerInput = false;
        }


        return RE::UI_MESSAGE_RESULTS::kPassOn;
    }
    }

    return RE::IMenu::ProcessMessage(a_message);
}

enum MapCodeMethod {
    ItemSelected = 0,
    BackPressed = 1,
    ItemHoverChanged = 2,
    CheckboxChanged = 3,
    SwitcherChanged = 4,
    Log = 5
};

void DirectApply::Call(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    switch ((*((std::uint32_t*)&(params.userData)))) {
    case ItemSelected:
        switch (m_state) {
        case MENU_STATE::ACTORS: actorsPushCall(params); break;
        case MENU_STATE::OPTIONS: optionsPushCall(params); break;
        case MENU_STATE::PRESETS: presetsPushCall(params); break;
        }
        break;
    case BackPressed:
        switch (m_state) {
        case MENU_STATE::ACTORS: actorsBackCall(params); break;
        case MENU_STATE::OPTIONS: optionsBackCall(params); break;
        case MENU_STATE::PRESETS: presetsBackCall(params); break;
        }
        break;
    case ItemHoverChanged:
        if (m_state == MENU_STATE::PRESETS) presetsHoverCall(params);
        break;
    case CheckboxChanged:
        if (m_state == MENU_STATE::OPTIONS) optionsCheckboxCall(params);
        break;
    case SwitcherChanged:
        if (m_state == MENU_STATE::OPTIONS) optionsSwitcherCall(params);
        break;
    case Log:
        if (params.argCount >= 1 && params.args[0].IsString()) {
            if (auto& ini = *globals::g_ini; !ini.at("DEBUG/bScaleformLog", false)) {
                break;
            }
            if (params.argCount > 0 && params.args[0].IsString()) {
                scaleformLogger(params.args[0].GetString());
            }
        }
        break;
    }
}

void DirectApply::actorsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 1 || params.args[0].IsString() == false) {
        logger::error("Invalid parameters in actorsPushCall.");
        return;
    }

    std::string element = params.args[0].GetString();;

    // Получим значение элемента менжду символами [ и ]
    auto start = element.find('[');
    auto end = element.find(']');
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        logger::error("Invalid actor element format: {}", element);
        return;
    }
    std::string actorIdStr = element.substr(start + 1, end - start - 1);
    try {
        auto actorId = std::stoul(actorIdStr, nullptr, 16); // Преобразуем в число
        m_options.actor = functions::getFormByFormID<RE::Actor>(actorId);
    }
    catch (const std::exception& e) {
        logger::error("Failed to parse actor ID from element '{}': {}", element, e.what());
        return;
    }
    catch (...) {
        logger::error("Unknown error while parsing actor ID from element '{}'.", element);
        return;
    }
    if (!m_options.actor) {
        logger::error("Actor with ID {} not found.", element);
        return;
    }

    showOptions();
}

void DirectApply::optionsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 1 || params.args[0].IsString() == false) {
        logger::error("Invalid parameters in optionsPushCall.");
        return;
    }

    std::string element = params.args[0].GetString();

    if (element == "Apply") {
        if (!m_options.actor) {
            logger::error("No actor selected, cannot apply preset.");
            return;
        }

		auto& manager = ActorsManager::get();
		auto excludedPresetTypes = manager.getExcludedTypes(m_options.actor);
        // нулевой/пустой набор = актёр исключён целиком (разыменовывать его нельзя)
        if (excludedPresetTypes && (!*excludedPresetTypes || (*excludedPresetTypes)->empty() || (*excludedPresetTypes)->contains(m_options.presetType))) {
			logger::warn("Preset type '{}' is excluded for [0x{:x}] {}, cannot apply preset.", GetPresetTypeString(m_options.presetType), m_options.actor->GetFormID(), m_options.actor->GetDisplayFullName());
            return;
        }
        showPresets();
    }
}

void DirectApply::presetsPushCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 1 || params.args[0].IsString() == false) {
        logger::error("Invalid parameters in presetsPushCall.");
        return;
    }

    std::string element = params.args[0].GetString();

    auto& pmanager = PresetsManager::get();
    auto preset = pmanager.getPreset(element);

    if (!preset) {
        logger::error("Preset '{}' not found.", element);
        return;
    }

    if (m_options.actor) {
        ActorsManager::get().applyActorPreset(m_options.actor, preset, true);
        if (ActorsManager::get().updateActorPresetInStorage(m_options.actor, preset)) {
            RE::SendHUDMessage::ShowHUDMessage("Preset Applied", "UIMenuOK", true, false);
        }
        else {
            RE::SendHUDMessage::ShowHUDMessage("ERROR", "UIMenuCancel", true, false);
        }
    }
    else {
        logger::error("No actor selected, cannot apply preset.");
        return;
    }
}

void DirectApply::actorsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    const auto msg = RE::UIMessageQueue::GetSingleton();
    const auto ui = RE::UI::GetSingleton();

    if (!msg || !ui) {
        logger::error("DBMenuToggleHandler: Failed to get UIMessageQueue or UI singleton");
        return;
    }

    if (!ui->menuMap.contains(MENU_NAME)) {
        logger::error("Menu '{}' not registered", MENU_NAME);
        return;
    }

    auto current_state_open = ui->GetMenuOpen(MENU_NAME);

    logger::info("try to {} DB_DirectApply menu!", current_state_open ? "close" : "open");
    if (current_state_open) {
        msg->AddMessage(DirectApply::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide);
    }
}

void DirectApply::optionsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    showActors();
}

void DirectApply::presetsBackCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    showOptions();
}

PresetType DirectApply::getPresetTypeFromString(const std::string& str) {
    for (auto i = 1; i < static_cast<int>(PresetType::END); ++i) {
        if (GetPresetTypeString(static_cast<PresetType>(i)) == str)
            return static_cast<PresetType>(i);
    }
    return PresetType::NONE; // Возвращаем NONE, если не найдено совпадение
}

namespace {
    std::chrono::steady_clock::time_point g_lastPresetApplyTime;
    std::atomic<bool> g_cooldownActive{ false };
    std::string g_lastElement;
    std::mutex g_cooldownMutex;
}

void DirectApply::presetsHoverCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 1 || !params.args[0].IsString()) {
        logger::error("Invalid parameters in presetsHoverCall.");
        return;
    }

    std::string element = params.args[0].GetString();

    {
        std::lock_guard<std::mutex> lock(g_cooldownMutex);
        g_lastElement = element;
    }

    auto now = std::chrono::steady_clock::now();
    if (!g_cooldownActive) {
        g_cooldownActive = true;
        g_lastPresetApplyTime = now;

        // Запускаем задачу с задержкой
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::string elementCopy;
            {
                std::lock_guard<std::mutex> lock(g_cooldownMutex);
                elementCopy = g_lastElement;
            }

            auto& pmanager = PresetsManager::get();
            auto preset = pmanager.getPreset(elementCopy);

            if (!preset) {
                logger::error("Preset '{}' not found.", elementCopy);
            }
            else if (m_options.actor) {
                ActorsManager::get().applyActorPreset(m_options.actor, preset, true);
            }
            else {
                logger::error("No actor selected, cannot apply preset.");
            }

            g_cooldownActive = false;
            }).detach();
    }
    // Если cooldown активен, просто обновляем g_lastElement и выходим
}

void DirectApply::optionsCheckboxCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 2 || params.args[0].IsString() == false || params.args[1].IsBoolean() != true) {
        logger::error("Invalid parameters in optionsCheckboxCall.");
        return;
    }

    auto flag = getFilterFromString(params.args[0].GetString());
    if (flag == Filter::None) {
        logger::error("Invalid filter element: {}", params.args[0].GetString());
        return;
    }
    auto enable = params.args[1].GetBoolean();
    m_options.filter = enable ? (m_options.filter | flag) : (m_options.filter & ~flag);
}

void DirectApply::optionsSwitcherCall(const RE::Scaleform::GFx::FunctionHandler::Params& params) {
    if (params.argCount < 2 || !params.args[0].IsString() || !params.args[1].IsInt()) {
        logger::error("Invalid parameters in optionsSwitcherCall.");
        return;
    }

    std::string element = params.args[0].GetString();
    if (element == getPresetsSwitcherString()) { // значит бадипресеты, у них особая логика
        int type = params.args[1].GetInt() + 1;
        if (type <= static_cast<int>(PresetType::NONE)) {
            type = static_cast<int>(PresetType::NONE) + 1;
        }
        else if (type >= static_cast<int>(PresetType::END)) {
            type = static_cast<int>(PresetType::END) - 1;
        }
        m_options.presetType = static_cast<PresetType>(type);
    }
}

void DirectApply::MapCodeObjectFunctions() {
    MapCodeMethodToASFunction("ItemSelected", ItemSelected);
    MapCodeMethodToASFunction("BackPressed", BackPressed);
    MapCodeMethodToASFunction("ItemHoverChanged", ItemHoverChanged);
    MapCodeMethodToASFunction("CheckboxChanged", CheckboxChanged);
    MapCodeMethodToASFunction("SwitcherChanged", SwitcherChanged);
    MapCodeMethodToASFunction("Log", Log);
    logger::info("MapCodeObject functions registered.");
}

void DirectApply::HandleEvent(const RE::ButtonEvent* a_event) {
    //logger::info("DirectApply: HandleEvent called with event type: {}", static_cast<int>(a_event->eventType.get()));
}

std::string DirectApply::getPresetsSwitcherString() const noexcept {
    std::string result{};
    for (auto type = PresetType::BODYMORPHS; type < PresetType::END; type = static_cast<PresetType>(static_cast<int>(type) + 1)) {
        if (!result.empty())
            result += ",";
        result += GetPresetTypeString(type);
    }
    return result;
}

void DirectApply::scaleformLogger(const std::string& log) const {
    static std::optional<std::filesystem::path> logPath;
    static std::ofstream logFile;
    static bool firstOpen = true;
    if (!logPath) {
        logPath = logger::log_directory();
        if (logPath) {
            *logPath /= fmt::format(FMT_STRING("{}_scaleform.log"), Version::Name);
            // Очищаем файл при первом запуске
            logFile.open(logPath->string(), std::ios::out | std::ios::trunc);
            if (logFile.is_open()) {
                logFile.close();
            }
            firstOpen = false;
        }
    }
    // Открываем для дозаписи (append)
    if (!logFile.is_open() && logPath) {
        logFile.open(logPath->string(), std::ios::out | std::ios::app);
    }
    if (logFile.is_open()) {
        logFile << log << std::endl;
        logFile.flush();
    }
}

// ===== МЕТОДЫ НАСТРОЙКИ ЦВЕТОВ =====

void DirectApply::setComponentColorsRGBA(int componentType, uint32_t normalColorRGBA, uint32_t textColorRGBA, uint32_t hoverColorRGBA, uint32_t selectedColorRGBA)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::setComponentColorsRGBA: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[5];
    args[0] = componentType;
    args[1] = normalColorRGBA;
    args[2] = textColorRGBA;
    args[3] = hoverColorRGBA;
    args[4] = selectedColorRGBA;

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setComponentColorsRGBA", &result, args, 5);
    logger::info("DirectApply: setComponentColorsRGBA({}, 0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}) called",
        componentType, normalColorRGBA, textColorRGBA, hoverColorRGBA, selectedColorRGBA);
}

void DirectApply::setScrollableMenuColorsRGBA(uint32_t borderColorRGBA, uint32_t backgroundColorRGBA)
{
    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::setScrollableMenuColorsRGBA: Menu not initialized");
        return;
    }

    RE::Scaleform::GFx::Value args[2];
    args[0] = borderColorRGBA;
    args[1] = backgroundColorRGBA;

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setScrollableMenuColorsRGBA", &result, args, 2);
    logger::info("DirectApply: setScrollableMenuColorsRGBA(0x{:08X}, 0x{:08X}) called", borderColorRGBA, backgroundColorRGBA);
}

// ===== ОПРЕДЕЛЕНИЯ ЦВЕТОВЫХ ТЕМ =====

// Тема Glass - полупрозрачные голубоватые тона
const DirectApply::ColorsTheme DirectApply::Glass = {
    // ScrollPane (border, background)
    0x80808080,  // Полупрозрачный серый бордер
    0x40000000,  // Полупрозрачный темный фон

    // Label (normal, text, hover, selected)
    0x80E0E0E0,  // Прозрачный фон
    0xFFFFFFFF,  // Светло-серый текст
    0x95FFFFFF,  // Слегка видимый при hover
    0x80E0E0E0,  // Более видимый при выделении

    // Button (normal, text, hover, selected)
    0x10FFFFFF,  // Полупрозрачный голубой
    0xCC000000,  // Белый текст
    0x95FFFFFF,  // Более яркий голубой при hover
    0xFFFFFFFF,  // Еще более яркий при выделении

    // Checkbox (normal, text, hover, selected)
    0x10FFFFFF,  // Приглушенный голубой
    0xCC000000,  // Светло-серый текст
    0x95FFFFFF,  // Ярче при hover
    0xFFFFFFFF,  // Яркий при выделении

    // Switcher (normal, text, hover, selected)
    0x10FFFFFF,  // Фиолетово-голубой
    0xCC000000,  // Белый текст
    0x95FFFFFF,  // Ярче при hover
    0xFFFFFFFF   // Яркий при выделении
};

// Тема Neon - яркие неоновые цвета
const DirectApply::ColorsTheme DirectApply::Neon = {
    // ScrollPane (border, background)
    0xFF00FFFF,  // Яркий циан бордер
    0x80001122,  // Темный фон с неоновым оттенком

    // Label (normal, text, hover, selected)
    0x00000000,  // Прозрачный фон
    0xFF00FFAA,  // Неоново-зеленый текст
    0x60FF0080,  // Розовый при hover
    0xA0FF00FF,  // Магента при выделении

    // Button (normal, text, hover, selected)
    0x80FF0080,  // Неоново-розовый
    0xFF000000,  // Черный текст
    0xA0FF00FF,  // Магента при hover
    0xC000FFFF,  // Циан при выделении

    // Checkbox (normal, text, hover, selected)
    0x6000FF80,  // Неоново-зеленый
    0xFFFFFF00,  // Желтый текст
    0x80FF8000,  // Оранжевый при hover
    0xA0FF0040,  // Красно-розовый при выделении

    // Switcher (normal, text, hover, selected)
    0x808000FF,  // Неоново-синий
    0xFFFFFFFF,  // Белый текст
    0xA0FF0080,  // Розовый при hover
    0xC000FF00   // Зеленый при выделении
};

// Тема Vintage - теплые винтажные тона
const DirectApply::ColorsTheme DirectApply::Vintage = {
    // ScrollPane (border, background)
    0xDD8B4513,  // Коричневый бордер (saddle brown)
    0xAA2F1B14,  // Темно-коричневый фон

    // Label (normal, text, hover, selected)
    0x00000000,  // Прозрачный фон
    0xFFF5DEB3,  // Цвет пшеницы (wheat)
    0x60D2691E,  // Шоколадный при hover
    0x80Daa520,  // Золотистый при выделении

    // Button (normal, text, hover, selected)
    0x90A0522D,  // Сиена (sienna)
    0xFFFFFAF0,  // Цвет слоновой кости
    0xB0CD853F,  // Перуанский желтый при hover
    0xD0DAA520,  // Золотистый при выделении

    // Checkbox (normal, text, hover, selected)
    0x70BC8F8F,  // Розово-коричневый (rosy brown)
    0xFFF5DEB3,  // Цвет пшеницы
    0x90D2691E,  // Шоколадный при hover
    0xB0B8860B,  // Темно-золотистый при выделении

    // Switcher (normal, text, hover, selected)
    0x80CD853F,  // Перуанский желтый
    0xFF2F1B14,  // Темно-коричневый текст
    0xA0D2691E,  // Шоколадный при hover
    0xC0DAA520   // Золотистый при выделении
};

// ===== РЕАЛИЗАЦИЯ МЕТОДОВ РАБОТЫ С ЦВЕТАМИ =====

void DirectApply::setColorsTheme(const ColorsTheme& theme)
{
    logger::info("DirectApply: Applying color theme");

    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::warn("DirectApply::setColorsTheme: Menu not initialized, storing theme for later");
        return;
    }

    // Применяем цвета ScrollPane
    RE::Scaleform::GFx::Value args[18];
    args[0] = theme.scrollPaneBorder;
    args[1] = theme.scrollPaneBackground;
    args[2] = theme.labelNormal;
    args[3] = theme.labelText;
    args[4] = theme.labelHover;
    args[5] = theme.labelSelected;
    args[6] = theme.buttonNormal;
    args[7] = theme.buttonText;
    args[8] = theme.buttonHover;
    args[9] = theme.buttonSelected;
    args[10] = theme.checkboxNormal;
    args[11] = theme.checkboxText;
    args[12] = theme.checkboxHover;
    args[13] = theme.checkboxSelected;
    args[14] = theme.switcherNormal;
    args[15] = theme.switcherText;
    args[16] = theme.switcherHover;
    args[17] = theme.switcherSelected;

    RE::Scaleform::GFx::Value result;
    uiMovie->asMovieRoot->Invoke("root.setColorsTheme", &result, args, 18);
    logger::info("DirectApply: Color theme applied successfully");
}

DirectApply::ComponentColors DirectApply::parseComponentColors(const std::string& colorString) const
{
    ComponentColors result;
    if (colorString.empty()) {
        return result;
    }

    std::vector<std::string> colors;
    std::stringstream ss(colorString);
    std::string color;

    // Разделяем по запятым
    while (std::getline(ss, color, ',')) {
        // Убираем пробелы
        color.erase(0, color.find_first_not_of(" \t"));
        color.erase(color.find_last_not_of(" \t") + 1);
        if (!color.empty()) {
            colors.push_back(color);
        }
    }

    // Назначаем цвета в порядке: Normal, Text, Hover, Selected
    if (colors.size() >= 1) result.normal = colors[0];
    if (colors.size() >= 2) result.text = colors[1];
    if (colors.size() >= 3) result.hover = colors[2];
    if (colors.size() >= 4) result.selected = colors[3];

    return result;
}

DirectApply::ScrollPaneColors DirectApply::parseScrollPaneColors(const std::string& colorString) const
{
    ScrollPaneColors result;
    if (colorString.empty()) {
        return result;
    }

    std::vector<std::string> colors;
    std::stringstream ss(colorString);
    std::string color;

    // Разделяем по запятым
    while (std::getline(ss, color, ',')) {
        // Убираем пробелы
        color.erase(0, color.find_first_not_of(" \t"));
        color.erase(color.find_last_not_of(" \t") + 1);
        if (!color.empty()) {
            colors.push_back(color);
        }
    }

    // Назначаем цвета в порядке: Border, Background
    if (colors.size() >= 1) result.border = colors[0];
    if (colors.size() >= 2) result.background = colors[1];

    return result;
}

uint32_t DirectApply::parseHexColor(const std::string& hexString) const
{
    if (hexString.empty()) {
        return 0;
    }

    std::string cleanHex = hexString;
    // Убираем префикс 0x если есть
    if (cleanHex.length() >= 2 && cleanHex.substr(0, 2) == "0x") {
        cleanHex = cleanHex.substr(2);
    }

    try {
        uint32_t color = std::stoul(cleanHex, nullptr, 16);

        // Если длина 6 символов (RGB), добавляем полную альфу
        if (cleanHex.length() == 6) {
            color |= 0xFF000000;
        }

        return color;
    }
    catch (const std::exception& e) {
        logger::warn("DirectApply::parseHexColor: Failed to parse color '{}': {}", hexString, e.what());
        return 0;
    }
}

void DirectApply::applyComponentColorOverride(int componentType, const ComponentColors& colors, const ColorsTheme& theme)
{
    if (colors.isEmpty()) {
        return; // Нет переопределений
    }

    // Получаем базовые цвета из темы
    uint32_t baseNormal, baseText, baseHover, baseSelected;

    switch (componentType) {
    case MENU_OBJECT::LABEL:
        baseNormal = theme.labelNormal;
        baseText = theme.labelText;
        baseHover = theme.labelHover;
        baseSelected = theme.labelSelected;
        break;
    case MENU_OBJECT::BUTTON:
        baseNormal = theme.buttonNormal;
        baseText = theme.buttonText;
        baseHover = theme.buttonHover;
        baseSelected = theme.buttonSelected;
        break;
    case MENU_OBJECT::CHECKBOX:
        baseNormal = theme.checkboxNormal;
        baseText = theme.checkboxText;
        baseHover = theme.checkboxHover;
        baseSelected = theme.checkboxSelected;
        break;
    case MENU_OBJECT::SWITCHER:
        baseNormal = theme.switcherNormal;
        baseText = theme.switcherText;
        baseHover = theme.switcherHover;
        baseSelected = theme.switcherSelected;
        break;
    default:
        logger::warn("DirectApply::applyComponentColorOverride: Unknown component type {}", componentType);
        return;
    }

    // Применяем переопределения (если цвет не пустой, используем его, иначе базовый)
    uint32_t finalNormal = colors.normal.empty() ? baseNormal : parseHexColor(colors.normal);
    uint32_t finalText = colors.text.empty() ? baseText : parseHexColor(colors.text);
    uint32_t finalHover = colors.hover.empty() ? baseHover : parseHexColor(colors.hover);
    uint32_t finalSelected = colors.selected.empty() ? baseSelected : parseHexColor(colors.selected);

    // Применяем финальные цвета
    setComponentColorsRGBA(componentType, finalNormal, finalText, finalHover, finalSelected);

    logger::info("DirectApply: Applied color override for component type {}", componentType);
}

void DirectApply::resetColorsToDefault()
{
    logger::info("DirectApply: Resetting colors to default Glass theme");
    setColorsTheme(Glass);
}

RE::Scaleform::GFx::Value DirectApply::getComponentColors(int componentType)
{
    RE::Scaleform::GFx::Value result;

    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::getComponentColors: Menu not initialized");
        return result;
    }

    RE::Scaleform::GFx::Value args[1];
    args[0] = componentType;

    uiMovie->asMovieRoot->Invoke("root.getComponentColors", &result, args, 1);
    logger::info("DirectApply: getComponentColors({}) called", componentType);

    return result;
}

RE::Scaleform::GFx::Value DirectApply::getScrollableMenuColors()
{
    RE::Scaleform::GFx::Value result;

    if (!m_isInitialized || !uiMovie || !uiMovie->asMovieRoot) {
        logger::error("DirectApply::getScrollableMenuColors: Menu not initialized");
        return result;
    }

    uiMovie->asMovieRoot->Invoke("root.getScrollableMenuColors", &result, nullptr, 0);
    logger::info("DirectApply: getScrollableMenuColors() called");

    return result;
}