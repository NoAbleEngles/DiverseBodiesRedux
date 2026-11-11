#pragma once

#include "REL/Relocation.h"

namespace RE
{
	namespace RTTI_AE
	{
		// AE (Anniversary Edition / Next-Gen) RTTI IDs
		// OG IDs remain in RTTI_IDs.h
		// Usage in classes: static constexpr auto RTTI{ REL::REL_ID_VER(RTTI::ClassName, RTTI_AE::ClassName) };

		// Memory Management
		inline constexpr REL::ID IMemoryStoreBase{ 4858639 };
		inline constexpr REL::ID IMemoryStore{ 4858637 };
		inline constexpr REL::ID IMemoryHeap{ 4858649 };
		inline constexpr REL::ID ScrapHeap{ 4858641 };

		//Actor
		inline constexpr REL::ID Actor{ 4839606 };
		inline constexpr REL::ID PlayerCharacter{ 4848842 };
		inline constexpr REL::ID ActorPackageData{ 4843528 };
		inline constexpr REL::ID EquippedItemData{ 4848794 };
		inline constexpr REL::ID EquippedWeaponData{ 4848796 };
		inline constexpr REL::ID MagicTarget{ 4847852 };
		inline constexpr REL::ID ActorState{ 4848596 };
		inline constexpr REL::ID IPostAnimationChannelUpdateFunctor{ 4848610 };

		//TESForm
		inline constexpr REL::ID TESForm{ 4839494 };
		inline constexpr REL::ID TESObjectREFR{ 4839510 };
		inline constexpr REL::ID TESBoundObject{ 4839882 };
		inline constexpr REL::ID TESNPC{ 4840540 };
		inline constexpr REL::ID TESActorBase{ 4840070 };
		inline constexpr REL::ID TESRace{ 4840678 };
		inline constexpr REL::ID TESObjectARMO{ 4840616 };
		inline constexpr REL::ID TESFaction{ 4840066 };
		inline constexpr REL::ID BGSKeyword{ 4839770 };
		inline constexpr REL::ID BGSLocationRefType{ 4839782 };
		inline constexpr REL::ID BGSAction{ 4842220 };
		inline constexpr REL::ID BGSTransform{ 4839810 };
		inline constexpr REL::ID BGSMenuIcon{ 4841930 };
		inline constexpr REL::ID TESGlobal{ 4840012 };
		inline constexpr REL::ID BGSDamageType{ 4841234 };
		inline constexpr REL::ID TESClass{ 4842350 };
		inline constexpr REL::ID TESEyes{ 4840530 };
		inline constexpr REL::ID EffectSetting{ 4839924 };
		inline constexpr REL::ID TESLandTexture{ 4841130 };
		inline constexpr REL::ID TESWeather{ 4841700 };
		inline constexpr REL::ID TESClimate{ 4841702 };
		inline constexpr REL::ID BGSShaderParticleGeometryData{ 4841682 };
		inline constexpr REL::ID BGSReferenceEffect{ 4839936 };
		inline constexpr REL::ID TESRegion{ 4841100 };
		inline constexpr REL::ID TESObjectCELL{ 4839496 };
		inline constexpr REL::ID TESObjectLAND{ 4841718 };
		inline constexpr REL::ID TESTopic{ 4840058 };
		inline constexpr REL::ID TESTopicInfo{ 4840056 };
		inline constexpr REL::ID TESIdleForm{ 4839556 };
		inline constexpr REL::ID TESLoadScreen{ 4842048 };
		inline constexpr REL::ID TESObjectANIO{ 4841472 };
		inline constexpr REL::ID TESEffectShader{ 4839912 };
		inline constexpr REL::ID BGSDebris{ 4840594 };
		inline constexpr REL::ID TESImageSpace{ 4840042 };
		inline constexpr REL::ID TESImageSpaceModifier{ 4839930 };
		inline constexpr REL::ID BGSListForm{ 4839946 };
		inline constexpr REL::ID BGSPerk{ 4839934 };
		inline constexpr REL::ID BGSBodyPartData{ 4842232 };
		inline constexpr REL::ID BGSCameraShot{ 4841860 };
		inline constexpr REL::ID BGSCameraPath{ 4841858 };
		inline constexpr REL::ID BGSVoiceType{ 4840284 };
		inline constexpr REL::ID BGSMaterialType{ 4841836 };
		inline constexpr REL::ID BGSImpactData{ 4841904 };
		inline constexpr REL::ID BGSImpactDataSet{ 4839908 };
		inline constexpr REL::ID BGSEncounterZone{ 4840050 };
		inline constexpr REL::ID BGSLocation{ 4840052 };
		inline constexpr REL::ID BGSMessage{ 4839780 };
		inline constexpr REL::ID BGSLightingTemplate{ 4841648 };
		inline constexpr REL::ID BGSMusicType{ 4840048 };
		inline constexpr REL::ID BGSFootstep{ 4841892 };
		inline constexpr REL::ID BGSFootstepSet{ 4841898 };
		inline constexpr REL::ID BGSDialogueBranch{ 4840060 };
		inline constexpr REL::ID BGSMusicTrackFormWrapper{ 4841970 };
		inline constexpr REL::ID TESWordOfPower{ 4842214 };
		inline constexpr REL::ID TESShout{ 4840682 };
		inline constexpr REL::ID BGSEquipSlot{ 4842262 };
		inline constexpr REL::ID BGSRelationship{ 4842312 };
		inline constexpr REL::ID BGSScene{ 4840072 };
		inline constexpr REL::ID BGSAssociationType{ 4842226 };
		inline constexpr REL::ID BGSOutfit{ 4841348 };
		inline constexpr REL::ID BGSMaterialObject{ 4841920 };
		inline constexpr REL::ID BGSMovementType{ 4842288 };
		inline constexpr REL::ID BGSSoundDescriptorForm{ 4841186 };
		inline constexpr REL::ID BGSSoundCategory{ 4840064 };
		inline constexpr REL::ID BGSSoundOutput{ 4840062 };
		inline constexpr REL::ID BGSCollisionLayer{ 4841360 };
		inline constexpr REL::ID BGSColorForm{ 4841148 };
		inline constexpr REL::ID BGSReverbParameters{ 4841854 };
		inline constexpr REL::ID BGSPackIn{ 4841354 };
		inline constexpr REL::ID BGSAimModel{ 4841280 };
		inline constexpr REL::ID BGSConstructibleObject{ 4841302 };
		inline constexpr REL::ID BGSMaterialSwap{ 4839802 };
		inline constexpr REL::ID BGSZoomData{ 4841400 };
		inline constexpr REL::ID BGSInstanceNamingRules{ 4841244 };
		inline constexpr REL::ID BGSSoundKeywordMapping{ 4841260 };
		inline constexpr REL::ID BGSAudioEffectChain{ 4841196 };
		inline constexpr REL::ID BGSAttractionRule{ 4841146 };
		inline constexpr REL::ID BGSSoundCategorySnapshot{ 4841254 };
		inline constexpr REL::ID BGSSoundTagSet{ 4839816 };
		inline constexpr REL::ID BSLensFlareRenderData{ 4841326 };
		inline constexpr REL::ID BGSLensFlare{ 4841324 };
		inline constexpr REL::ID BGSGodRays{ 4840076 };

