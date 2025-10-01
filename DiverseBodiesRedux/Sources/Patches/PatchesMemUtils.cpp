#include "PatchesMemUtils.h"
#define PAGE_EXECUTE_READWRITE_ 0x40

namespace patches
{
	bool IsReadableFast(const void* p, std::size_t minSize)
	{
		if (!p) return false;

		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery(p, &mbi, sizeof(mbi)))
			return false;

		if ((mbi.State & MEM_COMMIT) == 0)
			return false;

		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			return false;

		const DWORD prot = mbi.Protect & 0xFF;
		switch (prot) {
		case PAGE_READONLY:
		case PAGE_READWRITE:
		case PAGE_WRITECOPY:
		case PAGE_EXECUTE_READ:
		case PAGE_EXECUTE_READWRITE_:
		case PAGE_EXECUTE_WRITECOPY:
			break;
		default:
			return false;
		}

		const auto start = reinterpret_cast<std::uintptr_t>(p);
		const auto base = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
		const auto off = start - base;
		return (off + minSize) <= mbi.RegionSize;
	}

	bool IsWritableFast(void* p, std::size_t minSize)
	{
		if (!p) return false;

		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery(p, &mbi, sizeof(mbi)))
			return false;

		if ((mbi.State & MEM_COMMIT) == 0)
			return false;

		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			return false;

		const DWORD prot = mbi.Protect & 0xFF;
		switch (prot) {
		case PAGE_READWRITE:
		case PAGE_WRITECOPY:
		case PAGE_EXECUTE_READWRITE_:
		case PAGE_EXECUTE_WRITECOPY:
			break;
		default:
			return false;
		}

		const auto start = reinterpret_cast<std::uintptr_t>(p);
		const auto base = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
		const auto off = start - base;
		return (off + minSize) <= mbi.RegionSize;
	}

	bool ResolveGameTlsIndex(std::uint32_t& outIdx)
	{
		const auto base = reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr)); // Fallout4.exe base (0x140000000)
		const auto fn = reinterpret_cast<const std::uint8_t*>(base + kBSClothExtraData_SetWorld_RVA);
		if (!fn) return false;

		// »щем сигнатуру: 44 8B 05 xx xx xx xx
		for (std::size_t i = 0; i < 0x80; ++i) {
			const auto p = fn + i;
			if (p[0] == 0x44 && p[1] == 0x8B && p[2] == 0x05) {
				const auto disp = *reinterpret_cast<const std::int32_t*>(p + 3);
				const auto next = reinterpret_cast<std::uintptr_t>(p + 7);
				const auto idxAddr = next + disp;
				if (!::IsBadReadPtr(reinterpret_cast<const void*>(idxAddr), sizeof(std::uint32_t))) {
					outIdx = *reinterpret_cast<const std::uint32_t*>(idxAddr);
					return true;
				}
				return false;
			}
		}
		return false;
	}

	bool HasGameTLSGuardSlot()
	{
		const auto tlsArray = reinterpret_cast<const void**>(__readgsqword(0x58));
		if (!tlsArray) return false;

		static std::atomic<bool> s_inited{ false };
		static std::uint32_t s_tlsIndex{ 0 };
		static bool s_ok{ false };

		if (!s_inited.load(std::memory_order_acquire)) {
			std::uint32_t idx{};
			s_ok = ResolveGameTlsIndex(idx);
			if (s_ok) s_tlsIndex = idx;
			s_inited.store(true, std::memory_order_release);
		}
		if (!s_ok) return false;

		return *(tlsArray + s_tlsIndex) != nullptr;
	}
}