#!/usr/bin/env python3
"""Execute the original screen14 roster producer, skin consumer and visibility.

The pinned Xbox instructions at 25042D -> 22F042 -> 2AFEAE run unchanged.
Only session/voice providers, the UI-globals accessor and text-buffer sinks are
seams. Literal retail skin definitions and palette/rank tables are relocated.
The synthetic linked widgets preserve the proven type6 header/subtitle prefix;
allocation/animation/rendering/network transport are outside this fixture.
"""
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache, file_sha
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

OUT = ROOT / 'analysis/frontend/lobby'


class Roster(Retail):
    def __init__(self):
        super().__init__()
        self.cache = c = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
        self.text_sets = []
        self.items = []
        self.players_by_index = {}
        self.voice = {}
        self.matchmaking = False
        self.online = False
        self.buffer_owner = {}
        self.set_vtable = self.alloc(bytes(16))
        self.w(self.set_vtable + 4, 0x3000100)
        self.node_vtable = self.alloc(bytes(0x50))
        self.w(self.node_vtable + 0x3c, 0x3000200)
        self.tail = {}
        self.node_records = []
        _, p = c.tag(next(t['datum'] for t in c.tags.values() if t['group'] == 'matg'), 'matg')
        records = c.block(p + 0x160, 12)
        palette = self.alloc(c.data[records[0]:records[0] + 12 * len(records)])
        matg = self.alloc(c.data[p:p + 0x190])
        self.g.mem_write(matg + 0x160, struct.pack('<II', len(records), palette))
        multiplayer_datum = c.u32(p + 0x16c)
        _, multiplayer = c.tag(multiplayer_datum, 'mulg')
        multiplayer_header = self.alloc(c.data[multiplayer:multiplayer + 12])
        universal = c.u32(multiplayer + 4) - c.magic
        universal_header = self.alloc(c.data[universal:universal + 0x20])
        colors = c.block(universal + 0x10, 12)
        team_colors = self.alloc(c.data[colors[0]:colors[0] + 12 * len(colors)])
        self.team_colors = bytes(c.data[colors[0]:colors[0] + 12 * len(colors)])
        self.palette = bytes(c.data[records[0]:records[0] + 12 * len(records)])
        self.w(universal_header + 0x14, team_colors)
        self.w(multiplayer_header + 4, universal_header)
        self.w(self.table + (multiplayer_datum & 0xffff) * 16 + 8, multiplayer_header)
        self.w(0x4e034c, matg)
        self.w(0x4e0350, 1)
        _, p = c.tag(next(t['datum'] for t in c.tags.values() if t['group'] == 'wigl'), 'wigl')
        self.wigl = self.alloc(c.data[p:p + 452])
        records = c.block(p + 0x158, 4)
        self.rank_ranges = bytes(c.data[records[0]:records[0] + 4 * len(records)])
        ranges = self.alloc(c.data[records[0]:records[0] + 4 * len(records)])
        self.w(self.wigl + 0x15c, ranges)
        self.skin_datum = 0xe50d02ed
        _, p = c.tag(self.skin_datum, 'skin')
        self.skin = self.alloc(c.data[p:p + 64])
        self.w(self.table + (self.skin_datum & 0xffff) * 16 + 8, self.skin)
        self.skin_records = {}
        for offset, stride in [(28, 44), (36, 56), (44, 36)]:
            records = c.block(p + offset, stride)
            data = self.alloc(c.data[records[0]:records[0] + stride * len(records)])
            self.w(self.skin + offset + 4, data)
            self.skin_records[offset] = [data + i * stride for i in range(len(records))]
        self.bitmap_definitions = {}
        def relocate_bitmap(definition):
            datum = self.r(definition + 28)
            if datum in self.bitmap_definitions:
                return
            _, at = c.tag(datum, 'bitm')
            records = c.block(at + 68, 116)
            header = self.alloc(bytes(0x4c))
            self.w(header + 0x44, len(records))
            self.w(self.table + (datum & 0xffff) * 16 + 8, header)
            self.bitmap_definitions[datum] = header
        for definition in self.skin_records[36]:
            relocate_bitmap(definition)
        self.panes = []
        screen = next(t for t in c.tags.values() if t['group'] == 'wgit' and
                      c.u16(t['address'] - c.magic + 4) == 14)
        _, p = c.tag(screen['datum'], 'wgit')
        for pane in c.block(p + 32, 76):
            texts = [self.alloc(c.data[a:a + 44]) for a in c.block(pane + 28, 44)]
            bitmaps = [self.alloc(c.data[a:a + 56]) for a in c.block(pane + 36, 56)]
            grid = self.alloc(c.data[c.block(pane + 68, 24)[0]:c.block(pane + 68, 24)[0] + 24])
            for definition in bitmaps:
                relocate_bitmap(definition)
            self.panes.append((texts, bitmaps, grid))
        self.g.hook_add(UC_HOOK_CODE, self.roster_hook)
        self.case_heap_start = self.next

    def return_value(self, value=0, pop=0):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        self.ret(value)
        self.g.reg_write(UC_X86_REG_ESP, sp + 4 + pop)

    def roster_hook(self, guest, address, size, _):
        sp = guest.reg_read(UC_X86_REG_ESP)
        if address == 0x199994:
            self.return_value(int(self.matchmaking))
        elif address == 0x199971:
            self.return_value(int(self.online))
        elif address == 0x19aaa5:
            self.return_value(self.players_by_index[guest.reg_read(UC_X86_REG_EAX)])
        elif address == 0x53b40:
            self.return_value(int(self.voice[guest.reg_read(UC_X86_REG_EAX)] != 2))
        elif address == 0x53750:
            self.return_value(int(self.voice[guest.reg_read(UC_X86_REG_ESI)] == 0))
        elif address == 0x148350:
            self.return_value(self.wigl)
        elif address == 0x3000200:
            self.return_value(self.r(guest.reg_read(UC_X86_REG_ECX) + 0x90))
        elif address == 0x3000100:
            pointer = self.r(sp + 4)
            words = bytearray()
            while guest.mem_read(pointer + len(words), 2) != b'\0\0':
                words.extend(guest.mem_read(pointer + len(words), 2))
                assert len(words) < 1024
            self.text_sets.append(dict(node=self.buffer_owner[guest.reg_read(UC_X86_REG_ECX)],
                                       text=words.decode('utf-16le')))
            self.return_value(pop=4)
        elif address == 0x253b1a:
            # The selected local team label is a supplied localization sink.
            self.text_sets.append(dict(node=guest.reg_read(UC_X86_REG_EDI),
                                       string_id=f'{self.r(sp + 4):08x}'))
            self.return_value(pop=4)
        elif address == 0x22f042:
            count = self.r(sp + 8)
            first = guest.reg_read(UC_X86_REG_EAX)
            self.items = [bytes(guest.mem_read(first + i * 0x74, 0x74)).hex()
                          for i in range(count)]

    def node(self, parent, kind, definition=0, index=0):
        p = self.alloc(bytes(0x100))
        self.g.mem_write(p, struct.pack('<IIH', self.node_vtable, kind, 0))
        self.g.mem_write(p + 0xa, struct.pack('<h', index))
        self.w(p + 0x70, definition)
        self.w(p + 0x10, parent)
        self.g.mem_write(p + 0x6d, b'\1\1')
        self.g.mem_write(p + 0x28, struct.pack('<3f', 1, 1, 1))
        if parent in self.tail:
            self.w(self.tail[parent] + 0x18, p)
        elif parent:
            self.w(parent + 0x14, p)
        self.tail[parent] = p
        if kind == 6:
            buffer = self.alloc(bytes(16))
            self.w(buffer, self.set_vtable)
            self.w(p + 0x90, buffer)
            self.buffer_owner[buffer] = p
        self.node_records.append(dict(address=p, parent=parent, kind=kind, index=index))
        return p

    def case(self, mode, count=1, valid=0, voice=2, teams=False, local_team=None,
             rank=-1, member_count=-1, player_team=-1, appearance=None, first_name=None):
        self.next = self.case_heap_start
        self.buffer_owner = {}
        self.node_records = []
        self.tail = {}
        self.text_sets = []
        self.items = []
        self.players_by_index = {}
        self.voice = {}
        screen = self.alloc(bytes(0x1474))
        self.g.mem_write(screen + 0x142c, struct.pack('<h', mode))
        texts, bitmaps, grid = self.panes[mode]
        screen_texts = [self.node(screen, 6, index=i) for i in range(2)]
        screen_texts.extend(self.node(screen, 6, definition, i) for i, definition in enumerate(texts))
        screen_bitmaps = [self.node(screen, 8, definition, i) for i, definition in enumerate(bitmaps)]
        groups = []
        group_children = []
        for slot in range(16):
            group = self.node(screen, 10, grid, slot)
            # Group +70 is the index, +74 the grid definition (actual factory ABI).
            self.w(group + 0x70, slot)
            self.w(group + 0x74, grid)
            groups.append(group)
            children = []
            for field, kind in [(28, 6), (36, 8), (44, 9)]:
                children.extend(self.node(group, kind, definition, i)
                                for i, definition in enumerate(self.skin_records[field]))
            group_children.append(children)
        indices = list(range(count))
        players = self.alloc(struct.pack('<16I', *(indices + [0xffffffff] * (16 - count))))
        local = self.alloc(struct.pack('<4I', 0, 0xffffffff, 0xffffffff, 0xffffffff))
        session = self.alloc(bytes(0xc0))
        self.w(session + 0x48, int(teams))
        if local_team is not None:
            self.g.mem_write(screen + 0x814, struct.pack('<BBh', 1, 0, local_team))
        for i in indices:
            player = self.alloc(bytes(0x84))
            name = first_name if i == 0 and first_name is not None else f'Source Player {i + 1}'
            raw_name = (name + '\0').encode('utf-16le')
            assert len(raw_name) <= 0x40
            self.g.mem_write(player, raw_name)
            appearance_bytes = appearance if appearance is not None else bytes([8, 12, 9, 1, 0, 26, 9, 0]) + bytes(8)
            assert len(appearance_bytes) == 16
            self.g.mem_write(player + 0x40, appearance_bytes)
            self.g.mem_write(player + 0x7c, bytes([player_team & 255, 0, rank & 255, 0, 0, member_count & 255]))
            self.players_by_index[i] = player
            self.voice[i] = voice
        self.run(0x25042d, argv=[screen, valid & 0xffffffff, session, players, local, count])
        def state(p):
            return dict(visible=bool(self.g.mem_read(p + 0x6e, 1)[0]),
                        frame=self.read(p + 0x88, '<h')[0],
                        rgb_hex=bytes(self.g.mem_read(p + 0x28, 12)).hex())
        text_a, text_b = ((24, 8) if mode == 1 else (29, 13))
        slots = []
        for i, group in enumerate(groups):
            children = group_children[i]
            name_sets = [v for v in self.text_sets if v['node'] == children[0]]
            slots.append(dict(slot=i, group=state(group), name=name_sets,
                              name_color=state(children[0]),
                              skin_bitmaps=[state(p) for p in children[1:4]],
                              emblem=dict(state(children[4]),
                                          appearance_hex=bytes(self.g.mem_read(children[4] + 0x74, 16)).hex(),
                                          team=self.read(children[4] + 0x84, '<h')[0]),
                              text_a=state(screen_texts[text_a + i]),
                              text_b=state(screen_texts[text_b + i]),
                              bitmap_a=state(screen_bitmaps[5 + i]),
                              speaker=state(screen_bitmaps[21 + i])))
        shown = count if valid != -1 else 0
        assert [s['group']['visible'] for s in slots] == [i < shown for i in range(16)]
        assert [s['speaker']['visible'] for s in slots] == [i < shown for i in range(16)]
        assert not any(s['text_a']['visible'] or s['text_b']['visible'] or
                       s['bitmap_a']['visible'] for s in slots)
        for i in range(shown):
            assert slots[i]['speaker']['frame'] == voice
            assert slots[i]['name']
            assert slots[i]['emblem']['appearance_hex'] == appearance_bytes.hex()
        return dict(mode=mode, count=count, valid=valid, voice_input_frame=voice,
                    teams=teams, local_team=local_team, rank_input=rank,
                    member_count=member_count, player_team=player_team,
                    items_hex=self.items.copy(), slots=slots)


