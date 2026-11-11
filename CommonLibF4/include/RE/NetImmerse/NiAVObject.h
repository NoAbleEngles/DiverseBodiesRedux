#pragma once

#include "RE/NetImmerse/NiBound.h"
#include "RE/NetImmerse/NiCollisionObject.h"
#include "RE/NetImmerse/NiFlags.h"
#include "RE/NetImmerse/NiObjectNET.h"
#include "RE/NetImmerse/NiSmartPointer.h"
#include "RE/NetImmerse/NiTransform.h"

namespace RE
{
	class NiAlphaProperty;
	class NiCullingProcess;
	class NiNode;
	class NiUpdateData;
	class PerformOpFunc;

	class __declspec(novtable) NiAVObject :
		public NiObjectNET  // 000
	{
	public:
		static constexpr auto RTTI{ RTTI::NiAVObject };
		static constexpr auto VTABLE{ VTABLE::NiAVObject };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiAVObject };

		NiAVObject();
		virtual ~NiAVObject();  // NOLINT(modernize-use-override) 00

		// add
		virtual void UpdateControllers(NiUpdateData& a_data);                                                            // 28
		virtual void PerformOp(PerformOpFunc& a_operation);                                                              // 29
		virtual void AttachProperty([[maybe_unused]] NiAlphaProperty* a_prop) { return; }                                // 2A
		virtual void SetMaterialNeedsUpdate(bool) { return; }                                                            // 2B
		virtual void SetDefaultMaterialNeedsUpdateFlag(bool) { return; }                                                 // 2C
		virtual void SetAppCulled(bool a_appCulled);                                                                     // 2D
		virtual NiAVObject* GetObjectByName(const BSFixedString& a_name) { return name == a_name ? this : nullptr; }     // 2E
		virtual void SetSelectiveUpdateFlags(bool& a_selectiveUpdate, bool a_selectiveUpdateTransforms, bool& a_rigid);  // 2F
		virtual void UpdateDownwardPass([[maybe_unused]] NiUpdateData& a_data, [[maybe_unused]] std::uint32_t a_flags) {}									 // 30
		virtual void UpdateSelectedDownwardPass([[maybe_unused]] NiUpdateData& a_data, [[maybe_unused]] std::uint32_t a_flags) {};                         // 31
		virtual void UpdateRigidDownwardPass([[maybe_unused]] NiUpdateData& a_data, [[maybe_unused]] std::uint32_t a_flags) {};                            // 32
		virtual void UpdateWorldBound() { return; }                                                                      // 33
		virtual void UpdateWorldData([[maybe_unused]] NiUpdateData* a_data) {};                                                           // 34
		virtual void UpdateTransformAndBounds([[maybe_unused]] NiUpdateData& a_data) {};                                                  // 35
		virtual void UpdateTransforms(NiUpdateData& a_data) { UpdateWorldData(std::addressof(a_data)); }                 // 36
		virtual void PreAttachUpdate([[maybe_unused]] NiNode* a_eventualParent, [[maybe_unused]] NiUpdateData& a_data) {};                                 // 37
		virtual void PostAttachUpdate() {};                                                                              // 38
		virtual void OnVisible([[maybe_unused]] NiCullingProcess& a_culler) { return; }                                  // 39

		void Update(NiUpdateData& a_data)
		{
			UpdateDownwardPass(a_data, 0);
		}

		F4_HEAP_REDEFINE_ALIGNED_NEW(NiAVObject);

		[[nodiscard]] bool GetAppCulled() const noexcept { return flags.flags & 1; }
		[[nodiscard]] std::uint64_t GetFlags() const noexcept { return flags.flags; }
		[[nodiscard]] bool ShadowCaster() const noexcept { return ~(flags.flags >> 40) & 1; }

