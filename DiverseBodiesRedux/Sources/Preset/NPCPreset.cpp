#include "NPCPreset.h"
#include "globals.h"
#include <RE/Bethesda/BSBound.h>
#include <RE/Bethesda/Actor.h>
#include <RE/NetImmerse/NiNode.h>
#include <RE/Bethesda/PlayerCharacter.h>
#include <RE/Bethesda/BSFaceGenNiNode.h>
#include <RE/Bethesda/BSFaceGenAnimationData.h>
#include <RE/Bethesda/BSTriShape.h>
#include <RE/NetImmerse/NiMatrix3.h>
#include <F4SE/Interfaces.h>
#include <RE/Bethesda/BGSChargenUtils.h>

#include <thread>
#include <chrono>
#include <memory>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <functional>

#include "DirectApply/DirectApply.h" // только для DirectApply::MENU_NAME

#include "Patches/x-cell_patch.h"
#include "Details/ProcessingGuard.hpp"
#include "ActorsManager/ActorResetQueue.h"

std::set<uint32_t> NPCPreset::ALL_ITEMS_M{};
std::set<uint32_t> NPCPreset::ALL_ITEMS_F{};

std::unordered_set<uint32_t> NPCPreset::m_npcPresets;
std::mutex NPCPreset::m_npcPresetsMtx;

// Common small inline helpers for repeated patterns
namespace {
	inline std::string GetNodeName(const RE::NiAVObject* node)
	{
		return node && node->name.c_str() ? node->name.c_str() : std::string("(no name)");
	}

	inline std::string GetParentName(const RE::NiAVObject* node)
	{
		const auto parent = node ? node->parent : nullptr;
		return parent && parent->name.c_str() ? parent->name.c_str() : std::string("");
	}

	inline void ForEachChildNode(RE::NiNode* node, const std::function<void(RE::NiAVObject*, std::uint32_t)>& fn)
	{
		if (!node) return;
		for (std::uint32_t i = 0; i < node->children.size(); ++i) {
			#pragma warning(push)
			#pragma warning(disable : 4244)
			auto child = node->children[i].get();
			#pragma warning(pop)
			if (child) {
				fn(child, i);
			}
		}
	}

	inline void VisitNiTree(RE::NiAVObject* node, const std::function<void(RE::NiAVObject*)>& fn)
	{
		if (!node) return;
		fn(node);
		if (auto niNode = node->IsNode() ? static_cast<RE::NiNode*>(node) : nullptr) {
			ForEachChildNode(niNode, [&](RE::NiAVObject* child, std::uint32_t) {
				VisitNiTree(child, fn);
			});
		}
	}

	template <typename Type>
	inline void EnsureAllocated(Type*& ptr, const char* name, RE::Actor* actor)
	{
		if (!ptr) {
			logger::error("NPCPreset: {} is nullptr for actor [{:x}]", name, actor ? actor->formID : 0u);
			ptr = Allocate<Type>();
			ptr = new (ptr) Type();
		}
	}

	static auto& memoryManager = RE::MemoryManager::GetSingleton();

	template<typename T>
	inline T* Allocate() {
		return static_cast<T*>(memoryManager.Allocate(sizeof(T), alignof(T), true));
	}

	template<typename T>
	inline T* AllocateArray(size_t count) {
		return static_cast<T*>(memoryManager.Allocate(sizeof(T) * count, alignof(T), true));
	}

	inline void Deallocate(void* ptr) {
		memoryManager.Deallocate(ptr, true);
	}

	// --- Fix A: безопасное владение массивом TESNPC::headParts ---
	//
	// Исходный массив headParts (из ESM / расы / шаблона-донора лица) может быть
	// ОБЩИМ между расой, GetRootFaceNPC() и другими TESNPC. Его освобождение
	// приводило к use-after-free в IO-потоке движка (AIProcessPendingActorHeadData),
	// который читает headParts при построении головы актёра.
	//
	// Правила:
	//  * исходный (не наш) массив НИКОГДА не освобождаем;
	//  * освобождаем только массив, который выделили сами и на который больше никто не ссылается;
	//  * новый массив по ёмкости >= любого счётчика numHeadParts, который может
	//    увидеть конкурентный читатель; указатель пишем раньше счётчика.
	std::unordered_set<void*> g_ownedHeadPartArrays;
	std::mutex                g_ownedHeadPartArraysMtx;

	inline void registerOwnedHeadPartArray(void* ptr) {
		if (!ptr) return;
		std::lock_guard lock(g_ownedHeadPartArraysMtx);
		g_ownedHeadPartArrays.insert(ptr);
	}

	// true (и удаляет из реестра), если ptr — наша прежняя аллокация.
	inline bool takeOwnedHeadPartArray(void* ptr) {
		if (!ptr) return false;
		std::lock_guard lock(g_ownedHeadPartArraysMtx);
		return g_ownedHeadPartArrays.erase(ptr) > 0;
	}

	// Массив под capacity указателей BGSHeadPart* (aligned=false, парно с freeOwnedHeadPartArray).
	inline RE::BGSHeadPart** allocHeadPartArray(std::size_t capacity) {
		if (capacity == 0) capacity = 1;
		return static_cast<RE::BGSHeadPart**>(
			memoryManager.Allocate(sizeof(RE::BGSHeadPart*) * capacity, alignof(RE::BGSHeadPart*), false));
	}

	inline void freeOwnedHeadPartArray(void* ptr) {
		memoryManager.Deallocate(ptr, false);
	}

	std::vector<RE::BGSHeadPart*> s_femaleHairs;
	std::vector<RE::BGSHeadPart*> s_maleHairs;
	std::vector<RE::BGSHeadPart*> s_femaleEyes;
	std::vector<RE::BGSHeadPart*> s_maleEyes;
	std::mutex                    s_fallbackPartsMutex;
	bool                          s_fallbackPartsInitialized = false;

	inline void initFallbackHeadParts() {
		std::lock_guard lock(s_fallbackPartsMutex);
		if (s_fallbackPartsInitialized) return;

		auto dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) return;

		auto& headParts = dataHandler->GetFormArray<RE::BGSHeadPart>();
		for (auto* hp : headParts) {
			if (!hp || hp->IsExtraPart()) continue;

			// Restrict fallback hairs and eyes strictly to vanilla Fallout4.esm (index 0x00).
			// Mod hairs (such as physics hairs from KS Hairdos) crash actors without cloth bones on their skeleton.
			if ((hp->formID >> 24) != 0x00) continue;

			if (hp->type.get() == RE::BGSHeadPart::HeadPartType::kHair) {
				if (hp->flags.all(RE::BGSHeadPart::Flag::kPlayable)) {
					if (!hp->flags.all(RE::BGSHeadPart::Flag::kCantBeFemale)) {
						s_femaleHairs.push_back(hp);
					}
					if (!hp->flags.all(RE::BGSHeadPart::Flag::kCantBeMale)) {
						s_maleHairs.push_back(hp);
					}
				}
			} else if (hp->type.get() == RE::BGSHeadPart::HeadPartType::kEyes) {
				if (hp->flags.all(RE::BGSHeadPart::Flag::kPlayable)) {
					if (!hp->flags.all(RE::BGSHeadPart::Flag::kCantBeFemale)) {
						s_femaleEyes.push_back(hp);
					}
					if (!hp->flags.all(RE::BGSHeadPart::Flag::kCantBeMale)) {
						s_maleEyes.push_back(hp);
					}
				}
			}
		}

