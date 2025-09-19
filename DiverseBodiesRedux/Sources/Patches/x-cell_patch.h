#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <optional>
#include <algorithm>
#include <vector>
#include "PCH.h"

extern uintptr_t GetModule(const std::string& pluginName);

// Versioning macros for x-cell version encoding
#define MAKE_XCELL_VERSION_EX(major, minor, build, sub)	((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | (((build) & 0xFFF) << 4) | ((sub) & 0xF))
#define MAKE_XCELL_VERSION(major, minor, build)			MAKE_XCELL_VERSION_EX(major, minor, build, 0)

// Thread-safe singleton class for patching x-cell plugin functionality
class xcell_patch
{
public:
	enum class XCellVersion : uint32_t
	{
		Unknown = 0,
		v2_1_b126 = MAKE_XCELL_VERSION(2, 1, 125) // видимо Перчик забыл обновить версию в плагине
	};

	enum class XCellProperty : uint8_t
	{
		FacegenExceptionFormIDs = 0
	};

	struct VersionInfo
	{
		XCellVersion version = XCellVersion::Unknown;
		std::string versionString;
		bool supported = false;
	};

	static xcell_patch& getInstance() {
		static xcell_patch instance;
		return instance;
	}

	xcell_patch(const xcell_patch&) = delete;
	xcell_patch& operator=(const xcell_patch&) = delete;

	bool addExceptionFormID(uint32_t formID);
	bool removeExceptionFormID(uint32_t formID);
	bool hasExceptionFormID(uint32_t formID) const;
	size_t getExceptionFormIDCount() const;
	void clearExceptionFormIDs();
	VersionInfo getVersionInfo() const;
	bool isInitialized() const;
	bool reinitialize();

private:
	xcell_patch() = default;
	~xcell_patch() = default;

	mutable std::mutex m_mutex;
	bool m_initialized = false;
	VersionInfo m_versionInfo{};
	std::vector<uint32_t>* m_exceptionFormIDs = nullptr;
	uintptr_t m_xcellBaseAddress = 0;

	// Declaration only - definition will be in .cpp file
	static const std::unordered_map<XCellVersion, std::unordered_map<XCellProperty, uintptr_t>> s_offsetMap;

	bool ensureInitialized();
	bool initializeInternal();
	std::optional<VersionInfo> detectXCellVersion() const;
	bool isValidFormID(uint32_t formID) const;
	bool isValidModuleAddress(uintptr_t address) const;
	bool isValidAddress(uintptr_t address) const;
	bool isValidVectorPointer(const std::vector<uint32_t>* vec) const;
};