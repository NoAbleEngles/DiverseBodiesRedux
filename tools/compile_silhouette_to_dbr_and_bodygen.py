#!/usr/bin/env python3
import os, re, json, struct, zlib

data_dir = "/home/lavac/.local/share/Steam/steamapps/common/Fallout 4/Data"
plugins_txt = "/home/lavac/.local/share/Steam/steamapps/compatdata/377160/pfx/drive_c/users/steamuser/AppData/Local/Fallout4/plugins.txt"
sil_dir = "/home/lavac/.gemini/antigravity/brain/f414056b-e88d-4f2a-8025-fc9a9dedef6e/scratch/silhouette"

dbr_mod_base = "/home/lavac/Games/Amethyst/Fallout 4/profiles/Such_Fallout_4/mods/Diverse Bodies Redux/DiverseBodiesRedux"
dbr_data_base = os.path.join(data_dir, "DiverseBodiesRedux")

templates_ini_path = "/home/lavac/Games/Amethyst/Fallout 4/profiles/Such_Fallout_4/mods/Real Body Randomizer CBBE/Data/F4SE/Plugins/F4EE/BodyGen/Fallout4.esm/templates.ini"
morphs_ini_path = "/home/lavac/Games/Amethyst/Fallout 4/profiles/Such_Fallout_4/mods/CBBE Morphs NO Balloons (Immersion Version)/Data/F4SE/Plugins/F4EE/BodyGen/Fallout4.esm/morphs.ini"

# 1. Strip embedded BodyMorphs from DBR Head presets so Head randomizer only touches heads/faces
stripped_heads = 0
for sub in ["HeadsFemale", "HeadsMale", "HeadsFemaleRaiders", "HeadsFemaleSlaves", "HeadsFemaleFanatics"]:
    p = os.path.join(dbr_mod_base, sub)
    if not os.path.exists(p):
        continue
    for fn in sorted(os.listdir(p)):
        if fn.endswith(".json") and not fn.endswith("conds.json"):
            fp = os.path.join(p, fn)
            with open(fp) as f:
                d = json.load(f)
            if d.get("BodyMorphs") is not None:
                d["BodyMorphs"] = None
                with open(fp, "w") as f:
                    json.dump(d, f, indent=4)
                stripped_heads += 1
print(f"[1/5] Ensured embedded BodyMorphs=null across DBR Head preset folders (updated {stripped_heads} files).")

# 2. Scan active plugins in load order
base_masters = [
    "Fallout4.esm",
    "DLCRobot.esm",
    "DLCworkshop01.esm",
    "DLCCoast.esm",
    "DLCworkshop02.esm",
    "DLCworkshop03.esm",
    "DLCNukaWorld.esm",
]
cc_files = sorted([f for f in os.listdir(data_dir) if f.lower().startswith("cc") and f.lower().endswith((".esl", ".esm", ".esp"))])
active_plugins = list(base_masters) + [c for c in cc_files if c not in base_masters]
if os.path.exists(plugins_txt):
    with open(plugins_txt, errors="ignore") as f:
        for line in f:
            line = line.strip()
            if line.startswith("*"):
                p = line[1:]
                if p not in active_plugins and os.path.exists(os.path.join(data_dir, p)):
                    active_plugins.append(p)

disk_files = {f.lower(): f for f in os.listdir(data_dir)}
active_map = {p.lower(): p for p in active_plugins}

def resolve_fid(fid, masters, current_plugin):
    if fid == 0:
        return None
    idx = (fid >> 24) & 0xFF
    local_id = fid & 0x00FFFFFF
    if idx < len(masters):
        m_name = disk_files.get(masters[idx].lower(), masters[idx])
        return (m_name, local_id)
    else:
        return (current_plugin, local_id)

def parse_subrecords(raw):
    pos = 0
    n = len(raw)
    xxxx_size = None
    while pos + 6 <= n:
        stype = raw[pos:pos+4]
        sz = struct.unpack_from("<H", raw, pos+4)[0]
        pos += 6
        if stype == b"XXXX" and sz == 4:
            xxxx_size = struct.unpack_from("<I", raw, pos)[0]
            pos += 4
            continue
        if xxxx_size is not None:
            sz = xxxx_size
            xxxx_size = None
        sdata = raw[pos:pos+sz]
        pos += sz
        yield stype, sdata