		//ActorValueInfo
		inline constexpr REL::ID ActorValueInfo{ 24839932 };

		//Archive2
		inline constexpr REL::ID BSResource__Archive2__Index{ 4858801 };
		inline constexpr REL::ID BSResource__Archive2__ReaderStream{ 4858797 };
		inline constexpr REL::ID BSResource__Archive2__AsyncReaderStream{ 4858807 };

		// BGSBaseAliases
		inline constexpr REL::ID ExtraReferenceHandles{ 4840188 };
		inline constexpr REL::ID BGSRefAlias{ 4841170 };

		// BGSCharacterTint
		inline constexpr REL::ID BGSCharacterTint__Template__Entry{ 4841156 };
		inline constexpr REL::ID BGSCharacterTint__Template__Mask{ 4841158 };
		inline constexpr REL::ID BGSCharacterTint__Template__Palette{ 4841160 };
		inline constexpr REL::ID BGSCharacterTint__Template__TextureSet{ 4841164 };
		inline constexpr REL::ID BGSCharacterTint__Entry{ 4841150 };
		inline constexpr REL::ID BGSCharacterTint__MaskEntry{ 4841152 };
		inline constexpr REL::ID BGSCharacterTint__PaletteEntry{ 4841154 };
		inline constexpr REL::ID BGSCharacterTint__TextureSetEntry{ 4841162 };