		enum : uint64_t
		{
			kFlagAppCulled = (1 << 0),					// (Named "Hidden" in NifSkope) Forces the object to be permanently culled, has no effect when set on root node.
			kFlagSelectiveUpdate = (1 << 1),			// Automatically set by the engine
			kFlagSelectiveUpdateTransforms = (1 << 2),  // Automatically set by the engine
			kFlagSelectiveUpdateControllers = (1 << 3),	// Automatically set by the engine
			kFlagSelectiveUpdateRigid = (1 << 4),		// Unused/broken, supposed to move NiNode local bounds into the computed world bounds.
			kFlagDisplayObject = (1 << 5),              // Used only by sky objects
			kFlagDisableSorting = (1 << 6),             // Unused
			kFlagSelectiveUpdateTransformsOverride = (1 << 7),
			kFlagUnk_8 = (1 << 8),
			kFlagSaveExternalGeometryData = (1 << 9),	// Ignores transforms set on the geometry, cannot be set on root node or the mesh will turn invisible
			kFlagNoDecals = (1 << 10),					// Disallows the NIF from receiving impact decals such as bullet holes from firearms (can only be set on root node)
			kFlagAlwaysDraw = (1 << 11),				// Forces light inclusion, and skips culling
			kFlagIsMeshLOD = (1 << 12),					// Used to mark actor nodes for actor culling
			kFlagFixedBound = (1 << 13),				// Prevents bound updates, not recommended to use as culling behavior is erratic when on
			kFlagTopFadeNode = (1 << 14),				// Automatically set by the engine
			kFlagIgnoreFade = (1 << 15),				
			kFlagNoAnimSyncX = (1 << 16),				
			kFlagNoAnimSyncY = (1 << 17),
			kFlagNoAnimSyncZ = (1 << 18),
			kFlagNoAnimSyncS = (1 << 19),
			kFlagNoDismember = (1 << 20),				// Used for actor culling
			kFlagNoDismemberValidity = (1 << 21),		// disables activation prompts and makes the object unelectable in console. Useless to set because this flag is automatically removed at runtime by the engine. Instead use SetNifBlockFlag to set it
			kFlagRenderUse = (1 << 22),						
			kFlagMaterialsApplied = (1 << 23),			
			kFlagHighDetail = (1 << 24),				// Unused
			kFlagForceUpdate = (1 << 25),
			kFlagPreProcessedNode = (1 << 26),
			kFlagScenegraphChange = (1 << 29),
			kFlagInInstanceGroup = (1LL << 35),
			kFlagLODFadingOut = (1LL << 36),
			kFlagFadedIn = (1LL << 37),
			kFlagForcedFadeOut = (1LL << 38),
			kFlagNotVisible = (1LL << 39),
			kFlagShadowCaster = (1LL << 40),
			kFlagNeedsRendererData = (1LL << 41),
			kFlagAccumulated = (1LL << 42),
			kFlagAlreadyTraversed = (1LL << 43),
			kFlagPickOff = (1LL << 44),
			kFlagUpdateWorldData = (1LL << 45),
			kFlagHasPropController = (1LL << 46),
			kFlagHasLockedChildAccess = (1LL << 47),
			kFlagHasMovingSound = (1LL << 49),
		};

		// members
		NiNode* parent{ nullptr };                     // 027
		NiTransform local;                             // 030
		NiTransform world;                             // 070
		NiBound worldBound;                            // 0B0
		NiTransform previousWorld;                     // 0C0
		NiPointer<NiCollisionObject> collisionObject;  // 100
		NiTFlags<std::uint64_t, NiAVObject> flags;     // 108
		std::uintptr_t userData{ 0 };                  // 110
		float fadeAmount{ 1.0F };                      // 118
		std::int8_t multType{ 0 };                     // 11C
		std::int8_t meshLODFadingLevel{ 0 };           // 11D
		std::int8_t currentMeshLODLevel{ 0 };          // 11E
		std::int8_t previousMeshLODLevel{ 0 };         // 11F

		bool Visit(const std::function<bool(NiAVObject*)>& functor);
	};
	static_assert(sizeof(NiAVObject) == 0x120);
}