def scan_plugin(plugin_name, target_types={b"FACT", b"NPC_", b"LVLN", b"RACE"}):
    path = os.path.join(data_dir, plugin_name)
    with open(path, "rb") as f:
        buf = f.read()
    if len(buf) < 24 or buf[:4] != b"TES4":
        return None, [], []
    tes4_size, tes4_flags = struct.unpack_from("<II", buf, 4)
    is_esl = bool(tes4_flags & 0x200) or plugin_name.lower().endswith(".esl")
    masters = []
    for stype, sdata in parse_subrecords(buf[24:24+tes4_size]):
        if stype == b"MAST":
            masters.append(sdata.rstrip(b"\x00").decode("utf-8", errors="ignore"))

    records = []
    def walk_range(start, end):
        pos = start
        while pos + 24 <= end:
            rtype = buf[pos:pos+4]
            if rtype == b"GRUP":
                gsize = struct.unpack_from("<I", buf, pos+4)[0]
                glabel = buf[pos+8:pos+12]
                gtype = struct.unpack_from("<I", buf, pos+12)[0]
                if gsize < 24:
                    break
                if gtype == 0 and glabel not in target_types:
                    pos += gsize
                    continue
                walk_range(pos + 24, min(end, pos + gsize))
                pos += gsize
            else:
                rsize, rflags, rfid = struct.unpack_from("<III", buf, pos+4)
                pos_next = pos + 24 + rsize
                if rtype in target_types:
                    raw = buf[pos+24:pos_next]
                    if rflags & 0x00040000:
                        if len(raw) >= 4:
                            try:
                                raw = zlib.decompress(raw[4:])
                            except Exception:
                                raw = b""
                    records.append((rtype, rflags, rfid, raw))
                pos = pos_next

    walk_range(24 + tes4_size, len(buf))
    return is_esl, masters, records

plugin_is_esl = {}
facts = {}
races = {}
npcs = {}
lvlns = {}

for p in active_plugins:
    res = scan_plugin(p)
    if res[0] is None:
        continue
    is_esl, masters, recs = res
    plugin_is_esl[p] = is_esl
    for rtype, rflags, rfid, raw in recs:
        key = resolve_fid(rfid, masters, p)
        if not key:
            continue
        if rtype == b"FACT":
            for st, sd in parse_subrecords(raw):
                if st == b"EDID":
                    facts[key] = sd.rstrip(b"\x00").decode("utf-8", errors="ignore")
        elif rtype == b"RACE":
            for st, sd in parse_subrecords(raw):
                if st == b"EDID":
                    races[key] = sd.rstrip(b"\x00").decode("utf-8", errors="ignore")
        elif rtype == b"NPC_":
            entry = npcs.get(key, {
                "edid": "",
                "acbs_flags": 0,
                "template_flags": 0,
                "rnam": None,
                "snams": [],
                "tplt": None,
                "tptas": [],
            })
            snams = []
            has_snam = False
            for st, sd in parse_subrecords(raw):
                if st == b"EDID":
                    entry["edid"] = sd.rstrip(b"\x00").decode("utf-8", errors="ignore")
                elif st == b"ACBS" and len(sd) >= 20:
                    entry["acbs_flags"] = struct.unpack_from("<I", sd, 0)[0]
                    entry["template_flags"] = struct.unpack_from("<H", sd, 14)[0]
                elif st == b"RNAM" and len(sd) >= 4:
                    entry["rnam"] = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
                elif st == b"SNAM" and len(sd) >= 4:
                    has_snam = True
                    f_ref = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
                    if f_ref:
                        snams.append(f_ref)
                elif st == b"TPLT" and len(sd) >= 4:
                    entry["tplt"] = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
                elif st == b"TPTA" and len(sd) >= 4:
                    entry["tptas"] = [resolve_fid(struct.unpack_from("<I", sd, off)[0], masters, p)
                                      for off in range(0, len(sd) - 3, 4)]
            if has_snam:
                entry["snams"] = snams
            npcs[key] = entry
        elif rtype == b"LVLN":
            entry = lvlns.get(key, {"edid": "", "lvlos": []})
            lvlos = []
            has_lvlo = False
            for st, sd in parse_subrecords(raw):
                if st == b"EDID":
                    entry["edid"] = sd.rstrip(b"\x00").decode("utf-8", errors="ignore")
                elif st == b"LVLO" and len(sd) >= 8:
                    has_lvlo = True
                    ref = resolve_fid(struct.unpack_from("<I", sd, 4)[0], masters, p)
                    if ref:
                        lvlos.append(ref)
            if has_lvlo:
                entry["lvlos"] = lvlos
            lvlns[key] = entry

