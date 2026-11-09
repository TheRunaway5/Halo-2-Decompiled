#!/usr/bin/env python3
"""Authenticate remaining scheduler/controller seams without running gameplay."""
from pathlib import Path
import hashlib
import json
import os
import re
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'analysis/physics/native-outside-frame-scope-audit-20261102'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    OUT.mkdir(exist_ok=True)
    assert not (OUT/'report.json').exists()
    owned = Path(os.environ['HALO2_TEST_SCRATCH'])
    assert sha(NEW_DEFAULT) == NEW_PIN
    code = _Code(Xbe(NEW_DEFAULT), '.text')
    owners = []
    for owner in (0x14fec0, 0xb8f40, 0x13b6f0, 0xb9090, 0xb9180,
                  0xbe4a0, 0x10e630, 0x14f620, 0x151c90, 0x108080,
                  0x18a0c0, 0x18a300, 0x18a2a0, 0x1da750):
        body = complete_body(code, owner)
        instructions = [(pc, code.at(pc)) for pc in sorted(body.code)]
        listing = ''.join(f'{pc:x} {i.mnemonic} {i.op_str}\n' for pc, i in instructions)
        p = OUT/f'{owner:08x}.txt'
        p.write_text(listing)
        calls = []
        for pc, i in instructions:
            if i.mnemonic == 'call':
                calls.append(dict(site=pc, target=int(i.op_str, 16) if re.fullmatch(r'0x[0-9a-f]+', i.op_str) else i.op_str))
        owners.append(dict(owner=owner, sites=len(body.code), listing_sha256=sha(p), calls=calls))
    names = [
        'src/physics/physics_live_world_tu15.cpp',
        'src/physics/physics_live_support_tu15_generated.cpp',
        'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp',
        'src/gameplay/vehicles/source_vehicle_look_tu15_generated.cpp',
        'src/physics/physics_phantom_force_tu15_generated.cpp',
        'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp',
        'src/physics/vehicles/physics_vehicle_runtime_generated.cpp',
        'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
        'docs/PHYSICS_TU15_REMAINING_CHECKLIST.md',
        'docs/PHYSICS_NATIVE_KERNEL_MIGRATION.md',
    ]
    source = {name:sha(ROOT/name) for name in names}
    findings = [
        dict(area='Player scheduler', owner='14FEC0', authority='372 sites; Host step_objects call_actual(commands60028000)',
             remaining='Whole scheduler and named player/action children remain lifted; native Frame statistics exclude this invocation.'),
        dict(area='Object scheduler', owner='B8F40', authority='117 sites; typed candidate needs BE4A0 and10E630 services',
             remaining='Both selected object children remain separate Source owners; no whole object scheduler native admission.'),
        dict(area='Other tick phases', owners=['13B6F0','B9090','B9180'],
             remaining='Host calls remain outside native1CC8D0; later effect/light/engine owners are outside this bounded physics census too.'),
        dict(area='Generated constructors', owner='18A0C0', callers=['14F620:14F677','14F620:14F769','151C90:151CC3','151C90:151D10'],
             remaining='Static generated run_18a0c0 calls bypass selected native Host constructors.'),
        dict(area='Generated look/reorientation', owners=['18A300','18A2A0'],
             callers=['108080:1088DC','EC780:ECBC6','ED640:ED7EC'],
             remaining='Static generated callees remain lifted; their live operands/FP/caller liveness need separate qualification.'),
        dict(area='New canonical Host constructors', owner='18A0C0',
             callers=['prepare_player_look_births remote fallback','collect_native_input new-unit fallback'],
             remaining='Two current canonical raw Host calls are absent from frozen1481. Existing native92-site constructor can replace these via a narrow current-source overlay; preserve foreign caller conditions.'),
        dict(area='Lift', owner='1DA750',
             remaining='Whole force parent remains generated. Four native scalar/power/CRT sites and the scene117A8C delegation are bounded native children, not whole force-parent closure.'),
        dict(area='Conveyors', owner='1E0A60/184C50/355D0/13DE70',
             remaining='Typed PART/conveyor child closure is qualified in declared domains. Positive live conveyor/material paths, surrounding generated schedulers and exceptional function domains remain separate.'),
        dict(area='Vehicle forces/controllers', owners=['20D680','20D770'],
             remaining='Two complete native rate algorithms run through an explicit consumer scope; outer vehicle controllers remain generated. Outside an active live scope, compatibility eligibility can still decline to literal bodies. Active live scopes refuse unsupported observations instead of fallback.'),
    ]
    r = dict(passed=True, source_audit_only=True, no_gameplay_executed=True,
             no_canonical_or_active_cohort_writes=True, original_sha256=NEW_PIN,
             owned_scratch=str(owned), temporary_allocated_bytes_before_cleanup=0,
             source_sha256=source, owners=owners, findings=findings,
             checklist_links=['docs/PHYSICS_TU15_REMAINING_CHECKLIST.md#native-execution-and-parity',
                              'docs/PHYSICS_NATIVE_KERNEL_MIGRATION.md',
                              'docs/PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md'],
             next_owned_target='Isolated reuse of sole native18A0C0 at the two additional canonical Host sites; per-site native counters/no-old-call models. Does not close the generated callers or scheduler parents.',
             retained_large_artifacts=[])
    (OUT/'report.json').write_text(json.dumps(r, indent=2)+'\n')
    print(sha(OUT/'report.json'))


if __name__ == '__main__':
    main()
