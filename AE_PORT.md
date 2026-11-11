# Diverse Bodies Redux — Fallout 4 Anniversary Edition / AE (1.11.240.0) Source Port

## 1. Executive Summary

Upstream **Diverse Bodies Redux (DBR)** (**v2.10.0**, commit `c5206ba`) targets Fallout 4 runtime **1.10.163.0** and LooksMenu **1.6.20**. This port updates the C++ source tree to compile and run natively on **Fallout 4 Anniversary Edition (AE) 1.11.240.0** (and **1.11.191.0**) with **F4SE 0.7.2+** and **LooksMenu 1.7.0.4**.

### Key Source Updates
1. **UTF-8 Normalization & Cross-Compilation Compatibility**:
   - Converted all 35 UTF-16LE / CP1251 C++ source and header files under `DiverseBodiesRedux/Sources/` to clean UTF-8.
   - Standardized `#include` path casing for case-sensitive filesystems and resolved Windows SDK `interface` macro collisions.
2. **Fallout 4 AE (`1.11.240.0`) & F4SE Version Export**:
   - Added `F4SEPlugin_Version` (`F4SE::PluginVersionData`) in `main.cpp` declaring runtime compatibility for `1.11.191.0` and `1.11.240.0` via Address Library (`UsesAddressLibrary(true)`).
3. **LooksMenu 1.7.0.4 Support (`LooksMenuInterfaces.h`)**:
   - Added LooksMenu `1.7.0.4` PE timestamp checksum (`0x600771d`), plugin name (`"Fallout 4 Engine Extender"`), and interface offsets (`0x10DBD0`..`0x10E020` / `0xFD6D0`) while preserving backward compatibility for LooksMenu `1.6.20`.
4. **Updated Fallout 4 1.11.240.0 RVAs & DetourXS Hooks**:
   - All **DetourXS** hooks (`detourxs.cpp` + `LDE64x64.lib`) remain fully active and updated to `1.11.240.0` RVAs:
     - `LooksMenuHooks.hpp`: `ActorUpdateManager` (`TESObjectLoadedEvent`, `TESInitScriptEvent`, `Flush`) and `OverlayInterface::RemoveOverlay`.
     - `MainMenuHandler.cpp`: `MainMenu::ctor` (`0x12A2B20` -> `0x106E5F0`).
     - `Patches.cpp` & `PatchesMemUtils.h`:
       - `hclClothInstance::setTransformSet` overload 1 (`0x1DDC7A0` -> `0x18C8530`) & overload 2 (`0x18C85F0`).
       - `BSClothExtraData::SetSettleOnTransitionToSim` (`0x1DA7A40` -> `0x18A1B60`).
       - `BSClothUtils::BSTransformSet::QClothSupportsLOD` (`0x1DAF3B0` -> `0x18A79F0`).
       - `BSClothUtils::BSTransformSet::ResetPrediction` (`0x1DADA50` -> `0x18A6070`).
       - `BSClothExtraData::SetWorld` (`0x1DA6C30` -> `0x18A0DA0`): native implementation with dynamic TLS index resolution (`ResolveGameTlsIndex` accepting valid TLS index `0`) and null transformSet guards.
       - `TESNPC::ChangeHeadPart` (`0x5B9B30` -> `0x6599B0`), `TESNPC::ChangeHeadPartRemovePart` (`0x5B5560` -> `0x6555A0`), and `TESNPC::AddHeadPart` (`0x5B52E0` -> `0x655350`).
     - Engine Patches & Crash Guards:
       - `FaceGenNullGuard.cpp`: Guards `BSFaceGenManager::PrepareHeadPartModel` (`0x6E0880`) against null `a_headPart` or `a_headData` dereferences, preventing cell transition crashes without corrupting facegen geometries.
       - `FacialMorphIntensityFix.cpp`: Normalizes Facial Morph Intensity to 1.0f fleet-wide (NOPs `0x6503F2` `SetFacialBoneMorphIntensity` and overrides `0x654E20` `TESNPC::GetFacialBoneMorphIntensity` to return 1.0f), eliminating chargen face morph distortion across all 657 vanilla, DLC, and modded NPCs without requiring ESP/ESM plugin edits.
       - `ClothMorphFilter.cpp`: Hooks into F4EE morph dispatch (`bStripClothBodyMorphs=true`) to detect Havok cloth physics meshes (`isClothPhysicsNode`) and prevent non-zero body morph deltas from deforming physics garments.
   - Updated direct engine call RVAs:
     - `ActorResetQueue.cpp`: `TESNPC::GetBodyTintColor` (`0x5B58E0` -> `0x655910`), `ApplySkinTintToTree` (`0x68C530` -> `0x6EF070`), native `paintNodeSkinTint` (`BSVisit::VisitSkinTint` at `0x6EF070` + `BSVisit::RefreshFaceAndSkinRenderState` at `0x6E1140`).
     - `ActorHighlight.cpp`: `TESObjectREFR::ApplyEffectShader` (`0x422180` -> `0x5E21A0` / `NG_ID 2205201`).
     - `ActorsManager.cpp`: `TESObjectREFR` event source RVAs (`0x436A40`/`0x436A70`/`0x436590`/`0x4365C0` -> `0x52D940`/`0x52D970`/`0x52D560`/`0x52D590`).

---

## 2. Building From Source (`tools/build_dbr.py`)

```bash
python3 tools/build_dbr.py
```

Compiles all translation units (including `CommonLibF4` with `-DF4_GAME_VERSION_AE` and `DetourXS`) using `clang-cl` and links `DiverseBodiesRedux.dll` via `lld-link`.
