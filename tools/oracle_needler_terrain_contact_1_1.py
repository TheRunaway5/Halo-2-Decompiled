#!/usr/bin/env python3
"""Bounded original Needler terrain contact with explicit collision/storage.

Inherits the SMG oracle's declared generic storage, node, world and audio
services, then executes the actual Needler type constructor. This is not the
Needler upper allocator, terrain traversal, posed target attachment or Havok.
"""
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_impact_1_1 import ImpactTrace, COLLISION
from oracle_smg_projectile_1_1 import DESCRIPTOR, ERROR, PI
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, PROJECTILE, SEED, UC_X86_REG_EAX


def main():
    trace = ImpactTrace(clear=True)
    descriptor = bytearray(bytes.fromhex(next(e['request_descriptor_hex'] for e in trace.events
                                              if e.get('entry') == '000b8a30')))
    definition = 0xe3960221
    struct.pack_into('<I', descriptor, 0, definition)
    trace.g.mem_write(DESCRIPTOR, bytes(descriptor))
    trace.w(PROJECTILE, definition)
    trace.w(PROJECTILE + 0x12c, 0)
    trace.g.mem_write(PROJECTILE + 0x88, bytes(12))
    trace.w(SEED, 0x12345678)
    trace.active = 'needler_original_constructor'
    trace.run(0xfc2e0, (PI, DESCRIPTOR, ERROR))
    before = bytes(trace.g.mem_read(PROJECTILE, 428))
    raw = bytearray(0x5c)
    struct.pack_into('<If3f', raw, 0, 1, .5, .1, 0, 0)
    struct.pack_into('<h', raw, 0x24, 0)
    struct.pack_into('<3f', raw, 0x28, -1, 0, 0)
    for at in (0x14, 0x1c, 0x38, 0x3c, 0x40, 0x48, 0x4c, 0x50):
        struct.pack_into('<I', raw, at, 0xffffffff)
    for at in (0x18, 0x20, 0x44, 0x46, 0x5a):
        struct.pack_into('<H', raw, at, 0xffff)
    trace.g.mem_write(COLLISION, bytes(raw))
    velocity, output = 0x10e9300, 0x10e9400
    trace.g.mem_write(velocity, bytes(trace.g.mem_read(PROJECTILE + 0x88, 12)))
    first = len(trace.events)
    trace.active = 'impact_needler_terrain'
    trace.run(0xfeb40, (PI, COLLISION, output, velocity, 1), ((UC_X86_REG_EAX, velocity),))
    report = dict(scope=__doc__, xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  inherited_oracle_sha256=hashlib.sha256((ROOT / 'tools/oracle_smg_impact_1_1.py').read_bytes()).hexdigest(),
                  descriptor_hex=descriptor.hex(), collision_hex=raw.hex(),
                  before_hex=before.hex(), after_hex=bytes(trace.g.mem_read(PROJECTILE, 428)).hex(),
                  output_point=trace.vec(output), output_velocity=trace.vec(velocity),
                  events=trace.events[first:], original_calls=trace.impact_events,
                  instruction_patches=[],
                  boundaries={f'{a:08x}': v for a, v in trace.services.items()})
    target = ROOT / 'analysis/multiplayer/integration/needler-terrain-contact-original-20261012.json'
    target.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(report=str(target), flags=f'{trace.u(PROJECTILE + 0x12c):08x}',
                          state=trace.u(PROJECTILE + 0x130, 2), parent=f'{trace.u(PROJECTILE + 0x14):08x}')), flush=True)


if __name__ == '__main__':
    main()
