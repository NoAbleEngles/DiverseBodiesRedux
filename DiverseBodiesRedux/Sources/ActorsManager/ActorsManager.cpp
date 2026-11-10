#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
//#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
//#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
//#define NOUSER
#define NONLS
//#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
//#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <Windows.h>

#undef GetEnvironmentVariable
#undef GetFileVersionInfo
#undef GetFileVersionInfoSize
#undef GetModuleFileName
#undef GetModuleHandle
#undef MessageBox
#undef OutputDebugString
#undef VerQueryValue

#include <zlib.h>
#include "ActorsManager.h"
#include "Version.h"
#include <string>
#include <sstream>
#include <atomic>
#include <thread>
#include <chrono>
#include "Details/PresetsGenerator.hpp"
#include "Overrides/SessionOverrides.h"
#include "Overrides/SavedFormId.h"
#include <RE/Bethesda/Calendar.h>
#include <RE/Bethesda/BSFaceGenNiNode.h>
#include <RE/NetImmerse/NiNode.h>
#include <functional>

using namespace functions;
using namespace globals;


std::atomic_bool ActorsManager::m_gameInLoadingState{};

// --- JSON сериализация/десериализация пресетов ---
boost::json::value to_json(const ActorsManagerDefs::ActorPreset& actorPresets);
std::ostringstream& operator<<(std::ostringstream& os, const ActorsManagerDefs::ActorPreset& actorPresets);
ActorsManagerDefs::ActorPreset fromJson(const boost::json::object& obj);
ActorsManagerDefs::ActorPreset& operator<<(ActorsManagerDefs::ActorPreset& actorPreset, std::istringstream& is);
concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& operator<<(concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets, std::istringstream& is);
std::ostringstream& operator<<(std::ostringstream& os, const concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets);

namespace TESObjectREFR_Events {
	// Регистрация для события загрузки объекта
	void RegisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* sink);
	// Отмена регистрации для события загрузки объекта
	void UnregisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* sink);
	// Регистрация для события инициализации объекта
	void RegisterForInitScript(RE::BSTEventSink<RE::TESInitScriptEvent>* a_sink);
	// Отмена регистрации для события инициализации объекта
	void UnregisterForInitScript(RE::BSTEventSink<RE::TESInitScriptEvent>* a_sink);
}

namespace {
	std::atomic<uint32_t> s_previewCopy{ 0 };
	std::atomic_bool      s_previewSpawning{ false };
}

void ActorsManager::setPreviewSpawning(bool spawning) noexcept { s_previewSpawning.store(spawning); }
void ActorsManager::setPreviewCopy(uint32_t formId) noexcept { s_previewCopy.store(formId); }
bool ActorsManager::isPreviewCopy(uint32_t formId) noexcept { return formId != 0 && s_previewCopy.load() == formId; }

std::vector<ActorsManager::StoredActor> ActorsManager::getStoredPresets() const {
	std::vector<StoredActor> result;
	std::lock_guard lock(m_mutex);
	for (const auto& entry : m_presets) {
		StoredActor stored{ entry.first, 0, {} };
		if (auto meta = m_meta.find(entry.first); meta != m_meta.end()) {
			stored.baseId = meta->second.baseId;
		}
		for (const auto& preset : entry.second) {
			if (preset) {
				stored.presets.emplace_back(preset->type(), preset->id());
			}
		}
		if (!stored.presets.empty()) {
			result.push_back(std::move(stored));
		}
	}
	return result;
}

ActorsManager& ActorsManager::get()
{
	static ActorsManager instance;
	return instance;
}

ActorsManager::ActorsManager() :
	m_waitingActors([this](uint32_t actorFormId) { 
		return applyByFormId(actorFormId);
	})
{
	F4SE::GetSerializationInterface()->SetSaveCallback(serializationCallback);
	F4SE::GetSerializationInterface()->SetLoadCallback(deserializationCallback);

	TESObjectREFR_Events::RegisterForObjectLoaded(this);
	TESObjectREFR_Events::RegisterForInitScript(this);

	//DoUpdate3DModel_Handler.Install();

	// Подписка на событие завершения десериализации
	subscribeOnDeserialized([this]() {
		// Проверяем готовность системы и делаем flush если готовы
		if (isReady()) {
			flushWaitingActors(true);
		}
		}, false);

	// Подписка на событие готовности PresetsManager
	PresetsManager::get().subscribeOnValidated([this]() {
		// Проверяем готовность системы и делаем flush если готовы
		if (isReady()) {
			flushWaitingActors(true);
		}
		}, false);

	m_excludedActors.refreshExclusionList();
}

bool ActorsManager::containsActor(const RE::Actor* actor) const noexcept {
	if (actor)
		return m_presets.find(actor->formID) == m_presets.end() ? false : true;
	return false;
}

bool ActorsManager::containsActor(uint32_t formId) const noexcept {
	return m_presets.find(formId) == m_presets.end() ? false : true;
}

bool ActorsManager::isReady() {
	return PresetsManager::get().isReady() && ActorsManager::get().m_deserialized.load(std::memory_order_acquire) && !m_gameInLoadingState.load(std::memory_order_acquire);
}

// Внутренняя функция для ProcessEvent и flush что бы дважды не искать актёра по formId
bool ActorsManager::applyByActorPtr(RE::Actor* actor, bool reset3d) {
	if (!actor)
		return false;
	if (auto it = m_presets.find(actor->formID); it != m_presets.end() && !isStaleEntry(actor)) {
		// If a higher-priority preset specifically targeting this actor's FormID exists,
		// upgrade and override any generic preset previously stored in co-save.
		auto candidatePresets = PresetsGenerator{ actor }.getRandomPresets();
		bool hasExactOverride = false;
		if (!candidatePresets.second.empty()) {
			for (const auto& p : candidatePresets.second) {
				if (p && p->check(actor) == CoincidenceLevel::FULL) {
					bool alreadyHas = false;
					for (const auto& curP : it->second) {
						if (curP && curP->id() == p->id()) {
							alreadyHas = true;
							break;
						}
					}
					if (!alreadyHas) {
						hasExactOverride = true;
						break;
					}
				}
			}
		}
		if (hasExactOverride) {
			logger::info("ActorsManager: Upgrading actor 0x{:x} to exact FormID preset.", actor->formID);
			updateActorPresetsInStorage(candidatePresets);
			return applyActorPresets(candidatePresets, reset3d);
		}
		return applyActorPresets(*it, reset3d);
	}
	else {
		auto preset = PresetsGenerator{ actor }.getRandomPresets();
		if (!preset.second.empty()) {
			// Fix B: сохраняем сгенерированный набор ДО применения. applyActorPresets
			// может отложить применение HEAD (голова ещё строится движком) и повторно
			// вызвать applyByFormId — тогда набор уже должен быть в m_presets.
			updateActorPresetsInStorage(preset);
			return applyActorPresets(preset, reset3d);
		}
	}
	return false;
}

// Внутренняя функция для ProcessEvent и flush
bool ActorsManager::applyByFormId(uint32_t actorFormId, bool reset3d) {
	return applyByActorPtr(checkActorByFormId(actorFormId), reset3d);
}

