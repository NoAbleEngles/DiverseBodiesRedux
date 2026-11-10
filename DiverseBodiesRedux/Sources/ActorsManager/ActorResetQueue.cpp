#include "ActorResetQueue.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <chrono>
#include <thread>

namespace logger = F4SE::log;

namespace
{
	// TESNPC::GetBodyTintColor (Fallout4.exe 1.11.240.0 RVA 0x655910 / NG_ID 2207435; 1.10.163 RVA 0x5B58E0).
	// Возвращает цвет кожи NPC в out. force=true — заново найти в tintingData запись слота
	// kSkinTone и пересчитать кэш TESNPC::bodyTintColor{R,G,B,A}; force=false — вернуть кэш.
	// Голова зовёт её с force=true (поэтому берёт свежие тинты), тело и ладони — с force=false.
	// 5-й аргумент (байт на стеке) во всех вызовах движка равен 0.
	using GetBodyTintColor_t = void* (*)(RE::TESNPC*, RE::NiColorA*, void*, bool, bool);

	// BSVisit::VisitSkinTint (Fallout4.exe 1.11.240.0 RVA 0x6EF070) и
	// BSVisit::RefreshFaceAndSkinRenderState (RVA 0x6E1140) — нативная пара из TESNPC::UpdateSkinTint (RVA 0x6E31F0),
	// которая обходит граф 3D, записывает skinTintColor (material + 0xC0) во все BSLightingShaderMaterialSkinTint
	// (feature == 5) и обновляет константные буферы геометрии (0x1421C8CA0) без пересборки мешей/физики ткани.
	using VisitSkinTint_t = void (*)(RE::NiAVObject*, const RE::NiColorA*);
	using RefreshFaceAndSkinRenderState_t = std::uint32_t (*)(RE::NiAVObject*, bool*);

	inline bool getBodyTintColor(RE::TESNPC* npc, RE::NiColorA& out, bool force) {
		if (!npc) return false;
		REL::Relocation<GetBodyTintColor_t> func{ REL::Module::get().base() + 0x655910 };
		out = RE::NiColorA{};
		func(npc, &out, nullptr, force, false);
		return true;
	}

	inline void paintNodeSkinTint(RE::NiAVObject* root, const RE::NiColorA& color) {
		if (!root) return;
		REL::Relocation<VisitSkinTint_t> visitSkinTint{ REL::Module::get().base() + 0x6EF070 };
		REL::Relocation<RefreshFaceAndSkinRenderState_t> refreshRenderState{ REL::Module::get().base() + 0x6E1140 };
		visitSkinTint(root, &color);
		bool flag = false;
		refreshRenderState(root, &flag);
	}

	inline uint32_t packBodyTint(const RE::TESNPC* npc) {
		return (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorA)) << 24) |
		       (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorR)) << 16) |
		       (static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorG)) << 8) |
		        static_cast<uint32_t>(static_cast<uint8_t>(npc->bodyTintColorB));
	}
}

// ---------------------------------------------------------------------------
// Синглтон и управление фоновым потоком очереди
// ---------------------------------------------------------------------------

ActorResetQueue::ActorResetQueue() = default;

ActorResetQueue::~ActorResetQueue()
{
	m_stop = true;
	m_cv.notify_all();
	if (m_workerThread.joinable()) {
		m_workerThread.join();
	}
}

static ActorResetQueue s_instance;

ActorResetQueue& ActorResetQueue::GetSingleton() noexcept
{
	return s_instance;
}

void ActorResetQueue::StartWorker()
{
	if (!m_workerThread.joinable()) {
		m_stop = false;
		m_workerThread = std::thread(&ActorResetQueue::WorkerLoop, this);
		logger::info("[ActorResetQueue] Dedicated rate-limited worker thread started");
	}
}

