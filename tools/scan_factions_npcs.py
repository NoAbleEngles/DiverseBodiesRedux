#!/usr/bin/env python3
import os, struct, zlib, json

data_dir = "/home/lavac/.local/share/Steam/steamapps/common/Fallout 4/Data"
plugins_txt = "/home/lavac/.local/share/Steam/steamapps/compatdata/377160/pfx/drive_c/users/steamuser/AppData/Local/Fallout4/plugins.txt"
sil_dir = "/home/lavac/.gemini/antigravity/brain/f414056b-e88d-4f2a-8025-fc9a9dedef6e/scratch/silhouette"

base_masters = [
    "Fallout4.esm",
    "DLCRobot.esm",
    "DLCworkshop01.esm",
    "DLCCoast.esm",
    "DLCworkshop02.esm",
    "DLCworkshop03.esm",
    "DLCNukaWorld.esm",
]

# Also include cc* files in Data/
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

# Map lowercase plugin name -> exact case filename in Data/
disk_files = {f.lower(): f for f in os.listdir(data_dir)}

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
                # If top-level group (gtype == 0), skip if label not in target_types
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
                    if rflags & 0x00040000: # compressed
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
facts = {}   # (plugin, local_id) -> edid
races = {}   # (plugin, local_id) -> edid
npcs = {}    # (plugin, local_id) -> dict
lvlns = {}   # (plugin, local_id) -> dict

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
            })
            snams = []
            has_snam = False
            for st, sd in parse_subrecords(raw):
                if st == b"EDID":
                    entry["edid"] = sd.rstrip(b"\x00").decode("utf-8", errors="ignore")
                elif st == b"ACBS" and len(sd) >= 20:
                    entry["acbs_flags"] = struct.unpack_from("<I", sd, 0)[0]
                    entry["template_flags"] = struct.unpack_from("<H", sd, 18)[0]
                elif st == b"RNAM" and len(sd) >= 4:
                    entry["rnam"] = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
                elif st == b"SNAM" and len(sd) >= 4:
                    has_snam = True
                    f_ref = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
                    if f_ref:
                        snams.append(f_ref)
                elif st == b"TPLT" and len(sd) >= 4:
                    entry["tplt"] = resolve_fid(struct.unpack_from("<I", sd, 0)[0], masters, p)
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

print(f"Scanned {len(active_plugins)} plugins: {len(facts)} FACTs, {len(races)} RACEs, {len(npcs)} NPC_s, {len(lvlns)} LVLNs")

# Check Silhouette's factions.json against our extracted FACTs
with open(os.path.join(sil_dir, "factions.json")) as f:
    factions_data = json.load(f)

edid_to_fact = {edid.lower(): k for k, edid in facts.items()}
for fk, fv in factions_data["factions"].items():
    matched = []
    for f_plug, f_edid in fv["factions"]:
        fk_id = edid_to_fact.get(f_edid.lower())
        matched.append((f_plug, f_edid, fk_id))
    print(f"Faction {fk:12s}: {matched}")
EOF