RE::BSEventNotifyControl ActorsManager::ProcessEvent(const RE::TESObjectLoadedEvent& a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* /*a_source*/)
{
	// Обработка события загрузки объекта (актеров)
	auto actor = checkActorByFormId(a_event.formId);
	if (!actor) return RE::BSEventNotifyControl::kContinue;

	if (a_event.loaded) {
		handleActorLoading(actor);
		// Если пресет головы был применён раньше (например на TESInitScriptEvent до загрузки 3D),
		// убеждаемся, что только что собранные меши тела/рук покрашены в актуальный скин-тон лица.
		bool hasHeadPreset = false;
		{
			std::lock_guard lock(m_mutex);
			if (auto it = m_presets.find(actor->formID); it != m_presets.end()) {
				for (const auto& p : it->second) {
					if (p && p->type() == PresetType::HEAD) {
						hasHeadPreset = true;
						break;
					}
				}
			}
		}
		if (hasHeadPreset) {
			m_resetQueue.Schedule(actor, ActorResetQueue::ResetEntry{ .needsSkinTintRefresh = true });
		}
	} else {
		// Актёр выгружен — убираем из кэша, чтобы следующая загрузка этого же актёра
		// прошла через handleActorLoading и получила свежие пресеты.
		std::lock_guard lock(m_recentlyLoadedActorsMtx);
		m_recentlyLoadedActors.erase(a_event.formId);
	}
		
	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ActorsManager::ProcessEvent(const RE::TESInitScriptEvent& a_event, RE::BSTEventSource<RE::TESInitScriptEvent>* /*a_source*/)
{
	auto actor = checkActor(RE::fallout_cast<RE::Actor*>(a_event.reference));
	if (!actor) return RE::BSEventNotifyControl::kContinue;
	handleActorLoading(actor);
	return RE::BSEventNotifyControl::kContinue;
}

void ActorsManager::processingLogic(RE::Actor* actor) {
	if (!actor) return;
	
	logger::info("ActorsManager ProcessEvent : Actor loaded 0x{:x}", actor->formID);
		
	if (isReady()) {
		applyByActorPtr(actor);
	} else {
		m_waitingActors.push(actor->GetFormID());
	}
}

ActorsManager::CallbackId ActorsManager::subscribeOnDeserialized(const std::function<void()>& callback, bool oneShot) {
	std::lock_guard lock(m_deserializedCallbacksMutex);
	CallbackId id = m_nextDeserializedCallbackId++;
	m_deserializedCallbacks.emplace_back(id, [callback, oneShot, this, id]() {
		callback();
		if (oneShot) {
			unsubscribeFromDeserialized(id);
		}
		});
	return id;
}

bool ActorsManager::unsubscribeFromDeserialized(CallbackId id) {
	std::lock_guard lock(m_deserializedCallbacksMutex);
	auto it = std::remove_if(m_deserializedCallbacks.begin(), m_deserializedCallbacks.end(),
		[id](const auto& pair) { return pair.first == id; });
	if (it != m_deserializedCallbacks.end()) {
		m_deserializedCallbacks.erase(it, m_deserializedCallbacks.end());
		return true;
	}
	return false;
}

void ActorsManager::fireDeserialized() {
	logger::info("ActorsManager: Deserialization completed, firing callbacks.");
	std::vector<std::function<void()>> callbacks;
	{
		std::lock_guard lock(m_deserializedCallbacksMutex);
		for (auto& [id, cb] : m_deserializedCallbacks) {
			callbacks.push_back(cb);
		}
	}
	for (auto& cb : callbacks) {
		cb();
	}
}

void ActorsManager::clear() noexcept {
	std::lock_guard lock(m_mutex);
	logger::info("ActorsManager {} cleared!", m_presets.empty() ? 0 : m_presets.size());
	m_presets.clear();
	m_meta.clear();
}

// Записи co-save идут в том порядке, в котором их пишет serialize: сначала основная (пресеты актёров,
// Version::UID), потом оверрайды сессии (SessionOverrides). Незнакомые записи пропускаются.
void ActorsManager::deserialize(const F4SE::SerializationInterface* a_intrfc) {
	// --- Десериализация пресетов из co-save ---
	// Проверяем готовность PresetsManager
	if (!PresetsManager::get().isReady()) {
		logger::error("PresetsManager is not ready, skipping deserialization.");
		return;
	}

	m_deserialized.store(false, std::memory_order_release);
	clear(); // Очищаем текущие данные
	SessionOverrides::get().clear();
	NPCPreset::clearNpcMap();

	// Сброс карты актёров, если это указано в INI
	bool resetMap = false;
	if (auto& ini = *globals::g_ini; ini.reload(), ini.at("GENERAL/bResetMapOnNextLoad", false)) {
		logger::info("ActorsManager: Resetting recently loaded actors map.");
		ini.set("GENERAL/bResetMapOnNextLoad", false);
		NPCPreset::clearNpcMap();
		resetMap = true;
	}

	if (a_intrfc) {
		uint32_t recType{}, curVersion{}, length{};
		while (a_intrfc->GetNextRecordInfo(recType, curVersion, length)) {
			if (recType == Version::UID) {
				if (curVersion != Version::SerializationVer) {
					logger::critical("Invalid record version: expected {}, got {}", Version::SerializationVer, curVersion);
					continue;
				}
				if (!resetMap) {
					readPresetsRecord(a_intrfc, length);
				}
			}
			else if (recType == SessionOverrides::kRecordType) {
				SessionOverrides::get().Deserialize(a_intrfc, curVersion, length);
			}
			else {
				logger::warn("Unknown co-save record {} (version {}), skipped.", recType, curVersion);
			}
		}
	}
	else {
		logger::critical("ActorsManager: Serialization interface is not available.");
	}

	m_deserialized.store(true, std::memory_order_release);
	fireDeserialized();
}

// Основная запись: сжатый JSON-массив актёров. Любая ошибка не фатальна - карта остаётся пустой.
void ActorsManager::readPresetsRecord(const F4SE::SerializationInterface* a_intrfc, uint32_t length) {
	unsigned long fullSize{};

	if (length > INT_MAX) {
		logger::info("buf_size is more than {}", std::to_string(INT_MAX));
		return;
	}

	if (length <= sizeof(fullSize)) {
		logger::info("No serialized data.");
		return;
	}

	// Чтение сжатых данных
	std::vector<Bytef> compressedData(length);
	auto read = a_intrfc->ReadRecordData(compressedData.data(), length);
	if (read != length) {
		logger::warn("Failed to read the record data");
		return;
	}

	// Извлекаем размер разжатых данных
	if (compressedData.size() < sizeof(fullSize)) {
		logger::error("Compressed data is too small to contain fullSize.");
		return;
	}

	std::memcpy(&fullSize, compressedData.data() + compressedData.size() - sizeof(fullSize), sizeof(fullSize));
	compressedData.resize(compressedData.size() - sizeof(fullSize));

	// Разжимаем данные
	uLongf decompressedSize = static_cast<uLongf>(fullSize);
	std::string decompressedData(decompressedSize, '\0');
	if (uncompress(reinterpret_cast<Bytef*>(&decompressedData[0]), &decompressedSize, compressedData.data(), static_cast<uLong>(compressedData.size())) != Z_OK) {
		logger::error("Failed to decompress data.");
		return;
	}

	if (decompressedSize != fullSize) {
		logger::error("Decompressed data size does not match expected fullSize.");
		return;
	}

	// Десериализация JSON в m_presets (+ служебные поля base/seen в m_meta; старые сейвы их не имеют)
	{
		std::lock_guard lock(m_mutex);
		const std::string jsonText = decompressedData;
		std::istringstream iss(std::move(decompressedData));
		m_presets << iss;
		loadMetaLocked(jsonText);
	}
}

// Служебные поля записей: base - базовый NPC актёра, seen - игровой день, когда актёр последний
// раз был найден. По ним отсеиваются записи, FormID которых теперь указывает на другого актёра
// (временные 0xFF...... исчезают и переиспользуются), и давно пропавшие временные актёры.
void ActorsManager::loadMetaLocked(const std::string& jsonText) {
	try {
		auto jv = boost::json::parse(jsonText);
		if (!jv.is_array()) return;
		for (const auto& item : jv.as_array()) {
			if (!item.is_object()) continue;
			const auto& obj = item.as_object();
			const auto* formidValue = obj.if_contains("formid");
			if (!formidValue || !formidValue->is_number()) continue;
			const auto formid = ResolveSavedFormID(static_cast<uint32_t>(formidValue->to_number<int64_t>()));
			if (formid == 0 || m_presets.find(formid) == m_presets.end()) continue;
			ActorMeta meta;
			if (const auto* base = obj.if_contains("base"); base && base->is_number()) {
				meta.baseId = ResolveSavedFormID(static_cast<uint32_t>(base->to_number<int64_t>()));
			}
			if (const auto* seen = obj.if_contains("seen"); seen && seen->is_number()) {
				meta.seenDay = static_cast<float>(seen->to_number<double>());
			}
			if (meta.baseId != 0 || meta.seenDay >= 0.0f) {
				m_meta[formid] = meta;
			}
		}
	}
	catch (const std::exception& e) {
		logger::warn("ActorsManager: unreadable actor meta: {}", e.what());
	}
}

std::string ActorsManager::serializePresetsJson() {
	std::lock_guard lock(m_mutex);
	boost::json::array actorsArray;
	for (const auto& el : m_presets) {
		boost::json::object actorObj;
		actorObj["formid"] = el.first;

		boost::json::array presetsArray;
		for (const auto& preset : el.second) {
			if (preset) {
				boost::json::object presetObj;
				presetObj["type"] = static_cast<int>(preset->type());
				presetObj["id"] = preset->id();
				presetsArray.push_back(std::move(presetObj));
			}
		}
		actorObj["presets"] = std::move(presetsArray);
		if (auto meta = m_meta.find(el.first); meta != m_meta.end()) {
			if (meta->second.baseId != 0) actorObj["base"] = meta->second.baseId;
			if (meta->second.seenDay >= 0.0f) actorObj["seen"] = meta->second.seenDay;
		}
		actorsArray.push_back(std::move(actorObj));
	}
	return boost::json::serialize(actorsArray);
}

// Основная запись пишется всегда первой: старые версии мода читают только её. Оверрайды сессии
// идут следом отдельной записью, которую старая версия просто не видит.
void ActorsManager::serialize(const F4SE::SerializationInterface* a_intrfc) {
	writePresetsRecord(a_intrfc);
	SessionOverrides::get().Serialize(a_intrfc);
}

void ActorsManager::writePresetsRecord(const F4SE::SerializationInterface* a_intrfc) {
	// Перед записью выбрасываем недействительные записи и давно пропавших временных актёров.
	const auto pruned = pruneStalePresets(false);
	if (pruned.removed() > 0) {
		logger::info("ActorsManager: dropped {} stale and {} expired actor entries before saving.", pruned.invalid, pruned.expired);
	}

	if (m_presets.empty()) {
		logger::info("No presets to serialize.");
		return;
	}
	// --- Сериализация пресетов в co-save ---
	if (a_intrfc && a_intrfc->OpenRecord(Version::UID, Version::SerializationVer)) {
		// Сериализуем m_presets в строку
		std::string serializedData = serializePresetsJson();
		uint32_t fullSize{};

		if (serializedData.empty()) {
			logger::warn("Serialized data is empty, nothing to write.");
			return;
		}

		auto& ini = *globals::g_ini;
		auto serializationFilename = ini.at("DEBUG/sSerializationLog", std::string{});
		if (!serializationFilename.empty()) {
			//запустить отдельный поток, который создаст файл serialized.log и сохранит в него данные из ostream
			std::thread([serializedData, serializationFilename]() {
			std::ofstream logFile(serializationFilename);
			if (logFile.is_open()) {
				logFile << serializedData;
				logFile.close();
			}
			else {
				logger::error("Failed to open {} for writing.", serializationFilename);
			}
				}).detach();
		}

		fullSize = static_cast<uint32_t>(serializedData.size());

		// Сжимаем данные
		uLongf compressedSize = compressBound(static_cast<uLong>(serializedData.size()));
		std::vector<Bytef> compressedData(compressedSize);

		if (compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef*>(serializedData.data()), static_cast<uLong>(serializedData.size())) != Z_OK) {
			logger::error("Failed to compress serialized data.");
			return;
		}

		// Добавляем размер разжатых данных в конец
		compressedData.resize(static_cast<uLong>(compressedData.size()) + sizeof(fullSize));
		std::memcpy(compressedData.data() + compressedData.size() - sizeof(fullSize), &fullSize, sizeof(fullSize));

		// Записываем сжатые данные в интерфейс сериализации
		if (!a_intrfc->WriteRecordData(compressedData.data(), static_cast<uint32_t>(compressedData.size()))) {
			logger::warn("Failed to write all compressed data to the interface.");
		}
	}
	else {
		logger::critical("ActorsManager: Failed to open record for serialization. Serialization interface is not available.");
	}
}

