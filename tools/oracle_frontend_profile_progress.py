#!/usr/bin/env python3
"""Freeze original193 Highest Level/difficulty selectors and profile defaults.

24A886,24A855/828/763,1A06F0,19C1F0/440/4E0 and2AFD10 execute
unchanged against relocated literal mainmenu tables. Profile-list/pool access,
unrelated base/header update, child lookup and final text-buffer operations are
explicit supplied seams.1A03A0/0750/07B0 defaults and19043F monotonic writer
execute with supplied file/profile storage and unrelated name-copy seams.
Original22E27B establishes base admission, not a complete screen factory or
transition. No native/Xemu/GPU execution is performed.
"""
import hashlib
import json
import struct

from oracle_frontend_lobby_maps import Maps
from oracle_multiplayer_hud_positions import ROOT, PIN, Xbe, STOP
from unicorn.x86_const import *
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

OUT = ROOT / "analysis/frontend/profile-row-appearance"
GET_TEXT, SET_TEXT, LIST_ARRAY = STOP + 0x200, STOP + 0x204, STOP + 0x208
WATCH = (0x24A886, 0x24A855, 0x24A828, 0x24A763, 0x1A06F0,
         0x19C1F0, 0x19C440, 0x19C4E0, 0x2AFD10, 0x22E27B, 0x22E3B4,
         0x1A0750, 0x1A07B0, 0x1A03A0, 0x19043F)


