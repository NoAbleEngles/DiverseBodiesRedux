#ifndef GLOBALS_H
	#define GLOBALS_H

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include "Ini/ini.h"

namespace logger = F4SE::log;

namespace RE
{
	class BGSKeyword;
	class BGSListForm;
	class BGSPerk;
	class BGSMessage;
	class AlchemyItem;
}

template <class T>
inline T getPtr(uint32_t formId, const std::string_view& plugin = "")
{
	static_assert(std::is_pointer_v<T>, "T must be a pointer type");
	T ptr = nullptr;
	if (!plugin.empty())
		ptr = fallout_cast<T>(RE::TESDataHandler::GetSingleton()->LookupForm(formId, plugin));
	else
		ptr = fallout_cast<T>(RE::TESForm::GetFormByID(formId));
	return ptr;
}

namespace ids
{
	constexpr std::string_view plugin_name = "DiverseBodiesRedux.esp";

	//keywords
	constexpr uint32_t formid_kwd_diversed = 0x001;
	constexpr uint32_t formid_kwd_excluded = 0x002;

	//mgef
	constexpr uint32_t formid_mgef_diverse = 0x014;

	//formlist
	constexpr uint32_t formid_flst_excluded_npc = 0x734;
	constexpr uint32_t formid_flst_qualifying_race = 0x15;
	constexpr uint32_t formid_isPowerArmorFrame = 0x15503F;

	//messages
	constexpr uint32_t formid_msg_DB_WrongFile = 0xB;
	constexpr uint32_t formid_msg_DB_WrongFileActor = 0xC;

	//perk
	constexpr uint32_t formid_perk_diverse_perk = 0x7;

	//quests
	constexpr uint32_t formid_quest_choose_actor_quest = 0x8;
	constexpr uint32_t formid_quest_help_quest = 0xF99;

	//perk
	constexpr uint32_t formid_perk_diverse = 0x7;

	//spell in fallout4.esm
	constexpr uint32_t formid_spell_AbLegendaryCreatureItem = 0x1CCDA3;

	//potion
	constexpr uint32_t formid_alch_change_morphs_potion = 0x5;

	//races
	constexpr uint32_t formid_race_HumanRace = 0x13746;
	constexpr uint32_t formid_race_GhoulRace = 0xEAFB6;
	constexpr uint32_t formid_race_SynthGen2Race = 0x10BD65;

}

namespace globals
{
	inline ini::map* g_ini = nullptr;

	inline RE::BGSKeyword* kwd_diversed;
	inline RE::BGSKeyword* kwd_excluded;
	inline RE::BGSKeyword* isPowerArmorFrame;

	inline RE::BGSListForm* flst_excluded_npc;
	inline RE::BGSListForm* flst_qualifying_race;

	inline RE::BGSMessage* msg_DB_WrongFile;
	inline RE::BGSMessage* msg_DB_WrongFileActor;

	inline RE::BGSPerk* perk_diverse;
	inline RE::AlchemyItem* alch_change_morphs_potion;

	/*inline RE::TESRace* race_Human;
	inline RE::TESRace* race_Ghoul;
	inline RE::TESRace* race_Synth;*/

	using namespace ids;

	inline bool readIni()
	{
		namespace fs = std::filesystem;

		fs::path iniPath = fs::current_path() / "Data" / "MCM" / "Settings" / "DiverseBodiesRedux.ini";
		if (!fs::exists(iniPath)) {
			fs::path alternativeIni = fs::current_path() / "Data" / "MCM" / "Config" / "DiverseBodiesRedux" / "settings.ini";
			if (!fs::exists(alternativeIni)) {
				logger::info("Configuration INI file not found: {} or {}", iniPath.string(), alternativeIni.string());
				return false;
			}
			// Ensure the target directory exists
			fs::create_directories(iniPath.parent_path());
			// Copy alternative INI to the expected location
			try {
				fs::copy_file(alternativeIni, iniPath, fs::copy_options::overwrite_existing);
				logger::info("Copied default config from {} to {}", alternativeIni.string(), iniPath.string());
			}
			catch (const std::exception& e) {
				logger::info("Failed to copy config file: {}", e.what());
				return false;
			}
		}

		g_ini = new ini::map(iniPath.string());
		return true;
	}

