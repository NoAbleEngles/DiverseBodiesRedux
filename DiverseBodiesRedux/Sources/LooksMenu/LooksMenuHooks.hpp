#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"
#include <Hooks/Hooks.h>
#include "LooksMenuInterfaces.h"
#include "ActorsManager/ActorsManager.h"

namespace looksMenuHooks {
	// Типы функций для хуков LooksMenu
	using TESObjectLoadedEventHandler = RE::BSEventNotifyControl(*)(ActorUpdateManager*, RE::TESObjectLoadedEvent*, void*);
	using TESInitScriptEventHandler = RE::BSEventNotifyControl(*)(ActorUpdateManager*, RE::TESInitScriptEvent*, void*);
	using RemoveOverlayHandler = bool (*)(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid); //nopse fix for RemoveOverlay


	RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager*, RE::TESInitScriptEvent*, void*);
	RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager* auManager, RE::TESObjectLoadedEvent* a_event, void* ptr);
	bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid);

	Hooks<TESInitScriptEventHandler> TESInitScriptEventHook{ "f4ee.dll", 0x3C40, HookedReceiveEventInitScript };
	Hooks<TESObjectLoadedEventHandler> TESObjectLoadedEventHook { "f4ee.dll", 0x3DE0, HookedReceiveEventObjectLoaded };
	Hooks<RemoveOverlayHandler> RemoveOverlayHook{ "f4ee.dll", 0x420D, HookedRemoveOverlay };

	// Функции-хуки для LooksMenu
	inline RE::BSEventNotifyControl HookedReceiveEventObjectLoaded(ActorUpdateManager* auManager, RE::TESObjectLoadedEvent* a_event, void* ptr) {
		if (auto& manager = ActorsManager::get(); manager.isReady()) {
			if (!manager.containsActor(a_event->formId))
				TESObjectLoadedEventHook.GetOriginal()(auManager, a_event, ptr);
		}
		else {
			// Пока не знаю что с этим делать, вероятно надо складывать в стек, "разрывать" вглубь лукс меню и выполнять заказанные действия, которые должны происходить при загрузке.
			// Пока просто верну управление в оригинальный метод LooksMenu, посмотрим как это будет работать.
			TESObjectLoadedEventHook.GetOriginal()(auManager, a_event, ptr);
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	inline RE::BSEventNotifyControl HookedReceiveEventInitScript(ActorUpdateManager*, RE::TESInitScriptEvent*, void*) {
		return RE::BSEventNotifyControl::kContinue;
	}

	// Функция-хук для удаления оверлея
	inline bool HookedRemoveOverlay(OverlayInterface* overlayInterface, RE::Actor* actor, bool isFemale, UniqueID uid) {
		RE::BSSpinLock locker(overlayInterface->m_overlayLock);
		// logger::info("HookedRemoveOverlay function called");
		auto hit = overlayInterface->m_overlays[isFemale ? 1 : 0].find(actor->formID);
		if (hit == overlayInterface->m_overlays[isFemale ? 1 : 0].end())
			return false;
		// logger::info("HookedRemoveOverlay #1");
		OverlayInterface::PriorityMapPtr priorityMap = hit->second;
		if (!priorityMap)
			return false;
		// logger::info("HookedRemoveOverlay #2");
		for (auto it = priorityMap->begin(); it != priorityMap->end(); ++it) {
			OverlayInterface::OverlayDataPtr overlayPtr = it->second;
			if (!overlayPtr)
				continue;

			// logger::info("HookedRemoveOverlay #3");
			if (overlayPtr->uid == uid) {
				overlayInterface->m_dataMap.erase(overlayPtr->uid);
				logger::info("HookedRemoveOverlay #4");
				overlayInterface->m_freeIndices.push_back(overlayPtr->uid);
				logger::info("Remove overlay id has been added to 'm_freeIndices'");
				priorityMap->erase(it);
				return true;
			}
		}

		// logger::info("HookedRemoveOverlay #5");
		return false;
	}
}