#pragma once
#include "PCH.h"
#include <Hooks/Hooks.h>
#include <globals.h>

namespace patches {
	//с этим вылетает 0304E698 DLC03_Armor_Trapper_Suit_Hunter (equipitem 0304E698)
	using BSTransformSetHandler_t = bool(__fastcall*)(void* instance, uint32_t index, void* transformSet); 
	bool Hooked_BSSetTransformSet(void* instance, uint32_t index, void* transformSet);

	Hooks<BSTransformSetHandler_t> BSSetTransformSetHandler{ "", 0x1DAF3B0, Hooked_BSSetTransformSet };

	// патч
	inline bool Hooked_BSSetTransformSet(void* instance, uint32_t index, void* transformSet) {
		if (!transformSet) {  // Проверка на nullptr
			logger::warn("HookedSetTransformSet: transformSet is null. Skipping...");
			return false;
		}
		// Вызов оригинальной функции, если проверка пройдена
		return BSSetTransformSetHandler.GetOriginal()(instance, index, transformSet);
	}

// ######################################################################################################################################

	//Вылеты BSClothInstance
	using BSClothExtraData_SetSettle_t = void(__fastcall*)(void* thisPtr, bool enable);
	void Hooked_BSClothExtraData_SetSettle(void* thisPtr, bool enable);

	Hooks<BSClothExtraData_SetSettle_t> BSClothExtraData_SetSettle_Handler{ "", 0x1DA7A40, Hooked_BSClothExtraData_SetSettle };

	// патч
	inline void Hooked_BSClothExtraData_SetSettle(void* thisPtr, bool enable) {
		__try {
			// Проверка thisPtr
			if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}

			uint8_t* rcx_ptr = static_cast<uint8_t*>(thisPtr);

			// Проверка смещений внутри структуры
			if (IsBadReadPtr(rcx_ptr + 0x60, sizeof(uint8_t**)) || IsBadReadPtr(rcx_ptr + 0x70, sizeof(uint32_t))) {
				logger::warn("Hooked_BSClothExtraData_SetSettle: Corrupted structure at 0x{:x}", reinterpret_cast<uintptr_t>(thisPtr));
				return;
			}

			uint32_t edx_val = *reinterpret_cast<uint32_t*>(rcx_ptr + 0x70);
			uint8_t** rax_ptr = reinterpret_cast<uint8_t**>(rcx_ptr + 0x60);
			uint8_t** r8_ptr = rax_ptr + edx_val;

			// Обработка каждого элемента в массиве
			while (rax_ptr < r8_ptr) {
				// Проверка указателя rax_ptr
				if (IsBadReadPtr(rax_ptr, sizeof(uint8_t*))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid array pointer");
					break;
				}

				uint8_t* current = *rax_ptr;
				// Проверка current
				if (!current || IsBadReadPtr(current, sizeof(uint8_t))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Skipping invalid cloth instance");
					rax_ptr++;
					continue;
				}

				// Проверка current + 0x48
				if (IsBadReadPtr(current + 0x48, sizeof(uint32_t))) {
					logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid count field");
					rax_ptr++;
					continue;
				}

				if (*reinterpret_cast<uint32_t*>(current + 0x48) > 0) {
					// Проверка current + 0x40
					if (IsBadReadPtr(current + 0x40, sizeof(uint8_t*))) {
						logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid transform array");
						rax_ptr++;
						continue;
					}

					uint8_t* inner_rcx = *reinterpret_cast<uint8_t**>(current + 0x40);
					// Проверка inner_rcx
					if (!inner_rcx || IsBadReadPtr(inner_rcx, sizeof(uint8_t*))) {
						logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid transform data");
						rax_ptr++;
						continue;
					}

					uint8_t* rdx_val = *reinterpret_cast<uint8_t**>(inner_rcx);
					// Проверка rdx_val и целевого адреса
					if (!rdx_val || IsBadReadPtr(rdx_val + 0x1B7, sizeof(bool))) {
						logger::warn("Hooked_BSClothExtraData_SetSettle: Invalid settle flag address");
						rax_ptr++;
						continue;
					}

					*reinterpret_cast<bool*>(rdx_val + 0x1B7) = enable;
				}
				rax_ptr++;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Hooked_BSClothExtraData_SetSettle: Critical error at 0x{:x}", reinterpret_cast<uintptr_t>(_ReturnAddress()));
		}
	}

// ######################################################################################################################################

	//Вылеты BSClothInstance
	using SetTransformSet_Handler_t = void (__fastcall*)(void*, uint32_t, void*);
	void Hooked_SetTransformSet(void* instance, uint32_t index, void* transformSet);

	Hooks<SetTransformSet_Handler_t> SetTransformSet_Handler{ "", 0x1DDC7A0, Hooked_SetTransformSet };

	inline void Hooked_SetTransformSet(void* instance, uint32_t index, void* transformSet)
	{
		if (!transformSet) {  // Проверка на nullptr
			logger::warn("HookedSetTransformSet: transformSet is null. Skipping...");
			return;
		}
		// Вызов оригинальной функции, если проверка пройдена
		SetTransformSet_Handler.GetOriginal()(instance, index, transformSet);
	}

	// ######################################################################################################################################
	// с этим вылетает 0304E698 DLC03_Armor_Trapper_Suit_Hunter (equipitem 0304E698)
	using BSClothUtils__BSTransformSet__QClothSupportsLOD_Handler_t = bool (__fastcall*)(void* thisPtr);
	bool Hooked_QClothSupportsLOD(void* thisPtr);