		logger::info("initFallbackHeadParts: loaded {} female hairs, {} male hairs, {} female eyes, {} male eyes",
			s_femaleHairs.size(), s_maleHairs.size(), s_femaleEyes.size(), s_maleEyes.size());
		s_fallbackPartsInitialized = true;
	}

	inline bool isHeadPartGenderCompatible(const RE::BGSHeadPart* hp, bool isFemale) noexcept {
		if (!hp) return false;
		if (isFemale) {
			if (hp->flags.all(RE::BGSHeadPart::Flag::kCantBeFemale)) {
				return false;
			}
			if (hp->type.get() == RE::BGSHeadPart::HeadPartType::kFacialHair) {
				return false;
			}
		} else {
			if (hp->flags.all(RE::BGSHeadPart::Flag::kCantBeMale)) {
				return false;
			}
		}
		return true;
	}

	inline RE::BGSHeadPart* getFallbackHair(RE::Actor* actor) {
		if (!actor) return nullptr;
		initFallbackHeadParts();
		bool isFemale = actor->GetSex() == RE::Actor::Sex::Female;
		const auto& list = isFemale ? s_femaleHairs : s_maleHairs;
		if (list.empty()) return nullptr;

		std::vector<RE::BGSHeadPart*> raceMatches;
		raceMatches.reserve(list.size());
		for (auto* hp : list) {
			if (!hp->validRaces || !actor->race || hp->validRaces->ContainsItem(actor->race)) {
				raceMatches.push_back(hp);
			}
		}
		const auto& targetList = raceMatches.empty() ? list : raceMatches;
		std::size_t idx = static_cast<std::size_t>(actor->formID) % targetList.size();
		return targetList[idx];
	}

	inline RE::BGSHeadPart* getFallbackEyes(RE::Actor* actor) {
		if (!actor) return nullptr;
		initFallbackHeadParts();
		bool isFemale = actor->GetSex() == RE::Actor::Sex::Female;
		const auto& list = isFemale ? s_femaleEyes : s_maleEyes;
		if (list.empty()) return nullptr;

		std::vector<RE::BGSHeadPart*> raceMatches;
		raceMatches.reserve(list.size());
		for (auto* hp : list) {
			if (!hp->validRaces || !actor->race || hp->validRaces->ContainsItem(actor->race)) {
				raceMatches.push_back(hp);
			}
		}
		const auto& targetList = raceMatches.empty() ? list : raceMatches;
		std::size_t idx = static_cast<std::size_t>(actor->formID) % targetList.size();
		return targetList[idx];
	}
}

// Helper: collect editorIDs from TESNPC headParts
static std::vector<std::string> collectHeadPartEditorIDs(const RE::TESNPC* npc)
{
	std::vector<std::string> out;
	if (!npc || !npc->headParts || npc->numHeadParts <= 0)
		return out;
	for (int i = 0; i < npc->numHeadParts; ++i) {
		if (auto hp = npc->headParts[i]) {
			const char* eid = hp->GetFormEditorID();
			out.emplace_back(eid ? eid : "");
		}
	}
	return out;
}

// Helper: collect child names from BSFaceGenNiNode
static std::vector<std::string> collectFaceNodeChildNames(RE::Actor* actor)
{
	std::vector<std::string> out;
	if (!actor)
		return out;
	if (auto faceNode = actor->GetFaceNodeSkinned()) {
		auto asNiNode = reinterpret_cast<RE::NiNode*>(faceNode);
		ForEachChildNode(asNiNode, [&](RE::NiAVObject* child, std::uint32_t) {
			out.emplace_back("[" + GetParentName(child) + "]: " + GetNodeName(child));
		});
	}
	return out;
}

// Helper: log differences between headParts (by EditorID) and scene graph children
static void logHeadPartsVsSceneGraph(RE::Actor* actor, const RE::TESNPC* baseNPC, const RE::TESNPC* faceNPC, const std::string& stage)
{
	// Collect lists
	auto baseIDs = collectHeadPartEditorIDs(baseNPC);
	auto faceIDs = collectHeadPartEditorIDs(faceNPC);
	auto childNames = collectFaceNodeChildNames(actor);

	// Also log alternate list usage and content for both NPC and FaceNPC
	if (baseNPC) {
		bool usingAlt = baseNPC->UsingAlternateHeadPartList();
		logger::info("{}: BaseNPC UsingAlternateHeadPartList={}", stage, usingAlt);
		auto baseAlt = baseNPC->GetHeadParts(true);
		auto baseDefault = baseNPC->GetHeadParts(false);
		std::string baseAltList, baseDefList;
		for (auto* hp : baseAlt) { if (hp) { if (!baseAltList.empty()) baseAltList += ", "; baseAltList += hp->GetFormEditorID() ? hp->GetFormEditorID() : ""; } }
		for (auto* hp : baseDefault) { if (hp) { if (!baseDefList.empty()) baseDefList += ", "; baseDefList += hp->GetFormEditorID() ? hp->GetFormEditorID() : ""; } }
		logger::info("{}: BaseNPC AltHeadParts count={} [{}]", stage, (int)baseAlt.size(), baseAltList);
		logger::info("{}: BaseNPC DefaultHeadParts count={} [{}]", stage, (int)baseDefault.size(), baseDefList);
	}
	if (faceNPC) {
		bool usingAlt = faceNPC->UsingAlternateHeadPartList();
		logger::info("{}: FaceNPC UsingAlternateHeadPartList={}", stage, usingAlt);
		auto faceAlt = faceNPC->GetHeadParts(true);
		auto faceDefault = faceNPC->GetHeadParts(false);
		std::string faceAltList, faceDefList;
		for (auto* hp : faceAlt) { if (hp) { if (!faceAltList.empty()) faceAltList += ", "; faceAltList += hp->GetFormEditorID() ? hp->GetFormEditorID() : ""; } }
		for (auto* hp : faceDefault) { if (hp) { if (!faceDefList.empty()) faceDefList += ", "; faceDefList += hp->GetFormEditorID() ? hp->GetFormEditorID() : ""; } }
		logger::info("{}: FaceNPC AltHeadParts count={} [{}]", stage, (int)faceAlt.size(), faceAltList);
		logger::info("{}: FaceNPC DefaultHeadParts count={} [{}]", stage, (int)faceDefault.size(), faceDefList);
	}

	logger::info("{}: ===== HEADPARTS LISTS =====", stage);
	logger::info("{}: BaseNPC headParts count={}", stage, static_cast<int>(baseIDs.size()));
	for (const auto& id : baseIDs) {
		logger::info("{}: BaseNPC headPart [{}]", stage, id);
	}

	logger::info("{}: FaceNPC headParts count={}", stage, static_cast<int>(faceIDs.size()));
	for (const auto& id : faceIDs) {
		logger::info("{}: FaceNPC headPart [{}]", stage, id);
	}

	logger::info("{}: FaceNode children count={}", stage, int(childNames.size()));
	for (const auto& name : childNames) {
		logger::info("{}: FaceNode child [{}]", stage, name);
	}

	logger::info("{}: ====================================", stage);
}