namespace {
	float nowGameDays() {
		auto* calendar = RE::Calendar::GetSingleton();
		return calendar ? calendar->rawDaysPassed : 0.0f;
	}

	bool isTemporaryId(uint32_t formId) {
		return (formId >> 24) == 0xFF;
	}
}

void ActorsManager::touchMetaLocked(RE::Actor* actor) {
	if (!actor) return;
	auto* npc = actor->GetNPC();
	m_meta[actor->formID] = ActorMeta{ npc ? npc->formID : 0u, nowGameDays() };
}

// Запись актёра в карте не про него, если его базовый NPC теперь другой: временный FormID
// (0xFF......) мог указывать на другого персонажа (в том числе другой расы). Записи без служебных
// полей (из старых сейвов) считаются верными, поля заполняются при первом же совпадении.
bool ActorsManager::isStaleEntry(RE::Actor* actor) {
	if (!actor) return false;
	std::lock_guard lock(m_mutex);
	auto* npc = actor->GetNPC();
	if (auto meta = m_meta.find(actor->formID); meta != m_meta.end() && meta->second.baseId != 0 && npc && npc->formID != meta->second.baseId) {
		logger::warn("ActorsManager: entry 0x{:x} was stored for NPC 0x{:x}, but the actor is NPC 0x{:x} now -- dropped.",
			actor->formID, meta->second.baseId, npc->formID);
		m_presets.unsafe_erase(actor->formID);
		m_meta.erase(meta);
		return true;
	}
	touchMetaLocked(actor);
	return false;
}

