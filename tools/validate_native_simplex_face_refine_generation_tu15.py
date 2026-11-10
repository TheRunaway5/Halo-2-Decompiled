#!/usr/bin/env python3
"""Full current/fresh Primary+Death compilation and exact refinement overlays."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_simplex_face_refine_replacements as binding
from generated_routine_ownership import function_bodies
from specialize_native_operands import specialize
from validate_native_simplex_direction_tu15 import ROOT,sha
OWNERS={'primary':'src/physics/physics_live_support_tu15_generated.cpp','death':'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,scratch_owner=str(scratch),retained_large_artifacts=[],units=[])
    files=[*OWNERS.values(),'tools/generate_physics_live_support_tu15.py','tools/generate_death_cycle_runtime_tu15.py','tools/native_simplex_face_refine_replacements.py','tools/specialize_native_operands.py','src/physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp','src/physics/runtime/native_tu15_simplex_face_refine_abi.hpp','include/halo2/native_tu15_simplex_face_refine_kernel.hpp',str(Path(__file__).relative_to(ROOT))];pins={n:sha(ROOT/n)for n in files}
    try:
        for name,path in OWNERS.items():
            current=(ROOT/path).read_text();assert binding.replace_simplex_face_refine(current)==current;binding.installed_entries(current)
            # Each surgical overlay leaves every other complete routine byte-identical.
            original=binding.remove_simplex_face_refine(current);old={s.symbol:b for s,b in function_bodies(original)};new={s.symbol:b for s,b in function_bodies(current)}
            if set(old)!=set(new)or any(b!=new[n]for n,b in old.items()if n!='30d1a0'):raise ValueError('Refinement overlay changed another complete routine')
            for phase,source in [('current',current),('fresh',None)]:
                if phase=='fresh':
                    output=scratch/(name+'-fresh.cpp');script='generate_physics_live_support_tu15.py'if name=='primary'else'generate_death_cycle_runtime_tu15.py';flag='--manifest'if name=='primary'else'--directory';command=[str(ROOT/'.tools/python/bin/python3'),'-B',str(ROOT/'tools'/script),'--output',str(output),flag,str(scratch/(name+'.json'if name=='primary'else'death-data'))];run=subprocess.run(command,capture_output=True,text=True,timeout=180)
                    if run.returncode:raise ValueError('Full refinement source generation failed '+name+':\n'+run.stderr[-7000:])
                    source=output.read_text()
                    if binding.replace_simplex_face_refine(source)!=source:raise ValueError('Fresh complete generator omitted native refinement: '+name)
                binding.installed_entries(source);prepared,receipt=specialize(source)
                if receipt['native_simplex_face_refine_entries']!=['30d1a0']or specialize(prepared)[0]!=prepared:raise ValueError('Full refinement source preparation/idempotency differs')
                for prepared_mode,value in[(False,source),(True,prepared)]:
                    artifact=scratch/(name+'-'+phase+('-prepared'if prepared_mode else'-raw')+'.cpp');artifact.write_text(value);command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(artifact)];run=subprocess.run(command,capture_output=True,text=True,timeout=180)
                    if run.returncode:raise ValueError('Complete refinement unit syntax failed '+artifact.name+':\n'+run.stderr[-7000:])
                    match,resolver,dispatcher=binding.context(value);report['units'].append(dict(owner=name,phase=phase,prepared=prepared_mode,whole_source_sha256=binding.digest(value),body_resolver_dispatcher=[binding.digest(v)for v in(match[0],resolver,dispatcher)],syntax_passed=True,native_simplex_face_refine_entries=receipt['native_simplex_face_refine_entries']))
        report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items());report['source_sha256']=pins
        if not report['sources_unchanged']:raise ValueError('Full refinement generation/syntax input drift')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
