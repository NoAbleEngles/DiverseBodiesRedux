#!/usr/bin/env python3
import os
import sys
import subprocess
import concurrent.futures

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DBR = os.path.join(REPO_ROOT, "DiverseBodiesRedux", "Sources")
CLIB_ROOT = os.environ.get("COMMONLIBF4_DIR", "/tmp/CommonLibF4_PANDA/CommonLibF4")
SRC_CLIB = os.path.join(CLIB_ROOT, "src")
BOOST_INC = os.environ.get("BOOST_INCLUDE_DIR", "/tmp/boost_inc")
XWIN_DIR = os.environ.get("XWIN_DIR", os.path.expanduser("~/.xwin"))
BUILD_DIR = os.environ.get("BUILD_DIR", "/tmp/dbr_build")
OUT_DLL = os.environ.get("OUT_DLL", os.path.join(BUILD_DIR, "DiverseBodiesRedux.dll"))

os.makedirs(BUILD_DIR, exist_ok=True)

# Generate boost_json.cpp
boost_json_cpp = os.path.join(BUILD_DIR, "boost_json.cpp")
with open(boost_json_cpp, "w") as f:
    f.write("#include <boost/json/src.hpp>\n")

sources = [
    boost_json_cpp,
    # CommonLibF4
    f"{SRC_CLIB}/F4SE/API.cpp",
    f"{SRC_CLIB}/F4SE/Impl/PCH.cpp",
    f"{SRC_CLIB}/F4SE/Impl/WinAPI.cpp",
    f"{SRC_CLIB}/F4SE/Interfaces.cpp",
    f"{SRC_CLIB}/F4SE/Logger.cpp",
    f"{SRC_CLIB}/F4SE/Trampoline.cpp",
    f"{SRC_CLIB}/REL/Relocation.cpp",
    f"{SRC_CLIB}/RE/Bethesda/Actor.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BGSCharacterTint.cpp",
    # f"{SRC_CLIB}/RE/Bethesda/BGSChargenUtils.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BGSInventoryItem.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSExtraData.cpp",
    # f"{SRC_CLIB}/RE/Bethesda/BSFaceGenAnimationData.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSFaceGenNiNode.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSResource.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSResourceNiBinaryStream.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSResource/Stream.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSResource/StreamBase.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScaleformManager.cpp",
    # f"{SRC_CLIB}/RE/Bethesda/BSScript.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/Array.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/Internal/VirtualMachine.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/Object.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/ObjectTypeInfo.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/PackedInstructionStream.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/StackFrame.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/Struct.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/StructTypeInfo.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/TypeInfo.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSScript/Variable.cpp",
    f"{SRC_CLIB}/RE/Bethesda/BSTriShape.cpp",
    f"{SRC_CLIB}/RE/Bethesda/Calendar.cpp",
    f"{SRC_CLIB}/RE/Bethesda/CRC.cpp",
    f"{SRC_CLIB}/RE/Bethesda/FormComponents.cpp",
    f"{SRC_CLIB}/RE/Bethesda/MenuCursor.cpp",
    f"{SRC_CLIB}/RE/Bethesda/TESBoundAnimObjects.cpp",
    f"{SRC_CLIB}/RE/Bethesda/TESForms.cpp",
    f"{SRC_CLIB}/RE/Bethesda/TESObjectREFRs.cpp",
    f"{SRC_CLIB}/RE/Fallout.cpp",
    f"{SRC_CLIB}/RE/NetImmerse/NiAVObject.cpp",
    f"{SRC_CLIB}/RE/NetImmerse/NiBinaryStream.cpp",
    f"{SRC_CLIB}/RE/NetImmerse/NiObjectNET.cpp",
    f"{SRC_CLIB}/RE/NetImmerse/NiRect.cpp",
    f"{SRC_CLIB}/RE/Scaleform/GFx/GFx_Player.cpp",
    # DiverseBodiesRedux
    f"{SRC_DBR}/ActorsManager/ActorResetQueue.cpp",
    f"{SRC_DBR}/ActorsManager/ActorsManager.cpp",
    f"{SRC_DBR}/ActorsManager/Details/ExcludedActors.cpp",
    f"{SRC_DBR}/detourxs/detourxs.cpp",
    f"{SRC_DBR}/DirectApply/DirectApply.cpp",
    f"{SRC_DBR}/PrismaMenu/ActorHighlight.cpp",
    f"{SRC_DBR}/PrismaMenu/MenuLocales.cpp",
    f"{SRC_DBR}/PrismaMenu/PrismaMenu.cpp",
    f"{SRC_DBR}/PrismaMenu/PrismaViewHost.cpp",
    f"{SRC_DBR}/Overrides/SessionOverrides.cpp",
    f"{SRC_DBR}/PrismaMenu/PreviewCopy.cpp",
    f"{SRC_DBR}/Hooks/Hooks.cpp",
    f"{SRC_DBR}/Ini/Ini.cpp",
    f"{SRC_DBR}/LooksMenu/LooksMenuInterfaces.cpp",
    f"{SRC_DBR}/LooksMenu/ParseLooksMenuPreset.cpp",
    f"{SRC_DBR}/main.cpp",
    f"{SRC_DBR}/MainMenuHandler/MainMenuHandler.cpp",
    f"{SRC_DBR}/Patches/ClothMorphFilter.cpp",
    f"{SRC_DBR}/Patches/ClothWorldFix.cpp",
    f"{SRC_DBR}/Patches/FaceGenNullGuard.cpp",
    f"{SRC_DBR}/Patches/FacialMorphIntensityFix.cpp",
    f"{SRC_DBR}/Patches/Patches.cpp",
    f"{SRC_DBR}/Patches/PatchesMemUtils.cpp",
    f"{SRC_DBR}/Patches/x-cell_patch.cpp",
    f"{SRC_DBR}/PresetsManager/PresetsManager.cpp",
    f"{SRC_DBR}/Preset/Bodyhairs.cpp",
    f"{SRC_DBR}/Preset/Bodymorphs.cpp",
    f"{SRC_DBR}/Preset/Details/Tint.cpp",
    f"{SRC_DBR}/Preset/Nails.cpp",
    f"{SRC_DBR}/Preset/NPCPreset.cpp",
    f"{SRC_DBR}/Preset/OverlayPreset.cpp",
    f"{SRC_DBR}/Preset/Details/Conditions.cpp",
    f"{SRC_DBR}/Preset/Details/Overlay.cpp",
    f"{SRC_DBR}/Preset/Preset.cpp",
    f"{SRC_DBR}/Preset/BodyTattoos.cpp",
    f"{SRC_DBR}/PugiXML/pugixml.cpp",
    f"{SRC_DBR}/Utils/utility.cpp",
    f"{SRC_DBR}/Validate/ValidateOverlay.cpp",
    f"{SRC_DBR}/Validate/ValidateTint.cpp",
]

