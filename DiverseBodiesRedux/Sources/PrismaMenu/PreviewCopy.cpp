#include "PCH.h"
#include "PrismaMenu/PreviewCopy.h"

#include "ActorsManager/ActorsManager.h"
#include "LooksMenu/LooksMenuInterfaces.h"
#include "globals.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
	constexpr auto  kPollInterval = std::chrono::milliseconds(50);
	constexpr auto  kLoadTimeout = std::chrono::seconds(12);
	constexpr auto  kMinSettle = std::chrono::milliseconds(700);
	constexpr auto  kMaxSettle = std::chrono::milliseconds(2500);
	constexpr float kPi = 3.14159265f;

	// Где на экране стоит копия: нормированный x её центра (0 - середина, -1 - левый край).
	// Панель меню занимает правую часть.
	constexpr float kScreenX = -0.32f;

	constexpr float kDefaultDistance = 200.0f;  // в рост, чтобы была видна экипировка
	constexpr float kMinDistance = 60.0f;
	constexpr float kMaxDistance = 420.0f;

	constexpr auto kFriendlyFaction = "Fallout4.esm|0x03E0C8";  // CaptiveFaction

	RE::NiPoint3 Forward(float a_heading) { return { std::sin(a_heading), std::cos(a_heading), 0.0f }; }
	RE::NiPoint3 Right(float a_heading) { return { std::cos(a_heading), -std::sin(a_heading), 0.0f }; }

	// Горизонтальный угол обзора сцены, градусы.
	float WorldFOV()
	{
		const auto* camera = RE::PlayerCamera::GetSingleton();
		const float fov = camera ? camera->worldFOV : 0.0f;
		return (fov > 20.0f && fov < 140.0f) ? fov : 80.0f;
	}

	RE::NiPoint3 EyePosition(const RE::PlayerCharacter* a_player)
	{
		if (const auto* camera = RE::PlayerCamera::GetSingleton(); camera && camera->cameraRoot) {
			return camera->cameraRoot->world.translate;
		}
		auto position = a_player->GetPosition();
		position.z += 120.0f;
		return position;
	}

	bool IsFirstPerson()
	{
		const auto* camera = RE::PlayerCamera::GetSingleton();
		return camera && camera->currentState && camera->currentState.get() == camera->cameraStates[RE::CameraState::kFirstPerson].get();
	}

	// Game.ForceFirstPerson: PlayerCamera::SetState на состояние от первого лица.
	void ForceFirstPerson()
	{
		if (auto* camera = RE::PlayerCamera::GetSingleton(); camera && camera->cameraStates[RE::CameraState::kFirstPerson]) {
			camera->SetState(camera->cameraStates[RE::CameraState::kFirstPerson].get());
		}
	}

	void ForceThirdPerson()
	{
		if (auto* camera = RE::PlayerCamera::GetSingleton()) {
			camera->Force3rdPerson();
		}
	}

	RE::ObjectRefHandle Spawn(RE::TESBoundObject* a_object, const RE::NiPoint3& a_position, float a_heading)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* cell = player ? player->GetParentCell() : nullptr;
		auto* data = RE::TESDataHandler::GetSingleton();
		if (!cell || !data || !a_object) {
			return {};
		}
		RE::NEW_REFR_DATA request;
		request.location = a_position;
		request.direction = { 0.0f, 0.0f, a_heading };
		request.object = a_object;
		request.interior = cell->IsInterior() ? cell : nullptr;
		request.world = cell->IsInterior() ? nullptr : cell->worldSpace;
		request.initializeScripts = false;
		return data->CreateReferenceAtLocation(request);
	}

	template <class F>
	void ForEachEquippedArmor(RE::Actor* a_actor, F&& a_visit)
	{
		std::vector<RE::TESObjectARMO*> armors;
		if (auto* list = a_actor ? a_actor->inventoryList : nullptr) {
			const RE::BSAutoReadLock lock{ list->rwLock };
			for (auto& item : list->data) {
				auto* armor = item.object ? RE::fallout_cast<RE::TESObjectARMO*>(item.object) : nullptr;
				if (!armor) {
					continue;
				}
				for (auto* stack = item.stackData.get(); stack; stack = stack->nextStack.get()) {
					if (stack->IsEquipped()) {
						armors.push_back(armor);
						break;
					}
				}
			}
		}
		// Вне лока: экипировка меняет список.
		for (auto* armor : armors) {
			a_visit(armor);
		}
	}

	void Equip(RE::Actor* a_actor, RE::TESObjectARMO* a_armor)
	{
		if (a_actor->GetInventoryObjectCount(a_armor) == 0) {
			a_actor->AddObjectToContainer(a_armor, {}, 1, nullptr, RE::ITEM_REMOVE_REASON::kNone);
		}
		const RE::BGSObjectInstance instance(a_armor, nullptr);
		RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, instance, 0, 1, nullptr, false, true, false, true, false);
	}

	void Unequip(RE::Actor* a_actor, RE::TESObjectARMO* a_armor)
	{
		const RE::BGSObjectInstance instance(a_armor, nullptr);
		RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, &instance, 0xFFFFFFFF, nullptr, 0, false, true, false, true, nullptr);
	}

	// Новый актёр проявляется через high process (HighProcessData fadeState/fadeAlpha, двигает
	// AIProcess::FadeUpdate) и BSFadeNode. У копии это не продвигается, она оставалась прозрачной:
	// выставляем оба. HighProcessData в этом CommonLibF4 только объявлена; смещения из более нового
	// CommonLibF4 (OG): fadeState 0x220, fadeAlpha 0x224.
	void ForceVisible(RE::Actor* a_actor)
	{
		if (auto* process = a_actor->currentProcess; process && process->high) {
			auto* base = reinterpret_cast<std::uint8_t*>(process->high);
			*reinterpret_cast<std::uint32_t*>(base + 0x220) = 0;  // FADE_STATE::kNormal
			*reinterpret_cast<float*>(base + 0x224) = 1.0f;
		}
		if (auto* root = a_actor->Get3D(); root && root->IsFadeNode()) {
			root->IsFadeNode()->currentFade = 1.0f;
		}
	}

	RE::TESForm* FormByIdentifier(const char* a_identifier)
	{
		const std::string identifier = a_identifier;
		const auto bar = identifier.find('|');
		if (bar == std::string::npos) {
			return nullptr;
		}
		return functions::getFormFromString(identifier.substr(bar + 1), identifier.substr(0, bar));
	}

	// Весь внешний вид оригинала из LooksMenu (скин, оверлеи, морфы) - в том числе все пресеты DBR,
	// уже применённые к нему.
	void CloneAppearance(RE::Actor* a_source, RE::Actor* a_target)
	{
		auto* skin = LooksMenuInterfaces<SkinInterface>::GetInterface();
		auto* overlay = LooksMenuInterfaces<OverlayInterface>::GetInterface();
		auto* morph = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
		if (skin) {
			skin->CloneSkinOverride(a_source, a_target);
			skin->UpdateSkinOverride(a_target, true);
		}
		if (overlay) {
			overlay->CloneOverlays(a_source, a_target);
			overlay->UpdateOverlays(a_target);
		}
		if (morph) {
			morph->CloneMorphs(a_source, a_target);
			morph->UpdateMorphs(a_target);
		}
		logger::info("PreviewCopy: appearance cloned {:08X} -> {:08X}", a_source->GetFormID(), a_target->GetFormID());
	}

	// У копии в LooksMenu не должно остаться данных (они попали бы в co-save).
	void ForgetAppearance(RE::Actor* a_actor)
	{
		auto* skin = LooksMenuInterfaces<SkinInterface>::GetInterface();
		auto* overlay = LooksMenuInterfaces<OverlayInterface>::GetInterface();
		auto* morph = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
		for (const bool female : { false, true }) {
			if (overlay) overlay->RemoveAll(a_actor, female);
			if (morph) morph->ClearMorphs(a_actor, female);
		}
		if (skin) skin->RemoveSkinOverride(a_actor);
	}
}