	/*Hooks<BSClothUtils__BSTransformSet__QClothSupportsLOD_Handler_t> QClothSupportsLOD_Handler{ "", 0x1C3F3B0, Hooked_QClothSupportsLOD };

	inline bool Hooked_QClothSupportsLOD(void* thisPtr) {
		if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
			logger::warn("Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD: Invalid thisPtr (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
			return false;
		}

		__try {
			return QClothSupportsLOD_Handler.GetOriginal()(thisPtr);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger::error("Critical error in BSClothUtils::BSTransformSet::QClothSupportsLOD (RIP: 0x{:x})", reinterpret_cast<uintptr_t>(_ReturnAddress()));
			return false;
		}
	}*/

	// ######################################################################################################################################
	// Исправление для ChangeHeadPartRemovePart

	void removeWithExtra(RE::TESNPC* npc, RE::BGSHeadPart* hpart);
	void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove);
	
	using ChangeHeadPartRemovePartHandler_t = void(__fastcall*)(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts);
	void Hooked_ChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts);

	Hooks<ChangeHeadPartRemovePartHandler_t> ChangeHeadPartRemovePart{ "", 5985632, Hooked_ChangeHeadPartRemovePart };

	inline void Hooked_ChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts) {
		ProcessChangeHeadPart(npc, hpart, bRemoveExtraParts, true);
	}

	using ChangeHeadPartHandler_t = void(__fastcall*)(RE::TESNPC* npc, RE::BGSHeadPart* hpart);
	void Hooked_ChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart);

	Hooks<ChangeHeadPartHandler_t> ChangeHeadPart{ "", 6003504, Hooked_ChangeHeadPart };

	inline void Hooked_ChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
		ProcessChangeHeadPart(npc, hpart, false, false);
	}

	// Удаляет headpart и все вложенные extraParts
	inline void removeWithExtra(RE::TESNPC* npc, RE::BGSHeadPart* hpart) {
		// Используем стек для обхода в глубину
		std::vector<RE::BGSHeadPart*> stack;
		stack.push_back(hpart);

		while (!stack.empty()) {
			RE::BGSHeadPart* current = stack.back();
			stack.pop_back();

			// Сначала добавляем все extraParts в стек
			for (auto it = current->extraParts.rbegin(); it != current->extraParts.rend(); ++it) {
				RE::BGSHeadPart* extra = *it;
				if (!extra->extraParts.empty()) {
					stack.push_back(extra);
				}
				else {
					// Если нет вложенных extraParts, сразу удаляем
					ChangeHeadPartRemovePart.GetOriginal()(npc, extra, false);
				}
			}
			// После обработки всех extraParts удаляем сам current
			ChangeHeadPartRemovePart.GetOriginal()(npc, current, false);
		}
	}

	inline void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove)
	{
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

// ######################################################################################################################################
	//Unhandled exception "EXCEPTION_ACCESS_VIOLATION" at 0x7FF66D89B2F7 Fallout4.exe+067B2F7

	//using UpdateLoadState_t = void(__fastcall*)(void* thisPtr);
	//void Hooked_UpdateLoadState(void* thisPtr);

	//Hooks<UpdateLoadState_t> UpdateLoadStateHandler{ "", 0xACBB2D0, Hooked_UpdateLoadState };

	//inline void Hooked_UpdateLoadState(void* thisPtr)
	//{
	//	if (!thisPtr || IsBadReadPtr(thisPtr, sizeof(void*))) {
	//		logger::warn("UpdateLoadState: thisPtr is invalid (0x{:x})", reinterpret_cast<uintptr_t>(thisPtr));
	//		return;
	//	}

	//	uint8_t* rbx = static_cast<uint8_t*>(thisPtr);

	//	// Если уже обработано — просто выставляем флаг и выходим
	//	if (rbx[0x82] != 0) {
	//		rbx[0x82] = 1;
	//		return;
	//	}

	//	// Проверка field_60
	//	auto ptr1 = *reinterpret_cast<void**>(rbx + 0x60);
	//	if (!ptr1 || IsBadReadPtr(ptr1, sizeof(void*))) {
	//		logger::warn("UpdateLoadState: field_60 is invalid or nullptr");
	//		rbx[0x82] = 1;
	//		return;
	//	}

	//	// Проверка *(ptr1 + 0x1B8)
	//	auto ptr2 = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr1) + 0x1B8);
	//	if (!ptr2 || IsBadReadPtr(ptr2, 0x1A0 + sizeof(uint32_t))) {
	//		logger::warn("UpdateLoadState: *(field_60+0x1B8) is invalid or nullptr");
	//		rbx[0x82] = 1;
	//		return;
	//	}

	//	// Проверка *(ptr2 + 0x1A0)
	//	uint32_t val = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(ptr2) + 0x1A0);
	//	val >>= 1;
	//	if (!(val & 1)) {
	//		rbx[0x82] = 1;
	//		return;
	//	}

	//	// Всё в порядке — вызываем оригинальную функцию
	//	__try {
	//		UpdateLoadStateHandler.GetOriginal()(thisPtr);
	//	}
	//	__except (EXCEPTION_EXECUTE_HANDLER) {
	//		logger::error("UpdateLoadState: exception in original (RIP: 0x{:x})", reinterpret_cast<uintptr_t>(_ReturnAddress()));
	//	}

	//	// Как в оригинале, выставляем флаг
	//	rbx[0x82] = 1;
	//}
}