void ActorResetQueue::WorkerLoop()
{
	while (!m_stop) {
		uint32_t formID = 0;
		{
			std::unique_lock lock(m_mutex);
			m_cv.wait(lock, [this] { return m_stop.load() || !m_order.empty(); });
			if (m_stop) break;

			formID = m_order.front();
			m_order.pop_front();
		}

		auto done = std::make_shared<std::atomic<bool>>(false);
		auto didReset3D = std::make_shared<std::atomic<bool>>(false);
		F4SE::GetTaskInterface()->AddTask([this, formID, done, didReset3D]() {
			didReset3D->store(ExecuteReset(formID));
			done->store(true);
		});

		// Ждём пока игровой поток завершит сброс этого конкретного актёра
		while (!done->load() && !m_stop) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		if (m_stop) break;

		// Задержка 200 мс нужна только тогда, когда реально запускался асинхронный Reset3D.
		// Для быстрой перекраски скин-тинта (5 мкс) или пропущенных Get3D=false записей пауза не нужна.
		if (didReset3D->load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}
}

// ---------------------------------------------------------------------------
// Schedule
// ---------------------------------------------------------------------------

void ActorResetQueue::Schedule(RE::Actor* actor, ResetEntry entry, bool priority)
{
	if (!actor) return;
	const uint32_t id = actor->GetFormID();

	{
		std::lock_guard lock(m_mutex);

		// Мержим флаги, если запись уже есть
		auto it = m_pending.find(id);
		if (it != m_pending.end()) {
			it->second.merge(entry);
		} else {
			m_pending[id] = entry;
		}

		const bool isPlayer = (actor == RE::PlayerCharacter::GetSingleton());
		if (priority || isPlayer) {
			// Поднимаем в начало deque (убираем старое вхождение, вставляем в front)
			auto pos = std::find(m_order.begin(), m_order.end(), id);
			if (pos != m_order.end()) {
				m_order.erase(pos);
			}
			m_order.push_front(id);
		} else {
			// Добавляем в конец очереди только если ещё не запланирован
			if (std::find(m_order.begin(), m_order.end(), id) == m_order.end()) {
				m_order.push_back(id);
			}
		}

		StartWorker();
	}

	m_cv.notify_one();
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

void ActorResetQueue::Clear() noexcept
{
	std::lock_guard lock(m_mutex);
	m_pending.clear();
	m_order.clear();
	logger::info("[ActorResetQueue] Cleared (game loading started)");
}

// ---------------------------------------------------------------------------
// ExecuteReset — вызывается из ProcessNext на игровом потоке
// ---------------------------------------------------------------------------

bool ActorResetQueue::ExecuteReset(uint32_t formID)
{
	ResetEntry entry{};
	{
		std::lock_guard lock(m_mutex);
		auto it = m_pending.find(formID);
		if (it == m_pending.end()) {
			logger::info("[ActorResetQueue] ExecuteReset: actor={:#x} — no pending entry (cleared), skip", formID);
			return false;
		}
		entry = it->second;
		m_pending.erase(it);
	}

	// Получаем актёра. Если выгружен — пропускаем.
	RE::Actor* actor = nullptr;
	if (auto* form = RE::TESForm::GetFormByID(formID); form) {
		actor = RE::fallout_cast<RE::Actor*>(form);
	}

	if (!actor) {
		logger::warn("[ActorResetQueue] ExecuteReset: actor {:#x} not found (unloaded), skip", formID);
		return false;
	}

	// --- Цвет кожи тела/ладоней: синхронизируем кэш bodyTintColor и красим шейдеры 3D ---
	DoRefreshSkinTint(actor);

	if (!entry.needsBodyReset && !entry.needsHeadReset && !entry.needsDetach && !entry.needsOverlayUpdate) {
		return false;
	}

	if (!actor->GetFullyLoaded3D()) {
		// Get3D=false: движок ещё строит геометрию для этого актёра.
		// Reset3D здесь не нужен — движок сам прочитает уже обновлённые данные TESNPC
		// и построит правильную геометрию. Это нормальный путь при загрузке ячейки.
		logger::warn("[ActorResetQueue] ExecuteReset: actor={:#x} Get3D=false — skip Reset3D "
			"(engine builds geometry from TESNPC data directly)",
			formID);
		return false;
	}

	logger::info("[ActorResetQueue] ExecuteReset START: actor={:#x} name='{}' "
		"body={} head={} detach={} headFix={} overlayUpdate={} flags3D=0x{:x}",
		formID,
		actor->GetDisplayFullName() ? actor->GetDisplayFullName() : "?",
		entry.needsBodyReset, entry.needsHeadReset,
		entry.needsDetach, entry.needsHeadFix, entry.needsOverlayUpdate,
		actor->currentProcess ? static_cast<uint32_t>(actor->currentProcess->GetAll3DUpdateFlags()) : 0u);

	using R3D = RE::RESET_3D_FLAGS;

	// --- DetachOverlays (только если действительно обновляются оверлеи) ---
	if (entry.needsDetach && entry.needsOverlayUpdate) {
		logger::info("[ActorResetQueue] DoDetachOverlays: actor={:#x}", formID);
		DoDetachOverlays(actor);
	}

	// --- UpdateOverlays: применить таблицу оверлеев LooksMenu к текущему 3D ---
	if (entry.needsOverlayUpdate) {
		if (auto* overlayInterface = LooksMenuInterfaces<OverlayInterface>::GetInterface(); overlayInterface) {
			const bool ok = overlayInterface->UpdateOverlays(actor);
			logger::info("[ActorResetQueue] UpdateOverlays: actor={:#x} result={}", formID, ok);
		}
	}

	// --- Reset3D execution: ровно один вызов Reset3D на актёра ---
	auto* procBytes = actor->currentProcess ? reinterpret_cast<std::uint8_t*>(actor->currentProcess) : nullptr;
	const bool hasMiddleHigh = procBytes && (*reinterpret_cast<void* const*>(procBytes + 0x08) != nullptr);
	const bool hasHigh       = procBytes && (*reinterpret_cast<void* const*>(procBytes + 0x10) != nullptr);

	if (entry.needsHeadReset && !hasMiddleHigh) {
		logger::info("[ActorResetQueue] actor={:#x} has no middleHigh process (StartsDead/LOW) — suppressing head reset", formID);
		entry.needsHeadReset = false;
	}

	bool resetCalled = false;

	if (entry.needsBodyReset && !entry.needsHeadReset) {
		// Только тело: исключаем голову (excludeFlags = kHead | kFace)
		logger::info("[ActorResetQueue] Body Reset3D: actor={:#x} hasHigh={}", formID, hasHigh);
		if (procBytes && !hasHigh) {
			// Для StartsDead / LOW-process трупов (high == nullptr) движок игнорирует Update3DModel,
			// пока не взведён флаг currentProcess->unkE4 (+0xE4), как это делает Actor::EquipObject.
			procBytes[0xE4] = 1;
		}
		actor->Reset3D(false, R3D::kNone, true, R3D::kHead | R3D::kFace);
		if (procBytes && !hasHigh) {
			procBytes[0xE4] = 0;
		}
		resetCalled = true;
		logger::info("[ActorResetQueue] Body Reset3D called: actor={:#x} flags3D=0x{:x}",
			formID,
			actor->currentProcess ? static_cast<uint32_t>(actor->currentProcess->GetAll3DUpdateFlags()) : 0u);
	}
	else if (entry.needsHeadReset) {
		// Голова (или голова + тело): если нужно и тело, обновляем флаги для обоих
		if (auto* npc = actor->GetNPC(); npc) {
			if (entry.needsHeadFix) {
				if (auto* face = npc->GetRootFaceNPC(); face) {
					face->AddChange(0x800); // CHANGE_ACTOR_FACE_MORPHS
					npc->AddChange(0x800);
				}
			}
		}
		logger::info("[ActorResetQueue] {}Reset3D: actor={:#x}", entry.needsBodyReset ? "Head+Body " : "Head ", formID);
		const auto excludeFlags = entry.needsBodyReset ? R3D::kNone : (R3D::kModel | R3D::kSkin | R3D::kSkeleton | R3D::kScale);
		actor->Reset3D(false, entry.needsBodyReset ? R3D::kNone : (R3D::kHead | R3D::kFace), true, excludeFlags);
		resetCalled = true;
	}

	if (resetCalled) {
		DoRefreshSkinTint(actor);
	}

	logger::info("[ActorResetQueue] ExecuteReset DONE: actor={:#x}", formID);
	return resetCalled;
}

// ---------------------------------------------------------------------------
// SyncNPCSkinTintCache & DoRefreshSkinTint
//
// HEAD-пресет перезаписывает tintingData у faceNPC (в т.ч. тинт kSkinTone), и голова,
// которую движок собирает с force=true, получает новый цвет кожи. Тело и ладони читают
// кэш TESNPC::bodyTintColor, который никто не обновлял, — отсюда разный цвет кожи.
// Пересчитываем цвет из тинтов faceNPC, переносим в кэш базового NPC актёра и красим
// skin-tint шейдеры уже загруженного 3D (BSVisit::VisitSkinTint + RefreshFaceAndSkinRenderState,
// идентично нативной TESNPC::UpdateSkinTint @ RVA 0x6E31F0 без пересборки тела/физики ткани).
// ---------------------------------------------------------------------------

bool ActorResetQueue::SyncNPCSkinTintCache(RE::Actor* actor, RE::NiColorA* outColor)
{
	if (!actor) return false;

	auto* npc = actor->GetNPC();
	if (!npc) return false;
	auto* face = npc->GetRootFaceNPC();
	auto* source = face ? face : npc;

	const uint32_t npcBefore = packBodyTint(npc);
	const uint32_t faceBefore = packBodyTint(source);

	RE::NiColorA color{};
	if (!getBodyTintColor(source, color, true)) return false;

	// Распространяем пересчитанный кэш цвета и данные тинтов по всей цепочке шаблонов:
	for (auto* cur = npc; cur; cur = cur->faceNPC) {
		cur->bodyTintColorR = source->bodyTintColorR;
		cur->bodyTintColorG = source->bodyTintColorG;
		cur->bodyTintColorB = source->bodyTintColorB;
		cur->bodyTintColorA = source->bodyTintColorA;
		if (!cur->tintingData && source->tintingData) {
			cur->tintingData = source->tintingData;
		}
	}

	if (outColor) {
		*outColor = color;
	}

	logger::info("[SkinTint] actor={:#x} npc={:#x} face={:#x} cache npc 0x{:08x}->0x{:08x}, face 0x{:08x}->0x{:08x}, color=({:.3f},{:.3f},{:.3f},{:.3f})",
		actor->GetFormID(), npc->GetFormID(), source->GetFormID(),
		npcBefore, packBodyTint(npc), faceBefore, packBodyTint(source),
		color.r, color.g, color.b, color.a);
	return true;
}

void ActorResetQueue::DoRefreshSkinTint(RE::Actor* actor)
{
	RE::NiColorA color{};
	if (!SyncNPCSkinTintCache(actor, &color)) return;

	// Красим все BSLightingShaderMaterialSkinTint (feature == 5) уже загруженного 3D
	// (торс, руки, кисти, ноги, открытые участки кожи на одежде) без вызова Reset3D тела.
	auto* root3D = actor->Get3D(false);
	if (root3D) {
		paintNodeSkinTint(root3D, color);
	}

	if (const auto& biped = actor->GetBiped(false); biped && biped->root && biped->root != root3D) {
		paintNodeSkinTint(biped->root, color);
	}

	if (actor == RE::PlayerCharacter::GetSingleton()) {
		auto* root1P = actor->Get3D(true);
		if (root1P && root1P != root3D) {
			paintNodeSkinTint(root1P, color);
		}
		if (const auto& biped1P = RE::PlayerCharacter::GetSingleton()->GetBiped(true);
			biped1P && biped1P->root && biped1P->root != root1P && biped1P->root != root3D) {
			paintNodeSkinTint(biped1P->root, color);
		}
	}
}

// ---------------------------------------------------------------------------
// DoDetachOverlays — портирован из ResetBodyMorphs::Run
// ---------------------------------------------------------------------------

void ActorResetQueue::DoDetachOverlays(RE::Actor* actor)
{
	if (!actor) return;

	auto* bodyMorphInterface = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
	auto* overlayInterface   = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!bodyMorphInterface || !overlayInterface) return;

	const bool isPlayer = (actor == RE::PlayerCharacter::GetSingleton());
	uint32_t seenSlots = 0, morphableSlots = 0, destroyedSlots = 0;
	RE::BipedAnim* equipData[2];
	equipData[0] = actor->GetBiped().get();
	equipData[1] = isPlayer ? RE::PlayerCharacter::GetSingleton()->GetBiped(true).get() : nullptr;

	for (uint32_t s = 0; s < (isPlayer ? 2u : 1u); ++s) {
		if (!equipData[s]) continue;
		for (uint32_t i = 0; i < 32; ++i) {
			RE::NiPointer<RE::NiAVObject> slotNode =
				equipData[s]->object[i].partClone ? equipData[s]->object[i].partClone : nullptr;
			if (!slotNode) continue;
			++seenSlots;
			if (!IsNodeMorphable(slotNode.get())) continue;
			++morphableSlots;

			RE::NiNode* parent = slotNode->parent;
			if (!parent) continue;

			RE::NiNode* rootNode = GetRootNode(actor, slotNode.get());
			if (!rootNode) continue;

			RE::NiNode* overlayRoot = overlayInterface->GetOverlayRoot(actor, rootNode);
			if (overlayRoot) {
				overlayInterface->DestroyOverlaySlot(actor, overlayRoot, i);
				++destroyedSlots;
			}
		}
	}

	logger::info("[ActorResetQueue] DetachOverlays: actor={:#x} slots(seen={} morphable={} destroyed={})",
		actor->GetFormID(), seenSlots, morphableSlots, destroyedSlots);
}