def format_local_fid(plugin, raw_local_id):
    if plugin_is_esl.get(plugin, False):
        return f"{raw_local_id & 0xFFF:X}"
    return f"{raw_local_id & 0xFFFFFF:X}"

with open(os.path.join(sil_dir, "pool.json")) as f:
    pool_data = json.load(f)
with open(os.path.join(sil_dir, "factions.json")) as f:
    factions_data = json.load(f)
with open(os.path.join(sil_dir, "characters.json")) as f:
    chars_data = json.load(f)

edid_to_fact = {edid.lower(): k for k, edid in facts.items()}
fact_to_sil = {}
for fk, fv in factions_data["factions"].items():
    for f_plug, f_edid in fv["factions"]:
        fk_id = edid_to_fact.get(f_edid.lower())
        if fk_id:
            fact_to_sil[fk_id] = fk

extra_subfactions = {
    "bos": ["BoSGenericNPCFaction", "BoSSquireFaction", "SS2C3_BoSRemnantsFaction", "BunkerHillBoSFaction"],
    "minutemen": ["PotentialMinutemen", "Min02_MinutemenFaction", "RESC_FactionMinutemenFaction"],
    "gunner": ["DN102_FrenziedGunnerFaction", "SS2_C2_AlgernonGunnerFaction", "SS2C2_MQ16_ConcordGunnerFaction", "SS2C2_Vault75_GunnerFaction", "SS2C2_WesGunnerFaction", "SS2C3_LamontGunnerFaction"],
    "triggermen": ["TriggermanFriendFaction", "MS16TriggermanFaction"],
    "raiders": ["DLC01RaiderFaction", "DLC03TrapperFaction", "DLC03TrapperFactionCoast", "DLC03TrapperFactionWoods", "TheForgedFaction", "CZRaiderFaction", "DLC04CommonwealthRaiderFaction", "SS2_MQ04RaiderFaction", "SS2C3_Faction_CainRaiders", "SS2C3_BostonBattersFaction"],
    "institute": ["BunkerHillInstituteFaction", "InstituteMaintenanceFaction", "InstituteRefugeeFaction"],
    "railroad": ["RailroadGenericNPCFaction", "RailroadOvertAgentFaction"],
    "atom": ["DLC03ChildrenofAtomGenericNPCFaction", "DN166AtomFaction", "SS2_AtomAcolyteFaction"],
}
for fk, edids in extra_subfactions.items():
    for ed in edids:
        fk_id = edid_to_fact.get(ed.lower())
        if fk_id:
            fact_to_sil[fk_id] = fk

USE_TRAITS, USE_FACTIONS = 0x0001, 0x0004
SLOT_TRAITS, SLOT_FACTIONS = 0, 2

def template_for(n, flag, slot):
    if not (n["template_flags"] & flag):
        return None
    if len(n["tptas"]) > slot and n["tptas"][slot]:
        return n["tptas"][slot]
    return n["tplt"]

def _walk(npc_key, flag, slot, own_fn, visited):
    if npc_key in visited:
        return set()
    visited.add(npc_key)
    if npc_key in npcs:
        n = npcs[npc_key]
        t = template_for(n, flag, slot)
        if t:
            return _walk(t, flag, slot, own_fn, visited)
        return own_fn(n)
    if npc_key in lvlns:
        res = set()
        for c in lvlns[npc_key]["lvlos"]:
            res |= _walk(c, flag, slot, own_fn, visited)
        return res
    return set()

