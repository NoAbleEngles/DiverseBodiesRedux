#include "PCH.h"
#include "globals.h"
#include "Ini/Ini.h"
#include "PresetsManager/PresetsManager.h"
#include <filesystem>

void MessageHandler(F4SE::MessagingInterface::Message* a_msg);
void initAfterGameWasStarted();
void initAfterGameDataWasLoaded();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::Name);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	//spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::info("{} v{}", Version::Name, Version::Version);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::Name;
	a_info->version = Version::VersionCount;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical("unsupported runtime v{}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	if (!globals::readIni()) {
		logger::critical("Failed to read ini file... closing plugin.");
		return false;
	}

	const auto serialization = F4SE::GetSerializationInterface();
	if (!serialization) {
		logger::critical("Failed to get F4SE serialization interface, marking as incompatible.");
		return false;
	}
	else {
		serialization->SetUniqueID(Version::UID);
		//serialization->SetRevertCallback();
		//serialization->SetSaveCallback(dbr_manager::ActorsManager::Serialize);
		//serialization->SetLoadCallback(dbr_manager::ActorsManager::Deserialize);
		logger::info("Registered with F4SE serialization interface.");
	}

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::info("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	}
	else {
		logger::info("Registered with F4SE messaging interface.");
		logger::info("Starting...");
		initAfterGameWasStarted();
	}

	return true;
}

void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}
	switch (a_msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
	{
		initAfterGameDataWasLoaded();
		break;
	}
	case F4SE::MessagingInterface::kPreLoadGame:
	{
		break;
	}
	case F4SE::MessagingInterface::kPreSaveGame:
	{
		break;
	}
	/*case F4SE::MessagingInterface::kPostLoadGame:
		{
			global::add_potion();
			break;
		}*/
	}
}

void initAfterGameWasStarted() {

}

void initAfterGameDataWasLoaded() {
	globals::initForms();
	PresetsManager::get().validatePresets();
	logger::info("DiverseBodiesRedux loaded successfully.");
}