#include <Windows.h>
#include "ActorsManager.h"
#include "Version.h"
#include <string>
#include <sstream>
#include <zlib.h>
#include <atomic>

namespace globals {
	extern RE::BGSListForm* flst_qualifying_race;
	extern RE::BGSListForm* flst_excluded_npc;
	extern RE::BGSKeyword* kwd_excluded;
}

namespace functions {
	extern RE::TESNPC* getFaceTESNPC(RE::TESNPC* npc);
}

using namespace functions;
using namespace globals;

boost::json::value to_json(const ActorsManager::ActorPreset& actorPresets);
std::ostream& operator<<(std::ostream& os, const ActorsManager::ActorPreset& actorPresets);

ActorsManager& ActorsManager::get()
{
    static ActorsManager instance;
    return instance;
}

ActorsManager::ActorsManager() :
	m_waitingActors(&m_presets, [this](const ActorPreset& preset) { return this->applyActorPresets(preset); })
{
	
	if (!m_messaging) {
		logger::error("ActorsManager: Failed to get F4SE messaging interface.");
		return;
	}

	if (!m_messaging->RegisterListener(messageHandler)) {
		logger::error("ActorsManager: Failed to register F4SE messaging listener.");
		return;
	}
	
	TESObjectREFR_Events::RegisterForObjectLoaded(this);
	// Если менеджер пресетов загружен и валидирован - можно работать, если нет - подписываемся на ожидание сигнала окончания валидации.
	// Имея валидированный набор пресетов можно заполнять m_presets
	if (m_pmanager.isReady() && !m_waitingActors.empty()) {
		m_waitingActors.flush();
	}
	else {
		m_pmanager.subscribeOnValidated([this]() { m_waitingActors.flush(); }, true);
	}
}

void ActorsManager::clear() noexcept {
	m_presets.clear();
}

