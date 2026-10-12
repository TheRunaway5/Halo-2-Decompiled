#!/usr/bin/env python3
"""Retain the actual outer physics-frame input from the unchanged original run.

This private checkpoint is original-oracle input only. Capture is read-only;
the complete preexisting factory/player/world measurement must remain identical.
"""
import argparse
import hashlib
import json
import sys
import zlib
from pathlib import Path
import unicorn
from unicorn import x86_const as R
import oracle_physics_world_event_runtime_20261011 as original
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from export_mp_physics_contact_lifecycle import segments
from original_physics_file_hash import StatCheckedSha256

sha = StatCheckedSha256()
PARENT = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-event-runtime-20261011/disc-original-v5/fixture.json'
PARENT_PIN = '203e132fa7ccc83744682ba113f7d575366f6164cd38bc7e1327b28a8d4d033c'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--correspondence', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    assert sha(PARENT) == PARENT_PIN
    pair_pin = sha(a.correspondence)
    pair = json.loads(a.correspondence.read_text())
    assert pair['disc_1_1_sha256'] == PIN
    known = {int(p, 16) for p in pair['instruction_map']}
    assert 0x1c7660 in known
    tool_pin, retained_pin = sha(__file__), sha(original.__file__)
    path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path) == PIN
    image = Xbe(path)
    text = image.section('.text')
    a.output.mkdir(parents=True, exist_ok=False)
    directory = a.output / 'memory'
    directory.mkdir()
    captured, cases = [], []
    ParentTrace = original.Trace

    class FrameTrace(ParentTrace):
        def __init__(self, data):
            self.frame_active = None
            super().__init__(data)
            self.g.hook_add(unicorn.UC_HOOK_MEM_READ, self.frame_read)

        def hook(self, g, address, size, user):
            c = self.frame_active
            if c and (address == c['return_address'] or address not in known or address in self.services):
                c.update(exit=original.captured_abi(g), exit_entry=address,
                    complete=address == c['return_address'],
                    explicit_service_boundary=address in self.services)
                self.frame_active = None
            if self.slot_ready and address == 0x1c7660:
                assert not captured, 'Multiple frame entries need separate records'
                assert bytes(g.mem_read(text.va, text.rsize)) == image.section_bytes(text)
                entry = dict(entry=address, entry_abi=original.captured_abi(g), regions=[],
                    gdtr=list(g.reg_read(R.UC_X86_REG_GDTR)), ldtr=list(g.reg_read(R.UC_X86_REG_LDTR)),
                    segment_selectors={n: g.reg_read(getattr(R, 'UC_X86_REG_'+n))
                                       for n in ('CS', 'DS', 'ES', 'SS', 'FS', 'GS')})
                for n, (start, end, protection) in enumerate(g.mem_regions()):
                    raw = bytes(g.mem_read(start, end-start+1))
                    p = directory / f'{n:04d}.zlib'
                    p.write_bytes(zlib.compress(raw, level=1))
                    entry['regions'].append(dict(start=start, size=len(raw), protection=protection,
                        file=p.name, sha256=hashlib.sha256(raw).hexdigest(), compressed_sha256=sha(p)))
                captured.append(entry)
                c = dict(id=0, phase=self.active, entry=address, entry_abi=entry['entry_abi'],
                    return_address=self.u(g.reg_read(R.UC_X86_REG_ESP)),
                    instructions=[], events=[], estimates=[], timestamps=[], packets=[])
                cases.append(c)
                self.frame_active = c
            if self.frame_active:
                self.frame_active['instructions'].append(address)
                # Estimate observations remain in the independently retained
                # inner-world fixture; a new reached CPU owner stops capture.
                ins = self.runtime_code.at(address)
                if ins.mnemonic in ('rsqrtss', 'rdtsc'):
                    self.frame_active.update(exit=original.captured_abi(g), exit_entry=address,
                        complete=False, explicit_service_boundary=False,
                        cpu_boundary=ins.mnemonic)
                    self.frame_active['instructions'].pop()
                    self.frame_active = None
            super().hook(g, address, size, user)

        def frame_read(self, g, _access, p, n, _value, _user):
            if self.frame_active:
                self.frame_active['events'].append(dict(kind='R', instruction=g.reg_read(R.UC_X86_REG_EIP),
                    address=p, raw=bytes(g.mem_read(p, n)).hex()))

        def mem_write(self, g, access, p, n, value, user):
            if self.frame_active:
                self.frame_active['events'].append(dict(kind='W', instruction=g.reg_read(R.UC_X86_REG_EIP),
                    address=p, before=bytes(g.mem_read(p, n)).hex(),
                    written=(value & ((1 << (8*n))-1)).to_bytes(n, 'little').hex()))
            super().mem_write(g, access, p, n, value, user)

    original.Trace = FrameTrace
    argv = sys.argv
    try:
        sys.argv = [str(original.__file__), '--output', str(a.output / 'retained-measurement')]
        original.main()
    finally:
        sys.argv = argv
        original.Trace = ParentTrace
    measured = a.output / 'retained-measurement/fixture.json'
    assert sha(measured) == PARENT_PIN, 'Read-only outer capture changed the original retained run'
    assert len(captured) == len(cases) == 1 and 'exit' in cases[0]
    for c in cases:
        initial, current = {}, {}
        for e in c['events']:
            p = e['address']
            raw = bytes.fromhex(e['raw'] if e['kind'] == 'R' else e['before'])
            for n, value in enumerate(raw):
                if p+n not in current:
                    initial[p+n] = current[p+n] = value
                assert current[p+n] == value
            if e['kind'] == 'W':
                current.update({p+n: v for n, v in enumerate(bytes.fromhex(e['written']))})
        c['initial_banks'] = [dict(address=p, hex=b.hex()) for p, b in segments(initial)]
        c['final_banks'] = [dict(address=p, hex=b.hex()) for p, b in segments(current)]
        c['writes'] = [e for e in c.pop('events') if e['kind'] == 'W']
    assert sha(__file__) == tool_pin and sha(original.__file__) == retained_pin
    assert sha(a.correspondence) == pair_pin
    common = dict(schema=1, tool_sha256=tool_pin, original_capture_tool_sha256=retained_pin,
        original_sha256=PIN, correspondence_sha256=pair_pin,
        parent_fixture_sha256=PARENT_PIN, unchanged_original_prefix_sha256=sha(measured),
        original_text_unpatched=True, native_expected_data_used=False,
        production_checkpoint_replay=False, live_gameplay_proof=False, scope=__doc__)
    checkpoint = a.output / 'checkpoint.json'
    checkpoint.write_text(json.dumps(dict(common, captured_entry=captured[0]), indent=2, sort_keys=True)+'\n')
    fixture = a.output / 'fixture.json'
    fixture.write_text(json.dumps(dict(common, image='1.1', cases=cases), sort_keys=True, separators=(',', ':'))+'\n')
    c = cases[0]
    print(json.dumps(dict(checkpoint_sha256=sha(checkpoint), fixture_sha256=sha(fixture),
        instructions=len(c['instructions']), writes=len(c['writes']), stop=hex(c['exit_entry']),
        explicit_service_boundary=c['explicit_service_boundary'])))


if __name__ == '__main__':
    main()