def get_npc_races(k):
    return _walk(k, USE_TRAITS, SLOT_TRAITS,
                 lambda n: {races[n["rnam"]]} if n["rnam"] in races else set(), set())

def get_npc_factions(k):
    return _walk(k, USE_FACTIONS, SLOT_FACTIONS,
                 lambda n: {fact_to_sil[f] for f in n["snams"] if f in fact_to_sil}, set())

faction_priority = [
    "disciples",
    "operators",
    "pack",
    "triggermen",
    "atom",
    "railroad",
    "institute",
    "bos",
    "gunner",
    "minutemen",
    "raiders",
]

# Resolve every adult HumanRace NPC_ to its winning Silhouette faction
# Note: Because f4ee.dll maintains completely separate Male and Female hash maps,
# and template/leveled-chain NPC_ records (like EncRaider01Template or LvlRaiderMixed)
# are walked at runtime for both Female and Male spawned actors, registering both
# |Female and |Male on template/mixed NPCs (and matching sex on fixed NPCs, or both maps)
# guarantees 100% coverage!
faction_npc_targets = {fk: [] for fk in factions_data["factions"]}
for nk in sorted(npcs.keys(), key=lambda x: (active_plugins.index(x[0]) if x[0] in active_plugins else 999, x[1])):
    if nk[0] not in active_map.values():
        continue
    if nk[0] == "Fallout4.esm" and (nk[1] & 0xFFFFFF) in (0x7, 0xA7D34, 0xA7D35):
        continue
    rset = get_npc_races(nk)
    if "HumanRace" not in rset or "HumanChildRace" in rset:
        continue
    fset = get_npc_factions(nk)
    if not fset:
        continue
    winning_fk = None
    for p_fk in faction_priority:
        if p_fk in fset:
            winning_fk = p_fk
            break
    if winning_fk:
        faction_npc_targets[winning_fk].append((nk[0], nk[1], npcs[nk]["edid"]))

print("[2/5] Resolved adult HumanRace NPC_ records per faction (with TPLT + TPTA):")
for fk in factions_data["factions"]:
    print(f"  {fk:12s}: {len(faction_npc_targets[fk]):4d} NPC_ records")

def convert_sliders(val_dict):
    out = {}
    for k, v in val_dict.items():
        if k in ("DoubleMelon", "BreastFantasy"):
            continue
        fv = round(float(v) / 100.0, 4)
        if k == "Breasts" and fv > 0.55:
            fv = 0.55
        if fv != 0.0:
            out[k] = fv
    return out

def format_template_rhs(norm_dict, is_female=True):
    if not norm_dict:
        return "Ankles@0.00:0.00" if is_female else "BTAbDefinition@0.00:0.00"
    parts = [f"{k}@{v:.2f}:{v:.2f}" for k, v in sorted(norm_dict.items())]
    return ",".join(parts)

# 3. Write DBR JSON files + _conds.json in BodymorphsFemale and BodymorphsMale and sync symlinks
fem_mod_dir = os.path.join(dbr_mod_base, "BodymorphsFemale")
mal_mod_dir = os.path.join(dbr_mod_base, "BodymorphsMale")
fem_data_dir = os.path.join(dbr_data_base, "BodymorphsFemale")
mal_data_dir = os.path.join(dbr_data_base, "BodymorphsMale")

for d in (fem_mod_dir, mal_mod_dir, fem_data_dir, mal_data_dir):
    if os.path.exists(d):
        for fn in os.listdir(d):
            if fn.startswith("Sil_"):
                os.remove(os.path.join(d, fn))

def write_dbr_preset(is_female, base_name, norm_sliders, conds_obj=None):
    target_dir = fem_mod_dir if is_female else mal_mod_dir
    safe = re.sub(r"[^A-Za-z0-9_\-]", "_", base_name)
    json_fn = f"{safe}.json"
    json_path = os.path.join(target_dir, json_fn)
    payload = {
        "BodyMorphs": norm_sliders,
        "Gender": 1 if is_female else 0
    }
    with open(json_path, "w") as f:
        json.dump(payload, f, indent=4)
    if conds_obj is not None:
        conds_fn = f"{safe}_conds.json"
        conds_path = os.path.join(target_dir, conds_fn)
        with open(conds_path, "w") as f:
            json.dump(conds_obj, f, indent=4)