		// BGSDefaultObjectManager
		inline constexpr REL::ID BGSDefaultObjectManager{ 4841884 };
		inline constexpr REL::ID BGSDefaultObject{ 4841238 };

		// BGSHeadPart
		inline constexpr REL::ID BGSHeadPart{ 4842268 };

		// BGSInventoryItem
		inline constexpr REL::ID BGSInventoryItem__Stack{ 4840784 };
		inline constexpr REL::ID BGSInventoryItem__StackDataCompareFunctor{ 4840770 };
		inline constexpr REL::ID BGSInventoryItem__CheckStackIDFunctor{ 4840772 };
		inline constexpr REL::ID BGSInventoryItem__StackDataWriteFunctor{ 4840776 };
		inline constexpr auto BGSInventoryItem__ModifyModDataFunctor{ 0x3758a50 };
		inline constexpr REL::ID __ApplyChangesFunctor{ 4847168 };

		//BGSMod
		inline constexpr REL::ID BGSMod__Container{ 4839796 };
		inline constexpr REL::ID BGSMod__Attachment__Mod{ 4839788 };
		inline constexpr REL::ID BGSMod__Template__Item{ 4839794 };
		inline constexpr REL::ID BGSMod__Template__Items{ 4839790 };

		//BGSPrimitive
		inline constexpr REL::ID BGSPrimitive{ 4840836 };

		//BGSStoryManagerTreeVisitor
		inline constexpr REL::ID BGSStoryManagerTreeVisitor{ 4844060 };
		inline constexpr REL::ID BGSStoryManagerTreeForm{ 4840556 };
		inline constexpr REL::ID TESQuest{ 4840074 };
		inline constexpr REL::ID BGSStoryManagerNodeBase{ 4840554 };
		inline constexpr REL::ID BGSStoryManagerBranchNode{ 4840552 };
		inline constexpr REL::ID BGSStoryManagerQuestNode{ 4844082 };
		inline constexpr REL::ID BGSStoryManagerEventNode{ 4844076 };

		//BGSTextureSet
		inline constexpr REL::ID BGSTextureSet{ 4841294 };

		//bhkCharacterController
		inline constexpr REL::ID bhkNPCollisionObjectBase{ 4859769 };
		inline constexpr REL::ID bhkNPCollisionObject{ 4859767 };
		inline constexpr REL::ID bhkNPCollisionObjectUnlinked{ 4859771 };
		inline constexpr REL::ID bhkCharacterController{ 4859801 };

		//BSBound
		inline constexpr REL::ID BSBound{ 4859587 };
		inline constexpr REL::ID BSClothExtraData{ 4859827 };
		inline constexpr REL::ID BSClothUtils__BSTransformSet{ 4859835 };
		inline constexpr REL::ID BSClothUtils__BSTriShapeBuffer{ 4859837 };

