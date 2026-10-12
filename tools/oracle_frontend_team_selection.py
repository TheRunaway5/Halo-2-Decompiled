#!/usr/bin/env python3
"""Freeze the retail screen14 local team edit handler, using literal map data.

250B1E,250CDA,250D5C,251364,199967/19989D/199951,251139/2510E1
and19C5F0 run unchanged. Declared seams supply session/map/controller records,
idle-channel/countdown queries, base widget delegation and profile-update delivery.
No team choice, wrapping, availability or map count result is replaced.
"""
import hashlib
import json
import struct
from collections import Counter
from oracle_frontend_lobby_maps import Maps
from oracle_multiplayer_hud_positions import ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

OUT = ROOT / 'analysis/multiplayer/game-variants'
FIXTURE = ROOT / 'tests/fixtures/frontend_team_original.tsv'


class Teams(Maps):
    def __init__(self):
        super().__init__()
        self.team_screen = self.alloc(bytes(0x900))
        self.session = self.alloc(bytes(0x130))
        self.player = self.alloc(bytes(0x140))
        self.event = self.alloc(bytes(12))
        self.seams = Counter()
        self.visited = set()
        self.g.hook_add(UC_HOOK_CODE, self.team_services)
        self.configure()

    def configure(self, map_id=50, source_type=2, flags=1, protocol=0,
                  campaign=-1, session_present=True, map_present=True,
                  player_present=True, member=0, unready=True, busy=False,
                  signed_in=True, base_result=False):
        self.inputs = dict(map_id=map_id, source_type=source_type, flags=flags,
            protocol=protocol, campaign=campaign, session_present=session_present,
            map_present=map_present, player_present=player_present, member=member,
            unready=unready, busy=busy, signed_in=signed_in, base_result=base_result)
        self.w(self.session + 0x44, source_type)
        self.w(self.session + 0x48, flags)
        self.g.mem_write(self.player + 0x7c, struct.pack('<b', member))
        for slot in range(4):
            self.w(0x54e8e0 + slot * 0xc70, 16 if signed_in else 0)
            self.w(0x54eadc + slot * 0xc70, 100 + slot)
            self.g.mem_write(self.team_screen + 0x814 + slot * 4,
                            struct.pack('<BBh', 0, 0xa0 + slot, 20 + slot))
        self.notifications = []
        self.delegations = []

    def team_services(self, guest, address, size, _):
        self.visited.add(address)
        i = self.inputs
        if address == 0x649c0:
            self.returned(self.session if i['session_present'] else 0)
        elif address == 0x64c40:
            self.returned(i['protocol'])
        elif address == 0x64160:
            self.returned(1)  # Explicit online session subtype, if requested.
        elif address == 0x64970:
            self.w(self.arg(0), i['campaign']); self.w(self.arg(1), i['map_id'])
            self.w(self.arg(2), 0)
            self.returned(i['map_present'], 12)
        elif address == 0x19aa17:
            self.last_lookup_slot = self.arg(0)
            self.returned(0 if i['player_present'] else -1, 4)
        elif address == 0x19aaa5:
            assert guest.reg_read(UC_X86_REG_EAX) == 0
            self.returned(self.player)
        elif address == 0x18fe9e:
            slot = guest.reg_read(UC_X86_REG_EDI)
            self.notifications.append([slot, self.read(0x54eadc + slot * 0xc70, '<i')[0]])
            self.returned()
        elif address == 0x59330:
            self.returned(i['unready'])
        elif address == 0x14917c:
            assert self.arg(0) == 1
            self.returned(i['busy'], 4)
        elif address == 0x19a902:
            self.returned(0)  # Supplied idle lobby: no countdown.
        elif address == 0x2300ea:
            self.delegations.append(self.read(self.arg(0), '<3i'))
            self.returned(i['base_result'], 4)
        else:
            return
        self.seams[hex(address)] += 1

    def preview(self, slot):
        return self.read(self.team_screen + 0x814 + slot * 4, '<BBh')

    def set_preview(self, slot, valid, team):
        self.g.mem_write(self.team_screen + 0x814 + slot * 4,
                        struct.pack('<BBh', valid, 0xa0 + slot, team))

    def result(self, slot):
        return [*self.preview(slot), self.read(0x54eadc + slot * 0xc70, '<i')[0],
                len(self.notifications), len(self.delegations)]

    def event_run(self, slot, event_type, param):
        self.g.mem_write(self.event, struct.pack('<3i', event_type, slot, param))
        returned = self.run(0x250b1e, argv=[self.event],
                            registers=[(UC_X86_REG_ECX, self.team_screen)]) & 255
        return [returned, *self.result(slot)]


