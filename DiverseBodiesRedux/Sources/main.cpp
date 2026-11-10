#include "PCH.h"
#include "globals.h"
#include "Ini/Ini.h"
#include "PresetsManager/PresetsManager.h"
#include <filesystem>
#include "LooksMenu/LooksMenuHooks.hpp"
#include "Patches/Patches.h"
#include "Patches/ClothMorphFilter.h"
#include "Patches/FaceGenNullGuard.h"
#include "Patches/FacialMorphIntensityFix.h"
#include "DirectApply/DirectApply.h"
#include "Patches/x-cell_patch.h"
#include "PrismaMenu/PrismaMenu.h"
#include <functional>

void MessageHandler(F4SE::MessagingInterface::Message* a_msg);
void initAfterGameWasStarted();
void initAfterGameDataWasLoaded();
void initValidators();

void DBMenuToggleHandler() {
	using clock = std::chrono::steady_clock;
	static clock::time_point lastPress = clock::now() - std::chrono::milliseconds(500);
	
	auto now = clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPress).count();

	if (elapsed < 500) {
		return;
	}

	lastPress = now;

	// Новое меню на PrismaUI, если стоит PrismaUI_F4.dll нужной версии; иначе старое меню на AS 3.0.
	if (PrismaMenu::IsAvailable()) {
		PrismaMenu::get().Toggle();
		return;
	}

	const auto msg = RE::UIMessageQueue::GetSingleton();
	const auto ui = RE::UI::GetSingleton();
	
	if (!msg || !ui) {
		logger::error("DBMenuToggleHandler: Failed to get UIMessageQueue or UI singleton");
		return;
	}

	if (!ui->menuMap.contains(DirectApply::MENU_NAME)) {
		logger::error("Menu '{}' not registered", DirectApply::MENU_NAME);
		return;
	}

	auto current_state_open = ui->GetMenuOpen(DirectApply::MENU_NAME);

	logger::info("try to {} DB_DirectApply menu!", current_state_open ? "close" : "open");
	if (!current_state_open) {
		msg->AddMessage(DirectApply::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow);
	}
};

extern "C" DLLEXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};
	data.PluginVersion({ 2, 10, 0, 0 });
	data.PluginName(Version::Name);
	data.AuthorName("NoAbleEngles");
	data.UsesAddressLibrary(true);
	data.UsesSigScanning(false);
	data.IsLayoutDependent(true);
	data.HasNoStructUse(false);
	data.CompatibleVersions({ F4SE::RUNTIME_1_11_191, REL::Version(1, 11, 240, 0) });
	return data;
}();

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

	log->set_level(spdlog::level::critical);
	log->flush_on(spdlog::level::critical);

	spdlog::set_default_logger(std::move(log));
	//spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
	spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);

	logger::critical("{} v{}", Version::Name, Version::Version);

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
	if (auto path = logger::log_directory()) {
		*path /= fmt::format(FMT_STRING("{}.log"), Version::Name);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(spdlog::level::critical);
		log->flush_on(spdlog::level::critical);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%m/%d/%Y - %T] [%^%l%$] %v"s);
		logger::critical("{} v{}", Version::Name, Version::Version);
	}
	if (!globals::readIni()) {
		logger::critical("Failed to read ini file... closing plugin.");
		return false;
	}
	if (auto log = spdlog::get("global log"); log && globals::g_ini) {
		const auto logLevel = std::clamp(globals::g_ini->at("DEBUG/iLogLevel", 3), 0, 6);
		const auto logFlush = std::clamp(globals::g_ini->at("DEBUG/iLogFlush", 3), 0, 6);
		log->set_level(static_cast<spdlog::level::level_enum>(logLevel));
		log->flush_on(static_cast<spdlog::level::level_enum>(logFlush));
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
		logger::critical("Registered with F4SE serialization interface.");
	}

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		logger::critical("Failed to get F4SE messaging interface, marking as incompatible.");
		return false;
	}
	else {
		logger::critical("Registered with F4SE messaging interface.");
		logger::critical("Starting...");
	}

	// Install FaceGen null pointer guard (prevents CTD at Fallout4.exe+0x6E0887 when actor head data is null)
	patches::InstallFaceGenNullGuard();

	// Install FMIN Fix: normalizes Facial Morph Intensity to 1.0f fleet-wide (prevents collapsed/distorted faces on NPCs like Doctor Sun, Solomon, Vadim)
	patches::InstallFacialMorphIntensityFix();

	return true;
}