		//BSExtraData
		inline constexpr REL::ID BSExtraData{ 4839764 };
		inline constexpr REL::ID ExtraCellWaterType{ 4840038 };
		inline constexpr REL::ID ExtraLock{ 4840084 };
		inline constexpr REL::ID ExtraCulledBone{ 4840082 };
		inline constexpr REL::ID ExtraTeleport{ 4840086 };
		inline constexpr REL::ID ExtraStartingPosition{ 4840088 };
		inline constexpr REL::ID ExtraOwnership{ 4840090 };
		inline constexpr REL::ID ExtraGlobal{ 4840092 };
		inline constexpr REL::ID ExtraRank{ 4840094 };
		inline constexpr REL::ID ExtraCount{ 4840096 };
		inline constexpr REL::ID ExtraLeveledItem{ 4840098 };
		inline constexpr REL::ID ExtraOutfitItem{ 4840100 };
		inline constexpr REL::ID ExtraHealth{ 4840102 };
		inline constexpr REL::ID ExtraTimeLeft{ 4840104 };
		inline constexpr REL::ID ExtraCharg{ 4840106 };
		inline constexpr REL::ID ExtraScale{ 4840108 };
		inline constexpr REL::ID ExtraFollower{ 4840110 };
		inline constexpr REL::ID ExtraOriginalReference{ 4840112 };
		inline constexpr REL::ID ExtraPoison{ 4840114 };
		inline constexpr REL::ID ExtraHeadingTarget{ 4840116 };
		inline constexpr REL::ID ExtraCreatureAwakeSound{ 4840118 };
		inline constexpr REL::ID ExtraObjectHealth{ 4840120 };
		inline constexpr REL::ID ExtraActorCause{ 4840122 };
		inline constexpr REL::ID ExtraRadioData{ 4840124 };
		inline constexpr REL::ID ExtraPatrolRefData{ 4840126 };
		inline constexpr REL::ID ExtraNavMeshPortal{ 4840128 };
		inline constexpr REL::ID ExtraOcclusionPlaneRefData{ 4840130 };
		inline constexpr REL::ID ExtraPortalRefData{ 4840132 };
		inline constexpr REL::ID ExtraSceneData{ 4840134 };
		inline constexpr REL::ID ExtraBadPosition{ 4840136 };
		inline constexpr REL::ID ExtraHeadTrackingWeight{ 4840138 };
		inline constexpr REL::ID ExtraFavorCost{ 4840140 };
		inline constexpr REL::ID ExtraTextDisplayData{ 4840142 };
		inline constexpr REL::ID ExtraHorse{ 4840144 };
		inline constexpr REL::ID ExtraEnchantment{ 4840146 };
		inline constexpr REL::ID ExtraForcedTarget{ 4840148 };
		inline constexpr REL::ID ExtraUniqueID{ 4840150 };
		inline constexpr REL::ID ExtraFlags{ 4840152 };
		inline constexpr REL::ID ExtraWaterCurrentZoneData{ 4840154 };
		inline constexpr REL::ID ExtraMissingRefIDs{ 4840156 };
		inline constexpr REL::ID ExtraRangedDistOverride{ 4840158 };
		inline constexpr REL::ID ExtraSoundOutputOverride{ 4840160 };
		inline constexpr REL::ID ExtraEditorID{ 4840162 };
		inline constexpr REL::ID ExtraFavorite{ 4840164 };
		inline constexpr REL::ID ExtraPrimitive{ 4840166 };
		inline constexpr REL::ID ExtraAmmo{ 4840168 };
		inline constexpr REL::ID ExtraCombinedRefs{ 4840170 };
		inline constexpr REL::ID ExtraPreVisRefs{ 4840172 };
		inline constexpr REL::ID ExtraTransitionCellCount{ 4840174 };
		inline constexpr REL::ID ExtraGIDBuffer{ 4840176 };
		inline constexpr REL::ID ExtraFromAlias{ 4840180 };
		inline constexpr REL::ID ExtraOpenCloseActivateRef{ 4840182 };
		inline constexpr REL::ID ExtraTeleportName{ 4840184 };
		inline constexpr REL::ID ExtraCachedScale{ 4840186 };
		inline constexpr REL::ID ExtraLocation{ 4840190 };
		inline constexpr REL::ID ExtraTresPassPackage{ 4840192 };
		inline constexpr REL::ID ExtraAttachRef{ 4840194 };
		inline constexpr REL::ID ExtraAttachRefChildren{ 4840196 };
		inline constexpr REL::ID ExtraPowerLinks{ 4840198 };
		inline constexpr REL::ID ExtraReflectedRefs{ 4840200 };
		inline constexpr REL::ID ExtraReflectorRefs{ 4840202 };
		inline constexpr REL::ID ExtraWaterLightRefs{ 4840204 };
		inline constexpr REL::ID ExtraLitWaterRefs{ 4840206 };
		inline constexpr REL::ID ExtraHasNoRumors{ 4840208 };
		inline constexpr REL::ID ExtraModelSwap{ 4840210 };
		inline constexpr REL::ID ExtraRadius{ 4840212 };
		inline constexpr REL::ID ExtraCombatStyle{ 4840214 };
		inline constexpr REL::ID ExtraPackageData{ 4840216 };
		inline constexpr REL::ID ExtraCollisionData{ 4840218 };
		inline constexpr REL::ID ExtraLargeRefOwnerCells{ 4840220 };
		inline constexpr REL::ID ExtraLightData{ 4840222 };
		inline constexpr REL::ID ExtraModRank{ 4840224 };
		inline constexpr REL::ID ExtraAttachedArrows3D{ 4840226 };
		inline constexpr REL::ID ExtraAlphaCutoff{ 4840228 };
		inline constexpr REL::ID ExtraForcedLandingMarker{ 4840230 };
		inline constexpr REL::ID ExtraCellWaterEnvMap{ 4840232 };
		inline constexpr REL::ID ExtraKeywords{ 4840234 };
		inline constexpr REL::ID ExtraMaterialSwap{ 4840236 };
		inline constexpr REL::ID ExtraProjectedDecalData{ 4840238 };
		inline constexpr REL::ID ExtraActivateText{ 4840240 };
		inline constexpr REL::ID ExtraRadioReceiver{ 4840242 };
		inline constexpr REL::ID ExtraRadioRepeater{ 4840244 };
		inline constexpr REL::ID ExtraAnimSounds{ 4840246 };
		inline constexpr REL::ID ExtraActorValueStorage{ 4840248 };
		inline constexpr REL::ID ExtraObjectBreakable{ 4840250 };
		inline constexpr REL::ID ExtraInstanceData{ 4840444 };
		inline constexpr REL::ID BGSObjectInstanceExtra{ 4839762 };
		inline constexpr REL::ID ExtraStartingWorldOrCell{ 4840416 };
		inline constexpr REL::ID ExtraAliasInstanceArray{ 4840362 };