	inline void initForms() {

		kwd_diversed = getPtr<RE::BGSKeyword*>(formid_kwd_diversed, plugin_name);
		logger::info("kwd_diversed: {}", kwd_diversed ? "found" : "not found");

		kwd_excluded = getPtr<RE::BGSKeyword*>(formid_kwd_excluded, plugin_name);
		logger::info("kwd_excluded: {}", kwd_excluded ? "found" : "not found");

		isPowerArmorFrame = getPtr<RE::BGSKeyword*>(formid_isPowerArmorFrame);
		logger::info("isPowerArmorFrame: {}", isPowerArmorFrame ? "found" : "not found");

		flst_excluded_npc = getPtr<RE::BGSListForm*>(formid_flst_excluded_npc, plugin_name);
		logger::info("flst_excluded_npc: {}", flst_excluded_npc ? "found" : "not found");

		flst_qualifying_race = getPtr<RE::BGSListForm*>(formid_flst_qualifying_race, plugin_name);
		logger::info("flst_qualifying_race: {}", formid_flst_qualifying_race ? "found" : "not found");

		msg_DB_WrongFile = getPtr<RE::BGSMessage*>(formid_msg_DB_WrongFile, plugin_name);
		logger::info("msg_DB_WrongFile: {}", msg_DB_WrongFile ? "found" : "not found");

		msg_DB_WrongFileActor = getPtr<RE::BGSMessage*>(formid_msg_DB_WrongFileActor, plugin_name);
		logger::info("msg_DB_WrongFileActor: {}", msg_DB_WrongFileActor ? "found" : "not found");

		perk_diverse = getPtr<RE::BGSPerk*>(formid_perk_diverse, plugin_name);
		logger::info("perk_diverse: {}", perk_diverse ? "found" : "not found");

		alch_change_morphs_potion = getPtr<RE::AlchemyItem*>(formid_alch_change_morphs_potion, plugin_name);
		logger::info("alch_change_morphs_potion: {}", alch_change_morphs_potion ? "found" : "not found");

		/*race_Human = getPtr<RE::TESRace*>(formid_race_HumanRace);
		logger::info("race_Human: {}", alch_change_morphs_potion ? "found" : "not found");

		race_Ghoul = getPtr<RE::TESRace*>(formid_race_GhoulRace);
		logger::info("race_Ghoul: {}", alch_change_morphs_potion ? "found" : "not found");

		race_Synth = getPtr<RE::TESRace*>(formid_race_SynthGen2Race);
		logger::info("race_Synth: {}", alch_change_morphs_potion ? "found" : "not found");*/

		logger::info("Globals initialized.");
	}
}

namespace functions {
	inline RE::TESNPC* getFaceTESNPC(RE::TESNPC* npc) {
		while (npc->faceNPC)
			npc = npc->faceNPC;
		return npc;
	}

	inline RE::TESNPC* getLeveledTESNPC(RE::TESNPC* npc)
	{
		while (npc) {
			const auto& templateFlags = npc->actorData.templateUseFlags;
			const bool useTemplate = (templateFlags & RE::TESActorBaseData::TemplateFlags::kFlagTraits) != 0;

			if (!useTemplate || !npc->templateForms)
				break;

			auto nextNPC = RE::fallout_cast<RE::TESNPC*>(*npc->templateForms);
			if (!nextNPC)
				break;

			npc = nextNPC;
		}
		return npc;
	}

	inline void removeChargenConditions(RE::TESNPC* npc)
	{
		if (npc) {
			npc = getFaceTESNPC(npc);
			if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
				auto hparts = npc->GetHeadParts();
				for (auto& hp : hparts) {
					if (hp && hp->chargenConditions && hp->chargenConditions.head)
						hp->chargenConditions.head = nullptr;
				}
			}
		}
	}

	inline void remove_chargen_from_all_tesnpc(RE::TESNPC* npc)
	{
		if (npc) {
			npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;
			removeChargenConditions(npc);
			if (npc->faceNPC && npc->faceNPC->formType == RE::ENUM_FORM_ID::kNPC_) {
				remove_chargen_from_all_tesnpc(npc->faceNPC);
			}

			auto& templateFlags = npc->actorData.templateUseFlags;
			if (((templateFlags & RE::TESActorBaseData::TemplateFlags::kFlagTraits) != 0) && npc->templateForms) {
				npc = reinterpret_cast<RE::TESNPC*>(*npc->templateForms);
				if (npc && npc->formType == RE::ENUM_FORM_ID::kNPC_) {
					remove_chargen_from_all_tesnpc(npc);
				}
			}
		}
	}

	template <typename T>
	inline T* getFormByFormID(uint32_t form_id)
	{
		RE::TESForm* form = RE::TESForm::GetFormByID(form_id);
		if (form) {
			T* result = RE::fallout_cast<T*>(form);
			return result;
		}

		return nullptr;
	}

	template <typename T>
	inline T* getFormByResolvedFormID(uint32_t form_id)
	{
		static auto a_intfc = F4SE::GetSerializationInterface();
		std::optional<unsigned long> resolvedFormId = a_intfc->ResolveFormID(form_id);

		if (resolvedFormId) {
			return getFormByFormID<T>(static_cast<uint32_t>(*resolvedFormId));
		}
		else {
			logger::warn("GetFormByResolvedFormID : Unable to resolve formId : {}", form_id);
			return nullptr;
		}
	}

	template <typename T>
	inline T* getFormByResolvedFormID(const std::string& token)
	{
		static auto a_intfc = F4SE::GetSerializationInterface();
		std::optional<uint32_t> formId;

		try {
			formId = std::stoul(token);
		}
		catch (const std::invalid_argument& e) {
			logger::warn("GetFormByResolvedFormID : Invalid argument for stoul: {}", e.what());
			return nullptr;
		}
		catch (const std::out_of_range& e) {
			logger::warn("GetFormByResolvedFormID : Out of range for stoul: {}", e.what());
			return nullptr;
		}

		// Попробуем разрешить formId
		auto resolvedFormId = a_intfc->ResolveFormID(formId.value());

		if (resolvedFormId) {
			return getFormByFormID<T>(resolvedFormId.value());
		}
		else {
			logger::warn("GetFormByResolvedFormID : Unable to resolve formId for token : {}", token);
			return nullptr;
		}
	}
}
#endif // !GLOBALS_H

