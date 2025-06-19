#include "Preset.h"
#include <fstream>
#include "LooksMenu/LooksMenuInterfaces.h"

Preset::Preset(std::string id, PresetType type) :
	m_id (id), m_type(type) {}

Preset::~Preset() {};

std::string getJson(const std::filesystem::path& filepath) {
	std::ifstream file(filepath);
	if (!file) {
		throw std::runtime_error("Could not open file: " + filepath.string());
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

PresetType Preset::type() const noexcept { 
	return m_type; 
}

void Preset::clear() noexcept {
	m_id.clear();
	m_conditions.clear();
	m_type = PresetType::NONE;
}

Preset& Preset::operator=(const Preset& other) noexcept {
	if (this != &other) {
		m_id = other.m_id;
		m_type = other.m_type;
		m_conditions = other.m_conditions;
	}
	return *this;
}

Preset& Preset::operator=(Preset&& other) noexcept {
	if (this != &other) {
		m_id = std::move(other.m_id);
		m_type = std::move(other.m_type);
		m_conditions = std::move(other.m_conditions);
	}
	return *this;
}

bool Preset::operator==(const Preset&) const noexcept
{
	return m_id == m_id && m_type == m_type;
}

bool Preset::operator<(const Preset& other) const noexcept {
	if (m_type != other.m_type)
		return m_type < other.m_type;

	return m_id < other.m_id;
}

bool Preset::isCondtionsEmpty() const noexcept {
	return m_conditions.empty();
}

bool Preset::apply(RE::Actor*) const {
	logger::error("apply() not implemented for Preset class.");
	return false;  
}

bool Preset::check(RE::Actor* actor) const noexcept {
	return m_conditions.check(actor);
}

bool Preset::remove(RE::Actor*) const {
	logger::error("remove() not implemented for Preset class.");
	return false;
}

bool Preset::empty() const noexcept {
	logger::error("empty() not implemented for Preset class.");
	return true;
}

std::string Preset::id() const noexcept {
	return m_id;
}

bool Preset::isValid() const noexcept {
	logger::error("isValid() abstract base class can't be valid.");
	return false;
}

bool Preset::loadFromFile(const std::string&) {
	logger::error("loadFromFile() not implemented for Preset class.");
	return false;
}

std::vector<uint32_t> findOverlaysUid(RE::Actor* actor, const std::vector<std::string>& overlayIds) {
	std::vector<uint32_t> result{};
	
	if (!actor) {
		logger::error("Actor is null in findOverlayUid.");
		return result;
	}

	if (overlayIds.empty()) {
		logger::error("Overlay IDs are empty in findOverlayUid.");
		return result;
	}

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::critical("OverlayInterface is nullptr!");
		return result;
	}

	Interface->ForEachOverlay(
		actor,
		actor->GetSex() == RE::Actor::Female,
		[&result, &overlayIds] (int32_t uid, const OverlayInterface::OverlayDataPtr& overlay) {
			if (overlay && overlay->templateName) {
				for (auto overlayId : overlayIds) {
					if (strcmp(overlay->templateName.get()->c_str(), overlayId.c_str()) == 0) {
						result.push_back(uid);
					}
				}
			}
		}
	);

	return result;
}