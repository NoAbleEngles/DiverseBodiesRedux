#pragma once

#include "RE/Bethesda/BGSInventoryItem.h"
#include "RE/Bethesda/BSExtraData.h"
#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSSoundHandle.h"
#include "RE/Bethesda/BSStringT.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/BSTSmartPointer.h"
#include "RE/Bethesda/MemoryManager.h"
#include "RE/Bethesda/TESForms.h"
#include "RE/Havok/hknpBodyId.h"
#include "RE/Havok/hknpClosestUniqueBodyIdHitCollector.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "RE/NetImmerse/NiRefObject.h"
#include "RE/Bethesda/BSTriShape.h"

namespace RE
{
	//typedef TESObjectREFR* (*_PlaceAtMe_Native)(BSScript::IVirtualMachine* vm, uint32_t stackId, TESObjectREFR** target, TESForm* form, int32_t count, bool bForcePersist, bool bInitiallyDisabled, bool bDeleteWhenAble);
	//inline _PlaceAtMe_Native PlaceAtMe_Native = REL::Relocation<_PlaceAtMe_Native>(0x0140B0E0 + REL::Module::get().base()).get();
	//// Функция, вызывающая _PlaceAtMe_Native
	//inline TESObjectREFR* PlaceAtMe(BSScript::IVirtualMachine* vm, uint32_t stackId, TESObjectREFR** target, TESForm* form, int32_t count, bool bForcePersist, bool bInitiallyDisabled, bool bDeleteWhenAble)
	//{
	//	assert(PlaceAtMe_Native != nullptr);  // Убедитесь, что функция была правильно инициализирована
	//	return PlaceAtMe_Native(vm, stackId, target, form, count, bForcePersist, bInitiallyDisabled, bDeleteWhenAble);
	//}

	enum class BIPED_OBJECT;
	enum class IO_TASK_PRIORITY;

	namespace MagicSystem
	{
		enum class CastingSource;
	}

	namespace BGSBodyPartDefs
	{
		enum class LIMB_ENUM;
	}

	class ActorCause;
	class BGSAnimationSequencer;
	class BGSDecalGroup;
	class BGSInventoryList;
	class BGSObjectInstance;
	class BGSObjectInstanceExtra;
	class BSAnimationGraphChannel;
	class BSAnimationGraphManager;
	class BSFaceGenNiNode;
	class BShkbAnimationGraph;
	class DialogueResponse;
	class hkVector4f;
	class MagicCaster;
	class MagicTarget;
	class ModelReferenceEffect;
	class NiAVObject;
	class NiLight;
	class NiNode;
	class NiTransform;
	class NonActorMagicCaster;
	class SimpleAnimationGraphManagerLoadingTask;
	class TargetEntry;
	class TBO_InstanceData;
	class TrapData;
	class TrapEntry;
	class WeaponAnimationGraphManagerHolder;

	struct BSActiveGraphIfInactiveEvent;
	struct BSAnimationGraphEvent;
	struct BSAnimationUpdateData;

	namespace ActorValueEvents
	{
		struct ActorValueChangedEvent;
	}

	namespace BGSInventoryListEvent
	{
		enum class Type
		{
			kAddStack,
			kChangedStack,
			kAddNewItem,
			kRemoveItem,
			kClear,
			UpdateWeight
		};

		struct Event
		{
		public:
			// members
			stl::enumeration<Type, std::uint16_t> changeType;  // 00
			ObjectRefHandle owner;                             // 04
			TESBoundObject* objAffected;                       // 08
			std::uint32_t count;                               // 10
			std::uint32_t stackID;                             // 14
		};
		static_assert(sizeof(Event) == 0x18);
	}

	enum class ITEM_REMOVE_REASON
	{
		kNone = 0,
		kStealing = 1,
		kSelling = 2,
		KDropping = 3,
		kStoreContainer = 4,
		kStoreTeammate = 5
	};

	enum class RESET_3D_FLAGS : uint16_t
	{
		kNone = 0,
		kModel = 1u << 0,
		kSkin = 1u << 1,
		kHead = 1u << 2,
		kFace = 1u << 3,
		kScale = 1u << 4,
		kSkeleton = 1u << 5,
		kInitDefault = 1u << 6,
		kSkyCellSkin = 1u << 7,
		kHavok = 1u << 8,
		kDontAddOutfit = 1u << 9,
		kKeepHead = 1u << 10,
		kDismemberment = 1u << 11,
		kAll = kModel | kSkin | kHead | kFace | kScale | kSkeleton | kHavok
	};

	RESET_3D_FLAGS operator|(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS operator&(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS operator^(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS operator~(RESET_3D_FLAGS flags);
	RESET_3D_FLAGS& operator|=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS& operator&=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS& operator^=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs);
	RESET_3D_FLAGS& operator~(RESET_3D_FLAGS& flags);

	bool HasFlag(RESET_3D_FLAGS value, RESET_3D_FLAGS flag);

	class __declspec(novtable) BSHandleRefObject :
		public NiRefObject  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BSHandleRefObject };
		static constexpr auto VTABLE{ VTABLE::BSHandleRefObject };

		std::uint32_t DecRefCount()
		{
			stl::atomic_ref myRefCount{ refCount };
			const auto newRefCount = --myRefCount & 0x3FF;
			if (newRefCount == 0) {
				DeleteThis();
			}
			return newRefCount;
		}

		void IncRefCount()
		{
			stl::atomic_ref myRefCount{ refCount };
			++myRefCount;
		}

		[[nodiscard]] std::uint32_t QRefCount() const noexcept { return refCount & 0x3FF; }
	};
	static_assert(sizeof(BSHandleRefObject) == 0x10);

	class __declspec(novtable) IAnimationGraphManagerHolder
	{
	public:
		static constexpr auto RTTI{ RTTI::IAnimationGraphManagerHolder };
		static constexpr auto VTABLE{ VTABLE::IAnimationGraphManagerHolder };

		virtual ~IAnimationGraphManagerHolder() = default;  // 00

