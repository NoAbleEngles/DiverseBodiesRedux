#pragma once
#include "PCH.h"
#include <Hooks/Hooks.h>

namespace patches {
	using BodyMorphProcessor_Process_t = void(__fastcall*)(
		void* a_this,
		void* a_args,
		const char* a_modelName,
		RE::NiPointer<RE::NiNode>* a_root,
		std::uint32_t* a_typeOut);

	extern Hooks<BodyMorphProcessor_Process_t> BodyMorphProcessor_Process_Hook;

	void __fastcall Hooked_BodyMorphProcessor_Process(
		void* a_this,
		void* a_args,
		const char* a_modelName,
		RE::NiPointer<RE::NiNode>* a_root,
		std::uint32_t* a_typeOut);

	bool InstallClothMorphFilter();
}