class Progress(Maps):
    def __init__(self):
        # Reuse only the already source-audited literal table relocations.
        # Maps installs this overridden hook; none of its selector seams run.
        super().__init__()
        self.phase = "setup"
        self.calls, self.executed, self.text_sets = [], {}, []
        self.screen = self.alloc(bytes(0x1A00))
        self.profile_list = self.screen + 0x614
        self.vtable = self.alloc(bytes(0x80))
        self.w(self.vtable + 0x58, LIST_ARRAY)
        self.w(self.profile_list, self.vtable)
        self.pool = self.alloc(bytes(0x40)); self.w(self.profile_list + 0x70, self.pool)
        self.datum = self.alloc(bytes(8)); self.records = self.alloc(bytes(0x248 * 2))
        for i in range(2): self.w(self.records + i * 0x248, 7 + i)
        self.text_vtable = self.alloc(bytes(0x60)); self.w(self.text_vtable + 0x3C, GET_TEXT)
        self.buffer_vtable = self.alloc(bytes(0x20)); self.w(self.buffer_vtable + 4, SET_TEXT)
        self.buffers = [self.alloc(bytes(0x100)) for _ in range(2)]
        self.texts = [self.alloc(bytes(0x100)) for _ in range(2)]
        for widget, buffer in zip(self.texts, self.buffers):
            self.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, widget), (UC_X86_REG_EAX, 6)])
            self.w(widget, self.text_vtable); self.w(buffer, self.buffer_vtable)
        self.base_visibility = [self.read(p + 0x6E, "<B")[0] for p in self.texts]
        assert self.base_visibility == [1, 1]
        self.base_drawing_admission = [self.run(0x22E3B4, registers=[(UC_X86_REG_ECX, p)])
                                       for p in self.texts]
        assert self.base_drawing_admission == [1, 1]
        self.authored = json.loads((OUT / "screen193-details-authored.json").read_text())
        self.localized = self.authored["localized"]
        self.definitions = []
        for row in self.authored["panes"][0]["texts"][4:6]:
            self.definitions.append(self.alloc(bytes.fromhex(row["raw"])))
        self.w(0x47FF38, 0)  # Original cached current language: English.

    def wide(self, pointer):
        raw = bytearray()
        while (word := bytes(self.g.mem_read(pointer + len(raw), 2))) != b"\0\0":
            raw.extend(word); assert len(raw) < 1024
        return raw.decode("utf-16le")

    def services(self, _uc, address, _size, _context):
        reg = self.g.reg_read
        sp = reg(UC_X86_REG_ESP)
        if address in WATCH:
            self.executed[address] = self.executed.get(address, 0) + 1
        if address == 0x22E391: self.returned()
        elif address == 0x24C447:
            assert reg(UC_X86_REG_EAX) == self.profile_list
            self.returned(0x10000)
        elif address == 0x16BAE0:
            assert reg(UC_X86_REG_ESI) == self.pool
            self.returned(self.datum)
        elif address == LIST_ARRAY:
            assert reg(UC_X86_REG_ECX) == self.profile_list
            self.w(self.r(sp + 4), 2); self.returned(self.records, 4)
        elif address == 0x22F042: self.returned(0, 8)
        elif address == 0x22EE47:
            self.calls.append(dict(kind="visibility", arguments=self.read(sp + 4, "<3I")))
            self.returned(0, 12)
        elif address == 0x22EDB8:
            kind, index, recursive = self.read(sp + 4, "<3I")
            assert reg(UC_X86_REG_ECX) == self.screen and recursive == 0
            self.returned(self.texts[index - 6] if kind == 6 and index in (6, 7) else 0, 12)
        elif address == 0x253C06:
            assert reg(UC_X86_REG_ESI) == self.texts[0]
            self.calls.append(dict(kind="authored_definition", raw_index=4))
            self.returned(self.definitions[0])
        elif address == 0x253B1A:
            index = self.texts.index(reg(UC_X86_REG_EDI))
            string_id = self.r(sp + 4)
            self.text_sets.append(dict(field=index, method="string_id", string=f"{string_id:08x}",
                                       text=self.localized[f"{string_id:08x}"]))
            self.returned(0, 4)
        elif address == GET_TEXT:
            self.returned(self.buffers[self.texts.index(reg(UC_X86_REG_ECX))])
        elif address == SET_TEXT:
            index = self.buffers.index(reg(UC_X86_REG_ECX))
            self.text_sets.append(dict(field=index, method="localized_name", text=self.wide(self.r(sp + 4))))
            self.returned(0, 4)
        elif address == 0x215C00:
            assert self.phase == "create"
            self.returned(7, 8)
        elif address == 0x216240:
            assert self.phase == "create"
            assert reg(UC_X86_REG_ECX) == 0x1E0
            self.created_profile = bytes(self.g.mem_read(reg(UC_X86_REG_EDX), 0x1E0))
            self.returned(1, 4)
        elif address == 0x3207ED:
            # Guest/name CRT copy is unrelated to the four progress words.
            destination, source, count = self.read(sp + 4, "<3I")
            words = self.wide(source).encode("utf-16le")[:count * 2]
            self.g.mem_write(destination, words + bytes(count * 2 - len(words)))
            self.returned(destination)
        elif address == 0x18FC44:
            assert self.phase == "writer"
            assert reg(UC_X86_REG_ECX) == 2
            self.g.mem_write(reg(UC_X86_REG_EAX), self.writer_profile)
            self.w(reg(UC_X86_REG_EBX), self.writer_file)
            self.returned()
        elif address == 0x18FCC4:
            assert self.phase == "writer"
            controller, profile, file_index = self.read(sp + 4, "<3I")
            self.calls.append(dict(kind="commit", controller=controller, file_index=file_index,
                                   keys=self.read(profile + 0xEC, "<4i")))
            self.returned(0, 12)

    def getter(self, profile):
        best, difficulty = self.alloc(bytes(4)), self.alloc(bytes(4))
        self.run(0x1A06F0, argv=[best, difficulty], registers=[(UC_X86_REG_ECX, profile)])
        return self.read(best, "<i")[0], self.r(difficulty)