def main():
    t = Teams()
    rows = ['# Original retail local team preview/commit/cancel and literal map counts.']
    counts, edits, events = [], [], []
    for map_id in [*(m['map_id'] for m in t.catalog), 99999]:
        for source_type in range(11):
            t.configure(map_id=map_id, source_type=source_type)
            count = t.run(0x251139)
            raw = list(t.g.mem_read(t.definitions[map_id] + 0xc54, 16)) if map_id in t.definitions else [0] * 16
            counts.append(dict(map_id=map_id, source_type=source_type, raw=raw, count=count))
            rows.append(' '.join(map(str, ['N', map_id, source_type, *raw, count])))
    # Actual availability predicate executes through the original session enum.
    for protocol in (-1, 0, 1, 2, 3):
        for source_type in (0, 1, 2, 5, 6, 7, 9, 10):
            for flags in (0, 1):
                t.configure(protocol=protocol, source_type=source_type, flags=flags)
                admitted = t.run(0x251364, registers=[(UC_X86_REG_EAX, t.session)]) & 255
                rows.append(' '.join(map(str, ['Q', protocol, source_type, flags, admitted])))
    for slot in range(4):
        for update in (False, True):
            for present in (False, True):
                for flags in (0, 1):
                    for member in (-1, 0, 3, 7):
                        t.configure(flags=flags, member=member, player_present=present)
                        t.set_preview(slot, True, 6)
                        before = [t.preview(s) for s in range(4)]
                        t.run(0x250cda, argv=[int(update)], registers=[
                            (UC_X86_REG_ESI, t.team_screen), (UC_X86_REG_EAX, slot)])
                        assert all(t.preview(s) == before[s] for s in range(4) if s != slot)
                        result = t.result(slot)
                        edits.append(dict(slot=slot, update=update, present=present,
                            flags=flags, member=member, result=result))
                        rows.append(' '.join(map(str, ['B', slot, int(update), int(present), flags,
                            member, 1, 6, *result])))
    for map_id, source_type in [(50, 2), (50, 1), (120, 1), (120, 7), (99999, 2)]:
        for allow_none in (False, True):
            for valid in (False, True):
                for member in (-1, 0, 1, 3, 7):
                    for delta in (-1, 1, -2147483648, 2147483647):
                        t.configure(map_id=map_id, source_type=source_type, flags=1 | (32 if allow_none else 0))
                        t.set_preview(3, valid, member)
                        t.run(0x250d5c, argv=[delta], registers=[
                            (UC_X86_REG_ECX, t.team_screen), (UC_X86_REG_EAX, 3)])
                        rows.append(' '.join(map(str, ['C', map_id, source_type, int(allow_none),
                            int(valid), member, delta, t.preview(3)[2]])))
    sequences = [[(5, 2), (4, 0), (4, 0), (5, 0)],
                 [(5, 2), (2, 0), (5, 2)], [(5, 2), (4, 0), (5, 1)],
                 [(5, 2), (4, 0), (5, 13)], [(5, 2), (2, 0), (5, 12)],
                 [(2, 0), (4, 0), (5, 2)]]
    for slot in range(4):
        for flags in (0, 1, 33):
            for sequence_id, sequence in enumerate(sequences):
                t.configure(flags=flags, member=0)
                t.set_preview(slot, False, -1)
                # Unavailable X has no effect; do not exercise unrelated Leave
                # Party handling on B without an active edit in that sequence.
                if flags == 0 and sequence_id in (2, 3): continue
                for step, (kind, param) in enumerate(sequence):
                    result = t.event_run(slot, kind, param)
                    events.append(dict(slot=slot, flags=flags, sequence=sequence_id,
                        step=step, kind=kind, param=param, result=result))
                    rows.append(' '.join(map(str, ['E', slot, flags, sequence_id, step,
                        kind, param, *result])))
    for unready, busy, signed_in in [(False, False, True), (True, True, True), (True, False, False)]:
        t.configure(unready=unready, busy=busy, signed_in=signed_in)
        result = t.event_run(0, 5, 2)
        assert result == [1, 0, 0xa0, 20, 100, 0, 0]
        rows.append(' '.join(map(str, ['G', int(unready), int(busy), int(signed_in), *result])))
    OUT.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text('\n'.join(rows) + '\n')
    # Freeze every executed original instruction, with original bytes. Seams
    # remain separately named; return code is supplied only at those addresses.
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from xbe import Xbe
    x = Xbe(ROOT / 'assets/disc/default.xbe')
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    listing = []
    for address in sorted(t.visited):
        ins = next(decoder.disasm(x.read(address, 15), address))
        listing.append(f'{address:08x} {ins.bytes.hex():30} {ins.mnemonic} {ins.op_str}')
    (OUT / 'frontend-team-retail-instructions.txt').write_text('\n'.join(listing) + '\n')
    report = dict(xbe_sha256=PIN, map_sha256=hashlib.sha256(t.cache.data).hexdigest(),
        boundary=__doc__, seams=dict(t.seams), counts=counts, edits=edits, events=events,
        fixture_sha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
        rows=dict(Counter(row.split()[0] for row in rows[1:])))
    (OUT / 'frontend-team-original.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(rows=report['rows'], original_instructions=len(listing),
        fixture_sha256=report['fixture_sha256'])))


if __name__ == '__main__': main()
