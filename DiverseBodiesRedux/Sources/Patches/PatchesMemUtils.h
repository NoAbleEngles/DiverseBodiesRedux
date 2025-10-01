#pragma once
#include <cstdint>
#include <atomic>

namespace patches
{
	// ========= Memory checks (faster than IsBadReadPtr/IsBadWritePtr) =========

	// Быстрая проверка: адрес в коммитнутом регионе с правами чтения и размером не меньше minSize
	bool IsReadableFast(const void* p, std::size_t minSize = sizeof(void*));

	// Быстрая проверка: адрес в коммитнутом регионе с правами записи и размером не меньше minSize
	bool IsWritableFast(void* p, std::size_t minSize = sizeof(void*));

	// Легкая “проба” чтения с SEH — используйте только как редкую страховку
	template <class T>
	inline bool SafeProbeRead(const T* p, T& out)
	{
#ifdef _MSC_VER
		if (!p) return false;
		__try {
			out = *reinterpret_cast<volatile const T*>(p);
			return true;
		}
		__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
			? EXCEPTION_EXECUTE_HANDLER
			: EXCEPTION_CONTINUE_SEARCH) {
			return false;
		}
#else
		if (!p) return false;
		out = *p;
		return true;
#endif
	}

	// Легкая “проба” записи с SEH — используйте только как редкую страховку
	template <class T>
	inline bool SafeProbeWrite(T* p, const T& val)
	{
#ifdef _MSC_VER
		if (!p) return false;
		__try {
			*reinterpret_cast<volatile T*>(p) = val;
			return true;
		}
		__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
			? EXCEPTION_EXECUTE_HANDLER
			: EXCEPTION_CONTINUE_SEARCH) {
			return false;
		}
#else
		if (!p) return false;
		*p = val;
		return true;
#endif
	}

	// ========= Game TLS guard (Fallout4.exe TLS[TlsIndex] slot) =========

	// RVA BSClothExtraData::SetWorld(hclBSWorld*)
	static constexpr std::uintptr_t kBSClothExtraData_SetWorld_RVA = 0x1DA6C30;

	// В первых ~0x80 байтах функции есть: 44 8B 05 xx xx xx xx  (mov r8d, dword ptr [rip+disp32]) -> cs:TlsIndex
	bool ResolveGameTlsIndex(std::uint32_t& outIdx);

	// Проверяет, что TLS[TlsIndex] != nullptr для текущего потока (как в игре).
	// Это главный сторож от падения на Fallout4.exe+1DA6C5C.
	bool HasGameTLSGuardSlot();

	// Безопасный вызов оригинала с перехватом AV (не даем упасть процессу).
	template <class TFunc, class... Args>
	inline void CallOriginalSafe(TFunc fn, Args... args)
	{
#ifdef _MSC_VER
		__try {
			fn(args...);
		}
		__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
			? EXCEPTION_EXECUTE_HANDLER
			: EXCEPTION_CONTINUE_SEARCH) {
			// проглатываем, чтобы не уронить процесс
		}
#else
		fn(args...);
#endif
	}
}