void NPCPreset::addTintsFromThisToPossibleTints() {
	if (!empty()) {
		auto& store = m_conditions.gender() == RE::Actor::Sex::Male ? ALL_ITEMS_M : ALL_ITEMS_F;
		for (const auto& el : m_tints)
			store.emplace(el->id());
	}
}

NPCPreset::NPCPreset() :
	Preset(std::string{}) {
	addTintsFromThisToPossibleTints();
}

NPCPreset::NPCPreset(const std::filesystem::path& path) :
	Preset(path.stem().string())
{
	if (!loadFromFile(path.string())) {
		logger::error("Failed to load NPCPreset from path: {}", path.string());
		return;
	};

	addTintsFromThisToPossibleTints();
}

NPCPreset::NPCPreset(const std::string& path) :
	Preset(std::filesystem::path{ path }.stem().string().c_str())
{
	if (!loadFromFile(path)) {
		logger::error("Failed to load NPCPreset from path: {}", path);
		return;
	};

	addTintsFromThisToPossibleTints();
}

NPCPreset::~NPCPreset() {}

bool NPCPreset::operator==(const Preset& other) const noexcept
{
	if (this == &other) {
		return true;
	}

	auto ptrOther = preset_cast<const NPCPreset*>(&other);
	if (!ptrOther) {
		return false;
	}

	if (Preset::operator==(other)) {
		if (auto otherPreset = preset_cast<NPCPreset*>(&other); otherPreset) {
			return
				(
					m_tints == otherPreset->m_tints
					&& m_tintOrder == otherPreset->m_tintOrder
					&& Preset::operator<(other)
					);
		}
	}
	return false;
}

const std::set<uint32_t> NPCPreset::getAllPossibleMaleTints() noexcept {
	return ALL_ITEMS_M;
}

const std::set<uint32_t> NPCPreset::getAllPossibleFemaleTints() noexcept {
	return ALL_ITEMS_F;
}

void NPCPreset::revalidateAllPossibleTints(const std::set<uint32_t>& AllValidTints) {
	std::set<uint32_t> temp{};
	for (const auto& el : ALL_ITEMS_M) {
		if (AllValidTints.contains(el)) {
			temp.emplace(el);
		}
	}
}

bool NPCPreset::operator<(const Preset& other) const noexcept
{
	auto bp_other = preset_cast<const NPCPreset*>(&other);
	if (bp_other)
		return this->m_id < bp_other->m_id;

	return Preset::operator<(other);
}

CoincidenceLevel NPCPreset::check(const RE::Actor* actor, Filter filter) const noexcept
{
	return Preset::check(actor, filter);  // Проверяем базовые условия из Preset
}

PresetType NPCPreset::type() const noexcept {
	return PRESET_TYPE;
}

bool NPCPreset::remove(RE::Actor* actor) const {
	if (!actor || !actor->GetFullyLoaded3D())
		return false;

	if (!actor) {
		logger::info("NPCPreset Apply no actor provided!");
		return false;
	}

	auto actorNPC = actor->GetNPC();
	if (!actorNPC) {
		logger::warn("NPCPreset Apply no npc provided for actor [{:x}] {}!", actor->formID, actor->GetDisplayFullName());
		return false;
	}

	auto actorFace = actorNPC->GetRootFaceNPC();

	if (!actorFace) {
		logger::error("NPCPreset Actor [{}] {} has no face NPC provided!", actor->formID, actor->GetDisplayFullName());
		return false;
	}

	bool once = false;

	if (!m_tintOrder.empty() && actorFace->tintingData) {
		// Убираем все тинты - iterate using direct array access
		for (uint32_t i = 0; i < actorFace->tintingData->size; ++i) {
			auto* tint = actorFace->tintingData->data[i];
			if (!tint || tint->percent == 0)
				continue;
			if (tint->templateEntry) {
				tint->percent = 0;
				once = true;
			}
		}
	}
	else if (actorFace->tintingData) {
		auto& tintsToRemove = actor->GetSex() == RE::Actor::Sex::Female ? ALL_ITEMS_F : ALL_ITEMS_M;
		// Iterate using direct array access
		for (uint32_t i = 0; i < actorFace->tintingData->size; ++i) {
			auto* tint = actorFace->tintingData->data[i];
			if (!tint || tint->percent == 0 || !tint->templateEntry)
				continue;
			if (tintsToRemove.contains(tint->templateEntry->uniqueID)) {
				tint->percent = 0;
				once = true;
			}
		}
	}

	return once;
}

bool NPCPreset::apply(RE::Actor* actor, bool reset3d) const {
	return applyImpl(actor, reset3d, /*checkAlreadyApplied=*/true);
}

bool NPCPreset::forceApply(RE::Actor* actor, bool reset3d) const {
	return applyImpl(actor, reset3d, /*checkAlreadyApplied=*/false);
}