PreviewCopy& PreviewCopy::get()
{
	static PreviewCopy instance;
	return instance;
}

RE::Actor* PreviewCopy::Copy() const { return m_copy ? functions::getFormByFormID<RE::Actor>(m_copy) : nullptr; }
RE::Actor* PreviewCopy::Target() const { return m_target ? functions::getFormByFormID<RE::Actor>(m_target) : nullptr; }

void PreviewCopy::Begin(const Source& a_source, ReadyCallback a_ready)
{
	End();
	auto* player = RE::PlayerCharacter::GetSingleton();
	auto* target = a_source.actor;
	auto* source = target ? target->GetNPC() : a_source.npc;
	if (!player || !source || target == player) {
		if (a_ready) a_ready(false);
		return;
	}
	m_stored = a_source.presets;
	m_targetLoaded = target && target->GetFullyLoaded3D();

	// Камера: запоминаем режим и наклон, уходим в первое лицо, смотрим ровно.
	m_cameraChanged = true;
	m_wasThirdPerson = !IsFirstPerson();
	m_savedPitch = player->data.angle.x;
	player->data.angle.x = 0.0f;
	if (m_wasThirdPerson) {
		ForceFirstPerson();
	}

	// База копий: дубликат NPC цели, один на сессию, перед каждым показом заполняется целью заново.
	if (m_npc) {
		m_npc->Copy(source);
	} else {
		auto* duplicate = source->CreateDuplicateForm(false, nullptr);
		m_npc = duplicate ? RE::fallout_cast<RE::TESNPC*>(duplicate) : nullptr;
		if (!m_npc) {
			logger::critical("PreviewCopy: couldn't duplicate the NPC {:08X}", source->GetFormID());
			End();
			if (a_ready) a_ready(false);
			return;
		}
		logger::info("PreviewCopy: duplicate base {:08X}", m_npc->GetFormID());
	}

	// Copy() заменил ключевые слова целевыми: возвращаем те, что не пускают AAF / NAF к копии.
	for (const auto* identifier : { "AAF.esm|0x022BB0" /* ActorBlocked */, "NAF.esp|0x003" /* NAF_DoNotUse */ }) {
		if (auto* form = FormByIdentifier(identifier)) {
			if (auto* keyword = RE::fallout_cast<RE::BGSKeyword*>(form); keyword && !m_npc->HasKeyword(keyword)) {
				m_npc->AddKeyword(keyword);
			}
		}
	}
	// Одежда: у загруженного оригинала одеваем вручную ровно то, что надето (Dress); у остальных
	// его инвентарь не заполнен, там остаётся одежда по умолчанию из базы.
	if (m_targetLoaded && target->inventoryList) {
		m_npc->defOutfit = nullptr;
	}
	m_npc->sleepOutfit = nullptr;

	// Дружелюбна ко всем: фракция - CaptiveFaction (в её отношениях с фракциями, которые бывают
	// враждебны, везде "friend"), никакой своей фракции преступлений, агрессия 0.
	m_npc->factions.clear();
	if (auto* form = FormByIdentifier(kFriendlyFaction)) {
		if (auto* faction = RE::fallout_cast<RE::TESFaction*>(form)) {
			m_npc->factions.push_back(RE::FACTION_RANK{ faction, 0 });
		}
	} else {
		logger::warn("PreviewCopy: {} not found -- the copy has no faction", kFriendlyFaction);
	}
	m_npc->crimeFaction = nullptr;
	m_npc->aiData.aggression = 0;

	m_target = target ? target->GetFormID() : 0;
	m_turn = 0.0f;
	m_distance = kDefaultDistance;
	m_lift = 0.0f;
	m_lastRoot = nullptr;
	m_yaw = player->data.angle.z;
	m_eye = player->GetPosition();
	m_eye.z += 120.0f;

	// События загрузки могут прийти уже внутри создания ссылки: DBR пока не знает id копии.
	ActorsManager::setPreviewSpawning(true);
	const auto pos = m_eye + Forward(m_yaw) * m_distance;
	const auto handle = Spawn(m_npc, { pos.x, pos.y, player->GetPosition().z }, m_yaw + kPi);
	auto       copy = handle.get();
	if (!copy) {
		ActorsManager::setPreviewSpawning(false);
		logger::critical("PreviewCopy: couldn't place the copy");
		End();
		if (a_ready) a_ready(false);
		return;
	}
	m_copy = copy->GetFormID();
	ActorsManager::setPreviewCopy(m_copy);
	ActorsManager::setPreviewSpawning(false);

	if (auto* actor = RE::fallout_cast<RE::Actor*>(copy.get())) {
		// AI копии не работает: стоит на месте, не атакует, не колдует, не умирает.
		actor->SetRestrained(true);
		actor->boolFlags.set(RE::Actor::BOOL_FLAGS::kAttackingDisabled, RE::Actor::BOOL_FLAGS::kCastingDisabled, RE::Actor::BOOL_FLAGS::kEssential);
	}
	logger::info("PreviewCopy: copy {:08X} of {:08X} spawned (original {}, {} stored preset(s))", m_copy, m_target,
		target ? (m_targetLoaded ? "loaded" : "not loaded") : "not in memory", m_stored.size());

	m_onReady = std::move(a_ready);
	m_ready = false;
	m_dressed = false;
	m_spawnedAt = std::chrono::steady_clock::now();
	m_poller.Start(kPollInterval, [this] { Poll(); });
}