def main():
    m = Progress(); c = m.cache
    header = c.block(next(t["address"] - c.magic for t in c.tags.values() if t["group"] == "matg") + 0x178, 24)[0]
    campaign = []
    for p in c.block(header + 8, 0xB50):
        key = struct.unpack_from("<i", c.data, p + 4)[0]
        if key == -1: continue
        campaign.append(dict(campaign_id=c.u32(p), map_id=key,
                             names=[m.file_utf16(p + 0x10 + 0x40 * language, 32) for language in range(8)]))
    matg = next(t["address"] - c.magic for t in c.tags.values() if t["group"] == "matg")
    paths_header = c.block(matg + 0x170, 8)[0]
    paths = [dict(campaign_id=struct.unpack_from("<i", c.data, p)[0],
                  map_id=struct.unpack_from("<i", c.data, p + 4)[0], scenario_path=c.string(p + 8, 256))
             for p in c.block(paths_header, 0x108)]
    assert sorted((p["campaign_id"], p["map_id"]) for p in paths if p["map_id"] != -1) == sorted(
        (p["campaign_id"], p["map_id"]) for p in campaign)
    threshold = m.run(0x19C4E0, argv=[1]); assert threshold == 805
    profile = m.alloc(bytes(0x1E0)); defaults = []
    for function, registers, argv in ((0x1A0750, [(UC_X86_REG_EDX, profile)], []),
                                      (0x1A07B0, [(UC_X86_REG_ESI, profile)], [0])):
        m.phase = "default"; m.run(function, argv=argv, registers=registers)
        defaults.append(dict(function=f"{function:08x}", keys=m.read(profile + 0xEC, "<4i"),
                             selected=list(m.getter(profile))))
    m.phase = "create"; name = m.alloc("Progress Oracle\0".encode("utf-16le"))
    m.w(0x4E7408, m.alloc(struct.pack("<2I", 0, 0x12345678)))
    created = m.run(0x1A03A0, argv=[0, name])
    assert created == 7 and len(m.created_profile) == 0x1E0
    created_profile = m.alloc(m.created_profile)
    defaults.append(dict(function="001a03a0", keys=m.read(created_profile + 0xEC, "<4i"),
                         selected=list(m.getter(created_profile)), file_index=created))
    assert defaults[0]["keys"] == defaults[1]["keys"] == [0] * 4
    assert defaults[2]["keys"] == [-1] * 4
    assert all(row["selected"] == [-1, 1] for row in defaults)
    cases = []
    banks = [[0]*4, [-1]*4, [999]*4, [-2]*4, [101, 101, 101, 101], [805, 999, 301, 805],
             [1, 801, 705, 805], [101, 305, 401, 501]]
    for key in m.valid_keys:
        for slot in range(4):
            keys = [-1]*4; keys[slot] = key; banks.append(keys)
    for ordinal, keys in enumerate(banks):
        focus = 7 + ordinal % 2
        for i in range(2):
            m.g.mem_write(m.records + i * 0x248 + 0xF0,
                          struct.pack("<4i", *(keys if i == focus - 7 else [805, 805, 805, 805])))
        best, difficulty = m.getter(m.records + (focus - 7)*0x248 + 4)
        next_key = best if best == threshold else m.run(0x19C440, argv=[1, best])
        next_key = struct.unpack("<i", struct.pack("<I", next_key))[0]
        expected_name = next(x for x in campaign if x["map_id"] == next_key)["names"][0]
        m.w(m.datum + 4, focus); m.phase = "details"
        m.calls.clear(); m.executed.clear(); m.text_sets.clear()
        for widget in m.texts: m.g.mem_write(widget + 0x6E, b"\1")
        m.run(0x24A886, registers=[(UC_X86_REG_ECX, m.screen)])
        assert [x["text"] for x in m.text_sets] == [expected_name,
                ["on Easy", "on Normal", "on Heroic", "on Legendary"][difficulty]], (keys, best, difficulty, next_key, expected_name, m.text_sets)
        assert all(m.read(p + 0x6E, "<B")[0] == 1 for p in m.texts)
        assert m.executed.get(0x1A06F0) == 2 and m.executed.get(0x2AFD10) == 1
        cases.append(dict(keys=keys, focused_record=focus, best_key=best, difficulty_slot=difficulty,
                          displayed_map_id=next_key, text_sets=m.text_sets.copy(), visibility=[1, 1],
                          executed={f"{a:08x}": m.executed.get(a, 0) for a in WATCH}))
    create_new = []
    for initial_visibility in (0, 1):
        m.w(m.datum + 4, -1); m.text_sets.clear(); m.calls.clear(); m.executed.clear()
        for p in m.texts: m.g.mem_write(p + 0x6E, bytes([initial_visibility]))
        m.run(0x24A886, registers=[(UC_X86_REG_ECX, m.screen)])
        assert m.text_sets == [dict(field=0, method="string_id", string="0800010b", text="The Heretic"),
                               dict(field=1, method="string_id", string="060000b8", text="on Normal")]
        assert m.executed.get(0x1A06F0, 0) == 0
        assert [m.read(p + 0x6E, "<B")[0] for p in m.texts] == [initial_visibility]*2
        create_new.append(dict(initial_visibility=initial_visibility, final_visibility=[initial_visibility]*2,
                               text_sets=m.text_sets.copy(), calls=m.calls.copy()))
    missing_tables = []
    globals_pointer = m.r(0x4E034C)
    m.w(m.datum + 4, 7)
    m.g.mem_write(m.records + 0xF0, struct.pack("<4i", 805, -1, -1, -1))
    for field in (0x170, 0x178):
        saved = m.r(globals_pointer + field); m.w(globals_pointer + field, 0)
        m.text_sets.clear(); m.calls.clear(); m.executed.clear()
        m.run(0x24A886, registers=[(UC_X86_REG_ECX, m.screen)])
        assert m.text_sets[0] == dict(field=0, method="string_id", string="0800010b", text="The Heretic")
        assert [m.read(p + 0x6E, "<B")[0] for p in m.texts] == [1, 1]
        missing_tables.append(dict(absent_table_field=f"0x{field:x}", keys=[805, -1, -1, -1],
                                   text_sets=m.text_sets.copy(), calls=m.calls.copy()))
        m.w(globals_pointer + field, saved)
    m.g.mem_write(m.records + 0xF0, struct.pack("<4i", 301, -1, -1, -1))
    for p in m.texts: m.g.mem_write(p + 0x6E, b"\0")
    m.run(0x24A886, registers=[(UC_X86_REG_ECX, m.screen)])
    assert [m.read(p + 0x6E, "<B")[0] for p in m.texts] == [0, 0]
    saved_visibility_preservation = dict(initial_visibility=[0, 0], final_visibility=[0, 0])
    writers = []
    m.phase = "writer"
    for bank, key, file_index in (([0]*4, 101, 7), ([301]*4, 101, 7), ([301]*4, 301, 7),
                                 ([301]*4, 501, 7), ([0]*4, -1, 7), ([0]*4, 501, -1)):
        for slot in range(4):
            raw = bytearray(0x1E0); struct.pack_into("<4i", raw, 0xEC, *bank)
            m.writer_profile, m.writer_file = bytes(raw), file_index
            m.calls.clear(); m.executed.clear()
            m.run(0x19043F, argv=[2, slot], registers=[(UC_X86_REG_ESI, key)])
            expected = key != -1 and file_index != -1 and bank[slot] < key
            assert bool(m.calls) == expected
            if expected:
                expected_keys = bank.copy(); expected_keys[slot] = key
                assert m.calls == [dict(kind="commit", controller=2, file_index=7, keys=expected_keys)]
            writers.append(dict(keys=bank, slot=slot, supplied_key=key, file_index=file_index, calls=m.calls.copy()))
    result = dict(scope=__doc__, original_xbe_sha256=PIN,
                  mainmenu_sha256=hashlib.sha256(c.data).hexdigest(), campaign=campaign,
                  scenario_paths=paths, scenario_path_keys_equal_campaign_definition_keys=True,
                  progress_offset_settings="0xec", progress_offset_focused_record="0xf0",
                  threshold=threshold, authored_details=m.authored["panes"][0]["texts"][4:6],
                  defaults=defaults, cases=cases, create_new=create_new, writers=writers,
                  missing_tables=missing_tables, saved_visibility_preservation=saved_visibility_preservation,
                  base_visibility=m.base_visibility, base_drawing_admission=m.base_drawing_admission,
                  visibility_boundary="Actual22E27B initializes admission true.24A886 leaves both progress children admission unchanged; complete screen factory/transition and GPU are excluded.",
                  no_native_storage_or_progress_value_inferred=True)
    path = OUT / "profile-progress-original.json"; path.write_text(json.dumps(result, indent=2) + "\n")
    rows = ["# original193 progress keys0..3 best difficulty displayed_map name difficulty_text"]
    for row in cases:
        rows.append("\t".join(map(str, ["case", *row["keys"], row["best_key"], row["difficulty_slot"],
                                        row["displayed_map_id"], *[x["text"] for x in row["text_sets"]]])))
    (OUT / "profile-progress-original.tsv").write_text("\n".join(rows) + "\n")
    xbe = Xbe(ROOT / "assets/disc/default.xbe"); md = Cs(CS_ARCH_X86, CS_MODE_32)
    regions = ((0x24A9AD, 0xD7), (0x1A06F0, 0x60), (0x19C440, 0xA0),
               (0x2AFD10, 0x30), (0x1A03A0, 0x1A0), (0x19043F, 77), (0x1388E0, 114))
    (OUT / "profile-progress-original.asm").write_text("\n\n".join("\n".join(
        f"{i.address:08x} {i.mnemonic:8s} {i.op_str}" for i in md.disasm(xbe.read(at, size), at))
        for at, size in regions) + "\n")
    print(f"{len(cases)} original193 progress cases, {len(defaults)} defaults, {len(writers)} writer cases passed")
    print("JSON SHA256", hashlib.sha256(path.read_bytes()).hexdigest())


if __name__ == "__main__": main()
