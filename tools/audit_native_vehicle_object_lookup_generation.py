#!/usr/bin/env python3
"""Authenticate current/fresh complete VehicleEnvironment native lookup candidates."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import native_vehicle_object_lookup_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/vehicles/physics_vehicle_environment_state_tu15_generated.cpp'
GEN='tools/generate_vehicle_environment_state_tu15_20261011.py'
RESEARCH='analysis/multiplayer/sandbox-source/vehicle-environment-state-tu15-20261011/research-verified/report.json'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=(UNIT,GEN,RESEARCH,'tools/native_vehicle_object_lookup_replacements.py','tools/audit_native_vehicle_object_lookup_generation.py')
    pins={name:sha(ROOT/name)for name in names};report=dict(passed=False,production_bound=a.require_binding,source_before=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        current=(ROOT/UNIT).read_text()
        if a.require_binding:binding.installed_entries(current)
        fresh=scratch/'fresh.cpp'
        command=[sys.executable,'-B',str(ROOT/GEN),'--research',str(ROOT/RESEARCH),'--output',str(fresh),'--manifest',str(scratch/'generation.json')]
        done=subprocess.run(command,capture_output=True,text=True,timeout=600)
        if done.returncode:raise ValueError('Genuine VehicleEnvironment generation failed '+(done.stdout+done.stderr)[-6000:])
        contexts=[]
        for label,source in(('current',current),('fresh',fresh.read_text())):
            original=binding.remove_vehicle_object_lookup(source);candidate=binding.replace_vehicle_object_lookup(source)
            if binding.remove_vehicle_object_lookup(candidate)!=original or binding.replace_vehicle_object_lookup(candidate)!=candidate:
                raise ValueError('Vehicle exact inverse/idempotence failed')
            body=binding.PATTERN.findall(candidate)[0];controls=[]
            def refused(name,bad):
                try:binding.replace_vehicle_object_lookup(bad)
                except ValueError as error:controls.append(dict(name=name,error=str(error)));return
                raise ValueError('Vehicle candidate accepted '+name)
            refused('duplicate-include',binding.INCLUDE+candidate)
            refused('missing-include',candidate.replace(binding.INCLUDE,'',1))
            refused('external-prefix',candidate+'\n'+binding.PREFIX+'\n')
            refused('moved-prefix',candidate.replace(body,body.replace(binding.PREFIX,'',1).replace('s.push(',binding.PREFIX+'s.push(',1),1))
            refused('wrong-root',candidate.replace(body,body.replace('0x4ecbd0u','0x4ecbd4u',1),1))
            refused('wrong-stack-publication',candidate.replace(body,body.replace('s.push(0xbcdf0u,s.reg(6,4,0));','s.gp[4]+=4u;s.push(0xbcdf0u,s.reg(6,4,0));',1),1))
            refused('wrong-public-image',candidate.replace('SourcePhysicsContactImage::TitleUpdate15','SourcePhysicsContactImage::MapPack11',1))
            refused('wrong-lower-owner',candidate.replace('source_run_physics_vehicle_constraint_cleanup_runtime(','source_run_other_vehicle_constraint_cleanup_runtime(',1))
            checks=[];path=scratch/(label+'.cpp');path.write_text(candidate)
            for compiler in('g++','clang++'):
                syntax=[compiler,'-std=c++20','-Wno-ignored-attributes','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(path)]
                built=subprocess.run(syntax,capture_output=True,text=True,timeout=600)
                if built.returncode:raise ValueError('Whole Vehicle candidate syntax failed '+built.stderr[-6000:])
                checks.append(dict(compiler=compiler,command=syntax,passed=True))
            contexts.append(dict(variant=label,original_sha256=binding.digest(original),candidate_sha256=binding.digest(candidate),whole_cpp_syntax=checks,strict_refusals=controls,all_original_source_bytes_preserved=True,other_complete_functions_preserved=25))
        report.update(passed=True,contexts=contexts,genuine_generation_command=command)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
