#include "Bodymorphs.h"
#include "PugiXML/pugixml.hpp"
#include "LooksMenu/LooksMenuInterfaces.h"
#include "Ini/ini.h"
#include "globals.h"


BodymorphsPreset::BodymorphsPreset() :
	Preset(std::string{}) {}

BodymorphsPreset::BodymorphsPreset(const std::filesystem::path& path) :
	Preset(path.stem().string())
{
	if (!loadFromFile(path.string())) {
		logger::error("Failed to load BodymorphsPreset from path: {}", path.string());
	};
}

BodymorphsPreset::BodymorphsPreset(const std::string& path) :
	Preset(path)
{
	if (!loadFromFile(path)) {
		logger::error("Failed to load BodymorphsPreset from path: {}", path);
	};
}

bool BodymorphsPreset::operator==(const Preset& other) const noexcept
{
	if (auto bp_other = preset_cast<const BodymorphsPreset*>(&other); bp_other) {
		return *this == *bp_other;
	} else {
		return false;
	}
}

bool BodymorphsPreset::operator==(const BodymorphsPreset& other) const noexcept
{
	return Preset::operator==(other);
}

PresetType BodymorphsPreset::type() const noexcept { 
	return PRESET_TYPE; 
}

// @brief Упорядочивает пресеты по типу, затем по полу, затем по имени. Для сортировки в списке пресетов и удобного поиска. Поддерживает сравнение с базовым классом Preset для использования в контейнерах STL.
bool BodymorphsPreset::operator<(const Preset& other) const noexcept
{
	if (auto bp_other = preset_cast<const BodymorphsPreset*>(&other); bp_other) {
		return *this < *bp_other;
	} else {
		return Preset::operator<(other);
	}
}

// @brief Упорядочивает пресеты по типу, затем по полу, затем по имени. Для сортировки в списке пресетов и удобного поиска.
bool BodymorphsPreset::operator<(const BodymorphsPreset& other) const noexcept
{
	return Preset::operator<(other);
}

// @brief Проверяет, применим ли пресет к актеру. Возвращает true, если пол актера соответствует условиям пресета и есть морфы для применения.
CoincidenceLevel BodymorphsPreset::check(const RE::Actor* actor, Filter filter) const noexcept
{
	return m_conditions.check(actor, filter);  // Проверяем базовые условия из Preset
}

// @brief Применяет пресет к актеру, устанавливая морфы тела. Перед применением удаляет ранее применённые морфы тела, чтобы избежать конфликтов.
bool BodymorphsPreset::apply(RE::Actor* actor, bool reset3d) const
{
	if (!actor) {
		logger::info("BodyMorphs Apply no actor provided!");
		return false;
	}

	if (actor = RE::fallout_cast<RE::Actor*>(actor); !actor) {
		logger::info("BodyMorphs Apply no actor provided!");
		return false;
	}

	auto npc = actor->GetNPC();
	if (!npc) {
		logger::info("BodyMorphs Apply no NPC provided for actor: {:#x}", actor->formID);
		return false;
	}

	static std::unordered_set<RE::Actor*> processingActors{};
	if (processingActors.contains(actor)) {
		return false; // Предотвращаем повторную обработку одного и того же актёра
	}
	processingActors.insert(actor); // Добавляем актёра в список обрабатываемых
	
	// здесь не нужна проверка, т.к. иначе не будут работать пресеты применяемые вручную. Можно только пол проверять.
	//if (check(actor) == CoincidenceLevel::NONE) {
	//	logger::info("BodyMorphs Apply check failed for actor: {:#x}", actor->formID);
	//	return false;
	//}
	if (check(actor, Filter{ Filter::Gender }) == CoincidenceLevel::NONE) {
		logger::info("BodyMorphs Apply gender check failed for actor: {:#x}", actor->formID);
		processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
		return false;
	}

	remove(actor);

	if (!actor->GetFullyLoaded3D()) {
		processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
		return false;
	}

	for (const auto& [morphName, morphValue] : m_morphs) {
		auto Interface = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
		if (!Interface) {
			logger::critical("BodyMorphInterface is nullptr!");
			processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
			return false;
		}
		Interface->SetMorph(actor, actor->GetSex() == RE::Actor::Sex::Female, morphName, globals::kwd_diversed, morphValue);
	}
	
	{
		static std::unordered_map<uint32_t, std::mutex> npc_mutexes;
		std::mutex& mtx = npc_mutexes[npc->formID];
		std::lock_guard<std::mutex> lock(mtx);

		if (npc->morphWeight != m_morphWeight) {
			npc->morphWeight = m_morphWeight;
		}

		if (!actor->GetFullyLoaded3D()) {
			processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
			return true;
		}

		// Вызывает T-позу пока отключу.
		// Сброс 3D модели актера
		/*using R3D = RE::RESET_3D_FLAGS;
		if (reset3d && actor->GetFullyLoaded3D()) {
			actor->Reset3D(false, R3D::kModel | R3D::kSkeleton, true, R3D::kNone);
		}*/

		npc_mutexes.erase(npc->formID); // Удаляем мьютекс для этого NPC, чтобы избежать утечек памяти
	}

	processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
	return true;
}

