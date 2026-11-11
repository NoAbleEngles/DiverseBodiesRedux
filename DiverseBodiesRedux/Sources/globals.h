#pragma once
#include "PCH.h"
#include "Ini/Ini.h"

namespace logger = F4SE::log;

template <class T>
inline T getPtr(uint32_t formId, const std::string_view& plugin = "")
{
	static_assert(std::is_pointer_v<T>, "T must be a pointer type");
	T ptr = nullptr;
	if (!plugin.empty())
		ptr = RE::fallout_cast<T>(RE::TESDataHandler::GetSingleton()->LookupForm(formId, plugin));
	else
		ptr = RE::fallout_cast<T>(RE::TESForm::GetFormByID(formId));
	return ptr;
}

namespace ids
{
	constexpr std::string_view plugin_name = "DiverseBodiesRedux.esp";

	//keywords
	constexpr uint32_t formid_kwd_diversed = 0x001;

	//formlist
	constexpr uint32_t formid_flst_qualifying_race = 0x15;

	//races
	constexpr uint32_t formid_race_HumanRace = 0x13746;
	constexpr uint32_t formid_race_GhoulRace = 0xEAFB6;
	constexpr uint32_t formid_race_SynthGen2Race = 0x10BD65;

}

namespace globals
{
	inline ini::map* g_ini = nullptr;

	inline RE::BGSKeyword* kwd_diversed;
	inline RE::BGSKeyword* isPowerArmorFrame;

	inline RE::BGSListForm* flst_qualifying_race;

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

		if (!g_ini) g_ini = new ini::map(iniPath.string());
		else g_ini->~map(), g_ini = new (g_ini) ini::map(iniPath.string());

