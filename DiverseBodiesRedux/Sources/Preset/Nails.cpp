#include "Ini/ini.h"
#include "Nails.h"
#include <PugiXML/pugixml.hpp>
#include <thread>

std::set<std::string> NailsPreset::ALL_ITEMS_M{};
std::set<std::string> NailsPreset::ALL_ITEMS_F{};

void NailsPreset::addOverlaysFromThisToPossibleOverlays() {
	if (!empty()) {
		auto& store = m_conditions.gender() == RE::Actor::Sex::Male ? ALL_ITEMS_M : ALL_ITEMS_F;
		for (auto& el : m_overlays)
			store.emplace(el.id());
	}
}

NailsPreset::NailsPreset() : OverlayPreset() {}

NailsPreset::NailsPreset(const std::filesystem::path& path) :
	OverlayPreset(path.string()) {
	addOverlaysFromThisToPossibleOverlays();
}

NailsPreset::NailsPreset(const std::string& path) :
	OverlayPreset(path) {
	addOverlaysFromThisToPossibleOverlays();
}

const std::set<std::string> NailsPreset::getAllPossibleMaleOverlays() noexcept {
	return ALL_ITEMS_M;
}

const std::set<std::string> NailsPreset::getAllPossibleFemaleOverlays() noexcept {
	return ALL_ITEMS_F;
}

void NailsPreset::revalidateAllPossibleOverlays(const std::set<std::string>& AllValidOverlays) {
	std::set<std::string_view> temp{};
	for (const auto& overlay : ALL_ITEMS_M) {
		if (AllValidOverlays.contains(overlay.data())) {
			temp.emplace(overlay);
		}
	}
}

bool NailsPreset::operator==(const Preset& other) const noexcept {
	auto ptrOther = preset_cast<const NailsPreset*>(&other);
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

bool NailsPreset::operator<(const Preset& other) const noexcept
{
	return OverlayPreset::operator<(other);
}

PresetType NailsPreset::type() const noexcept {
	return PRESET_TYPE;
}

bool NailsPreset::remove(RE::Actor* actor) const {
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
	static std::unordered_set<RE::Actor*> processingActors{};
	if (processingActors.contains(actor)) {
		return false; // Предотвращаем повторную обработку одного и того же актёра
	}
	processingActors.insert(actor); // Добавляем актёра в список обрабатываемых

	for (const auto& uid : overlaysUIDs) {
		if (Interface->RemoveOverlay(actor, actor->GetSex() == RE::Actor::Female, uid) && !once) once = true;
	}

	processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых

	return once;
}

bool NailsPreset::apply(RE::Actor* actor, bool reset3d) const {
	static std::unordered_set<RE::Actor*> processingActors{};
	if (processingActors.contains(actor)) {
		return false; // Предотвращаем повторную обработку одного и того же актёра
	}
	processingActors.insert(actor); // Добавляем актёра в список обрабатываемых

	auto res = OverlayPreset::apply(actor, reset3d); // Используем базовый метод для применения оверлеев

	processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых

	return res;
}

std::future<bool> NailsPreset::isValidAsync() const noexcept {
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