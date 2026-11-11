#include "FacialMorphIntensityFix.h"
#include "PatchesMemUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstring>
#include <cstdint>

namespace patches {
	namespace {
		// Fallout 4 Next-Gen 1.11.240.0 RVAs:
		// RVA 0x6503F2: 5-byte CALL in TESNPC record loader that reads FMIN subrecord and calls SetFacialBoneMorphIntensity.
		// Expected: E8 C9 49 00 00 -> CALL Fallout4.exe+0x654DC0
		constexpr std::uintptr_t kFMIN_LoaderCall_RVA = 0x6503F2;
		constexpr std::uint8_t kExpectedLoaderCall[5] = { 0xE8, 0xC9, 0x49, 0x00, 0x00 };

		// RVA 0x654E20: TESNPC::GetFacialBoneMorphIntensity (Address Library ID 2207416)
		// Expected prologue: 53 48 83 EC 30 -> push rbx; sub rsp, 0x30
		constexpr std::uintptr_t kGetFacialBoneMorphIntensity_RVA = 0x654E20;
		constexpr std::uint8_t kExpectedGetterPrologue[5] = { 0x53, 0x48, 0x83, 0xEC, 0x30 };

		bool g_fminFixInstalled = false;
	}

	bool InstallFacialMorphIntensityFix()
	{
		if (g_fminFixInstalled) {
			return true;
		}

		const auto base = reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr));
		if (!base) {
			logger::error("[FMIN_Fix] Failed to get Fallout4.exe base address.");
			return false;
		}

		// 1. Patch the loader call at RVA 0x6503F2 with 5 NOPs so FMIN is NEVER loaded from any plugin into memory
		const auto loaderCallAddr = base + kFMIN_LoaderCall_RVA;
		if (IsReadableFast(reinterpret_cast<const void*>(loaderCallAddr), sizeof(kExpectedLoaderCall)) &&
			std::memcmp(reinterpret_cast<const void*>(loaderCallAddr), kExpectedLoaderCall, sizeof(kExpectedLoaderCall)) == 0) {
			DWORD oldProt = 0;
			if (::VirtualProtect(reinterpret_cast<void*>(loaderCallAddr), 5, F4SE::WinAPI::PAGE_EXECUTE_READWRITE, &oldProt)) {
				constexpr std::uint8_t nops[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
				std::memcpy(reinterpret_cast<void*>(loaderCallAddr), nops, 5);
				::VirtualProtect(reinterpret_cast<void*>(loaderCallAddr), 5, oldProt, &oldProt);
				::FlushInstructionCache(::GetCurrentProcess(), reinterpret_cast<void*>(loaderCallAddr), 5);
				logger::info("[FMIN_Fix] Successfully patched TESNPC FMIN record loader at Fallout4.exe+0x{:X} with NOPs.", kFMIN_LoaderCall_RVA);
			} else {
				logger::error("[FMIN_Fix] VirtualProtect failed on Fallout4.exe+0x{:X}.", kFMIN_LoaderCall_RVA);
			}
		} else {
			logger::warn("[FMIN_Fix] Bytes at Fallout4.exe+0x{:X} mismatch expected loader CALL; skipping loader patch.", kFMIN_LoaderCall_RVA);
		}

		// 2. Patch GetFacialBoneMorphIntensity at RVA 0x654E20 to always return 1.0f in XMM0:
		// B8 00 00 80 3F -> mov eax, 0x3F800000 (1.0f)
		// 66 0F 6E C0    -> movd xmm0, eax
		// C3             -> ret
		const auto getterAddr = base + kGetFacialBoneMorphIntensity_RVA;
		if (IsReadableFast(reinterpret_cast<const void*>(getterAddr), sizeof(kExpectedGetterPrologue)) &&
			std::memcmp(reinterpret_cast<const void*>(getterAddr), kExpectedGetterPrologue, sizeof(kExpectedGetterPrologue)) == 0) {
			DWORD oldProt = 0;
			constexpr std::uint8_t returnOneStub[10] = {
				0xB8, 0x00, 0x00, 0x80, 0x3F, // mov eax, 0x3F800000 (1.0f)
				0x66, 0x0F, 0x6E, 0xC0,       // movd xmm0, eax
				0xC3                          // ret
			};
			if (::VirtualProtect(reinterpret_cast<void*>(getterAddr), sizeof(returnOneStub), F4SE::WinAPI::PAGE_EXECUTE_READWRITE, &oldProt)) {
				std::memcpy(reinterpret_cast<void*>(getterAddr), returnOneStub, sizeof(returnOneStub));
				::VirtualProtect(reinterpret_cast<void*>(getterAddr), sizeof(returnOneStub), oldProt, &oldProt);
				::FlushInstructionCache(::GetCurrentProcess(), reinterpret_cast<void*>(getterAddr), sizeof(returnOneStub));
				logger::info("[FMIN_Fix] Successfully patched TESNPC::GetFacialBoneMorphIntensity at Fallout4.exe+0x{:X} to return 1.0f.", kGetFacialBoneMorphIntensity_RVA);
			} else {
				logger::error("[FMIN_Fix] VirtualProtect failed on Fallout4.exe+0x{:X}.", kGetFacialBoneMorphIntensity_RVA);
			}
		} else {
			logger::warn("[FMIN_Fix] Bytes at Fallout4.exe+0x{:X} mismatch expected getter prologue; skipping getter patch.", kGetFacialBoneMorphIntensity_RVA);
		}

		g_fminFixInstalled = true;
		return true;
	}
}
