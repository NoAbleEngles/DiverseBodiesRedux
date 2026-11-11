#include "Ini/Ini.h"
#include "OverlayPreset.h"
#include <PugiXML/pugixml.hpp>
#include <thread>
#include "Details/ProcessingGuard.hpp"
#include "ActorsManager/ActorResetQueue.h"

OverlayPreset::OverlayPreset() :
	Preset(std::string{}) {
}

OverlayPreset::OverlayPreset(const std::filesystem::path& path) :
	Preset(path.stem().string())
{
	if (!loadFromFile(path.string())) {
		logger::error("Failed to load OverlayPreset from path: {}", path.string());
	};
}

OverlayPreset::OverlayPreset(const std::string& path) :
	Preset(std::filesystem::path{ path }.stem().string().c_str())
{
	if (!loadFromFile(path)) {
		logger::error("Failed to load OverlayPreset from path: {}", path);
	};
}

OverlayPreset::~OverlayPreset() {}

bool OverlayPreset::operator==(const Preset& other) const noexcept
{
	if (this == &other) {
		return true;
	}

	if (Preset::operator==(other)) {
		if (auto otherOverlayPreset = preset_cast<OverlayPreset*>(&other); otherOverlayPreset) {
			if (m_overlays == otherOverlayPreset->m_overlays)
				return true;
		}
	}
	return false;
}

bool OverlayPreset::operator<(const Preset& other) const noexcept
{
	auto bp_other = preset_cast<const OverlayPreset*>(&other);
	if (bp_other)
		return this->m_id < bp_other->m_id;

	return Preset::operator<(other);
}

CoincidenceLevel OverlayPreset::check(const RE::Actor* actor, Filter filter) const noexcept
{
	return Preset::check(actor, filter);  // Проверяем базовые условия из Preset
}

bool OverlayPreset::apply(RE::Actor* actor, bool reset3d) const
{
	if (!actor) {
		logger::info("OverlayPreset Apply no actor provided!");
		return false;
	}

	if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
		logger::info("OverlayPreset Apply no actor provided!");
		return false;
	}

	// здесь не нужна проверка, т.к. иначе не будут работать пресеты применяемые вручную. Можно только пол проверять.
	/*if (check(actor) == CoincidenceLevel::NONE) {
		logger::info("OverlayPreset Apply check failed for actor: {:#x}", actor->formID);
		return false;
	}*/

	auto guard = std::make_shared<ProcessingGuard<std::set<uint32_t>>>(actor->GetFormID(), m_processingActors, m_processingActorsMtx);
	if (!guard->acquire_sync_for(std::chrono::seconds{})) {
		return false; // таймаут
	}

	if (check(actor, Filter{ Filter::Gender }) == CoincidenceLevel::NONE) {
		logger::info("Apply gender check failed for actor: {:#x}", actor->formID);
		return false;
	}

	if (!actor->GetFullyLoaded3D()) {
		return false;
	}

	auto Interface = LooksMenuInterfaces<OverlayInterface>::GetInterface();
	if (!Interface) {
		logger::critical("OverlayInterface is nullptr!");
		return false;
	}
	
	auto countActorOverlays = [&]() {
		uint32_t n = 0;
		Interface->ForEachOverlay(actor, actor->GetSex() == RE::Actor::Sex::Female,
			[&n](int32_t, const OverlayInterface::OverlayDataPtr&) { ++n; });
		return n;
	};
	const auto before = countActorOverlays();

	const bool removed = remove(actor);  // Удаляем все оверлеи этого типа перед применением нового пресета
	const auto afterRemove = countActorOverlays();

	for (const auto& overlay : m_overlays) {
		overlay.apply(actor);
	}
	logger::info("[Overlay] apply: actor={:#x} preset='{}' overlays in LooksMenu: before={} removed={} afterRemove={} afterAdd={} (preset has {})",
		actor->formID, id(), before, removed, afterRemove, countActorOverlays(), m_overlays.size());

	logger::info("[Overlay] apply done: actor={:#x} preset='{}' reset3d={} fullyLoaded3D={}",
		actor->formID, id(), reset3d, actor->GetFullyLoaded3D() != nullptr);

	if (reset3d && actor->GetFullyLoaded3D()) {
		// Вместо синхронного actor->Reset3D() — планируем через ActorResetQueue.
		// excludeFlags=kHead|kFace: Overlay затрагивает только тело (татуировки, наклейки
		// на тело, волосы тела и т.д.), голову не трогаем.
		// Без этого при merge с BM-записью (excludeFlags=kHead|kFace):
		//   (kHead|kFace) & kNone = kNone → полный Reset3D включая голову →
		//   IO-гонка с BSFaceGenPendingHeadData → краш.
		//
		// needsDetach=true: AddOverlay/RemoveOverlay правят только таблицу LooksMenu, а геометрия
		// оверлеев живёт в overlay-root актёра и пересоздаётся лишь для уничтоженных слотов.
		// Reset3D(reloadAll=false) старый 3D не выбрасывает (в отличие от прежнего Reset3D(true, ...)),
		// поэтому без DestroyOverlaySlot на теле остаются прежние узлы и смена Bodyhairs/Tattoos/Nails
		// визуально не видна. Тот же DetachOverlays делает путь BODYMORPHS.
		using R3D = RE::RESET_3D_FLAGS;
		logger::info("[Overlay] Schedule reset: actor={:#x} preset='{}'", actor->formID, id());
		ActorResetQueue::GetSingleton().Schedule(actor, ActorResetQueue::ResetEntry{
			.needsBodyReset = true,
			.needsHeadReset = false,
			.needsDetach    = true,
			.needsHeadFix   = false,
			.needsOverlayUpdate = true,
		});
	}

	return true;
}

