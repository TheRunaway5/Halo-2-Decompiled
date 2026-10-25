#!/usr/bin/env python3
"""Install independently qualified native vehicle selectors, retaining literal controls."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
INCLUDE='// BEGIN native vehicle choice include\n#include "gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp"\n// END native vehicle choice include\n'
PINS={0x15cb60:('light','3e142a0f43030b5115930bdbf751551f43c21171c34971bbbe651fa5a25c0489'),
      0x15cc70:('heavy','f83a92e6580d01427fb7d680e53d9a80fdaa2cfa808eb61963eff88e33a643e4'),
      0x15cdf0:('turret','7e98f490a3b684439c3408648b4f26bc7a6b53a3a332a7768ef07c5514f3d61b'),
      0x15cd30:('flying','893eedf85f7d16f9189ae117eef2ddc6ac7570c2a730e6e5eb7cbca476941409')}


def route(family):
    return ('// BEGIN native vehicle choice route\n'
        'if(s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
        '    vehicle_choice_native_tu15(s,VehicleChoiceFamilyTu15::'+family+',\n'
        '        [&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);});\n'
        '    return;\n}\n// END native vehicle choice route\n')


def strip_native_vehicle_choice(source):
    source=source.replace(INCLUDE,'')
    for family,_ in PINS.values():source=source.replace(route(family),'')
    if 'BEGIN native vehicle choice' in source or 'END native vehicle choice' in source:
        raise ValueError('Native vehicle-choice route changed')
    return source


def install_native_vehicle_choice(source):
    source=strip_native_vehicle_choice(source)
    for entry,(family,pin) in PINS.items():
        anchor=f'static void run_{entry:x}(WorldState &s) {{\n'
        if source.count(anchor)!=1:raise ValueError('Absent or duplicate complete selector')
        start=source.index(anchor);end=source.index('\n}\n\n',start)+2
        if hashlib.sha256(source[start:end].encode()).hexdigest()!=pin:
            raise ValueError('Complete original selector literal changed: '+hex(entry))
        source=source.replace(anchor,anchor+route(family),1)
    return INCLUDE+source


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('qualification','negative-control','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--flying-negative-control',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    q=json.loads(args.qualification.read_text());n=json.loads(args.negative_control.read_text())
    flying=json.loads(args.flying_negative_control.read_text())
    kernel=ROOT/'src/gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp'
    if(not q['passed'] or q['cases']!=3444 or q['comparisons']!=6888 or not q['source_unchanged'] or
       q['families']!=['flying','heavy','light','turret'] or
       q['native_kernel_sha256']!=sha(kernel) or not n['passed'] or not n['comparison_rejected_mutation'] or
       n['negative_control']['before']!=sha(kernel) or not flying['passed'] or
       not flying['comparison_rejected_mutation'] or flying['negative_control']['before']!=sha(kernel) or
       flying['negative_control'].get('kind')!='wrong flying reference'):
        raise ValueError('Current kernel requires whole-original qualification and deliberate mutation rejection')
    if any(sha(ROOT/name)!=pin for name,pin in q['source_hashes'].items()):
        raise ValueError('A qualified native selector dependency changed before admission')
    path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';before=path.read_text();after=install_native_vehicle_choice(before)
    if strip_native_vehicle_choice(after)!=strip_native_vehicle_choice(before):raise ValueError('An unrelated literal owner would change')
    if sha(path)!=q['literal_module_sha256']:raise ValueError('Live support changed since qualification')
    args.output.mkdir(parents=True,exist_ok=False)
    old=sha(path);path.write_text(after)
    (args.output/'routes.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='before',tofile='after',n=2)))
    report=dict(passed=True,path=str(path.relative_to(ROOT)),before=old,after=sha(path),
        literal_owners_preserved=True,body_pins=PINS,native_kernel_sha256=sha(kernel),
        qualification_sha256=sha(args.qualification),negative_control_sha256=sha(args.negative_control),
        flying_negative_control_sha256=sha(args.flying_negative_control),
        runtime_verified=False,launcher_installed=False,full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'admission.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,report=str(args.output/'admission.json'))),flush=True)


if __name__=='__main__':main()