// Чистка карты. Всегда выбрасывает записи, которые заведомо не про актёра: форма есть, но это не
// актёр; базовый NPC не тот, что при записи; раса не из списка. Временных актёров (0xFF......),
// которых сейчас нет в памяти, она трогает только когда они не находились дольше
// GENERAL/iTempActorExpireDays игровых дней (по умолчанию 40: игра сбрасывает ячейки раз в 30) или
// когда dropUnresolvedTemp. Остальные ненайденные (обычные формы из плагинов) остаются: ячейка
// просто не загружена.
ActorsManager::PruneResult ActorsManager::pruneStalePresets(bool dropUnresolvedTemp) {
	PruneResult result;
	std::lock_guard lock(m_mutex);
	const float now = nowGameDays();
	const float maxAge = static_cast<float>((std::max)(1, globals::g_ini ? globals::g_ini->at("GENERAL/iTempActorExpireDays", 40) : 40));

	std::vector<uint32_t> invalid;
	std::vector<uint32_t> expired;
	for (const auto& entry : m_presets) {
		const uint32_t id = entry.first;
		auto* form = RE::TESForm::GetFormByID(id);
		if (!form) {
			if (!isTemporaryId(id)) continue;
			auto meta = m_meta.find(id);
			if (dropUnresolvedTemp) {
				expired.push_back(id);
			}
			else if (meta == m_meta.end() || meta->second.seenDay < 0.0f) {
				m_meta[id].seenDay = now;  // с этого дня считаем, сколько его нет
			}
			else if (now - meta->second.seenDay > maxAge) {
				expired.push_back(id);
			}
			continue;
		}

		auto* actor = RE::fallout_cast<RE::Actor*>(form);
		if (!actor) {
			invalid.push_back(id);
			continue;
		}
		auto* npc = actor->GetNPC();
		auto meta = m_meta.find(id);
		if (meta != m_meta.end() && meta->second.baseId != 0 && npc && npc->formID != meta->second.baseId) {
			invalid.push_back(id);
			continue;
		}
		if (flst_qualifying_race && actor->race && !isQualifiedRace(actor)) {
			invalid.push_back(id);
			continue;
		}
		m_meta[id] = ActorMeta{ npc ? npc->formID : 0u, now };
	}

	for (const auto id : invalid) {
		m_presets.unsafe_erase(id);
		m_meta.erase(id);
	}
	for (const auto id : expired) {
		m_presets.unsafe_erase(id);
		m_meta.erase(id);
	}
	result.invalid = invalid.size();
	result.expired = expired.size();
	return result;
}

bool ActorsManager::forgetActor(uint32_t formId) {
	std::lock_guard lock(m_mutex);
	const bool had = m_presets.find(formId) != m_presets.end();
	m_presets.unsafe_erase(formId);
	m_meta.erase(formId);
	return had;
}

RE::Actor* ActorsManager::checkActorByFormId(uint32_t formId) {
	auto form = RE::TESForm::GetFormByID(formId);
	if (!form) {
		return nullptr;
	}
	auto actor = RE::fallout_cast<RE::Actor*>(form);
	return checkActor(actor);
}

const RE::Actor* ActorsManager::checkActor(const RE::Actor* actor) {
	if (!actor) {
		return nullptr;
	}
	if (!isQualifiedRace(actor)) {
		return nullptr;
	}
	if (isExcluded(actor)) {
		return nullptr;
	}
	return actor;
}

RE::Actor* ActorsManager::checkActor(RE::Actor* actor) {
	return const_cast<RE::Actor*>(checkActor(const_cast<const RE::Actor*>(actor)));
}

bool ActorsManager::applyActorPreset(RE::Actor* actor, const std::shared_ptr<Preset>& preset, bool force, bool reset3d) {
	auto veryfied_actor = RE::fallout_cast<const RE::Actor*>(actor);
	if (!veryfied_actor || !preset || preset->empty()) {
		return false;
	}

	auto npcPreset = std::dynamic_pointer_cast<NPCPreset>(preset);

	// Для BODYMORPHS и всех OVERLAY-наследников (BODYHAIRS, BODYTATTOOS, NAILS) reset3d передаётся
	// напрямую в apply(): они инкапсулируют весь цикл сброса сами и не нуждаются в doReset3D.
	//   - BodymorphsPreset: Reset3D асинхронно через ResetBodyMorphs::Run().
	//   - OverlayPreset и его наследники: Reset3D синхронно внутри apply().
	//
	// Для NPCPreset (HEAD) applyImpl выставляет Set3DUpdateFlag, но НЕ вызывает
	// actor->Reset3D() — поэтому reset3d подавляется (false), а полный сброс
	// делается ниже через doReset3D.
	const bool isSelfResetting = preset->type() != PresetType::HEAD;

	bool applied = force && npcPreset
		? npcPreset->forceApply(actor, isSelfResetting ? reset3d : false)
		: preset->apply(actor, isSelfResetting ? reset3d : false);

	// doReset3D вызывается только для HEAD (NPCPreset), который сам не зовёт actor->Reset3D().
	// Для всех остальных типов doReset3D лишний и ломает порядок операций.
	if (applied && reset3d && !isSelfResetting) {
		doReset3D(actor, { preset->type() });
	}

	logger::info("Called applyActorPreset for actor 0x{:x} with preset type {}. Applied: {}. Reset3D: {}", 
		(actor ? actor->formID : 0), 
		(preset ? GetPresetTypeString(preset->type()) : GetPresetTypeString(PresetType::END)), 
		applied, 
		reset3d
	);
	return applied;
}

bool ActorsManager::updateActorPresetInStorage(RE::Actor* actor, const std::shared_ptr<Preset>& preset) noexcept {
	if (!actor || !preset || preset->empty()) {
		logger::error("updateActorPresetInStorage: actor or preset is null/empty.");
		return false;
	}
	std::lock_guard lock(m_mutex);
	touchMetaLocked(actor);
	auto it = m_presets.find(actor->formID);
	if (it != m_presets.end()) {
		// Если актёр уже есть, делаем swap
		for (auto presets_it = it->second.begin(); presets_it != it->second.end();) {
			if (*presets_it && (*presets_it)->type() == preset->type()) {
				presets_it = it->second.erase(presets_it);
			} else {
				++presets_it;
			}
		}
		it->second.emplace(preset);
		return true;
	}
	else {
		// Если актёр не найден, добавляем новый
		ActorsManagerDefs::ActorPreset actorPresets{ actor->formID, {preset} };
		m_presets.insert(std::move(std::make_pair(actorPresets.first, std::move(actorPresets.second))));
		return true;
	}
}

