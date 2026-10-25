#!/usr/bin/env python3
"""Restore read-only constructor diagnostics required by the native aim fixture.

Only this managed source copy changes. Pin every complete production preimage
so the two diagnostic scopes can be removed before ordinary runtime delivery.
"""
import argparse
import json
import os
from pathlib import Path
from weapon_test_cache import sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';prepared=json.loads(args.prepared.read_text());pins=dict(prepared['source_hashes'])
    if Path(prepared['owner'])!=owner or any(sha(source/name)!=pin for name,pin in pins.items()):
        raise ValueError('Use this same unchanged managed candidate')
    name='src/physics/physics_live_support_tu15_generated.cpp'
    path=source/name;original=path.read_text();text=original
    include='#include "halo2/source_physics_entry_observation.hpp"\n'
    if text.startswith(include):raise ValueError('Private constructor diagnostics are already installed')
    for entry in (0xb9b70,0xba490):
        marker=f'static void run_{entry:x}(WorldState &s) {{'
        if text.count(marker)!=1:raise ValueError('Native constructor diagnostic owner changed')
        text=text.replace(marker,marker+f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{entry:x}u,[&]{{return s.save();}});',1)
    text=include+text
    restored=text[len(include):]
    for entry in (0xb9b70,0xba490):
        restored=restored.replace(f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{entry:x}u,[&]{{return s.save();}});','',1)
    if restored!=original:raise ValueError('Read-only diagnostics alter another production byte')
    args.output.mkdir(parents=True,exist_ok=False)
    before=sha(path);path.write_text(text);pins[name]=sha(path)
    report=dict(prepared,source_hashes=pins,status='prepared',passed=False,validation_not_run=True,
        previous_prepared_sha256=sha(args.prepared),private_diagnostic_hooks_removed=False,
        private_generated_owner_instrumentation=[dict(path=name,original_sha256=before,
            observed_sha256=sha(path),entries=['B9B70','BA490'])],
        original_instruction_changes=[],supplied_admission_or_fire_results=False,
        full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',diagnostic_scopes=2,output=str(args.output))),flush=True)


if __name__=='__main__':main()