void MessageHandler(F4SE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}

	switch (a_msg->type) {
	case F4SE::MessagingInterface::kPostLoad: {
		if (!globals::g_ini || globals::g_ini->at("PATCH/bStripClothBodyMorphs", true)) {
			patches::InstallClothMorphFilter();
		}
		break;
	}
	case F4SE::MessagingInterface::kPreLoadGame: {
		PrismaMenu::get().OnGameLoaded();
		ActorsManager::notifyGameStartLoading();
		initValidators();
		break;
	}
	case F4SE::MessagingInterface::kGameDataReady:
		initAfterGameDataWasLoaded();
		break;
	case F4SE::MessagingInterface::kPostLoadGame:
		ActorsManager::notifyGameEndLoading();
		initAfterGameWasStarted();
		break;
	case F4SE::MessagingInterface::kNewGame:
		PrismaMenu::get().OnGameLoaded();
		NPCPreset::clearNpcMap();
		ActorsManager::notifyNewGameStart();
		initAfterGameWasStarted();
		initValidators();
		break;
	}
	
}

void initAfterGameWasStarted() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		MenuButtonHandler::get().setButtonPressedCallback(&DBMenuToggleHandler);
		MenuButtonHandler::get().loadFromIni();  // кнопка и время удержания из [MENU] ini
	}
}


