#include "Hooks/Hooks.h"
#include <functional>

class MenuLoaderListener {
public:
	using MainMenuLoaderHandler = void(__fastcall*)(void* instance);

	static MenuLoaderListener& get();

	void AddFunction(const std::string& id, const std::function<void()>& func) noexcept;
	void AddFunction(const std::pair<const std::string&, const std::function<void()>>& func) noexcept;

private:
	MenuLoaderListener();

	static void Hooked_MainMenuLoaderHandler(void* instance);
	static Hooks<MainMenuLoaderHandler> mainMenuLoaderHandler;
	static std::vector<std::pair<std::string, std::function<void()>>> HookedMainMenuLoaderActions;
};