void PreviewCopy::Poll()
{
	if (!m_copy) {
		return;
	}
	auto* copy = Copy();
	if (m_ready) {
		// Пока показана - держим на месте и проявленной. Без коллизий копию ничто не держит на земле,
		// поэтому позиция выставляется каждый тик; пересобранный 3D (пресет на копии) - заново без
		// коллизий и с привязкой к сцене.
		if (copy) {
			if (auto* root = copy->Get3D(); root != m_lastRoot) {
				copy->DisableCollision();
				AttachIfDetached();
				m_lastRoot = root;
			}
			Place();
			ForceVisible(copy);
		}
		return;
	}
	const auto now = std::chrono::steady_clock::now();
	if (!m_dressed) {
		if (copy && copy->Get3D()) {
			Dress();
		} else if (now - m_spawnedAt > kLoadTimeout) {
			m_poller.Stop();
			logger::warn("PreviewCopy: the copy's 3D never loaded -- giving up");
			auto callback = std::move(m_onReady);
			End();
			if (callback) callback(false);
		}
		return;
	}

	// Свежая ссылка проявляется первые кадры (BSFadeNode::currentFade 0 -> 1), голова и броня
	// цепляются чуть позже. Ждём, пока проявится (или достаточно долго), потом форсируем.
	FollowCamera();
	AttachIfDetached();
	Place();
	if (copy) {
		ForceVisible(copy);
	}
	auto*      root = copy ? copy->Get3D() : nullptr;
	auto*      fade = root ? root->IsFadeNode() : nullptr;
	const auto since = now - m_dressedAt;
	const bool faded = fade && fade->currentFade >= 0.99f;
	if ((faded && since > kMinSettle) || since > kMaxSettle) {
		if (fade) {
			fade->currentFade = 1.0f;
		}
		Finish();
	}
}

