#!/usr/bin/env python3
"""Unchanged TU1.1 Needler type constructor with declared generic storage.

This isolates FC2E0, not the full allocation/muzzle/guidance/impact world. Input
position/basis/inherited velocity and absent parent/platform are fixtures. All
Needler authored fields, random writes and constructor outputs are original.
"""
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import (
    ROOT, PIN, CACHE_PIN, Trace, POOL, HEADERS, BITMAP, PROJECTILE, SEED, TIME,
    UC_X86_REG_EAX,
)
from oracle_smg_projectile_1_1 import DESCRIPTOR, ERROR, PI
from inspect_multiplayer import Cache


def main():
    trace = Trace(1)
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    weapon = next(t for t in cache.tags.values() if t['group'] == 'weap' and
                  t['name'] == r'objects\weapons\pistol\needler\needler')
    barrel = cache.block(weapon['offset'] + 0x2d0, 0xec)[0]
    definition = cache.u(barrel + 0x90)
    tag = cache.tags[definition]
    assert tag['group'] == 'proj'
    trace.w(POOL + 0x38, 3)
    trace.w(BITMAP, 7)
    trace.sh(TIME + 2, 30)
    descriptor = bytearray(0xc4)
    struct.pack_into('<I', descriptor, 0, definition)
    struct.pack_into('<3f', descriptor, 0x1c, 1.25, -2.5, .75)
    struct.pack_into('<3f', descriptor, 0x28, 1., 0., 0.)
    struct.pack_into('<3f', descriptor, 0x34, 0., 0., 1.)
    struct.pack_into('<fII', descriptor, 0x58, 1., 0xffffffff, 0xffffffff)
    rows = []
    instructions = {}
    from unicorn import UC_HOOK_CODE

    def audit(g, address, size, _):
        instructions[address] = bytes(g.mem_read(address, size)).hex()

    trace.g.hook_add(UC_HOOK_CODE, audit)
    seeds = [0, 1, 0x12345678, 0xffffffff]
    seeds += [(n * 0x9e3779b9) & 0xffffffff for n in range(1, 97)]
    for index, seed in enumerate(seeds):
        for inherited in ((0., 0., 0.), (1.5, -2., .25)):
            trace.active = 'declared_storage_then_original_needler_FC2E0'
            trace.tick = index
            trace.w(TIME + 8, index)
            trace.w(SEED, seed)
            struct.pack_into('<3f', descriptor, 0x40, *inherited)
            trace.g.mem_write(DESCRIPTOR, bytes(descriptor))
            trace.g.mem_write(PROJECTILE, bytes(0x1000))
            trace.g.mem_write(HEADERS + 24, struct.pack('<HBBHHI', 1, 3, 5, 0xffff, 428, PROJECTILE))
            trace.w(PROJECTILE, definition)
            trace.w(PROJECTILE + 0x14, -1)
            trace.sh(PROJECTILE + 0x2c, -1)
            for source, target, length in ((0x1c, 0x64, 12), (0x28, 0x70, 12),
                                           (0x34, 0x7c, 12), (0x58, 0xa0, 4)):
                trace.g.mem_write(PROJECTILE + target, bytes(descriptor[source:source + length]))
            trace.g.mem_write(PROJECTILE + 0x88, struct.pack('<3f', *inherited))
            first = len(trace.events)
            trace.run(0xfc2e0, (PI, DESCRIPTOR, ERROR))
            events = trace.events[first:]
            random = [e for e in events if 'rng_write_instruction' in e]
            assert len(random) == 2
            assert not any('service' in e for e in events)
            assert trace.g.reg_read(UC_X86_REG_EAX) & 255 == 1
            raw = bytes(trace.g.mem_read(PROJECTILE, 428))
            rows.append(dict(seed_before=f'{seed:08x}', seed_after=f'{trace.u(SEED):08x}',
                             inherited_velocity=list(inherited), descriptor_hex=descriptor.hex(), object_hex=raw.hex(),
                             fields={f'{at:03x}': f'{trace.u(PROJECTILE + at):08x}' for at in
                                     (0x12c, 0x15c, 0x164, 0x16c, 0x170, 0x180, 0x184, 0x188, 0x18c, 0x190)},
                             velocity=trace.vec(PROJECTILE + 0x88), rng_stores=random))
    constants = {f'{at:08x}': f'{trace.u(at):08x}' for at in (0x465030, 0x4650c0)}
    for address, raw in instructions.items():
        assert trace.image.read(address,len(bytes.fromhex(raw))) == bytes.fromhex(raw)
    out = ROOT / 'analysis/multiplayer/integration/needler-projectile-birth-original-20261012.json'
    out.write_text(json.dumps(dict(scope=__doc__, xbe_sha256=PIN, cache_sha256=CACHE_PIN,
        oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_oracle_sha256=hashlib.sha256((ROOT / 'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
        tag=tag, authored_hex=bytes(cache.data[tag['offset']:tag['offset'] + tag['size']]).hex(),
        descriptor_hex=descriptor.hex(), constants=constants, rows=rows,
        instruction_patches=[], executed_instructions={f'{a:08x}': v for a, v in sorted(instructions.items())}),
        indent=2, sort_keys=True) + '\n')
    fixture = ROOT / 'tests/fixtures/needler_projectile_birth_original.tsv'
    lines = ['# Original FC2E0 Needler constructor; explicit generic storage and descriptor in provenance.']
    for row in rows:
        values = [int(row['seed_before'], 16), int(row['seed_after'], 16)]
        values += list(struct.unpack('<3I', struct.pack('<3f', *row['inherited_velocity'])))
        values += [int(row['fields'][f'{at:03x}'], 16) for at in (0x15c, 0x164, 0x16c, 0x170, 0x18c, 0x190)]
        values += [int(v, 16) for v in row['velocity']['bits']]
        lines.append(' '.join(f'{v:08x}' for v in values))
    fixture.write_text('\n'.join(lines) + '\n')
    print(json.dumps(dict(report=str(out), cases=len(rows), original_rng_stores=sum(len(r['rng_stores']) for r in rows),
                          unchanged_instructions=len(instructions))), flush=True)


if __name__ == '__main__':
    main()
