#include "Patches.h"
#include "PatchesMemUtils.h"
#include "ClothWorldFix.h"
#include <unordered_set>

namespace patches {
	// Hook handler instances
	// BSSetTransformSetHandler был удалён: адрес 0x1DAF3B0 = BSClothUtils::BSTransformSet::QClothSupportsLOD,
	// которую перекрывает BSClothUtils_BSTransformSet_QClothSupportsLOD_Handler с правильной сигнатурой.
	// Дублирование с неверной сигнатурой (void*, uint32_t, void*) вместо (void*) повреждало стек и вызывало краш.
	Hooks<BSClothExtraData_SetSettle_t> BSClothExtraData_SetSettle_Handler{ "", 0x18A1B60 /* 1.10.163: 0x1DA7A40 */, Hooked_BSClothExtraData_SetSettle };
	Hooks<SetTransformSet_Handler_t> SetTransformSet_Handler{ "", 0x18C8530 /* 1.10.163: 0x1DDC7A0 */, Hooked_SetTransformSet };
	Hooks<SetTransformSet_Handler_t> SetTransformSet2_Handler{ "", 0x18C85F0, Hooked_SetTransformSet2 };
	Hooks<ChangeHeadPartRemovePartHandler_t> ChangeHeadPartRemovePart{ "", 0x6555A0 /* 1.10.163: 0x5B5560 (5985632) */, Hooked_ChangeHeadPartRemovePart };
	Hooks<ChangeHeadPartHandler_t> ChangeHeadPart{ "", 0x6599B0 /* 1.10.163: 0x5B9B30 (6003504) */, Hooked_ChangeHeadPart };
	Hooks<cbp_sub_1800024C0_t> cbp_sub_1800024C0_Hook{ "cbp.dll", 0x24C0, Hooked_cbp_sub_1800024C0 };
	Hooks<bhkStream_LoadBinary_t> bhkStream_LoadBinary_Handler{ "", 0x18AA10, Hooked_bhkStream_LoadBinary };
	Hooks<HavokDeserializationCaller_t> HavokDeserializationCaller_Handler{ "", 0x175D80, Hooked_HavokDeserializationCaller };
	Hooks<AddHeadPartHandler_t> AddHeadPart{ "", 0x655350 /* 1.10.163: 0x5B52E0 */, Hooked_AddHeadPart };
	Hooks<BSClothExtraData__SetSettleOnTransitionToSim_Bool_handler_t> BSClothExtraData__SetSettleOnTransitionToSim_Bool_Handler{ "", 0x18A1B60 /* 1.10.163: 0x1DA7A40 */, Hooked_BSClothExtraData__SetSettleOnTransitionToSim_Bool };
	Hooks<BSClothUtils_BSTransformSet_QClothSupportsLOD_handler_t> BSClothUtils_BSTransformSet_QClothSupportsLOD_Handler{ "", 0x18A79F0 /* 1.10.163: 0x1DAF3B0 */, Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD };
	Hooks<BSClothUtils_BSTransformSet_ResetPrediction_void_handler_t> BSClothUtils_BSTransformSet_ResetPrediction_void_Handler{ "", 0x18A6070 /* 1.10.163: 0x1DADA50 */, Hooked_BSClothUtils_BSTransformSet_ResetPrediction_void };
	Hooks<BSClothExtraData_SetWorld_handler_t> BSClothExtraData_SetWorld_Handler{ "", 0x18A0DA0 /* 1.10.163: 0x1DA6C30 */, Hooked_BSClothExtraData_SetWorld };
	Hooks<PendingActorHeadData_Process_t> PendingActorHeadData_Process{ "", 0x6E0DA0 /* 1.10.163: 0x67B2D0 */, Hooked_PendingActorHeadData_Process };

	// Function implementations

