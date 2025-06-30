#include "Ini/ini.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include "Preset.h"
#include <PugiXML/pugixml.hpp>


BodyhairsPreset::BodyhairsPreset() :
	Preset(std::string{}, PresetType::BODYHAIRS) {}

BodyhairsPreset::BodyhairsPreset(const std::filesystem::path& path) :
	Preset(path.string(), PresetType::BODYHAIRS)
{
	if (!loadFromFile(path.string())) {
		logger::error("Failed to load BodyhairsPreset from path: {}", path.string());
	};
}

BodyhairsPreset::BodyhairsPreset(const std::string& path) :
	Preset(path, PresetType::BODYHAIRS)
{
	if (!loadFromFile(path)) {
		logger::error("Failed to load BodyhairsPreset from path: {}", path);
	};
}

Preset& BodyhairsPreset::operator=(const Preset& other) noexcept {
	if (this != &other) {
		Preset::operator=(other);
		if (auto bp_other = dynamic_cast<const BodyhairsPreset*>(&other); bp_other) {
			*this = *bp_other;
		}
		else {
			logger::error("Failed to cast Preset to BodyhairsPreset in assignment operator.");
		}
	}
	return *this;
}

Preset& BodyhairsPreset::operator=(Preset&& other) noexcept
{
	if (this != &other) {
		if (auto bp_other = dynamic_cast<BodyhairsPreset*>(&other); bp_other) {
			*this = std::move(*bp_other);
		}
		else {
			logger::error("Failed to cast Preset to BodyhairsPreset in move assignment operator.");
		}
	}
	return *this;
}

BodyhairsPreset& BodyhairsPreset::operator=(const BodyhairsPreset& other) noexcept
{
	if (this != &other) {
		Preset::operator=(other);
	}
	return *this;
}

BodyhairsPreset& BodyhairsPreset::operator=(BodyhairsPreset&& other) noexcept
{
	if (this != &other) {
		Preset::operator=(std::move(other));
	}
	return *this;
}

bool BodyhairsPreset::operator==(const Preset& other) const noexcept
{
	auto bp_other = dynamic_cast<const BodyhairsPreset*>(&other);
	if (bp_other)
		return *this == *bp_other;
	return Preset::operator==(other);
}

bool BodyhairsPreset::operator==(const BodyhairsPreset& other) const noexcept
{
	return Preset::operator==(other);
}

bool BodyhairsPreset::operator<(const Preset& other) const noexcept
{
	auto bp_other = dynamic_cast<const BodyhairsPreset*>(&other);
	if (bp_other)
		return *this < *bp_other;

	return Preset::operator<(other);
}

bool BodyhairsPreset::operator<(const BodyhairsPreset& other) const noexcept
{
	return Preset::operator<(other);
}

bool BodyhairsPreset::isCondtionsEmpty() const noexcept
{
	return Preset::m_conditions.empty();
}

CoincidenceLevel BodyhairsPreset::check(const RE::Actor* actor, Filter filter) const noexcept
{
	return Preset::check(actor, filter);  // Проверяем базовые условия из Preset
}

bool BodyhairsPreset::apply(RE::Actor* actor) const
{
	if (!actor) {
		logger::info("BodyhairsPreset Apply no actor provided!");
		return false;
	}

	if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
		logger::info("BodyhairsPreset Apply no actor provided!");
		return false;
	}

	if (check(actor) == CoincidenceLevel::NONE) {
		logger::info("BodyhairsPreset Apply check failed for actor: {:#x}", actor->formID);
		return false;
	}

	remove(actor);

	if (!actor->GetFullyLoaded3D()) {
		return false;
	}

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::critical("OverlayInterface is nullptr!");
		return false;
	}

	auto overlaysUIDs = findOverlaysUid(actor, m_overlaysToRemove);

	for (const auto& uid : overlaysUIDs) {
		Interface->RemoveOverlay(actor, actor->GetSex() == RE::Actor::Female, uid);
	}

	for (const auto& overlay : m_overlays) {
		overlay.apply(actor);
	}

	return true;
}

bool BodyhairsPreset::remove(RE::Actor* actor) const
{
	if (!actor || !actor->GetFullyLoaded3D())
		return false;

	if (m_overlays.empty())
		return true;  // Nothing to remove

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::error("BodyMorphInterface is nullptr!");
		return false;
	}

	std::vector<std::string> overlayIds(m_overlays.size());
	for (size_t i = 0; i < m_overlays.size(); ++i) {
		overlayIds[i] = m_overlays[i].id();
	}

	auto overlaysUIDs = findOverlaysUid(actor, overlayIds);

	for (const auto& overlay : overlaysUIDs) {
		Interface->RemoveOverlay(actor, actor->GetSex() == RE::Actor::Female, overlay);
	}
}

bool BodyhairsPreset::empty() const noexcept
{
	return id().empty();
}

std::string BodyhairsPreset::id() const noexcept
{
	return Preset::id();
}

