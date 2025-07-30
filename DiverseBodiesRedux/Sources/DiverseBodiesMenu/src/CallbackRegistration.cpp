// Пример регистрации колбэков DirectApply Menu в main.cpp

#include "DirectApply.h"

// В функции инициализации F4SE plugin
bool InitializePlugin() {
    logger::info("Initializing DirectApply Menu Plugin");
    
    // Регистрируем колбэки для Flash
    auto scaleformManager = RE::BSScaleformManager::GetSingleton();
    if (scaleformManager) {
        // Регистрируем статические функции как колбэки
        scaleformManager->AddCallbackHandler("onPushButtonComponent", 
            [](const RE::Scaleform::GFx::FunctionHandler::Params& params) {
                if (params.argCount >= 1 && params.args[0].IsString()) {
                    DirectApply::onPushButtonComponent(params.args[0].GetString());
                }
            });
            
        scaleformManager->AddCallbackHandler("onBackButtonComponent", 
            [](const RE::Scaleform::GFx::FunctionHandler::Params& params) {
                if (params.argCount >= 1 && params.args[0].IsString()) {
                    DirectApply::onBackButtonComponent(params.args[0].GetString());
                }
            });
            
        scaleformManager->AddCallbackHandler("onHoverButtonComponent", 
            [](const RE::Scaleform::GFx::FunctionHandler::Params& params) {
                if (params.argCount >= 1 && params.args[0].IsString()) {
                    DirectApply::onHoverButtonComponent(params.args[0].GetString());
                }
            });
            
        scaleformManager->AddCallbackHandler("onChangeCheckboxComponent", 
            [](const RE::Scaleform::GFx::FunctionHandler::Params& params) {
                if (params.argCount >= 2 && params.args[0].IsString() && params.args[1].IsBool()) {
                    DirectApply::onChangeCheckboxComponent(params.args[0].GetString(), 
                                                          params.args[1].GetBool());
                }
            });
            
        scaleformManager->AddCallbackHandler("onChangeSwitcherComponent", 
            [](const RE::Scaleform::GFx::FunctionHandler::Params& params) {
                if (params.argCount >= 2 && params.args[0].IsString() && params.args[1].IsNumber()) {
                    DirectApply::onChangeSwitcherComponent(params.args[0].GetString(), 
                                                          static_cast<int>(params.args[1].GetNumber()));
                }
            });
            
        logger::info("DirectApply Menu callbacks registered");
    }
    
    // Регистрируем само меню
    auto ui = RE::UI::GetSingleton();
    if (ui) {
        ui->Register(DirectApply::MENU_NAME, DirectApply::Create);
        logger::info("DirectApply Menu registered");
    }
    
    return true;
}

// Альтернативный способ через более простую регистрацию (если предыдущий не работает)
void RegisterDirectApplyCallbacks() {
    // Используем глобальные функции вместо лямбд
    RE::BSScaleformManager::GetSingleton()->RegisterFunction(
        "onPushButtonComponent", 
        [](RE::Scaleform::GFx::Movie* movie, RE::Scaleform::GFx::Value* result, 
           const char* methodName, const RE::Scaleform::GFx::Value* args, size_t argCount) {
            if (argCount >= 1 && args[0].IsString()) {
                DirectApply::onPushButtonComponent(args[0].GetString());
            }
        });
        
    // Аналогично для остальных колбэков...
}
