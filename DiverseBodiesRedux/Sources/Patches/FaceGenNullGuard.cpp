#include "FaceGenNullGuard.h"
#include "PatchesMemUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstring>
#include <cstdint>

namespace patches {
	namespace {
		// Fallout 4 Next-Gen 1.11.240.0 FaceGen Engine RVAs
		constexpr std::uintptr_t kFaceGen_NullGuard_RVA          = 0x6E0880;
		constexpr std::uintptr_t kFaceGen_Resume_RVA             = 0x6E088F;
		constexpr std::uintptr_t kFaceGen_Bypass_RVA             = 0x6E0B53;

		// Expected 15 bytes at 0x6E0880
		// 48 8B 55 E7             mov rdx, [rbp-0x19]
		// 45 8B DC                mov r11d, r12d
		// 44 0F B7 92 34 01 00 00 movzx r10d, word ptr [rdx+0x134]
		constexpr std::uint8_t kExpectedPrologue[15] = {
			0x48, 0x8b, 0x55, 0xe7, 0x45, 0x8b, 0xdc,
			0x44, 0x0f, 0xb7, 0x92, 0x34, 0x01, 0x00, 0x00
		};

		bool g_nullGuardInstalled = false;
	}

	bool InstallFaceGenNullGuard()
	{
		if (g_nullGuardInstalled) {
			return true;
		}

		const auto base = reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr));
		if (!base) {
			logger::error("[FaceGenNullGuard] Failed to get Fallout4.exe base address.");
			return false;
		}

		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		if (!IsReadableFast(dos, sizeof(IMAGE_DOS_HEADER)) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
			logger::error("[FaceGenNullGuard] Invalid DOS header at Fallout4.exe base (0x{:X}).", base);
			return false;
		}

		// -------------------------------------------------------------
		// Install FaceGen Null Pointer Guard Trampoline at RVA 0x6E0880
		// Guards against CTD if [rbp-0x19] is null (GetDescription)
		// -------------------------------------------------------------
		const auto patchAddr = base + kFaceGen_NullGuard_RVA;
		if (!IsReadableFast(reinterpret_cast<const void*>(patchAddr), sizeof(kExpectedPrologue))) {
			logger::error("[FaceGenNullGuard] Address Fallout4.exe+0x{:X} is not readable.", kFaceGen_NullGuard_RVA);
			return false;
		}

		if (std::memcmp(reinterpret_cast<const void*>(patchAddr), kExpectedPrologue, sizeof(kExpectedPrologue)) != 0) {
			logger::warn("[FaceGenNullGuard] Bytes at Fallout4.exe+0x{:X} mismatch expected prologue; skipping patch.",
				kFaceGen_NullGuard_RVA);
			return false;
		}

		// Allocate executable memory for the 48-byte stub
		void* stubMem = ::VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, F4SE::WinAPI::PAGE_EXECUTE_READWRITE);
		if (!stubMem) {
			logger::error("[FaceGenNullGuard] VirtualAlloc failed for null guard stub.");
			return false;
		}

		const std::uint64_t resumeAddr = base + kFaceGen_Resume_RVA;
		const std::uint64_t bypassAddr = base + kFaceGen_Bypass_RVA;

		std::uint8_t* p = reinterpret_cast<std::uint8_t*>(stubMem);

		// mov rdx, [rbp-0x19]
		p[0] = 0x48; p[1] = 0x8B; p[2] = 0x55; p[3] = 0xE7;
		// test rdx, rdx
		p[4] = 0x48; p[5] = 0x85; p[6] = 0xD2;
		// jz +0x19 (.bypass at offset 34 / 0x22)
		p[7] = 0x74; p[8] = 0x19;
		// mov r11d, r12d
		p[9] = 0x45; p[10] = 0x8B; p[11] = 0xDC;
		// movzx r10d, word ptr [rdx+0x134]
		p[12] = 0x44; p[13] = 0x0F; p[14] = 0xB7; p[15] = 0x92;
		p[16] = 0x34; p[17] = 0x01; p[18] = 0x00; p[19] = 0x00;
		// jmp qword ptr [rip+0] -> resumeAddr
		p[20] = 0xFF; p[21] = 0x25; p[22] = 0x00; p[23] = 0x00; p[24] = 0x00; p[25] = 0x00;
		std::memcpy(p + 26, &resumeAddr, sizeof(resumeAddr));

		// .bypass:
		// jmp qword ptr [rip+0] -> bypassAddr
		p[34] = 0xFF; p[35] = 0x25; p[36] = 0x00; p[37] = 0x00; p[38] = 0x00; p[39] = 0x00;
		std::memcpy(p + 40, &bypassAddr, sizeof(bypassAddr));

		::FlushInstructionCache(::GetCurrentProcess(), stubMem, 48);

		// Write 15-byte absolute jump at patchAddr:
		// FF 25 00 00 00 00 <8-byte stubMem> 90
		std::uint8_t patchBytes[15];
		patchBytes[0] = 0xFF;
		patchBytes[1] = 0x25;
		patchBytes[2] = 0x00;
		patchBytes[3] = 0x00;
		patchBytes[4] = 0x00;
		patchBytes[5] = 0x00;
		const std::uint64_t stubAddrVal = reinterpret_cast<std::uint64_t>(stubMem);
		std::memcpy(patchBytes + 6, &stubAddrVal, sizeof(stubAddrVal));
		patchBytes[14] = 0x90; // NOP to pad to 15 bytes

		DWORD oldProt = 0;
		if (::VirtualProtect(reinterpret_cast<void*>(patchAddr), sizeof(patchBytes), F4SE::WinAPI::PAGE_EXECUTE_READWRITE, &oldProt)) {
			std::memcpy(reinterpret_cast<void*>(patchAddr), patchBytes, sizeof(patchBytes));
			::VirtualProtect(reinterpret_cast<void*>(patchAddr), sizeof(patchBytes), oldProt, &oldProt);
			::FlushInstructionCache(::GetCurrentProcess(), reinterpret_cast<void*>(patchAddr), sizeof(patchBytes));
			logger::info("[FaceGenNullGuard] Successfully installed FaceGen null guard (stub 0x{:X}) at Fallout4.exe+0x{:X}.",
				stubAddrVal, kFaceGen_NullGuard_RVA);
			g_nullGuardInstalled = true;
			return true;
		} else {
			logger::error("[FaceGenNullGuard] VirtualProtect failed on Fallout4.exe+0x{:X}.", kFaceGen_NullGuard_RVA);
			return false;
		}
	}
}