void initAfterGameDataWasLoaded() {
	auto& ini = *globals::g_ini;
	auto log = spdlog::get("global log");
	auto logLevel = ini.at("DEBUG/iLogLevel", 3);
	auto logFlush = ini.at("DEBUG/iLogFlush", 3);
	logLevel = std::clamp(logLevel, 0, 6);
	logFlush = std::clamp(logFlush, 0, 6);
	log->set_level(static_cast<spdlog::level::level_enum>(logLevel));
	log->flush_on(static_cast<spdlog::level::level_enum>(logFlush));
	
	globals::initForms();
	LooksMenuInterfaces<OverlayInterface>::GetInterface()->LoadOverlayMods(); // Принудительно загружаем оверлеи
	auto charGenInterface = LooksMenuInterfaces<CharGenInterface>::GetInterface();
	charGenInterface->LoadTintTemplateMods();
	charGenInterface->LoadHairColorMods();

	if (!looksMenuHooks::initializeHookActorUpdateManagerTESObjectLoadedEvent()) 
		logger::critical("Failed to install ActorUpdateManagerTESObjectLoadedEventHook");
	
	if (!looksMenuHooks::initializeHookActorUpdateManagerTESInitScriptEvent()) 		
		logger::critical("Failed to install ActorUpdateManagerTESInitScriptEventHook");

	if (ini.at("PATCH/bHookActorUpdateManagerFlush", false)) {
		if (!looksMenuHooks::initializeHookActorUpdateManagerFlush())
			logger::critical("Failed to install ActorUpdateManagerFlushHook");
	}

	/*if (!looksMenuHooks::initializeF4EEUpdateOverlaysRun()) 
		logger::critical("Failed to install F4EEUpdateOverlaysRunHook");*/

	if (ini.at("PATCH/bLooksMenuRemoveOverlayHook", false)) {
		if (!looksMenuHooks::initializeHookRemoveOverlay())
			logger::critical("Failed to install RemoveOverlayHook");
	}

	// Устанавливаем патчи
	if (ini.at("PATCH/bCBP2507", true)) {
		HMODULE cbpModule = GetModuleHandleA("cbp.dll");
		if (cbpModule) {
			int64_t bytes = 0;
			// Получаем адрес загруженного модуля
			auto base = reinterpret_cast<uint8_t*>(cbpModule);

			// Структура заголовка DOS
			auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
			if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
				// Структура заголовка NT
				auto ntHeader = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dosHeader->e_lfanew);
				if (ntHeader->Signature == IMAGE_NT_SIGNATURE) {
					bytes = ntHeader->OptionalHeader.SizeOfImage;
				}
			}

			if (bytes == 446464) {
				patches::cbp_sub_1800024C0_Hook.Install();
			}
			else {
				logger::warn("Wrong cbp.dll is loaded, can't apply bCBP2507 fix.");
			}
		}
		else {
			logger::warn("cbp.dll is not loaded");
		}
		
	}

	// Резолвим TLS-индекс ПОКА байты BSClothExtraData::SetWorld ещё не пропатчены
	// нашим же хуком ниже — иначе HasGameTLSGuardSlot() навсегда сломается
	// (см. PrewarmClothTlsGuard() в Patches.cpp) и физика ткани пропадёт целиком.
	patches::PrewarmClothTlsGuard();

	// Critical Havok cloth crash guards & simulation handlers:
	// SetTransformSet, SetTransformSet2, SetSettleOnTransitionToSim, and SetWorld
	// guard against null transformSet in hclClothInstance and BSClothExtraData.
	// In vanilla Fallout4.exe, physics hair (Dragonfly, KS Hairdos, AzarPonytails) or cloth meshes with missing bones pass
	// transformSet = nullptr, causing crashes at:
	// 1. 0x18C85FF (SetTransformSet2: cmp word ptr [r8+0xA], 0)
	// 2. 0x18A1B90 (SetSettleOnTransitionToSim: mov [rcx+0x1B7], r9b)
	// 3. 0x18A79F0 (SetWorld -> QClothSupportsLOD: cmp dword ptr [rcx+0x19C], 0xFFFFFFFF)
	// Both SetSettleOnTransitionToSim and SetWorld are implemented as clean native C++ handlers without GetOriginal(),
	// safely skipping null transformSets while properly attaching valid cloth (skirts, dresses, labcoats) to the Havok world
	// and enabling seamless cloth simulation without freezing or detached meshes.
	patches::SetTransformSet_Handler.Install();
	patches::SetTransformSet2_Handler.Install();
	patches::BSClothExtraData__SetSettleOnTransitionToSim_Bool_Handler.Install();
	patches::BSClothExtraData_SetWorld_Handler.Install();

	// Simulation sub-hooks that remain unhooked:
	// QClothSupportsLOD and ResetPrediction are safely handled inside the native SetWorld implementation above.
	// Hooking them directly is unsafe because ResetPrediction (0x18A6070) is only 10 bytes long, and QClothSupportsLOD has RIP branches.
	// patches::BSClothUtils_BSTransformSet_QClothSupportsLOD_Handler.Install();
	// patches::BSClothUtils_BSTransformSet_ResetPrediction_void_Handler.Install();

	if (ini.at("PATCH/bHeadDataGuard", false)) {
		patches::PendingActorHeadData_Process.Install();
	}
	if (ini.at("PATCH/bChangeHeadPart", true)) {
		patches::ChangeHeadPart.Install();
		patches::ChangeHeadPartRemovePart.Install();
	}
	if (ini.at("PATCH/bStripClothBodyMorphs", true)) {
		patches::InstallClothMorphFilter();
	}

	xcell_patch::getInstance().reinitialize();

	ActorsManager::get(); // Инициализируем ActorsManager
	RE::UI::GetSingleton()->RegisterMenu(DirectApply::MENU_NAME, DirectApply::Create);
	logger::critical("{} {} registered!", DirectApply::MENU_NAME, RE::UI::GetSingleton()->menuMap.contains(DirectApply::MENU_NAME) ? "succesfully" : "not");

	logger::critical("DiverseBodiesRedux loaded successfully.");

	if (auto debugPath = ini.at("DEBUG/sPrintAllPresets", std::string{}); !debugPath.empty()) {
		std::ofstream file(debugPath, std::ios::out | std::ios::trunc);
		if (file.is_open()) {
			file << PresetsManager::get().print();
			file.close();
		}
	}

	if (auto debugPath = ini.at("DEBUG/sPrintExclusions", std::string{}); !debugPath.empty()) {
		std::ofstream file(debugPath, std::ios::out | std::ios::trunc);
		if (file.is_open()) {
			file << ActorsManager::get().printExclusions();
			file.close();
		}
	}
}

// Должны быть вызваны один раз, максимально близко к началу работы игрового мира, что бы все пресеты были валидны обработаны и добавлены игроку и в TESRACE, это в общем-то касается только VaidateTints, ValidateOverlays валидирует нормально и в меню, т.к. Looks Menu подгружает данные раньше (по запросу) чем это делает игровой движок.
void initValidators() {
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		logger::info("Initializing validators...");
		PresetsManager::get().validatePresets();
	}
}