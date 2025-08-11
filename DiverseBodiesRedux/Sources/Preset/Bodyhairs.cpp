#include "Ini/ini.h"
#include "Bodyhairs.h"
#include <PugiXML/pugixml.hpp>
#include <thread>

std::set<std::string> BodyhairsPreset::ALL_ITEMS_M{};
std::set<std::string> BodyhairsPreset::ALL_ITEMS_F{};

void BodyhairsPreset::addOverlaysFromThisToPossibleOverlays() {
	if (!empty()) {
		auto& store = m_conditions.gender() == RE::Actor::Sex::Male ? ALL_ITEMS_M : ALL_ITEMS_F;
		for (auto& el : m_overlays)
			store.emplace(el.id());
	}
}

BodyhairsPreset::BodyhairsPreset() : OverlayPreset() {}

BodyhairsPreset::BodyhairsPreset(const std::filesystem::path& path) :
	OverlayPreset(path.string()) {
	addOverlaysFromThisToPossibleOverlays();
}

BodyhairsPreset::BodyhairsPreset(const std::string& path) :
	OverlayPreset(path) {
	addOverlaysFromThisToPossibleOverlays();
}

const std::set<std::string> BodyhairsPreset::getAllPossibleMaleOverlays() noexcept {
	return ALL_ITEMS_M;
}

const std::set<std::string> BodyhairsPreset::getAllPossibleFemaleOverlays() noexcept {
	return ALL_ITEMS_F;
}

void BodyhairsPreset::revalidateAllPossibleOverlays(const std::set<std::string>& AllValidOverlays) {
	std::set<std::string_view> temp{};
	for (const auto& overlay : ALL_ITEMS_M) {
		if (AllValidOverlays.contains(overlay.data())) {
			temp.emplace(overlay);
		}
	}
}

bool BodyhairsPreset::operator==(const Preset& other) const noexcept {
	auto ptrOther = preset_cast<const BodyhairsPreset*>(&other);
	if (!ptrOther) {
		return false;
	}

	if (OverlayPreset::operator==(other)) {
		if (auto otherOverlayPreset = preset_cast<OverlayPreset*>(&other); otherOverlayPreset) {
			return *static_cast<const OverlayPreset*>(this) == other;
		}
	}
	return false;
}

bool BodyhairsPreset::operator<(const Preset& other) const noexcept
{
	return OverlayPreset::operator<(other);
}

PresetType BodyhairsPreset::type() const noexcept {
	return PRESET_TYPE;
}

void BodyhairsPreset::clear() noexcept {
	OverlayPreset::clear();
}

bool BodyhairsPreset::remove(RE::Actor* actor) const {
	if (!actor || !actor->GetFullyLoaded3D())
		return false;
	
	bool isFemale = actor->GetSex() == RE::Actor::Sex::Female;
	auto overlaysUIDs = findOverlaysUid(actor, isFemale ? ALL_ITEMS_F : ALL_ITEMS_M);

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::critical("OverlayInterface is nullptr!");
		return false;
	}

	bool once = false;
	for (const auto& uid : overlaysUIDs) {
		if (Interface->RemoveOverlay(actor, actor->GetSex() == RE::Actor::Female, uid) && !once) once = true;
	}

	return once;
}

std::future<bool> BodyhairsPreset::isValidAsync() const noexcept {
	static bool runOnce = false;
	if (!runOnce) {
		runOnce = true;
		std::set<std::string> validM, validF;
		auto& validator = ValidateOverlay::validateOverlay();

		// Проверяем мужские оверлеи
		for (const auto& id : ALL_ITEMS_M) {
			auto fut = validator(id);
			if (fut.get()) {
				validM.insert(id);
			}
		}
		// Проверяем женские оверлеи
		for (const auto& id : ALL_ITEMS_F) {
			auto fut = validator(id);
			if (fut.get()) {
				validF.insert(id);
			}
		}
		ALL_ITEMS_M.swap(validM);
		ALL_ITEMS_F.swap(validF);
	}
	return OverlayPreset::isValidAsync(); // Вызов базового метода для проверки валидности оверлеев
}