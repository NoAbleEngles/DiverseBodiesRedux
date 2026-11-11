#include "F4SE/Impl/PCH.h"
#include "RE/Bethesda/BGSCharacterTint.h"

namespace RE::BGSCharacterTint {

// Template::Groups implementation
Template::Groups::Groups()
{
	REL::Relocation<void(Template::Groups*)> ctor{ REL::Offset(0x60C600) };
	ctor(this);
}

Template::Groups::~Groups()
{
	REL::Relocation<void(Template::Groups*)> dtor{ REL::Offset(0x2A5BE0) };
	dtor(this);
}

Template::Group* Template::Groups::AddGroup(const char* a_name)
{
	REL::Relocation<Template::Group*(Template::Groups*, const char*)> func{ REL::Offset(0x2A5C40) };
	return func(this, a_name);
}

Template::Entry* Template::Groups::GetFirstEntryForSlot(EntrySlot a_slot)
{
	REL::Relocation<Template::Entry*(Template::Groups*, EntrySlot)> func{ REL::Offset(0x2A5D80) };
	return func(this, a_slot);
}

// Entries implementation
Entries::Entries()
{
	// Default constructor implementation - just initialize to zero
	data = nullptr;
	capacity = 0;
	pad0C = 0;
	size = 0;
}

Entries::Entries(std::uint32_t a_reserve)
{
	REL::Relocation<void(Entries*, std::uint32_t)> ctor{ REL::Offset(0x5CB6A0) };
	ctor(this, a_reserve);
}

Entries::~Entries()
{
	REL::Relocation<void(Entries*)> dtor{ REL::Offset(0x5CC700) };
	dtor(this);
}

void Entries::Clear()
{
	if (data) {
		for (std::uint32_t i = 0; i < size; ++i) {
			delete data[i];
		}
	}
	size = 0;
}

void Entries::RemoveAt(std::uint32_t a_index)
{
	if (data && a_index < size) {
		delete data[a_index];
		for (std::uint32_t i = a_index; i + 1 < size; ++i) {
			data[i] = data[i + 1];
		}
		--size;
	}
}

void Entries::CopyFrom(Entries& a_rhs)
{
	Clear();
	for (std::uint32_t i = 0; i < a_rhs.size; ++i) {
		if (a_rhs.data && a_rhs.data[i]) {
			Add(a_rhs.data[i], nullptr);
		}
	}
}

void Entries::Add(Entry* a_entry, Template::Groups* a_groups)
{
	REL::Relocation<void(Entries*, Entry*, Template::Groups*)> func{ REL::SelectVersionID(1227376, 2196506) };
	func(this, a_entry, a_groups);
}

Entry* Entries::GetEntryByID(std::uint16_t a_id)
{
	for (std::size_t i = 0; i < size; ++i) {
		if (data && data[i] && data[i]->tintIndex == a_id) {
			return data[i];
		}
	}
	return nullptr;
}

Entry* Entries::GetFirstEntryForSlot(Template::Groups* a_groups, EntrySlot a_slot)
{
	REL::Relocation<Entry*(Entries*, Template::Groups*, EntrySlot)> func{ REL::Offset(0x2A47D0) };
	return func(this, a_groups, a_slot);
}

void Entries::MakeContiguous(Template::Groups* a_groups)
{
	REL::Relocation<void(Entries*, Template::Groups*)> func{ REL::Offset(0x2A48E0) };
	func(this, a_groups);
}

void Entries::MakeMinimal(Template::Groups* a_groups)
{
	REL::Relocation<void(Entries*, Template::Groups*)> func{ REL::Offset(0x2A4950) };
	func(this, a_groups);
}

void Entries::SetEntryData(Template::Groups& a_groups, std::uint16_t a_id, float a_value, std::uint32_t a_flags)
{
	REL::Relocation<void(Entries*, Template::Groups&, std::uint16_t, float, std::uint32_t)> func{ REL::Offset(0x2A4980) };
	func(this, a_groups, a_id, a_value, a_flags);
}

void Entries::Sort(Template::Groups* a_groups)
{
	REL::Relocation<void(Entries*, Template::Groups*)> func{ REL::Offset(0x2A4BC0) };
	func(this, a_groups);
}

void Entries::LoadGame(Template::Groups* a_groups, BGSLoadFormBuffer* a_buf, std::uint32_t a_arg)
{
	REL::Relocation<void(Entries*, Template::Groups*, BGSLoadFormBuffer*, std::uint32_t)> func{ REL::Offset(0x2A4D40) };
	func(this, a_groups, a_buf, a_arg);
}

void Entries::SaveGame(Template::Groups& a_groups, BGSSaveFormBuffer& a_buf)
{
	REL::Relocation<void(Entries*, Template::Groups&, BGSSaveFormBuffer&)> func{ REL::Offset(0x2A4F70) };
	func(this, a_groups, a_buf);
}

void Entries::SaveToBuffer(Template::Groups& a_groups, BSStreamer::ScrapStreamer& a_stream)
{
	REL::Relocation<void(Entries*, Template::Groups&, BSStreamer::ScrapStreamer&)> func{ REL::Offset(0x2A5030) };
	func(this, a_groups, a_stream);
}

void Entries::LoadFromBuffer(Template::Groups* a_groups, BSStreamer::ScrapStreamer& a_stream, std::uint32_t a_flags)
{
	REL::Relocation<void(Entries*, Template::Groups*, BSStreamer::ScrapStreamer&, std::uint32_t)> func{ REL::Offset(0x2A50E0) };
	func(this, a_groups, a_stream, a_flags);
}

} // namespace RE::BGSCharacterTint

