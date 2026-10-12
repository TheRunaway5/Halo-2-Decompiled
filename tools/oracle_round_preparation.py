#!/usr/bin/env python3
"""Original preparation replication gate, including unprovided-owner stops.

Raw engine/clock banks and entity/flag inputs are declared API fixtures.
Original engine registration and whole preparation wrapper/phase execute.
The dirty-entity owner is stopped at entry, never replaced by a return.
This is caller policy, not original session tracking or native networking.
"""
import argparse
import json
import struct
from pathlib import Path

from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_game_over import CountdownTrace, SPECS, M, C, ROOT
from oracle_round_lifecycle import OWNERS, compact_raw_records
from oracle_mp_full_scoreboard_1_5 import literal, complete_tokens, references, sha

OUT = ROOT/'analysis/multiplayer/game-variants/round-preparation'
ENTRIES = {'retail': (0xa7810, 0xb58c0), 'tu11': (0xa85c0, 0xb6760), 'u5': (0xb1a10, 0xb7e50)}


class DirtyBoundary(Exception):
    pass


class PreparationTrace(CountdownTrace):
    def __init__(self, name):
        self.dirty = ENTRIES[name][1]
        self.boundary = None
        super().__init__(SPECS[name])

    def hook(self, g, address, size, user):
        if address == self.dirty:
            sp = g.reg_read(UC_X86_REG_ESP)
            self.boundary = dict(instruction=f'{address:08x}',
                                 return_site=f'{self.u(sp):08x}',
                                 arguments=[self.u(sp+4), self.u(sp+8)])
            raise DirtyBoundary()
        super().hook(g, address, size, user)

    def record(self):
        return dict(engine_hex=bytes(self.g.mem_read(M, 0xc18)).hex(),
                    clock_hex=bytes(self.g.mem_read(C, 0x24)).hex())

    def observe(self, entry, arguments=()):
        self.boundary = None
        before = self.record()
        try:
            self.call(entry, arguments)
            completion = 'complete_original_return'
        except DirtyBoundary:
            completion = 'stopped_before_dirty_entity_owner'
        return dict(entry=f'{entry:08x}', arguments=list(arguments), before=before,
                    after=self.record(), completion=completion, stop=self.boundary,
                    eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT/'report.json')
    parser.add_argument('--fixture', type=Path, help='Also save these original records as a native fixture')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    versions, traces, listings = {}, {}, []
    for name in ENTRIES:
        t = PreparationTrace(name)
        traces[name] = t
        wrappers, phases = [], []
        for selector in (0, 1, 2, 3, 4, 5, 7, 9):
            for entity in (0xffffffff, 0, 0x12340003, 0xfffffffe):
                for flags in (0, 8, 0xffffffff):
                    t.setup()
                    t.g.mem_write(M+0xc14, struct.pack('<I', selector))
                    t.g.mem_write(M+0x24, struct.pack('<I', entity))
                    present = t.u(t.spec['table']+selector*4) != 0
                    record = t.observe(ENTRIES[name][0], (flags,))
                    assert record['before'] == record['after']
                    assert (record['stop'] is not None) == (present and entity != 0xffffffff)
                    if record['stop']:
                        assert record['stop']['arguments'] == [entity, flags]
                    wrappers.append(dict(selector=selector, engine_present=present,
                                         entity=entity, flags=flags, **record))
        for index in (0, 32767):
            for entity in (0xffffffff, 0, 0x12340003):
                for rate in (-32768, 30):
                    t.setup(rate=rate)
                    t.g.mem_write(M+0x24, struct.pack('<I', entity))
                    t.g.mem_write(M+0x6c, struct.pack('<hh', 1, index))
                    t.g.mem_write(M+0xc00, struct.pack('<ii', 71, 3))
                    record = t.observe(OWNERS[name]['phase'])
                    after = bytes.fromhex(record['after']['engine_hex'])
                    assert struct.unpack_from('<h', after, 0x6e)[0] == (index+1 if index < 32767 else -32768)
                    assert struct.unpack_from('<i', after, 0xc00)[0] == (rate if entity == 0xffffffff else 71)
                    assert struct.unpack_from('<h', after, 0x6c)[0] == (3 if entity == 0xffffffff else 1)
                    assert (record['stop'] is None) == (entity == 0xffffffff)
                    if record['stop']:
                        assert record['stop']['arguments'] == [entity, 8]
                    phases.append(dict(index=index, entity=entity, rate=rate, **record))
        versions[name] = dict(source_sha256=sha(ROOT/t.spec['path']), owners=ENTRIES[name],
                              wrappers=wrappers, phases=phases)
        directory = args.output.parent/'instructions'
        directory.mkdir(exist_ok=True)
        for entry in (ENTRIES[name][0], OWNERS[name]['phase']):
            rows = literal(t.code, entry, t.starts)[1]
            path = directory/f'{name}-{entry:08x}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows))
            listings.append(dict(file=path.name, sha256=sha(path), rows=len(rows)))
    pairs = []
    for name in ('retail', 'tu11'):
        a, b = traces[name], traces['u5']
        left = literal(a.code, ENTRIES[name][0], a.starts)[1]
        right = literal(b.code, ENTRIES['u5'][0], b.starts)[1]
        assert complete_tokens(left, a.image) == complete_tokens(right, b.image)
        assert [(r['before'], r['after'], r['completion'])for r in versions[name]['phases']] == [
            (r['before'], r['after'], r['completion'])for r in versions['u5']['phases']]
        pairs.append(dict(earlier=name, later='u5', complete_shape_equal=True,
                          relocated_operands=references(left, right, a.image, b.image)))
    dependencies = ('tools/oracle_game_over.py', 'tools/oracle_game_variants.py',
                    'tools/oracle_mp_full_scoreboard_1_5.py', 'tools/oracle_round_lifecycle.py')
    result = compact_raw_records(dict(schema=1, boundary=__doc__, oracle_sha256=sha(__file__),
        versions=versions, instruction_patches=[], supplied_returns=[],
        source_dependencies={p:sha(ROOT/p)for p in dependencies},
        instruction_listings=listings, complete_source_pairs=pairs))
    serialized = json.dumps(result, sort_keys=True, separators=(',', ':'))+'\n'
    args.output.write_text(serialized)
    if args.fixture:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_text(serialized)
    print(json.dumps(dict(report=str(args.output), sha256=sha(args.output),
                         wrapper_records=sum(len(v['wrappers'])for v in versions.values()),
                         phase_records=sum(len(v['phases'])for v in versions.values()))))


if __name__ == '__main__':
    main()
