#include "MainMenuHandler.h"

Hooks<MenuLoaderListener::MainMenuLoaderHandler> MenuLoaderListener::mainMenuLoaderHandler{"", 0x12A2B20, MenuLoaderListener::Hooked_MainMenuLoaderHandler};
std::vector<std::pair<std::string, std::function<void()>>> MenuLoaderListener::HookedMainMenuLoaderActions{};

MenuLoaderListener& MenuLoaderListener::get() {
    static MenuLoaderListener instance;
    return instance;
}

MenuLoaderListener::MenuLoaderListener() {
    if (!mainMenuLoaderHandler.IsInstalled()) {
        mainMenuLoaderHandler.Install();
    }
}

void MenuLoaderListener::AddFunction(const std::pair<const std::string&, const std::function<void()>>& func) noexcept {
    HookedMainMenuLoaderActions.push_back(func);
}

void MenuLoaderListener::AddFunction(const std::string& id, const std::function<void()>& func) noexcept {
    AddFunction(std::make_pair(id, func));
}

void MenuLoaderListener::Hooked_MainMenuLoaderHandler(void* instance) {
    mainMenuLoaderHandler.GetOriginal()(instance);
    for (const auto& action : HookedMainMenuLoaderActions) {
        action.second();
    }
}
