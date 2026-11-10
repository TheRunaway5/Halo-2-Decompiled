#!/usr/bin/env python3
"""Reproduce complete Shadow owners and qualify raw/prepared queue bindings."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import native_bsp_queue_find_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
PREPARED_PIN='0d7500836b1e1fcf1657dda2e9b7ca03b8f4914c4e003726772cd265ac035111'
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);unit=ROOT/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp';before=unit.read_bytes()
    from generated_routine_ownership import function_bodies
    if args.require_binding and binding.replace_bsp_queue_find(before.decode())!=before.decode():raise ValueError('Canonical166C00 default prefix missing')
    command=[sys.executable,'-B',str(ROOT/'tools/validate_shadow_generation_tu15.py'),'--report',str(scratch/'shadow-generation-report.json')]
    generated=subprocess.run(command,capture_output=True,text=True)
    if generated.returncode:raise ValueError('Fresh complete Shadow generation failed: '+(generated.stdout+generated.stderr)[-6000:])
    raw=scratch/'shadow-generated.cpp';text=raw.read_text();bound=binding.replace_bsp_queue_find(text)
    if args.require_binding and text!=bound:raise ValueError('Fresh166C00 default prefix missing')
    if binding.replace_bsp_queue_find(bound)!=bound:raise ValueError('Fresh166C00 default binding is not idempotent')
    from specialize_native_operands import specialize
    prepared,receipt=specialize(bound);body=next(body for sig,body in function_bodies(prepared)if sig[1]=='166c00')
    if binding.digest(body)!=PREPARED_PIN:raise ValueError('Complete prepared166C00 output body drift')
    if specialize(prepared)[0]!=prepared:raise ValueError('Repeated complete Shadow preparation is not idempotent')
    raw.write_text(bound);target=scratch/'shadow-prepared.cpp';target.write_text(prepared)
    for source in(raw,target):
        tested=subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(source)],capture_output=True,text=True)
        if tested.returncode:raise ValueError('Fresh complete Shadow syntax failed: '+tested.stderr[-6000:])
    if unit.read_bytes()!=before:raise ValueError('Canonical Shadow source changed during generation proof')
    result=dict(passed=True,owned_scratch=str(scratch),complete_shadow_generation=json.loads((scratch/'shadow-generation-report.json').read_text()),raw_native_prefix_authenticated=True,prepared_full_body_sha256=PREPARED_PIN,repeat_preparation_idempotent=True,raw_and_prepared_whole_cpp_syntax=True,compiler=args.compiler,retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,complete_shadow_owners=result['complete_shadow_generation']['entries'])));return 0
if __name__=='__main__':raise SystemExit(main())