bool NPCPreset::applyImpl(RE::Actor* actor, bool reset3d, bool checkAlreadyApplied) const {
	if (!actor) {
		logger::warn("NPCPreset Apply no actor provided!");
		return false;
	}

	auto actorGuard = std::make_shared<ProcessingGuard<std::set<uint32_t>>>(actor->GetFormID(), m_processingActors, m_processingActorsMtx);
	if (!actorGuard->acquire_sync_for(std::chrono::seconds{})) {
		return false; // таймаут
	}

	auto actorNPC = actor->GetNPC();
	if (!actorNPC) {
		logger::warn("NPCPreset Apply no npc provided for actor [{:x}] {}!", actor->formID, actor->GetDisplayFullName());
		return false;
	}

	auto npcGuard = std::make_shared<ProcessingGuard<std::set<uint32_t>>>(actorNPC->GetFormID(), m_processingNpc, m_processingNpcMtx);
	if (!npcGuard->acquire_sync_for(std::chrono::seconds{ 5 })) {
		return false; // таймаут
	}

	// --- Схема 2 (2026-10-23): разрываем общий faceNPC-шаблон ДО применения пресета ---
	// TESNPC::faceNPC — это цепочка "общих лицевых данных": headParts/tints/morphs
	// читаются и движком, и нашим кодом не из actorNPC, а из GetRootFaceNPC() —
	// WHILE root->faceNPC walk. Несколько актёров одного шаблона (например,
	// рандомизированные жители с общим "лицевым донором") разделяют ОДНУ TESNPC-
	// запись шаблона. Когда applyImpl писал пресет именно в неё — применение
	// пресета одним актёром стирало/портило внешность ВСЕХ остальных актёров
	// того же шаблона (и наоборот при следующей обработке). Это и было корнем
	// бага с "улетающей причёской": один из актёров шаблона 0xc9ac5 успешно
	// применял свой пресет ПОСЛЕ другого, стирая её headParts/facialBoneRegion.
	//
	// Фикс: каждый актёр "усыновляет" свою собственную, уже существующую TESNPC-
	// запись (actorNPC) как корень лицевых данных, отключаясь от общего шаблона.
	// Поля headParts/headRelatedData/tintingData/morphSliderValues и т.д. — это
	// ОБЫЧНЫЕ поля TESNPC, выделенные ещё при загрузке формы из ESM; при разделении
	// лица с шаблоном они просто не используются (nullptr), пока faceNPC != null.
	// После detach actorFace всегда == actorNPC, и весь код ниже применяет пресет
	// по тому же пути, что и для обычного (не шаблонного) NPC — уже проверенному
	// на практике для всех уникальных акторов. См. память headparts-facegen-race.md.
	if (actorNPC->faceNPC) {
		logger::info("NPCPreset::applyImpl: detaching actor [{:x}] NPC [{:x}] from shared faceNPC template [{:x}] — giving independent head/face data copy",
			actor->formID, actorNPC->formID, actorNPC->faceNPC->formID);
		actorNPC->faceNPC = nullptr;
	}

	auto actorFace = actorNPC->GetRootFaceNPC();
	if (!actorFace) {
		logger::error("NPCPreset Actor [{:x}] {} has no face NPC provided!", actor->formID, actor->GetDisplayFullName());
		return false;
	}

	// Схема 2: actorFace == actorNPC ВСЕГДА (см. detach выше) — отдельная блокировка
	// по formID "общего faceNPC" больше не нужна, этот случай физически не возникает.

	bool actorHasHair = false;
	if (actorFace && actorFace->headParts && actorFace->numHeadParts > 0) {
		for (int i = 0; i < actorFace->numHeadParts; ++i) {
			if (actorFace->headParts[i] && actorFace->headParts[i]->type.get() == RE::BGSHeadPart::HeadPartType::kHair) {
				actorHasHair = true;
				break;
			}
		}
	}

	// Проверка и пометка NPC как обрабатываемого должны быть атомарными:
	// два потока для разных actor-ов с одинаковым faceNPC могут одновременно
	// пройти find() == end() и оба начать модифицировать одну TESNPC → гонка.
	{
		std::lock_guard lock(m_npcPresetsMtx);
		bool firstTime = m_npcPresets.find(actorFace->formID) == m_npcPresets.end();
		if (checkAlreadyApplied && !firstTime && actorHasHair) {
			// Блокируем только автоматическое повторное применение, если у актора уже есть волосы
			logger::info("TESNPC id {:x} for actor [{:x}] {} is already modified, blocking automatic reapplication!", actorFace->formID, actor->formID, actor->GetDisplayFullName());
			return false;
		}
		// Всегда помечаем NPC как модифицированного
		m_npcPresets.insert(actorFace->formID);
	}
	

	logger::info("NPCPreset::applyImpl faceNPC={:08X}", actorFace->formID);
	
	auto& ini = *globals::g_ini; 
	auto AddChargenFlag = ini.at("GENERAL/bAddChargenFlag", false);
	bool isChargenFace = static_cast<bool>(actorNPC->actorData.actorBaseFlags & RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset);
	logger::info("NPCPreset::applyImpl chargen before: add={}, hasFlag={}", AddChargenFlag, isChargenFace);
	// If configured, ensure CharGen flag is enabled
	if (AddChargenFlag && !isChargenFace) {
		functions::enableRuntimeFaceChargen(actorNPC);
		isChargenFace = true;
		logger::info("NPCPreset::applyImpl enabled CharGen flag for NPC {:08X}", actorNPC->GetFormID());
	} else if (isChargenFace) {
		logger::info("NPCPreset::applyImpl CharGen flag already present");
	} else {
		return false; // Do not apply preset if CharGen flag is absent and AddChargenFlag is true
	}
	// Do not block application if flag is absent and AddChargenFlag is false
	 
	remove(actor);

	bool success = false;
	auto flags{ RE::RESET_3D_FLAGS::kNone };

	// Создаем временный контейнер для упорядоченных тинтов (для ORDER)
	std::vector<uint32_t> tintOrderToApply = m_tintOrder; // может быть пустым

	using scrapAllocator = RE::BSScrapArrayAllocator;

	const bool isActorFemale = (actor->GetSex() == RE::Actor::Sex::Female);

	std::set<RE::BGSHeadPart*> currentHeadParts{};
	auto append = [&currentHeadParts, isActorFemale](RE::BGSHeadPart** arr, std::int8_t count) {
		if (!arr || count <= 0) return;
		for (int i = 0; i < count; ++i) {
			if (arr[i] && isHeadPartGenderCompatible(arr[i], isActorFemale)) {
				currentHeadParts.insert(arr[i]);
			}
		}
	};

	append(actorNPC->headParts, actorNPC->numHeadParts);
	if (actorFace && actorFace != actorNPC) {
		append(actorFace->headParts, actorFace->numHeadParts);
	}

	auto onlyTints = m_conditions.onlyTints();
	if (!onlyTints && (!m_headParts.empty() || !actorHasHair)) {
		// Добавляем в исключения xCell
		auto& xcell = xcell_patch::getInstance();
		if (xcell.isInitialized() && !xcell.hasExceptionFormID(actorFace->formID)) {
			xcell.addExceptionFormID(actorFace->formID);
		}

		// 1. Собираем типы частей головы, которые есть в пресете
		std::unordered_set<RE::BGSHeadPart::HeadPartType> presetTypes;
		std::vector<RE::BGSHeadPart*> mergedHeadParts;

		for (auto* hp : m_headParts) {
			if (hp) {
				if (!isHeadPartGenderCompatible(hp, isActorFemale)) {
					logger::warn("NPCPreset: skipping gender-incompatible headpart {:08X} ('{}', type {}) for actor [{:x}] sex {}",
						hp->GetFormID(), hp->GetFormEditorID(), static_cast<int>(hp->type.get()), actor->formID, isActorFemale ? "Female" : "Male");
					continue;
				}
				presetTypes.insert(hp->type.get());
				mergedHeadParts.push_back(hp);
			}
		}

		// 2. Если в пресете отсутствует часть какого-то типа (например, причёска или глаза были
		// в моде, которого нет в игре, или пресет просто их не задаёт), сохраняем оригинальную часть
		// головы актёра, чтобы NPC не становился лысым или с пустыми глазницами!
		auto alreadyHasPart = [&](RE::BGSHeadPart* part) {
			return std::find(mergedHeadParts.begin(), mergedHeadParts.end(), part) != mergedHeadParts.end();
		};

		for (auto* origPart : currentHeadParts) {
			if (!origPart) continue;
			if (!isHeadPartGenderCompatible(origPart, isActorFemale)) continue;
			if (presetTypes.find(origPart->type.get()) == presetTypes.end()) {
				if (!alreadyHasPart(origPart)) {
					mergedHeadParts.push_back(origPart);
					logger::info("NPCPreset: Retaining actor original headpart {:08X} (type {}) for preset '{}'",
						origPart->GetFormID(), static_cast<int>(origPart->type.get()), id());
				}
				for (auto* extra : origPart->extraParts) {
					if (extra && isHeadPartGenderCompatible(extra, isActorFemale) && !alreadyHasPart(extra)) {
						mergedHeadParts.push_back(extra);
					}
				}
			}
		}

		// 3. Fallback: если ни пресет, ни исходный актёр не предоставили причёску или глаза
		// (например, мод на причёски отсутствует, а актёр ранее уже был обработан с пустой причёской),
		// гарантируем, что персонаж НЕ останется лысым или без глаз!
		bool hasHair = false;
		bool hasEyes = false;
		for (auto* hp : mergedHeadParts) {
			if (!hp) continue;
			if (hp->type.get() == RE::BGSHeadPart::HeadPartType::kHair) hasHair = true;
			if (hp->type.get() == RE::BGSHeadPart::HeadPartType::kEyes) hasEyes = true;
		}

		if (!hasHair) {
			if (auto* fallbackHair = getFallbackHair(actor)) {
				if (!alreadyHasPart(fallbackHair)) {
					mergedHeadParts.push_back(fallbackHair);
					logger::info("NPCPreset: Assigned fallback hair {:08X} ({}) for actor [{:x}] {} preset '{}'",
						fallbackHair->GetFormID(), fallbackHair->GetFormEditorID(), actor->formID, actor->GetDisplayFullName(), id());
					for (auto* extra : fallbackHair->extraParts) {
						if (extra && !alreadyHasPart(extra)) {
							mergedHeadParts.push_back(extra);
						}
					}
				}
			}
		}

		if (!hasEyes) {
			if (auto* fallbackEyes = getFallbackEyes(actor)) {
				if (!alreadyHasPart(fallbackEyes)) {
					mergedHeadParts.push_back(fallbackEyes);
					logger::info("NPCPreset: Assigned fallback eyes {:08X} ({}) for actor [{:x}] {} preset '{}'",
						fallbackEyes->GetFormID(), fallbackEyes->GetFormEditorID(), actor->formID, actor->GetDisplayFullName(), id());
					for (auto* extra : fallbackEyes->extraParts) {
						if (extra && !alreadyHasPart(extra)) {
							mergedHeadParts.push_back(extra);
						}
					}
				}
			}
		}

		// Fix A: количество headParts ограничено типом numHeadParts (int8_t).
		std::size_t newCount = mergedHeadParts.size();
		if (newCount > 127) {
			logger::warn("NPCPreset: preset '{}' has {} head parts, clamping to 127", id(), newCount);
			newCount = 127;
		}

		auto* oldNPCHeadParts  = actorNPC->headParts;
		auto* oldFaceHeadParts = actorFace->headParts;
		const std::size_t oldNPCCount  = actorNPC->numHeadParts  > 0 ? static_cast<std::size_t>(actorNPC->numHeadParts)  : 0u;
		const std::size_t oldFaceCount = actorFace->numHeadParts > 0 ? static_cast<std::size_t>(actorFace->numHeadParts) : 0u;
		const bool hadSamePointer = (oldNPCHeadParts == oldFaceHeadParts);

		// Ёмкость нового массива >= любого счётчика, который может увидеть
		// конкурентный читатель (IO-поток движка при построении головы), — чтобы он
		// не вышел за границы, даже прочитав новый указатель со старым numHeadParts.
		const std::size_t capacity = (std::max)({ newCount, oldNPCCount, oldFaceCount, std::size_t{ 1 } });

		auto fillHeadParts = [&](RE::BGSHeadPart** mem) {
			for (std::size_t i = 0; i < newCount; ++i) mem[i] = mergedHeadParts[i];
			RE::BGSHeadPart* pad = newCount ? mergedHeadParts[newCount - 1] : nullptr;
			for (std::size_t i = newCount; i < capacity; ++i) mem[i] = pad;
		};

		auto* memNPC = allocHeadPartArray(capacity);
		if (!memNPC) {
			logger::error("Failed to allocate memory for head parts (NPC)");
			return false;
		}
		fillHeadParts(memNPC);
		// Указатель пишем ДО счётчика (обе записи по отдельности атомарны на x64).
		actorNPC->headParts    = memNPC;
		actorNPC->numHeadParts = static_cast<std::int8_t>(newCount);
		registerOwnedHeadPartArray(memNPC);

		if (!hadSamePointer) {
			auto* memFace = allocHeadPartArray(capacity);
			if (!memFace) {
				logger::error("Failed to allocate memory for head parts (FaceNPC)");
				return false;
			}
			fillHeadParts(memFace);
			actorFace->headParts    = memFace;
			actorFace->numHeadParts = static_cast<std::int8_t>(newCount);
			registerOwnedHeadPartArray(memFace);
		} else {
			actorFace->headParts    = actorNPC->headParts;
			actorFace->numHeadParts = actorNPC->numHeadParts;
		}

		// Освобождаем СТАРЫЕ массивы только если это НАШИ прежние аллокации.
		// Исходный (движковый / расовый / шаблонный) массив не трогаем: он может
		// быть общим и читается IO-потоком движка при построении головы →
		// use-after-free (краш в AIProcessPendingActorHeadData).
		if (oldFaceHeadParts && oldFaceHeadParts != actorNPC->headParts && takeOwnedHeadPartArray(oldFaceHeadParts)) {
			freeOwnedHeadPartArray(oldFaceHeadParts);
		}
		if (oldNPCHeadParts && oldNPCHeadParts != actorNPC->headParts && oldNPCHeadParts != oldFaceHeadParts && takeOwnedHeadPartArray(oldNPCHeadParts)) {
			freeOwnedHeadPartArray(oldNPCHeadParts);
		}

		flags |= RE::RESET_3D_FLAGS::kHead;
		flags |= RE::RESET_3D_FLAGS::kSkeleton;
		flags |= RE::RESET_3D_FLAGS::kModel;
		success = true;
	}

	// Применяем цвет волос.
	// EnsureAllocated: после detach (схема 2) headRelatedData у "усыновлённого"
	// actorFace почти всегда nullptr (раньше этот указатель нёс за неё шаблон) —
	// без аллокации цвет волос молча не применялся бы для таких актёров.
	if (!onlyTints) {
		EnsureAllocated(actorFace->headRelatedData, "headRelatedData", actor);
		if (m_hairColor && actorFace->headRelatedData->hairColor != m_hairColor) {
			actorFace->headRelatedData->hairColor = m_hairColor;
			success = true;
		}
		if (!isActorFemale) {
			if (m_faceHairColor && actorFace->headRelatedData->facialHairColor != m_faceHairColor) {
				actorFace->headRelatedData->facialHairColor = m_faceHairColor;
				success = true;
			}
		} else if (actorFace->headRelatedData->facialHairColor) {
			actorFace->headRelatedData->facialHairColor = nullptr;
		}

		flags |= RE::RESET_3D_FLAGS::kHead;
	}

	// Применяем слайдеры морфов лица (Morphs/Presets).
	// LooksMenu (старый движок) пишет в actor->baseForm напрямую, там baseForm == GetRootFaceNPC().
	// В новом движке actorNPC->faceNPC может указывать на другой NPC (root face),
	// и движок читает лицевые данные именно из GetRootFaceNPC(), а не из baseForm актора.
	// Поэтому пишем в actorFace (= GetRootFaceNPC()), а не в actorNPC.
	if (!onlyTints && !m_morphSliderValues.empty()) {

		EnsureAllocated(actorFace->morphSliderValues, "morphSliderValues", actor);

		auto& sliders = *actorFace->morphSliderValues;

		// Полная замена: сначала сбрасываем старые ванильные значения,
		// иначе слайдеры которых нет в пресете остаются с ванильными значениями
		// и смешиваются с новыми — итог: испорченная геометрия лица.
		// LooksMenu делает то же самое (полная замена, а не merge).
		if (!sliders.empty()) {
			sliders.clear();
		}

		for (const auto& [key, value] : m_morphSliderValues) {
			sliders.emplace(key, value);
		}

		flags |= RE::RESET_3D_FLAGS::kFace;
		success = true;
	}

	// Применяем регионы лицевых костей (Morphs/Regions).
	// Движок нового поколения читает facialBoneRegionSliderValues из GetRootFaceNPC().
	// Пишем в actorFace, а не в actorNPC.
	if (!onlyTints && !m_facialBoneRegionSliderValues.empty()) {

		actorFace->ResetFacialBoneValues();

		int verifyCount = 0;
		for (const auto& [key, value] : m_facialBoneRegionSliderValues) {
			actorFace->SetFacialBoneRegionTransform(key, value);
			// Верификация: читаем обратно и сравниваем для первых 2 регионов
			if (verifyCount < 2) {
				RE::BGSCharacterMorph::Transform readback{};
				actorFace->GetFacialBoneRegionTransform(key, readback);
				logger::info("[NPC] FacialBoneRegion verify: actor={:#x} key={} "
					"wrote=({:.3f},{:.3f},{:.3f}|{:.3f},{:.3f},{:.3f}|{:.3f}) "
					"read=({:.3f},{:.3f},{:.3f}|{:.3f},{:.3f},{:.3f}|{:.3f})",
					actor->formID, key,
					static_cast<double>(value.position.x), static_cast<double>(value.position.y), static_cast<double>(value.position.z),
					static_cast<double>(value.rotation.x), static_cast<double>(value.rotation.y), static_cast<double>(value.rotation.z),
					static_cast<double>(value.scale.x),
					static_cast<double>(readback.position.x), static_cast<double>(readback.position.y), static_cast<double>(readback.position.z),
					static_cast<double>(readback.rotation.x), static_cast<double>(readback.rotation.y), static_cast<double>(readback.rotation.z),
					static_cast<double>(readback.scale.x)
				);
				++verifyCount;
			}
		}

		flags |= RE::RESET_3D_FLAGS::kFace;
		success = true;
	} else if (!onlyTints) {
		// Если facialBoneRegionSliderValues пустые или bypassed — это может быть причиной сброса translate причёски.
		bool bypassed = (!m_facialBoneRegionSliderValues.empty());
		logger::info("[NPC] FacialBoneRegion SKIPPED: actor={:#x} preset='{}' empty={} bypassed={}",
			actor->formID, id(),
			m_facialBoneRegionSliderValues.empty(),
			bypassed
		);
	}

	// Применяем значения preset-весов морфов (Morphs/Values) — 5 float.
	// Пишем в actorFace (GetRootFaceNPC()) — движок читает из root face NPC.
	if (!onlyTints && !m_morphRegionSliderValues.empty()) {

		EnsureAllocated(actorFace->morphRegionSliderValues, "morphRegionSliderValues", actor);
		auto& sliders = *actorFace->morphRegionSliderValues;

		if (!sliders.empty()) {
			sliders.clear();
		}

		sliders = std::move(MorphRegionSliderValues{ m_morphRegionSliderValues.begin(), m_morphRegionSliderValues.end() });

		flags |= RE::RESET_3D_FLAGS::kFace;
		success = true;
	}

	// Применяем тинты через новый API с добавлением недостающих и порядком
	auto find = [](const std::vector<uint32_t>& sortOrder, uint32_t id) -> size_t {
		auto it = std::find(sortOrder.begin(), sortOrder.end(), id);
		return static_cast<size_t>(it - sortOrder.begin());
	};

	if (!m_tints.empty()) {
		// EnsureAllocated: та же причина, что и у headRelatedData выше — после detach
		// (схема 2) tintingData у актёра обычно nullptr, иначе Tint::applyTints
		// молча ничего не применит (AddTint бейлится на "tintingData is null").
		EnsureAllocated(actorFace->tintingData, "tintingData", actor);

		bool tintsApplied = false;
		if (!m_tintOrder.empty()) {
			std::vector<Tint*> orderedTints;
			orderedTints.reserve(m_tints.size());
			for (const auto& uniqueTint : m_tints) {
				orderedTints.push_back(uniqueTint.get());
			}

			std::sort(orderedTints.begin(), orderedTints.end(), [this, find](const Tint* a, const Tint* b) {
				auto indexA = find(m_tintOrder, a->id());
				auto indexB = find(m_tintOrder, b->id());

				// Элементы не в порядке идут в конец
				if (indexA == m_tintOrder.size() && indexB != m_tintOrder.size()) return false;
				if (indexB == m_tintOrder.size() && indexA != m_tintOrder.size()) return true;
				if (indexA == m_tintOrder.size() && indexB == m_tintOrder.size()) {
					return a->id() < b->id();  // Сортировка по ID для элементов вне порядка
				}

				return indexA < indexB;
				});

			tintsApplied = Tint::applyTints(actorFace, orderedTints);
		}
		else {
			tintsApplied = Tint::applyTints(actorFace, m_tints);
		}

		logger::info("NPCPreset::applyImpl tints: count={}, order={}, applied={}", m_tints.size(), tintOrderToApply.size(), tintsApplied);
		
		if (tintsApplied) {
		 flags |= RE::RESET_3D_FLAGS::kFace;
		 success = true;
		 // Немедленно обновляем кэш TESNPC::bodyTintColor, чтобы фоновая сборка тела/рук
		 // (QueuedCharacter -> GetBodyTintColor с force=false) сразу брала новый цвет кожи,
		 // и планируем покраску шейдеров уже загруженного 3D на игровом потоке.
		 ActorResetQueue::SyncNPCSkinTintCache(actor);
		 if (actor->Get3D()) {
			 ActorResetQueue::GetSingleton().Schedule(actor, ActorResetQueue::ResetEntry{ .needsSkinTintRefresh = true });
		 }
		}
	}

	// SetFacialBoneMorphIntensity — вызываем на actorFace (GetRootFaceNPC()),
	// т.к. движок читает intensity из root face NPC.
	// ВАЖНО: если пресет не содержит Intensity — сбрасываем в 1.0f, а не оставляем ванильное значение NPC.
	// NPC вроде Solomon имеют FMIN=3.0 в esm-данных; оставить это значение при новых трансформах
	// из пресета (которые рассчитаны под intensity=1.0) даёт искажённое лицо.
	// LooksMenu поступает так же: defaults intensity to 1.0f if not present in preset JSON.
	if (!onlyTints && actor != RE::PlayerCharacter::GetSingleton()) {
		float intensity = m_facialMorphIntensity.value_or(1.0f);
		actorFace->SetFacialBoneMorphIntensity(intensity);
	}

	static auto toLower = [](const std::string& s) -> std::string {
		std::string result = s;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return result;
		};
	
	if (success && flags != RE::RESET_3D_FLAGS::kNone) {
		if (reset3d && actor->GetFullyLoaded3D()) {
			if (actorFace) actorFace->AddChange(0x800);
			if (actorNPC) actorNPC->AddChange(0x800);
			if (actor && actor->GetFullyLoaded3D() && actor->currentProcess) actor->currentProcess->Set3DUpdateFlag(actor->currentProcess->GetAll3DUpdateFlags() | flags);
		}

		// Диагностика: логируем что именно было применено, чтобы установить причину артефактов с позицией причёски.
		// flags показывает какие части 3D перестраиваются — если kHead без kFace, facial bone transforms не применяются.
		logger::info("[NPC] Apply OK: actor={:#x} preset='{}' flags={:#x} headParts={} faceBoneRegions={} morphSliders={} morphRegions={}",
			actor->formID, id(),
			static_cast<uint32_t>(flags),
			m_headParts.size(),
			m_facialBoneRegionSliderValues.size(),
			m_morphSliderValues.size(),
			m_morphRegionSliderValues.size()
		);
		return true;
	}

	logger::warn("[NPC] Apply FAIL: actor={:#x} preset='{}' success={}",
		actor->formID, id(), success
	);

	// UB-фикс: не-void функция обязана вернуть значение на всех путях.
	return false;
}