# 3a. General Pool -> DBR JSON
for pname, pinfo in pool_data["presets"].items():
    is_fem = (pinfo["sex"] == "female")
    norm_s = convert_sliders(pinfo["values"])
    write_dbr_preset(is_fem, f"Sil_Pool_{pname}", norm_s, None)

# 3b. Faction Pool -> DBR JSON + _conds.json
faction_in_faction_strs = {}
for fk, fv in factions_data["factions"].items():
    inf_list = []
    for f_plug, f_edid in fv["factions"]:
        fk_id = edid_to_fact.get(f_edid.lower())
        if fk_id:
            f_hex = format_local_fid(fk_id[0], fk_id[1])
            inf_list.append(f"0x{f_hex}|{fk_id[0]}")
    faction_in_faction_strs[fk] = inf_list

for pname, pinfo in factions_data["presets"].items():
    is_fem = (pinfo["sex"] == "female")
    fk = pinfo["faction"]
    norm_s = convert_sliders(pinfo["values"])
    f_fids = [
        f"0x{format_local_fid(plug, lid)}|{plug}"
        for plug, lid, _ in faction_npc_targets.get(fk, [])
    ]
    conds = {
        "gender": 1 if is_fem else 0,
        "formIDs": f_fids,
        "notFormIDs": [],
        "inFaction": faction_in_faction_strs.get(fk, []),
        "notInFaction": [],
        "hasKeyword": [],
        "hasNotKeyword": []
    }
    write_dbr_preset(is_fem, f"Sil_Faction_{pname}", norm_s, conds)

# 3c. Characters -> DBR JSON + _conds.json
for cname, cinfo in chars_data["characters"].items():
    is_fem = (cinfo["sex"] == "female")
    norm_s = convert_sliders(cinfo["values"])
    c_fids = []
    for plug, edid, fid_hex in cinfo["forms"]:
        real_plug = active_map.get(plug.lower(), plug)
        fid_int = int(fid_hex, 16)
        f_hex = format_local_fid(real_plug, fid_int)
        c_fids.append(f"0x{f_hex}|{real_plug}")
    conds = {
        "gender": 1 if is_fem else 0,
        "formIDs": c_fids,
        "notFormIDs": [],
        "inFaction": [],
        "notInFaction": [],
        "hasKeyword": [],
        "hasNotKeyword": []
    }
    write_dbr_preset(is_fem, f"Sil_Char_{cname}", norm_s, conds)

for mod_d, data_d in [(fem_mod_dir, fem_data_dir), (mal_mod_dir, mal_data_dir)]:
    os.makedirs(data_d, exist_ok=True)
    for fn in sorted(os.listdir(mod_d)):
        src = os.path.join(mod_d, fn)
        dst = os.path.join(data_d, fn)
        if not os.path.lexists(dst):
            os.symlink(src, dst)

print(f"[3/5] Wrote DBR JSONs & _conds.json: BodymorphsFemale={len(os.listdir(fem_mod_dir))} files, BodymorphsMale={len(os.listdir(mal_mod_dir))} files.")

# 4. Update BodyGen templates.ini
with open(templates_ini_path) as f:
    orig_templates = f.read()

marker = "# === SILHOUETTE TEMPLATES (POOL + FACTIONS + CHARACTERS) ==="
if marker in orig_templates:
    base_templates = orig_templates.split(marker)[0].rstrip()
else:
    base_templates = orig_templates.rstrip()

sil_template_lines = [
    "",
    marker,
    "# Auto-generated from ReidenXerx/fo4-silhouette (pool.json, factions.json, characters.json)",
]

for pname, pinfo in pool_data["presets"].items():
    is_fem = (pinfo["sex"] == "female")
    tname = f"Sil_{pname}"
    rhs = format_template_rhs(convert_sliders(pinfo["values"]), is_fem)
    sil_template_lines.append(f"{tname}={rhs}")

for pname, pinfo in factions_data["presets"].items():
    is_fem = (pinfo["sex"] == "female")
    tname = f"Sil_{pname}"
    rhs = format_template_rhs(convert_sliders(pinfo["values"]), is_fem)
    sil_template_lines.append(f"{tname}={rhs}")