		//BSHavok
		inline constexpr REL::ID bhkWorld{ 4859719 };

		//BSInputDeviceManager
		inline constexpr REL::ID BSInputDevice{ 4858701 };

		//BSInputEventReceiver
		inline constexpr REL::ID BSInputEventReceiver{ 4844972 };

		//BSInputEventSingleUser
		inline constexpr REL::ID BSInputEventSingleUser{ 4844986 };

		//BSInputEventUser
		inline constexpr REL::ID BSInputEventUser{ 4840884 };

		//BSMTAManager
		inline constexpr REL::ID BSBatchRenderer{ 4863639 };

		//BSScaleformManager
		inline constexpr REL::ID BSScaleformManager{ 4860791 };

		//BSShader
		inline constexpr REL::ID BSReloadShaderI{ 4863139 };
		inline constexpr REL::ID BSShader{ 4863137 };
		inline constexpr REL::ID BSUtilityShader{ 4863685 };

		//BSSystemUtility
		inline constexpr REL::ID BSAwardsSystemUtility{ 4856978 };
		inline constexpr REL::ID BSSteamAwardsSystemUtility{ 4856976 };

		//BSTEvent
		inline constexpr REL::ID BSTGlobalEvent{ 4858661 };

		//BSTextureSet
		inline constexpr REL::ID BSShaderProperty{ 4847582 };
		inline constexpr REL::ID BSTextureSet{ 4842024 };

		//BSTSmartPointer
		inline constexpr REL::ID BSIntrusiveRefCounted{ 4839608 };

		//FavoritesManager
		inline constexpr REL::ID FavoritesManager{ 4854514 }; 

