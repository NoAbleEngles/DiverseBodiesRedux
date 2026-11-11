#include "Ini/Ini.h"
#include "BodyTattoos.h"
#include <PugiXML/pugixml.hpp>
#include <thread>

std::set<std::string> BodyTattoosPreset::ALL_ITEMS_M{};
std::set<std::string> BodyTattoosPreset::ALL_ITEMS_F{};

void BodyTattoosPreset::addOverlaysFromThisToPossibleOverlays() {
	if (!empty()) {
		auto& store = m_conditions.gender() == RE::Actor::Sex::Male ? ALL_ITEMS_M : ALL_ITEMS_F;
		for (auto& el : m_overlays)
			store.emplace(el.id());
	}
}

BodyTattoosPreset::BodyTattoosPreset() : OverlayPreset() {}

BodyTattoosPreset::BodyTattoosPreset(const std::filesystem::path& path) :
	OverlayPreset(path.string()) {
	addOverlaysFromThisToPossibleOverlays();
}

BodyTattoosPreset::BodyTattoosPreset(const std::string& path) :
	OverlayPreset(path) {
	addOverlaysFromThisToPossibleOverlays();
}

const std::set<std::string> BodyTattoosPreset::getAllPossibleMaleOverlays() noexcept {
	return ALL_ITEMS_M;
}

const std::set<std::string> BodyTattoosPreset::getAllPossibleFemaleOverlays() noexcept {
	return ALL_ITEMS_F;
}

void BodyTattoosPreset::revalidateAllPossibleOverlays(const std::set<std::string>& AllValidOverlays) {
	std::set<std::string_view> temp{};
	for (const auto& overlay : ALL_ITEMS_M) {
		if (AllValidOverlays.contains(overlay.data())) {
			temp.emplace(overlay);
		}
	}
}

bool BodyTattoosPreset::operator==(const Preset& other) const noexcept {
	auto ptrOther = preset_cast<const BodyTattoosPreset*>(&other);
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

bool BodyTattoosPreset::operator<(const Preset& other) const noexcept
{
	return OverlayPreset::operator<(other);
}

PresetType BodyTattoosPreset::type() const noexcept {
	return PRESET_TYPE;
}

bool BodyTattoosPreset::remove(RE::Actor* actor) const {
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

bool BodyTattoosPreset::apply(RE::Actor* actor, bool reset3d) const {
	auto res = OverlayPreset::apply(actor, reset3d); // Используем базовый метод для применения оверлеев
	
	return res;
}

std::future<bool> BodyTattoosPreset::isValidAsync() const noexcept {
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