		return true;
	}

	inline void initForms() {

		kwd_diversed = getPtr<RE::BGSKeyword*>(formid_kwd_diversed, plugin_name);
		logger::info("kwd_diversed: {}", kwd_diversed ? "found" : "not found");

		flst_qualifying_race = getPtr<RE::BGSListForm*>(formid_flst_qualifying_race, plugin_name);
		logger::info("flst_qualifying_race: {}", formid_flst_qualifying_race ? "found" : "not found");

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

	inline void enableRuntimeFaceChargen(RE::TESNPC* npc)
	{
		if (!npc) {
			return;
		}

		do {
			npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;

			if (npc->GetFormType() == RE::ENUM_FORM_ID::kNPC_) {
				auto hparts = npc->GetHeadParts();
				for (auto& hp : hparts) {
					if (hp && hp->chargenConditions && hp->chargenConditions.head) {
						hp->chargenConditions.head = nullptr;
					}
				}
			}

			npc = npc->faceNPC;
		} while (npc);
	}

	inline void remove_chargen_from_all_tesnpc(RE::TESNPC* npc)
	{
		if (npc) {
			npc->actorData.actorBaseFlags |= RE::TESActorBase::ACTOR_BASE_DATA_FLAGS::kFlagIsCharGenFacePreset;
			enableRuntimeFaceChargen(npc);
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

	/**
	 * @brief Получает список актёров в радиусе от указанного объекта.
	 * @param a_ref Ссылка на объект-центр поиска (по умолчанию игрок).
	 * @param a_maxDistance Максимальная дистанция поиска.
	 * @param a_maxActorsCount Максимальное количество актёров.
	 * @param a_includeDead Включать ли мёртвых актёров.
	 * @return Вектор указателей на найденных актёров.
	 */
	inline std::vector<RE::Actor*> getActorsInRange(RE::TESObjectREFR* a_ref,
		std::uint32_t a_maxDistance, int a_maxActorsCount,
		bool a_includeDead, std::function<bool(const RE::Actor*)> filter)
	{
		std::vector<RE::Actor*> result;

		auto originPos{ a_ref->GetPosition() };

		const auto processLists = RE::ProcessLists::GetSingleton();

		concurrency::parallel_for_each(processLists->highActorHandles.begin(), processLists->highActorHandles.end(),
			[&](const RE::ActorHandle& actorHandle)
			{
				const auto actorPtr = actorHandle.get();
				const auto currentActor = actorPtr.get();
				if (!currentActor) {
					return;
				}
				if (!a_includeDead && currentActor->IsDead(true)) {
					return;
				}
				if (currentActor == a_ref) {
					return;
				}
				if (filter && filter(currentActor) == false) {
					return;
				}
				if (originPos.GetDistance(currentActor->GetPosition()) <= a_maxDistance) {
					// Блокируем доступ к вектору при добавлении элемента
					static std::mutex mtx;
					std::lock_guard<std::mutex> lock(mtx);
					result.push_back(currentActor);
				}
			}
		);

		if (result.size() > a_maxActorsCount) result.resize(a_maxActorsCount);
		return result;
	}

	inline RE::TESForm* getFormFromString(const std::string& xFormID, const std::string& plugin)
	{
		if (xFormID.empty()) {
			return nullptr;
		}
		std::string trimmedID = xFormID.length() > 6 ? xFormID.substr(xFormID.length() - 6) : xFormID;
		trimmedID.erase(trimmedID.begin(), std::find_if(trimmedID.begin(), trimmedID.end(), [](char c) { return c != '0'; }));

		if (!trimmedID.empty() && trimmedID.front() == 'x') {
			trimmedID.erase(trimmedID.begin());
		}

		if (trimmedID.empty()) {
			return nullptr;
		}

		trimmedID = "0x" + trimmedID;

		try {
			uint32_t FormID_uint = std::stoul(trimmedID, nullptr, 16);
			return !plugin.empty() ?
				RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESForm>(FormID_uint, plugin) :
				RE::TESForm::GetFormByID(FormID_uint);
		}
		catch (const std::invalid_argument&) {
			return nullptr;
		}
		catch (const std::out_of_range&) {
			return nullptr;
		}
		catch (...) {
			return nullptr;
		}
	}

	inline RE::TESForm* getLeveledForm(const RE::Actor* actor) {
		if (actor == nullptr)
			return nullptr;

		if (actor->extraList == nullptr)
			return nullptr;
		auto& extraData = actor->extraList->extraData;
		auto leveledCreatureExtra = extraData.GetByType(RE::EXTRA_DATA_TYPE::kLeveledCreature);
		if (leveledCreatureExtra) {
			return *reinterpret_cast<RE::TESForm**>(reinterpret_cast<uintptr_t>(leveledCreatureExtra) + 0x18);
		}
		return nullptr;
	}

	inline void DoEquip(RE::NiPointer<RE::Actor> a, RE::TESForm* obj, RE::TBO_InstanceData* instData = nullptr) {
		auto inst = RE::BGSObjectInstance(obj, instData);
		RE::ActorEquipManager::GetSingleton()->EquipObject(a.get(), inst, 0, 1, nullptr, true, false, true, true, false);
	}

	inline void DoUnequip(RE::NiPointer<RE::Actor> a, RE::TESForm* obj, RE::TBO_InstanceData* instData = nullptr) {
		auto inst = RE::BGSObjectInstance(obj, instData);
		RE::ActorEquipManager::GetSingleton()->UnequipObject(a.get(), &inst, 0xFFFFFFFF, nullptr, 0, true, true, true, true, nullptr);
	}

	inline RE::BIPOBJECT* GetWornItem(RE::Actor* actor, RE::BIPED_OBJECT slot)
	{
		if (!actor || actor->As<RE::Actor>() == nullptr) {
			return nullptr;
		}

		auto biped = actor->GetBiped(false);

		if (!biped) {
			return nullptr;
		}

		const auto slotIndex = RE::stl::to_underlying(slot);
		if (slotIndex >= RE::stl::to_underlying(RE::BIPED_OBJECT::kTotal) || slotIndex <= RE::stl::to_underlying(RE::BIPED_OBJECT::kNone)) {
			return nullptr;
		}

		return biped->object[slotIndex].part ? &biped->object[slotIndex] : nullptr;
	}

	inline void EquipItemHelper(RE::NiPointer<RE::Actor> a, RE::BIPED_OBJECT slot, const RE::BGSObjectInstance& unequippedItem) {
		auto taskInterface = F4SE::GetTaskInterface();
		taskInterface->AddTask([a, slot, unequippedItem]() {
			if (!a || a->IsDeleted() || a->IsDisabled() || !a->Get3D()) {
				logger::info("DoReEquip - Equip ABORTED: Actor {} [0x{:x}] is invalid",
					a.get() ? a->GetDisplayFullName() : "",
					a.get() ? a->GetFormID() : 0
				);
				return;
			}

			if (!unequippedItem.object) {
				logger::info("DoReEquip - Equip ABORTED: Item object is null for actor  {} [0x{:x}]",
					a->GetDisplayFullName(),
					a->GetFormID()
				);
				return;
			}

			auto biped = a->GetBiped();
			if (!biped) {
				logger::info("DoReEquip - Equip ABORTED: Actor {} [0x{:x}] has no biped",
					a->GetDisplayFullName(),
					a->GetFormID()
				);
				return;
			}

			logger::info("DoReEquip - Equip: Actor {} [0x{:x}], slot {}, wornItem 0x{:x}",
				a->GetDisplayFullName(),
				a->GetFormID(),
				static_cast<uint32_t>(slot),
				unequippedItem.object->GetFormID()
			);

			RE::ActorEquipManager::GetSingleton()->EquipObject(
				a.get(),
				unequippedItem,
				0,
				1,
				nullptr,
				true,
				true,
				false,
				true,
				false
			);
			});
	}

	inline bool DoReEquip(
		RE::NiPointer<RE::Actor> a,
		RE::BIPED_OBJECT slot,
		size_t waitMilliseconds = 0,
		std::function<bool(RE::NiPointer<RE::NiAVObject>)> filter = nullptr
	)
	{
		if (!a) {
			return false;
		}

		auto wornItem = GetWornItem(a.get(), slot);

		if (wornItem == nullptr || wornItem->partClone == nullptr) {
			return false;
		}

		if (filter != nullptr) {
			if (filter(wornItem->partClone) == false) {
				return false;
			}
		}

		if (!wornItem->parent.object) {
			return false;
		}

		logger::critical("DoReEquip - Unequip: Actor [0x{:x}]{}, slot {}, wornItem 0x{:x}",
			a->GetFormID(),
			a->GetDisplayFullName(),
			static_cast<uint32_t>(slot),
			wornItem->parent.object->GetFormID()
		);

		// Копируем данные НЕМЕДЛЕННО, пока wornItem ещё валиден
		auto unequippedItem = wornItem->parent;

		// Сохраняем FormID для логирования (безопасно)
		auto actorFormID = a->GetFormID();
		auto actorName = std::string(a->GetDisplayFullName());
		auto itemFormID = unequippedItem.object ? unequippedItem.object->GetFormID() : 0;

		auto taskInterface = F4SE::GetTaskInterface();

		// Первая задача: снять предмет
		taskInterface->AddTask([a, slot, unequippedItem, waitMilliseconds]() {
			// Проверяем валидность актёра
			if (!a) return;

			// Проверяем, что актёр не удалён и не отключён
			if (a->IsDeleted() || a->IsDisabled()) return;

			// Проверяем, что у актёра есть 3D
			if (!a->Get3D()) return;

			a->UnequipArmorFromSlot(slot, true);

			// Надеть предмет обратно
			if (waitMilliseconds > 0) {
				std::thread([a, slot, unequippedItem, waitMilliseconds] {
					std::this_thread::sleep_for(std::chrono::milliseconds(waitMilliseconds));
					auto taskInterface = F4SE::GetTaskInterface();
					taskInterface->AddTask([a, slot, unequippedItem] {
						EquipItemHelper(a, slot, unequippedItem);
					});
				}).detach();
			}
			else {
				EquipItemHelper(a, slot, unequippedItem);
			}
		});

		return true;
	}

	inline double GetDistance(RE::NiPoint3 pt1, RE::NiPoint3 pt2)
	{
		return sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2) + pow(pt2.z - pt1.z, 2) * 1.0);
	}

	inline static bool RefIsDisabled(RE::TESObjectREFR* targetRef) {
		return targetRef == nullptr || targetRef->IsDeleted() || targetRef->IsDisabled();
	}

	inline static bool ActorIsEnabled(RE::Actor* targetActor, bool require3d = true) {
		return !RefIsDisabled(targetActor) && (!require3d || targetActor->Get3D() != nullptr);
	}

	inline static bool ActorIsAlive(RE::Actor* targetActor, bool require3d = true) {
		return !targetActor ? false : ActorIsEnabled(targetActor, require3d) && targetActor->lifeState != RE::ACTOR_LIFE_STATE::kDead;
	}

	template<class T>
	inline static std::vector<T*> GetRefsInCell(RE::TESObjectCELL* cell, const std::function<bool(T*)>& filter)
	{
		std::vector<T*> result;

		if (!cell) {
			return result;
		}

		RE::BSAutoLock l{ cell->spinLock };
		auto& references = cell->references;

		for (uint32_t i = 0; i < references.size(); i++) {
			T* refr = references[i]->As<T>();

			if (refr && filter(refr)) {
				result.push_back(refr);
			}
		}

		return result;
	}

	template<class T>
	inline static std::vector<T*> GetRefsInLoadedArea(const std::function<bool(T*)>& filter)
	{
		std::vector<T*> result;

		const auto& [map, lock] = RE::TESForm::GetAllForms();
		RE::BSAutoReadLock l{ lock };
		std::mutex results_lock;

		concurrency::parallel_for_each(map->begin(), map->end(), [&](RE::BSTTuple<const uint32_t, RE::TESForm*> ele) {
			RE::TESObjectCELL* cell = ele.second->As<RE::TESObjectCELL>();

			if (cell && cell->loadedData) {
				auto refs = GetRefsInCell(cell, filter);
				std::scoped_lock lock(results_lock);
				result.reserve(result.size() + refs.size());
				result.insert(result.end(), refs.begin(), refs.end());
			}
			});

		return result;
	}
}

