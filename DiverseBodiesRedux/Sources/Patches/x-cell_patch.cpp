#include "x-cell_patch.h"

// Static member definition
const std::unordered_map<xcell_patch::XCellVersion, std::unordered_map<xcell_patch::XCellProperty, uintptr_t>> 
xcell_patch::s_offsetMap = {
	{
		xcell_patch::XCellVersion::v2_1_b126,
		{
			{xcell_patch::XCellProperty::FacegenExceptionFormIDs, 0x142F68},
		}
	}
};

bool xcell_patch::addExceptionFormID(uint32_t formID) {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (!ensureInitialized()) {
		logger::error("x-cell patch: not initialized, cannot add formID {:08X}", formID);
		return false;
	}

	if (!isValidFormID(formID)) {
		logger::warn("x-cell patch: invalid formID {:08X}", formID);
		return false;
	}

	if (!m_exceptionFormIDs) {
		logger::error("x-cell patch: exception form IDs vector is null");
		return false;
	}

	// Check if already exists
	if (std::find(m_exceptionFormIDs->begin(), m_exceptionFormIDs->end(), formID) != m_exceptionFormIDs->end()) {
		logger::debug("x-cell patch: formID {:08X} already in exception list", formID);
		return false;
	}

	try {
		m_exceptionFormIDs->push_back(formID);
		logger::info("x-cell patch: added exception formID {:08X} (total: {})", formID, m_exceptionFormIDs->size());
		return true;
	}
	catch (const std::exception& e) {
		logger::error("x-cell patch: failed to add formID {:08X}: {}", formID, e.what());
		return false;
	}
}

bool xcell_patch::removeExceptionFormID(uint32_t formID) {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (!ensureInitialized() || !m_exceptionFormIDs) {
		return false;
	}

	auto it = std::find(m_exceptionFormIDs->begin(), m_exceptionFormIDs->end(), formID);
	if (it != m_exceptionFormIDs->end()) {
		m_exceptionFormIDs->erase(it);
		logger::info("x-cell patch: removed exception formID {:08X} (total: {})", formID, m_exceptionFormIDs->size());
		return true;
	}

	logger::debug("x-cell patch: formID {:08X} not found in exception list", formID);
	return false;
}

bool xcell_patch::hasExceptionFormID(uint32_t formID) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (!m_initialized || !m_exceptionFormIDs) {
		return false;
	}

	return std::find(m_exceptionFormIDs->begin(), m_exceptionFormIDs->end(), formID) != m_exceptionFormIDs->end();
}

size_t xcell_patch::getExceptionFormIDCount() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (!m_initialized || !m_exceptionFormIDs) {
		return 0;
	}

	return m_exceptionFormIDs->size();
}

void xcell_patch::clearExceptionFormIDs() {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	if (!m_initialized || !m_exceptionFormIDs) {
		return;
	}

	size_t count = m_exceptionFormIDs->size();
	m_exceptionFormIDs->clear();
	logger::info("x-cell patch: cleared {} exception formIDs", count);
}

xcell_patch::VersionInfo xcell_patch::getVersionInfo() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_versionInfo;
}

bool xcell_patch::isInitialized() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_initialized;
}

bool xcell_patch::reinitialize() {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	m_initialized = false;
	m_exceptionFormIDs = nullptr;
	m_versionInfo = {};
	
	return initializeInternal();
}

bool xcell_patch::ensureInitialized() {
	if (m_initialized) {
		return true;
	}
	
	return initializeInternal();
}

bool xcell_patch::initializeInternal() {
	logger::debug("x-cell patch: initializing...");

	auto versionOpt = detectXCellVersion();
	if (!versionOpt) {
		logger::warn("x-cell patch: x-cell plugin not detected or not loaded");
		return false;
	}

	m_versionInfo = *versionOpt;
	
	if (!m_versionInfo.supported) {
		logger::error("x-cell patch: unsupported x-cell version {}", m_versionInfo.versionString);
		return false;
	}

	m_xcellBaseAddress = GetModule("x-cell-og.dll");
	if (!m_xcellBaseAddress) {
		logger::error("x-cell patch: failed to get x-cell module base address");
		return false;
	}

	if (!isValidModuleAddress(m_xcellBaseAddress)) {
		logger::error("x-cell patch: invalid x-cell module address: {:016X}", m_xcellBaseAddress);
		return false;
	}

	auto offsetIt = s_offsetMap.find(m_versionInfo.version);
	if (offsetIt == s_offsetMap.end()) {
		logger::error("x-cell patch: no offsets defined for version {}", m_versionInfo.versionString);
		return false;
	}

	auto propertyIt = offsetIt->second.find(XCellProperty::FacegenExceptionFormIDs);
	if (propertyIt == offsetIt->second.end()) {
		logger::error("x-cell patch: no FacegenExceptionFormIDs offset for version {}", m_versionInfo.versionString);
		return false;
	}

	uintptr_t exceptionFormIDsOffset = propertyIt->second;
	uintptr_t exceptionFormIDsAddress = m_xcellBaseAddress + exceptionFormIDsOffset;

	if (!isValidAddress(exceptionFormIDsAddress)) {
		logger::error("x-cell patch: invalid exception form IDs address: {:016X}", exceptionFormIDsAddress);
		return false;
	}

	m_exceptionFormIDs = reinterpret_cast<std::vector<uint32_t>*>(exceptionFormIDsAddress);
	
	if (!isValidVectorPointer(m_exceptionFormIDs)) {
		logger::error("x-cell patch: invalid exception form IDs vector pointer");
		m_exceptionFormIDs = nullptr;
		return false;
	}

	m_initialized = true;
	logger::info("x-cell patch: initialized successfully for {} (base: {:016X})", 
				 m_versionInfo.versionString, m_xcellBaseAddress);
	
	return true;
}

std::optional<xcell_patch::VersionInfo> xcell_patch::detectXCellVersion() const {
	auto xcellInfo = F4SE::GetPluginInfo("x-cell");
	if (!xcellInfo.has_value()) {
		return std::nullopt;
	}

	VersionInfo info{};
	info.version = static_cast<XCellVersion>(xcellInfo->version);
	
	uint32_t versionInt = static_cast<uint32_t>(info.version);
	uint8_t major = (versionInt >> 24) & 0xFF;
	uint8_t minor = (versionInt >> 16) & 0xFF;
	uint16_t build = (versionInt >> 4) & 0xFFF;
	uint8_t sub = versionInt & 0xF;
	
	if (sub > 0) {
		info.versionString = fmt::format("{}.{}.{}.{}", major, minor, build, sub);
	} else {
		info.versionString = fmt::format("{}.{}.{}", major, minor, build);
	}

	info.supported = s_offsetMap.find(info.version) != s_offsetMap.end();

	return info;
}

bool xcell_patch::isValidFormID(uint32_t formID) const {
	return formID != 0 && formID != 0xFFFFFFFF;
}

bool xcell_patch::isValidModuleAddress(uintptr_t address) const {
#ifdef _WIN64
	return address >= 0x10000 && address < 0x7FFFFFFFFFFF;
#else
	return address >= 0x10000 && address < 0x80000000;
#endif
}

bool xcell_patch::isValidAddress(uintptr_t address) const {
	return isValidModuleAddress(address);
}

bool xcell_patch::isValidVectorPointer(const std::vector<uint32_t>* vec) const {
	if (!vec) {
		return false;
	}

	try {
		[[maybe_unused]] auto size = vec->size();
		[[maybe_unused]] auto capacity = vec->capacity();
		
		return capacity < 1000000;
	}
	catch (...) {
		return false;
	}
}