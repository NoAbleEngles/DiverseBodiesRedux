#include "F4SE/Impl/PCH.h"
#include "RE/Bethesda/Actor.h"

#include "RE/Bethesda/TESBoundAnimObjects.h"
#include "RE/Bethesda/TESBoundObjects.h"

namespace RE
{
	TESNPC* Actor::GetNPC() const noexcept
	{
		const auto objRef = GetObjectReference();
		assert(objRef->GetFormType() == ENUM_FORM_ID::kNPC_);
		return static_cast<TESNPC*>(objRef);
	}

	Actor::Sex Actor::GetSex() const noexcept
	{
		const auto npc = GetNPC();
		return npc ? static_cast<Sex>(npc->GetSex()) : Sex::None;
	}
}