bool NPCPreset::empty() const noexcept
{
	return type() <= PresetType::NONE || type() >= PresetType::END || id().empty() || 
	(
		m_conditions.onlyTints() && m_tints.empty()
	) || 
	(
		(m_tints.empty()) &&
		m_morphSliderValues.empty() &&
		m_morphRegionSliderValues.empty() &&
		m_facialBoneRegionSliderValues.empty() &&
		m_facialMorphIntensity.has_value() == false &&
		!m_hairColor &&
		!m_faceHairColor &&
		m_headParts.empty()
	);
}

void NPCPreset::clear() noexcept {
	m_tints.clear();
	m_tintOrder.clear();  // Добавляем очистку m_tintOrder
	m_morphSliderValues.clear();
	m_morphRegionSliderValues.clear();
	m_facialBoneRegionSliderValues.clear();
	m_facialMorphIntensity.reset();
	m_hairColor = nullptr;
	m_faceHairColor = nullptr;
	m_headParts.clear();
	Preset::clear();
}

std::future<bool> NPCPreset::isValidAsync() const noexcept {
	static bool runOnce = false;
	if (!runOnce) {
		runOnce = true;
	 std::set<uint32_t> validM, validF;
		auto& validator = ValidateTint::validateTint();

		// Проверяем мужские оверлеи
		for (const auto& id : ALL_ITEMS_M) {
			//Потому что пока не придумал как сделать реальную валидацию. Пока фейк. Проблема в том, что 
			// 1. Стрим текструрных архивов открывается как-то иначе. (всегда возвращает false)
			// 2. Тинты добавленные плагинами загружаются как-то сильно позже чем луксменюшные, уже после загрузки игры.
			auto fut = validator(id);
			if (fut.get()) {
				validM.insert(id);
			}
		}
		// Проверяем женские оверлеи
		for (const auto& id : ALL_ITEMS_F) {
			//Потому что пока не придумал как сделать реальную валидацию. Пока фейк. Проблема в том, что 
			// 1. Стрим текструрных архивов открывается как-то иначе. (всегда возвращает false)
			// 2. Тинты добавленные плагинами загружаются как-то сильно позже чем луксменюшные, уже после загрузки игры.
			auto fut = validator(id);
			if (fut.get()) {
				validF.insert(id);
			}
		}
		ALL_ITEMS_M.swap(validM);
		ALL_ITEMS_F.swap(validF);
	}

	// Если пресет пустой — сразу false
	if (empty()) {
		return std::async(std::launch::deferred, [] { return false; });
	}

	// Копируем id оверлеев для потокобезопасности
	std::vector<uint32_t> ids;
	ids.reserve(m_tints.size());
	for (const auto& tint : m_tints) {
		ids.push_back(tint->id());
	}

	// Для каждого id запускаем асинхронную валидацию
	std::vector<std::future<bool>> futures;
	futures.reserve(ids.size());
	for (const auto& id : ids) {
		futures.push_back(validate(id));
	}

	// Возвращаем общий future, который завершится, когда все проверки завершатся
	return std::async(std::launch::async, [futures = std::move(futures), ids, this]() mutable {
		bool anyValid = false;
		for (size_t i = 0; i < futures.size(); ++i) {
			// Ждём завершения каждой проверки, но не блокируем основной поток, т.к. isValidAsync сам асинхронный
			if (futures[i].get()) {
				anyValid = true;
			}
		}
		if (!anyValid) {
			logger::error("OverlayPreset {} is not valid: no valid overlays found.", id().empty() ? "[no id]" : id());
		}
		return anyValid;
		});
}

