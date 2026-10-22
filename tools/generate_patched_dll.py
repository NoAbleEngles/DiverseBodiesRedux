#!/usr/bin/env python3
import struct, os, subprocess, tempfile, pefile, capstone

def build_dbr_ng(input_dll: str, output_dll: str):
    with open(input_dll, 'rb') as f:
        data = bytearray(f.read())

    # 1. Patch LooksMenu 1.7.0.4 Checksum & Offsets
    instances = [
        (0x17270, [0x17430, 0x1750e, 0x175ec, 0x176ca, 0x177a8]),
        (0x17a10, [0x17bd0, 0x17cae, 0x17d8c, 0x17e6a, 0x17f48]),
        (0x5b4b0, [0x5b670, 0x5b74e, 0x5b82c, 0x5b90a, 0x5b9e8]),
        (0x5bc50, [0x5be10, 0x5beee, 0x5bfcc, 0x5c0aa, 0x5c188]),
    ]
    NEW_CHECKSUM = 0x600771d
    NEW_BODYMORPH = 0x10DD30
    NEW_OVERLAY = 0x10DEA0
    NEW_SKIN = 0x10E020
    NEW_CHARGEN = 0x10DBD0
    NEW_ACTORUPDATE = 0xFD6D0

    for cs_off, offs in instances:
        old_cs = struct.unpack_from('<I', data, cs_off)[0]
        assert old_cs == 0x59F7081, f'Mismatch cs: {hex(old_cs)} at {hex(cs_off)}'
        struct.pack_into('<I', data, cs_off, NEW_CHECKSUM)
        struct.pack_into('<I', data, offs[0], NEW_BODYMORPH)
        struct.pack_into('<I', data, offs[1], NEW_OVERLAY)
        struct.pack_into('<I', data, offs[2], NEW_SKIN)
        struct.pack_into('<I', data, offs[3], NEW_CHARGEN)
        struct.pack_into('<I', data, offs[4], NEW_ACTORUPDATE)

    # 1b. Bypass GetPluginInfo("F4EE") check in all 4 copies of InitializeInterfaces()
    # (LooksMenu 1.7.0.4 registers as "Fallout 4 Engine Extender" instead of "F4EE",
    #  while checksum("Data/F4SE/Plugins/f4ee.dll") and GetModuleHandleA("f4ee.dll") succeed)
    for je_off in [0x171e8, 0x17988, 0x5b428, 0x5bbc8]:
        assert data[je_off:je_off+6] == b'\x0f\x84\xca\x06\x00\x00', f'Mismatch je at {hex(je_off)}'
        data[je_off:je_off+6] = b'\x90' * 6

    print('Patched LooksMenu 1.7.0.4 checksum, offsets & InitializeInterfaces check.')

    # 1c. Patch hardcoded 1.10.163.0 Fallout4.exe hook RVAs to 1.11.240.0 RVAs
    hook_patches = [
        (0x00B73, 0x012A2B20, 0x0106E5F0, 'MainMenu::ctor'),
        (0x00D07, 0x01DDC7A0, 0x018C8530, 'hclClothInstance::setTransformSet'),
        (0x00E97, 0x005B5560, 0x006555A0, 'TESNPC::ChangeHeadPartRemovePart'),
        (0x01027, 0x005B9B30, 0x006599B0, 'TESNPC::ChangeHeadPart'),
        (0x01667, 0x005B52E0, 0x00655350, 'TESNPC::AddHeadPart'),
        (0x01833, 0x01DAF3B0, 0x018A79F0, 'BSClothUtils::QClothSupportsLOD'),
        (0x018A3, 0x01DADA50, 0x018A6070, 'BSClothUtils::ResetPrediction'),
        (0x01937, 0x01DA6C30, 0x018A0DA0, 'BSClothExtraData::SetWorld (Hook)'),
        (0x01AA3, 0x0067B2D0, 0x006E0DA0, 'PendingActorHeadData::Process'),
        (0x61057, 0x01DA7A40, 0x018A1B60, 'BSClothExtraData::SetSettle'),
        (0x62F63, 0x01DA6C30, 0x018A0DA0, 'ResolveGameTlsIndex start'),
        (0x62F73, 0x01DA6C30, 0x018A0DA0, 'ResolveGameTlsIndex end'),
    ]
    for off, old_rva, new_rva, name in hook_patches:
        cur_rva = struct.unpack_from('<I', data, off)[0]
        assert cur_rva == old_rva, f'Mismatch for {name} at {hex(off)}: got {hex(cur_rva)}, expected {hex(old_rva)}'
        struct.pack_into('<I', data, off, new_rva)

    print(f'Patched {len(hook_patches)} Fallout4.exe 1.11.240.0 hook RVAs.')

    # 1d. Patch all 22 direct REL::Offset(...) instructions in DiverseBodiesRedux.dll
    rel_offset_patches = [
        (0x1EBA6, 0x00E749A0, 0x00D36440, 'AIProcess::GetAll3DUpdateFlags_1'),
        (0x1F9E5, 0x00E749A0, 0x00D36440, 'AIProcess::GetAll3DUpdateFlags_2'),
        (0x1FF30, 0x00E749A0, 0x00D36440, 'AIProcess::GetAll3DUpdateFlags_3'),
        (0x7B251, 0x00E749A0, 0x00D36440, 'AIProcess::GetAll3DUpdateFlags_4'),
        (0x1EBC5, 0x00E74920, 0x00D363C0, 'AIProcess::Set3DUpdateFlag_1'),
        (0x1FA04, 0x00E74920, 0x00D363C0, 'AIProcess::Set3DUpdateFlag_2'),
        (0x1FF4F, 0x00E74920, 0x00D363C0, 'AIProcess::Set3DUpdateFlag_3'),
        (0x7B26F, 0x00E74920, 0x00D363C0, 'AIProcess::Set3DUpdateFlag_4'),
        (0x1EC92, 0x003F4D40, 0x004FBB90, 'TESObjectCELL::IsCellLoaded_1'),
        (0x20008, 0x003F4D40, 0x004FBB90, 'TESObjectCELL::IsCellLoaded_2'),
        (0x1ECD3, 0x00DAEEC0, 0x00C9BEA0, 'Actor::ClearPath_1'),
        (0x20049, 0x00DAEEC0, 0x00C9BEA0, 'Actor::ClearPath_2'),
        (0x28680, 0x00E79180, 0x00D3AE90, 'AIProcess::GetHeadNode'),
        (0x3FEB9, 0x01B97A10, 0x016BD5D0, 'NiObjectNET::GetExtraData_1'),
        (0x3FF3F, 0x01B97A10, 0x016BD5D0, 'NiObjectNET::GetExtraData_2'),
        (0x74FC5, 0x002A4780, 0x003FE800, 'BGSCharacterTint::Entries::GetEntryByID_1'),
        (0x752FC, 0x002A4780, 0x003FE800, 'BGSCharacterTint::Entries::GetEntryByID_2'),
        (0x75120, 0x002A45B0, 0x003FE630, 'BGSCharacterTint::Entries::Add'),
        (0x7A98B, 0x005B4E40, 0x00654EC0, 'TESNPC::MorphHelper_5B4E40'),
        (0x7AA10, 0x005B4890, 0x006549C0, 'TESNPC::MorphHelper_5B4890'),
        (0x7AA57, 0x005B4790, 0x00654880, 'TESNPC::MorphHelper_5B4790'),
        (0x7B1C7, 0x005B4C30, 0x00654DC0, 'TESNPC::MorphHelper_5B4C30'),
    ]
    for off, old_rva, new_rva, name in rel_offset_patches:
        cur_rva = struct.unpack_from('<I', data, off)[0]
        assert cur_rva == old_rva, f'Mismatch for {name} at {hex(off)}: got {hex(cur_rva)}, expected {hex(old_rva)}'
        struct.pack_into('<I', data, off, new_rva)

    # Pass furnRef (rdi) instead of parentCell (rbx) to 0x4FBB90 (TESObjectREFR::IsParentCellLoaded)
    for mov_off in [0x1EC96, 0x2000C]:
        assert data[mov_off:mov_off+3] == b'\x48\x8b\xcb', f'Mismatch mov rcx, rbx at {hex(mov_off)}'
        data[mov_off:mov_off+3] = b'\x48\x8b\xcf'

    print(f'Patched {len(rel_offset_patches)} direct REL::Offset call sites.')

    # 1e. Enable early trace logging (level 0, flush_on 0) in F4SEPlugin_Query:
    # 0x53aee: b8 05 00 00 00 -> b8 00 00 00 00
    # 0x53af8: b8 05 00 00 00 -> b8 00 00 00 00
    assert data[0x53aee:0x53aee+5] == b'\xb8\x05\x00\x00\x00', 'Mismatch log level in Query'
    assert data[0x53af8:0x53af8+5] == b'\xb8\x05\x00\x00\x00', 'Mismatch flush_on in Query'
    data[0x53aee:0x53aee+5] = b'\xb8\x00\x00\x00\x00'
    data[0x53af8:0x53af8+5] = b'\xb8\x00\x00\x00\x00'

    # 1f. In initAfterGameDataWasLoaded: NOP out dangerous dereferencing of rdi (spdlog logger):
    # 0x54a48..0x54a52: 8b 00 87 47 40 8b 02 87 47 44 -> 90 * 10
    assert data[0x54a48:0x54a52] == bytes.fromhex('8b008747408b02874744'), 'Mismatch logger setter in initAfterGameDataWasLoaded'
    data[0x54a48:0x54a52] = b'\x90' * 10
    print('Guarded initAfterGameDataWasLoaded spdlog dereference crash.')

    # 1g. Patch busy spin loops in LooksMenuInterfaces<T>::GetInterface():
    # 0x1658a, 0x1663a, 0x5a47a: 74 a4 (je -0x5c) -> 90 90 (NOP NOP, return instead of spin loop)
    for spin_off in [0x1658a, 0x1663a, 0x5a47a]:
        assert data[spin_off:spin_off+2] == b'\x74\xa4', f'Mismatch spin loop je at {hex(spin_off)}'
        data[spin_off:spin_off+2] = b'\x90\x90'
    # 1h. Patch hardcoded TESObjectREFR_Events RVAs (ActorsManager::ActorsManager)
    # 0x1c502: 0x00436a40 -> 0x0052D940 (RegisterForObjectLoaded)
    # 0x1c515: 0x00436590 -> 0x0052D560 (RegisterForInitScript)
    cur_loaded = struct.unpack_from('<I', data, 0x1c502)[0]
    assert cur_loaded == 0x436a40, f'Mismatch RegisterForObjectLoaded at 0x1c502: {hex(cur_loaded)}'
    struct.pack_into('<I', data, 0x1c502, 0x52D940)

    cur_init = struct.unpack_from('<I', data, 0x1c515)[0]
    assert cur_init == 0x436590, f'Mismatch RegisterForInitScript at 0x1c515: {hex(cur_init)}'
    struct.pack_into('<I', data, 0x1c515, 0x52D560)
    print('Patched TESObjectREFR_Events RVAs (0x52D940, 0x52D560).')

    # 1i. Neutralize mainMenuLoaderHandler.Install() at file offset 0x64d9a
    # Prevents DetourXS hook on MainMenu::ctor from destroying rax and corrupting main menu instance
    assert data[0x64d9a:0x64d9a+5] == b'\xe8\x91\x50\xff\xff', 'Mismatch call mainMenuLoaderHandler.Install'
    data[0x64d9a:0x64d9a+5] = b'\x90' * 5
    print('Neutralized mainMenuLoaderHandler.Install() hook.')

    # 1j. Neutralize dangerous/broken DetourXS engine & overlay hooks
    # ChangeHeadPartRemovePart copies RIP-relative je 0x140655755 into heap trampoline causing immediate CTD.
    # Havok cloth physics hooks patch low-level engine physics functions causing desync/crashes in Next-Gen.
    dangerous_hooks = [
        (0x5500F, b'\xe8\xbc\x3f\x00\x00', 'SetTransformSet'),
        (0x5501B, b'\xe8\xd0\x3c\x00\x00', 'SetSettle'),
        (0x55027, b'\xe8\x24\x4b\x00\x00', 'QClothSupportsLOD'),
        (0x55033, b'\xe8\xf8\x4d\x00\x00', 'ResetPrediction'),
        (0x5503F, b'\xe8\xcc\x50\x00\x00', 'SetWorld'),
        (0x5508D, b'\xe8\xbe\x4a\x00\x00', 'RemoveOverlayHook'),
        (0x550DB, b'\xe8\xb0\x44\x00\x00', 'ChangeHeadPart'),
        (0x550E7, b'\xe8\xc4\x41\x00\x00', 'ChangeHeadPartRemovePart'),
    ]
    for off, expected, name in dangerous_hooks:
        assert data[off:off+5] == expected, f'Mismatch call {name} at {hex(off)}: got {data[off:off+5].hex()}'
        data[off:off+5] = b'\x90' * 5
        print(f'Neutralized {name}.Install() hook at {hex(off)}.')

    # Neutralize CRT startup DetourXS hooks at 0x1843 (cloth LOD) and 0x1ab3 (PendingActorHeadData)
    assert data[0x1843:0x1848] == bytes.fromhex('e8480e0600'), 'Mismatch call DetourXS at 0x1843'
    data[0x1843:0x1848] = b'\x90' * 5
    assert data[0x1ab3:0x1ab8] == bytes.fromhex('e8d80b0600'), 'Mismatch call DetourXS at 0x1ab3'
    data[0x1ab3:0x1ab8] = b'\x90' * 5
    print('Neutralized CRT startup DetourXS hooks (0x1843, 0x1ab3).')

    # 2. Precomputed 87-entry dynamic relocation table for Fallout 4 1.11.240.0
    STUB_343176_OFF = 0x600
    STUB_RET_OFF = 0x610
    STUB_GETORCREATESTREAM_OFF = 0x620
    STUB_CONTAINSITEM_OFF = 0x640
    STUB_REMOVEAT_OFF = 0x6B0

    DUMMY_MEM_MANAGER_OFF = 0x700
    DUMMY_VARS_OFF = 0x740
    BST_GLOBAL_EVENT_MOCK_OFF = 0x780
    BST_GLOBAL_EVENT_SINGLETON_PTR_OFF = 0x7F8

    TABLE_ENTRIES = [(40925, 1552, 1),
 (65166, 10955472, 0),
 (67637, 28419616, 0),
 (74587, 23525408, 0),
 (82123, 51284768, 0),
 (84112, 36456110, 0),
 (115170, 38305000, 0),
 (122536, 49931696, 0),
 (129904, 13483936, 0),
 (150211, 28418000, 0),
 (177801, 1552, 1),
 (206895, 1552, 1),
 (278402, 28328784, 0),
 (287808, 38305064, 0),
 (302888, 13058800, 0),
 (303410, 53334896, 0),
 (325206, 52366216, 0),
 (343176, 1536, 1),
 (404814, 28385280, 0),
 (421543, 1856, 2),
 (422985, 51297992, 0),
 (454316, 1856, 2),
 (486180, 49919744, 0),
 (507142, 23584032, 0),
 (526727, 1856, 2),
 (544646, 28328736, 0),
 (544871, 52366208, 0),
 (548587, 51284272, 0),
 (578487, 64738160, 0),
 (583584, 10955664, 0),
 (642758, 51298000, 0),
 (652630, 49939464, 0),
 (652767, 23428576, 0),
 (655847, 28115616, 0),
 (688500, 1600, 1),
 (691815, 51298072, 0),
 (711558, 51278080, 0),
 (728864, 12983296, 0),
 (765242, 1552, 1),
 (769140, 49954952, 0),
 (788691, 28107168, 0),
 (833923, 1856, 2),
 (856221, 28142512, 0),
 (863029, 10955328, 0),
 (876001, 28329216, 0),
 (910917, 51298080, 0),
 (912291, 1552, 1),
 (923307, 23447568, 0),
 (937304, 28416224, 0),
 (939898, 64740736, 0),
 (967277, 2281568, 0),
 (968674, 4816656, 0),
 (988029, 13524512, 0),
 (1065592, 28327616, 0),
 (1071829, 28415232, 0),
 (1085394, 23446080, 0),
 (1134204, 1856, 2),
 (1143155, 1568, 1),
 (1162828, 49923192, 0),
 (1163005, 10711664, 0),
 (1174340, 52356136, 0),
 (1182019, 27825712, 0),
 (1204430, 23581856, 0),
 (1212056, 5322288, 0),
 (1216256, 12966192, 0),
 (1229383, 28409200, 0),
 (1241790, 28420368, 0),
 (1263128, 28420384, 0),
 (1274450, 10955936, 0),
 (1279207, 1856, 2),
 (1306546, 1856, 2),
 (1321764, 10711968, 0),
 (1344421, 49919272, 0),
 (1367353, 10955824, 0),
 (1404410, 23525200, 0),
 (1424022, 2040, 2),
 (1425657, 3004416, 0),
 (1473512, 49936424, 0),
 (1495042, 10711984, 0),
 (1495205, 23425616, 0),
 (1517430, 28084960, 0),
 (1519575, 27797200, 0),
 (1524934, 38304936, 0),
 (1554334, 10955584, 0),
 (1569706, 51278272, 0),
 (1573164, 23434240, 0),
 (1582181, 23429696, 0)]
    table_entries = list(TABLE_ENTRIES)
    assert len(table_entries) == 87

    # 3. Create .dbrng Section Data
    sec_rva = 0x142000
    sec_file_offset = len(data)
    sec_size = 0x3000
    sec_data = bytearray(sec_size)

    # 3a. F4SE wrapper trampoline at sec_rva + 0x000:
    disp_query = 0x544d0 - (sec_rva + 0x13)
    disp_load = 0x54b10 - (sec_rva + 0x1d)
    trampoline = bytearray([
        0x48, 0x83, 0xec, 0x48,             # sub rsp, 0x48
        0x48, 0x89, 0x4c, 0x24, 0x20,       # mov [rsp+0x20], rcx
        0x48, 0x8d, 0x54, 0x24, 0x28,       # lea rdx, [rsp+0x28]
        0xe8, 0x00, 0x00, 0x00, 0x00,       # call F4SEPlugin_Query
        0x48, 0x8b, 0x4c, 0x24, 0x20,       # mov rcx, [rsp+0x20]
        0xe8, 0x00, 0x00, 0x00, 0x00,       # call F4SEPlugin_Load
        0x48, 0x83, 0xc4, 0x48,             # add rsp, 0x48
        0xc3                                # ret
    ])
    struct.pack_into('<i', trampoline, 0x0f, disp_query)
    struct.pack_into('<i', trampoline, 0x19, disp_load)
    sec_data[0:len(trampoline)] = trampoline

    # 3b. Version Data at 0x040
    ver_off = 0x40
    struct.pack_into('<I', sec_data, ver_off + 0, 1) # dataVersion = 1
    struct.pack_into('<I', sec_data, ver_off + 4, 0x02070100) # pluginVersion = 2.7.1.0
    sec_data[ver_off + 8 : ver_off + 8 + len(b'DiverseBodiesRedux\0')] = b'DiverseBodiesRedux\0'
    sec_data[ver_off + 0x108 : ver_off + 0x108 + len(b'Panda\0')] = b'Panda\0'
    struct.pack_into('<I', sec_data, ver_off + 0x208, 4) # AddressLibrary 1.11.137+
    struct.pack_into('<I', sec_data, ver_off + 0x20C, 4) # 1.11.137 Layout
    struct.pack_into('<I', sec_data, ver_off + 0x210, 0x010B0F00) # RUNTIME_VERSION 1.11.240

    # 3c. Export Directory at 0x500
    exp_off = 0x500
    exp_rva = sec_rva + exp_off
    eat_off = 0x530
    enpt_off = 0x540
    eot_off = 0x550
    dll_name_off = 0x560
    load_name_off = 0x580
    query_name_off = 0x5a0
    ver_name_off = 0x5c0

    sec_data[dll_name_off : dll_name_off + len(b'DiverseBodiesRedux.dll\0')] = b'DiverseBodiesRedux.dll\0'
    sec_data[load_name_off : load_name_off + len(b'F4SEPlugin_Load\0')] = b'F4SEPlugin_Load\0'
    sec_data[query_name_off : query_name_off + len(b'F4SEPlugin_Query\0')] = b'F4SEPlugin_Query\0'
    sec_data[ver_name_off : ver_name_off + len(b'F4SEPlugin_Version\0')] = b'F4SEPlugin_Version\0'

    struct.pack_into('<III', sec_data, eat_off, sec_rva, 0x544d0, sec_rva + ver_off)
    struct.pack_into('<III', sec_data, enpt_off, sec_rva + load_name_off, sec_rva + query_name_off, sec_rva + ver_name_off)
    struct.pack_into('<HHH', sec_data, eot_off, 0, 1, 2)

    struct.pack_into('<IIHHIIIIIII', sec_data, exp_off,
        0, 0xffffffff,
        0, 0,
        sec_rva + dll_name_off,
        1,
        3, 3,
        sec_rva + eat_off,
        sec_rva + enpt_off,
        sec_rva + eot_off
    )

    # 3d. Stubs at 0x400
    # stub_343176: lea rax, [rip + disp_to_dummy_mem_manager]; ret
    disp_to_dmm = DUMMY_MEM_MANAGER_OFF - (STUB_343176_OFF + 7)
    code_343176 = bytearray([0x48, 0x8d, 0x05, 0, 0, 0, 0, 0xc3])
    struct.pack_into('<i', code_343176, 3, disp_to_dmm)
    sec_data[STUB_343176_OFF : STUB_343176_OFF + len(code_343176)] = code_343176

    # stub_ret: ret
    sec_data[STUB_RET_OFF] = 0xc3

    # stub_getorcreatestream:
    # test rdx, rdx; jz .done; mov qword ptr [rdx], 0; .done: mov eax, 1; ret
    code_gocs = bytearray([
        0x48, 0x85, 0xd2,             # test rdx, rdx
        0x74, 0x07,                   # jz +7 (.done)
        0x48, 0xc7, 0x02, 0, 0, 0, 0, # mov qword ptr [rdx], 0
        0xb8, 0x01, 0x00, 0x00, 0x00, # mov eax, 1
        0xc3                          # ret
    ])
    sec_data[STUB_GETORCREATESTREAM_OFF : STUB_GETORCREATESTREAM_OFF + len(code_gocs)] = code_gocs

    # stub_containsitem:
    code_containsitem = bytearray([
        0x48, 0x85, 0xc9,             # test rcx, rcx
        0x74, 0x53,                   # jz .not_found (offset +0x58)
        0x48, 0x85, 0xd2,             # test rdx, rdx
        0x74, 0x4e,                   # jz .not_found (offset +0x58)
        # 1. Check arrayOfForms (rcx + 0x20 is TESForm**, rcx + 0x2c is uint32_t count)
        0x4c, 0x8b, 0x41, 0x20,       # mov r8, [rcx + 0x20]
        0x44, 0x8b, 0x49, 0x2c,       # mov r9d, [rcx + 0x2c]
        0x4d, 0x85, 0xc0,             # test r8, r8
        0x74, 0x14,                   # jz .check_temp
        0x45, 0x85, 0xc9,             # test r9d, r9d
        0x74, 0x0f,                   # jz .check_temp
        0x31, 0xc0,                   # xor eax, eax
        # .loop_forms:
        0x49, 0x39, 0x14, 0xc0,       # cmp [r8 + rax*8], rdx
        0x74, 0x37,                   # je .found (offset +0x5b)
        0xff, 0xc0,                   # inc eax
        0x44, 0x39, 0xc8,             # cmp eax, r9d
        0x72, 0xf3,                   # jb .loop_forms
        # .check_temp:
        # 2. Check scriptAddedTempForms (BSTArray<TESFormID>* at rcx + 0x38)
        0x4c, 0x8b, 0x41, 0x38,       # mov r8, [rcx + 0x38]
        0x4d, 0x85, 0xc0,             # test r8, r8
        0x74, 0x24,                   # jz .not_found
        0x4d, 0x8b, 0x10,             # mov r10, [r8]
        0x45, 0x8b, 0x48, 0x0c,       # mov r9d, [r8 + 0x0c]
        0x4d, 0x85, 0xd2,             # test r10, r10
        0x74, 0x18,                   # jz .not_found
        0x45, 0x85, 0xc9,             # test r9d, r9d
        0x74, 0x13,                   # jz .not_found
        0x44, 0x8b, 0x5a, 0x14,       # mov r11d, [rdx + 0x14] (formID)
        0x31, 0xc0,                   # xor eax, eax
        # .loop_temp:
        0x45, 0x39, 0x1c, 0x82,       # cmp [r10 + rax*4], r11d
        0x74, 0x0a,                   # je .found
        0xff, 0xc0,                   # inc eax
        0x44, 0x39, 0xc8,             # cmp eax, r9d
        0x72, 0xf3,                   # jb .loop_temp
        # .not_found: (offset 0x58)
        0x30, 0xc0,                   # xor al, al
        0xc3,                         # ret
        # .found: (offset 0x5b)
        0xb0, 0x01,                   # mov al, 1
        0xc3                          # ret
    ])
    sec_data[STUB_CONTAINSITEM_OFF : STUB_CONTAINSITEM_OFF + len(code_containsitem)] = code_containsitem

    # stub_removeat: sets r8d = 1 (a_count = 1), adds 0x004034A0 to rax (Fallout4.exe base), and jumps to rax
    code_removeat = bytearray([
        0x41, 0xb8, 0x01, 0x00, 0x00, 0x00, # mov r8d, 1
        0x48, 0x05, 0xa0, 0x34, 0x40, 0x00, # add rax, 0x004034A0
        0xff, 0xe0                          # jmp rax
    ])
    sec_data[STUB_REMOVEAT_OFF : STUB_REMOVEAT_OFF + len(code_removeat)] = code_removeat

    # Patch RemoveAt call site at RVA 0x7515b (file offset 0x7455b, 17 bytes)
    assert data[0x7455b:0x7456c] == bytes.fromhex('4c8b80d00000004981c0b0b22a0041ffd0')
    disp_to_removeat = (sec_rva + STUB_REMOVEAT_OFF) - (0x75162 + 5)
    patch_removeat = bytearray([
        0x48, 0x8b, 0x80, 0xd0, 0x00, 0x00, 0x00, # mov rax, [rax + 0xd0] (7 bytes)
        0xe8, 0x00, 0x00, 0x00, 0x00,             # call stub_removeat    (5 bytes)
        0x0f, 0x1f, 0x44, 0x00, 0x00              # 5-byte NOP            (5 bytes)
    ])
    struct.pack_into('<i', patch_removeat, 8, disp_to_removeat)
    data[0x7455b:0x7456c] = patch_removeat

    # 3e. Buffers at 0x700
    # bst_global_event_singleton_ptr points to bst_global_event_mock_obj (absolute runtime address)
    # Since imagebase varies, dbrng_rel_lookup will return runtime address of BST_GLOBAL_EVENT_SINGLETON_PTR_OFF
    # We initialize the pointer at runtime or make dbrng_rel_lookup compute it directly!
    # For type == 2, dbrng_rel_lookup returns (dbrng_runtime_base + offset).
    # Inside bst_global_event_singleton_ptr (0x7F8), we store an 8-byte pointer to BST_GLOBAL_EVENT_MOCK_OFF!
    # When dbrng_rel_lookup is first called for 1424022, it initializes the pointer to &mock_obj!

    # 3f. dbrng_rel_lookup function at 0x800:
    LOOKUP_OFF = 0x800
    TABLE_OFF = 0xA00

    lookup_asm = f'''
.intel_syntax noprefix
.text
.globl dbrng_rel_lookup
dbrng_rel_lookup:
    mov rdx, [rcx]
    push rcx
    push rbx
    push rsi
    push rdi
    push r12
    sub rsp, 0x28
    
    xor esi, esi
    mov edi, {len(table_entries) - 1}
    lea r12, [rip + {TABLE_OFF - (LOOKUP_OFF + 0x1b)}]

.Lsearch:
    cmp esi, edi
    jg .Lfallback
    
    mov eax, esi
    add eax, edi
    shr eax, 1
    
    lea ecx, [rax + rax*2]
    shl rcx, 3
    add rcx, r12
    
    mov r8, [rcx]
    cmp rdx, r8
    je .Lfound
    jl .Lleft
    lea esi, [rax + 1]
    jmp .Lsearch

.Lleft:
    test eax, eax
    jz .Lfallback
    dec eax
    mov edi, eax
    jmp .Lsearch

.Lfound:
    mov r8, [rcx + 8]
    mov eax, [rcx + 16]
    cmp eax, 0
    je .Ltype_rva
    
    # type 1 or 2: offset in .dbrng
    # rip-relative to start of .dbrng
    lea rax, [rip + {0x000 - (LOOKUP_OFF + 0x5b)}]
    add rax, r8
    
    # Special check: if r8 == {BST_GLOBAL_EVENT_SINGLETON_PTR_OFF}:
    # initialize pointer at rax to point to mock_obj ({BST_GLOBAL_EVENT_MOCK_OFF})
    cmp r8, {BST_GLOBAL_EVENT_SINGLETON_PTR_OFF}
    jne .Ldone
    lea rdx, [rip + {BST_GLOBAL_EVENT_MOCK_OFF - (LOOKUP_OFF + 0x6e)}]
    mov [rax], rdx

.Ldone:
    add rsp, 0x28
    pop r12
    pop rdi
    pop rsi
    pop rbx
    pop rcx
    ret

.Ltype_rva:
    mov rax, gs:[0x60]
    mov rax, [rax + 0x10]
    add rax, r8
    add rsp, 0x28
    pop r12
    pop rdi
    pop rsi
    pop rbx
    pop rcx
    ret

.Lfallback:
    lea rax, [rip + {DUMMY_VARS_OFF - (LOOKUP_OFF + 0x9e)}]
    add rsp, 0x28
    pop r12
    pop rdi
    pop rsi
    pop rbx
    pop rcx
    ret
'''

    # Assemble lookup_asm
    with tempfile.NamedTemporaryFile(suffix='.s', mode='w') as f_s:
        f_s.write(lookup_asm)
        f_s.flush()
        o_file = f_s.name.replace('.s', '.o')
        bin_file = f_s.name.replace('.s', '.bin')
        subprocess.check_call(['clang', '-c', f_s.name, '-o', o_file])
        subprocess.check_call(['llvm-objcopy', '-O', 'binary', '--only-section=.text', o_file, bin_file])
        lookup_code = bytearray(open(bin_file, 'rb').read())
        os.unlink(o_file)
        os.unlink(bin_file)

    sec_data[LOOKUP_OFF : LOOKUP_OFF + len(lookup_code)] = lookup_code
    print(f'Installed dbrng_rel_lookup ({len(lookup_code)} bytes).')

    # 3g. Write Table entries at 0xA00
    for idx, (id_val, val, typ) in enumerate(table_entries):
        entry_off = TABLE_OFF + idx * 24
        struct.pack_into('<QQII', sec_data, entry_off, id_val, val, typ, 0)

    print(f'Wrote {len(table_entries)} table entries at offset 0x{TABLE_OFF:X}.')

    # 4. Hook 0x151b0 in DiverseBodiesRedux.dll with 5-byte JMP to LOOKUP_OFF
    disp_to_lookup = (sec_rva + LOOKUP_OFF) - (0x151b0 + 5)
    hook_jmp = bytearray([0xe9, 0, 0, 0, 0])
    struct.pack_into('<i', hook_jmp, 1, disp_to_lookup)
    data[0x400 + 0x151b0 - 0x1000 : 0x400 + 0x151b0 - 0x1000 + 5] = hook_jmp
    print(f'Hooked RVA 0x151b0 -> RVA 0x{sec_rva + LOOKUP_OFF:X} (disp: {hex(disp_to_lookup)}).')

    # 5. Append section data and update PE headers
    data.extend(sec_data)

    e_lfanew = struct.unpack_from('<I', data, 0x3c)[0]
    num_sec = struct.unpack_from('<H', data, e_lfanew + 6)[0]
    assert num_sec == 7
    struct.pack_into('<H', data, e_lfanew + 6, 8)

    # SizeOfImage
    struct.pack_into('<I', data, e_lfanew + 24 + 56, sec_rva + sec_size)
    # Export Table Directory
    struct.pack_into('<II', data, e_lfanew + 24 + 112, exp_rva, 0x100)

    # Section Header
    sec_hdr_off = 0x350
    sec_hdr = bytearray(40)
    sec_hdr[0:7] = b'.dbrng\0'
    struct.pack_into('<IIIIIIHHI', sec_hdr, 8,
        sec_size,           # Misc_VirtualSize
        sec_rva,            # VirtualAddress
        sec_size,           # SizeOfRawData
        sec_file_offset,    # PointerToRawData
        0, 0, 0, 0,
        0xE0000060          # Characteristics (RWX)
    )
    data[sec_hdr_off : sec_hdr_off + 40] = sec_hdr

    with open(output_dll, 'wb') as f:
        f.write(data)

    print(f'Successfully built {output_dll} ({len(data)} bytes).')

if __name__ == '__main__':
    import sys, shutil
    if len(sys.argv) >= 3:
        src = sys.argv[1]
        dst = sys.argv[2]
    elif len(sys.argv) == 2:
        src = sys.argv[1]
        dst = os.path.splitext(src)[0] + '_NG.dll'
    else:
        # Default fallback
        src = 'DiverseBodiesRedux.dll'
        dst = 'DiverseBodiesRedux_NG.dll'
        if not os.path.exists(src):
            src = '/home/lavac/Games/Amethyst/Fallout 4/profiles/Such_Fallout_4/mods/Diverse Bodies Redux/F4SE/Plugins/DiverseBodiesRedux.dll.orig'
            dst = '/home/lavac/.gemini/antigravity/brain/f414056b-e88d-4f2a-8025-fc9a9dedef6e/scratch/DiverseBodiesRedux_NG.dll'

    print(f'Building Next-Gen DiverseBodiesRedux: {src} -> {dst}')
    build_dbr_ng(src, dst)
