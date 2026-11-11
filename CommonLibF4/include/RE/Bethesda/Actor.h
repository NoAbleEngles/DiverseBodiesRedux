#pragma once

#include "RE/Bethesda/AITimeStamp.h"
#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSLock.h"
#include "RE/Bethesda/BSPointerHandle.h"
#include "RE/Bethesda/BSSoundHandle.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTEvent.h"
#include "RE/Bethesda/BSTSingleton.h"
#include "RE/Bethesda/BSTSmartPointer.h"
#include "RE/Bethesda/BSTTuple.h"
#include "RE/Bethesda/IMovementInterface.h"
#include "RE/Bethesda/TESObjectREFRs.h"
#include "RE/NetImmerse/NiFlags.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "RE/NetImmerse/NiSmartPointer.h"

#include "REL/IDSelection.h"
#include "RE/RTTI_IDs.h"
#include "RE/RTTI_IDs_AE.h"
#include "RE/VTABLE_IDs.h"
#include "RE/VTABLE_IDs_AE.h"

namespace RE
{
	enum class ACTOR_CRITICAL_STAGE;
	enum class ACTOR_LOS_LOCATION;
	enum class ATTACK_STATE_ENUM;
	enum class PACKAGE_OBJECT_TYPE;
	enum class POWER_ATTACK_TYPE;
	enum class RESET_3D_FLAGS : uint16_t;
	enum class SIT_SLEEP_STATE;

	template <class>
	class AnimationStanceFireLocationData;

	template <class>
	class BSPointerHandleSmartPointer;

	template <class, class, class, template <class> class>
	class BSResponse;

	template <class>
	class DoNothingUnhandledPolicy;

	class ActiveEffect;
	class ActorMagicCaster;
	class ActorMover;
	class ActorPackageLoadFormBuffer;
	class AimModel;
	class AIProcess;
	class BGSSaveFormBuffer;
	class bhkCharacterController;
	class bhkCharacterMoveFinishEvent;
	class bhkCharacterStateChangeEvent;
	class bhkNonSupportContactEvent;
	class bhkNPCollisionObject;
	class bhkRagdollPenetrationUtil;
	class BipedAnim;
	class BSCloneReserver;
	class BSClothExtraData;
	class BSFaceGenAnimationData;
	class BSLightingShaderProperty;
	class BSPathingRequest;
	class BSTransformDeltaEvent;
	class CombatController;
	class CombatGroup;
	class EffectItem;
	class EquippedItem;
	class EquippedItemData;
	class HitData;
	class IAnimationStanceData;
	class IProcedureTreeExecState;
	class MovementControllerNPC;
	class MovementMessageActorCollision;
	class MovementMessageNewPath;
	class MovementMessageUpdateRequestImmediate;
	class MuzzleFlash;
	class PackageLocation;
	class PerkEntryVisitor;
	class QueuedFile;
	class QueuedItem;

	struct ActorCPMEvent;
	struct ActorInventoryEvent;
	struct ActorMotionFeedbackData;
	struct ActorMotionFeedbackOutput;
	struct AIPerkData;
	struct BSAnimationGraphVariableCache;
	struct BSMovementDataChangedEvent;
	struct BSSubGraphActivationUpdate;
	struct CastPowerItem;
	struct DeferredHideLimb;
	struct HighProcessData;
	struct ObjectstoAcquire;
	struct Perks;

	enum class ACTOR_LIFE_STATE : std::uint32_t
	{
		kAlive = 0,
		kDying = 1,
		kDead = 2,
		kUnconcious = 3,
		kReanimate = 4,
		kRecycle = 5,
		kRestrained = 6,
		kEssentialDown = 7,
		kBleedout = 8
	};

	namespace ActorEquipManagerEvent
	{
		struct Event
		{
			enum class Type : std::int32_t
			{
				kEquip = 0x0,
				kUnequip = 0x1
			};

			Type changeType;				// 00
			const BGSObjectInstance* item;	// 08
			Actor* actor;					// 10
			std::uint32_t stackID;			// 18
		};
		static_assert(sizeof(Event) == 0x20);
	}

	namespace MagicSystem
	{
		enum class CannotCastReason;
		enum class WardState;
	}

	namespace PerkValueEvents
	{
		struct PerkValueChangedEvent;
		struct PerkEntryUpdatedEvent;
	}

	enum class INTERACTING_STATE : std::uint32_t
	{
		kNotInteracting,
		kWaitingToInteract,
		kInteracting,
		kWaitingToStopInteracting
	};

	enum class PTYPE : std::int32_t
	{
		kExplore = 0,
		kFollow = 1,
		kEscore = 2,
		kEat = 3,
		kSleep = 4,
		kWander = 5,
		kTravel = 6,
		kAccompany = 7,
		kUseItemAt = 8,
		kAmbush = 9,
		kFleeNonCombat = 10,
		kCastMagic = 11,
		kSandbox = 12,
		kPatrol = 13,
		kGuard = 14,
		kDialogue = 15,
		kUseWeapon = 16,
		kFind = 17,
		kCustom = 18,
		kTemplate = 19,
		kActivate = 20,
		kAlarm = 21,
		kFlee = 22,
		kTrespass = 23,
		kSpectator = 24,
		kGreet = 25,
		kGetUp = 26,
		kDoNothing = 27,
		kInGameDialogue = 28,
		kSurface = 29,
		kSearchForAttacker = 30,
		kAvoidReference = 31,
		kBumpReaction = 32,
		kGrenadeMineReaction = 33,
		kStealWarning = 34,
		kPickpocketWarning = 35,
		kMovementBlocked = 36,
		kVampireFeed = 37,
		kCannibal = 38,
		kLand = 39,
		kUnused = 40,
		kMountActor = 41,
		kDismountActor = 42,
		kClearMountPosition = 43,
		kClearPowerArmorExit = 44,
	};

	struct MiddleLowProcessData
	{
	public:
		// members
		std::int32_t hourPackageEvaluated;  // 0
	};
	static_assert(sizeof(MiddleLowProcessData) == 0x4);

