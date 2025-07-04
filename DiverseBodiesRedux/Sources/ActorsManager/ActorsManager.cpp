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
#include "Details/PresetsGenerator.hpp"
#include "globals.h"

using namespace functions;
using namespace globals;

// --- JSON сериализация/десериализация пресетов ---
boost::json::value to_json(const ActorsManagerDefs::ActorPreset& actorPresets);
std::ostringstream& operator<<(std::ostringstream& os, const ActorsManagerDefs::ActorPreset& actorPresets);
ActorsManagerDefs::ActorPreset fromJson(const boost::json::object& obj);
ActorsManagerDefs::ActorPreset& operator<<(ActorsManagerDefs::ActorPreset& actorPreset, std::istringstream& is);
std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& operator<<(std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets, std::istringstream& is);
std::ostringstream& operator<<(std::ostringstream& os, const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets);


namespace TESObjectREFR_Events {
	// Регистрация для события загрузки объекта
	void RegisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* sink);
	// Отмена регистрации для события загрузки объекта
	void UnregisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* sink);
}

ActorsManager& ActorsManager::get()
{
	static ActorsManager instance;
	return instance;
}

ActorsManager::ActorsManager() :
	m_waitingActors(&m_presets, [this](const ActorsManagerDefs::ActorPreset& preset) { return this->applyActorPresets(preset); }),
	m_directApply(
		&m_presets, 
		&m_mutex, 
		[this](RE::Actor* actor, const std::shared_ptr<Preset>& preset) { return this->applyActorPreset(actor, preset); },
		[this](RE::Actor* actor, const std::shared_ptr<Preset>& preset) { return this->updateActorPresetInStorage(actor, preset); }
	)
{
	F4SE::GetSerializationInterface()->SetSaveCallback(serializationCallback);
	F4SE::GetSerializationInterface()->SetLoadCallback(deserializationCallback);

	TESObjectREFR_Events::RegisterForObjectLoaded(this);

	// Подписка на событие завершения десериализации
	subscribeOnDeserialized([this]() {
		if (isReady() && !m_waitingActors.empty()) {
			m_waitingActors.flush();
		}
		}, false);

	// Если PresetsManager уже готов, flush сразу после десериализации
	if (m_pmanager.isReady()) {
		if (!m_waitingActors.empty() && m_deserialized.load())
			m_waitingActors.flush();
	}
	else {
		// Если PresetsManager не готов, flush произойдёт после его валидации
		m_pmanager.subscribeOnValidated([this]() {
			if (m_deserialized.load() && !m_waitingActors.empty())
				m_waitingActors.flush();
			}, true);
	}
}

bool ActorsManager::containsActor(const RE::Actor* actor) const noexcept {
	if (actor)
		return m_presets.contains(actor->formID);
	return false;
}

bool ActorsManager::containsActor(uint32_t formId) const noexcept {
	return m_presets.contains(formId);
}

bool ActorsManager::isReady() {
	return m_pmanager.isReady() && m_deserialized.load(std::memory_order_relaxed);
}

RE::BSEventNotifyControl ActorsManager::ProcessEvent(const RE::TESObjectLoadedEvent& a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* /*a_source*/)
{
	// Обработка события загрузки объекта (актеров)
	if (a_event.loaded) {
		if (isReady()) {
			auto actor = checkActorByFormId(a_event.formId);
			if (!actor || isExcluded(actor)) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (auto it = m_presets.find(a_event.formId); it != m_presets.end()) {
				applyActorPresets(*it);
			}
			else {
				auto preset = PresetsGenerator{ actor }.getRandomPresets();
				if (!preset.second.empty() && applyActorPresets(preset)) {
					updateActorPresetsInStorage(preset);
				}
			}
		}
		else {
			auto actor = checkActorByFormId(a_event.formId);
			if (actor) {
				m_waitingActors.push_back(a_event.formId);
			}
		}
	}
	return RE::BSEventNotifyControl::kContinue;
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
	m_presets.clear();
}