		// add
		virtual bool NotifyAnimationGraphImpl(const BSFixedString& a_eventName);                                                                                                                                                     // 01
		virtual void EventSuccessfullyProcessed() { return; }                                                                                                                                                                        // 02
		virtual void ModifyInitialAnimationStateImpl(const bool) { return; }                                                                                                                                                         // 03
		virtual bool GetAnimationGraphManagerImpl(BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) const = 0;                                                                                                               // 04
		virtual bool SetAnimationGraphManagerImpl(const BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) = 0;                                                                                                               // 05
		virtual bool PopulateGraphNodesToTarget([[maybe_unused]] BSScrapArray<NiAVObject*>& a_nodesToAnimate) const { return false; }                                                                                                // 06
		virtual bool ConstructAnimationGraph(BSTSmartPointer<BShkbAnimationGraph>& a_animGraph) = 0;                                                                                                                                 // 07
		virtual bool InitializeAnimationGraphVariables([[maybe_unused]] const BSTSmartPointer<BShkbAnimationGraph>& a_newGraph) const { return true; }                                                                               // 08
		virtual bool SetupAnimEventSinks([[maybe_unused]] const BSTSmartPointer<BShkbAnimationGraph>& a_newGraph) { return true; }                                                                                                   // 09
		virtual void DoFailedToLoadGraph() const { return; }                                                                                                                                                                         // 0A
		virtual bool CreateAnimationChannels([[maybe_unused]] BSScrapArray<BSTSmartPointer<BSAnimationGraphChannel>>& a_channels) { return false; }                                                                                  // 0B
		virtual void PostCreateAnimationGraphManager([[maybe_unused]] BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) { return; }                                                                                          // 0C
		virtual void PostChangeAnimationManager([[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_newAnimGraphMgr, [[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_oldAnimGraphMgr) { return; }  // 0D
		virtual bool ShouldUpdateAnimation() { return true; }                                                                                                                                                                        // 0E
		virtual std::uint32_t GetGraphVariableCacheSize() const { return 0; }                                                                                                                                                        // 0F
		virtual bool GetGraphVariableImpl([[maybe_unused]] std::uint32_t a_graphVarID, [[maybe_unused]] float& a_out) const { return false; }                                                                                        // 10
		virtual bool GetGraphVariableImpl([[maybe_unused]] std::uint32_t a_graphVarID, [[maybe_unused]] bool& a_out) const { return false; }                                                                                         // 11
		virtual bool GetGraphVariableImpl([[maybe_unused]] std::uint32_t a_graphVarID, [[maybe_unused]] std::int32_t& a_out) const { return false; }                                                                                 // 12
		virtual bool GetGraphVariableImpl(const BSFixedString& a_variable, float& a_out) const;                                                                                                                                      // 13
		virtual bool GetGraphVariableImpl(const BSFixedString& a_variable, std::int32_t& a_out) const;                                                                                                                               // 14
		virtual bool GetGraphVariableImpl(const BSFixedString& a_variable, bool& a_out) const;                                                                                                                                       // 15
		virtual void PreUpdateAnimationGraphManager([[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) const { return; }                                                                               // 16
		virtual void PostUpdateAnimationGraphManager([[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) const { return; }                                                                              // 17
		virtual void PreLoadAnimationGraphManager([[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) { return; }                                                                                       // 18
		virtual void PostLoadAnimationGraphManager([[maybe_unused]] const BSTSmartPointer<BSAnimationGraphManager>& a_animGraphMgr) { return; }                                                                                      // 19

		// There are overloads of this for int (81D3F0), float (81D410), NiPoint3 (81D430), and NiQuaternion (81D440) values. (game ver. 1-10-163)
		bool SetGraphVariable(const BSFixedString& name, bool value)
		{
			using func_t = decltype(&IAnimationGraphManagerHolder::SetGraphVariable);
			REL::Relocation<func_t> func{ REL::ID(1437197) };
			return func(this, name, value);
		}

		void GetAnimationGraphProjectName(BSFixedString& a_nameOut) {
			using func_t = decltype(&IAnimationGraphManagerHolder::GetAnimationGraphProjectName);
			REL::Relocation<func_t> func{ REL::ID(190119) };
			return func(this, a_nameOut);
		}
	};
	static_assert(sizeof(IAnimationGraphManagerHolder) == 0x8);

	class __declspec(novtable) SimpleAnimationGraphManagerHolder :
		public IAnimationGraphManagerHolder
	{
	public:
		static constexpr auto RTTI{ RTTI::SimpleAnimationGraphManagerHolder };
		static constexpr auto VTABLE{ VTABLE::SimpleAnimationGraphManagerHolder };

		virtual ~SimpleAnimationGraphManagerHolder() = default;  // 00

		// add
		virtual void BackgroundTaskFinishedLoading() { return; }  // 01

		// members
		BSTSmartPointer<BSAnimationGraphManager> animationGraphManager;  // 08
		NiPointer<SimpleAnimationGraphManagerLoadingTask> loadingTask;   // 10
	};
	static_assert(sizeof(SimpleAnimationGraphManagerHolder) == 0x18);

	class BGSEquipIndex
	{
	public:
		~BGSEquipIndex() noexcept {}  // NOLINT(modernize-use-equals-default)

		// members
		std::uint32_t index;  // 0
	};
	static_assert(sizeof(BGSEquipIndex) == 0x4);

	struct SubgraphIdentifier
	{
	public:
		// members
		std::size_t identifier;  // 0
	};
	static_assert(sizeof(SubgraphIdentifier) == 0x8);

	struct BGSBehaviorRootData
	{
	public:
		// members
		BSFixedString idleManagerRootToUse;  // 00
		BSFixedString behaviorRootFilename;  // 08
		SubgraphIdentifier identifier;       // 10
		bool firstPerson;                    // 18
	};
	static_assert(sizeof(BGSBehaviorRootData) == 0x20);

	class BGSObjectInstance
	{
	public:
		BGSObjectInstance(TESForm* a_object, TBO_InstanceData* a_instanceData) :
			object(a_object),
			instanceData(a_instanceData)
		{}

		// members
		TESForm* object{ nullptr };                      // 00
		BSTSmartPointer<TBO_InstanceData> instanceData;  // 08
	};
	static_assert(sizeof(BGSObjectInstance) == 0x10);

	template <class T>
	class BGSObjectInstanceT :
		public BGSObjectInstance
	{
	public:
		BGSObjectInstanceT(T* a_object, TBO_InstanceData* a_instanceData) :
			BGSObjectInstance(a_object, a_instanceData)
		{
		}
	};

	struct OBJ_REFR
	{
	public:
		// members
		NiPoint3A angle;                  // 00
		NiPoint3A location;               // 10
		TESBoundObject* objectReference;  // 20
	};
	static_assert(sizeof(OBJ_REFR) == 0x30);

	struct LOADED_REF_DATA
	{
	public:
		// members
		void* handleList;                // 00 - TODO
		NiPointer<NiAVObject> data3D;    // 08
		TESWaterForm* currentWaterType;  // 10
		float relevantWaterHeight;       // 18
		float cachedRadius;              // 1C
		std::uint16_t flags;             // 20
		std::int16_t underwaterCount;    // 22
	};
	static_assert(sizeof(LOADED_REF_DATA) == 0x28);

	class BGSInventoryList :
		public BSTEventSource<BGSInventoryListEvent::Event>  // 00
	{
	public:
		BGSInventoryList(const TESContainer* a_container, ObjectRefHandle a_owner)
		{
			auto native_handle = a_owner.native_handle();
			ctor(a_container, &native_handle);
		}

		F4_HEAP_REDEFINE_NEW(BGSInventoryList)

		[[nodiscard]] static bool StandardObjectCompareCallbackFn(TESBoundObject* a_lhs, TESBoundObject* a_rhs)
		{
			return a_lhs == a_rhs;
		}

		void FindAndWriteStackDataForItem(
			TESBoundObject* a_object,
			BGSInventoryItem::StackDataCompareFunctor& a_compareFunc,
			BGSInventoryItem::StackDataWriteFunctor& a_writeFunc,
			bool (*a_objCompFn)(TESBoundObject*, TESBoundObject*) = StandardObjectCompareCallbackFn,
			bool a_alwaysContinue = false)
		{
			using func_t = decltype(&BGSInventoryList::FindAndWriteStackDataForItem);
			REL::Relocation<func_t> func{ REL::ID(1354005) };
			return func(this, a_object, a_compareFunc, a_writeFunc, a_objCompFn, a_alwaysContinue);
		}

		// DOES NOT LOCK
		void ForEachStack(
			std::function<bool(BGSInventoryItem&)> a_filter,                             // return true to iterate stacks
			std::function<bool(BGSInventoryItem&, BGSInventoryItem::Stack&)> a_continue  // return false to return control from function
		)
		{
			for (auto& elem : data) {
				if (a_filter(elem)) {
					for (auto stack = elem.stackData.get(); stack; stack = stack->nextStack.get()) {
						if (!a_continue(elem, *stack)) {
							return;
						}
					}
				}
			}
		}

		// members
		BSTArray<BGSInventoryItem> data;  // 58
		float cachedWeight;               // 70
		ObjectRefHandle owner;            // 74
		BSReadWriteLock rwLock;           // 78

	private:
		void ctor(const TESContainer* a_container, std::uint32_t* a_owner)
		{
			using func_t = decltype(&BGSInventoryList::ctor);
			REL::Relocation<func_t> func{ REL::ID(845050) };
			return func(this, a_container, a_owner);
		}
	};
	static_assert(sizeof(BGSInventoryList) == 0x80);

	enum class BIPED_OBJECT
	{
		kNone = static_cast<std::underlying_type_t<BIPED_OBJECT>>(-1),

		kHairTop_30,
		kHairLong_31,
		kFaceGenHead_32,
		kBODY_33,
		kLHand_34,
		kRHand_35,
		kU_Torso_36,
		kU_LArm_37,
		kU_RArm_38,
		kU_LLeg_39,
		kU_RLeg_40,
		kA_Torso_41,
		kA_LArm_42,
		kA_RArm_43,
		kA_LLeg_44,
		kA_RLeg_45,
		kHeadband_46,
		kEyes_47,
		kBeard_48,
		kMouth_49,
		kNeck_50,
		kRing_51,
		kScalp_52,
		kDecapitation_53,
		kUnnamed_54,
		kUnnamed_55,
		kUnnamed_56,
		kUnnamed_57,
		kUnnamed_58,
		kShield_59,
		kPipboy_60,
		kFX_61,

		kEditorCount = 32,

		kWeaponHand = kEditorCount,
		kWeaponSword,
		kWeaponDagger,
		kWeaponAxe,
		kWeaponMace,
		kWeaponTwoHandMelee,
		kWeaponBow,
		kWeaponStaff,
		kQuiver,
		kWeaponGun,
		kWeaponGrenade,
		kWeaponMine,

		kTotal
	};

	struct BIPOBJECT
	{
	public:
		~BIPOBJECT();

		// members
		BGSObjectInstance parent;          // 00
		BGSObjectInstanceExtra* modExtra;  // 10
		TESObjectARMA* armorAddon;         // 18
		TESModel* part;                    // 20
		BGSTextureSet* skinTexture;        // 28
		NiPointer<NiAVObject> partClone;   // 30
		void* handleList;                  // 38 - TODO: BSModelDB::HandleListHead
		union
		{
			std::byte spare40;
			BSTSmartPointer<WeaponAnimationGraphManagerHolder> objectGraphManager;
		};  // 40 - TODO
		union
		{
			std::byte spare48;
			NiPointer<ModelReferenceEffect> hitEffect;
		};             // 48 - TODO
		bool skinned;  // 50
	};
	static_assert(sizeof(BIPOBJECT) == 0x58);

	class BipedAnim :
		public BSIntrusiveRefCounted  // 0000
	{
	public:
		// members
		NiNode* root;                                                         // 0008
		BIPOBJECT object[stl::to_underlying(BIPED_OBJECT::kTotal)];           // 0010
		BIPOBJECT bufferedObjects[stl::to_underlying(BIPED_OBJECT::kTotal)];  // 0F30
		ObjectRefHandle actorRef;                                             // 1E50

		// --- Инициализация ---
		void InitRoot(TESObjectREFR& a_ref, NiAVObject& a_obj)
		{
			using func_t = decltype(&BipedAnim::InitRoot);
			REL::Relocation<func_t> func{ REL::Offset(0x58DC90) };
			return func(this, a_ref, a_obj);
		}
		void InitNaked(TESRace* race, TESObjectARMO* armor, std::uint32_t sex)
		{
			using func_t = decltype(&BipedAnim::InitNaked);
			REL::Relocation<func_t> func{ REL::Offset(0x58E6D0) };
			func(this, race, armor, sex);
		}

		// --- Загрузка и обработка частей ---
		void RemoveClothingAndArmor()
		{
			using func_t = decltype(&BipedAnim::RemoveClothingAndArmor);
			REL::Relocation<func_t> func{ REL::Offset(0x58DD20) };
			func(this);
		}
		void RemoveAllParts()
		{
			using func_t = decltype(&BipedAnim::RemoveAllParts);
			REL::Relocation<func_t> func{ REL::Offset(0x58DD80) };
			func(this);
		}
		void RemovePartA(BIPOBJECT* part, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::RemovePartA);
			REL::Relocation<func_t> func{ REL::Offset(0x58DF50) };
			func(this, part, a_flag);
		}
		void RemovePartB(BIPED_OBJECT obj, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::RemovePartB);
			REL::Relocation<func_t> func{ REL::Offset(0x58E4F0) };
			func(this, obj, a_flag);
		}
		void RemovePartC(TESModel* model, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::RemovePartC);
			REL::Relocation<func_t> func{ REL::Offset(0x58E520) };
			func(this, model, a_flag);
		}
		void Revert()
		{
			using func_t = decltype(&BipedAnim::Revert);
			REL::Relocation<func_t> func{ REL::Offset(0x58E5B0) };
			func(this);
		}
		void SetBipedPart(BGSObjectInstanceT<TESObjectARMO>& instance, TESObjectARMA* arma, TESModel* model, BGSTextureSet* tex)
		{
			using func_t = decltype(&BipedAnim::SetBipedPart);
			REL::Relocation<func_t> func{ REL::Offset(0x58E950) };
			func(this, instance, arma, model, tex);
		}
		void LoadBipedParts()
		{
			using func_t = decltype(&BipedAnim::LoadBipedParts);
			REL::Relocation<func_t> func{ REL::Offset(0x58ECB0) };
			func(this);
		}
		void QueueBipedParts(IO_TASK_PRIORITY priority)
		{
			using func_t = decltype(&BipedAnim::QueueBipedParts);
			REL::Relocation<func_t> func{ REL::Offset(0x58FA40) };
			func(this, priority);
		}

		// --- Анимация ---
		void LoadBipedAnimationGraphs()
		{
			using func_t = decltype(&BipedAnim::LoadBipedAnimationGraphs);
			REL::Relocation<func_t> func{ REL::Offset(0x58FDB0) };
			func(this);
		}
		void FadeNodeOrParentCulled(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::FadeNodeOrParentCulled);
			REL::Relocation<func_t> func{ REL::Offset(0x58FE50) };
			func(this, obj);
		}

		// --- Работа с оружием/боеприпасами/фонарём ---
		void SetBipedWeapon(BGSObjectInstanceT<TESObjectWEAP>& instance, BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::SetBipedWeapon);
			REL::Relocation<func_t> func{ REL::Offset(0x58FEE0) };
			func(this, instance, equipIndex);
		}
		void AttachBipedWeapon(BGSObjectInstanceT<TESObjectWEAP>& instance, BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::AttachBipedWeapon);
			REL::Relocation<func_t> func{ REL::Offset(0x590040) };
			func(this, instance, equipIndex);
		}
		void RemoveBipedWeapon(BIPED_OBJECT obj, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::RemoveBipedWeapon);
			REL::Relocation<func_t> func{ REL::Offset(0x5903E0) };
			func(this, obj, a_flag);
		}
		void ClearDroppedWeapons()
		{
			using func_t = decltype(&BipedAnim::ClearDroppedWeapons);
			REL::Relocation<func_t> func{ REL::Offset(0x590640) };
			func(this);
		}
		uint32_t GetGearedUpWeaponCount()
		{
			using func_t = decltype(&BipedAnim::GetGearedUpWeaponCount);
			REL::Relocation<func_t> func{ REL::Offset(0x5908E0) };
			return func(this);
		}
		void SetBipedAmmo(TESAmmo* ammo)
		{
			using func_t = decltype(&BipedAnim::SetBipedAmmo);
			REL::Relocation<func_t> func{ REL::Offset(0x590960) };
			func(this, ammo);
		}
		void AttachBipedAmmo(TESAmmo* ammo)
		{
			using func_t = decltype(&BipedAnim::AttachBipedAmmo);
			REL::Relocation<func_t> func{ REL::Offset(0x590A90) };
			func(this, ammo);
		}
		void RemoveBipedAmmo()
		{
			using func_t = decltype(&BipedAnim::RemoveBipedAmmo);
			REL::Relocation<func_t> func{ REL::Offset(0x590CE0) };
			func(this);
		}
		void SetBipedTorch(TESObjectLIGH* torch, BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::SetBipedTorch);
			REL::Relocation<func_t> func{ REL::Offset(0x590D00) };
			func(this, torch, equipIndex);
		}
		void AttachBipedTorch(TESObjectLIGH* torch, BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::AttachBipedTorch);
			REL::Relocation<func_t> func{ REL::Offset(0x590E60) };
			func(this, torch, equipIndex);
		}
		void RemoveBipedTorch(BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::RemoveBipedTorch);
			REL::Relocation<func_t> func{ REL::Offset(0x5912C0) };
			func(this, equipIndex);
		}

