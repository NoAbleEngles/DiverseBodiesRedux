# Diverse Bodies Redux - Fallout 4 Next-Gen (1.11.240.0) Port & Binary Patching Architecture

## 1. Executive Summary

Upstream **Diverse Bodies Redux (DBR)** (v2.7.1) was compiled against Fallout 4 runtime **1.10.163.0** and LooksMenu **1.6.20**. When loaded on Fallout 4 Next-Gen (**1.11.240.0**) with F4SE **0.7.2** and LooksMenu **1.7.0.4**, the unmodified plugin fails immediately with multiple fatal access violations, infinite spin-locks, or silent CTDs during actor 3D initialization.

This repository branch (`next-gen-port`) provides:
1. **Automated Binary Patcher (`tools/generate_patched_dll.py`)**: Transforms the pre-compiled upstream `DiverseBodiesRedux.dll` into a fully functioning Next-Gen native DLL (`DiverseBodiesRedux_NG.dll`) with an injected `.dbrng` PE section.
2. **Preset & Silhouette Integration Compiler (`tools/compile_silhouette_to_dbr_and_bodygen.py`)**: Strips embedded morphs from head presets and synthesizes BodyGen/DBR presets across all factions and NPC archetypes.
3. **Source Code Modernization**: UTF-8 conversion of all upstream sources and alignment with Next-Gen CommonLibF4 and Address Library standards.

---

## 2. Incompatibility Root Causes & Fix Mechanics

### A. LooksMenu 1.7.0.4 Checksum & Interface Offsets
- **Root Cause**: DBR inlines 4 separate verification routines (`InitializeInterfaces`) that compute an Adler/CRC checksum of `f4ee.dll` and fetch raw interface pointers at hardcoded offsets.
- **Upstream values (1.6.20)**:
  - Checksum: `0x59F7081`
  - `BodyMorphInterface`: `0xF4DF0`
  - `OverlayInterface`: `0xF5040`
  - `SkinInterface`: `0xF4F60`
  - `CharGenInterface`: `0xF4BC0`
  - `ActorUpdateManager`: `0xF4C70`
- **Next-Gen values (1.7.0.4)**:
  - Checksum: `0x600771d`
  - `BodyMorphInterface`: `0x10DD30`
  - `OverlayInterface`: `0x10DEA0`
  - `SkinInterface`: `0x10E020`
  - `CharGenInterface`: `0x10DBD0`
  - `ActorUpdateManager`: `0xFD6D0`
- **Plugin Name Mismatch**: LooksMenu 1.7.0.4 registers itself with F4SE as `"Fallout 4 Engine Extender"`, whereas DBR calls `GetPluginInfo("F4EE")`. The patch NOPs out the `je` branch (`0x0F 0x84 0xCA 0x06 0x00 0x00` $\to$ `90 * 6`) across all 4 call sites (`0x171e8`, `0x17988`, `0x5b428`, `0x5bbc8`).
- **Spin-Lock Removal**: Bypassed busy-spin loops in `LooksMenuInterfaces<T>::GetInterface()` at `0x1658a`, `0x1663a`, and `0x5a47a` (`74 a4` $\to$ `90 90`).

### B. DetourXS Engine Hook Incompatibilities
- **Root Cause**: Upstream DBR utilized DetourXS to place runtime jumps on internal Fallout 4 engine functions. On Next-Gen (1.11.240.0), these detours fail catastrophically:
  1. `TESNPC::ChangeHeadPartRemovePart`: The prologue contains a RIP-relative conditional branch (`je 0x140655755`). When copied directly into the heap trampoline by DetourXS without RIP recalculation, it branches into unmapped process memory, resulting in an immediate CTD.
  2. `MainMenu::ctor`: The detour hook stomps `rax`, corrupting the `MainMenu` singleton pointer.
  3. Havok Cloth Physics Hooks (`hclClothInstance::setTransformSet`, `BSClothUtils`): Low-level Havok calling convention changes cause unaligned SIMD faults.
- **Fix**: Safely neutralized/NOP'd out all dangerous DetourXS hooks in `generate_patched_dll.py` (`SetTransformSet`, `SetSettle`, `QClothSupportsLOD`, `ResetPrediction`, `SetWorld`, `RemoveOverlayHook`, `ChangeHeadPart`, `ChangeHeadPartRemovePart`, and CRT startup detours).

### C. Direct `REL::Offset` Engine Call Sites (22 sites)
- Upstream DBR hardcoded 22 raw function RVAs without going through Address Library ID resolution.
- All 22 call sites are re-addressed in `generate_patched_dll.py` to their verified 1.11.240.0 addresses:
  - `AIProcess::GetAll3DUpdateFlags`: `0x00E749A0` $\to$ `0x00D36440` (4 sites)
  - `AIProcess::Set3DUpdateFlag`: `0x00E74920` $\to$ `0x00D363C0` (4 sites)
  - `TESObjectCELL::IsCellLoaded`: `0x003F4D40` $\to$ `0x004FBB90` (2 sites; also changed register operand to pass `furnRef` in `rdi` instead of `rbx`)
  - `Actor::ClearPath`: `0x00DAEEC0` $\to$ `0x00C9BEA0` (2 sites)
  - `AIProcess::GetHeadNode`: `0x00E79180` $\to$ `0x00D3AE90`
  - `NiObjectNET::GetExtraData`: `0x01B97A10` $\to$ `0x016BD5D0` (2 sites)
  - `BGSCharacterTint::Entries::GetEntryByID`: `0x002A4780` $\to$ `0x003FE800` (2 sites)
  - `BGSCharacterTint::Entries::Add`: `0x002A45B0` $\to$ `0x003FE630`
  - `TESNPC::MorphHelper_*`: `0x005B4E40`, `0x005B4890`, `0x005B4790`, `0x005B4C30` $\to$ `0x00654EC0`, `0x006549C0`, `0x00654880`, `0x00654DC0`
  - `TESObjectREFR_Events`: `0x00436a40`, `0x00436590` $\to$ `0x0052D940`, `0x0052D560`

### D. Dynamic Relocation Resolution Layer (`dbrng_rel_lookup`)
- DBR makes 80+ queries to the old Address Library database.
- A custom 169-byte assembly trampoline (`dbrng_rel_lookup`) and 87-entry lookup table are embedded into the newly injected `.dbrng` section.
- Intercepts Address Library lookups at RVA `0x151b0`, translating legacy IDs to 1.11.240.0 RVAs, static dummy buffers (for removed singletons like `bFileUncacheOnMenuOpen`), or safe stub functions.

### E. Next-Gen F4SE Plugin Version Structure
- Fallout 4 Next-Gen F4SE (0.7.2+) requires plugins to export an `F4SEPluginVersionData` structure or `F4SEPlugin_Version`.
- The patch appends a `.dbrng` PE section with:
  - `F4SEPluginVersionData` initialized for Fallout 4 1.11.240.0.
  - An export directory exporting `F4SEPlugin_Load`, `F4SEPlugin_Query`, and `F4SEPlugin_Version`.
  - A trampoline that executes `F4SEPlugin_Query` (initializing spdlog file logging) prior to `F4SEPlugin_Load`.

---

## 3. How to Generate the Patched DLL

```bash
# Requirements: python3, pefile, capstone
pip install pefile capstone

# Run the patcher
python3 tools/generate_patched_dll.py
```

The script reads the original `DiverseBodiesRedux.dll` and outputs `DiverseBodiesRedux_NG.dll`. Deploy this file to `Data/F4SE/Plugins/DiverseBodiesRedux.dll`.