namespace ppl_utils {

	/**
	 * @brief Потокобезопасное удаление элемента из concurrent_unordered_set через копирование
	 * @param set Concurrent set для модификации
	 * @param item Элемент для удаления
	 * @return true если элемент был удалён
	 */
	template<typename T>
	inline bool safe_erase_copy_swap(ppl::concurrent_unordered_set<T>& set, const T& item) {
		// Создаём копию всех элементов кроме удаляемого
		std::remove_reference_t<decltype(set)> new_set;
		bool found = false;

		// Копируем все элементы кроме удаляемого
		for (const auto& element : set) {
			if (element == item && !found) {
				found = true; // Пропускаем первое вхождение
			}
			else {
				new_set.insert(element);
			}
		}

		if (found) {
			set.swap(new_set);
		}

		return found;
	}

	/**
	 * @brief Потокобезопасное удаление с предикатом через копирование
	 * @param set Concurrent set для модификации
	 * @param predicate Функция для определения элементов к удалению
	 * @return Количество удалённых элементов
	 */
	template<typename T, typename Predicate>
	inline size_t safe_erase_if_copy_swap(ppl::concurrent_unordered_set<T>& set, Predicate predicate) {
		std::remove_reference_t<decltype(set)> new_set;
		size_t removed_count = 0;

		for (const auto& element : set) {
			if (predicate(element)) {
				++removed_count;
			}
			else {
				new_set.insert(element);
			}
		}

		if (removed_count > 0) {
			set.swap(new_set);
		}

		return removed_count;
	}
}

