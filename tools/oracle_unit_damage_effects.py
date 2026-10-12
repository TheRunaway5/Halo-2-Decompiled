#!/usr/bin/env python3
"""Retain original TU1.1 shield effect attempts through the native unit adapter.

Runs unchanged DA710/DA2B0 and D8070 over relocated original Lockout metadata.
Effect allocation, reaction/animation and spatial services remain supplied by
DamageOracle. Synthetic thresholds/vitalities/flags are explicit input cases;
they do not establish live collision, shield shimmer or effect presentation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn.x86_const import UC_X86_REG_EDI
from oracle_mp_spawn_smg_damage import DamageOracle, Cache, ROOT, OBJECTS, UNIT, EVENT, ACCUM
from oracle_mp_spawn import CACHE_PIN


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f'{value & 0xffffffff:08x}'


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path,
                   default=ROOT/'analysis/damage-shields-goal-20261011')
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    cache = Cache(ROOT/'assets/disc/maps/lockout.map')
    if hashlib.sha256(cache.data).hexdigest() != CACHE_PIN:
        raise ValueError('Original Lockout cache changed')
    original = DamageOracle(cache)
    cases = []
    for shield in (1., .25, 0.):
        for amount in (0., 5., 100.):
            for radius in (False, True):
                cases.append(dict(shield=shield, body=1., amount=amount,
                                  radius=radius, threshold=.5, flags=0, bypass=0))
    # Damage threshold before a body-only death; repeat depleted hits; shield
    # bypass and unshielded body death; original authored threshold remains0.
    cases += [
        dict(shield=.25, body=.01, amount=5., radius=True, threshold=.5, flags=0, bypass=0),
        dict(shield=.25, body=.01, amount=5., radius=True, threshold=.5, flags=2, bypass=0),
        dict(shield=0., body=.01, amount=5., radius=False, threshold=.5, flags=10, bypass=0),
        dict(shield=1., body=.01, amount=5., radius=False, threshold=.5, flags=0, bypass=512),
        dict(shield=.25, body=1., amount=100., radius=False, threshold=.5, flags=0, bypass=64),
        dict(shield=0., body=.01, amount=5., radius=False, threshold=.5, flags=0, bypass=0, maximum_shield=0.),
        dict(shield=1., body=1., amount=100., radius=False, threshold=0., flags=0, bypass=0),
    ]
    lines, reports = [], []
    for n, case in enumerate(cases):
        original.fixture(cache, shield=case['shield'], body=case['body'])
        info = original.info
        definition = original.damage_definition + 0x10
        maximum = case.get('maximum_shield', original.u(OBJECTS+0xe8))
        if 'maximum_shield' in case:
            maximum = bits(maximum)
            original.w(OBJECTS+0xe8, maximum)
        original.f(info+0xa0, case['threshold'])
        original.w(definition+4, case['bypass'])
        original.sh(OBJECTS+0x10a, case['flags'])
        original.g.mem_write(EVENT+0x68, bytes([case['radius']]))
        original.f(ACCUM+0x18, case['amount'])
        original.f(ACCUM+0x1c, case['amount'])
        original.damage_events = []
        if maximum and not case['bypass'] & 512:
            original.call(0xda710, (UNIT, definition, EVENT, ACCUM))
        if not case['bypass'] & 64:
            original.call(0xda2b0, (UNIT, definition, EVENT),
                          ((UC_X86_REG_EDI, ACCUM),))
        attempts = [row['stack_arguments'][0] for row in original.damage_events
                    if row['kind']=='supplied_service' and row['entry']=='001781f0']
        damaged = original.u(info+0xa8)
        depleted = original.u(info+0xb0)
        assert damaged != depleted
        effects = [('D' if int(tag,16)==depleted else 'H', tag) for tag in attempts]
        assert all(int(tag,16) in (damaged, depleted) for tag in attempts)
        h = original.health()
        fields = [str(n), word(bits(case['shield'])), word(bits(case['body'])),
                  word(bits(case['amount'])), str(int(case['radius'])),
                  word(bits(case['threshold'])), word(case['flags']), word(case['bypass']),
                  word(maximum), h['body_bits'], h['shield_bits'], h['flags'],
                  str(h['body_stun']), str(h['shield_stun']), str(len(effects))]
        fields += [f'{kind} {tag}' for kind, tag in effects]
        lines.append(' '.join(fields))
        reports.append(dict(input=case, after=h, effect_attempts=effects,
                            calls=original.damage_events))
    fixtures = args.output/'unit-damage-effects.tsv'
    fixtures.write_text('# Unchanged original TU1.1 unit damage; explicit synthetic inputs\n'
                        +'\n'.join(lines)+'\n')
    (args.output/'unit-damage-effects-original.json').write_text(json.dumps(dict(
        method=__doc__, cases=len(cases), fixtures_sha256=sha(fixtures),
        dependencies={str(path.relative_to(ROOT)):sha(path) for path in (
            ROOT/'assets/disc/maps/lockout.map',
            ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
            Path(__file__).resolve(), ROOT/'tools/oracle_mp_spawn_smg_damage.py')},
        results=reports), indent=2)+'\n')
    print(f'{len(cases)} original unit-damage cases; '
          f'{sum(len(r["effect_attempts"]) for r in reports)} ordered effect attempts')


if __name__ == '__main__':
    main()
