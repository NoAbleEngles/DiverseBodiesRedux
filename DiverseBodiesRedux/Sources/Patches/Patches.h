#pragma once
#include "PCH.h"
#include <Hooks/Hooks.h>
#include <globals.h>

namespace patches {
	// BSClothExtraData SetSettle patch
	using BSClothExtraData_SetSettle_t = void(__fastcall*)(RE::BSClothExtraData* thisPtr, bool enable);
	void Hooked_BSClothExtraData_SetSettle(RE::BSClothExtraData* thisPtr, bool enable);
	extern Hooks<BSClothExtraData_SetSettle_t> BSClothExtraData_SetSettle_Handler;

	// SetTransformSet patch
	using SetTransformSet_Handler_t = void(__fastcall*)(void*, uint32_t, void*);
	void Hooked_SetTransformSet(void* instance, uint32_t index, void* transformSet);
	extern Hooks<SetTransformSet_Handler_t> SetTransformSet_Handler;
	void Hooked_SetTransformSet2(void* instance, uint32_t index, void* transformSet);
	extern Hooks<SetTransformSet_Handler_t> SetTransformSet2_Handler;

	// ChangeHeadPart patches
	void removeWithExtra(RE::TESNPC* npc, RE::BGSHeadPart* hpart);
	void ProcessChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts, bool isRemove);

	using ChangeHeadPartRemovePartHandler_t = void(__fastcall*)(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts);
	void Hooked_ChangeHeadPartRemovePart(RE::TESNPC* npc, RE::BGSHeadPart* hpart, bool bRemoveExtraParts);
	extern Hooks<ChangeHeadPartRemovePartHandler_t> ChangeHeadPartRemovePart;

	using ChangeHeadPartHandler_t = void(__fastcall*)(RE::TESNPC* npc, RE::BGSHeadPart* hpart);
	void Hooked_ChangeHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* hpart);
	extern Hooks<ChangeHeadPartHandler_t> ChangeHeadPart;

	// CBP.dll patch
	using cbp_sub_1800024C0_t = bool(__fastcall*)(void* rcx, uint32_t arg1, uint64_t arg2, uint64_t arg3);
	bool Hooked_cbp_sub_1800024C0(void* rcx, uint32_t arg1, uint64_t arg2, uint64_t arg3);
	extern Hooks<cbp_sub_1800024C0_t> cbp_sub_1800024C0_Hook;

	// bhkStream LoadBinary patch
	using bhkStream_LoadBinary_t = void(__fastcall*)(void* thisPtr, void* stream, void* rootContainer);
	void Hooked_bhkStream_LoadBinary(void* thisPtr, void* stream, void* rootContainer);
	extern Hooks<bhkStream_LoadBinary_t> bhkStream_LoadBinary_Handler;

	// Havok deserialization caller patch
	using HavokDeserializationCaller_t = void(__fastcall*)(void* thisPtr, void* stream);
	void Hooked_HavokDeserializationCaller(void* thisPtr, void* stream);
	extern Hooks<HavokDeserializationCaller_t> HavokDeserializationCaller_Handler;

	// AddHeadPart patch
	using AddHeadPartHandler_t = void(__fastcall*)(RE::TESNPC* npc, RE::BGSHeadPart* headPart, bool checkUnique, bool addExtra, bool updateActor);
	void Hooked_AddHeadPart(RE::TESNPC* npc, RE::BGSHeadPart* headPart, bool checkUnique, bool addExtra, bool updateActor);
	extern Hooks<AddHeadPartHandler_t> AddHeadPart;

	// FaceGen pending-head-data guard.
	// AIProcess::PendingActorHeadData processor (Fallout4.exe+0x67B2D0) читает на IO-потоке
	// pending->npc->formRace (TESNPC+0x1B8) -> race->data.flags. Если актёр выгрузился и его
	// динамический/шаблонный TESNPC освобождён — указатель висячий -> EXCEPTION_ACCESS_VIOLATION.
	// Хук проверяет NPC и расу и молча выбрасывает устаревшую запись вместо вылета.
	using PendingActorHeadData_Process_t = bool(__fastcall*)(void* self);
	bool Hooked_PendingActorHeadData_Process(void* self);
	extern Hooks<PendingActorHeadData_Process_t> PendingActorHeadData_Process;
	// BSClothExtraData SetSettleOnTransitionToSim patch
	using BSClothExtraData__SetSettleOnTransitionToSim_Bool_handler_t = void(__fastcall*)(RE::BSClothExtraData* thisPtr, bool enable);
	void Hooked_BSClothExtraData__SetSettleOnTransitionToSim_Bool(RE::BSClothExtraData* thisPtr, bool enable);
	extern Hooks<BSClothExtraData__SetSettleOnTransitionToSim_Bool_handler_t> BSClothExtraData__SetSettleOnTransitionToSim_Bool_Handler;

	bool ValidateClothChain(RE::BSClothExtraData* thisPtr);

	// BSClothUtils patches
	using BSClothUtils_BSTransformSet_QClothSupportsLOD_handler_t = bool(__fastcall*)(void* thisPtr);
	bool Hooked_BSClothUtils_BSTransformSet_QClothSupportsLOD(void* thisPtr);
	extern Hooks<BSClothUtils_BSTransformSet_QClothSupportsLOD_handler_t> BSClothUtils_BSTransformSet_QClothSupportsLOD_Handler;

	using BSClothUtils_BSTransformSet_ResetPrediction_void_handler_t = void(__fastcall*)(void* thisPtr);
	void Hooked_BSClothUtils_BSTransformSet_ResetPrediction_void(void* thisPtr);
	extern Hooks<BSClothUtils_BSTransformSet_ResetPrediction_void_handler_t> BSClothUtils_BSTransformSet_ResetPrediction_void_Handler;

	// BSClothExtraData SetWorld patch
	using BSClothExtraData_SetWorld_handler_t = void(__fastcall*)(RE::BSClothExtraData* thisPtr, void* newWorld);
	void Hooked_BSClothExtraData_SetWorld(RE::BSClothExtraData* thisPtr, void* newWorld);
	extern Hooks<BSClothExtraData_SetWorld_handler_t> BSClothExtraData_SetWorld_Handler;

	// Utility functions for BSClothExtraData validation
	uint32_t GetClothInstanceCount(RE::BSClothExtraData* obj);
	void** GetClothInstanceArray(RE::BSClothExtraData* obj);
	int GetTransformSetCount(void* clothInstance);
	void** GetTransformSetArray(void* clothInstance);

	// Старая сигнатура оставлена для совместимости — теперь она проверяет игровой TLS-слот
	bool HasTLSArray();

	// ВАЖНО: вызывать ДО BSClothExtraData_SetWorld_Handler.Install() — см. Patches.cpp.
	// Прогревает кэш TLS-индекса, который HasGameTLSGuardSlot() читает из живых байт
	// самой функции BSClothExtraData::SetWorld — после установки хука на этот же адрес
	// их читать уже поздно (детур их перезаписывает).
	void PrewarmClothTlsGuard();

	// Валидация без IsBadReadPtr/IsBadWritePtr: быстрые проверки массивов и null-check
	bool ValidateAllBSTransformSets(RE::BSClothExtraData* obj);
}