		//FormComponents
		inline constexpr REL::ID ActorValueOwner{ 4841772 };
		inline constexpr REL::ID BGSNavmeshableObject{ 4841338 };
		inline constexpr REL::ID BGSOpenCloseForm{ 4841382 };
		inline constexpr REL::ID BSIAudioEffectChain{ 4841194 };
		inline constexpr REL::ID BSIMusicTrack{ 4841944 };
		inline constexpr REL::ID BSIMusicType{ 4841976 };
		inline constexpr REL::ID BSIReverbType{ 4841982 };
		inline constexpr REL::ID BSISoundCategory{ 4841988 };
		inline constexpr REL::ID BSISoundDescriptor{ 4841208 };
		inline constexpr REL::ID BSISoundOutputModel{ 4842000 };
		inline constexpr REL::ID BSMaterialObject{ 4841918 };
		inline constexpr REL::ID TESChildCell{ 4841720 };
		inline constexpr REL::ID TESMagicCasterForm{ 4841500 };
		inline constexpr REL::ID TESMagicTargetForm{ 4841380 };
		inline constexpr REL::ID TBO_InstanceData{ 4841462 };
		inline constexpr REL::ID BaseFormComponent{ 4839768 };
		inline constexpr REL::ID TESTexture{ 4839890 };
		inline constexpr REL::ID TESIcon{ 4839888 };
		inline constexpr REL::ID BGSAttackData{ 4842610 };
		inline constexpr REL::ID BGSAttackDataMap{ 4842612 };
		inline constexpr REL::ID BGSAttackDataForm{ 4840588 };
		inline constexpr REL::ID BGSBipedObjectForm{ 4840590 };
		inline constexpr REL::ID BGSBlockBashData{ 4840592 };
		inline constexpr REL::ID BGSCraftingUseSound{ 4839776 };
		inline constexpr REL::ID BGSDestructibleObjectForm{ 4839898 };
		inline constexpr REL::ID BGSFeaturedItemMessage{ 4839778 };
		inline constexpr REL::ID BGSForcedLocRefType{ 4839784 };
		inline constexpr REL::ID BGSIdleCollection{ 4840600 };
		inline constexpr REL::ID BGSInstanceNamingRulesForm{ 4839786 };
		inline constexpr REL::ID IKeywordFormBase{ 4839874 };
		inline constexpr REL::ID BGSMenuDisplayObject{ 4839940 };
		inline constexpr REL::ID BGSMessageIcon{ 4839892 };
		inline constexpr REL::ID BGSNativeTerminalForm{ 4839806 };
		inline constexpr REL::ID BGSOverridePackCollection{ 4840606 };
		inline constexpr REL::ID BGSPerkRankArray{ 4840608 };
		inline constexpr REL::ID BGSPickupPutdownSounds{ 4839900 };
		inline constexpr REL::ID BGSPreloadable{ 4841308 };
		inline constexpr REL::ID BGSPreviewTransform{ 4839812 };
		inline constexpr REL::ID BGSPropertySheet{ 4839814 };
		inline constexpr REL::ID BGSSkinForm{ 4840614 };
		inline constexpr REL::ID BGSSoundTagComponent{ 4839818 };
		inline constexpr REL::ID TESContainer{ 4840652 };
		inline constexpr REL::ID TESDescription{ 4839902 };
		inline constexpr REL::ID TESEnchantableForm{ 4840624 };
		inline constexpr REL::ID TESFullName{ 4839792 };
		inline constexpr REL::ID TESHealthForm{ 4840626 };
		inline constexpr REL::ID TESModel{ 4839804 };
		inline constexpr REL::ID BGSBehaviorGraphModel{ 4842406 };
		inline constexpr REL::ID BGSModelMaterialSwap{ 4839800 };
		inline constexpr REL::ID BGSTextureModel{ 4842404 };
		inline constexpr REL::ID TESModelRDT{ 4840650 };
		inline constexpr REL::ID TESActorBaseData{ 4840620 };
		inline constexpr REL::ID TESAIForm{ 4840618 };
		inline constexpr REL::ID TESBipedModelForm{ 4840474 };
		inline constexpr REL::ID TESImageSpaceModifiableForm{ 4840638 };
		inline constexpr REL::ID TESLeveledList{ 4840628 };
		inline constexpr REL::ID TESModelTri{ 4842270 };
		inline constexpr REL::ID TESProduceForm{ 4840630 };
		inline constexpr REL::ID TESRaceForm{ 4840632 };
		inline constexpr REL::ID TESReactionForm{ 4840636 };
		inline constexpr REL::ID TESSpellList{ 4840634 };
		inline constexpr REL::ID TESTexture1024{ 4841842 };
		inline constexpr REL::ID TESValueForm{ 4839956 };
		inline constexpr REL::ID TESWeightForm{ 4839894 };
		
		//FormFactory
		inline constexpr REL::ID IFormFactory{ 4839872 };
		inline constexpr REL::ID __DefaultObjectFormFactory{ 4841236 };

		//GameScript
		inline constexpr REL::ID GameScript__DelayFunctor{ 4854792 };
		inline constexpr REL::ID GameScript__Logger{ 4854986 };
		inline constexpr REL::ID GameScript__HandlePolicy{ 4855364 };
		inline constexpr REL::ID GameScript__ObjectBindPolicy{ 4855366 };
		inline constexpr REL::ID GameScript__Profiler{ 4855146 };
		inline constexpr REL::ID GameScript__SavePatcher{ 4855164 };
		inline constexpr REL::ID GameScript__BasicEventHandler{ 4856666 };
		inline constexpr REL::ID GameScript__CombatEventHandler{ 4854826 };
		inline constexpr REL::ID GameScript__BaseHandleReaderWriter{ 4854798 };
	}

	namespace Ni_RTTI_AE
	{
		//BSBound
		inline constexpr REL::ID BSBound{ 2704312 };
		inline constexpr REL::ID BSClothExtraData{ 2704836 };

		//BSHavok
		inline constexpr REL::ID bhkWorld{ 2704769 };

		//BSTextureSet
		inline constexpr REL::ID BSShaderProperty{ 2712385 };
		inline constexpr REL::ID BSTextureSet{ 2712388 };
	}
}
