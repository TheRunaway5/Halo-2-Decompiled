#!/usr/bin/env python3
"""Fresh complete renderer units, strict fog identities and repeat preparation."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import native_fog_membership_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
PREPARED_PIN='4e9612a6c73c60bff7be9a56ce7bf89ad07712586488cb24d54571db0c8e156d'
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:binding.digest((ROOT/name).read_text())for name in(*UNITS,'tools/generate_physics_live_support_tu15.py','tools/native_fog_membership_replacements.py','src/rendering/native_tu15_fog_membership.hpp','include/halo2/native_tu15_fog_membership_kernel.hpp')};receipts=[]
    if args.require_binding:
        public=(ROOT/'src/rendering/world/bsp_atmosphere.cpp').read_text()
        if binding.replace_public(public)!=public:raise ValueError('Public shared fog DATA owner missing')
    from generated_routine_ownership import function_bodies
    from specialize_native_operands import specialize
    for index,unit in enumerate(UNITS):
        directory=scratch/str(index);directory.mkdir();script='validate_shadow_generation_tu15.py'if not index else'validate_native_sphere_frustum_generation_tu15.py'
        command=[sys.executable,'-B',str(ROOT/'tools'/script),'--report',str(directory/'generation-report.json')]
        if index:command+=['--compiler',args.compiler]
        tested=subprocess.run(command,capture_output=True,text=True,env=dict(os.environ,HALO2_TEST_SCRATCH=str(directory)))
        if tested.returncode:raise ValueError('Complete renderer regeneration failed: '+(tested.stdout+tested.stderr)[-6500:])
        raw=directory/('shadow-generated.cpp'if not index else'particle.cpp');text=raw.read_text();bound=binding.replace_fog_membership(text)
        if args.require_binding and bound!=text:raise ValueError('Fresh renderer fog default missing')
        if binding.replace_fog_membership(bound)!=bound:raise ValueError('Fresh fog owner not idempotent')
        prepared,receipt=specialize(bound);body=next(body for sig,body in function_bodies(prepared)if sig[1]=='171070')
        if binding.digest(body)!=PREPARED_PIN:raise ValueError('Complete prepared fog body drift')
        if specialize(prepared)[0]!=prepared:raise ValueError('Complete renderer repeat preparation drift')
        raw.write_text(bound);output=directory/'prepared.cpp';output.write_text(prepared)
        for source in(raw,output):
            checked=subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(source)],capture_output=True,text=True)
            if checked.returncode:raise ValueError('Fresh renderer whole CPP syntax failed: '+checked.stderr[-6500:])
        receipts.append(dict(source=unit,generation=json.loads((directory/'generation-report.json').read_text()),original_body_sha256=binding.digest(binding.authenticate(text)[1]),prepared_body_sha256=PREPARED_PIN,raw_prepared_whole_cpp_syntax=True))
    changed=[name for name,pin in pins.items()if binding.digest((ROOT/name).read_text())!=pin]
    if changed:raise ValueError('Fog generation input drift: '+', '.join(changed))
    result=dict(passed=True,owned_scratch=str(scratch),source_sha256=pins,units=receipts,compiler=args.compiler,retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,complete_units=2,raw_prepared_syntax_checks=4)));return 0
if __name__=='__main__':raise SystemExit(main())
