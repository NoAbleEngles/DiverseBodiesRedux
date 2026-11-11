#include "F4SE/Impl/PCH.h"
#include "RE/Bethesda/TESObjectREFRs.h"

#include "RE/NetImmerse/NiAVObject.h"

namespace RE
{
	BIPOBJECT::~BIPOBJECT()
	{
		stl::memzero(this);
	}

	RESET_3D_FLAGS operator|(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) |
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	RESET_3D_FLAGS operator&(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) &
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	RESET_3D_FLAGS operator^(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) ^
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	RESET_3D_FLAGS operator~(RESET_3D_FLAGS flags)
	{
		return static_cast<RESET_3D_FLAGS>(
			~static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(flags));
	}

	RESET_3D_FLAGS& operator|=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	RESET_3D_FLAGS& operator&=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	RESET_3D_FLAGS& operator^=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	RESET_3D_FLAGS& operator~(RESET_3D_FLAGS& flags)
	{
		flags = static_cast<RESET_3D_FLAGS>(~static_cast<uint16_t>(flags));  
		return flags; 
	}

	bool HasFlag(RESET_3D_FLAGS value, RESET_3D_FLAGS flag)
	{
		return (static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(value) &
				   static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(flag)) != 0;
	}
}