// @brief Удаляет морфы тела у актера, если они были применены ранее. Используется для очистки морфов перед применением нового пресета. Убирает только морфы этого мода.
bool BodymorphsPreset::remove(RE::Actor* actor) const
{
	if (!actor) {
		logger::info("BodyMorphs Remove no actor provided!");
		return false;
	}

	static std::unordered_set<RE::Actor*> processingActors{};
	if (processingActors.contains(actor)) {
		return false; // Предотвращаем повторную обработку одного и того же актёра
	}
	processingActors.insert(actor); // Добавляем актёра в список обрабатываемых

	auto Interface = LooksMenuInterfaces<BodyMorphInterface>::GetInterface();
	if (!Interface) {
		logger::critical("BodyMorphInterface is nullptr!");
		processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
		return false;
	}

	Interface->RemoveMorphsByKeyword(actor, actor->GetSex() == RE::Actor::Sex::Female, globals::kwd_diversed);

	processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
	return true;
}

// @brief Возвращает true, если пресет пустой или не валидный. Пустой пресет - это пресет без имени.
bool BodymorphsPreset::empty() const noexcept
{
	return type() <= PresetType::NONE || type() >= PresetType::END || id().empty();
}

std::future<bool> BodymorphsPreset::isValidAsync() const noexcept {
	// Копируем необходимые данные для потокобезопасности
	bool isEmpty = empty();
	bool hasMorphs = !m_morphs.empty();
	bool hasConds = !m_conditions.empty();

	// Возвращаем future, который выполнит проверку асинхронно
	return std::async(std::launch::async, [isEmpty, hasMorphs, hasConds]() {
		return !isEmpty && hasMorphs && hasConds;
	});
}