for cname, cinfo in chars_data["characters"].items():
    is_fem = (cinfo["sex"] == "female")
    tname = f"Sil_Char_{cname}"
    rhs = format_template_rhs(convert_sliders(cinfo["values"]), is_fem)
    sil_template_lines.append(f"{tname}={rhs}")

with open(templates_ini_path, "w") as f:
    f.write(base_templates + "\n" + "\n".join(sil_template_lines) + "\n")

print(f"[4/5] Updated templates.ini with {len(pool_data['presets']) + len(factions_data['presets']) + len(chars_data['characters'])} Silhouette templates.")

# 5. Build BodyGen morphs.ini with 4-tier priority hierarchy
pool_weights = pool_data["weights"]
fem_pool_weighted = []
mal_pool_weighted = []
for pname, pinfo in pool_data["presets"].items():
    w = pool_weights.get(pinfo["tier"], 1)
    tname = f"Sil_{pname}"
    if pinfo["sex"] == "female":
        fem_pool_weighted.extend([tname] * w)
    else:
        mal_pool_weighted.extend([tname] * w)

existing_curated_female = [
    "tmbv2", "Victoria", "Rhaenys", "Rhaenys [Outfit]", "Pear (Outfit)", "Pear",
    "NoirOutfit", "NoirNudeBody", "Natural Curves", "Natural Curves(clothes)",
    "CBBE Slim (Outfit)", "CBBE Petite (Outfit)", "CBBE Curvy (Outfit)",
    "[VM] Vanilla+ BASE (outfit)", "Realistic (Small) (O)"
]
all_female_human_rhs = "|".join(fem_pool_weighted + existing_curated_female)
all_male_human_rhs = "|".join(mal_pool_weighted)

fac_weights = factions_data["weights"]
faction_rhs = {fk: {"female": [], "male": []} for fk in factions_data["factions"]}
for pname, pinfo in factions_data["presets"].items():
    fk = pinfo["faction"]
    w = fac_weights.get(pinfo["tier"], 1)
    tname = f"Sil_{pname}"
    faction_rhs[fk][pinfo["sex"]].extend([tname] * w)

morph_lines = [
    "# =============================================================================",
    "# BODYGEN MORPHS.INI — SILHOUETTE + CURATED OVERRIDES",
    "# Priority Order (later lines overwrite earlier lines in f4ee.dll):",
    "#   Tier 1: General Race Pools (All|Sex|Race)",
    "#   Tier 2: Faction Pool Override Selectors (Plugin|FormID|Sex across load order)",
    "#   Tier 3: Named Character Override Selectors (207 Silhouette + Custom NPCs)",
    "#   Tier 4: Player & Pre-War Spouse Exclusions (No Change)",
    "# =============================================================================",
    "",
    "# --- TIER 1: GENERAL RACE POOLS ---",
    "All|Female|GhoulRace=CBBE Slim (Outfit)|CBBE Petite (Outfit)",
    f"All|Female|HumanRace={all_female_human_rhs}",
    f"All|Male|HumanRace={all_male_human_rhs}",
    "",
    "# --- TIER 2: FACTION POOL OVERRIDE SELECTORS (Plugin|FormID|Sex) ---",
]

tier2_count = 0
for fk in factions_data["factions"]:
    fem_rhs_str = "|".join(faction_rhs[fk]["female"])
    mal_rhs_str = "|".join(faction_rhs[fk]["male"])
    morph_lines.append(f"# Faction: {fk} ({factions_data['factions'][fk]['look']})")
    for plug, lid, edid in faction_npc_targets[fk]:
        f_hex = format_local_fid(plug, lid)
        # Emit both |Female (populates f4ee Female map) and |Male (populates f4ee Male map)
        # so template roots (e.g. EncRaider01Template) work for both Female and Male spawned actors!
        morph_lines.append(f"{plug}|{f_hex}|Female={fem_rhs_str}")
        morph_lines.append(f"{plug}|{f_hex}|Male={mal_rhs_str}")
        tier2_count += 2
    morph_lines.append("")