		// --- Получение объектов экипировки ---
		NiAVObject* GetTorchObject(BGSEquipIndex equipIndex, NiPointer<NiAVObject>& outObj)
		{
			using func_t = decltype(&BipedAnim::GetTorchObject);
			REL::Relocation<func_t> func{ REL::Offset(0x591440) };
			return func(this, equipIndex, outObj);
		}
		NiAVObject* GetShieldObject()
		{
			using func_t = decltype(&BipedAnim::GetShieldObject);
			REL::Relocation<func_t> func{ REL::Offset(0x5915B0) };
			return func(this);
		}
		NiAVObject* GetBodyObject()
		{
			using func_t = decltype(&BipedAnim::GetBodyObject);
			REL::Relocation<func_t> func{ REL::Offset(0x591650) };
			return func(this);
		}
		NiNode* LoadFaceBonesModel(BIPED_OBJECT obj, NiPointer<NiNode>& outNode)
		{
			using func_t = decltype(&BipedAnim::LoadFaceBonesModel);
			REL::Relocation<func_t> func{ REL::Offset(0x591720) };
			return func(this, obj, outNode);
		}

		// --- Работа с графами анимаций ---
		void SetBipObjectGraphVarA(BIPED_OBJECT obj, BSFixedString& name, int value)
		{
			using func_t = decltype(&BipedAnim::SetBipObjectGraphVarA);
			REL::Relocation<func_t> func{ REL::Offset(0x591980) };
			func(this, obj, name, value);
		}
		void SetBipObjectGraphVarB(BIPED_OBJECT obj, BSFixedString& name, float value)
		{
			using func_t = decltype(&BipedAnim::SetBipObjectGraphVarB);
			REL::Relocation<func_t> func{ REL::Offset(0x5919B0) };
			func(this, obj, name, value);
		}
		void AttachToSkeleton(NiNode* node1, NiAVObject* avObj, NiNode* node2, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::AttachToSkeleton);
			REL::Relocation<func_t> func{ REL::Offset(0x5919E0) };
			func(this, node1, avObj, node2, a_flag);
		}
		void AddMissingBones(NiNode* node1, NiAVObject* avObj, NiNode* node2)
		{
			using func_t = decltype(&BipedAnim::AddMissingBones);
			REL::Relocation<func_t> func{ REL::Offset(0x591D50) };
			func(this, node1, avObj, node2);
		}
		void LoadAndAttachAddOn(TESModel* model, BIPED_OBJECT obj, TESObjectREFR* ref, BSTSmartPointer<BipedAnim>& biped, NiAVObject* avObj)
		{
			using func_t = decltype(&BipedAnim::LoadAndAttachAddOn);
			REL::Relocation<func_t> func{ REL::Offset(0x591E60) };
			func(this, model, obj, ref, biped, avObj);
		}
		void AddAddonNodes(NiNode* node1, NiNode* node2, BIPED_OBJECT obj, TESObjectREFR* ref, BSTSmartPointer<BipedAnim>& biped)
		{
			using func_t = decltype(&BipedAnim::AddAddonNodes);
			REL::Relocation<func_t> func{ REL::Offset(0x592420) };
			func(this, node1, node2, obj, ref, biped);
		}
		void* GetLightingProperty(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::GetLightingProperty);
			REL::Relocation<func_t> func{ REL::Offset(0x592510) };
			return func(this, obj);
		}
		NiAVObject* GetSkinBipedObject(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::GetSkinBipedObject);
			REL::Relocation<func_t> func{ REL::Offset(0x592570) };
			return func(this, obj);
		}
		uint32_t GetSkinModelFlags(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::GetSkinModelFlags);
			REL::Relocation<func_t> func{ REL::Offset(0x5925E0) };
			return func(this, obj);
		}
		NiNode* FindSkinnedNode(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::FindSkinnedNode);
			REL::Relocation<func_t> func{ REL::Offset(0x592670) };
			return func(this, obj);
		}
		ExtraDataList* FindParentExtra(NiAVObject* obj)
		{
			using func_t = decltype(&BipedAnim::FindParentExtra);
			REL::Relocation<func_t> func{ REL::Offset(0x5926A0) };
			return func(this, obj);
		}
		void AttachToParent(NiNode* node1, NiNode* node2, NiNode* node3, BSTSmartPointer<BipedAnim>& biped, BIPED_OBJECT obj)
		{
			using func_t = decltype(&BipedAnim::AttachToParent);
			REL::Relocation<func_t> func{ REL::Offset(0x5926E0) };
			func(this, node1, node2, node3, biped, obj);
		}

		// --- Получение объектов/слотов ---
		NiAVObject* GetBipedObject(TESObjectREFR* ref, TESForm* form, BGSEquipIndex equipIndex)
		{
			using func_t = decltype(&BipedAnim::GetBipedObject);
			REL::Relocation<func_t> func{ REL::Offset(0x592DD0) };
			return func(this, ref, form, equipIndex);
		}
		BIPED_OBJECT GetBipedObjectEquipIndex(TESObjectREFR* ref, TESForm* form, BIPED_OBJECT obj)
		{
			using func_t = decltype(&BipedAnim::GetBipedObjectEquipIndex);
			REL::Relocation<func_t> func{ REL::Offset(0x592E20) };
			return func(this, ref, form, obj);
		}

		// --- Применение/буферизация частей тела ---
		void ApplySkinnedObjects(NiNode* node, BIPED_OBJECT obj, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::ApplySkinnedObjects);
			REL::Relocation<func_t> func{ REL::Offset(0x592E60) };
			func(this, node, obj, a_flag);
		}
		void BuildObjectArray(BSTArray<BGSObjectInstance>& result)
		{
			using func_t = decltype(&BipedAnim::BuildObjectArray);
			REL::Relocation<func_t> func{ REL::Offset(0x5931A0) };
			func(this, result);
		}
		void InitHavok(BSTSmartPointer<BipedAnim>& biped, BIPED_OBJECT obj, NiAVObject* avObj)
		{
			using func_t = decltype(&BipedAnim::InitHavok);
			REL::Relocation<func_t> func{ REL::Offset(0x593240) };
			func(this, biped, obj, avObj);
		}
		void HideShowBufferedSkin(NiAVObject* obj, BIPED_OBJECT bipObj, bool show)
		{
			using func_t = decltype(&BipedAnim::HideShowBufferedSkin);
			REL::Relocation<func_t> func{ REL::Offset(0x593470) };
			func(this, obj, bipObj, show);
		}
		void HideShowSkinParts(BSTriShape* triShape, BIPED_OBJECT bipObj, bool show)
		{
			using func_t = decltype(&BipedAnim::HideShowSkinParts);
			REL::Relocation<func_t> func{ REL::Offset(0x593530) };
			func(this, triShape, bipObj, show);
		}
		void HideHeadExtraGeometry(BIPED_OBJECT obj)
		{
			using func_t = decltype(&BipedAnim::HideHeadExtraGeometry);
			REL::Relocation<func_t> func{ REL::Offset(0x593900) };
			func(this, obj);
		}
		void AttachSkinnedObject(NiNode* node1, NiNode* node2, BIPED_OBJECT obj, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::AttachSkinnedObject);
			REL::Relocation<func_t> func{ REL::Offset(0x593A10) };
			func(this, node1, node2, obj, a_flag);
		}
		void AdjustSkinComplexion(NiAVObject* obj, BIPED_OBJECT bipObj)
		{
			using func_t = decltype(&BipedAnim::AdjustSkinComplexion);
			REL::Relocation<func_t> func{ REL::Offset(0x594110) };
			func(this, obj, bipObj);
		}

		// --- Графы анимации ---
		void CreateAnimationGraphManagerHolder(BIPED_OBJECT obj, TESObjectREFR& ref)
		{
			using func_t = decltype(&BipedAnim::CreateAnimationGraphManagerHolder);
			REL::Relocation<func_t> func{ REL::Offset(0x594150) };
			func(this, obj, ref);
		}
		void RetrieveProjectForBipedSlot(BIPED_OBJECT obj, BSFixedString& outStr)
		{
			using func_t = decltype(&BipedAnim::RetrieveProjectForBipedSlot);
			REL::Relocation<func_t> func{ REL::Offset(0x594510) };
			func(this, obj, outStr);
		}

		// --- Прочие ---
		void RunBiped3DDetach(BSPointerHandle<TESObjectREFR>* handle, NiAVObject* obj, bool a_flag)
		{
			using func_t = decltype(&BipedAnim::RunBiped3DDetach);
			REL::Relocation<func_t> func{ REL::Offset(0x58DE30) };
			func(this, handle, obj, a_flag);
		}
	};
	static_assert(sizeof(BipedAnim) == 0x1E58);

	enum class LOCK_LEVEL
	{
		kUnlocked = static_cast<std::underlying_type_t<BIPED_OBJECT>>(-1),
		kEasy = 0,
		kAverage = 1,
		kHard = 2,
		kVeryHard = 3,
		kRequiresKey = 4,
		kInaccessible = 5,
		kTerminal = 6,
		kBarred = 7,
		kChained = 8,
	};

	struct REFR_LOCK
	{
	public:
		[[nodiscard]] LOCK_LEVEL GetLockLevel(TESObjectREFR* a_owner)
		{
			using func_t = decltype(&REFR_LOCK::GetLockLevel);
			REL::Relocation<func_t> func{ REL::ID(782953) };
			return func(this, a_owner);
		}

		void SetLocked(bool a_locked)
		{
			using func_t = decltype(&REFR_LOCK::SetLocked);
			REL::Relocation<func_t> func{ REL::ID(157617) };
			return func(this, a_locked);
		}

		// members
		std::uint8_t baseLevel;  // 00
		TESKey* key;             // 08
		std::uint8_t flags;      // 10
		std::uint32_t numTries;  // 14
	};
	static_assert(sizeof(REFR_LOCK) == 0x18);

	class __declspec(novtable) TESObjectREFR :
		public TESForm,                                                  // 000
		public BSHandleRefObject,                                        // 020
		public BSTEventSink<BSActiveGraphIfInactiveEvent>,               // 030
		public BSTEventSink<BSAnimationGraphEvent>,                      // 038
		public BSTEventSink<BGSInventoryListEvent::Event>,               // 040
		public IAnimationGraphManagerHolder,                             // 048
		public IKeywordFormBase,                                         // 050
		public ActorValueOwner,                                          // 058
		public BSTEventSource<ActorValueEvents::ActorValueChangedEvent>  // 060
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::TESObjectREFR, RTTI_AE::TESObjectREFR) };
		static constexpr auto VTABLE{ VTABLE::TESObjectREFR };
		static constexpr auto FORM_ID{ ENUM_FORM_ID::kREFR };

		struct RemoveItemData
		{
		public:
			RemoveItemData(TESForm* a_form, std::int32_t a_count) :
				RemoveItemData(a_form->As<TESBoundObject>(), a_count)
			{}

			RemoveItemData(TESBoundObject* a_object, std::int32_t a_count) :
				object(a_object), count(a_count)
			{}

			// members
			BSTSmallArray<std::uint32_t, 4> stackData;               // 00
			TESBoundObject* object{ nullptr };                       // 20
			std::int32_t count{ 0 };                                 // 28
			ITEM_REMOVE_REASON reason{ ITEM_REMOVE_REASON::kNone };  // 2C
			TESObjectREFR* a_otherContainer{ nullptr };              // 30
			const NiPoint3* dropLoc{ nullptr };                      // 38
			const NiPoint3* rotate{ nullptr };                       // 40
		};
		static_assert(sizeof(RemoveItemData) == 0x48);

		F4_HEAP_REDEFINE_NEW(TESObjectREFR);

		// add
		virtual void Predestroy();                                                                                                                                                                                                                    // 4A
		virtual void PreModelLoaderQueueing() { return; }                                                                                                                                                                                             // 4B
		virtual BGSLocation* GetEditorLocation() const;                                                                                                                                                                                               // 4C
		virtual bool GetEditorLocation(NiPoint3& a_originalLocation, NiPoint3& a_originalAngle, TESForm*& a_locationFormOut, TESForm* a_locationForm);                                                                                                // 4D
		virtual bool IsTalking() const;                                                                                                                                                                                                               // 4E
		virtual void ForceEditorLocation(BGSLocation* a_location);                                                                                                                                                                                    // 4F
		virtual void Update3DPosition(bool a_warp);                                                                                                                                                                                                   // 50
		virtual void UpdateSoundCallBack(bool a_endSceneAction);                                                                                                                                                                                      // 51
		virtual bool SetDialoguewithPlayer(bool a_flag, bool a_forceGreet, TESTopicInfo* a_topicInfo);                                                                                                                                                // 52
		virtual void AddDynamicIdleCRC([[maybe_unused]] std::uint32_t a_crc) { return; }                                                                                                                                                              // 53
		virtual void GetPreviousDynamicIdleCRCS(std::uint32_t* a_crcs, std::uint32_t a_requestSize) const;                                                                                                                                            // 54
		virtual bool GetFullLODRef() const;                                                                                                                                                                                                           // 55
		virtual void SetFullLODRef(bool a_val);                                                                                                                                                                                                       // 56
		virtual BGSAnimationSequencer* GetSequencer() const;                                                                                                                                                                                          // 57
		virtual bool QCanUpdateSync() const { return true; }                                                                                                                                                                                          // 58
		virtual bool GetAllowPromoteToPersistent() const { return true; }                                                                                                                                                                             // 59
		virtual bool DoesFloat() const { return false; }                                                                                                                                                                                              // 5A
		virtual TESPackage* CheckForCurrentAliasPackage() { return nullptr; }                                                                                                                                                                         // 5B
		virtual BGSScene* GetCurrentScene() const;                                                                                                                                                                                                    // 5C
		virtual void SetCurrentScene(BGSScene* a_scene);                                                                                                                                                                                              // 5D
		virtual bool UpdateInDialogue(DialogueResponse* a_response, bool);                                                                                                                                                                            // 5E
		virtual BGSDialogueBranch* GetExclusiveBranch() const;                                                                                                                                                                                        // 5F
		virtual void SetExclusiveBranch(BGSDialogueBranch* a_exclusiveBranch);                                                                                                                                                                        // 60
		virtual void StopCurrentDialogue(bool a_force);                                                                                                                                                                                               // 61
		virtual void FadeCurrentDialogue();                                                                                                                                                                                                           // 62
		virtual void PauseCurrentDialogue();                                                                                                                                                                                                          // 63
		virtual void ResumeCurrentDialogue();                                                                                                                                                                                                         // 64
		virtual void SetActorCause(ActorCause* a_actorCause);                                                                                                                                                                                         // 65
		virtual ActorCause* GetActorCause() const;                                                                                                                                                                                                    // 66
		virtual float GetActorWidthOrRefDiameter() const;                                                                                                                                                                                             // 67
		virtual float GetActorHeightOrRefBound() const;                                                                                                                                                                                               // 68
		virtual NiPoint3 GetStartingAngle();                                                                                                                                                                                                          // 69
		virtual NiPoint3 GetStartingLocation();                                                                                                                                                                                                       // 6A
		virtual void SetStartingPosition(const NiPoint3& a_position);                                                                                                                                                                                 // 6B
		virtual void UpdateRefLight(float a_delta);                                                                                                                                                                                                   // 6C
		virtual ObjectRefHandle RemoveItem(RemoveItemData& a_data);                                                                                                                                                                                   // 6D
		virtual void AddWornOutfitImpl([[maybe_unused]] BGSOutfit* a_outfit, [[maybe_unused]] bool a_queueItem) { return; }                                                                                                                           // 6E
		virtual bool AddWornItem(TESBoundObject* a_object, BSTSmartPointer<ExtraDataList> a_extra, std::int32_t a_number, bool a_forceEquip, BGSEquipIndex a_equipIndex);                                                                             // 6F
		virtual void DoTrap([[maybe_unused]] TrapData& a_trapData) { return; }                                                                                                                                                                        // 70
		virtual void DoTrap([[maybe_unused]] TrapEntry* a_trap, [[maybe_unused]] TargetEntry* a_target) { return; }                                                                                                                                   // 71
		virtual void SetEquipStateLocked(TESBoundObject* a_object, bool a_lock);                                                                                                                                                                      // 72
		virtual void SendContainerChangedEvent(const TESObjectREFR* a_oldContainer, const TESObjectREFR* apNewContainer, const TESBoundObject* a_baseObject, std::int32_t a_count, std::uint32_t a_referenceFormID, std::uint16_t a_uniqueID) const;  // 73
		virtual void UnequipArmorFromSlot([[maybe_unused]] BIPED_OBJECT a_bipedObject, [[maybe_unused]] bool a_markAsReequippable) { return; }                                                                                                        // 74
		virtual void RemoveAllObjectsWorn();                                                                                                                                                                                                          // 75
		virtual void PostRemoveWornObject(const BGSObjectInstance& a_object, const BGSEquipSlot* a_equipSlot);                                                                                                                                        // 76
		virtual void Set3DUpdateFlag([[maybe_unused]] RESET_3D_FLAGS a_flag) { return; }                                                                                                                                                              // 77
		virtual void ClearAll3DUpdateFlags() { return; }                                                                                                                                                                                              // 78
		virtual float AdjustItemWeight([[maybe_unused]] TESBoundObject& a_object, [[maybe_unused]] const BGSInventoryItem::Stack& a_stack, float a_weight, [[maybe_unused]] bool* a_shouldModifyWholeStackOut) { return a_weight; }                   // 79
		virtual void AddObjectToContainer(TESBoundObject* a_obj, BSTSmartPointer<ExtraDataList> a_extra, std::int32_t a_count, TESObjectREFR* a_oldContainer, ITEM_REMOVE_REASON a_reason);                                                           // 7A
		virtual NiPoint3 GetLookingAtLocation() const;                                                                                                                                                                                                // 7B
		virtual MagicCaster* GetMagicCaster(MagicSystem::CastingSource a_source);                                                                                                                                                                     // 7C
		virtual MagicTarget* GetMagicTarget();                                                                                                                                                                                                        // 7D
		virtual bool IsChild() const { return false; }                                                                                                                                                                                                // 7E
		virtual TESActorBase* GetTemplateActorBase() { return nullptr; }                                                                                                                                                                              // 7F
		virtual void SetTemplateActorBase(TESActorBase*) { return; }                                                                                                                                                                                  // 80
		virtual BSFaceGenNiNode* GetFaceNodeSkinned() { return nullptr; }                                                                                                                                                                             // 81
		virtual BSFaceGenNiNode* GetFaceNode() { return GetFaceNodeSkinned(); }                                                                                                                                                                       // 82
		virtual bool ClampToGround();                                                                                                                                                                                                                 // 83
		virtual bool DetachHavok(NiAVObject* a_obj3D);                                                                                                                                                                                                // 84
		virtual void InitHavok();                                                                                                                                                                                                                     // 85
		virtual NiAVObject* Load3D(bool a_backgroundLoading);                                                                                                                                                                                         // 86
		virtual void Release3DRelatedData();                                                                                                                                                                                                          // 87
		virtual void Set3D(NiAVObject* a_object, bool a_queue3DTasks);                                                                                                                                                                                // 88
		virtual bool ShouldBackgroundClone() const;                                                                                                                                                                                                   // 89
		virtual bool IsReadyForAttach([[maybe_unused]] const IO_TASK_PRIORITY& a_priority) const { return true; }                                                                                                                                     // 8A
		virtual NiAVObject* Get3D([[maybe_unused]] bool a_firstPerson) const { return Get3D(); }                                                                                                                                                      // 8B
		virtual NiAVObject* Get3D() const;                                                                                                                                                                                                            // 8C
		virtual NiAVObject* GetFullyLoaded3D() const;                                                                                                                                                                                                 // 8D
		virtual bool Is3rdPersonVisible() const { return true; }                                                                                                                                                                                      // 8E
		virtual bool PopulateGraphProjectsToLoad(const NiAVObject* a_obj3D, BSScrapArray<BSStaticStringT<260>>& a_projectFilenames) const;                                                                                                            // 8F
		virtual TESModel* GetTESModel() const;                                                                                                                                                                                                        // 90
		virtual TESRace* GetVisualsRace() const;                                                                                                                                                                                                      // 91
		virtual void GetPhysicsTransform(NiTransform& a_outTransform) const;                                                                                                                                                                          // 92
		virtual NiPoint3 GetBoundMin() const;                                                                                                                                                                                                         // 93
		virtual NiPoint3 GetBoundMax() const;                                                                                                                                                                                                         // 94
		virtual bool GetBehaviorRootNameOverrides([[maybe_unused]] BSScrapArray<BSFixedString>& a_behaviorRoots) const { return false; }                                                                                                              // 95
		virtual bool GetBehaviorRootNameReplacement([[maybe_unused]] BSFixedString& a_rootNameReplacement) const { return false; }                                                                                                                    // 96
		virtual bool GetBehaviorRootData(bool a_onlyCollectActiveRoots, BSScrapArray<BGSBehaviorRootData>& a_behaviorRoots) const;                                                                                                                    // 97
		virtual void OnHeadInitialized() { return; }                                                                                                                                                                                                  // 98
		virtual bool InitNonNPCAnimation(NiNode& a_nodeForAnim);                                                                                                                                                                                      // 99
		virtual bool CheckAndFixSkinAndBoneOrder(NiNode& a_nodeToTest);                                                                                                                                                                               // 9A
		virtual void ModifyAnimationUpdateData([[maybe_unused]] BSAnimationUpdateData& a_updateData) { return; }                                                                                                                                      // 9B
		virtual bool ShouldSaveAnimationOnUnloading() const;                                                                                                                                                                                          // 9C
		virtual bool ShouldSaveAnimationOnSaving() const;                                                                                                                                                                                             // 9D
		virtual bool ShouldPerformRevert() const { return true; }                                                                                                                                                                                     // 9E
		virtual void UpdateAnimation(float a_delta);                                                                                                                                                                                                  // 9F
		virtual void CollectPickNodes() { return; }                                                                                                                                                                                                   // A0
		virtual const BSTSmartPointer<BipedAnim>& GetBiped() const;                                                                                                                                                                                   // A1
		virtual const BSTSmartPointer<BipedAnim>& GetBiped(bool a_firstPerson) const;                                                                                                                                                                 // A2
		virtual const BSTSmartPointer<BipedAnim>& GetCurrentBiped() const { return GetBiped(); }                                                                                                                                                      // A3
		virtual void SetBiped([[maybe_unused]] const BSTSmartPointer<BipedAnim>& a_biped) { return; }                                                                                                                                                 // A4 
		virtual void AttachWeapon(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, BGSEquipIndex a_equipIndex);                                                                                                                                     // A4
		virtual void RemoveWeapon([[maybe_unused]] const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, [[maybe_unused]] BGSEquipIndex a_equipIndex, [[maybe_unused]] bool a_queue3DTasks) { return; }                                                  // A5
		virtual void CreateInventoryList(const TESContainer* a_container);                                                                                                                                                                            // A6
		virtual void DestroyInventoryList();                                                                                                                                                                                                          // A7
		virtual void SetRunsInLow(bool) { return; }                                                                                                                                                                                                   // A8
		virtual void SetObjectReference(TESBoundObject* a_object);                                                                                                                                                                                    // A9
		virtual void MoveHavok(bool a_forceRec);                                                                                                                                                                                                      // AA
		virtual void GetLinearVelocity(NiPoint3& a_velocity) const;                                                                                                                                                                                   // AB
		virtual void ApplyMovementDelta(float a_timeDelta, const NiPoint3A& a_delta, const NiPoint3& a_angleDelta);                                                                                                                                   // AC
		virtual void SetActionComplete([[maybe_unused]] bool a_value) { return; }                                                                                                                                                                     // AD
		virtual void SetMovementComplete(bool) { return; }                                                                                                                                                                                            // AF
		virtual void Disable();                                                                                                                                                                                                                       // B0
		virtual void ResetInventory(bool a_leveledOnly);                                                                                                                                                                                              // B1
		virtual NiAVObject* GetCurrent3D() const { return Get3D(); }                                                                                                                                                                                  // B2
		virtual const Explosion* IsExplosion() const { return nullptr; }                                                                                                                                                                              // B3
		virtual Explosion* IsExplosion() { return nullptr; }                                                                                                                                                                                          // B4
		virtual const Projectile* IsProjectile() const { return nullptr; }                                                                                                                                                                            // B5
		virtual Projectile* IsProjectile() { return nullptr; }                                                                                                                                                                                        // B6
		virtual bool OnAddCellPerformQueueReference([[maybe_unused]] TESObjectCELL& a_cell) const { return true; }                                                                                                                                    // B7
		virtual void DoMoveToHigh() { return; }                                                                                                                                                                                                       // B8
		virtual void TryMoveToMiddleLow() { return; }                                                                                                                                                                                                 // B9
		virtual bool TryChangeSkyCellActorsProcessLevel() { return false; }                                                                                                                                                                           // BA
		virtual void SetDisplayGeometry(bool) { return; }                                                                                                                                                                                             // BB
		virtual void TryUpdateActorLastSeenTime() { return; }                                                                                                                                                                                         // BC
		virtual void SaveGameTest();                                                                                                                                                                                                                  // BD
		virtual TESObjectCELL* GetSaveParentCell() const;                                                                                                                                                                                             // BE
		virtual void SetParentCell(TESObjectCELL* a_cell);                                                                                                                                                                                            // BF
		virtual bool IsDead(bool a_notEssential) const;                                                                                                                                                                                               // C0
		virtual bool ProcessInWater(hknpBodyId a_bodyID, float a_waterHeight, float a_deltaTime);                                                                                                                                                     // C1
		virtual bool ApplyCurrent([[maybe_unused]] float a_deltaTime, [[maybe_unused]] const hkVector4f& a_linVel, const hkVector4f&) { return false; }                                                                                               // C2
		virtual BGSDecalGroup* GetDecalGroup() const;                                                                                                                                                                                                 // C3
		virtual void InitDefaultWornImpl(bool a_weapon, bool a_allowChanges);                                                                                                                                                                         // C4
		virtual bool HasKeywordHelper(const BGSKeyword* a_keyword, const TBO_InstanceData* a_data) const;                                                                                                                                             // C5

		bool ActivateRef(TESObjectREFR* a_actionRef, TESBoundObject* a_objectToGet, std::int32_t a_count, bool a_defaultProcessingOnly, bool a_fromScript, bool a_looping)
		{
			using func_t = decltype(&TESObjectREFR::ActivateRef);
			REL::Relocation<func_t> func{ REL::ID(753531) };
			return func(this, a_actionRef, a_objectToGet, a_count, a_defaultProcessingOnly, a_fromScript, a_looping);
		}

		void AddInventoryItem(TESBoundObject* a_object, BSTSmartPointer<ExtraDataList> a_extra, std::uint32_t a_count, TESObjectREFR* a_oldContainer, const INSTANCE_FILTER* a_filter, TESObjectREFR* a_overrideRef)
		{
			using func_t = decltype(&TESObjectREFR::AddInventoryItem);
			REL::Relocation<func_t> func{ REL::ID(78185) };
			return func(this, a_object, a_extra, a_count, a_oldContainer, a_filter, a_overrideRef);
		}

		void AddLockChange()
		{
			using func_t = decltype(&TESObjectREFR::AddLockChange);
			REL::Relocation<func_t> func{ REL::ID(1578706) };
			return func(this);
		}

		void Enable(bool a_resetInventory)
		{
			using func_t = decltype(&TESObjectREFR::Enable);
			REL::Relocation<func_t> func{ REL::ID(1396707) };
			return func(this, a_resetInventory);
		}

		void SetNoCollision(bool a_noCollision)
		{
			using func_t = decltype(&TESObjectREFR::SetNoCollision);
			REL::Relocation<func_t> func{ REL::ID(660095) };
			return func(this, a_noCollision);
		}

		bool GetNoCollision()
		{
			using func_t = decltype(&TESObjectREFR::GetNoCollision);
			REL::Relocation<func_t> func{ REL::ID(393708) };
			return func(this);
		}

		void UpdateReference3D()
		{
			using func_t = decltype(&TESObjectREFR::UpdateReference3D);
			REL::Relocation<func_t> func{ REL::ID(1568075) };
			return func(this);
		}

		void DisableInputForPlayer(const char* a_layerName, uint32_t a_inputToDisable)
		{
			using func_t = decltype(&TESObjectREFR::DisableInputForPlayer);
			REL::Relocation<func_t> func{ REL::ID(53346) };
			return func(this, a_layerName, a_inputToDisable);
		}

		void ReenableInputForPlayer()
		{
			using func_t = decltype(&TESObjectREFR::ReenableInputForPlayer);
			REL::Relocation<func_t> func{ REL::ID(478350) };
			return func(this);
		}

		void ModifyKeyword(const BGSKeyword* a_kw, bool a_addTrueRemoveFalse)
		{
			using func_t = decltype(&TESObjectREFR::ModifyKeyword);
			REL::Relocation<func_t> func{ REL::ID(1504561) };
			return func(this, a_kw, a_addTrueRemoveFalse);
		}

		RE::TESWorldSpace* GetWorldSpace() const
		{
			using func_t = decltype(&TESObjectREFR::GetWorldSpace);
			REL::Relocation<func_t> func{ REL::ID(576133) };
			return func(this);
		}

		void FindAndWriteStackDataForInventoryItem(
			TESBoundObject* a_object,
			BGSInventoryItem::StackDataCompareFunctor& a_compareFunc,
			BGSInventoryItem::StackDataWriteFunctor& a_writeFunc,
			bool (*a_objCompFn)(TESBoundObject*, TESBoundObject*) = BGSInventoryList::StandardObjectCompareCallbackFn,
			bool a_alwaysContinue = false)
		{
			if (inventoryList) {
				inventoryList->FindAndWriteStackDataForItem(a_object, a_compareFunc, a_writeFunc, a_objCompFn, a_alwaysContinue);
			}
		}

		[[nodiscard]] TESBoundObject* GetObjectReference() const noexcept { return data.objectReference; }

		[[nodiscard]] TESForm* GetOwner()
		{
			using func_t = decltype(&TESObjectREFR::GetOwner);
			REL::Relocation<func_t> func{ REL::ID(1323437) };
			return func(this);
		}

		[[nodiscard]] TESObjectCELL* GetParentCell() const noexcept { return parentCell; }

		[[nodiscard]] NiPoint3 GetPosition() const noexcept { return data.location; }

		[[nodiscard]] float GetPositionX() const noexcept { return data.location.x; }

		[[nodiscard]] float GetPositionY() const noexcept { return data.location.y; }

		[[nodiscard]] float GetPositionZ() const noexcept { return data.location.z; }

		[[nodiscard]] std::int64_t GetGoldAmount()
		{
			using func_t = decltype(&TESObjectREFR::GetGoldAmount);
			REL::Relocation<func_t> func{ REL::ID(564356) };
			return func(this);
		}

		[[nodiscard]] ObjectRefHandle GetHandle() const
		{
			using func_t = decltype(&TESObjectREFR::GetHandle);
			REL::Relocation<func_t> func{ REL::ID(1573130) };
			return func(this);
		}

		[[nodiscard]] std::uint32_t GetInventoryObjectCount(const TESBoundObject* a_object)
		{
			if (!a_object || !inventoryList) {
				return 0;
			}
			std::uint32_t count = 0;
			for (const auto& item : inventoryList->data) {
				if (item.object == a_object) {
					for (auto* stack = item.stackData.get(); stack; stack = stack->nextStack.get()) {
						count += static_cast<std::uint32_t>(stack->count);
					}
				}
			}
			return count;
		}

		[[nodiscard]] TESObjectREFR* GetLinkedRef(BGSKeyword* a_keyword)
		{
			using func_t = decltype(&TESObjectREFR::GetLinkedRef);
			REL::Relocation<func_t> func{ REL::ID(897287) };
			return func(this, a_keyword);
		}

		[[nodiscard]] REFR_LOCK* GetLock()
		{
			using func_t = decltype(&TESObjectREFR::GetLock);
			REL::Relocation<func_t> func{ REL::ID(930785) };
			return func(this);
		}

		[[nodiscard]] float GetWeightInContainer()
		{
			using func_t = decltype(&TESObjectREFR::GetWeightInContainer);
			REL::Relocation<func_t> func{ REL::ID(1377567) };
			return func(this);
		}

		[[nodiscard]] bool IsCrimeToActivate()
		{
			using func_t = decltype(&TESObjectREFR::IsCrimeToActivate);
			REL::Relocation<func_t> func{ REL::ID(836011) };
			return func(this);
		}

		void MoveRefToNewSpace(TESObjectCELL* a_interior, TESWorldSpace* a_world)
		{
			using func_t = decltype(&TESObjectREFR::MoveRefToNewSpace);
			REL::Relocation<func_t> func{ REL::ID(629658) };
			return func(this, a_interior, a_world);
		}

		void SetAngleOnReference(const NiPoint3& a_point)
		{
			using func_t = decltype(&TESObjectREFR::SetAngleOnReference);
			REL::Relocation<func_t> func{ REL::ID(1049748) };
			return func(this, a_point);
		}

		void SetLinkedRef(const TESObjectREFR* a_refr, BGSKeyword* a_keyword)
		{
			using func_t = decltype(&TESObjectREFR::SetLinkedRef);
			REL::Relocation<func_t> func{ REL::ID(192840) };
			return func(this, a_refr, a_keyword);
		}

		void SetLocationOnReference(const NiPoint3& a_point)
		{
			using func_t = decltype(&TESObjectREFR::SetLocationOnReference);
			REL::Relocation<func_t> func{ REL::ID(1101832) };
			return func(this, a_point);
		}

		void SetWantsDelete(bool a_delete)
		{
			using func_t = decltype(&TESObjectREFR::SetWantsDelete);
			REL::Relocation<func_t> func{ REL::ID(761346) };
			return func(this, a_delete);
		}

		void SetScale(float a_scale)
		{
			refScale = static_cast<std::uint16_t>(a_scale * 100.0f);
			if (auto node = Get3D()) {
				node->local.scale = a_scale;
			}
		}

		float GetScale() const
		{
			return static_cast<float>(refScale) / 100.0f;
		}

		void UpdateParentCell()
		{
			using func_t = decltype(&TESObjectREFR::UpdateParentCell);
			REL::Relocation<func_t> func{ REL::ID(1080481) };
			return func(this);
		}

		const char* GetBaseFullName()
		{
			using func_t = decltype(&TESObjectREFR::GetBaseFullName);
			REL::Relocation<func_t> func{ REL::ID(1502668) };
			return func(this);
		}

		const char* GetDisplayFullName()
		{
			using func_t = decltype(&TESObjectREFR::GetDisplayFullName);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1212056, 2201126) };
			return func(this);
		}

		NiAVObject* GetLookAtNode()
		{
			using func_t = decltype(&TESObjectREFR::GetLookAtNode);
			REL::Relocation<func_t> func{ REL::ID(682158) };
			return func(this);
		}

		bool IsFurniture()
		{
			using func_t = decltype(&TESObjectREFR::IsFurniture);
			REL::Relocation<func_t> func{ REL::ID(527147) };
			return func(this);
		}

		void FreeUpAllMarkers()
		{
			using func_t = decltype(&TESObjectREFR::FreeUpAllMarkers);
			REL::Relocation<func_t> func{ REL::ID(1324833) };
			return func(this);
		}

		bool WornHasKeyword(const BGSKeyword* keyword) const {
			using func_t = decltype(&TESObjectREFR::WornHasKeyword);
			REL::Relocation<func_t> func{ REL::Offset(0x400050) };
			return func(this, keyword);
		}

		//Panda GetParentCell()
		/*bool WaitFor3DLoad() const
		{
			if (!this)
				return false;

			if (!GetParentCell() || !GetParentCell()->IsCellLoaded()) {
				return false;
			}

			if (!this->Get3D())
				return false;


		}*/

		// members
		TESObjectCELL* parentCell;                 // 0B8
		OBJ_REFR data;                             // 0C0
		LOADED_REF_DATA* loadedData;               // 0F0
		BGSInventoryList* inventoryList;           // 0F8
		BSTSmartPointer<ExtraDataList> extraList;  // 100
		std::uint16_t refScale;                    // 018
		std::int8_t modelState;                    // 10A
		bool predestroyed;                         // 10B
	};
	static_assert(sizeof(TESObjectREFR) == 0x110);

	class __declspec(novtable) Explosion :
		public TESObjectREFR  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::Explosion };
		static constexpr auto VTABLE{ VTABLE::Explosion };
		static constexpr auto FORM_ID{ ENUM_FORM_ID::kREFR };

		struct ExplodedLimb
		{
		public:
			// members
			stl::enumeration<BGSBodyPartDefs::LIMB_ENUM, std::int32_t> limb;  // 00
			ActorValueInfo* limbCondition;                                    // 08
			float distance;                                                   // 10
		};
		static_assert(sizeof(ExplodedLimb) == 0x18);

		class ExplosionTarget
		{
		public:
			// members
			ObjectRefHandle ref;               // 00
			std::uint32_t flags;               // 04
			float hitFromExplosionSqrLen;      // 08
			BSTArray<ExplodedLimb> limbArray;  // 10
		};
		static_assert(sizeof(ExplosionTarget) == 0x28);

		// add
		virtual void Initialize();           // C6
		virtual void Update(float a_delta);  // C7
		virtual void FindTargets();          // C8

		// members
		hknpClosestUniqueBodyIdHitCollector collector;   // 110
		void* explosionDBHandle;                         // 520 - TODO
		float age;                                       // 528
		float obj3Dlifetime;                             // 52C
		float audioLifetime;                             // 530
		float buildTime;                                 // 534
		float innerRadius;                               // 538
		float outerRadius;                               // 53C
		float imageSpaceRadius;                          // 540
		float damageMult;                                // 544
		BSSimpleList<ExplosionTarget*> targetList;       // 548
		BSTSmallArray<BSSoundHandle, 4> soundHandles;    // 558
		NiPointer<NiLight> light;                        // 588
		ObjectRefHandle owner;                           // 590
		ObjectRefHandle explodedRef;                     // 594
		ObjectRefHandle createdRef;                      // 598
		BSTSmartPointer<ActorCause> actorCause;          // 5A0
		NonActorMagicCaster* caster;                     // 5A8
		BGSObjectInstanceT<TESObjectWEAP> weaponSource;  // 5B0
		std::int32_t frameCount;                         // 5C0
		NiPoint3A closestPoint;                          // 5D0
		NiPoint3A closestPointNormal;                    // 5E0
		float calculatedDamage;                          // 5F0
		float scale;                                     // 5F4
		std::uint32_t flags;                             // 5F8
	};
	static_assert(sizeof(Explosion) == 0x600);

	class __declspec(novtable) Hazard :
		public TESObjectREFR  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::Hazard };
		static constexpr auto VTABLE{ VTABLE::Hazard };
		static constexpr auto FORM_ID{ ENUM_FORM_ID::kPHZD };

		// members
		void* hazardDBHandle;      // 110 - TODO
		ActorHandle ownerActor;    // 118
		float age;                 // 11C
		float lifetime;            // 120
		float targetTimer;         // 124
		float radius;              // 128
		float magnitude;           // 12C
		BGSHazard* hazard;         // 130
		NiPointer<NiLight> light;  // 138
		BSSoundHandle sound;       // 140
		std::uint32_t flags;       // 148
	};
	static_assert(sizeof(Hazard) == 0x150);
}