bool ActorsManager::applyActorPresets(const ActorsManagerDefs::ActorPreset& actorPresets, bool reset3d) {
	auto actor = RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(actorPresets.first));
	if (!actor || actorPresets.second.empty() || !isQualifiedRace(actor) || isExcluded(actor)) {
		return false;
	}

	std::set<PresetType> appliedType;

	auto excludedTypes = getExcludedTypes(actor);

	// Структура для группировки пресетов по типам
	std::unordered_map<PresetType, std::shared_ptr<Preset>> presetsByType;
	for (const auto& preset : actorPresets.second) {
		if (preset && (!excludedTypes.has_value() || !(*excludedTypes)->contains(preset->type()))) {
			presetsByType[preset->type()] = preset;
		}
	}

	if (auto it = presetsByType.find(PresetType::HEAD); it != presetsByType.end()) {
		if (it->second->apply(actor, false)) {
			appliedType.insert(it->first);
		} else if (reset3d && NPCPreset::isFaceNpcModified(actor)) {
			// apply вернул false потому что faceNPC уже модифицирован другим актёром
			// с тем же NPC base. Данные в TESNPC уже правильные, но этот конкретный
			// actor ещё не получил doReset3D — без него Reset3D от BodyMorphs/Overlay
			// перестраивает голову с изменёнными headparts без правильных флагов kHead,
			// что даёт сломанную причёску.
			appliedType.insert(it->first);
		}
	}

	// Считаем всё что реально применилось, включая self-resetting типы.
	// appliedType используется только для doReset3D (HEAD), поэтому self-resetting
	// типы туда не входят — но для корректного return нужен отдельный флаг.
	bool anyApplied = !appliedType.empty();

	// Флаг: применяется ли HEAD (или isFaceNpcModified) одновременно с BODYMORPHS.
	// Если да — BODYMORPHS получает reset3d=false (только Detach оверлеев, без Reset3D),
	// а HEAD-reset добавляется как отдельный F4SE-таск (ResetActor) внутри m_doAfterAll
	// в конце ResetBodyMorphs::Run. Это гарантирует FIFO-порядок внутри F4SE-очереди:
	// 1. ResetBodyMorphs (Detach)
	// 2. ResetActor (Reset3D с kHead|kFace)
	// Без этого движковый Reset3D из doReset3D и F4SE-таск ResetBodyMorphs попадают
	// в разные очереди с недетерминированным порядком — отсюда сломанная причёска.
	const bool headApplied = appliedType.contains(PresetType::HEAD);
	const bool bodymorphsPresent = reset3d && presetsByType.contains(PresetType::BODYMORPHS);
	const bool needsDeferredHeadReset = reset3d && headApplied && bodymorphsPresent;

	// BODYMORPHS управляет своим Reset3D через асинхронный ResetBodyMorphs::Run().
	// Если HEAD тоже применяется (needsDeferredHeadReset) — передаём reset3d=false,
	// чтобы ResetBodyMorphs сделал только Detach. HEAD-reset добавим в m_doAfterAll ниже.
	// Если HEAD не применяется — BODYMORPHS делает Reset3D самостоятельно (reset3d=true).
	if (auto it = presetsByType.find(PresetType::BODYMORPHS); it != presetsByType.end()) {
		// reset3d=false при needsDeferredHeadReset: только Detach, Reset3D — через ResetActor.
		// reset3d=оригинальное значение иначе: BODYMORPHS самостоятельно делает Reset3D.
		bool bmReset = needsDeferredHeadReset ? false : reset3d;
		if (it->second->apply(actor, bmReset)) {
			anyApplied = true;
			// Намеренно не вставляем в appliedType — doReset3D для BODYMORPHS не нужен.
		}
	}

	// OverlayPreset-наследники (BODYHAIRS, BODYTATTOOS, NAILS) тоже self-resetting:
	// они вызывают Reset3D синхронно внутри своего apply().
	// Передаём оригинальный reset3d. В appliedType не добавляем —
	// чтобы doReset3D ниже не дублировал сброс.
	for (auto type = static_cast<int>(PresetType::BODYHAIRS); 
		 type < static_cast<int>(PresetType::END); 
		 ++type) {

		PresetType currentType = static_cast<PresetType>(type);

		if (currentType == PresetType::HEAD) {
			continue;
		}

		if (auto it = presetsByType.find(currentType); it != presetsByType.end()) {
			if (it->second->apply(actor, reset3d)) {
				anyApplied = true;
				// Overlay-типы self-resetting: в appliedType не добавляем.
			}
		}
	}

	if (reset3d && headApplied) {
		// Reset3D нужен только если 3D уже загружена. Если Get3D=false — движок
		// построит геометрию сам из уже записанных данных TESNPC. Планировать
		// Reset3D в этом случае бесполезно и порождает лишние таски в очереди.
		// Также пропускаем HEAD Reset3D для StartsDead/LOW-process трупов (middleHigh == nullptr),
		// так как AIProcess::DoUpdate3DModel(0x20) при завершении PendingActorHeadData требует middleHigh != nullptr.
		const auto* procBytes = actor->currentProcess ? reinterpret_cast<const std::uint8_t*>(actor->currentProcess) : nullptr;
		const bool hasMiddleHigh = procBytes && (*reinterpret_cast<void* const*>(procBytes + 0x08) != nullptr);
		if (!actor->GetFullyLoaded3D() || !hasMiddleHigh) {
			logger::info("applyActorPresets: actor={:#x} HEAD applied but Get3D={} hasMiddleHigh={} — "
				"skip HEAD Reset3D",
				actor->formID, actor->GetFullyLoaded3D() != nullptr, hasMiddleHigh);
		} else if (needsDeferredHeadReset) {
			// HEAD + BODYMORPHS: Reset3D делается через ResetActor — F4SE-таск,
			// который будет добавлен в конце ResetBodyMorphs::Run через m_doAfterAll.
			// Это гарантирует что Detach оверлеев завершится ДО Reset3D.
			// AddChange для faceNPC делаем здесь синхронно — до запуска таска.
			auto actorNPC = actor->GetNPC();
			if (actorNPC) {
				if (auto actorFace = actorNPC->GetRootFaceNPC(); actorFace) {
					actorFace->AddChange(0x800); // CHANGE_ACTOR_FACE_MORPHS
					actorNPC->AddChange(0x800);
				}
				if (actor->currentProcess) {
					using R3D = RE::RESET_3D_FLAGS;
					actor->currentProcess->Set3DUpdateFlag(
						actor->currentProcess->GetAll3DUpdateFlags() | R3D::kHead | R3D::kFace);
				}
			}
			// Маршрутизируем HEAD-сброс через ActorResetQueue.
			// needsHeadFix=true: внутри таска будут повторно проставлены AddChange(0x800),
			// что гарантирует правильный порядок: BODYMORPHS-DetachOverlays → HEAD-Reset3D.
			using R3D = RE::RESET_3D_FLAGS;
			logger::info("applyActorPresets: actor={:#x} HEAD+BODYMORPHS — schedule HEAD Reset3D via ActorResetQueue", actor->formID);
			m_resetQueue.Schedule(actor, ActorResetQueue::ResetEntry{
				.needsBodyReset = false,
				.needsHeadReset = true,
				.needsDetach    = false,
				.needsHeadFix   = true,
			});
		} else {
			// Только HEAD (без BODYMORPHS): используем синхронный doReset3D как прежде.
			doReset3D(actor, appliedType);
		}
	}

	return anyApplied;
}