	class __declspec(novtable) ActorPackageData
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::ActorPackageData, RTTI_AE::ActorPackageData) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::ActorPackageData, VTABLE_AE::ActorPackageData) };

		virtual ~ActorPackageData();  // 00

		// add
		virtual PTYPE GetPackageType() = 0;                                                                             // 01
		virtual IProcedureTreeExecState* GetProcedureExecState() { return nullptr; }                                    // 02
		virtual void SetProcedureExecState([[maybe_unused]] IProcedureTreeExecState& a_procedureExecState) { return; }  // 03
		virtual void ClearProcedureExecState() { return; }                                                              // 04
		virtual void SaveGame(BGSSaveFormBuffer* a_saveGameBuffer) = 0;                                                 // 05
		virtual void LoadGame(ActorPackageLoadFormBuffer* a_loadGameBuffer) = 0;                                        // 06
		virtual void InitLoadGame([[maybe_unused]] ActorPackageLoadFormBuffer* a_loadGameBuffer) { return; }            // 07
		virtual void ResetPackData([[maybe_unused]] TESPackage* a_pack) { return; }                                     // 08
	};
	static_assert(sizeof(ActorPackageData) == 0x8);

	class ActorPackage
	{
	public:
		// members
		BSSpinLock packageLock;               // 00
		TESPackage* package;                  // 08
		ActorPackageData* data;               // 10
		ObjectRefHandle target;               // 18
		std::int32_t currentProcedureIndex;   // 1C
		float packageStartTime;               // 20
		std::uint32_t modifiedPackageFlag;    // 24
		std::uint16_t modifiedInterruptFlag;  // 28
		std::int8_t actorPackageFlags;        // 2A
		std::int8_t preferredSpeed;           // 2B
	};
	static_assert(sizeof(ActorPackage) == 0x30);

	struct CommandedActorData
	{
	public:
		// members
		ActorHandle commandedActor;  // 00
		ActiveEffect* activeEffect;  // 08
	};
	static_assert(sizeof(CommandedActorData) == 0x10);

	class ActiveEffectList
	{
	public:
		// members
		BSTArray<BSTSmartPointer<ActiveEffect>> data;  // 00
		std::int8_t iterating;                         // 18
	};
	static_assert(sizeof(ActiveEffectList) == 0x20);

	struct SubGraphIdleRootData
	{
	public:
		// members
		SubgraphIdentifier subGraphID;  // 00
		BSFixedString idleRootName;     // 08
		std::int8_t count;              // 10
		std::int8_t activeCount;        // 11
		bool forFirstPerson;            // 12
	};
	static_assert(sizeof(SubGraphIdleRootData) == 0x18);

	class __declspec(novtable) EquippedItemData :
		public NiRefObject  // 00
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::EquippedItemData, RTTI_AE::EquippedItemData) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::EquippedItemData, VTABLE_AE::EquippedItemData) };

		// add
		virtual void SaveGame(BGSSaveFormBuffer* a_saveGameBuffer, const EquippedItem& a_equippedItem) const = 0;  // 02
		virtual void LoadGame(BGSLoadFormBuffer* a_loadGameBuffer, const EquippedItem& a_equippedItem) = 0;        // 03
		virtual std::uint32_t GetObjectType() const = 0;                                                           // 04
	};
	static_assert(sizeof(EquippedItemData) == 0x10);

	class __declspec(novtable) EquippedWeaponData :
		public EquippedItemData  // 00
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::EquippedWeaponData, RTTI_AE::EquippedWeaponData) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::EquippedWeaponData, VTABLE_AE::EquippedWeaponData) };

		// members
		TESAmmo* ammo;                                                                               // 10
		std::uint32_t ammoCount;                                                                     // 18
		AimModel* aimModel;                                                                          // 20
		MuzzleFlash* muzzleFlash;                                                                    // 28
		NiAVObject* fireNode;                                                                        // 30
		ATTACK_STATE_ENUM attackState;                                                               // 38
		BSTArray<BSTTuple<std::uint32_t, AnimationStanceFireLocationData<NiPoint3>>> fireLocations;  // 40
		NiPointer<QueuedFile> weaponPreload;                                                         // 58
		NiPointer<QueuedFile> projectilePreload;                                                     // 60
		NiPointer<BSCloneReserver> reserveProjectileClones;                                          // 68
		BSSoundHandle idleSound;                                                                     // 70
		BSSoundHandle attackSound;                                                                   // 78
		BSSoundHandle reverbSound;                                                                   // 80
		BSSoundHandle prevAttack;                                                                    // 88
		BSSoundHandle prevReverb;                                                                    // 90
		const BGSSoundKeywordMapping* attackSoundData;                                               // 98
		bool reverbSoundIsTail;                                                                      // A0
	};
	static_assert(sizeof(EquippedWeaponData) == 0xA8);

	class EquippedItem
	{
	public:
		// members
		BGSObjectInstance item;            // 00
		const BGSEquipSlot* equipSlot;     // 10
		BGSEquipIndex equipIndex;          // 18
		NiPointer<EquippedItemData> data;  // 20
	};
	static_assert(sizeof(EquippedItem) == 0x28);

	struct SubgraphHandle
	{
	public:
		// members
		std::uint64_t handle;  // 0
	};
	static_assert(sizeof(SubgraphHandle) == 0x8);

	template <class T>
	class AnimationStanceNodeLocationData
	{
	public:
		// members
		T nodeLocationArray[3];  // 00
	};

	struct MiddleHighProcessData
	{
	public:
		// members
		BSTEventSource<ActorInventoryEvent> inventoryEventSource;                                                 // 000
		NiPointer<bhkNPCollisionObject> poseBound;                                                                // 058
		ActorPackage runOncePackage;                                                                              // 060
		BSTArray<ActorHandle> deadDetectList;                                                                     // 090
		BSSimpleList<TESObjectREFR*> refListChairBed;                                                             // 0A8
		NiPoint3 rotation;                                                                                        // 0B8
		NiPoint3 rotationSpeed;                                                                                   // 0C4
		NiPoint3 actorMountPosition;                                                                              // 0D0
		NiPoint3 furniturePathPoint;                                                                              // 0DC
		NiPoint3 lastSeenPosition;                                                                                // 0E8
		ActorHandle bleedoutAttacker;                                                                             // 0F4
		stl::enumeration<MagicSystem::WardState, std::int32_t> wardState;                                         // 0F8
		BSTSmartPointer<BSResponse<BSFixedString, Actor, BSFixedString, DoNothingUnhandledPolicy>> animResponse;  // 100
		BSTArray<CommandedActorData> commandedActors;                                                             // 108
		NiNode* damageRootNode[26];                                                                               // 120
		NiNode* weaponBone;                                                                                       // 1F0
		NiAVObject* headNode;                                                                                     // 1F8
		NiAVObject* torsoNode;                                                                                    // 200
		NiAVObject* faceTargetSourceNode;                                                                         // 208
		BSFaceGenNiNode* faceNodeSkinned;                                                                         // 210
		NiPointer<BSLightingShaderProperty> lightingProperty;                                                     // 218
		QueuedItem* listItemstoEquipUnequip;                                                                      // 220
		HitData* lastHitData;                                                                                     // 228
		DeferredHideLimb* headDeferredHideLimb;                                                                   // 230
		ActiveEffectList activeEffects;                                                                           // 238
		BSTSmartPointer<BSAnimationGraphManager> animationGraphManager;                                           // 258
		BSAnimationGraphVariableCache* animationVariableCache;                                                    // 260
		BSTArray<SubGraphIdleRootData> subGraphIdleManagerRoots;                                                  // 268
		BSSpinLock equippedItemsLock;                                                                             // 280
		BSTArray<EquippedItem> equippedItems;                                                                     // 288
		BSTArray<BSClothExtraData*> clothExtraDataCache;                                                          // 2A0
		BSTArray<BSTSmallArray<SubgraphHandle, 2>> subGraphHandles;                                               // 2B8
		BSTSmallArray<SubgraphIdentifier, 2> currentDefaultSubGraphID;                                            // 2D0
		BSTSmallArray<SubgraphIdentifier, 2> requestedDefaultSubGraphID;                                          // 2F0
		BSTSmallArray<SubgraphIdentifier, 2> currentWeaponSubGraphID;                                             // 310
		BSTSmallArray<SubgraphIdentifier, 2> requestedWeaponSubGraphID;                                           // 330
		BSTSmartPointer<IAnimationStanceData> stanceData;                                                         // 350
		BSTArray<BSTTuple<std::uint32_t, AnimationStanceNodeLocationData<NiPoint3>>> nodeLocationArray;           // 358
		float stanceHeightArray[6][2];                                                                            // 370
		float headHeightOffset;                                                                                   // 3A0
		ObjectRefHandle currentFurniture;                                                                         // 3A4
		ObjectRefHandle occupiedFurniture;                                                                        // 3A8
		TESIdleForm* currentIdle;                                                                                 // 3B0
		ActorHandle commandingActor;                                                                              // 3B8
		const TESIdleForm* furnitureIdle;                                                                         // 3C0
		BSFaceGenAnimationData* faceAnimationData;                                                                // 3C8
		MagicItem* currentPackageSpell;                                                                           // 3D0
		TESObjectWEAP* lastBoundWeapon;                                                                           // 3D8
		NiPointer<bhkCharacterController> charController;                                                         // 3E0
		BSTSmartPointer<bhkRagdollPenetrationUtil> penetrationDetectUtil;                                         // 3E8
		NiPointer<QueuedFile> bodyPartPreload;                                                                    // 3F0
		TESIdleForm* lastIdlePlayed;                                                                              // 3F8
		AIPerkData* perkData;                                                                                     // 400
		NiPoint3 lookAtLocation;                                                                                  // 408
		float pursueTimer;                                                                                        // 414
		float furnitureAngle;                                                                                     // 418
		float furnitureEntryFootstepDeltaTarget;                                                                  // 41C
		float packageIdleTimer;                                                                                   // 420
		float equippedWeight;                                                                                     // 424
		float desiredSpeed;                                                                                       // 428
		float animationSpeed;                                                                                     // 42C
		float bleedoutTimer;                                                                                      // 430
		float bleedoutRate;                                                                                       // 434
		float bleedoutMaxHealth;                                                                                  // 438
		float maxWardPower;                                                                                       // 43C
		float animGraphEventTimeout;                                                                              // 440
		float torchEvalTimer;                                                                                     // 444
		float alphaMult;                                                                                          // 448
		float scriptRefractPower;                                                                                 // 44C
		float sleepingTimer;                                                                                      // 450
		float deferredKillTimer;                                                                                  // 454
		float killMoveTimer;                                                                                      // 458
		float staggerTimer;                                                                                       // 45C
		float mountDismountSafetyTimer;                                                                           // 460
		AITimeStamp rangeWaypointIdleTimer;                                                                       // 464
		std::int32_t packageIdleNumber;                                                                           // 468
		std::int32_t reservationSlot;                                                                             // 46C
		SubgraphIdentifier currentFurnitureSubgraphID;                                                            // 470
		std::uint32_t currentFurnitureMarkerID;                                                                   // 478
		std::uint32_t occupiedFurnitureMarkerID;                                                                  // 47C
		std::uint64_t nextExtraArrow3DUpdate;                                                                     // 480
		std::uint32_t deferredKill;                                                                               // 488
		std::uint32_t flareFlags;                                                                                 // 48C
		stl::enumeration<PACKAGE_OBJECT_TYPE, std::int32_t> useItem;                                              // 490
		std::int16_t animActionSuccess;                                                                           // 494
		std::uint16_t update3DModel;                                                                              // 496
		std::int8_t weaponCullCounter;                                                                            // 498
		std::int8_t archetypeChangeType;                                                                          // 499
		bool animWeaponCull;                                                                                      // 49A
		bool aimingTarget;                                                                                        // 49B
		bool doneClothesChange;                                                                                   // 49C
		bool pickPackIdle;                                                                                        // 49D
		bool doneOnce;                                                                                            // 49E
		bool refreshFlareFlags;                                                                                   // 49F
		bool pickPocketed;                                                                                        // 4A0
		bool summonedCreature;                                                                                    // 4A1
		bool forceNextUpdate;                                                                                     // 4A2
		bool playedBeginIdles;                                                                                    // 4A3
		bool playedEndIdles;                                                                                      // 4A4
		bool quickPlacement;                                                                                      // 4A5
		bool beenAttacked;                                                                                        // 4A6
		bool alwaysHit;                                                                                           // 4A7
		bool doNoDamage;                                                                                          // 4A8
		bool soulTrapped;                                                                                         // 4A9
		bool lookAt;                                                                                              // 4AA
		bool eating;                                                                                              // 4AB
		bool calcLight;                                                                                           // 4AC
		bool preventCombat;                                                                                       // 4AD
		bool dyingFromBleedout;                                                                                   // 4AE
		bool fleeing;                                                                                             // 4AF
		bool instantGetOut;                                                                                       // 4B0
		bool hostileGuard;                                                                                        // 4B1
		bool stopIdleFailed;                                                                                      // 4B2
		bool killQueued;                                                                                          // 4B3
		bool ragdollInstant;                                                                                      // 4B4
		bool scriptDeferredKill;                                                                                  // 4B5
		bool furnitureEntryLeftFootFirst;                                                                         // 4B6
		bool furnitureAnimationPlayed;                                                                            // 4B7
		bool queuedInstantInteractionAnimation;                                                                   // 4B8
		bool queuedModifyInitialAnimationPose;                                                                    // 4B9
		bool avoidPlayer;                                                                                         // 4BA
		bool usingPathingFaceTargetWhileTrackingOutOfRange;                                                       // 4BB
	};
	static_assert(sizeof(MiddleHighProcessData) == 0x4C0);

	class CachedValueData
	{
	public:
		// members
		float value;  // 0
		bool dirty;   // 4
	};
	static_assert(sizeof(CachedValueData) == 0x8);

	class CachedValues
	{
	public:
		// members
		float cachedRadius;                                  // 00
		float cachedWidth;                                   // 04
		float cachedLength;                                  // 08
		float cachedForwardLength;                           // 0C
		float cachedDPS;                                     // 10
		float cachedEyeLevel;                                // 14
		float cachedWalkSpeed;                               // 18
		float cachedRunSpeed;                                // 1C
		float cachedJogSpeed;                                // 20
		float cachedFastWalkSpeed;                           // 24
		std::uint32_t booleanValues;                         // 28
		std::uint32_t flags;                                 // 2C
		BSTArray<CachedValueData> actorValueCache;           // 30
		BSTArray<CachedValueData> permanentActorValueCache;  // 48
	};
	static_assert(sizeof(CachedValues) == 0x60);

	class AITimer
	{
	public:
		float startTime;   // 0
		float targetTime;  // 4
	};
	static_assert(sizeof(AITimer) == 0x8);

	class AIProcess
	{
	public:

		[[nodiscard]] TESAmmo* GetCurrentAmmo(BGSEquipIndex a_equipIndex) const
		{
			using func_t = decltype(&AIProcess::GetCurrentAmmo);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1154936, 2232455) };
			return func(this, a_equipIndex);
		}

		void KnockExplosion(Actor* a_actor, const NiPoint3& a_location, float a_magnitude)
		{
			using func_t = decltype(&AIProcess::KnockExplosion);
			REL::Relocation<func_t> func{ REL::SelectVersionID(533106, 2232384) };
			return func(this, a_actor, a_location, a_magnitude);
		}

		void SetActorsDetectionEvent(Actor* a_actor, const NiPoint3& a_location, std::int32_t a_soundLevel, TESObjectREFR* a_refr)
		{
			using func_t = decltype(&AIProcess::SetActorsDetectionEvent);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1376336, 2231738) };
			return func(this, a_actor, a_location, a_soundLevel, a_refr);
		}

		void SetCurrentAmmo(BGSEquipIndex a_equipIndex, TESAmmo* a_ammo)
		{
			using func_t = decltype(&AIProcess::SetCurrentAmmo);
			REL::Relocation<func_t> func{ REL::SelectVersionID(795983, 2232302) };
			return func(this, a_equipIndex, a_ammo);
		}

		//action should be DEFAULT_OBJECT::kActionIdle (0x35)
		bool PlayIdle(Actor* a_actor, uint32_t a_action, TESIdleForm* a_idle, bool a_checkConditions = true, TESObjectREFR* a_targetRef = 0)
		{
			using func_t = decltype(&AIProcess::PlayIdle);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1446774, 2231704) };
			return func(this, a_actor, a_action, a_idle, a_checkConditions, a_targetRef);
		}

		void StopCurrentIdle(RE::Actor* a, bool flag1 = false, bool flag2 = true)
		{
			using func_t = decltype(&AIProcess::StopCurrentIdle);
			REL::Relocation<func_t> func{ REL::SelectVersionID(434460, 2231705) };
			return func(this, a, flag1, flag2);
		}

		void ToggleDoNotInterruptAnimation()
		{
			using func_t = decltype(&AIProcess::ToggleDoNotInterruptAnimation);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1429771, 2231708) };
			return func(this);
		}

		bool GetDoNotInterruptAnimation()
		{
			using func_t = decltype(&AIProcess::GetDoNotInterruptAnimation);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1164050, 2231709) };
			return func(this);
		}

		bool GetAnimationGraphDelta(NiPoint3& a_pointOut, float& a_floatOut)
		{
			using func_t = decltype(&AIProcess::GetAnimationGraphDelta);
			REL::Relocation<func_t> func{ REL::SelectVersionID(472285, 2232041) };
			return func(this, a_pointOut, a_floatOut);
		}

		void SetAnimationGraphDelta(const NiPoint3& a_point, float a_float)
		{
			using func_t = decltype(&AIProcess::SetAnimationGraphDelta);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1083937, 2232042) };
			return func(this, a_point, a_float);
		}

		void ClearCurrentPackage(RE::Actor* a_actor)
		{
			using func_t = decltype(&AIProcess::ClearCurrentPackage);
			REL::Relocation<func_t> func{ REL::SelectVersionID(241540, 2231582) };
			return func(this, a_actor);
		}

		void ClearCurrentDataForProcess(RE::Actor* a_actor)
		{
			using func_t = decltype(&AIProcess::ClearCurrentDataForProcess);
			REL::Relocation<func_t> func{ REL::SelectVersionID(577581, 2232435) };
			return func(this, a_actor);
		}

		TESObjectREFR* SetTarget(RE::Actor* a_actor, const ObjectRefHandle* a_target) {
			using func_t = decltype(&AIProcess::SetTarget);
			REL::Relocation<func_t> func{ REL::SelectVersionID(376043, 2231565) };
			return func(this, a_actor, a_target);
		}

		//panda 0xE74920
		void Set3DUpdateFlag(RE::RESET_3D_FLAGS flags)
		{
			using func_t = decltype(&AIProcess::Set3DUpdateFlag);
			REL::Relocation<func_t> func{ REL::SelectVersionID(236542, 2232389) };
			return func(this, flags);
		}

		//panda E749A0
		RE::RESET_3D_FLAGS GetAll3DUpdateFlags() const
		{
			using func_t = decltype(&AIProcess::GetAll3DUpdateFlags);
			REL::Relocation<func_t> func{ REL::SelectVersionID(582098, 2232393) };
			return func(this);
		}

		//panda E60860
		void DoUpdate3dModel(Actor* a_actor, RE::RESET_3D_FLAGS flags) {
			using func_t = decltype(&AIProcess::DoUpdate3dModel);
			REL::Relocation<func_t> func{ REL::SelectVersionID(114457, 2232144) };
			return func(this, a_actor, flags);
		}

		//panda 0xE78FB0
		void RefreshClothExtraDataCache(RE::Actor* a_actor) {
			using func_t = decltype(&AIProcess::RefreshClothExtraDataCache);
			REL::Relocation<func_t> func{ REL::SelectVersionID(340568, 2232486) };
			return func(this, a_actor);
		};

		//panda 0xE79140
		void ClearClothExtraDataCache(RE::Actor* a_actor)
		{
			using func_t = decltype(&AIProcess::ClearClothExtraDataCache);
			REL::Relocation<func_t> func{ REL::SelectVersionID(233747, 2232487) };
			return func(this, a_actor);
		};

		//panda 0xE79180 this is just jump to AccessClothExtraDataCache
		RE::BSClothExtraData* GetClothExtraDataCache()
		{
			using func_t = decltype(&AIProcess::GetClothExtraDataCache);
			REL::Relocation<func_t> func{ REL::SelectVersionID(406238, 2232488) };
			return func(this);
		};

		//panda 0xE839E0
		RE::BSClothExtraData* AccessClothExtraDataCache(RE::Actor* a_actor)
		{
			using func_t = decltype(&AIProcess::AccessClothExtraDataCache);
			REL::Relocation<func_t> func{ REL::SelectVersionID(457101, 2232583) };
			return func(this, a_actor);
		};

		//panda 0xE79D20
		void ResetDeferredKillStatus() {
			using func_t = decltype(&AIProcess::ResetDeferredKillStatus);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1318960, 2232523) };
			return func(this);
		}

		//panda 0xE7A830
		void UpdateWeaponCullState(RE::Actor* a_actor) {
			using func_t = decltype(&AIProcess::UpdateWeaponCullState);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1447551, 2232536) };
			return func(this, a_actor);
		}

		//panda 0xE7A870
		void ResetCullWeapons() {
			using func_t = decltype(&AIProcess::ResetCullWeapons);
			REL::Relocation<func_t> func{ REL::SelectVersionID(37789, 2232537) };
			return func(this);
		}

		//panda 0xE28200
		void Update(Actor* actor) {
			using func_t = decltype(&AIProcess::Update);
			REL::Relocation<func_t> func{ REL::SelectVersionID(775148, 2231533) };
			return func(this, actor);
		}

		//panda 0xE73370
		bool AlignToFurnitureAndAddPostAnimSitSleep(Actor* actor) {
			using func_t = decltype(&AIProcess::AlignToFurnitureAndAddPostAnimSitSleep);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1468710, 2232357) };
			return func(this, actor);
		}

		/*
		## Важные детали реализации:

			### GetEquippedItem (BGSEquipSlot):
			- Использует BSSpinLock для потокобезопасности
			- Выполняет двоичный поиск по слотам экипировки
			- Проверяет конфликты слотов через BGSEquipSlot::CheckForSlotConflict
			- Возвращает BGSObjectInstance с ref-counted указателями

			### SetEquippedItem:
			- Создает EquippedWeaponData для оружия (formType == 0x2B)
			- Использует TlsIndex для thread-local storage
			- Выполняет бинарную вставку для поддержания сортировки
			- Массив equippedItems хранится в MiddleHighProcessData+0x288

			### ClearEquippedItem:
			- Использует цикл для удаления всех конфликтующих предметов
			- Для оружия вызывает EquippedWeaponData::ReleaseSounds(false)
			- Отправляет ActorInventoryEvent с type=0

			### NotifyItemEquipped:
			- Устанавливает текущие патроны через SetCurrentAmmo
			- Предзагружает звуки с BSAudioManager::PrecacheDescriptor
			- Вычисляет расстояние до слушателя через BSAudioManager::GetListenerDistance
			- Использует приоритеты 0x11 (атака) и 0x12 (3D звук атаки)

			## Структура данных:

			// MiddleHighProcessData+0x280: BSSpinLock equippedItemsLock
			// MiddleHighProcessData+0x288: BSTArray<EquippedItem> equippedItems

			struct EquippedItem {
				BGSObjectInstance item;            // 00
				const BGSEquipSlot* equipSlot;     // 10
				BGSEquipIndex equipIndex;          // 18
				NiPointer<EquippedItemData> data;  // 20
			};

			class EquippedWeaponData : public EquippedItemData {
				TESAmmo* ammo;                     // 10
				uint32_t ammoCount;                // 18
				AimModel* aimModel;                // 20
				MuzzleFlash* muzzleFlash;          // 28
				NiAVObject* fireNode;              // 30
				ATTACK_STATE_ENUM attackState;     // 38
				// ... звуковые хендлы
			};

			## Использование:

			// Получить оружие в правой руке
			EquippedItem rightHandItem;
			if (process->GetEquippedItem(BGSEquipIndex::kRight, &rightHandItem)) {
				// rightHandItem содержит экипированное оружие
			}

			// Получить все предметы в определенном слоте
			BSScrapArray<EquippedItem> items;
			process->GetItemsInSlot(slot, items);

			// Экипировать предмет
			BGSObjectInstance instance;
			process->SetEquippedItem(actor, instance, slot);

			// Снять предмет
			process->ClearEquippedItem(actor, form, slot);
		*/

		// Возвращает экипированный предмет по слоту экипировки
		// Использует двоичный поиск по отсортированному массиву экипированных предметов
		BGSObjectInstance* GetEquippedItemSlot(const BGSEquipSlot* a_slot, BGSObjectInstance* a_outInstance)
		{
			using func_t = decltype(&AIProcess::GetEquippedItemSlot);
			REL::Relocation<func_t> func{ REL::SelectVersionID(111648, 2231624) };
			return func(this, a_slot, a_outInstance);
		}

		// Возвращает экипированный предмет по индексу экипировки (левая/правая рука)
		// Линейный поиск по массиву equippedItems
		// Возвращает true если предмет найден
		bool GetEquippedItemIndex(BGSEquipIndex a_equipIndex, EquippedItem* a_outItem)
		{
			using func_t = decltype(&AIProcess::GetEquippedItemIndex);
			REL::Relocation<func_t> func{ REL::SelectVersionID(4042, 2231625) };
			return func(this, a_equipIndex, a_outItem);
		}

		// Возвращает все предметы, экипированные в конфликтующих слотах
		// Используется для определения, какие предметы нужно снять при экипировке нового
		// a_outItems - выходной массив BSScrapArray<EquippedItem>
		void GetItemsInSlot(const BGSEquipSlot* a_slot, BSScrapArray<EquippedItem>& a_outItems)
		{
			using func_t = decltype(&AIProcess::GetItemsInSlot);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1573363, 2231626) };
			return func(this, a_slot, a_outItems);
		}

		// Экипирует предмет на актора
		// Создает EquippedWeaponData для оружия (TESObjectWEAP)
		// Выполняет бинарную вставку в отсортированный массив equippedItems
		// Вызывает NotifyItemEquipped для уведомления системы
		void SetEquippedItem(Actor* a_actor, const BGSObjectInstance& a_object, const BGSEquipSlot* a_slot)
		{
			using func_t = decltype(&AIProcess::SetEquippedItem);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1200276, 2231627) };
			return func(this, a_actor, a_object, a_slot);
		}

		// Снимает экипированный предмет
		// Освобождает звуки оружия через EquippedWeaponData::ReleaseSounds
		// Удаляет из массива equippedItems
		// Отправляет ActorInventoryEvent
		void ClearEquippedItem(Actor* a_actor, TESForm* a_form, const BGSEquipSlot* a_slot)
		{
			using func_t = decltype(&AIProcess::ClearEquippedItem);
			REL::Relocation<func_t> func{ REL::SelectVersionID(28721, 2231628) };
			return func(this, a_actor, a_form, a_slot);
		}

		// Снимает все экипированные предметы
		// Освобождает все звуки оружия
		// Очищает массив equippedItems
		// Отправляет ActorInventoryEvent для каждого оружия
		void ClearEquippedItems(Actor* a_actor)
		{
			using func_t = decltype(&AIProcess::ClearEquippedItems);
			REL::Relocation<func_t> func{ REL::SelectVersionID(440142, 2231629) };
			return func(this, a_actor);
		}

		// Уведомляет систему о экипировке предмета
		// Предзагружает звуковые дескрипторы для оружия:
		//   - AttackSound (приоритет 0x11)
		//   - Attack3DSound (приоритет 0x12)
		//   - AttackLoopSound (приоритет 0x11)
		// Отправляет ActorInventoryEvent
		void NotifyItemEquipped(Actor* a_actor, const EquippedItem& a_item)
		{
			using func_t = decltype(&AIProcess::NotifyItemEquipped);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1477172, 2231630) };
			return func(this, a_actor, a_item);
		}

		// members
		MiddleLowProcessData* middleLow;                    // 00
		MiddleHighProcessData* middleHigh;                  // 08
		HighProcessData* high;                              // 10
		ActorPackage currentPackage;                        // 18
		float hourLastProcessed;                            // 48
		float timeAdjustmentsMade;                          // 4C
		std::uint32_t dateLastProcessed;                    // 50
		CachedValues* cachedValues;                         // 58
		std::int32_t numberItemsActivate;                   // 60
		BSSimpleList<ObjectstoAcquire*> objectList;         // 68
		BSSimpleList<TESObjectREFR*> genericLocationsList;  // 78
		ObjectstoAcquire* acquireObject;                    // 88
		ObjectstoAcquire* savedAcquireObject;               // 90
		float essentialDownTimer;                           // 98
		float deathTime;                                    // 9C
		float trackedDamage;                                // A0
		TESBoundObject* itemBeingUsed;                      // A8
		AITimer combatDelayTimer;                           // B0
		ObjectRefHandle followTarget;                       // B8
		ObjectRefHandle target;                             // BC
		ObjectRefHandle genericLocation;                    // C0
		ObjectRefHandle genericSecondLocation;              // C4
		BGSListForm* targetFormList;                        // C8
		std::uint32_t currentSpeakingTopicID;               // D0
		ObjectRefHandle patrolLocation;                     // D4
		ObjectRefHandle idleTarget;                         // D8
		std::int16_t formListNumb;                          // DC
		char lowProcessFlags;                               // DE
		char processLevel;                                  // DF
		bool skippedTimeStampForPathing;                    // E0
		bool ignoringCombat;                                // E1
		bool endAlarmOnActor;                               // E2
		bool escortingPlayer;                               // E3
		bool updateEditorDeadModel;                         // E4
		bool editorDeadActive;                              // E5
		bool initializingPosition;                          // E6
	};
	static_assert(sizeof(AIProcess) == 0xE8);

	class __declspec(novtable) MagicTarget
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::MagicTarget, RTTI_AE::MagicTarget) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::MagicTarget, VTABLE_AE::MagicTarget) };

		struct AddTargetData;
		struct SpellDispelData;

		// members
		virtual ~MagicTarget();  // 00

		// add
		virtual bool AddTarget(AddTargetData& a_data);                                                                                                                               // 01
		virtual TESObjectREFR* GetTargetStatsObject() { return nullptr; }                                                                                                            // 02
		virtual bool MagicTargetIsActor() { return false; }                                                                                                                          // 03
		virtual bool IsInvulnerable() const { return false; }                                                                                                                        // 04
		virtual void InvalidateCommandedActorEffect([[maybe_unused]] ActiveEffect* a_effect) { return; }                                                                             // 05
		virtual bool CanAddActiveEffect() const = 0;                                                                                                                                 // 06
		virtual ActiveEffectList* GetActiveEffectList() = 0;                                                                                                                         // 07
		virtual float CheckResistance([[maybe_unused]] MagicItem* a_spell, [[maybe_unused]] EffectItem* a_effect, [[maybe_unused]] TESBoundObject* a_source) const { return 1.0F; }  // 08
		virtual void EffectAdded([[maybe_unused]] ActiveEffect* a_effect) { return; }                                                                                                // 09
		virtual void EffectRemoved([[maybe_unused]] ActiveEffect* a_effect) { return; }                                                                                              // 0A
		virtual void EffectActiveStatusChanged([[maybe_unused]] ActiveEffect* a_effect) { return; }                                                                                  // 0B
		virtual bool CheckAbsorb([[maybe_unused]] Actor* a_caster, [[maybe_unused]] MagicItem* a_spell, [[maybe_unused]] const EffectItem* a_effectItem) { return false; }           // 0C

		[[nodiscard]] bool IsTakingHealthDamageFromActiveEffect()
		{
			using func_t = decltype(&MagicTarget::IsTakingHealthDamageFromActiveEffect);
			REL::Relocation<func_t> func{ REL::SelectVersionID(999442, 2226397) };
			return func(this);
		}

		[[nodiscard]] bool IsTakingRadDamageFromActiveEffect()
		{
			using func_t = decltype(&MagicTarget::IsTakingRadDamageFromActiveEffect);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1079111, 2226398) };
			return func(this);
		}

		// members
		SpellDispelData* postUpdateDispelList;  // 08
		std::int8_t flags;                      // 10
	};
	static_assert(sizeof(MagicTarget) == 0x18);

	enum class WEAPON_STATE : std::uint32_t
	{
		kSheathed,
		kWantToDraw,
		kDrawing,
		kDrawn,
		kWantToSheathe,
		kSheathing
	};

	class __declspec(novtable) ActorState :
		public IMovementState  // 00
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::ActorState, RTTI_AE::ActorState) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::ActorState, VTABLE_AE::ActorState) };

		// add
		virtual bool SetWeaponMagicDrawn(bool a_drawn);                // 21
		virtual bool SetWeaponState(WEAPON_STATE a_state);             // 22
		virtual bool DoSetSitSleepState(SIT_SLEEP_STATE a_state) = 0;  // 23
		virtual SIT_SLEEP_STATE DoGetSitSleepState() const = 0;        // 24
		virtual bool SetInIronSightsImpl(bool a_sighted) = 0;          // 25
		virtual void SetReloadingImpl(bool a_reloading);               // 26

		[[nodiscard]] bool GetWeaponMagicDrawn() const noexcept { return weaponState >= WEAPON_STATE::kDrawn; }

		// members
		std::uint32_t moveMode: 14;             // 08:00
		std::uint32_t flyState: 3;              // 08:14
		ACTOR_LIFE_STATE lifeState: 4;          // 08:17
		std::uint32_t knockState: 4;            // 08:21
		std::uint32_t meleeAttackState: 3;      // 08:25
		std::uint32_t talkingToPlayer: 1;       // 08:28
		std::uint32_t forceRun: 1;              // 08:29
		std::uint32_t forceSneak: 1;            // 08:30
		std::uint32_t headTracking: 1;          // 08:31
		std::uint32_t reanimating: 1;           // 0C:00
		WEAPON_STATE weaponState: 3;            // 0C:01
		std::uint32_t wantBlocking: 1;          // 0C:04
		std::uint32_t flightBlocked: 1;         // 0C:05
		std::uint32_t recoil: 2;                // 0C:06
		std::uint32_t allowFlying: 1;           // 0C:08
		std::uint32_t staggered: 1;             // 0C:09
		std::uint32_t inWrongProcessLevel: 1;   // 0C:10
		std::uint32_t stance: 3;                // 0C:11
		std::uint32_t gunState: 4;              // 0C:14
		INTERACTING_STATE interactingState: 2;  // 0C:18
		std::uint32_t headTrackRotation: 1;     // 0C:20
		std::uint32_t inSyncAnim: 1;            // 0C:21
	};
	static_assert(sizeof(ActorState) == 0x10);

	class __declspec(novtable) IPostAnimationChannelUpdateFunctor
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::IPostAnimationChannelUpdateFunctor, RTTI_AE::IPostAnimationChannelUpdateFunctor) };
		static constexpr auto VTABLE{ REL::SelectVersionVTABLE(VTABLE::IPostAnimationChannelUpdateFunctor, VTABLE_AE::IPostAnimationChannelUpdateFunctor) };

		virtual ~IPostAnimationChannelUpdateFunctor() = default;  // 00

		// add
		virtual void DoPostAnimationChannelUpdate() = 0;  // 01
	};
	static_assert(sizeof(IPostAnimationChannelUpdateFunctor) == 0x8);

	struct Modifiers
	{
	public:
		// members
		float modifiers[3];  // [0] = base value, [1] = relative value, [2] = ???
	};
	static_assert(sizeof(Modifiers) == 0xC);

	class ActorValueStorage
	{
	public:
		// members
		BSTArray<BSTTuple<std::uint32_t, float>> baseValues;     // 00
		BSTArray<BSTTuple<std::uint32_t, Modifiers>> modifiers;  // 18
		BSReadWriteLock avLock;                                  // 30
	};
	static_assert(sizeof(ActorValueStorage) == 0x38);

	class __declspec(novtable) Actor :
		public TESObjectREFR,                                           // 000
		public MagicTarget,                                             // 110
		public ActorState,                                              // 128
		public BSTEventSink<BSMovementDataChangedEvent>,                // 138
		public BSTEventSink<BSTransformDeltaEvent>,                     // 140
		public BSTEventSink<BSSubGraphActivationUpdate>,                // 148
		public BSTEventSink<bhkCharacterMoveFinishEvent>,               // 150
		public BSTEventSink<bhkNonSupportContactEvent>,                 // 158
		public BSTEventSink<bhkCharacterStateChangeEvent>,              // 160
		public IPostAnimationChannelUpdateFunctor,                      // 168
		public BSTEventSource<MovementMessageUpdateRequestImmediate>,   // 170
		public BSTEventSource<PerkValueEvents::PerkValueChangedEvent>,  // 1C8
		public BSTEventSource<PerkValueEvents::PerkEntryUpdatedEvent>,  // 220
		public BSTEventSource<ActorCPMEvent>                            // 278
	{
	public:
		static constexpr auto RTTI{ REL::SelectVersionID(RTTI::Actor, RTTI_AE::Actor) };
		static constexpr auto VTABLE{ VTABLE::Actor };	//Same OG/AE
		static constexpr auto FORM_ID{ ENUM_FORM_ID::kACHR };

		enum class BOOL_FLAGS
		{
			kNone = 0,
			kScenePackage = 1 << 0,
			kIsAMount = 1 << 1,
			kIsMountPointClear = 1 << 2,
			kIsGettingOnOffMount = 1 << 3,
			kInRandomScene = 1 << 4,
			kNoBleedoutRecovery = 1 << 5,
			kInBleedoutAnimation = 1 << 6,
			kCanDoFavor = 1 << 7,
			kShouldAnimGraphUpdate = 1 << 8,
			kCanSpeakToEssentialDown = 1 << 9,
			kBribedByPlayer = 1 << 10,
			kAngryWithPlayer = 1 << 11,
			kIsTresspassing = 1 << 12,
			kCanSpeak = 1 << 13,
			kIsInKillMove = 1 << 14,
			kAttackOnSight = 1 << 15,
			kIsCommandedActor = 1 << 16,
			kForceOneAnimGraphUpdate = 1 << 17,
			kEssential = 1 << 18,
			kProtected = 1 << 19,
			kAttackingDisabled = 1 << 20,
			kCastingDisabled = 1 << 21,
			kSceneHeadtrackRotation = 1 << 22,
			kForceIncMinBoneUpdate = 1 << 23,
			kCrimeSearch = 1 << 24,
			kMovingIntoLoadedArea = 1 << 25,
			kDoNotShowOnStealthMeter = 1 << 26,
			kMovementBlocked = 1 << 27,
			kAllowInstantFurniturePopInPlayerCell = 1 << 28,
			kForceAnimGraphUpdate = 1 << 29,
			kCheckAddEffectDualCast = 1 << 30,
			kUnderwater = 1 << 31,
		};

		// add
		virtual void PlayPickUpSound(TESBoundObject* a_boundObj, bool a_pickUp, bool a_use);                                                                                                             // 0C6
		virtual float GetHeading() const { return data.angle.z; }                                                                                                                                        // 0C7
		virtual void SetAvoidanceDisabled([[maybe_unused]] bool a_tf) { return; }                                                                                                                        // 0C8
		virtual void DrawWeaponMagicHands(bool a_draw);                                                                                                                                                  // 0C9
		virtual void SetPosition(const NiPoint3& a_pos, bool a_updateCharController);                                                                                                                    // 0CA
		virtual void KillDying();                                                                                                                                                                        // 0CB
		virtual void Resurrect(bool a_resetInventory, bool a_attach3D);                                                                                                                                  // 0CC
		virtual bool PutActorInChairBedQuick(TESObjectREFR* a_furniture, std::uint32_t a_index);                                                                                                         // 0CD
		virtual bool PutActorOnMountQuick();                                                                                                                                                             // 0CE
		virtual void Update(float a_delta);                                                                                                                                                              // 0CF
		virtual void UpdateNoAI(float);                                                                                                                                                                  // 0D0
		virtual void UpdateMotionDrivenState();                                                                                                                                                          // 0D1
		virtual void UpdateCharacterControllerSimulationSettings(bhkCharacterController& a_charController);                                                                                              // 0D2
		virtual void PotentiallyFixRagdollState();                                                                                                                                                       // 0D3
		virtual void UpdateNonRenderSafe(float);                                                                                                                                                         // 0D4
		virtual bool ShouldHandleEquipNow() const;                                                                                                                                                       // 0D5
		virtual void SetCrimeGoldValue(TESFaction* a_faction, bool a_violent, std::uint32_t a_value);                                                                                                    // 0D6
		virtual std::uint32_t ModCrimeGoldValue(TESFaction* a_faction, bool a_violent, std::int32_t a_value);                                                                                            // 0D7
		virtual void RemoveCrimeGoldValue(TESFaction* a_faction, bool a_violent, std::int32_t a_value);                                                                                                  // 0D8
		virtual std::uint32_t GetCrimeGoldValue(const TESFaction* a_faction) const;                                                                                                                      // 0D9
		virtual void GoToPrison([[maybe_unused]] TESFaction* a_faction, [[maybe_unused]] bool a_removeItems, [[maybe_unused]] bool a_realJail) { return; }                                               // 0DA
		virtual void ServePrisonTime() { return; }                                                                                                                                                       // 0DB
		virtual void PayFine([[maybe_unused]] TESFaction* a_faction, [[maybe_unused]] bool a_movetoMarker, [[maybe_unused]] bool a_removeStolenItems) { return; }                                        // 0DC
		virtual float GetCurrentEyeLevel() const;                                                                                                                                                        // 0DD
		virtual void SetInDialoguewithPlayer(bool a_flag);                                                                                                                                               // 0DE
		virtual bool GetCannibal() { return false; }                                                                                                                                                     // 0DF
		virtual void SetCannibal(bool) { return; }                                                                                                                                                       // 0E0
		virtual bool GetVampireFeed() { return false; }                                                                                                                                                  // 0E1
		virtual void SetVampireFeed(bool) { return; }                                                                                                                                                    // 0E2
		virtual void InitiateVampireFeedPackage(Actor* a_target, TESObjectREFR* a_furniture);                                                                                                            // 0E3
		virtual void InitiateCannibalPackage(Actor* a_target);                                                                                                                                           // 0E4
		virtual float GetEyeHeading() const;                                                                                                                                                             // 0E5
		virtual void GetEyeAngles(float& a_eyeHeading, float& a_eyeLooking) const;                                                                                                                       // 0E6
		virtual void GetEyeVector(NiPoint3& a_origin, NiPoint3& a_direction, bool a_includeCameraOffset) const;                                                                                          // 0E7
		virtual void SetRefraction(bool a_enable, float a_refractionPower);                                                                                                                              // 0E8
		virtual float GetAcrobatics() const { return 1.0F; }                                                                                                                                             // 0E9
		virtual bool Get3DUpdateFlag(RESET_3D_FLAGS a_flag) const;                                                                                                                                       // 0EA
		virtual ObjectRefHandle DropObject(const BGSObjectInstance& a_object, BSTSmallArray<std::uint32_t, 4>* a_stackData, std::int32_t a_number, const NiPoint3* a_point, const NiPoint3* a_rotate);   // 0EB
		virtual void PickUpObject(TESObjectREFR* a_objREFR, std::int32_t a_count, bool a_playPickUpSounds);                                                                                              // 0EC
		virtual void AttachArrow(const BSTSmartPointer<BipedAnim>& a_biped, BGSEquipIndex a_equipIndex);                                                                                                 // 0ED
		virtual void DetachArrow(const BSTSmartPointer<BipedAnim>& a_biped, BGSEquipIndex a_equipIndex);                                                                                                 // 0EE
		virtual bool ReloadWeapon(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, BGSEquipIndex a_equipIndex);                                                                                        // 0EF
		virtual std::uint32_t UseAmmo(const BGSObjectInstanceT<TESObjectWEAP>& a_weapon, BGSEquipIndex a_equipIndex, std::uint32_t a_shotCount);                                                         // 0F0
		virtual bool CalculateCachedOwnerIsInCombatantFaction() const;                                                                                                                                   // 0F1
		virtual CombatGroup* GetCombatGroup() const;                                                                                                                                                     // 0F2
		virtual void SetCombatGroup(CombatGroup* a_group);                                                                                                                                               // 0F3
		virtual bool CheckValidTarget(TESObjectREFR& a_ref) const;                                                                                                                                       // 0F4
		virtual bool InitiateDialogue(Actor* a_target, PackageLocation* a_packLoc, PackageLocation* a_packSecondLoc);                                                                                    // 0F5
		virtual void EndDialogue();                                                                                                                                                                      // 0F6
		virtual Actor* SetUpTalkingActivatorActor(Actor* a_target, Actor*& a_talkingactivator);                                                                                                          // 0F7
		virtual void InitiateFlee(TESObjectREFR* a_fleeRef, bool a_runonce, bool a_knows, bool a_combatMode, TESObjectCELL* a_cell, TESObjectREFR* a_ref, float a_fleeFromDist, float a_fleeToDist);     // 0F8
		virtual void InitiateGetUpPackage();                                                                                                                                                             // 0F9
		virtual void PutCreatedPackage(TESPackage* a_pack, bool a_tempPack, bool a_isACreatedPackage, bool a_allowFromFurniture);                                                                        // 0FA
		virtual void UpdateAlpha();                                                                                                                                                                      // 0FB
		virtual void SetAlpha(float a_newAlpha);                                                                                                                                                         // 0FC
		virtual float GetAlpha();                                                                                                                                                                        // 0FD
		virtual bool IsInCombat() const;                                                                                                                                                                 // 0FE
		virtual void UpdateCombat();                                                                                                                                                                     // 0FF
		virtual void StopCombat();                                                                                                                                                                       // 100
		virtual bool GetUsesAttackPercents() { return false; }                                                                                                                                           // 101
		virtual std::uint8_t GetPowerAttackPercent(POWER_ATTACK_TYPE) { return 0; }                                                                                                                      // 102
		virtual void WeaponSwingCallBack(BGSEquipIndex a_equipIndex);                                                                                                                                    // 103
		virtual void SetActorStartingPosition();                                                                                                                                                         // 104
		virtual void SetLifeState(ACTOR_LIFE_STATE a_lifeState);                                                                                                                                         // 105
		virtual bool HasBeenAttacked();                                                                                                                                                                  // 106
		virtual void SetBeenAttacked(bool a_flag);                                                                                                                                                       // 107
		virtual void UseSkill(ActorValueInfo*, float, TESForm*) { return; }                                                                                                                              // 108
		virtual bool IsAtPoint(const NiPoint3& a_point, float a_radius, bool a_expandRadius, bool a_alwaysTestHeight);                                                                                   // 109
		virtual bool IsInFaction(const TESFaction* a_faction) const;                                                                                                                                     // 10A
		virtual bool HasPerkEntries(std::uint8_t a_entryPoint) const;                                                                                                                                    // 10B
		virtual void ForEachPerkEntry(std::uint8_t a_entryPoint, PerkEntryVisitor& a_visitor) const;                                                                                                     // 10C
		virtual void ApplyPerksFromBase();                                                                                                                                                               // 10D
		virtual void StartPowerAttackCoolDown() { return; }                                                                                                                                              // 10E
		virtual bool IsPowerAttackCoolingDown() const { return false; }                                                                                                                                  // 10F
		virtual void HandleHealthDamage(Actor* a_attacker, float a_damage);                                                                                                                              // 110
		virtual bool QSpeakingDone() const;                                                                                                                                                              // 111
		virtual void SetSpeakingDone(bool a_done);                                                                                                                                                       // 112
		virtual NiPoint3 CalculateLOSLocation(ACTOR_LOS_LOCATION a_location) const;                                                                                                                      // 113
		virtual void CreateMovementController();                                                                                                                                                         // 114
		virtual bool ShouldPivotToFaceCamera() const { return false; }                                                                                                                                   // 115
		virtual BGSKeyword* GetSpeakingAnimArchType() { return speakingAnimArchType; }                                                                                                                   // 116
		virtual void KillImpl(Actor* a_attacker, float a_damage, bool a_sendEvent, bool a_ragdollInstant);                                                                                               // 117
		virtual void DoReparentWeapon(const TESObjectWEAP* a_weapon, BGSEquipIndex a_equipIndex, bool a_weaponDrawn);                                                                                    // 118
		virtual bool DrinkPotion(AlchemyItem* a_potion, std::uint32_t a_stackID);                                                                                                                        // 119
		virtual bool CheckCast(MagicItem* a_spell, bool a_dualCast, MagicSystem::CannotCastReason* a_reason);                                                                                            // 11A
		virtual void CheckTempModifiers() { return; }                                                                                                                                                    // 11B
		virtual void SetLastRiddenMount([[maybe_unused]] ActorHandle a_mount) { return; }                                                                                                                                 // 11C
		virtual ActorHandle QLastRiddenMount() const;                                                                                                                                                    // 11D
		virtual bool CalculateCachedOwnerIsUndead() const;                                                                                                                                               // 11E
		virtual bool CalculateCachedOwnerIsNPC() const;                                                                                                                                                  // 11F
		virtual bool SetSneaking(bool a_sneaking);                                                                                                                                                       // 120
		virtual void ResetHavokPhysics();                                                                                                                                                                // 121
		virtual bool ShouldDoCharacterUpdate() const { return true; }                                                                                                                                    // 122
		virtual bool GetAnimationCanBeInterpolated() const;                                                                                                                                              // 123
		virtual const BSFixedString& GetResponseString() const;                                                                                                                                          // 124
		virtual void ModifyMovementData(float a_delta, NiPoint3& a_moveDelta, NiPoint3& a_angleDelta);                                                                                                   // 125
		virtual void UpdateCombatControllerSettings();                                                                                                                                                   // 126
		virtual void UpdateFadeSettings(bhkCharacterController* a_charController);                                                                                                                       // 127
		virtual bool ComputeMotionFeedbackSpeedAndDirection(const ActorMotionFeedbackData& a_data, float a_delta, ActorMotionFeedbackOutput& a_retData);                                                 // 128
		virtual bool UpdateFeedbackGraphSpeedAndDirection(const ActorMotionFeedbackOutput& a_data);                                                                                                      // 129
		virtual void UpdateActor3DPosition();                                                                                                                                                            // 12A
		virtual void PrecacheData();                                                                                                                                                                     // 12B
		virtual void WornArmorChanged(const BGSObjectInstance& a_object);                                                                                                                                // 12C
		virtual void ProcessTracking(float a_delta, NiAVObject* a_ob3D);                                                                                                                                 // 12D
		virtual void CreateActorMover();                                                                                                                                                                 // 12E
		virtual void DestroyActorMover();                                                                                                                                                                // 12F
		virtual bool ShouldRespondToActorCollision(const MovementMessageActorCollision&, const BSPointerHandleSmartPointer<BSPointerHandleManagerInterface<Actor, HandleManager>>& a_otherActor) const;  // 130
		virtual float CheckClampDamageModifier(ActorValueInfo& a_info, float a_delta);                                                                                                                   // 131
		virtual void ValidateNewPath([[maybe_unused]] const MovementMessageNewPath& a_newPathMessage) { return; }                                                                                    // 132

		void AddPerk(BGSPerk* a_perk, std::uint8_t a_rank = 0)
		{
			using func_t = decltype(&Actor::AddPerk);
			REL::Relocation<func_t> func{ REL::SelectVersionID(187096, 2230121) };
			return func(this, a_perk, a_rank);
		}

		void QueueUpdate(bool bDoFaceGen, uint32_t unk2, bool DoQueue, uint32_t flags3D) 
		{
			using func_t = decltype(&Actor::QueueUpdate);
			REL::Relocation<func_t> func{ REL::SelectVersionID(302888, 2229913) };
			return func(this, bDoFaceGen, unk2, DoQueue, flags3D);
		}

		[[nodiscard]] TESAmmo* GetCurrentAmmo(BGSEquipIndex a_equipIndex) const
		{
			return currentProcess ? currentProcess->GetCurrentAmmo(a_equipIndex) : nullptr;
		}

		[[nodiscard]] bool GetHostileToActor(Actor* a_actor)
		{
			using func_t = decltype(&Actor::GetHostileToActor);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1148686, 2229968) };
			return func(this, a_actor);
		}

		[[nodiscard]] std::int16_t GetLevel()
		{
			using func_t = decltype(&Actor::GetLevel);
			REL::Relocation<func_t> func{ REL::SelectVersionID(661617, 2229734) };
			return func(this);
		}

		[[nodiscard]] TESNPC* GetNPC() const noexcept;

		[[nodiscard]] std::uint8_t GetPerkRank(BGSPerk* a_perk)
		{
			using func_t = decltype(&Actor::GetPerkRank);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1368313, 2230125) };
			return func(this, a_perk);
		}

		[[nodiscard]] bool IsJumping()
		{
			using func_t = decltype(&Actor::IsJumping);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1041558, 2229640) };
			return func(this);
		}

		void RemovePerk(BGSPerk* a_perk)
		{
			using func_t = decltype(&Actor::RemovePerk);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1316475, 2230122) };
			return func(this, a_perk);
		}

		void Reset3D(bool a_reloadAll, RESET_3D_FLAGS a_additionalFlags, bool a_queueReset, RESET_3D_FLAGS a_excludeFlags)
		{
			using func_t = decltype(&Actor::Reset3D);
			REL::Relocation<func_t> func{ REL::SelectVersionID(302888, 2229913) };
			return func(this, a_reloadAll, a_additionalFlags, a_queueReset, a_excludeFlags);
		}

		void RewardExperience(float a_amount, bool a_direct, TESObjectREFR* a_actionTarget, TESObjectREFR* a_killWeapon)
		{
			using func_t = decltype(&Actor::RewardExperience);
			REL::Relocation<func_t> func{ REL::SelectVersionID(262786, 2230428) };
			return func(this, a_amount, a_direct, a_actionTarget, a_killWeapon);
		}

		void SetCurrentAmmo(BGSEquipIndex a_equipIndex, TESAmmo* a_ammo)
		{
			if (currentProcess) {
				currentProcess->SetCurrentAmmo(a_equipIndex, a_ammo);
			}
		}

		void TrespassAlarm(TESObjectREFR* a_refr, TESForm* a_owner, std::int32_t a_crime)
		{
			using func_t = decltype(&Actor::TrespassAlarm);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1109888, 2229834) };
			return func(this, a_refr, a_owner, a_crime);
		}

		void EvaluatePackage(bool a_commandMode /*a_queue*/, bool a_force)
		{
			using func_t = decltype(&Actor::EvaluatePackage);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1395257, 2229805) };
			return func(this, a_commandMode, a_force);
		}

		//panda 0xD7FF20
		bool EvaluatePackageAfter3DLoaded(bool flag) {
			using func_t = decltype(&Actor::EvaluatePackageAfter3DLoaded);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1326547, 2229806) };
			return func(this, flag);
		}

		//All flags are set to true when making an actor instantly snap out of an interaction.
		void StopInteractingQuick(bool flag1 = true, bool flag2 = true, bool a_initializeNodes = false)
		{
			using func_t = decltype(&Actor::StopInteractingQuick);
			REL::Relocation<func_t> func{ REL::SelectVersionID(129904, 2231227) };
			//REL::Relocation<func_t> func{ REL::Offset(0xE0EAA0) };
			return func(this, flag1, flag2, a_initializeNodes);
		}

		void SetGhost(bool a_ghost)
		{
			using func_t = decltype(&Actor::SetGhost);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1377793, 2229669) };
			return func(this, a_ghost);
		}

		bool GetGhost()
		{
			using func_t = decltype(&Actor::GetGhost);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1298473, 2229668) };
			return func(this);
		}

		ActorHandle GetActorHandle()
		{
			using func_t = decltype(&Actor::GetActorHandle);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1145222, 2229894) };
			return func(this);
		}

		void SetRestrained(bool a_restrained)
		{
			using func_t = decltype(&Actor::SetRestrained);
			REL::Relocation<func_t> func{ REL::SelectVersionID(316742, 2229910) };
			return func(this, a_restrained);
		}

		virtual void ModifyInitialAnimationStateImpl(bool a_initInstant) override
		{
			using func_t = decltype(&Actor::ModifyInitialAnimationStateImpl);
			REL::Relocation<func_t> func{ REL::SelectVersionID(825615, 2230455) };
			return func(this, a_initInstant);
		}

		void DisableCollision()
		{
			using func_t = decltype(&Actor::DisableCollision);
			REL::Relocation<func_t> func{ REL::SelectVersionID(475952, 2229653) };
			return func(this);
		}

		void EnableCollision()
		{
			using func_t = decltype(&Actor::EnableCollision);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1274117, 2229652) };
			return func(this);
		}

		void WarpActorAndFollowers(const TESObjectREFR* a_locationRef, bool a_spaceOutFollowers, bool a_queue, bool a_resetAI)
		{
			using func_t = decltype(&Actor::WarpActorAndFollowers);
			REL::Relocation<func_t> func{ REL::SelectVersionID(574970, 2229839) };
			return func(this, a_locationRef, a_spaceOutFollowers, a_queue, a_resetAI);
		}

		void WarpTo(const NiPoint3& a_point, float a_zRotation, float a_xRotation, TESObjectCELL* a_cell, TESWorldSpace* a_worldspace, bool doPathing, bool doFade = false, bool isPlayer = false)
		{
			using func_t = decltype(&Actor::WarpTo);
			REL::Relocation<func_t> func{ REL::SelectVersionID(728864, 2229838) };
			return func(this, a_point, a_zRotation, a_xRotation, a_cell, a_worldspace, doPathing, doFade, isPlayer);
		}

		void WarpToRef(const TESObjectREFR* a_dest) {
			WarpTo(a_dest->data.location, a_dest->data.angle.z, 0.0f, a_dest->parentCell, a_dest->GetWorldSpace(), false);
		}

		void ClearLookAtTarget()
		{
			using func_t = decltype(&Actor::ClearLookAtTarget);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1345971, 2230291) };
			return func(this);
		}

		void TurnOffHeadtracking()
		{
			using func_t = decltype(&Actor::TurnOffHeadtracking);
			REL::Relocation<func_t> func{ REL::SelectVersionID(149104, 2230240) };
			return func(this);
		}

		void StopAlarmOnActor()
		{
			using func_t = decltype(&Actor::StopAlarmOnActor);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1564830, 2229667) };
			return func(this);
		}

		bool GetCanDoFavor(bool a_unk = false)
		{
			using func_t = decltype(&Actor::GetCanDoFavor);
			REL::Relocation<func_t> func{ REL::SelectVersionID(46113, 2230148) };
			return func(this, a_unk);
		}

		bool SetCanDoFavor(bool a_canDoFavor)
		{
			using func_t = decltype(&Actor::SetCanDoFavor);
			REL::Relocation<func_t> func{ REL::SelectVersionID(391670, 2230149) };
			return func(this, a_canDoFavor);
		}

		void StopMoving(float a_decelerationMult = 1.0f)
		{
			using func_t = decltype(&Actor::StopMoving);
			REL::Relocation<func_t> func{ REL::SelectVersionID(940952, 2230262) };
			return func(this, a_decelerationMult);
		}

		enum Sex : int32_t
		{
			None = -1,
			Male = 0,
			Female = 1
		};

		[[nodiscard]] Sex GetSex() const noexcept;

		TESObjectREFR* GetFurnitureReference()
		{
			NiPointer<TESObjectREFR> refr;

			auto currentProc = this->currentProcess;
			if (!currentProc)
				return nullptr;

			RE::ObjectRefHandle furnitureHandle;
			auto middleHigh = currentProc->middleHigh;
			if (!middleHigh)
				return nullptr;

			if (this->interactingState == INTERACTING_STATE::kInteracting || this->interactingState == INTERACTING_STATE::kWaitingToInteract) {
				furnitureHandle = middleHigh->occupiedFurniture;
			} else {
				furnitureHandle = middleHigh->currentFurniture;
			}

			if (furnitureHandle) {
				refr = furnitureHandle.get();
			} else {
				return nullptr;
			}

			return refr.get();
		}

		void ClearPath() {
			using func_t = decltype(&Actor::ClearPath);
			REL::Relocation<func_t> func{ REL::SelectVersionID(927158, 2230263) };
			return func(this);
			}

		// members
		NiTFlags<std::uint32_t, Actor> niFlags;                              // 2D0
		float updateTargetTimer;                                             // 2D4
		NiPoint3 editorLocCoord;                                             // 2D8
		NiPoint3 editorLocRot;                                               // 2E4
		TESForm* editorLocForm;                                              // 2F0
		BGSLocation* editorLocation;                                         // 2F8
		AIProcess* currentProcess;                                           // 300
		ActorMover* actorMover;                                              // 308
		BGSKeyword* speakingAnimArchType;                                    // 310
		BSTSmartPointer<MovementControllerNPC> movementController;           // 318
		TESPackage* initialPackage;                                          // 320
		CombatController* combatController;                                  // 328
		TESFaction* vendorFaction;                                           // 330
		ActorValueStorage avStorage;                                         // 338
		BGSDialogueBranch* exclusiveBranch;                                  // 370
		stl::enumeration<ACTOR_CRITICAL_STAGE, std::int32_t> criticalStage;  // 378
		ObjectRefHandle dialogueItemTarget;                                  // 37C
		ActorHandle currentCombatTarget;                                     // 380
		ActorHandle myKiller;                                                // 384
		float checkMyDeadBodyTimer;                                          // 388
		float voiceTimer;                                                    // 38C
		float voiceLengthTotal;                                              // 390
		float underWaterTimer;                                               // 394
		std::int32_t thiefCrimeStamp;                                        // 398
		std::int32_t actionValue;                                            // 39C
		float timeronAction;                                                 // 3A0
		AITimeStamp calculateVendorFactionTimer;                             // 3A4
		std::uint32_t intimidateBribeDayStamp;                               // 3A8
		float equippedWeight;                                                // 3AC
		BSTSmallArray<SpellItem*> addedSpells;                               // 3B0
		ActorMagicCaster* magicCasters[4];                                   // 3C8
		MagicItem* selectedSpell[4];                                         // 3E8
		CastPowerItem* castPowerItems;                                       // 408
		TESForm* selectedPower;                                              // 410
		TESRace* race;                                                       // 418
		Perks* perks;                                                        // 420
		BSTSmartPointer<BipedAnim> biped;                                    // 428
		BSNonReentrantSpinLock addingToOrRemovingFromScene;                  // 430
		BSReadWriteLock perkArrayLock;                                       // 434
		stl::enumeration<BOOL_FLAGS, std::uint32_t> boolFlags;               // 43C
		std::uint32_t moreFlags;                                             // 440
		Modifiers healthModifiers;                                           // 444
		Modifiers actionPointsModifiers;                                     // 450
		Modifiers staminaModifiers;                                          // 45C
		Modifiers radsModifiers;                                             // 468
		float lastUpdate;                                                    // 474
		std::uint32_t lastSeenTime;                                          // 478
		float armorRating;                                                   // 47C
		float armorBaseFactorSum;                                            // 480
		std::uint32_t visFlags: 4;                                           // 484:00
		std::int8_t raceSwitchPending: 1;                                    // 488:0
		std::int8_t soundCallBackSet;                                        // 489
		bool trespassing;                                                    // 48A
	};
	static_assert(sizeof(Actor) == 0x490);

	class ActorEquipManager :
		public BSTSingletonSDM<ActorEquipManager>,            // 00
		public BSTEventSource<ActorEquipManagerEvent::Event>  // 08
	{
	public:
		[[nodiscard]] static ActorEquipManager* GetSingleton()
		{
			REL::Relocation<ActorEquipManager**> singleton{ REL::SelectVersionID(1174340, 4798287) };
			return *singleton;
		}

		bool EquipObject(
			Actor* a_actor,
			const BGSObjectInstance& a_object,
			std::uint32_t a_stackID,
			std::uint32_t a_number,
			const BGSEquipSlot* a_slot,
			bool a_queueEquip,
			bool a_forceEquip,
			bool a_playSounds,
			bool a_applyNow,
			bool a_locked)
		{
			using func_t = decltype(&ActorEquipManager::EquipObject);
			REL::Relocation<func_t> func{ REL::SelectVersionID(988029, 2231392) };
			return func(this, a_actor, a_object, a_stackID, a_number, a_slot, a_queueEquip, a_forceEquip, a_playSounds, a_applyNow, a_locked);
		}

		bool UnequipObject(
			Actor* a_actor,
			const BGSObjectInstance* a_object,
			std::uint32_t a_number,
			const BGSEquipSlot* a_slot,
			std::uint32_t a_stackID,
			bool a_queueEquip,
			bool a_forceEquip,
			bool a_playSounds,
			bool a_applyNow,
			const BGSEquipSlot* a_slotBeingReplaced)
		{
			using func_t = decltype(&ActorEquipManager::UnequipObject);
			REL::Relocation<func_t> func{ REL::SelectVersionID(1292493, 2231395) };
			return func(this, a_actor, a_object, a_number, a_slot, a_stackID, a_queueEquip, a_forceEquip, a_playSounds, a_applyNow, a_slotBeingReplaced);
		}
	};
	static_assert(sizeof(ActorEquipManager) == 0x60);
}