// @brief загружает пресет из файла. Обнуляет ранее загруженные данные, чтобы избежать конфликтов. 
// Даже если файл не был загружен, обнуляет старые данные.
bool BodymorphsPreset::loadFromFile(const std::string& presetFile)
{
	BodymorphsPreset::clear();  // Очищаем предыдущие данные, чтобы избежать конфликтов
	if (presetFile.ends_with("_conds.json")) {
		// Если файл - это условия, то не загружаем его как пресет
		return false;
	}

	auto path = std::filesystem::path(presetFile);
	if (path.filename().string() == "conds.json") {
		// Если файл - это условия, то не загружаем его как пресет
		return false;
	}

	if (!std::filesystem::exists(path)) {
		logger::error("BodyMorphs preset file does not exist: {}", presetFile);
		return false;
	}

	auto getConditions = [&, this](ParseLMPreset* preset = nullptr) {
		// Первым делом проверяем есть ли условия, т.к. если их нет, то и загружать дальше нет смысла
		auto conditions_path = path.parent_path() / (path.stem().string() + "_conds.json");
		if (std::filesystem::exists(conditions_path)) {
			LoadConditions(conditions_path);
		}

		if (preset && m_conditions.empty()) {
			m_conditions = std::move(preset->conditions());  // Загружаем условия из основного файла, если нет отдельного файла с условиями
		}

		if (m_conditions.empty()) {
			conditions_path = path.parent_path() / "conds.json"; // Пробуем загрузить условия из файла "conds.json" в той же папке
			if (std::filesystem::exists(conditions_path)) {
				LoadConditions(conditions_path);
			}
		}
	};

	if (path.extension() == ".json") {
		ParseLMPreset preset(path);
		if (!preset.isLoaded() || preset.isEmpty()) {
			logger::error("BodymorphsPreset::loadFromFile: Failed to load preset from file: {}", presetFile);
			BodymorphsPreset::clear();
			return false;
		}

		getConditions(&preset); // Загружаем условия из пресета

		if (m_conditions.empty()) {
			logger::warn("BodymorphsPreset::loadFromFile: No conditions found in preset file: {}", presetFile);
			BodymorphsPreset::clear();  // Очищаем данные, если пресет не валиден
			return false;
		}
			
		if (auto sex = preset.gender(); sex != RE::Actor::None) {
			m_conditions.setGender(sex); // Устанавливаем пол из пресета, т.к. пресет не может быть применён к актёру другого пола.
		}

		m_morphs = preset.bodyMorphs();
		m_morphWeight = preset.morphWeight();
		
	} else if (path.extension() == ".xml") {

		getConditions(); // Загружаем условия из пресета

		if (m_conditions.empty()) {
			logger::warn("BodymorphsPreset::loadFromFile: No conditions found in preset file: {}", presetFile);
			BodymorphsPreset::clear();  // Очищаем данные, если пресет не валиден
			return false;
		}

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(path.string().c_str());

		if (!result) {
			logger::info("...FAILED : {} wrong xml format", presetFile);
			return false;
		}

		pugi::xml_node presetNode = doc.child("SliderPresets").child("Preset");
		if (presetNode) {
			for (pugi::xml_node sliderNode = presetNode.child("SetSlider"); sliderNode; sliderNode = sliderNode.next_sibling("SetSlider")) {
				auto nameAttr = sliderNode.attribute("name");
				auto valueAttr = sliderNode.attribute("value");
				if (!nameAttr || !valueAttr) {
					logger::info("...FAILED : missing name or value attribute in slider");
					continue;
				}
				std::string key = nameAttr.value();
				float value = valueAttr.as_float() / 100.0f;
				m_morphs.emplace(std::move(key), value);
			}
		}
	}

	if (m_morphs.empty() || m_conditions.empty()) {
		logger::warn("BodymorphsPreset::loadFromFile: Preset is empty or conditions are missing in file: {}", presetFile);
		BodymorphsPreset::clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	m_id = path.stem().string();  // Устанавливаем имя пресета из имени файла
	logger::info("BodymorphsPreset::loadFromFile: {}", id());
	return true;
}

// @breif Очищает объект
void BodymorphsPreset::clear() noexcept {
	m_morphs.clear();
	Preset::clear();
}

// DiverseBodiesRedux\Sources\Preset\Bodymorphs.cpp
#include <sstream>

std::string BodymorphsPreset::print() const
{
	std::ostringstream oss;
	oss << "BodymorphsPreset: " << id() << "\n";
	oss << "  Type: " << GetPresetTypeString(type()) << "\n";
	oss << "  Morphs: ";
	if (m_morphs.empty()) {
		oss << "(none)\n";
	}
	else {
		oss << "\n";
		for (const auto& [name, value] : m_morphs) {
			oss << "    " << name << " = " << value << "\n";
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