void ActorsManager::doReset3D(RE::Actor* actor, const std::set<PresetType>& appliedTypes) {
	if (!actor || appliedTypes.empty()) {
		return;
	}

	logger::info("doReset3D: actor={:#x} has3D={} hasProcess={} hasExtraList={}",
		actor->formID,
		actor->Get3D() != nullptr,
		actor->currentProcess != nullptr,
		actor->extraList != nullptr
	);

	// Проверяем что у актёра вообще есть загруженная 3D-модель.
	// Без 3D сбрасывать нечего, а движок может упасть.
	if (!actor->Get3D()) {
		return;
	}

	const auto* procBytes = actor->currentProcess ? reinterpret_cast<const std::uint8_t*>(actor->currentProcess) : nullptr;
	const bool hasMiddleHigh = procBytes && (*reinterpret_cast<void* const*>(procBytes + 0x08) != nullptr);
	if (!hasMiddleHigh && appliedTypes.contains(PresetType::HEAD) && !appliedTypes.contains(PresetType::BODYMORPHS)) {
		logger::info("doReset3D: actor={:#x} has no middleHigh process (StartsDead/LOW) — skipping HEAD Reset3D", actor->formID);
		return;
	}

	auto actorNPC = (RE::TESNPC*)nullptr;
	auto actorFace = (RE::TESNPC*)nullptr;

	// NPC-данные нужны только если менялась голова или морфы тела.
	if (appliedTypes.contains(PresetType::HEAD) || appliedTypes.contains(PresetType::BODYMORPHS)) actorNPC = actor->GetNPC();
	if (appliedTypes.contains(PresetType::HEAD) && actorNPC) actorFace = actorNPC->GetRootFaceNPC();

	//auto forceRebuildHead = [this, actor, actorFace]() {
	//	RE::NiPointer<RE::BSFaceGenNiNode> newHead;
	//	bool built = RE::BSFaceGenUtils::CreateHeadForNPC(actorFace, newHead, /*highPoly*/ false, /*cache*/ false, /*pending*/ nullptr);

	//	if (built && newHead) {
	//		RE::BSFaceGenUtils::AttachHeadHelper(actorFace, newHead.get(), actor, actor);
	//		logger::info("AttachHeadHelper: attached newly built head node (children={})", newHead->children.size());
	//	}
	//	else {
	//		// Fallback: позволяем движку построить/переиспользовать голову внутри attach
	//		RE::BSFaceGenUtils::AttachHeadHelper(actorFace, nullptr, actor, actor);
	//		logger::info("AttachHeadHelper: attached using nullptr (engine-built)");
	//	}
	//};

	//if (faceChanged && actorFace) forceRebuildHead();

	auto flags = RE::RESET_3D_FLAGS::kNone;
	bool containsHead = appliedTypes.contains(PresetType::HEAD);
	bool containsBodyMorphs = appliedTypes.contains(PresetType::BODYMORPHS);

	// AddChange сообщает движку, что нужно перестроить соответствующие части геометрии.
	// 0x800 = CHANGE_ACTOR_FACE_MORPHS.
	// 0x4000 (CHANGE_ACTOR_FULL_LOD_SWAPPED) намеренно НЕ ставим — это заставляет движок
	// подменять геометрию на LOD-модели, ломая меши тела и одежды (пропадают ноги/бедра).
	if (containsHead && actorFace) actorFace->AddChange(0x800);
	if (containsHead && actorNPC) actorNPC->AddChange(0x800);

	/*if (containsHead) */flags |= RE::RESET_3D_FLAGS::kHead | RE::RESET_3D_FLAGS::kFace;

	auto furnRef = actor->GetFurnitureReference();

	if (actor->currentProcess) actor->currentProcess->Set3DUpdateFlag(actor->currentProcess->GetAll3DUpdateFlags() | flags);
	else logger::warn("Actor 0x{:x} has no current process to set 3D update flags.", actor->formID);

	// Если актёр сидит на мебели, Reset3D может застревать. Сначала выводим его оттуда.
	if (furnRef) {
		if (actor && actor->Get3D() && furnRef && furnRef->Get3D() && furnRef->GetParentCell()->IsCellLoaded()) {
			actor->StopInteractingQuick(true, true, true);
			actor->ClearPath();

			auto pos = furnRef->GetPosition();
			auto angle = furnRef->GetStartingAngle();

			actor->WarpTo(
				pos, angle.x, angle.y,
				furnRef->parentCell,
				furnRef->parentCell->GetWorldSpace(),
				false,
				false,
				actor == RE::PlayerCharacter::GetSingleton());
		}
	}

	// currentProcess и extraList обязательны: без них Reset3D вызывает краш.
	if (actor->currentProcess && actor->extraList) {
		using R3D = RE::RESET_3D_FLAGS;
		auto excludeFlags = containsBodyMorphs ? R3D::kNone : (R3D::kModel | R3D::kSkin | R3D::kSkeleton | R3D::kScale);
		actor->Reset3D(false, flags, true, excludeFlags);
	}
	if (containsHead) {
		ActorResetQueue::DoRefreshSkinTint(actor);
	}
}

bool ActorsManager::updateActorPresetsInStorage(const ActorsManagerDefs::ActorPreset& actorPresets) noexcept {
	std::lock_guard lock(m_mutex);
	touchMetaLocked(RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(actorPresets.first)));
	auto it = m_presets.find(actorPresets.first);
	if (it != m_presets.end()) {
		// Если актёр уже есть, делаем swap
		it->second = (actorPresets.second);
		return true;
	} else {
		// Если актёр не найден, добавляем новый
		m_presets.insert(std::move(std::make_pair(actorPresets.first, actorPresets.second)));
		return true;
	}
}

bool ActorsManager::containsPresetTypeForActor(uint32_t id, PresetType type) const noexcept {
	if (auto it = m_presets.find(id); it != m_presets.end()) {
		for (auto preset : it->second) {
			if (preset && preset->type() == type) {
				return true;
			}
		}
	}
	return false;
}

std::shared_ptr<Preset> ActorsManager::getActorPreset(uint32_t id, PresetType type) const noexcept {
	if (auto it = m_presets.find(id); it != m_presets.end()) {
		for (auto preset : it->second) {
			if (preset && preset->type() == type) {
				return preset;
			}
		}
	}
	return std::shared_ptr<Preset>{};
}

std::optional<std::shared_ptr<std::unordered_set<PresetType>>> ActorsManager::getExcludedTypes(const RE::Actor* actor) const noexcept {
	return m_excludedActors.getExcludedTypes(actor);
}

std::string ActorsManager::printExclusions() const noexcept {
	return m_excludedActors.print();
}

