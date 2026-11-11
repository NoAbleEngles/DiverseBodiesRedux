#pragma once

#include "REL/Relocation.h"
#include "REL/Version.h"

namespace REL
{
    // Constexpr helper to select between OG and AE IDs at compile time
    constexpr ID SelectVersionID(std::uint64_t a_ogID, std::uint64_t a_aeID) noexcept
    {
        if constexpr (GAME_VERSION == GameVersion::AE) {
            return ID(a_aeID);
        } else {
            return ID(a_ogID);
        }
    }

	constexpr ID SelectVersionID(const ID& a_ogID, const ID& a_aeID) noexcept
	{
		if constexpr (GAME_VERSION == GameVersion::AE) {
			return a_aeID;
		} else {
			return a_ogID;
		}
	}

	// Constexpr helper for VTABLE arrays
	constexpr std::array<ID, 1> SelectVersionVTABLE(std::uint64_t a_ogID, std::uint64_t a_aeID) noexcept
	{
		return { SelectVersionID(a_ogID, a_aeID) };
	}

	template<std::size_t N>
	constexpr std::array<ID, N> SelectVersionVTABLE(const std::array<ID, N>& a_ogVTABLE, const std::array<ID, N>& a_aeVTABLE) noexcept
	{
		if constexpr (GAME_VERSION == GameVersion::AE) {
			return a_aeVTABLE;
		} else {
			return a_ogVTABLE;
		}
	}

	// Constexpr helper to select between OG and AE offsets at compile time
	constexpr Offset SelectVersionOffset(std::size_t a_ogOffset, std::size_t a_aeOffset) noexcept
	{
		if constexpr (GAME_VERSION == GameVersion::AE) {
			return Offset(a_aeOffset);
		} else {
			return Offset(a_ogOffset);
		}
	}

	constexpr Offset SelectVersionOffset(const Offset& a_ogOffset, const Offset& a_aeOffset) noexcept
	{
		if constexpr (GAME_VERSION == GameVersion::AE) {
			return a_aeOffset;
		} else {
			return a_ogOffset;
		}
	}

	// Constexpr helper for VTABLE arrays using offsets
	constexpr std::array<Offset, 1> SelectVersionVTABLEOffset(std::size_t a_ogOffset, std::size_t a_aeOffset) noexcept
	{
		return { SelectVersionOffset(a_ogOffset, a_aeOffset) };
	}

	constexpr std::array<Offset, 1> SelectVersionVTABLEOffset(const Offset& a_ogOffset, const Offset& a_aeOffset) noexcept
	{
		return { SelectVersionOffset(a_ogOffset, a_aeOffset) };
	}

	template<std::size_t N>
	constexpr std::array<Offset, N> SelectVersionVTABLEOffset(const std::array<Offset, N>& a_ogVTABLE, const std::array<Offset, N>& a_aeVTABLE) noexcept
	{
		if constexpr (GAME_VERSION == GameVersion::AE) {
			return a_aeVTABLE;
		} else {
			return a_ogVTABLE;
		}
	}

	// Alias for RTTI - it's the same as SelectVersionOffset but with clearer naming
	constexpr Offset SelectVersionRTTI(std::size_t a_ogOffset, std::size_t a_aeOffset) noexcept
	{
		return SelectVersionOffset(a_ogOffset, a_aeOffset);
	}

	constexpr Offset SelectVersionRTTI(const Offset& a_ogOffset, const Offset& a_aeOffset) noexcept
	{
		return SelectVersionOffset(a_ogOffset, a_aeOffset);
	}
}
