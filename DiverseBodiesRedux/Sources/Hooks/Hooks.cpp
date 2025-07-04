#include "Hooks.h"
#include <Windows.h>

uintptr_t GetModule(const std::string& pluginName)
{
    HMODULE module = pluginName.empty() ? GetModuleHandleA(NULL) : GetModuleHandleA(pluginName.c_str());
    if (!module) {
        F4SE::log::error("Failed to get module handle for {}", pluginName);
        return 0;
    }
    return reinterpret_cast<uintptr_t>(module);
}