bool ActorsManager::isQualifiedRace(const RE::Actor* actor) noexcept {
	if (!actor || !actor->race || !flst_qualifying_race)
		return false;
	return (flst_qualifying_race->ContainsItem(actor->race));
}

bool ActorsManager::isExcluded(const RE::Actor* actor) noexcept {
	auto res = ActorsManager::get().getExcludedTypes(actor);
	if (!res.has_value()) {
		return false;
	}

	if ((*res) == nullptr || (*res)->empty()) {
		return true; // Если типы исключены, но список пуст - значит актёр полностью исключён
	}

	return false;
}

void ActorsManager::serializationCallback(const F4SE::SerializationInterface* a_intrfc) {
	ActorsManager::get().serialize(a_intrfc);
}

void ActorsManager::deserializationCallback(const F4SE::SerializationInterface* a_intrfc) {
	ActorsManager::get().deserialize(a_intrfc);
}

namespace TESObjectREFR_Events {
	void RegisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

		using func_t = decltype(&RegisterForObjectLoaded);
		REL::Relocation<func_t> func{ baseAddr + 0x52D940 /* 1.10.163: 0x436A40, NG_ID: 2201666 */ };
		return func(a_sink);
	}

	void UnregisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

		using func_t = decltype(&UnregisterForObjectLoaded);
		REL::Relocation<func_t> func{ baseAddr + 0x52D970 /* 1.10.163: 0x436A70, NG_ID: 2201667 */ };
		return func(a_sink);
	}

	void RegisterForInitScript(RE::BSTEventSink<RE::TESInitScriptEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

		using func_t = decltype(&RegisterForInitScript);
		REL::Relocation<func_t> func{ baseAddr + 0x52D560 /* 1.10.163: 0x436590, NG_ID: 2201651 */ };
		return func(a_sink);
	}

	void UnregisterForInitScript(RE::BSTEventSink<RE::TESInitScriptEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

		using func_t = decltype(&UnregisterForInitScript);
		REL::Relocation<func_t> func{ baseAddr + 0x52D590 /* 1.10.163: 0x4365C0, NG_ID: 2201652 */ };
		return func(a_sink);
	}
}

// --- JSON сериализация/десериализация пресетов ---
boost::json::value to_json(const ActorsManagerDefs::ActorPreset& actorPreset) {
	boost::json::object obj;
	obj["formID"] = actorPreset.first;

	boost::json::array presetsArray;
	for (const auto& presetPtr : actorPreset.second) {
		if (!presetPtr) continue;
		presetsArray.push_back(presetPtr->id().data());
	}
	obj["preset"] = std::move(presetsArray);

	return obj;
}

std::ostringstream& operator<<(std::ostringstream& os, const ActorsManagerDefs::ActorPreset& actorPreset) {
	os << boost::json::serialize(to_json(actorPreset));
	return os;
}

boost::json::value to_json(const concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets) {
	boost::json::array arr;
	for (const auto& [formID, presetSet] : presets) {
		arr.push_back(to_json(ActorsManagerDefs::ActorPreset{ formID, presetSet }));
	}
	return arr;
}

//std::ostringstream& operator<<(std::ostringstream& os, const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets) {
//	os << boost::json::serialize(to_json(presets));
//	return os;
//}

ActorsManagerDefs::ActorPreset fromJson(const boost::json::object& obj) {
	auto it = obj.find("formID");
	if (it == obj.end() || !it->value().is_int64()) {
		logger::error("Invalid or missing 'formID' in JSON object.");
		return { 0, {} };
	}

	uint32_t formID = static_cast<uint32_t>(it->value().as_uint64());
	auto actor = getFormByResolvedFormID<RE::Actor>(formID);

	if (!actor) {
		logger::error("Actor with formID {} not found.", formID);
		return { 0, {} };
	}

	std::pair<uint32_t, ActorsManagerDefs::Presets> actorPreset;
	actorPreset.first = actor->formID;

	it = obj.find("preset");
	if (it == obj.end() || !it->value().is_array()) {
		logger::error("Invalid or missing 'preset' in JSON object.");
		return { 0, {} };
	}

	for (const auto& item : it->value().as_array()) {
		if (item.is_string()) {
			auto preset = PresetsManager::get()[item.as_string().c_str()];
			if (!preset) {
				logger::error("Preset with ID '{}' not found.", item.as_string().c_str());
				continue;
			}
			actorPreset.second.emplace(preset);
		}
	}

	if (actorPreset.second.empty()) {
		logger::error("No valid presets found in JSON object for formID {}", actorPreset.first);
		return { 0, {} };
	}

	return actorPreset;
}

ActorsManagerDefs::ActorPreset& operator<<(ActorsManagerDefs::ActorPreset& actorPreset, std::istringstream& is) {
	boost::json::value jv;
	is >> jv;
	if (jv.is_object()) {
		auto obj = jv.as_object();
		auto tempActorPreset = fromJson(obj);
		actorPreset.first = tempActorPreset.first;
		actorPreset.second = std::move(tempActorPreset.second);
	}
	return actorPreset;
}

//std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& operator<<(std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets, std::istringstream& is) {
//	boost::json::value jv;
//	is >> jv;
//	if (jv.is_array()) {
//		auto arr = jv.as_array();
//		for (const auto& item : arr) {
//			if (item.is_object()) {
//				auto actorPreset = fromJson(item.as_object());
//				if (actorPreset.first != 0 && !actorPreset.second.empty()) {
//					// Добавляем в мапу только валидные пресеты
//					presets.emplace(actorPreset);
//				}
//			}
//		}
//	}
//	return presets;
//}

std::ostringstream& operator<<(std::ostringstream& stream, const concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets)
{
	// Формируем boost::json::array из всех актёров
	boost::json::array actorsArray;

	for (const auto& el : presets) {
		boost::json::object actorObj;
		actorObj["formid"] = el.first;

		boost::json::array presetsArray;
		for (const auto& preset : el.second) {
			if (preset) {
				boost::json::object presetObj;
				presetObj["type"] = static_cast<int>(preset->type());
				presetObj["id"] = preset->id();
				presetsArray.push_back(std::move(presetObj));
			}
		}
		actorObj["presets"] = std::move(presetsArray);
		actorsArray.push_back(std::move(actorObj));
	}

	stream << boost::json::serialize(actorsArray);
	return stream;
}

concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& operator<<(concurrent_unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets, std::istringstream& stream) {
	// Парсим строку как boost::json::array
	boost::json::value jv = boost::json::parse(stream.str());
	if (!jv.is_array()) {
		return presets;
	}

	PresetsManager& pmanager = PresetsManager::get();
	auto& arr = jv.as_array();

	for (const auto& actorVal : arr) {
		if (!actorVal.is_object()) continue;
		const auto& actorObj = actorVal.as_object();

		// Получаем formid
		auto formidIt = actorObj.find("formid");
		if (formidIt == actorObj.end() || !formidIt->value().is_int64()) continue;
		uint32_t formid = static_cast<uint32_t>(formidIt->value().as_int64());
		// FormID записан в порядке плагинов сохранения: пересчитываем штатным ResolveFormID. Без этого
		// после смены набора плагинов запись указывала бы на чужого актёра. 0 - плагина больше нет.
		formid = ResolveSavedFormID(formid);
		if (formid == 0) continue;

		// Получаем массив пресетов
		auto presetsIt = actorObj.find("presets");
		if (presetsIt == actorObj.end() || !presetsIt->value().is_array()) continue;
		const auto& presetsArray = presetsIt->value().as_array();

		ActorsManagerDefs::Presets actorPresets;
		for (const auto& presetVal : presetsArray) {
			if (!presetVal.is_object()) continue;
			const auto& presetObj = presetVal.as_object();

			// Получаем id и type
			auto idIt = presetObj.find("id");
			auto typeIt = presetObj.find("type");
			if (idIt == presetObj.end() || !idIt->value().is_string()) continue;
			if (typeIt == presetObj.end() || !typeIt->value().is_int64()) continue;

			std::string id = idIt->value().as_string().c_str();
			int type = static_cast<int>(typeIt->value().as_int64());

			std::shared_ptr<Preset> preset = pmanager.getPreset(id);
			if (preset && static_cast<int>(preset->type()) == type) {
				actorPresets.emplace(preset);
			}
		}

		if (!actorPresets.empty()) {
			presets[formid] = std::move(actorPresets);
		}
	}

	return presets;
}

void ActorsManager::notifyGameStartLoading() noexcept {
	{ std::lock_guard lock(ActorsManager::get().m_recentlyLoadedActorsMtx); ActorsManager::get().m_recentlyLoadedActors.clear(); }
	// Сбрасываем очередь сброса 3D: таски для прошлой локации не нужны.
	ActorResetQueue::GetSingleton().Clear();
	ActorsManager::get().m_deserialized.store(false, std::memory_order_release);
	m_gameInLoadingState.store(true, std::memory_order_release);
	logger::info("ActorsManager: Game started loading.");
}

void ActorsManager::notifyGameEndLoading() noexcept {
	m_gameInLoadingState.store(false, std::memory_order_release);
	logger::info("ActorsManager: Game finished loading.");
	
	// Проверяем готовность системы после окончания загрузки
	if (isReady()) {
		ActorsManager::get().flushWaitingActors(true);
	}
}

void ActorsManager::notifyNewGameStart() noexcept {
	// Новая игра в той же сессии: карта актёров и оверрайды прошлой игры не должны остаться.
	ActorsManager::get().clear();
	SessionOverrides::get().clear();
	NPCPreset::clearNpcMap();
	ActorsManager::get().m_deserialized.store(true, std::memory_order_release);
	{ std::lock_guard lock(ActorsManager::get().m_recentlyLoadedActorsMtx); ActorsManager::get().m_recentlyLoadedActors.clear(); }
	m_gameInLoadingState.store(true, std::memory_order_release);
	logger::info("ActorsManager: New game started.");
}

size_t ActorsManager::waitingActorsSize() const noexcept {
	m_waitingActors.size();
}

void ActorsManager::flushWaitingActors(bool reloadClosestActors) noexcept {
	if (isReady() && !m_waitingActors.empty()) {
		logger::info("ActorsManager: Flushing {} waiting actors.{}", m_waitingActors.size(), reloadClosestActors ? " Reloading closest." : "");
		m_waitingActors.flush();
	}
}

void ActorsManager::handleActorLoading(RE::Actor* actor) {
	if (!actor) return;

	// Копия-предпросмотр (и всё, что грузится, пока она создаётся) - не наш подопечный.
	if (s_previewSpawning.load() || isPreviewCopy(actor->formID)) return;

	// find+insert должны быть атомарными: TESObjectLoadedEvent и TESInitScriptEvent
	// могут прийти для одного актёра почти одновременно. Без мьютекса оба проходят
	// find()==end() до того как первый успевает вставить — и оба запускают processingLogic,
	// что даёт два конкурирующих PresetsGenerator на одну TESNPC.
	{
		std::lock_guard lock(m_recentlyLoadedActorsMtx);
		if (m_recentlyLoadedActors.find(actor->formID) != m_recentlyLoadedActors.end()) {
			logger::info("ActorsManager::handleActorLoading: Actor 0x{:x} already loaded, skipping", actor->GetFormID());
			return;
		}
		m_recentlyLoadedActors.insert(actor->GetFormID());
	}

	processingLogic(actor);
	scheduleReequipIfNeeded(actor);
}

void ActorsManager::scheduleReequipIfNeeded(RE::Actor* actor) {
	if (!actor) return;

	auto time = globals::g_ini->at("PATCH/iReequipClothesWithExtraData", -1);
	if (time < 0) return;

	const auto formID = actor->GetFormID(); // only id passed between threads

	// Дебаунс: при большом всплеске загрузки актёров (например, десятки NPC сразу)
	// каждый handleActorLoading() порождал СВОЙ поток-таймер на formID — без ограничения
	// их число росло неограниченно (наблюдалось до ~600 за 60с в стресс-тесте), что через
	// iReequipClothesWithExtraData секунд обрушивает волной AddTask-задач на игровой поток
	// одновременно — выглядит как зависание. Не связано с общим faceNPC-шаблоном (схема 2),
	// это отдельная проблема масштабирования именно этого механизма re-equip.
	// Если для formID уже запланирован отложенный re-equip — не плодим второй.
	static std::mutex s_pendingMtx;
	static std::unordered_set<std::uint32_t> s_pending;
	{
		std::lock_guard lock(s_pendingMtx);
		if (!s_pending.insert(formID).second) {
			return; // уже запланировано для этого актёра
		}
	}

	std::thread([formID, time]() {
		if (time > 0) {
			std::this_thread::sleep_for(std::chrono::seconds(time));
		}

		{
			std::lock_guard lock(s_pendingMtx);
			s_pending.erase(formID);
		}

		auto taskInterface = F4SE::GetTaskInterface();
		taskInterface->AddTask([formID]() {
			// resolve actor on main/game thread
			auto form = RE::TESForm::GetFormByID(formID);
			auto resolvedActor = RE::fallout_cast<RE::Actor*>(form);
			if (!resolvedActor) return;

			// final safety checks on main thread
			if (ActorIsEnabled(resolvedActor) == false) return;

			functions::DoReEquip(RE::NiPointer<RE::Actor>(resolvedActor), RE::BIPED_OBJECT::kBODY_33, 0,
				[formID](RE::NiPointer<RE::NiAVObject> p_niobj) -> bool {
					auto a = RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(formID));
					if (p_niobj) {
						bool foundClothData = false;
						p_niobj->Visit([&foundClothData, a](RE::NiAVObject* a_obj) -> bool {
							if (ActorIsEnabled(a) == false) {
								return false;
							}
							auto process = a->currentProcess;
							if (!process) {
								return false;
							}
							auto extraDataCache = a->currentProcess->GetClothExtraDataCache();
							if (extraDataCache) {
								foundClothData = true;
								return false;
							}
							return true; // Продолжаем обход
							});
						return foundClothData;
					}
					return false;
				}
			);
		});
	}).detach();
}