	void Hooked_BSClothExtraData_SetSettle(RE::BSClothExtraData* thisPtr, bool enable)
	{
		__try {
			if (!thisPtr) {
				return;
			}
			if (!IsReadableFast(thisPtr, sizeof(void*))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid thisPtr 0x{:x}", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}
			BSClothExtraData_SetSettle_Handler.GetOriginal()(thisPtr, enable);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothExtraData_SetSettle: Critical error at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	void Hooked_SetTransformSet(void* instance, uint32_t index, void* transformSet) {
		// Реальная BSTransformSet::SetTransformSet (0x1DDC7A0) разыменовывает 3-й аргумент
		// БЕЗ проверки на null (cmp word ptr [r8+0xA],0 — самая первая инструкция) — вызов
		// с transformSet=nullptr гарантированно валит движок. instance тоже не проверялся —
		// падает на чтении [instance+0x40] (массив transform-сетов). Добавили SEH и
		// IsReadableFast на оба указателя, по аналогии с остальными хуками в этом файле.
		__try {
			if (!instance || !IsReadableFast(instance, 0x48)) {
				logger::warn("Hooked_SetTransformSet: invalid instance (0x{:x})", reinterpret_cast<uintptr_t>(instance));
				return;
			}
			if (!transformSet) {
				logger::warn("Hooked_SetTransformSet: transformSet is null. Skipping...");
				return;
			}
			if (!IsReadableFast(transformSet, 0x0C)) {
				logger::warn("Hooked_SetTransformSet: invalid transformSet (0x{:x})", reinterpret_cast<uintptr_t>(transformSet));
				return;
			}
			SetTransformSet_Handler.GetOriginal()(instance, index, transformSet);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_SetTransformSet: exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	static void HavokAddRef(void* obj) {
		if (!obj) return;
		auto* p16 = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(obj) + 0x0A);
		if (*p16 == 0) return;
		auto* p32 = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uint8_t*>(obj) + 0x08);
		uint32_t cur = *p32;
		while (true) {
			uint32_t next = (cur & 0xFFFF0000) | ((cur + 1) & 0xFFFF);
			uint32_t prev = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(p32), next, cur);
			if (prev == cur) break;
			cur = prev;
		}
	}

	static void HavokRelease(void* obj) {
		if (!obj) return;
		auto* p16 = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(obj) + 0x0A);
		if (*p16 == 0) return;
		auto* p32 = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uint8_t*>(obj) + 0x08);
		uint32_t cur = *p32;
		uint32_t next = 0;
		while (true) {
			next = (cur & 0xFFFF0000) | ((cur - 1) & 0xFFFF);
			uint32_t prev = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(p32), next, cur);
			if (prev == cur) break;
			cur = prev;
		}
		if ((next & 0xFFFF) == 0) {
			auto** vtbl = *reinterpret_cast<void***>(obj);
			using Dtor_t = void(__fastcall*)(void*);
			auto dtor = reinterpret_cast<Dtor_t>(vtbl[3]); // [vtable + 0x18]
			dtor(obj);
		}
	}

	void Hooked_SetTransformSet2(void* instance, uint32_t index, void* transformSet) {
		// Twin overload of hclClothInstance::setTransformSet at RVA 0x18C85F0.
		// Dereferences transformSet at +0x0A (cmp word ptr [r8+0xA], 0).
		// When called with transformSet = nullptr (e.g. physics hair/outfit missing cloth bones),
		// it crashes at Fallout4.exe+18C85FF without null check.
		__try {
			if (!instance || !IsReadableFast(instance, 0x48)) {
				logger::warn("Hooked_SetTransformSet2: invalid instance (0x{:x})", reinterpret_cast<uintptr_t>(instance));
				return;
			}
			if (!transformSet) {
				// Safely set transformSets[index] = nullptr, releasing any old transformSet
				auto** transformSets = *reinterpret_cast<uint8_t***>(reinterpret_cast<uint8_t*>(instance) + 0x40);
				int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(instance) + 0x48);
				if (transformSets && static_cast<int32_t>(index) < count) {
					void* oldTs = transformSets[index];
					if (oldTs) {
						HavokRelease(oldTs);
					}
					transformSets[index] = nullptr;
				}
				return;
			}
			if (!IsReadableFast(transformSet, 0x0C)) {
				logger::warn("Hooked_SetTransformSet2: invalid transformSet (0x{:x})", reinterpret_cast<uintptr_t>(transformSet));
				return;
			}
			SetTransformSet2_Handler.GetOriginal()(instance, index, transformSet);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_SetTransformSet2: exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	void removeWithExtra(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
		std::vector<RE::BGSHeadPart*> stack;
		std::unordered_set<RE::BGSHeadPart*> visited;
		stack.push_back(hpart);
		visited.insert(hpart);

		while (!stack.empty()) {
			RE::BGSHeadPart* current = stack.back();
			stack.pop_back();

			// Сначала добавляем все extraParts в стек с защитой от циклов
			for (auto it = current->extraParts.rbegin(); it != current->extraParts.rend(); ++it) {
				RE::BGSHeadPart* extra = *it;
				if (extra && visited.insert(extra).second) {
					if (!extra->extraParts.empty()) {
						stack.push_back(extra);
					}
					else {
						ChangeHeadPartRemovePart.GetOriginal()(npc, extra, false);
					}
				}
			}
			// После обработки всех extraParts удаляем сам current
			ChangeHeadPartRemovePart.GetOriginal()(npc, current, false);
		}
	}

	void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove) {
		if (!npc || !hpart)
			return;

		npc = functions::getFaceTESNPC(npc);
		auto formID{ 0 };
		if (npc) {
			formID = npc->formID;
		}

		if (isRemove) {
			if (bRemoveExtraParts)
				removeWithExtra(npc, hpart);
			else
				ChangeHeadPartRemovePart.GetOriginal()(npc, hpart, false);

		}
		else {
			ChangeHeadPart.GetOriginal()(npc, hpart);
		}
	}

	void Hooked_ChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts) {
		ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, true);
	}

	void Hooked_ChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
		ProcessChangeHeadPart(npc, hpart, false, false);
	}

	//bool Hooked_cbp_sub_1800024C0(void* rcx, uint32_t arg1, uint64_t arg2, uint64_t arg3) {
	//	__try {
	//		if (!rcx || !IsReadableFast(rcx, sizeof(void*))) {
	//			logger::warn("cbp_sub_1800024C0: rcx is invalid (0x{:x})", reinterpret_cast<uintptr_t>(rcx));
	//			return false;
	//		}
	//		// rax = [rcx+0x428]
	//		auto* obj = static_cast<std::uint8_t*>(rcx);
	//		if (!IsReadableFast(obj + 0x428, sizeof(void*))) {
	//			logger::warn("cbp_sub_1800024C0: [rcx+0x428] is invalid");
	//			return false;
	//		}
	//		auto* rax = *reinterpret_cast<std::uint8_t**>(obj + 0x428);
	//		if (!rax || !IsReadableFast(rax, 0x3D8 + sizeof(void*))) {
	//			logger::warn("cbp_sub_1800024C0: rax or [rax+0x3D8] is invalid");
	//			return false;
	//		}
	//		// If all checks pass, call original
	//		return cbp_sub_1800024C0_Hook.GetOriginal()(rcx, arg1, arg2, arg3);
	//	}
	//	__except (EXCEPTION_EXECUTE_HANDLER) {
	//		logger::error("cbp_sub_1800024C0: Exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
	//		return false;
	//	}
	//}

	bool Hooked_cbp_sub_1800024C0(void* rcx, uint32_t arg1, uint64_t arg2, uint64_t arg3) {
		__try {
			if (!rcx || IsBadReadPtr(rcx, sizeof(void*))) {
				logger::warn("cbp_sub_1800024C0: rcx is invalid (0x{:x})", reinterpret_cast<uintptr_t>(rcx));
				return false;
			}
			// rax = [rcx+0x428]
			uint8_t* obj = static_cast<uint8_t*>(rcx);
			if (IsBadReadPtr(obj + 0x428, sizeof(void*))) {
				logger::warn("cbp_sub_1800024C0: [rcx+0x428] is invalid");
				return false;
			}
			uint8_t* rax = *reinterpret_cast<uint8_t**>(obj + 0x428);
			if (!rax || IsBadReadPtr(rax, 0x3D8 + sizeof(void*))) {
				logger::warn("cbp_sub_1800024C0: rax or [rax+0x3D8] is invalid");
				return false;
			}
			// If all checks pass, call original
			return cbp_sub_1800024C0_Hook.GetOriginal()(rcx, arg1, arg2, arg3);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("cbp_sub_1800024C0: Exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
			return false;
		}
	}


	void Hooked_bhkStream_LoadBinary(void* thisPtr, void* stream, void* rootContainer) {
		__try {
			if (!thisPtr || !IsReadableFast(thisPtr, sizeof(void*))) {
				logger::warn("bhkStream_LoadBinary: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}

			if (!stream || !IsReadableFast(stream, sizeof(void*))) {
				logger::warn("bhkStream_LoadBinary: Invalid stream (0x{:x})", reinterpret_cast<uintptr_t>(stream));
				return;
			}

			if (!rootContainer || !IsReadableFast(rootContainer, sizeof(void*))) {
				logger::warn("bhkStream_LoadBinary: Invalid rootContainer (0x{:x})", reinterpret_cast<uintptr_t>(rootContainer));
				return;
			}

			// Доп. валидация TLS — падает после TlsGetValue при доступе [rax+0x18]
			//DWORD tlsIndex = 0; // здесь просто локальная переменная — реальный индекс берется внутри движка
			//LPVOID tlsData = TlsGetValue(tlsIndex);
			//if (!tlsData || !IsReadableFast(tlsData, 0x20)) { // нужно минимум до +0x18
			//	logger::warn("bhkStream_LoadBinary: Invalid TLS data (0x{:x})", reinterpret_cast<uintptr_t>(tlsData));
			//	return;
			//}

			//auto* tlsPtr = static_cast<std::uint8_t*>(tlsData);
			//void* ptrAt18 = *reinterpret_cast<void**>(tlsPtr + 0x18);
			//if (!ptrAt18 || !IsReadableFast(ptrAt18, sizeof(void*))) {
			//	logger::warn("bhkStream_LoadBinary: Invalid pointer at TLS+0x18 (0x{:x})", reinterpret_cast<uintptr_t>(ptrAt18));
			//	return;
			//}

			bhkStream_LoadBinary_Handler.GetOriginal()(thisPtr, stream, rootContainer);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("bhkStream_LoadBinary: Exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	void Hooked_HavokDeserializationCaller(void* thisPtr, void* stream) {
		__try {
			if (!thisPtr || !IsReadableFast(thisPtr, sizeof(void*))) {
				logger::warn("HavokDeserializationCaller: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}

			if (!stream || !IsReadableFast(stream, sizeof(void*))) {
				logger::warn("HavokDeserializationCaller: Invalid stream (0x{:x})", reinterpret_cast<uintptr_t>(stream));
				return;
			}

			// Легкая проверка TLS
			/*DWORD tlsIndex = 0;
			LPVOID tlsData = TlsGetValue(tlsIndex);
			if (!tlsData || !IsReadableFast(tlsData, 0x20)) {
				logger::warn("HavokDeserializationCaller: Invalid TLS data, skipping deserialization");
				return;
			}*/

			HavokDeserializationCaller_Handler.GetOriginal()(thisPtr, stream);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("HavokDeserializationCaller: Exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	void Hooked_AddHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* headPart, bool checkUnique, bool addExtra, bool updateActor) {
		AddHeadPart.GetOriginal()(npc, headPart, checkUnique, addExtra, updateActor);
	}

	// FaceGen pending-head-data guard — см. Patches.h.
	bool Hooked_PendingActorHeadData_Process(void* self)
	{
		__try {
			if (!self || !IsReadableFast(self, 0x90)) {
				return true; // нечего обрабатывать — считаем запись завершённой
			}

			const auto base = reinterpret_cast<std::uintptr_t>(self);
			auto* doneFlag = reinterpret_cast<std::uint8_t*>(base + 0x82);

			// Запись уже помечена как обработанная — оригинал выйдет по быстрому пути.
			if (*doneFlag != 0) {
				return PendingActorHeadData_Process.GetOriginal()(self);
			}

			// pending->npc (= actor->data.objectReference), захвачен сырым указателем при постановке в очередь.
			auto* npc = *reinterpret_cast<RE::TESNPC**>(base + 0x60);

			bool valid = npc
				&& IsReadableFast(npc, 0x1C0)
				&& npc->GetFormType() == RE::ENUM_FORM_ID::kNPC_;

			if (valid) {
				// TESNPC::formRace (TESRaceForm@0x1B0, указатель @0x1B8). Оригинал читает race->data.flags.
				auto* race = *reinterpret_cast<RE::TESRace**>(reinterpret_cast<std::uintptr_t>(npc) + 0x1B8);
				valid = race
					&& IsReadableFast(race, 0x1B0)
					&& race->GetFormType() == RE::ENUM_FORM_ID::kRACE;
			}

			if (!valid) {
				*doneFlag = 1; // помечаем обработанной → движок выкинет запись
				static std::atomic<int> s_logged{ 0 };
				if (s_logged.fetch_add(1, std::memory_order_relaxed) < 25) {
					logger::warn("[HeadDataGuard] dropped pending head data with dangling/invalid NPC (self=0x{:x} npc=0x{:x})",
						base, reinterpret_cast<std::uintptr_t>(npc));
				}
				return true;
			}

			return PendingActorHeadData_Process.GetOriginal()(self);
		}
		__except (
			[](unsigned int code, struct _EXCEPTION_POINTERS* ep) -> int {
				auto* rec = ep ? ep->ExceptionRecord : nullptr;
				logger::error("[HeadDataGuard] suppressed exception 0x{:x} at 0x{:x} while processing pending head data",
					code, rec ? reinterpret_cast<uintptr_t>(rec->ExceptionAddress) : 0);
				return EXCEPTION_EXECUTE_HANDLER;
			}(GetExceptionCode(), GetExceptionInformation())
		) {
			// CRITICAL FIX: Must mark *doneFlag = 1 and return true so the engine dequeues
			// the invalid entry. Returning false causes the engine to endlessly retry the entry,
			// causing an infinite loop, 100% CPU lockup, and game freeze.
			__try {
				if (self && reinterpret_cast<std::uintptr_t>(self) >= 0x10000) {
					*reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(self) + 0x82) = 1;
				}
			} __except (EXCEPTION_EXECUTE_HANDLER) {}
			return true;
		}
	}


	bool ValidateClothChain(RE::BSClothExtraData* thisPtr) {
		if (!thisPtr)
			return false;

		std::uint32_t count = *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(thisPtr) + 0x70);
		if (count == 0)
			return true;

		auto* arrBase = *reinterpret_cast<std::uint8_t**>(reinterpret_cast<std::uint8_t*>(thisPtr) + 0x60);
		if (!arrBase)
			return false;

		// Проверяем, что массив cloth instances читаем
		if (!IsReadableFast(arrBase, count * 8))
			return false;

		auto* arrEnd = arrBase + count * 8;
		for (auto* ptr = arrBase; ptr != arrEnd; ptr += 8) {
			auto* object = *reinterpret_cast<std::uint8_t**>(ptr);
			if (!object)
				return false;

			// Проверяем, что структура cloth instance читаема до +0x48
			if (!IsReadableFast(object, 0x50))  // 0x48 + запас
				return false;

			auto someCount = *reinterpret_cast<std::uint32_t*>(object + 0x48);
			if (someCount > 0) {
				auto* obj2 = *reinterpret_cast<std::uint8_t**>(object + 0x40);
				if (!obj2)
					return false;

				// Проверяем, что массив transform sets читаем
				if (!IsReadableFast(obj2, someCount * 8))
					return false;

				auto* obj3 = *reinterpret_cast<std::uint8_t**>(obj2);
				if (!obj3)
					return false;

				// КРИТИЧНО: Проверяем, что offset 0x1B7 записываем!
				if (!IsWritableFast(obj3 + 0x1B7, 1))  // 1 байт для settle flag
					return false;
			}
		}
		return true;
	}

	void Hooked_BSClothExtraData__SetSettleOnTransitionToSim_Bool(RE::BSClothExtraData* thisPtr, bool enable)
	{
		if (!thisPtr) {
			return;
		}

		// Full clean native implementation of BSClothExtraData::SetSettleOnTransitionToSim (0x18A1B60).
		// In vanilla Fallout4.exe, the loop over clothInstance->transformSets[0] at 0x18A1B90 dereferences
		// the transformSet pointer without checking for nullptr (mov [rcx+0x1B7], r9b).
		// When physics hair (e.g. AzarPonytailHairstyles) or cloth meshes with missing bones pass
		// transformSet = nullptr via SetTransformSet2, vanilla 0x18A1B90 immediately crashes with
		// EXCEPTION_ACCESS_VIOLATION writing to 0x0000000001B7.
		// Furthermore, DetourXS cannot relocate the relative branch 'je 0x18a1ba0' at 0x18A1B69 into
		// its heap trampoline, so calling GetOriginal() is unsafe.
		// By implementing the full loop here with explicit null checks and without GetOriginal(),
		// valid cloth (skirts, dresses, labcoats) receives the settle flag cleanly while null transformSets
		// are safely skipped without crashing.
		__try {
			auto* thisBytes = reinterpret_cast<uint8_t*>(thisPtr);
			if (!IsReadableFast(thisBytes + 0x70, sizeof(uint32_t))) {
				return;
			}
			uint32_t count = *reinterpret_cast<uint32_t*>(thisBytes + 0x70);
			if (count == 0) {
				return;
			}

			if (!IsReadableFast(thisBytes + 0x60, sizeof(uintptr_t))) {
				return;
			}
			auto** arr = *reinterpret_cast<uint8_t***>(thisBytes + 0x60);
			if (!arr || !IsReadableFast(arr, count * sizeof(uintptr_t))) {
				return;
			}

			for (uint32_t i = 0; i < count; ++i) {
				auto* clothInstance = arr[i];
				if (!clothInstance || !IsReadableFast(clothInstance + 0x48, sizeof(int32_t))) {
					continue;
				}

				int32_t numTransformSets = *reinterpret_cast<int32_t*>(clothInstance + 0x48);
				if (numTransformSets > 0) {
					if (!IsReadableFast(clothInstance + 0x40, sizeof(uintptr_t))) {
						continue;
					}
					auto** transformSets = *reinterpret_cast<uint8_t***>(clothInstance + 0x40);
					if (transformSets && IsReadableFast(transformSets, sizeof(uintptr_t))) {
						auto* transformSet = transformSets[0];
						if (transformSet && IsWritableFast(transformSet + 0x1B7, 1)) {
							transformSet[0x1B7] = enable ? 1 : 0;
						}
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothExtraData__SetSettleOnTransitionToSim_Bool: exception handled");
		}
	}

	bool Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD(void* thisPtr) {
		// SEH: thisPtr проходит дешёвую проверку IsReadableFast (страница замаплена), но это
		// не гарантирует что объект не является use-after-free с мусором вместо vtable —
		// оригинал делает виртуальный вызов через thisPtr. Остальные хуки в файле обёрнуты
		// в __try/__except, эти два — нет; добавляем по аналогии.
		__try {
			if (!thisPtr || !IsReadableFast(thisPtr, sizeof(void*))) {
				logger::warn("Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
				return false;
			}

			return BSClothUtils_BSTransformSet_QClothSupportsLOD_Handler.GetOriginal()(thisPtr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD: exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
			return false;
		}
	}

	void Hooked_BSClothUtils_BSTransformSet_ResetPrediction_void(void* thisPtr) {
		__try {
			if (!thisPtr || !IsReadableFast(thisPtr, sizeof(void*))) {
				logger::warn("Hooked_BSClothUtils_BSTransformSet_ResetPrediction_void: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}
			BSClothUtils_BSTransformSet_ResetPrediction_void_Handler.GetOriginal()(thisPtr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothUtils_BSTransformSet_ResetPrediction_void: exception at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

	// ===== Cloth helpers =====

	uint32_t GetClothInstanceCount(RE::BSClothExtraData* obj) {
		return *reinterpret_cast<uint32_t*>(reinterpret_cast<std::uintptr_t>(obj) + 0x70);
	}

	void** GetClothInstanceArray(RE::BSClothExtraData* obj) {
		return *reinterpret_cast<void***>(reinterpret_cast<std::uintptr_t>(obj) + 0x60);
	}

	int GetTransformSetCount(void* clothInstance) {
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(clothInstance) + 0x48);
	}

	void** GetTransformSetArray(void* clothInstance) {
		return *reinterpret_cast<void***>(reinterpret_cast<std::uintptr_t>(clothInstance) + 0x40);
	}

	// Старая сигнатура — теперь прокси к проверке игрового TLS-слота
	bool HasTLSArray() {
		return HasGameTLSGuardSlot();
	}

	// ResolveGameTlsIndex() ищет байт-паттерн "44 8B 05" (mov r8d,[rip+disp]) в ЖИВЫХ
	// байтах Fallout4.exe::BSClothExtraData::SetWorld (kBSClothExtraData_SetWorld_RVA),
	// чтобы вычислить адрес TLS-индекса. Но это ТОТ ЖЕ адрес, что хукает
	// BSClothExtraData_SetWorld_Handler (0x1DA6C30) — а DetourXS для 64-битного джампа
	// перезаписывает первые ~14-17 байт функции, включая саму искомую инструкцию.
	// Если ResolveGameTlsIndex вызвать ВПЕРВЫЕ уже ПОСЛЕ установки этого хука (а он
	// вызывается лениво изнутри Hooked_BSClothExtraData_SetWorld), скан ничего не
	// находит, HasGameTLSGuardSlot кэширует s_ok=false НАВСЕГДА — и с этого момента
	// АБСОЛЮТНО ЛЮБОЙ вызов SetWorld (с любого потока, включая игровой) считается
	// "не с игрового потока" и оригинал никогда не вызывается: ткань никогда не
	// крепится к физическому миру — физика одежды полностью и необратимо пропадает.
	// Фикс: прогреть кэш здесь, вызовом ДО Install() — тогда скан видит чистые,
	// ещё не пропатченные байты.
	void PrewarmClothTlsGuard()
	{
		GetGameTlsIndexPtr();
	}

	// Легкая валидация без IsBadReadPtr/IsBadWritePtr — проверяем массивы через VirtualQuery и null-check'и
	bool ValidateAllBSTransformSets(RE::BSClothExtraData* obj) {
		if (!obj)
			return false;

		const uint32_t ciCount = GetClothInstanceCount(obj);
		if (ciCount == 0)
			return true;

		void** ciArray = GetClothInstanceArray(obj);
		if (!ciArray)
			return false;

		// Проверяем, что массив hkRefPtr<hclClothInstance> читаем целиком
		if (!IsReadableFast(ciArray, sizeof(void*) * ciCount))
			return false;

		for (uint32_t ci = 0; ci < ciCount; ++ci) {
			void* clothInstance = ciArray[ci];
			if (!clothInstance)
				return false;

			const int tsCount = GetTransformSetCount(clothInstance);
			if (tsCount <= 0)
				continue;

			void** tsArray = GetTransformSetArray(clothInstance);
			if (!tsArray)
				return false;

			// Проверяем, что массив трансформов читаем целиком
			if (!IsReadableFast(tsArray, sizeof(void*) * tsCount))
					return false;

				// Проверяем каждый трансформ на null — движок обращается к [ts+0x19C] без проверки
				for (int t = 0; t < tsCount; ++t) {
					void* ts = tsArray[t];
					if (!ts)
						return false;
					if (!IsReadableFast(reinterpret_cast<std::uint8_t*>(ts) + 0x19C, sizeof(std::uint32_t)))
						return false;
				}
		}
		return true;
	}

	void Hooked_BSClothExtraData_SetWorld(RE::BSClothExtraData* thisPtr, void* newWorld)
	{
		if (!thisPtr) {
			return;
		}

		__try {
			// Setup TLS task slot (0x32 = cloth task id)
			uint32_t* taskSlot = nullptr;
			uint32_t oldTask = 0;
			const auto* tlsIdxPtr = GetGameTlsIndexPtr();
			if (tlsIdxPtr) {
				uint32_t tlsIndex = *tlsIdxPtr;
				auto* tlsArray = reinterpret_cast<uint8_t**>(__readgsqword(0x58));
				if (tlsArray && tlsArray[tlsIndex]) {
					taskSlot = reinterpret_cast<uint32_t*>(tlsArray[tlsIndex] + 0x9C0);
					oldTask = *taskSlot;
					*taskSlot = 0x32;
				}
			}

			auto* thisBytes = reinterpret_cast<uint8_t*>(thisPtr);
			void* oldWorld = *reinterpret_cast<void**>(thisBytes + 0x78);

			if (newWorld == oldWorld) {
				if (taskSlot) *taskSlot = oldTask;
				return;
			}

			uint32_t count = *reinterpret_cast<uint32_t*>(thisBytes + 0x70);
			if (count == 0) {
				if (taskSlot) *taskSlot = oldTask;
				return;
			}

			if (oldWorld) {
				using RemoveCloth_t = void(__fastcall*)(void*, void*);
				REL::Relocation<RemoveCloth_t>{ 0x141871C90 }(oldWorld, thisBytes + 0x60);
			}

			HavokAddRef(newWorld);
			HavokRelease(oldWorld);

			*reinterpret_cast<void**>(thisBytes + 0x78) = newWorld;

			if (!newWorld) {
				if (taskSlot) *taskSlot = oldTask;
				return;
			}

			using AddCloth_t = void(__fastcall*)(void*, void*);
			REL::Relocation<AddCloth_t>{ 0x141871BD0 }(newWorld, thisBytes + 0x60);

			count = *reinterpret_cast<uint32_t*>(thisBytes + 0x70);
			if (count == 0) {
				if (taskSlot) *taskSlot = oldTask;
				return;
			}

			auto** arr = *reinterpret_cast<uint8_t***>(thisBytes + 0x60);
			if (!arr) {
				if (taskSlot) *taskSlot = oldTask;
				return;
			}

			using SetLOD_t = void(__fastcall*)(void*, uint32_t);
			REL::Relocation<SetLOD_t> SetLOD{ 0x1418C8370 };

			using QClothSupportsLOD_t = bool(__fastcall*)(void*);
			REL::Relocation<QClothSupportsLOD_t> QClothSupportsLOD{ 0x1418A79F0 };

			using ResetPrediction_t = void(__fastcall*)(void*);
			REL::Relocation<ResetPrediction_t> ResetPrediction{ 0x1418A6070 };

			for (uint32_t i = 0; i < count; ++i) {
				auto* clothInstance = arr[i];
				if (!clothInstance) continue;

				int32_t numTransformSets = *reinterpret_cast<int32_t*>(clothInstance + 0x48);
				auto** transformSets = *reinterpret_cast<uint8_t***>(clothInstance + 0x40);

				uint32_t lod = 0;
				if (numTransformSets > 0 && transformSets) {
					auto* ts0 = transformSets[0];
					// CRITICAL NULL GUARD: Check ts0 before calling QClothSupportsLOD!
					// In vanilla Fallout4.exe, physics hair (Dragonfly, KS Hairdos) or outfits
					// with missing bones pass ts0 = nullptr, causing QClothSupportsLOD to dereference
					// [rcx + 0x19C] at Fallout4.exe+18A79F0 with rcx=0 (EXCEPTION_ACCESS_VIOLATION).
					if (ts0 && IsReadableFast(ts0 + 0x198, sizeof(uint32_t))) {
						if (QClothSupportsLOD(ts0)) {
							lod = *reinterpret_cast<uint32_t*>(ts0 + 0x198);
						}
					}
				}

				SetLOD(clothInstance, lod);

				if (numTransformSets > 0 && transformSets) {
					for (int32_t j = 0; j < numTransformSets; ++j) {
						auto* ts = transformSets[j];
						// CRITICAL NULL GUARD: Check ts before calling ResetPrediction
						// and writing lodPrediction (0xFFFFFFFF) at ts + 0x1A0!
						if (ts && IsWritableFast(ts + 0x1A0, sizeof(uint32_t))) {
							ResetPrediction(ts);
							*reinterpret_cast<uint32_t*>(ts + 0x1A0) = 0xFFFFFFFF;
						}
					}
				}
			}

			if (taskSlot) *taskSlot = oldTask;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothExtraData_SetWorld: exception handled");
		}
	}
}