void PreviewCopy::Dress()
{
	auto* copy = Copy();
	auto* target = Target();
	if (!copy) {
		auto callback = std::move(m_onReady);
		End();
		if (callback) callback(false);
		return;
	}

	// Одеваем точно как загруженный оригинал; у остального остаётся одежда по умолчанию.
	if (target && m_targetLoaded) {
		ForEachEquippedArmor(copy, [&](RE::TESObjectARMO* a_armor) { Unequip(copy, a_armor); });
		ForEachEquippedArmor(target, [&](RE::TESObjectARMO* a_armor) { Equip(copy, a_armor); });
	}

	copy->SetScale(target ? static_cast<float>(target->refScale) / 100.0f : 1.0f);
	if (target) {
		CloneAppearance(target, copy);
	}
	// Оригинал не загружен или его нет вовсе: его внешний вид - это сохранённые пресеты, кладём их
	// на копию сами (поверх того, что нашлось в LooksMenu).
	if (!m_targetLoaded) {
		for (const auto& preset : m_stored) {
			ActorsManager::get().applyActorPreset(copy, preset, true);
		}
	}

	copy->DisableCollision();  // копию можно таскать вверх-вниз, и она ни во что не упирается
	m_lastRoot = copy->Get3D();

	AttachIfDetached();
	FollowCamera();  // камера уже встала в первое лицо
	Place();
	m_dressed = true;
	m_dressedAt = std::chrono::steady_clock::now();
}

