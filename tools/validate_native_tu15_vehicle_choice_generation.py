#!/usr/bin/env python3
"""Fresh whole-primary generation and raw/prepared vehicle DATA admission proof."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from generated_routine_ownership import function_bodies
from native_vehicle_choice_data_replacements import replace_vehicle_choice_data,prefix
from native_vehicle_choice_kernel_tu15 import PINS
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    tools=('tools/generate_physics_live_support_tu15.py','tools/native_vehicle_choice_data_replacements.py','tools/native_vehicle_choice_kernel_tu15.py','tools/specialize_native_operands.py','tools/validate_native_tu15_vehicle_choice_generation.py')
    pins={name:sha(ROOT/name)for name in tools};raw=scratch/'primary.cpp';manifest=scratch/'generation.json'
    commands=[[sys.executable,'-B',str(ROOT/tools[0]),'--output',str(raw),'--manifest',str(manifest)]]
    subprocess.run(commands[-1],check=True)
    source=raw.read_text()
    if replace_vehicle_choice_data(source)!=source:raise ValueError('Fresh primary lost complete vehicle DATA binding')
    prepared,receipt=specialize(source);specialized=scratch/'primary.prepared.cpp';specialized.write_text(prepared)
    for name,text in(('raw',source),('prepared',prepared)):
        bodies={signature[1]:body for signature,body in function_bodies(text)}
        for entry,(family,_)in PINS.items():
            if prefix(family)not in bodies[f'{entry:x}']:raise ValueError(f'{name} vehicle DATA prefix missing for{entry:x}')
    for path in(raw,specialized):
        command=['clang++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)]
        commands.append(command);subprocess.run(command,check=True)
    if any(sha(ROOT/name)!=pin for name,pin in pins.items()):raise ValueError('Fresh vehicle preparation tools changed during proof')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,
        complete_owners=len(list(function_bodies(source))),raw_sha256=sha(raw),prepared_sha256=sha(specialized),raw_and_prepared_syntax_passed=True,
        raw_authenticated_binding_idempotent=True,prepared_prefixes_preserved=True,source_sha256=pins,commands=commands,
        specialization_receipt_sha256=hashlib.sha256(json.dumps(receipt,sort_keys=True).encode()).hexdigest(),scratch_owner=str(scratch),
        runtime_executed=False,retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n');return 0
if __name__=='__main__':raise SystemExit(main())
