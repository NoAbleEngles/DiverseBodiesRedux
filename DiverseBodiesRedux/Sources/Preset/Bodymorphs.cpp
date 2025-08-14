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


	if (!actor->GetFullyLoaded3D()) {
		processingActors.erase(actor); // Удаляем актёра из списка обрабатываемых
		return true;
	}

	// Сброс 3D модели актера
	using R3D = RE::RESET_3D_FLAGS;
	actor->Reset3D(false, R3D::kModel | R3D::kSkeleton, true, R3D::kNone);

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
	// виртуальный clear может вызвать проблемы, если он вызывается в деструкторе или конструкторе.
	auto clear = [this]() {
		m_bodytype = BodyType::NONE;
		m_morphs.clear();
		m_id.clear();
		m_conditions.clear();
	};

	this->clear();  // Очищаем предыдущие данные, чтобы избежать конфликтов
	if (presetFile.ends_with("_conds.json")) {
		// Если файл - это условия, то не загружаем его как пресет
		return false;
	}

	auto path = std::filesystem::path(presetFile);
	if (!std::filesystem::exists(path)) {
		logger::error("BodyMorphs preset file does not exist: {}", presetFile);
		return false;
	}

	auto condsPath = path.parent_path() / (path.stem().string() + "_conds.json");
	auto externalConds = std::filesystem::exists(condsPath);
	if (externalConds) {
		try {
			m_conditions = std::move(ConditionSettings(condsPath.string()));
			logger::info("Loaded conditions from {}", condsPath.string());
		} catch (const std::exception& e) {
			logger::error("Failed to load conditions from {}: {}", condsPath.string(), e.what());
			externalConds = false;
		}
	} /*else {
		logger::info("No conditions file found for BodyMorphs preset: {}", condsPath.string());
	}*/

	auto gender = RE::Actor::Sex::None;

	if (path.extension() == ".json") {
		boost::json::object json_obj;
		try {
			json_obj = boost::json::parse(getJson(path)).as_object();
		} catch (...) {
			logger::info("...FAILED : {} wrong json format", presetFile);
			return false;
		}

		auto pValue = json_obj.if_contains("Gender");
		if (pValue && pValue->is_int64()) {
			gender = pValue->as_int64() == 0 ? RE::Actor::Sex::Male : RE::Actor::Sex::Female;
		}

		auto morphsValue = json_obj.if_contains("BodyMorphs");
		if (!morphsValue || !morphsValue->is_object()) {
			logger::info("...FAILED : no morphs found in {}", presetFile);
		} else {
			json_obj = morphsValue->as_object();
		}

		if (!json_obj.empty()) {
			int i = 0;
			std::for_each(json_obj.begin(), json_obj.end(), [&](const auto& o) {
				std::string key = o.key();
				float value;
				if (o.value().is_double()) {
					value = static_cast<float>(o.value().as_double());
				}
				else if (o.value().is_int64()) {
					value = static_cast<float>(o.value().as_int64());
				}
				else {
					logger::info("...FAILED : wrong value type {}", (o.value().is_string() ? o.value().as_string().c_str() : ""));
					return;
				}
				m_morphs.emplace(key, std::move(value));
				++i;
			});
		} else {
			logger::info("...FAILED : no morphs found in {}", presetFile);
		}
	} else if (path.extension() == ".xml") {
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

		if (m_morphs.size() && isCondtionsEmpty()) {
			if (!path.filename().empty()) {
				if (*path.filename().string().begin() == 'F')
					gender = RE::Actor::Sex::Female;
				else if (*path.filename().string().begin() == 'M')
					gender = RE::Actor::Sex::Male;
			} else {
				logger::info("...FAILED : empty filename in {}", presetFile);
			}
		}
	}

	if (!externalConds && gender != RE::Actor::Sex::None)
		m_conditions = ConditionSettings(gender);

	if (!m_conditions.empty() && m_morphs.size()) {
		if (presetFile.find("!FAT!") != std::string::npos)
			m_bodytype = BodyType::FAT;
		else if (presetFile.find("!SLIM!") != std::string::npos)
			m_bodytype = BodyType::SLIM;
		else if (presetFile.find("!SEXY!") != std::string::npos)
			m_bodytype = BodyType::SEXY;
		else if (presetFile.find("!ATHL!") != std::string::npos)
			m_bodytype = BodyType::ATHL;
		else {
			logger::info("{} no body type found, setting to SEXY", presetFile);
		}
		
	} else {
		logger::critical("BodymorphsPreset::loadFromFile: Unsupported file format: {}", presetFile);
		clear();  // Очищаем данные, если пресет не валиден
		return false;
	}

	m_id = path.stem().string();  // Устанавливаем имя пресета из имени файла
	logger::info("BodymorphsPreset::loadFromFile: {}. Loaded body type: {}", id(), static_cast<int>(m_bodytype));
	return true;
}

// @breif Очищает объект
void BodymorphsPreset::clear() noexcept {
	m_bodytype = BodyType::NONE;
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