void ActorsManager::deserialize() {
	deserializing.store(true, std::memory_order_relaxed);
	clear(); // Очищаем текущие данные
	uint32_t fullSize{};

	auto GetNextRecord = [this, &fullSize]() {
		uint32_t recType{}, length{}, curVersion{};
		if (!m_serialization->GetNextRecordInfo(recType, curVersion, length))
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

	if (!m_serialization && m_serialization->OpenRecord(Version::UID, Version::SerializationVer)) {
		logger::critical("ActorsManager: Failed to open record for deserialization. Serialization interface is not available.");
		deserializing.store(false, std::memory_order_relaxed);
		return;
	}

	auto length = GetNextRecord();

	if (!length) {
		deserializing.store(false, std::memory_order_relaxed);
		return;
	}

	// Выделяем буфер для чтения данных
	std::vector<Bytef> compressedData(length);
	auto read = m_serialization->ReadRecordData(compressedData.data(), length);
	if (read != length) {
		logger::warn("Failed to read the record data");
		deserializing.store(false, std::memory_order_relaxed);
		return;
	}

	// Извлекаем fullSize из конца compressedData
	if (compressedData.size() < sizeof(fullSize)) {
		logger::error("Compressed data is too small to contain fullSize.");
		deserializing.store(false, std::memory_order_relaxed);
		return;
	}

	std::memcpy(&fullSize, compressedData.data() + compressedData.size() - sizeof(fullSize), sizeof(fullSize));
	compressedData.resize(compressedData.size() - sizeof(fullSize));  // Уменьшаем размер на sizeof(fullSize)

	// Определяем размер для разжатых данных
	uLongf decompressedSize = fullSize;
	std::string decompressedData(decompressedSize, '\0');

	// Разжимаем данные
	if (uncompress(reinterpret_cast<Bytef*>(&decompressedData[0]), &decompressedSize, compressedData.data(), compressedData.size()) != Z_OK) {
		logger::error("Failed to decompress data.");
		deserializing.store(false, std::memory_order_relaxed);
		return;
	}

	/* Обрабатываем разжатые данные как JSON, должны получить массив объектов вида :
	{
		"formID": 12345678,
		"preset" : [ "name0", "name1", "name2" ]
	}
	*/
	deserializing.store(false, std::memory_order_relaxed);
}

void ActorsManager::serialize() {
	if (m_serialization && m_serialization->OpenRecord(Version::UID, Version::SerializationVer)) {
		// Create a string to hold the serialized data
		std::string serializedData;
		uint32_t fullSize{};

		// Use a stream to serialize actors
		std::ostringstream ostream;
		ostream << m_presets;
		
		serializedData = ostream.str();
		fullSize = serializedData.size();

		// Определяем размер сжатых данных
		uLongf compressedSize = compressBound(serializedData.size());
		std::vector<Bytef> compressedData(compressedSize);

		// Сжимаем данные
		if (compress(compressedData.data(), &compressedSize, reinterpret_cast<const Bytef*>(serializedData.data()), serializedData.size()) != Z_OK) {
			logger::error("Failed to compress serialized data.");
			return;
		}

		compressedData.resize(compressedData.size() + sizeof(fullSize));
		std::memcpy(compressedData.data() + compressedData.size() - sizeof(fullSize), &fullSize, sizeof(fullSize));  // Копируем fullSize в конец

		// Записываем сжатые данные в интерфейс
		if (!m_serialization->WriteRecordData(compressedData.data(), compressedData.size())) {
			logger::warn("Failed to write all compressed data to the interface.");
		}
	} else {
		logger::critical("ActorsManager: Failed to open record for serialization. Serialization interface is not available.");
	}
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

void ActorsManager::messageHandler(F4SE::MessagingInterface::Message* a_msg) {
	if (!a_msg) {
		return;
	}
	switch (a_msg->type) {
		case F4SE::MessagingInterface::kPreLoadGame:
			ActorsManager::get().clear(); // Очищаем текущие данные перед загрузкой игры
			ActorsManager::get().deserialize();
			ActorsManager::get().m_waitingActors.flush();
			break;
		case F4SE::MessagingInterface::kPreSaveGame:
			ActorsManager::get().serialize();
			break;
	}
}

bool ActorsManager::applyActorPresets(const ActorPreset& actorPresets) {
	
	auto actor = RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(actorPresets.first));
	if (!actor) {
		return false;
	}

	if (actorPresets.second.empty()) {
		return false;
	}

	if (!isQualifiedRace(actor) || isExcluded(actor)) {
		return false;
	}

	bool atLeastOneApplied = false;

	for (const auto& preset : actorPresets.second) {
		if (!preset) {
			continue;
		}
		if (preset->apply(actor)) {
			atLeastOneApplied = true;
		}
	}

	return atLeastOneApplied;
}

bool ActorsManager::isQualifiedRace(RE::Actor* actor) const noexcept {
	if (!actor || !actor->race || !flst_qualifying_race)
		return false;

	return (flst_qualifying_race->ContainsItem(actor->race));
}

bool ActorsManager::isExcluded(RE::Actor* actor) const noexcept
{
	if (!actor || !kwd_excluded)
		return false;

	if (kwd_excluded && actor->HasKeyword(kwd_excluded))
		return true;

	auto base = actor->GetTemplateActorBase();
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

RE::BSEventNotifyControl ActorsManager::ProcessEvent(const RE::TESObjectLoadedEvent& a_event, RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source)
{
	if (a_event.loaded) {
		if (!deserializing.load()) {
			auto actor = RE::fallout_cast<RE::Actor*>(RE::TESForm::GetFormByID(a_event.formId));
			if (!isQualifiedRace(actor)) {
				return RE::BSEventNotifyControl::kContinue;
			}
			m_presets.find(a_event.formId);
			if (auto it = m_presets.find(a_event.formId); it == m_presets.end()) {
				applyActorPresets(*it);
			}
			else {
				// Presets presets = генератор необходимости применения пресетов и применение, если нужно.
				// Если генератор вернул true - запускается асинхронно без необходимости контроля потока и результата выполнения.
				// applyActorPreset()
				// m_presets.emplace(a_event.formId, Presets{});
			}
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}

boost::json::value to_json(const ActorsManager::ActorPreset& actorPreset) {
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

std::ostream& operator<<(std::ostream& os, const ActorsManager::ActorPreset& actorPreset) {
	os << boost::json::serialize(to_json(actorPreset));
	return os;
}

boost::json::value to_json(const std::unordered_map<uint32_t, ActorsManager::Presets>& presets) {
	boost::json::array arr;
	for (const auto& [formID, presetSet] : presets) {
		arr.push_back(to_json(ActorsManager::ActorPreset{ formID, presetSet }));
	}
	return arr;
}

std::ostream& operator<<(std::ostream& os, const std::unordered_map<uint32_t, ActorsManager::Presets>& presets) {
	os << boost::json::serialize(to_json(presets));
	return os;
}

std::istream& operator>>(std::istream& is, ActorsManager::ActorPreset& actorPreset) {
	boost::json::value jv;
	is >> jv;
	if (jv.is_object()) {
		auto obj = jv.as_object();
		auto first = static_cast<uint32_t>(obj["formID"].as_uint64());
		if (obj.contains("preset") && obj["preset"].is_array()) {
			for (const auto& item : obj["preset"].as_array()) {
				if (item.is_string()) {
					auto preset = PresetsManager::get()[item.as_string().c_str()];
					if (!preset) {
						logger::error("Preset with ID '{}' not found.", item.as_string().c_str());
						continue;
					}
					actorPreset.second.emplace(preset);
				}
			}
		}
	}
	return is;
}