def resolve_src(path):
    if path.startswith(SRC_CLIB):
        rel = os.path.relpath(path, SRC_CLIB)
        local = os.path.join(REPO_ROOT, "CommonLibF4", "src", rel)
        if os.path.exists(local):
            return local
    return path

sources = [resolve_src(s) for s in sources]

INCLUDES = [
    f"-I{SRC_DBR}",
    f"-I{SRC_DBR}/spdlog/include",
    f"-I{SRC_DBR}/mmio/include",
    f"-I{SRC_DBR}/fmt/include",
    f"-I{SRC_DBR}/zlib/include",
    f"-I{REPO_ROOT}/CommonLibF4/include",
    f"-I{CLIB_ROOT}/include",
    f"-I{BOOST_INC}",
]

DEFINES = [
    "-DNDEBUG", "-D_CONSOLE", "-DWIN32", "-D_WINDOWS", "-D_UNICODE", "-DUNICODE",
    "-DF4_GAME_VERSION_AE", "-DNOMINMAX",
    "-DBOOST_STL_INTERFACES_DISABLE_CONCEPTS", "-DBOOST_JSON_NO_LIB", "-DBOOST_CONTAINER_NO_LIB",
    "-DWINVER=0x0601", "-D_WIN32_WINNT=0x0601", "-DFMT_STATIC", "-DFMT_USE_CONSTEVAL=0",
]

WARNINGS = [
    "-Wno-pragma-once-outside-header", "-Wno-microsoft-include", "-Wno-invalid-offsetof",
    "-Wno-unused-parameter", "-Wno-deprecated-declarations", "-Wno-return-type",
    "-Wno-ignored-attributes", "-Wno-enum-compare-typo", "-Wno-inconsistent-missing-override",
    "-Wno-microsoft-template-shadow", "-Wno-pointer-to-int-cast", "-Wno-absolute-value",
]

COMMON_FLAGS = [
    "clang-cl", "/c", "/std:c++latest", "/EHsc", "/O2", "/MD",
    "-winsysroot", XWIN_DIR,
    "-Xclang", "-fdelayed-template-parsing",
] + DEFINES + WARNINGS + INCLUDES

def compile_source(src):
    base = os.path.splitext(os.path.basename(src))[0]
    parent = os.path.basename(os.path.dirname(src))
    obj_name = f"{parent}_{base}.obj" if parent not in ("src", "Sources") else f"{base}.obj"
    obj_path = os.path.join(BUILD_DIR, obj_name)

    cmd = COMMON_FLAGS + [src, f"-Fo{obj_path}"]
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if res.returncode != 0:
        return (False, src, res.stdout)
    return (True, src, obj_path)

print(f"Compiling {len(sources)} translation units with {os.cpu_count()} threads...")
objs = []
failed = False

with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
    futures = {executor.submit(compile_source, src): src for src in sources}
    for future in concurrent.futures.as_completed(futures):
        ok, src, out = future.result()
        if not ok:
            print(f"FAILED: {src}")
            print(out)
            failed = True
        else:
            objs.append(out)
            print(f"Compiled: {os.path.basename(src)}")

if failed:
    print("Build failed.")
    sys.exit(1)

print(f"All {len(objs)} objects compiled successfully! Linking {OUT_DLL}...")

LIB_PATHS = [
    f"-libpath:{XWIN_DIR}/Windows Kits/10/Lib/10.0.26100/um/x86_64",
    f"-libpath:{XWIN_DIR}/Windows Kits/10/Lib/10.0.26100/ucrt/x86_64",
    f"-libpath:{XWIN_DIR}/VC/Tools/MSVC/14.44.17.14/lib/x86_64",
]

LIBS = [
    f"{SRC_DBR}/fmt/lib/fmt.lib",
    f"{SRC_DBR}/detourxs/lib/LDE64x64.lib",
    f"{SRC_DBR}/zlib/lib/zlibstatic.lib",
    f"{SRC_DBR}/mmio/lib/mmio.lib",
    "version.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "shell32.lib",
    "ole32.lib", "oleaut32.lib", "advapi32.lib", "shlwapi.lib",
]

link_cmd = [
    "lld-link",
    "/dll",
    f"/out:{OUT_DLL}",
    "/opt:ref", "/opt:icf",
] + LIB_PATHS + objs + LIBS

link_res = subprocess.run(
    link_cmd,
    cwd=os.path.join(REPO_ROOT, "DiverseBodiesRedux"),
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True
)
print(link_res.stdout)
if link_res.returncode != 0:
    print("Link failed.")
    sys.exit(1)

print(f"SUCCESS! {OUT_DLL} created.")