bool OverlayPreset::empty() const noexcept
{
	return type() <= PresetType::NONE || type() >= PresetType::END || id().empty();
}

void OverlayPreset::clear() noexcept {
	m_overlays.clear();
	Preset::clear();
}

std::future<bool> OverlayPreset::isValidAsync() const noexcept {
	// Если пресет пустой — сразу false
	if (empty()) {
		return std::async(std::launch::deferred, [] { return false; });
	}

	// Копируем id оверлеев для потокобезопасности
	std::vector<std::string> overlayIds;
	overlayIds.reserve(m_overlays.size());
	for (const auto& overlay : m_overlays) {
		overlayIds.push_back(overlay.id());
	}

	// Для каждого id запускаем асинхронную валидацию
	std::vector<std::future<bool>> futures;
	futures.reserve(overlayIds.size());
	for (const auto& id : overlayIds) {
		futures.push_back(validate(id));
	}

	// Возвращаем общий future, который завершится, когда все проверки завершатся
	return std::async(std::launch::async, [futures = std::move(futures), overlayIds, this]() mutable {
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

bool OverlayPreset::loadFromFile(const std::string& presetFile)
{
	// виртуальный clear может вызвать проблемы, если он вызывается в деструкторе или конструкторе.
	auto clear = [this]() {
		m_overlays.clear();
		m_id.clear();
		m_conditions.clear();
	};
	
	// Очищаем предыдущие данные
	clear();
	
	auto path = std::filesystem::path(presetFile.c_str());
	if (!std::filesystem::exists(path)) {
		logger::error("NPCPreset preset file does not exist: {}", presetFile);
		return false;
	}

	if (path.extension() != ".json") {
		logger::critical("NPCPreset::loadFromFile: Unsupported file format: {}", presetFile);
		clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	ParseLMPreset preset(path);
	if (!preset.isLoaded() || preset.isEmpty()) {
		logger::error("NPCPreset::loadFromFile: Failed to load preset from file: {}", presetFile);
		clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	// Первым делом проверяем есть ли условия, т.к. если их нет, то и загружать дальше нет смысла
	auto conditions_path = path.parent_path() / (path.stem().string() + "_conds.json");
	if (std::filesystem::exists(conditions_path)) {
		LoadConditions(conditions_path);
	}

	if (m_conditions.empty()) {
		m_conditions = std::move(preset.conditions());  // Загружаем условия из основного файла, если нет отдельного файла с условиями
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

	auto overlays = std::move(preset.overlays());
	
	if (overlays.empty()) {
		logger::warn("OverlayPreset::loadFromFile: No overlays found in preset file: {}", presetFile);
		return false;
	}

	m_overlays = std::move(std::vector<Overlay>{ overlays.begin(), overlays.end() });

	m_id = path.stem().string();

	logger::info("OverlayPreset::loadFromFile: {}. Loaded {} overlays.", presetFile, m_overlays.size());
	return true;
}

#include <sstream>

std::string OverlayPreset::print() const
{
	std::ostringstream oss;
	oss << "OverlayPreset: " << id() << "\n";
	oss << "  Type: " << GetPresetTypeString(type()) << "\n";
	oss << "  Overlays: ";
	if (m_overlays.empty()) {
		oss << "(none)\n";
	}
	else {
		oss << "\n";
		for (const auto& overlay : m_overlays) {
			oss << "    id=" << overlay.id() << "\n";
		}
	}
	oss << "  Conditions: ";
	if (m_conditions.empty()) {
		oss << "(none)\n";
	}
	else {
		oss << m_conditions.print();
	}
	return oss.str();
}