void PreviewCopy::Finish()
{
	if (!Copy()) {
		auto callback = std::move(m_onReady);
		End();
		if (callback) callback(false);
		return;
	}
	m_ready = true;
	logger::info("PreviewCopy: copy ready");
	if (auto callback = std::move(m_onReady)) {
		callback(true);
	}
}

void PreviewCopy::Place()
{
	auto* copy = Copy();
	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!copy || !player) {
		return;
	}
	const float side = m_distance * kScreenX * std::tan(WorldFOV() * 0.5f * kPi / 180.0f);
	auto        pos = m_eye + Forward(m_yaw) * m_distance + Right(m_yaw) * side;
	pos.z = player->GetPosition().z + m_lift;

	// Лицом к камере плюс поворот пользователем.
	const float facing = std::atan2(m_eye.x - pos.x, m_eye.y - pos.y) + m_turn;
	copy->SetPosition(pos, true);
	copy->data.angle.z = facing;
	copy->Update3DPosition(true);
}

void PreviewCopy::FollowCamera()
{
	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return;
	}
	m_eye = EyePosition(player);
	m_yaw = player->data.angle.z;
}

void PreviewCopy::AttachIfDetached()
{
	auto* copy = Copy();
	auto* root = copy ? copy->Get3D() : nullptr;
	if (!root || root->parent) {
		return;
	}
	// Вешаем туда, где висят загруженные NPC (узел актёров ячейки), не туда, где 3D игрока: от
	// первого лица та ветка скрыта, и копия вместе с ней.
	RE::NiNode* parent = nullptr;
	const auto  consider = [&](RE::Actor* a_actor) {
		if (parent || !a_actor || a_actor == RE::PlayerCharacter::GetSingleton() || a_actor->GetFormID() == m_copy) {
			return;
		}
		if (auto* other = a_actor->Get3D(); other && other->parent) {
			parent = other->parent;
		}
	};
	consider(Target());
	if (auto* lists = RE::ProcessLists::GetSingleton()) {
		for (const auto& handle : lists->highActorHandles) {
			consider(handle.get().get());
		}
	}
	// Никого вокруг: корень самой ячейки.
	if (!parent) {
		if (auto* cell = copy->GetParentCell(); cell && cell->loadedData) {
			parent = cell->loadedData->cell3D.get();
		}
	}
	if (!parent) {
		logger::warn("PreviewCopy: the copy's 3D has no scene parent and none was found to attach it to");
		return;
	}
	parent->AttachChild(root, true);
	RE::NiUpdateData update;
	root->Update(update);
	m_attachedByUs = true;
	logger::info("PreviewCopy: the copy's 3D wasn't in the scene -- attached");
}

void PreviewCopy::DeleteRef()
{
	if (auto* actor = Copy()) {
		actor->Disable();
		actor->SetDelete(true);
	}
	m_copy = 0;
}

void PreviewCopy::End()
{
	m_poller.Stop();
	m_onReady = nullptr;
	if (auto* copy = Copy()) {
		ForgetAppearance(copy);
		if (auto* root = copy->Get3D(); m_attachedByUs && root && root->parent) {
			root->parent->DetachChild(root);
		}
	}
	m_attachedByUs = false;
	m_stored.clear();
	m_lastRoot = nullptr;
	DeleteRef();
	ActorsManager::setPreviewCopy(0);
	m_target = 0;
	m_ready = false;
	m_dressed = false;
	if (!m_cameraChanged) {
		return;
	}
	m_cameraChanged = false;
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->data.angle.x = m_savedPitch;
	}
	if (m_wasThirdPerson) {
		ForceThirdPerson();
		m_wasThirdPerson = false;
	}
	logger::info("PreviewCopy: ended");
}

void PreviewCopy::Rotate(float a_degrees)
{
	m_turn = std::fmod(m_turn + a_degrees * kPi / 180.0f, 2.0f * kPi);
	Place();
}

void PreviewCopy::Zoom(float a_steps)
{
	m_distance = std::clamp(m_distance * (1.0f - 0.1f * a_steps), kMinDistance, kMaxDistance);
	Place();
}

void PreviewCopy::Lift(float a_units)
{
	m_lift = std::clamp(m_lift + a_units, -120.0f, 260.0f);
	Place();
}
