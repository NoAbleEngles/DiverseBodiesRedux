#include <windows.h>
#include "PatchesMemUtils.h"
#define PAGE_EXECUTE_READWRITE_ 0x40

namespace patches
{
	bool IsReadableFast(const void* p, std::size_t minSize)
	{
		if (!p) return false;
		const auto u = reinterpret_cast<std::uintptr_t>(p);
		return (u >= 0x10000ULL && u < 0x00007FFFFFFE0000ULL);
	}

	bool IsWritableFast(void* p, std::size_t minSize)
	{
		if (!p) return false;
		const auto u = reinterpret_cast<std::uintptr_t>(p);
		return (u >= 0x10000ULL && u < 0x00007FFFFFFE0000ULL);
	}

	static const std::uint32_t* s_gameTlsIndexPtr = nullptr;
	static std::atomic<bool> s_tlsPtrResolved{ false };

	const std::uint32_t* GetGameTlsIndexPtr()
	{
		if (s_tlsPtrResolved.load(std::memory_order_acquire)) {
			return s_gameTlsIndexPtr;
		}

		const auto base = reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr)); // Fallout4.exe base (0x140000000)
		if (!base) return nullptr;

		// RVA 0x3E73018: адрес переменной _tls_index в Fallout 4 (1.10.980+ / AE 1.11.240.0).
		// См. инструкцию в BSClothExtraData::SetWorld (RVA 0x18A0DAA): mov r8d, dword ptr [rip + 0x25d2267] -> 0x3E73018.
		const auto tlsIndexAddr = reinterpret_cast<const std::uint32_t*>(base + 0x3E73018);
		if (IsReadableFast(tlsIndexAddr, sizeof(std::uint32_t))) {
			s_gameTlsIndexPtr = tlsIndexAddr;
		}

		s_tlsPtrResolved.store(true, std::memory_order_release);
		return s_gameTlsIndexPtr;
	}

	bool ResolveGameTlsIndex(std::uint32_t& outIdx)
	{
		const auto* ptr = GetGameTlsIndexPtr();
		if (!ptr) return false;
		outIdx = *ptr;
		return outIdx < 1088; // TLS_OUT_OF_INDEXES = 0xFFFFFFFF, валидный диапазон Win32 TLS: 0..1087
	}

	bool HasGameTLSGuardSlot()
	{
		const auto tlsArray = reinterpret_cast<const void* const*>(__readgsqword(0x58));
		if (!tlsArray) return false;

		const auto* ptr = GetGameTlsIndexPtr();
		if (!ptr) return false;

		const auto idx = *ptr;
		if (idx >= 1088) return false;

		const void* slot = nullptr;
#ifdef _MSC_VER
		__try {
			slot = tlsArray[idx];
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
#else
		slot = tlsArray[idx];
#endif

		return slot != nullptr;
	}
}