void NPCPreset::clearNpcMap() {
	std::lock_guard lock(m_npcPresetsMtx);
	m_npcPresets.clear();
}

bool NPCPreset::isFaceNpcModified(RE::Actor* actor) {
	if (!actor) return false;
	auto npc = actor->GetNPC();
	if (!npc) return false;
	auto face = npc->GetRootFaceNPC();
	if (!face) return false;
	std::lock_guard lock(m_npcPresetsMtx);
	return m_npcPresets.count(face->formID) > 0;
}

bool NPCPreset::loadFromFile(const std::string& presetFile)
{
	// Очищаем предыдущие данные
	NPCPreset::clear();

	auto path = std::filesystem::path(presetFile.c_str());
	if (!std::filesystem::exists(path)) {
		logger::error("NPCPreset preset file does not exist: {}", presetFile);
		return false;
	}

	if (path.extension() != ".json") {
		logger::critical("NPCPreset::loadFromFile: Unsupported file format: {}", presetFile);
		NPCPreset::clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	if (presetFile.ends_with("_conds.json")) {
		// Если файл - это условия, то не загружаем его как пресет
		return false;
	}

	if (path.filename().string() == "conds.json") {
		// Если файл - это условия, то не загружаем его как пресет
		return false;
	}

	ParseLMPreset preset(path);
	if (!preset.isLoaded() || preset.isEmpty()) {
		logger::error("NPCPreset::loadFromFile: Failed to load preset from file: {}", presetFile);
		NPCPreset::clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	// Первым делом проверяем есть ли условия, т.к. если их нет, то и загружать дальше нет смысла
	auto conditions_path = path.parent_path() / (path.stem().string() + "_conds.json");
	if (std::filesystem::exists(conditions_path)) {
		LoadConditions(conditions_path);
	}

	if (m_conditions.empty()) {
		m_conditions = std::move(preset.conditions());  // Загружаем условия из основного файла, если нет отдельного файла с CONDITIONS
	}

	if (m_conditions.empty()) {
		conditions_path = path.parent_path() / "conds.json"; // Пробуем загрузить условия из файла "conds.json" в той же папке
		if (std::filesystem::exists(conditions_path)) {
			LoadConditions(conditions_path);
		}
	}

	if (m_conditions.empty()) {
		logger::warn("NPCPreset::loadFromFile: No conditions found in preset file: {}", presetFile);
		return false;
	}

	if (auto sex = preset.gender(); sex != RE::Actor::None) {
		m_conditions.setGender(sex); // Устанавливаем пол из пресета, т.к. пресет не может быть применён к актёру другого пола.
	}

	// Копируем данные из ParseLMPreset в NPCPreset
	m_id = preset.id();
	m_tintOrder = std::move(preset.tintOrder());
	m_tints = std::move(preset.tints());
	m_morphSliderValues = std::move(preset.faceMorphsSliderValues());
	m_morphRegionSliderValues = std::move(preset.morphRegionSliderValues());
	m_facialBoneRegionSliderValues = std::move(preset.facialBoneRegionSliderValues());
	m_facialMorphIntensity = preset.facialMorphIntensity();
	m_hairColor = std::move(preset.hairColor());
	m_faceHairColor = std::move(preset.faceHairColor());
	m_headParts = std::move(preset.headParts());

	std::vector<RE::BGSHeadPart*> headpartsWithExtraParts{ m_headParts.begin(), m_headParts.end() };

	auto contains = [&headpartsWithExtraParts](const RE::BGSHeadPart* hpart) {
		for (auto& el : headpartsWithExtraParts) {
			if (el == hpart) return true;
		}
		return false;
	};

	for (size_t i = 0; i < headpartsWithExtraParts.size(); ++i) {
		if (!headpartsWithExtraParts[i]) continue;

		for (const auto& extraPart : headpartsWithExtraParts[i]->extraParts) {
			if (extraPart && !contains(extraPart)) {
				headpartsWithExtraParts.push_back(extraPart);
			}
		}
	}

	m_headParts = std::move(HeadParts{ headpartsWithExtraParts.begin(), headpartsWithExtraParts.end() });

	logger::info("NPCPreset::loadFromFile: {}. Loaded {} tints, {} face morphs, {} morph regions, {} face regions, intensity {}, {} hair color, {} face hair color, {} head parts",
		presetFile, 
		m_tints.size(), 
		m_morphSliderValues.size(),
		m_morphRegionSliderValues.size(),
		m_facialBoneRegionSliderValues.size(),
		m_facialMorphIntensity.has_value() ? std::to_string(*m_facialMorphIntensity) : "no value",
		m_hairColor ? m_hairColor->GetFormEditorID() : "no",
		m_faceHairColor ? m_faceHairColor->GetFormEditorID() : "no",
		m_headParts.size()
	);

	return !empty();
}

#include <sstream>

std::string NPCPreset::print() const
{
	using size_type = RE::BSTArray<float, RE::BSTArrayHeapAllocator>::size_type;
	
	std::ostringstream oss;
	oss << "NPCPreset: " << id() << "\n";
	oss << "  Type: " << GetPresetTypeString(type()) << "\n";

	// Tint Order
	oss << "  TintOrder: ";
	if (m_tintOrder.empty()) {
		oss << "(none)\n";
	} else {
		for (auto id : m_tintOrder) {
			oss << std::hex << std::showbase << id << " ";
		}
		oss << "\n";
	}

	// Tints
	oss << "  Tints: ";
	if (m_tints.empty()) {
		oss << "(none)\n";
	} else {
		oss << "\n";
		for (const auto& el : m_tints) {
			oss << "    id=" << std::hex << std::showbase << el->id() << ", percent=" << std::dec << (int)el->percent() << ", type=" << (int)el->getType() << "\n";
		}
	}

	// Morph Slider Values
	oss << "  MorphSliderValues: ";
	if (m_morphSliderValues.empty()) {
		oss << "(none)\n";
	} else {
		oss << "\n";
		for (const auto& [id, value] : m_morphSliderValues) {
			oss << "    id=" << std::hex << std::showbase << id << ", value=" << std::dec << value << "\n";
		}
	}

	// Morph Region Slider Values  
	oss << "  MorphRegionSliderValues: ";
	if (m_morphRegionSliderValues.empty()) {
		oss << "(none)\n";
	} else {
		oss << "\n";
		for (size_type i = 0; i < m_morphRegionSliderValues.size(); ++i) {
			oss << "    index=" << std::dec << i << ", value=" << m_morphRegionSliderValues[i] << "\n";
		}
	}

	// Facial Bone Region Slider Values
	oss << "  FacialBoneRegionSliderValues: ";
	if (m_facialBoneRegionSliderValues.empty()) {
		oss << "(none)\n";
	} else {
		oss << "\n";
		for (auto it = m_facialBoneRegionSliderValues.begin(); it != m_facialBoneRegionSliderValues.end(); ++it) {
			oss << "    id=" << std::hex << std::showbase << it->first << ", pos=(" << it->second.position.x << "," << it->second.position.y << "," << it->second.position.z << ")";
			oss << ", rot=(" << it->second.rotation.x << "," << it->second.rotation.y << "," << it->second.rotation.z << ")";
			oss << ", scale=(" << it->second.scale.z << "," << it->second.scale.y << "," << it->second.scale.z << ")\n";
		}
	}

	if (m_facialMorphIntensity) {
		oss << "  FacialMorphIntensity: " << *m_facialMorphIntensity << "\n";
	} else {
		oss << "  FacialMorphIntensity: (none)\n";
	}

	// Hair Color
	oss << "  HairColor: ";
	if (m_hairColor) {
		oss << m_hairColor->GetFormEditorID() << "\n";
	} else {
		oss << "(none)\n";
	}

	// Face Hair Color
	oss << "  FaceHairColor: ";
	if (m_faceHairColor) {
		oss << m_faceHairColor->GetFormEditorID() << "\n";
	} else {
		oss << "(none)\n";
	}

	// Head Parts
	oss << "  HeadParts: ";
	if (m_headParts.empty()) {
		oss << "(none)\n";
	} else {
		oss << "\n";
		for (const auto& hp : m_headParts) {
			if (hp) {
				oss << "    FormID=" << std::hex << std::showbase << hp->GetFormID() << ", EditorID=" << hp->GetFormEditorID() << "\n";
			}
		}
	}

	// Conditions
	oss << "  Conditions: ";
	if (m_conditions.empty()) {
		oss << "(none)\n";
	} else {
		oss << m_conditions.print();
	}
	return oss.str();
}