std::future<bool> BodyhairsPreset::isValidAsync() const noexcept {
	// Если пресет пустой — сразу false
	if (empty()) {
		return std::async(std::launch::deferred, [] { return false; });
	}

	// Копируем id оверлеев для потокобезопасности
	std::vector<std::string> overlayIds;
	overlayIds.reserve(m_overlays.size());
	for (const auto& overlay : m_overlays) {
		overlayIds.push_back(overlay.id());
	}

	// Для каждого id запускаем асинхронную валидацию
	std::vector<std::future<bool>> futures;
	futures.reserve(overlayIds.size());
	for (const auto& id : overlayIds) {
		futures.push_back(validate(id));
	}

	// Возвращаем общий future, который завершится, когда все проверки завершатся
	return std::async(std::launch::async, [futures = std::move(futures), overlayIds, this]() mutable {
		bool anyValid = false;
		for (size_t i = 0; i < futures.size(); ++i) {
			// Ждём завершения каждой проверки, но не блокируем основной поток, т.к. isValidAsync сам асинхронный
			if (futures[i].get()) {
				anyValid = true;
			}
		}
		if (!anyValid) {
			logger::error("BodyhairsPreset {} is not valid: no valid overlays found.", id().empty() ? "[no id]" : id());
		}
		return anyValid;
	});
}

bool BodyhairsPreset::loadFromFile(const std::string& presetFile)
{
	this->clear();  // Очищаем предыдущие данные, чтобы избежать конфликтов

	auto path = std::filesystem::path(presetFile);
	if (!std::filesystem::exists(path)) {
		logger::error("BodyMorphs preset file does not exist: {}", presetFile);
		return false;
	}

	if (path.extension() == ".json") {
		boost::json::object json_obj;
		try {
			json_obj = boost::json::parse(getJson(path)).as_object();
		} catch (...) {
			logger::info("...FAILED : {} wrong json format", presetFile);
			return false;
		}

		auto it = json_obj.find("add");

		if (it != json_obj.end()) {
			if (auto val = it->value(); val.is_array()) {
				for (const auto& item : val.as_array()) {
					auto overlay = Overlay{ item.as_object() };
					if (!overlay.empty()) {
						m_overlays.emplace_back(overlay);
					}
				}
			}
			else {
				logger::info("...FAILED : wrong remove value type in {}", presetFile);
			}
		}
		else {
			logger::error("BodyhairsPreset::loadFromFile: 'add' key not found in JSON preset file: {}", presetFile);
			clear();  // Очищаем данные, если пресет не валиден
			return false;
		}

		if (m_overlays.empty()) {
			logger::info("...FAILED : no overlays in {}", presetFile);
			clear();  // Очищаем данные, если пресет не валиден
			return false;
		}

		it = json_obj.find("remove");

		if (it != json_obj.end()) {
			if (auto val = it->value(); val.is_array()) {
				for (const auto& item : val.as_array()) {
					if (item.is_string()) {
						m_overlaysToRemove.emplace_back(item.as_string().c_str());
					}
					else {
						logger::info("...FAILED : wrong remove value type in {}", presetFile);
					}
				}
			}
			else {
				logger::info("...FAILED : wrong remove value type in {}", presetFile);
			}
		}

		it = json_obj.find("conditions");

		if (it != json_obj.end()) {
			m_conditions = std::move(ConditionSettings(it->value().as_object()));
		}

	} else {
		logger::critical("BodyhairsPreset::loadFromFile: Unsupported file format: {}", presetFile);
		clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	m_id = path.stem().string();
	logger::info("BodyhairsPreset::loadFromFile: {}. Loaded {} overlays, {} overlays to remove.", presetFile, m_overlays.size(), m_overlaysToRemove.size());
	return true;
}

// @breif Очищает объект
void BodyhairsPreset::clear() noexcept
{
	*this = std::move(BodyhairsPreset{});  // Используем оператор присваивания для очистки
}

BodyhairsPreset* BodyhairsPreset::clone() const {
	return new BodyhairsPreset{ *this };
}

// DiverseBodiesRedux\Sources\Preset\Bodyhairs.cpp
#include <sstream>

std::string BodyhairsPreset::print() const
{
	std::ostringstream oss;
	oss << "BodyhairsPreset: " << id() << "\n";
	oss << "  Type: " << static_cast<int>(m_type) << "\n";
	oss << "  Overlays: ";
	if (m_overlays.empty()) {
		oss << "(none)\n";
	}
	else {
		oss << "\n";
		for (const auto& overlay : m_overlays) {
			oss << "    id=" << overlay.id() << "\n";
		}
	}
	oss << "  OverlaysToRemove: ";
	if (m_overlaysToRemove.empty()) {
		oss << "(none)\n";
	}
	else {
		for (const auto& id : m_overlaysToRemove) {
			oss << id << " ";
		}
		oss << "\n";
	}
	oss << "  Conditions: ";
	if (m_conditions.empty()) {
		oss << "(none)\n";
	}
	else {
		oss << m_conditions.print();
	}
	return oss.str();
}