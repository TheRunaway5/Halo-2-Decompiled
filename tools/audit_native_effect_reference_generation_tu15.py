#!/usr/bin/env python3
"""Read-only complete current/fresh effect-reference owner generation audit."""
import argparse,difflib,hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from native_model_marker_lookup_replacements import function
from native_selected_operand_binding import require_selected_identity
OWNERS=(0x178070,0x13a6b0,0x22b790,0x22aca0,0x22b710)
UNITS=(
 ('combat','src/gameplay/units/source_combat_action_tu15_generated.cpp','tools/generate_combat_action_tu15.py','source_run_combat_action_tu15','source_combat_action_tu15_owns_entry'),
 ('death','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp','tools/generate_death_cycle_runtime_tu15.py','source_run_death_cycle_runtime_tu15','source_death_cycle_runtime_tu15_owns_entry'),
 ('particle','src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp','tools/generate_effect_particle_frame_tu15.py','source_run_effect_particle_frame_tu15','source_effect_particle_frame_tu15_owns_entry'))
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def context(source,run,owns):
    state=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)
    if not state or source.count('using WorldState=LiveState;')!=1:raise ValueError('Effect-reference complete LiveState ownership drift')
    resolver=function(source,'selected');dispatcher=function(source,'dispatch');bodies={}
    for entry in OWNERS:
        if not re.search(r'static void run_'+f'{entry:x}'+r'\(WorldState &s\) \{',source):continue
        body=function(source,f'run_{entry:x}');require_selected_identity(source,body,{digest(resolver)})
        bodies[f'{entry:x}']=dict(sha256=digest(body),lines=len(body.splitlines()))
    return dict(owners=bodies,resolver=digest(resolver),dispatcher=digest(dispatcher),state=digest(state[0]),run=digest(function(source,run)),owns=digest(function(source,owns)))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='g++');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    files={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()for tree in('src','include','tools')for path in(ROOT/tree).rglob('*')if path.suffix in('.cpp','.hpp','.h','.inc','.py')and '__pycache__'not in path.parts}
    report=dict(passed=False,production_binding=False,owned_scratch=str(scratch),source_before={},units=[],retained_large_artifacts=[])
    names={__file__,str(ROOT/'tools/generate_physics_live_support_tu15.py'),str(ROOT/'tools/native_model_marker_lookup_replacements.py'),str(ROOT/'tools/native_selected_operand_binding.py')}
    names.update(str(ROOT/unit)for _,unit,*_ in UNITS);names.update(str(ROOT/generator)for _,_,generator,*_ in UNITS)
    for name in names:
        path=Path(name).resolve();relative=str(path.relative_to(ROOT));report['source_before'][relative]=files[relative]
    try:
        for name,unit,generator,run,owns in UNITS:
            directory=scratch/name;directory.mkdir();fresh=directory/'fresh.cpp'
            command=[sys.executable,'-B',str(ROOT/generator),'--output',str(fresh),'--directory',str(directory/'generation')]
            generated=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=300)
            if generated.returncode:raise ValueError(name+' complete generation failed '+generated.stderr[-7000:])
            before=(ROOT/unit).read_text();after=fresh.read_text();current=context(before,run,owns);new=context(after,run,owns)
            deltas={}
            for entry in current['owners']:
                old=function(before,'run_'+entry);new_body=function(after,'run_'+entry)
                if old!=new_body:deltas[entry]=''.join(difflib.unified_diff(old.splitlines(keepends=True),new_body.splitlines(keepends=True),fromfile='current',tofile='fresh'))
            checked=[]
            for origin,path in(('current',ROOT/unit),('fresh',fresh)):
                dep=directory/(origin+'.d');cmd=[a.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/gameplay/weapons'),'-MD','-MF',str(dep),str(path)]
                result=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
                if result.returncode:raise ValueError(name+' '+origin+' complete CPP syntax failed '+result.stderr[-7000:])
                checked.append(dict(origin=origin,command=cmd,passed=True))
            report['units'].append(dict(name=name,source=unit,generator_command=command,current=current,fresh=new,current_source=digest(before),fresh_source=digest(after),complete_cpp_checks=checked,exact_retained_body_deltas=deltas))
        # The complete code snapshot is disposable. Retain hashes only for
        # maintained generator/authority sources, never generated banks/code.
        report.update(passed=True,complete_cpp_checks=6,owner_contexts=6,whole_current_fresh_source_equality_claim=False,
            behavioral_equivalence_claim=False,scope='Read-only exact current/fresh owner/context identities, bounded source deltas and whole-CPP syntax only')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
