#pragma once
#undef interface
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <Hooks/Hooks.h>
#include "LooksMenuInterfaces.h"
#include "ActorsManager/ActorsManager.h"
#include <Windows.h>
#include <thread>
#include <chrono>

// Hooks and utilities for LooksMenu integration
namespace looksMenuHooks {

    // ============================================================================
    // UTILITY FUNCTIONS
    // ============================================================================

    // Safe address resolution utility (replaces DetourXS::RecurseJumps)
    inline uintptr_t resolveJumps(void* addr) {
        if (!addr) return 0;
        
        uint8_t* bytes = reinterpret_cast<uint8_t*>(addr);
        
        // Validate memory address
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0 || 
            mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE) {
            return reinterpret_cast<uintptr_t>(addr);
        }
        
        // Relative Near Jump (E9)
        if (bytes[0] == 0xE9) {
            int32_t offset = *reinterpret_cast<int32_t*>(bytes + 1);
            uintptr_t target = reinterpret_cast<uintptr_t>(bytes) + 5 + offset;
            logger::info("resolveJumps: Found E9 jump from {:X} to {:X} (offset: {:X})", 
                reinterpret_cast<uintptr_t>(addr), target, offset);
            return resolveJumps(reinterpret_cast<void*>(target));
        }
        
        // Absolute Jump (FF 25)
        if (bytes[0] == 0xFF && bytes[1] == 0x25) {
            int32_t offset = *reinterpret_cast<int32_t*>(bytes + 2);
            uintptr_t* targetPtr = reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(bytes) + 6 + offset);
            uintptr_t target = *targetPtr;
            logger::info("resolveJumps: Found FF 25 jump from {:X} to {:X}", 
                reinterpret_cast<uintptr_t>(addr), target);
            return resolveJumps(reinterpret_cast<void*>(target));
        }
        
        // Short Jump (EB)
        if (bytes[0] == 0xEB) {
            int8_t offset = static_cast<int8_t>(bytes[1]);
            uintptr_t target = reinterpret_cast<uintptr_t>(bytes) + 2 + offset;
            logger::info("resolveJumps: Found EB short jump from {:X} to {:X}", 
                reinterpret_cast<uintptr_t>(addr), target);
            return resolveJumps(reinterpret_cast<void*>(target));
        }
        
        // Not a jump - return original address
        return reinterpret_cast<uintptr_t>(addr);
    }

    // Get virtual function address from vtable with jump resolution
    inline uintptr_t getVirtualFncAddr(void* instance, size_t vtableIndex) {
        if (!instance) {
            return 0;
        }
        uintptr_t* vtable = *(uintptr_t**)instance;
        uintptr_t funcAddr = vtable[vtableIndex];

        // Follow jumps to actual function
        return resolveJumps(reinterpret_cast<void*>(funcAddr));
    }

    // Debug vtable inspection utility
    inline void debugVtable(void* instance, const char* className) {
        if (!instance) {
            logger::error("debugVtable: instance is nullptr for {}", className);
            return;
        }
        
        uintptr_t* vtable = *(uintptr_t**)instance;
        logger::info("debugVtable: {} vtable at {:X}", className, reinterpret_cast<uintptr_t>(vtable));
        
        for (int i = 0; i < 20; ++i) {
            uintptr_t funcAddr = vtable[i];
            
            // Simple address validation
            MEMORY_BASIC_INFORMATION mbi;
            bool isValid = (VirtualQuery(reinterpret_cast<LPCVOID>(funcAddr), &mbi, sizeof(mbi)) != 0) &&
                          (mbi.State == MEM_COMMIT) && (mbi.Type == MEM_IMAGE);
            
            logger::info("  vtable[{}] = {:X} {}", i, funcAddr, isValid ? "(valid)" : "(INVALID)");
        }
    }

    // Generic hook initialization helper
    template<typename HandlerType>
    inline bool initializeHookHelper(Hooks<HandlerType>& hook, void* a_interface, size_t vtableIndex, 
                                     HandlerType hookFunction, const char* hookName = "Unknown") {
        if (!a_interface) {
            logger::error("{}: interface is nullptr!", hookName);
            return false;
        }

        hook = Hooks<HandlerType>{
            getVirtualFncAddr(a_interface, vtableIndex),
            hookFunction
        };

        logger::info("Installing {}...", hookName);
        bool result = hook.Install();
        logger::info("{}.Install() result: {}", hookName, result);
        return result;
    }

    // Common actor validation for event hooks
    inline bool isActorProcessed(uint32_t formId) {
        auto& mgr = ActorsManager::get();
        return mgr.isReady() && mgr.containsActor(formId);
    }

    // Forward declaration for RestartTask
    template <typename EventT, typename HookT, typename SourceT>
    static void RestartTask(ActorUpdateManager* manager, EventT evnCopy, HookT& hook, 
                           std::function<bool()> shouldCallOrigin, SourceT* source);

    // Common async task restart logic
    template <typename EventT, typename HookT, typename SourceT>
    inline void scheduleAsyncRestart(ActorUpdateManager* manager, EventT evnCopy, HookT& hook, 
                                   std::function<bool()> shouldCallOrigin, SourceT* source) {
        std::thread([manager, evnCopy, &hook, shouldCallOrigin, source]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            RestartTask(manager, evnCopy, hook, shouldCallOrigin, source);
        }).detach();
    }

    // F4SE task restart utility
    template <typename EventT, typename HookT, typename SourceT>
    static void RestartTask(ActorUpdateManager* manager, EventT evnCopy, HookT& hook, 
                           std::function<bool()> shouldCallOrigin, SourceT* source) {

       F4SE::GetTaskInterface()->AddTask([manager, evnCopy, &hook, shouldCallOrigin, source]() mutable {
           auto& m = ActorsManager::get();
           if (!m.isReady()) {
               return RestartTask(manager, evnCopy, hook, shouldCallOrigin, source);
           }

           if (!shouldCallOrigin()) {
               return;
           }

           auto eventCopy = evnCopy; // Create local copy for reference passing
           hook.GetOriginal()(manager, eventCopy, source);
       });
    }

    // ============================================================================
    // ACTOR UPDATE MANAGER HOOKS
    // ============================================================================

    // --- TESObjectLoadedEvent Hook ---
    using ActorUpdateManagerTESObjectLoadedEventHandler = RE::BSEventNotifyControl(*)(
        ActorUpdateManager* thisPtr, const RE::TESObjectLoadedEvent& evn, 
        RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source);

    Hooks<ActorUpdateManagerTESObjectLoadedEventHandler> actorUpdateManagerTESObjectLoadedEventHook;

    RE::BSEventNotifyControl ActorUpdateManagerTESObjectLoadedEventHook(
        ActorUpdateManager* thisPtr, const RE::TESObjectLoadedEvent& evn,
        RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) {

        auto actor = RE::TESForm::GetFormByID<RE::Actor>(evn.formId);
        if (!actor) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (isActorProcessed(evn.formId)) {
            return RE::BSEventNotifyControl::kContinue; // Already processed
        }

        const auto id = evn.formId;
        auto evCopy = evn;

        auto shouldCallOrigin = [id]() {
            auto& m = ActorsManager::get();
            return !m.containsActor(id);
            };

        auto& mgr = ActorsManager::get();
        if (!mgr.isReady()) {
            scheduleAsyncRestart(thisPtr, evCopy, actorUpdateManagerTESObjectLoadedEventHook, shouldCallOrigin, a_source);
            return RE::BSEventNotifyControl::kContinue;
        }

        // Call original function
        return shouldCallOrigin() ? actorUpdateManagerTESObjectLoadedEventHook.GetOriginal()(thisPtr, evn, a_source) : RE::BSEventNotifyControl::kContinue;
    }

    bool initializeHookActorUpdateManagerTESObjectLoadedEvent() {
        return initializeHookHelper(
            actorUpdateManagerTESObjectLoadedEventHook,
            LooksMenuInterfaces<ActorUpdateManager>::GetInterface(),
            1,
            ActorUpdateManagerTESObjectLoadedEventHook,
            "ActorUpdateManagerTESObjectLoadedEventHook"
        );
    }

    // --- TESInitScriptEvent Hook ---
    using ActorUpdateManagerTESInitScriptEventHandler = RE::BSEventNotifyControl(*)(
        ActorUpdateManager* thisPtr, const RE::TESInitScriptEvent& evn, 
        RE::BSTEventSource<RE::TESInitScriptEvent>* a_source);

    Hooks<ActorUpdateManagerTESInitScriptEventHandler> actorUpdateManagerTESInitScriptEventHook;

    RE::BSEventNotifyControl ActorUpdateManagerTESInitScriptEventHook(
        ActorUpdateManager* thisPtr, const RE::TESInitScriptEvent& evn,
        RE::BSTEventSource<RE::TESInitScriptEvent>* a_source) {

        auto actor = evn.reference->As<RE::Actor>();
        if (!actor) {
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto formId = evn.reference->GetFormID();
        if (isActorProcessed(formId)) {
            return RE::BSEventNotifyControl::kContinue; // Already processed
        }

        auto evCopy = evn;
        auto shouldCallOrigin = [formId]() {
            auto& m = ActorsManager::get();
            return !m.containsActor(formId);
        };

        auto& mgr = ActorsManager::get();
        if (!mgr.isReady()) {
            scheduleAsyncRestart(thisPtr, evCopy, actorUpdateManagerTESInitScriptEventHook, shouldCallOrigin, a_source);
            return RE::BSEventNotifyControl::kContinue;
        }

        // Call original function
        return shouldCallOrigin() ? actorUpdateManagerTESInitScriptEventHook.GetOriginal()(thisPtr, evn, a_source) : RE::BSEventNotifyControl::kContinue;
    }

    bool initializeHookActorUpdateManagerTESInitScriptEvent() {
        return initializeHookHelper(
            actorUpdateManagerTESInitScriptEventHook,
            LooksMenuInterfaces<ActorUpdateManager>::GetInterface(),
            3,
            ActorUpdateManagerTESInitScriptEventHook,
            "ActorUpdateManagerTESInitScriptEventHook"
        );
    }

    // --- ActorUpdateManager Flush Hook ---
    using ActorUpdateManagerFlushHandler = void (*)(ActorUpdateManager* thisPtr);
    
    Hooks<ActorUpdateManagerFlushHandler> ActorUpdateManagerFlushHook;

    void HookedActorUpdateManagerFlush(ActorUpdateManager* m) {
		logger::info("HookedActorUpdateManagerFlush called");
		auto orig = ActorUpdateManagerFlushHook.GetOriginal();
		if (orig) {
			orig(m);
		}
    }

    bool initializeHookActorUpdateManagerFlush() {
        return initializeHookHelper(
            ActorUpdateManagerFlushHook,
            LooksMenuInterfaces<ActorUpdateManager>::GetInterface(),
            4,
            HookedActorUpdateManagerFlush,
            "ActorUpdateManagerFlushHook"
        );
    }

    // ============================================================================
    // OVERLAY INTERFACE HOOKS
    // ============================================================================

    // --- Remove Overlay Hook ---
    using RemoveOverlayHandler = bool (*)(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid);
    
    Hooks<RemoveOverlayHandler> RemoveOverlayHook;

    __pragma(optimize("", off))
    bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid) {
        RE::BSSpinLock locker(overlayInterface->m_overlayLock);
        auto& overlays = overlayInterface->m_overlays[isFemale ? 1 : 0];
        auto hit = overlays.find(actor->formID);
        if (hit == overlays.end())
            return false;
            
        OverlayInterface::PriorityMapPtr priorityMap = hit->second;
        if (!priorityMap)
            return false;
            
        for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it) {
            OverlayInterface::OverlayDataPtr overlayPtr = it->second;
            if (!overlayPtr || overlayPtr->uid != uid)
                continue;
                
            overlayInterface->m_dataMap.erase(overlayPtr->uid);
            overlayInterface->m_freeIndices.push_back(overlayPtr->uid);
            it = priorityMap->erase(it);
            return true;
        }
        return false;
    }
    __pragma(optimize("", on))

    bool initializeHookRemoveOverlay() {
        return initializeHookHelper(
            RemoveOverlayHook,
            LooksMenuInterfaces<OverlayInterface>::GetInterface(),
            9,
            HookedRemoveOverlay,
            "RemoveOverlayHook"
        );
    }

    // --- Add Overlay Hook ---
    using AddOverlayHandler = UniqueID (*)(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, 
                                          int32_t priority, const F4EEFixedString& templateName, 
                                          const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, 
                                          const RE::NiPoint2& scaleUV);
    
    Hooks<AddOverlayHandler> AddOverlayHook;

    __pragma(optimize("", off))
    UniqueID HookedAddOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, 
                             int32_t priority, const F4EEFixedString& templateName, 
                             const RE::NiColorA& tintColor, const RE::NiPoint2& offsetUV, 
                             const RE::NiPoint2& scaleUV) {
        return AddOverlayHook.GetOriginal()(overlayInterface, actor, isFemale, priority, 
                                          templateName, tintColor, offsetUV, scaleUV);
    }
    __pragma(optimize("", on))

    bool initializeHookAddOverlay() {
        return initializeHookHelper(
            AddOverlayHook,
            LooksMenuInterfaces<OverlayInterface>::GetInterface(),
            9,
            HookedAddOverlay,
            "AddOverlayHook"
        );
    }

    // --- Update Overlay Hook ---
    using UpdateOverlayHandler = bool (*)(OverlayInterface* overlayInterface, RE::Actor* actor, uint32_t slotIndex);
    
    Hooks<UpdateOverlayHandler> UpdateOverlayHook;

    __pragma(optimize("", off))
    bool HookedUpdateOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, uint32_t slotIndex) {
        return UpdateOverlayHook.GetOriginal()(overlayInterface, actor, slotIndex);
    }
    __pragma(optimize("", on))

    bool initializeHookUpdateOverlay() {
        return initializeHookHelper(
            UpdateOverlayHook,
            LooksMenuInterfaces<OverlayInterface>::GetInterface(),
            17,
            HookedUpdateOverlay,
            "UpdateOverlayHook"
        );
    }

    // --- Update Overlays Hook ---
    using UpdateOverlaysHandler = bool (*)(OverlayInterface* overlayInterface, RE::Actor* actor);
    
    Hooks<UpdateOverlaysHandler> UpdateOverlaysHook;

    __pragma(optimize("", off))
    bool HookedUpdateOverlays(OverlayInterface* overlayInterface, RE::Actor* actor) {
        return UpdateOverlaysHook.GetOriginal()(overlayInterface, actor);
    }
    __pragma(optimize("", on))

    bool initializeHookUpdateOverlays() {
        return initializeHookHelper(
            UpdateOverlaysHook,
            LooksMenuInterfaces<OverlayInterface>::GetInterface(),
            16,
            HookedUpdateOverlays,
            "UpdateOverlaysHook"
        );
    }

    // ============================================================================
    // F4EE HOOKS
    // ============================================================================

    // --- F4EEUpdateOverlays Run() Hook ---
    using F4EEUpdateOverlaysRunHandler = void (*)(F4EEUpdateOverlays* thisPtr);
    
    Hooks<F4EEUpdateOverlaysRunHandler> F4EEUpdateOverlaysRunHook;

    __pragma(optimize("", off))
    void HookedF4EEUpdateOverlaysRun(F4EEUpdateOverlays* thisPtr) {
        auto& mgr = ActorsManager::get();
  
        if (mgr.isReady()) {
            if (mgr.containsActor(thisPtr->formID())) {
                return; // Already processed in ActorUpdateManager
            }
        }
        else {
            // Restart task via F4SE Task Interface
            auto taskInterface = F4SE::GetTaskInterface();
            if (taskInterface) {
                auto copy = F4EEUpdateOverlays::CreateCopy(thisPtr);
                if (copy) {
                    taskInterface->AddTask(copy);
                    return;
                }
            }
        }

        // Call original function
        F4EEUpdateOverlaysRunHook.GetOriginal()(thisPtr);
    }
    __pragma(optimize("", on))

    //bool initializeF4EEUpdateOverlaysRun() {
    //    // Check if f4ee.dll is loaded
    //    auto f4eeModule = GetModule("f4ee.dll");
    //    if (!f4eeModule) {
    //        logger::error("f4ee.dll module not found!");
    //        return false;
    //    }
    //
    //    auto vtableAddr = f4eeModule + 0xC1538;
    //    uintptr_t* vtable = reinterpret_cast<uintptr_t*>(vtableAddr); 
    //    uintptr_t runFuncAddr = vtable[1];
    //    auto finalAddr = resolveJumps(reinterpret_cast<void*>(runFuncAddr));
    //    
    //    if (finalAddr == 0 || finalAddr == 0xFFFFFFFFFFFFFFFF) {
    //        logger::critical("F4EEUpdateOverlays::Run() address is invalid!");
    //        return false;
    //    }

    //    F4EEUpdateOverlaysRunHook = Hooks<F4EEUpdateOverlaysRunHandler>{
    //        finalAddr, HookedF4EEUpdateOverlaysRun
    //    };

    //    logger::info("Installing F4EEUpdateOverlaysRunHook...");
    //    bool result = F4EEUpdateOverlaysRunHook.Install();
    //    logger::info("F4EEUpdateOverlaysRunHook.Install() result: {}", result);
    //    return result;
    //}

    // ============================================================================
    // INITIALIZATION
    // ============================================================================

    void testHooksInstall() {
        initializeHookAddOverlay();
        //initializeHookUpdateOverlay();
        initializeHookUpdateOverlays();
    }

} // namespace looksMenuHooks