void ActorsManager::deserialize(const F4SE::SerializationInterface* a_intrfc) {
	// --- Десериализация пресетов из co-save ---
	// Проверяем готовность PresetsManager
	if (!PresetsManager::get().isReady()) {
		logger::error("PresetsManager is not ready, skipping deserialization.");
		return;
	}

	m_deserialized.store(false, std::memory_order_relaxed);
	clear(); // Очищаем текущие данные
	uint32_t fullSize{};

	// Получаем информацию о следующей записи сериализации
	auto GetNextRecord = [this, a_intrfc, &fullSize]() {
		uint32_t recType{}, length{}, curVersion{};
		if (!a_intrfc->GetNextRecordInfo(recType, curVersion, length))
			return 0u;

		if (recType != Version::UID || curVersion != Version::SerializationVer) {
			logger::critical("Invalid record type or version: expected {}, got {} with version {}", Version::UID, recType, curVersion);
			return 0u;
		}

		if (length > INT_MAX) {
			logger::info("buf_size is more than {}", std::to_string(INT_MAX));
			return 0u;
		}

		if (length <= sizeof(fullSize)) {
			logger::info("No serialized data.");
			return 0u;
		}

		return length;
		};

	// Проверяем доступность интерфейса сериализации
	if (!a_intrfc && a_intrfc->OpenRecord(Version::UID, Version::SerializationVer)) {
		logger::critical("ActorsManager: Failed to open record for deserialization. Serialization interface is not available.");
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	auto length = GetNextRecord();

	if (!length) {
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	// Чтение сжатых данных
	std::vector<Bytef> compressedData(length);
	auto read = a_intrfc->ReadRecordData(compressedData.data(), length);
	if (read != length) {
		logger::warn("Failed to read the record data");
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	// Извлекаем размер разжатых данных
	if (compressedData.size() < sizeof(fullSize)) {
		logger::error("Compressed data is too small to contain fullSize.");
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	std::memcpy(&fullSize, compressedData.data() + compressedData.size() - sizeof(fullSize), sizeof(fullSize));
	compressedData.resize(compressedData.size() - sizeof(fullSize));

	// Разжимаем данные
	uLongf decompressedSize = static_cast<uLongf>(fullSize);
	std::string decompressedData(decompressedSize, '\0');
	if (uncompress(reinterpret_cast<Bytef*>(&decompressedData[0]), &decompressedSize, compressedData.data(), static_cast<uLong>(compressedData.size())) != Z_OK) {
		logger::error("Failed to decompress data.");
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	if (decompressedSize != fullSize) {
		logger::error("Decompressed data size does not match expected fullSize.");
		m_deserialized.store(true, std::memory_order_relaxed);
		fireDeserialized();
		return;
	}

	// Десериализация JSON в m_presets
	{
		std::lock_guard lock(m_mutex);
		std::istringstream iss(std::move(decompressedData));
		m_presets << iss;
	}

	m_deserialized.store(true, std::memory_order_relaxed);
	fireDeserialized();
}

void ActorsManager::serialize(const F4SE::SerializationInterface* a_intrfc) {
	// --- Сериализация пресетов в co-save ---
	if (a_intrfc && a_intrfc->OpenRecord(Version::UID, Version::SerializationVer)) {
		// Сериализуем m_presets в строку
		std::string serializedData;
		uint32_t fullSize{};

		std::ostringstream ostream;
		ostream << m_presets;
		serializedData = ostream.str();
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

bool ActorsManager::applyActorPreset(RE::Actor* actor, const std::shared_ptr<Preset>& preset) {
	auto veryfied_actor = RE::fallout_cast<const RE::Actor*>(actor);
	if (!veryfied_actor || !preset || preset->empty()) {
		return false;
	}
	return preset->apply(actor);
}

bool ActorsManager::updateActorPresetInStorage(RE::Actor* actor, const std::shared_ptr<Preset>& preset) noexcept {
	if (!actor || !preset || preset->empty()) {
		logger::error("updateActorPresetInStorage: actor or preset is null/empty.");
		return false;
	}
	std::lock_guard lock(m_mutex);
	auto it = m_presets.find(actor->formID);
	if (it != m_presets.end()) {
		// Если актёр уже есть, делаем swap
		for (auto& existingPreset : it->second) {
			if (existingPreset && existingPreset->type() == preset->type()) {
				*existingPreset = *preset;
				return true;
			}
		}
		it->second.emplace(preset);
		return true;
	}
	else {
		// Если актёр не найден, добавляем новый
		ActorsManagerDefs::ActorPreset actorPresets{ actor->formID, {preset} };
		m_presets.emplace(actorPresets.first, std::move(actorPresets.second));
		return true;
	}
}

bool ActorsManager::applyActorPresets(const ActorsManagerDefs::ActorPreset& actorPresets) {
	auto actor = RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(actorPresets.first));
	if (!actor || actorPresets.second.empty() || !isQualifiedRace(actor) || isExcluded(actor)) {
		return false;
	}
	bool atLeastOneApplied = false;
	for (const auto& preset : actorPresets.second) {
		if (preset && preset->apply(actor)) {
			atLeastOneApplied = true;
		}
	}
	return atLeastOneApplied;
}

bool ActorsManager::updateActorPresetsInStorage(const ActorsManagerDefs::ActorPreset& actorPresets) noexcept {
	std::lock_guard lock(m_mutex);
	auto it = m_presets.find(actorPresets.first);
	if (it != m_presets.end()) {
		// Если актёр уже есть, делаем swap
		it->second = (actorPresets.second);
		return true;
	} else {
		// Если актёр не найден, добавляем новый
		m_presets.emplace(actorPresets.first, std::move(actorPresets.second));
		return true;
	}
}

bool ActorsManager::isQualifiedRace(const RE::Actor* actor) const noexcept {
	if (!actor || !actor->race || !flst_qualifying_race)
		return false;
	return (flst_qualifying_race->ContainsItem(actor->race));
}

bool ActorsManager::isExcluded(const RE::Actor* actor) const noexcept {
	if (!actor || !kwd_excluded)
		return false;
	if (kwd_excluded && actor->HasKeyword(kwd_excluded))
		return true;
	auto base = const_cast<RE::Actor*>(actor)->GetTemplateActorBase();
	if (!base)
		return true;
	auto face = getFaceTESNPC(actor->GetNPC());
	if (!face)
		return true;
	if (!flst_excluded_npc)
		return false;
	if (flst_excluded_npc->ContainsItem(actor) || flst_excluded_npc->ContainsItem(face))
		return true;
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
		REL::Relocation<func_t> func{ baseAddr + 0x436A40 };
		return func(a_sink);
	}

	void UnregisterForObjectLoaded(RE::BSTEventSink<RE::TESObjectLoadedEvent>* a_sink)
	{
		HMODULE module = GetModuleHandleA(NULL);
		uintptr_t baseAddr = reinterpret_cast<uintptr_t>(module);

		using func_t = decltype(&UnregisterForObjectLoaded);
		REL::Relocation<func_t> func{ baseAddr + 0x436A70 };
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
		presetsArray.push_back(presetPtr->id().c_str());
	}
	obj["preset"] = std::move(presetsArray);

	return obj;
}

std::ostringstream& operator<<(std::ostringstream& os, const ActorsManagerDefs::ActorPreset& actorPreset) {
	os << boost::json::serialize(to_json(actorPreset));
	return os;
}

boost::json::value to_json(const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets) {
	boost::json::array arr;
	for (const auto& [formID, presetSet] : presets) {
		arr.push_back(to_json(ActorsManagerDefs::ActorPreset{ formID, presetSet }));
	}
	return arr;
}

std::ostringstream& operator<<(std::ostringstream& os, const std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets) {
	os << boost::json::serialize(to_json(presets));
	return os;
}

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

std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& operator<<(std::unordered_map<uint32_t, ActorsManagerDefs::Presets>& presets, std::istringstream& is) {
	boost::json::value jv;
	is >> jv;
	if (jv.is_array()) {
		auto arr = jv.as_array();
		for (const auto& item : arr) {
			if (item.is_object()) {
				auto actorPreset = fromJson(item.as_object());
				if (actorPreset.first != 0 && !actorPreset.second.empty()) {
					// Добавляем в мапу только валидные пресеты
					presets.emplace(actorPreset);
				}
			}
		}
	}
	return presets;
}