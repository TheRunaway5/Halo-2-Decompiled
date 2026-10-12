#!/usr/bin/env python3
"""Observe original node delivery and camera lookup on a naturally admitted corpse.

The frozen moving-death phases and eight complete original ticks produce the
body bank. The only added inputs are the declared 1D7310 caller-owned zero mask
and BBCB0 callee-ABI datum queries; no pose, admission, contact or lookup result
is assigned. This is owner integration evidence, not native gameplay proof.
"""
import argparse
import json
from pathlib import Path
from unicorn import x86_const as reg
import oracle_mp_spartan_ragdoll_constructor_1_1 as ctor
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, _trace

p = ctor.prior


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    ctor.verify()
    a.output.parent.mkdir(parents=True, exist_ok=True)
    data = json.loads((ROOT / 'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261011/with-math/raw-host-context.json').read_text())
    p.motion.CommandPoseTrace = ctor.ConstructorTrace
    t, _ = p.motion.prepare(data)
    for lo, hi, permissions, raw in t.source_fixture:
        t.g.mem_write(lo, raw)
    t.g.context_restore(t.source_fixture_cpu)
    p.motion.probe(t, 'authored_heading_forward', 0x3f800000, 0, True)
    t.selected_physics_fixup(0xe9f2086d)
    t.death_enabled = True
    for name, entry, args in (
        ('original_effect_storage_constructor', 0x1775d0, ()),
        ('original_effect_map_reset', 0x177690, ()),
        ('original_current_damage_owner', 0xd7cd0, (t.spawned_unit, PLACEMENT + 0x800)),
        ('explicit_original_death_request', 0xd7e00, (t.spawned_unit, PLACEMENT + 0x800, 1, 0)),
        ('explicit_original_ordinary_death_cleanup', 0xcbdc0, (t.spawned_unit, 0xffffffff, 0, 0, 0)),
        ('original_after_death_object_prephysics', 0xb7e00, ()),
        ('original_after_death_world_physics', 0x1c7660, ()),
        ('original_after_death_object_postphysics', 0xb7f50, ()),
        ('original_after_death_object_newflags', 0xb8040, ()),
    ):
        assert t.phase(name, entry, args)
    old = json.loads((ctor.BASE / 'report.json').read_text())
    for k, v in (('stages', t.death_stages), ('calls', t.death_calls),
                 ('reads', t.death_reads), ('stores', t.death_stores)):
        assert old[k] == v, k
    t.death_enabled = t.constructor_observe = False
    for tick in range(8):
        t.g.emu_start(0x139863, 0x13986b, count=2)
        for entry in (0x13a380, 0xb7e00, 0x1c7660, 0xb7f50, 0xb8040):
            t.run(entry)
    pool = t.u(0x4e78d0)
    unit = t.u(t.u(pool + 0x44) + (t.spawned_unit & 0xffff) * 12 + 8)
    component = t.u(t.u(0x526248) + 0x44) + (t.u(unit + 0xb4) & 0xffff) * 0xa0
    old = json.loads((ROOT / 'analysis/multiplayer/integration/death-runtime-20261011/natural-admission-v2/report.json').read_text())['frames'][8]
    assert t.u(t.u(0x518444) + 8) == old['clock'] == 8
    assert bytes(t.g.mem_read(unit, 5750)).hex() == old['unit_hex']
    assert bytes(t.g.mem_read(component, 0xa0)).hex() == old['component_hex']
    assert t.u(component + 0x74) == 10 and t.u(component + 0x80) == 9
    offsets = {k: len(v) for k, v in (('stages', t.death_stages),
               ('calls', t.death_calls), ('reads', t.death_reads), ('stores', t.death_stores))}
    p.ENTRIES.update(t.starts | {0xbbcb0, 0x23fd40})
    t.death_enabled = t.constructor_observe = True
    t.death_executed = set()
    mask = PLACEMENT + 0x1800
    t.g.mem_write(mask, bytes(32))
    before_unit = bytes(t.g.mem_read(unit, 5750)).hex()
    assert t.phase('natural_corpse_physical_nodes', 0x1d7310, (component, mask, 0))
    queries = []
    for datum in (t.spawned_unit, t.spawned_unit ^ 0x10000, 0xffffffff,
                  (t.spawned_unit & 0xffff0000) | t.u(pool + 0x38)):
        assert t.phase(f'camera_full_datum_{datum:08x}', 0xbbcb0, (0xffffffff,),
                       ((reg.UC_X86_REG_EAX, datum),))
        result = t.g.reg_read(reg.UC_X86_REG_EAX)
        row = dict(datum=f'{datum:08x}', body=f'{result:08x}')
        if result:
            node = result + t.u(result + 0x116, 2)
            row.update(node0_hex=bytes(t.g.mem_read(node + 0x28, 12)).hex(),
                       type=t.u(result + 0xaa, 1), biped_flags=t.u(result + 0x348, 2))
        queries.append(row)
    player_pool = t.u(0x4f01f4)
    high_water, stride = t.u(player_pool + 0x38), t.u(player_pool + 0x24)
    player_backing = [dict(address=f'{at:08x}', raw_hex=bytes(t.g.mem_read(at, n)).hex())
                      for at, n in ((player_pool, 0x54),
                                    (t.u(player_pool + 0x44), high_water * stride),
                                    (t.u(player_pool + 0x48), ((high_water + 31) // 32) * 4))]
    players = []
    for index in range(high_water):
        if not t.u(t.u(player_pool + 0x48) + (index >> 5) * 4) & (1 << (index & 31)):
            continue
        row = t.u(player_pool + 0x44) + index * stride
        players.append(dict(datum=f'{(t.u(row, 2) << 16) | index:08x}',
                            current=f'{t.u(row + 0x2c):08x}', previous=f'{t.u(row + 0x30):08x}',
                            team=t.u(row + 0xc0, 1)))
    selections = []
    for watched in (0xffffffff, t.spawned_player):
        for team in (0, 1):
            assert t.phase(f'camera_player_selection_{watched:08x}_{team}',
                           0x23fd40, (t.spawned_player, watched, team))
            selections.append(dict(own=f'{t.spawned_player:08x}', watched=f'{watched:08x}',
                                   team=team, result=f'{t.g.reg_read(reg.UC_X86_REG_EAX):08x}'))
    calls = t.death_calls[offsets['calls']:]
    listings = []
    entries = {int(c['entry'], 16) for c in calls}
    for entry in sorted(entries):
        f = _trace(t.code, entry, t.starts | entries)
        rows = [t.code.at(at) for at in sorted(f.code)]
        for r in rows:
            assert r.bytes == t.image.read(r.address, len(r.bytes)) == bytes(t.g.mem_read(r.address, len(r.bytes)))
        path = a.output.parent / 'instructions' / f'{entry:08x}.asm'
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n' for r in rows))
        listings.append(dict(entry=f'{entry:08x}', file=str(path.relative_to(a.output.parent)), sha256=p.digest(path)))
    a.output.write_text(json.dumps(dict(
        oracle_sha256=p.digest(__file__), source_sha256=PIN, cache_sha256=CACHE_PIN,
        frozen_prior_byte_identical=True, natural_admission_snapshot_byte_identical=True,
        component=f'{component:08x}', unit=f'{unit:08x}', pool=f'{pool:08x}',
        mask=f'{mask:08x}', before_unit_hex=before_unit,
        after_unit_hex=bytes(t.g.mem_read(unit, 5750)).hex(),
        queries=queries, player_pool=f'{player_pool:08x}', player_backing=player_backing,
        players=players, selections=selections,
        calls=calls, reads=t.death_reads[offsets['reads']:],
        stores=t.death_stores[offsets['stores']:], stages=t.death_stages[offsets['stages']:],
        instructions=listings, instruction_patches=[], supplied_gameplay_results=[],
        boundaries=__doc__), indent=2) + '\n')
    print(json.dumps(dict(report_sha256=p.digest(a.output), calls=len(calls), queries=queries)), flush=True)


if __name__ == '__main__':
    main()