def main():
    r = Roster()
    cases = []
    for mode in range(3):
        for count in (0, 1, 2, 4, 16):
            for voice in (0, 1, 2):
                cases.append(r.case(mode, count, voice=voice))
        cases.append(r.case(mode, 1, valid=-1))
        cases.append(r.case(mode, 1, teams=True))
        for team in range(-1, 8):
            cases.append(r.case(mode, 1, local_team=team))
        for rank in (-1, 0, 1, 10, 25, 50, 127):
            cases.append(r.case(mode, 1, rank=rank))
        for members in (-1, 0, 1, 2, 3, 4, 127):
            cases.append(r.case(mode, 1, member_count=members))
        for team in range(8):
            cases.append(r.case(mode, 1, teams=True, player_team=team))
    output = dict(xbe_sha256=PIN, map_sha256=file_sha(r.cache.path),
                  executed=['25042d', '22f042', '2afeae', '22edb8', '22ee47',
                            '2b01a2', '149ead', '7f790', '1318d0', '131a00', '2b12ca', '2b0a14'],
                  seams=['session/voice provider returns', 'UI-global pointer relocation',
                         'text-buffer sink/localization sink', 'synthetic widget allocation'],
                  boundary='Literal skin and linked widget fixture; no transport, full lobby clock, rendering or live single-user voice-state claim.',
                  cases=cases)
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / 'roster-original.json'
    path.write_text(json.dumps(output, indent=2) + '\n')
    def words(raw):
        return ' '.join(f'{word:08x}' for word in struct.unpack('<' + 'I' * (len(raw) // 4), raw))
    lines = ['# Unchanged25042D/22F042/2AFEAE fixture; explicit network and voice inputs.']
    for label, data in [('palette', r.palette), ('team_palette', r.team_colors)]:
        for i in range(len(data) // 12):
            lines.append(f'{label} {i} {words(data[i * 12:i * 12 + 12])}')
    for i, (low, high) in enumerate(struct.iter_unpack('<hh', r.rank_ranges)):
        lines.append(f'range {i} {low} {high}')
    for ci, case in enumerate(cases):
        local = -999 if case['local_team'] is None else case['local_team']
        lines.append(f"case {ci} {case['mode']} {case['count']} {case['valid']} {case['voice_input_frame']} {int(case['teams'])} {local} {case['rank_input']} {case['member_count']} {case['player_team']}")
        for slot in case['slots']:
            if not slot['group']['visible']:
                lines.append(f"hidden {slot['slot']} {int(slot['speaker']['visible'])}")
                continue
            item = bytes.fromhex(case['items_hex'][slot['slot']])
            name = slot['name'][0]
            sid = name.get('string_id', 'ffffffff')
            bitmaps = slot['skin_bitmaps']
            flags = struct.unpack_from('<I', item)[0]
            no_team = item[0x5f] if flags & 0x80 else 0
            lines.append(f"row {slot['slot']} {flags:08x} {slot['emblem']['team']} {no_team} {struct.unpack_from('<h',item,0x60)[0]} {sid} {slot['speaker']['frame']} {int(bitmaps[0]['visible'])} {bitmaps[0]['frame']} {int(bitmaps[1]['visible'])} {int(bitmaps[2]['visible'])} {bitmaps[2]['frame']} {int(slot['emblem']['visible'])} {words(bytes.fromhex(slot['name_color']['rgb_hex']))} {words(bytes.fromhex(bitmaps[0]['rgb_hex']))}")
    (OUT / 'roster-original.tsv').write_text('\n'.join(lines) + '\n')
    live_path = OUT / 'reference/roster-live.json'
    if live_path.exists():
        live = json.loads(live_path.read_text())
        assert live['complete'] and live['count'] == 1
        player = live['players'][0]
        voice = {query['query']: query['returned'] for query in live['voice']}
        frame = int(not voice.get('53750', 0)) if voice['53b40'] else 2
        original = r.case(live['mode'], 1, live['valid'], frame,
                          teams=bool(live['session_flags'] & 1),
                          rank=player['rank_input'], member_count=player['member_count'],
                          player_team=player['team'], first_name=player['name'],
                          appearance=bytes.fromhex(player['appearance_hex']))['slots'][0]
        widgets = live['widgets']
        group = next(w for w in widgets if w['kind'] == 10 and w['runtime_type_ordinal'] == 0)
        children = {(w['kind'], w['authored_index']): w for w in group['children']}
        name, primary, secondary, rank, emblem = [children[key] for key in
                [(6, 0), (8, 0), (8, 1), (8, 2), (9, 0)]]
        speaker = next(w for w in widgets if w['kind'] == 8 and w['runtime_type_ordinal'] == 21)
        checks = dict(group_visible=group['visible'] == original['group']['visible'],
                      name_rgb=name['rgb_hex'] == original['name_color']['rgb_hex'],
                      primary_rgb=primary['rgb_hex'] == original['skin_bitmaps'][0]['rgb_hex'],
                      skin_visibility=[primary['visible'], secondary['visible'], rank['visible']] ==
                          [b['visible'] for b in original['skin_bitmaps']],
                      emblem_visible=emblem['visible'] == original['emblem']['visible'],
                      emblem_appearance=emblem['appearance_hex'] == original['emblem']['appearance_hex'],
                      emblem_team=emblem['team'] == original['emblem']['team'],
                      speaker_frame=speaker['frame'] == original['speaker']['frame'],
                      speaker_visible=speaker['visible'] == original['speaker']['visible'],
                      unused_groups=all(not w['visible'] for w in widgets if w['kind'] == 10 and w['runtime_type_ordinal'] > 0),
                      unused_speakers=all(not w['visible'] for w in widgets if w['kind'] == 8 and 22 <= w['runtime_type_ordinal'] <= 36))
        assert all(checks.values()), checks
        (OUT / 'roster-live-comparison.json').write_text(json.dumps(dict(
            live_fixture_sha256=hashlib.sha256(live_path.read_bytes()).hexdigest(),
            same_ui_clock=live['ui_clock'] == live['ui_clock_post'], checks=checks,
            source_name=player['name'], source_appearance=player['appearance_hex'],
            source_row=original,
            boundary='Compare captured post25042D semantic fields with the unchanged source instructions; widget allocation addresses/hidden uninitialized fields excluded.'), indent=2) + '\n')
    print(f'{len(cases)} unchanged retail roster cases passed; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