morph_lines.append("# --- TIER 3: NAMED CHARACTER OVERRIDE SELECTORS ---")
tier3_active = 0
tier3_commented = 0
silhouette_covered_forms = set()

for cname, cinfo in chars_data["characters"].items():
    sex_label = "Female" if cinfo["sex"] == "female" else "Male"
    tname = f"Sil_Char_{cname}"
    for plug, edid, fid_hex in cinfo["forms"]:
        fid_int = int(fid_hex, 16)
        if plug.lower() in active_map:
            real_plug = active_map[plug.lower()]
            f_hex = format_local_fid(real_plug, fid_int)
            silhouette_covered_forms.add((real_plug.lower(), int(f_hex, 16)))
            morph_lines.append(f"# {cname} ({edid})")
            morph_lines.append(f"{real_plug}|{f_hex}|{sex_label}={tname}")
            tier3_active += 1
        else:
            f_hex = f"{fid_int & 0xFFFFFF:X}"
            morph_lines.append(f"# {cname} ({edid}) [unloaded mod]")
            morph_lines.append(f"#{plug}|{f_hex}|{sex_label}={tname}")
            tier3_commented += 1

extra_custom_npcs = [
    ("Companion Charlotte", "companion_charlotte.esp", 0x1740, "Female", "Charlotte_Body(clothing)"),
    ("Katy Pinn", "Fallout4.esm", 0x2A827, "Female", "PlasticSurgery BodySlide Preset"),
    ("Professor Scara", "Fallout4.esm", 0xB2FC7, "Female", "KillerCurves_Alice_SmallNips1"),
    ("Rowdy", "Fallout4.esm", 0x1DB56, "Female", "PerfectBody_SmallBoobsClothed"),
    ("Scarlet", "Fallout4.esm", 0x4B240, "Female", "PerfectBody_SmallBoobsClothed"),
    ("Roxy (Atom Cat)", "Fallout4.esm", 0x1164F7, "Female", "CBBE_CUSTOM_HOT_NUDE"),
    ("Myrna (Diamond City Surplus)", "Fallout4.esm", 0x1A054, "Female", "[VM] Vanilla+ BASE (outfit)|Realistic (Small) (O)|CBBE Slim (Outfit)"),
    ("Cathy (Diamond City Mega Surgery)", "Fallout4.esm", 0x1A057, "Female", "[VM] Vanilla+ BASE (outfit)|Realistic (Small) (O)|CBBE Petite (Outfit)"),
    ("Polly (Diamond City Choice Chops)", "Fallout4.esm", 0x1A056, "Female", "[VM] Vanilla+ BASE (outfit)|CBBE Slim (Outfit)"),
    ("Becky Fallon (Fallon's Basement)", "Fallout4.esm", 0x1A053, "Female", "[VM] Vanilla+ BASE (outfit)|CBBE Petite (Outfit)"),
]

morph_lines.append("")
morph_lines.append("# --- TIER 3B: ADDITIONAL CUSTOM NPC OVERRIDES ---")
for label, plug, lid, sex_label, rhs in extra_custom_npcs:
    if (plug.lower(), lid) in silhouette_covered_forms:
        continue
    if plug.lower() in active_map:
        real_plug = active_map[plug.lower()]
        f_hex = format_local_fid(real_plug, lid)
        morph_lines.append(f"# {label}")
        morph_lines.append(f"{real_plug}|{f_hex}|{sex_label}={rhs}")
        tier3_active += 1
    else:
        morph_lines.append(f"# {label} [unloaded mod]")
        morph_lines.append(f"#{plug}|{lid:X}|{sex_label}={rhs}")

morph_lines.extend([
    "",
    "# --- TIER 4: PLAYER & PRE-WAR SPOUSE EXCLUSIONS (ALWAYS LAST) ---",
    "Fallout4.esm|7=No Change",
    "Fallout4.esm|A7D34=No Change",
    "Fallout4.esm|A7D35=No Change",
    ""
])

with open(morphs_ini_path, "w") as f:
    f.write("\n".join(morph_lines))

print(f"[5/5] Wrote morphs.ini: {tier2_count} faction selectors, {tier3_active} active named character selectors ({tier3_commented} unloaded commented), 3 player exclusions.")
