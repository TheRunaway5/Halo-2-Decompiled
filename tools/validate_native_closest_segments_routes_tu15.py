#!/usr/bin/env python3
"""Private three-owner/eight-copy312620 routing and live admission proof."""
import argparse,json,os,re,subprocess
from pathlib import Path
import native_closest_segments_replacements as binding
from specialize_native_operands import specialize
from fix_physics_world_observation_tu15 import observation_baseline
from validate_native_closest_segments_frame_tu15 import ROOT,PINS,reference
from validate_native_simplex_direction_tu15 import sha
SCENE='src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
def prepare(scratch,require_binding=False):
    reference(scratch);sources=[(ROOT/n).read_text()for n in PINS];sources.append(observation_baseline(sources[0])[0]);sources.append((ROOT/SCENE).read_text());bound=[];receipts=[];blocks=[]
    for n,source in enumerate(sources):
        value=binding.replace_closest_segments(source)
        if require_binding and n!=2 and value!=source:raise ValueError('Complete production closest-segments overlay absent')
        if binding.remove_closest_segments(value)!=binding.remove_closest_segments(source):raise ValueError('Closest source insertion changed another byte')
        bound.append(value)
    variants=bound+[specialize(source)[0]for source in bound]
    helper=re.search(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',sources[3],re.S)[0]
    if binding.digest(helper)!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Closest Scene retained spill helper drift')
    for n,value in enumerate(variants):
        if binding.installed_entries(value)!=['312620']:raise ValueError('Closest source receipt differs')
        match,resolver,dispatcher=binding.context(value);original=binding.canonical_body(match[0]);prefix=binding.prefix(match[0]);at=match[0].index('{')+1+len(prefix);body=match[0][:at]+f'\n++literal_entries[{n}];'+match[0][at:]
        body=body.replace('run_'+match[1]+'(',f'bound_{n}(',1);original=original.replace('run_'+match[1]+'(',f'reference_{n}(',1)
        for old,new in [('ObservedWorldState &s','ActualState &s')]+([('State &s','WorldState &s')]if match[2]=='State'else[]):body=body.replace(old,new);original=original.replace(old,new)
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body);original=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',original)
        changes={'body':value[:match.start()]+match[0].replace('0x10u','0x20u',1)+value[match.end():],
            'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(prefix,'',1),'missing_body':value[:match.start()]+value[match.end():],
            'resolver':value.replace(resolver,resolver.replace('{','{if(false)return 0;',1),1),
            'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)};controls={}
        for name,changed in changes.items():
            if changed==value:raise ValueError('Closest context mutation seam drift: '+name)
            try:binding.installed_entries(changed)
            except ValueError as error:controls[name]=str(error)
            else:raise ValueError('Closest strict source drift accepted: '+name)
        if n<4 and binding.replace_closest_segments(value)!=value:raise ValueError('Closest source overlay not idempotent')
        if specialize(value)[0]!=(variants[n+4]if n<4 else value):raise ValueError('Closest repeated preparation changed source')
        blocks.append(f'namespace copy_{n} {{\n'+resolver+'\n'+(helper+'\n'if n%4==3 else'')+body+'\n'+original+'\n}\n')
        receipts.append(dict(variant=n,symbol=match[1],state=match[2],body_resolver_dispatcher_sha256=[binding.digest(v)for v in(match[0],resolver,dispatcher)],strict_controls=controls))
    suffix='static void run_actual(ActualState&s,unsigned n,bool reference){switch(n){'+''.join(f'case {n}:if(reference)copy_{n}::reference_{n}(s);else copy_{n}::bound_{n}(s);return;'for n in(0,4))+'default:throw std::runtime_error("Wrong closest actual source copy");}}\n'
    suffix+='static void run_canonical(WorldState&s,unsigned n,bool reference){switch(n){'+''.join(f'case {n}:if(reference)copy_{n}::reference_{n}(s);else copy_{n}::bound_{n}(s);return;'for n in(1,2,3,5,6,7))+'default:throw std::runtime_error("Wrong closest canonical or Scene source copy");}}\n'
    text='static std::array<unsigned,8>literal_entries{};\n'+'\n'.join(blocks)+suffix;(scratch/'closest_segments_routes.inc').write_text(text)
    return dict(source_variants=receipts,source_copy_sha256=binding.digest(text),retained_scene_spill_helper_sha256=binding.digest(helper),test_instrumentation='Only literal entry counters after exact prefixes, function/type namespace transport and authenticated retained Scene spill helper',forced_reference_scope='Exact retained source-copy behavior, independently compared at full ABI; known legacy C1 differences from unchanged original/native are counted explicitly')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,SCENE,'tools/native_closest_segments_replacements.py','tools/specialize_native_operands.py','tools/fix_physics_world_observation_tu15.py','tools/validate_native_closest_segments_frame_tu15.py','tests/native_tu15_closest_segments_routes_tests.cpp','src/physics/runtime/native_tu15_closest_segments_source_owner.hpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)};report['source_sha256']=pins
    try:
        report['preparation']=prepare(scratch,args.require_binding);binary=scratch/'routes';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_closest_segments_routes_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Closest route compile failed:\n'+'\n'.join(v for v in built.stderr.splitlines()if 'error:'in v)+built.stderr[-5000:])
        environment=os.environ.copy();environment.pop('HALO2_REFERENCE_CLOSEST_SEGMENTS',None);report['routes']={}
        for mode in('default','owner-reference','unrelated-controls'):
            env=environment.copy()
            if mode=='owner-reference':env['HALO2_REFERENCE_CLOSEST_SEGMENTS']='1'
            if mode=='unrelated-controls':
                for name in('MATRIX_LEAVES','SIMPLEX_TRIANGLE','SIMPLEX_FACE_SELECT','SIMPLEX_FACE_REFINE','POLYGON_SUPPORT','POLYGON_FEATURES','RAW_ALLOCATOR'):env['HALO2_REFERENCE_'+name]='1'
            run=subprocess.run([str(binary)],capture_output=True,text=True,env=env)
            if run.returncode:raise ValueError('Closest source routing failed '+mode+':\n'+run.stderr[-5000:])
            report['routes'][mode]=json.loads(run.stderr.strip().splitlines()[-1])
            handoff=subprocess.run([str(binary),'--handoff'],capture_output=True,text=True,env=env)
            if handoff.returncode:raise ValueError('Closest actual source deferral failed '+mode+':\n'+handoff.stderr[-5000:])
            report.setdefault('world_owner_deferral',{})[mode]=json.loads(handoff.stderr.strip().splitlines()[-1])
        run=subprocess.run([str(binary),'--refusals'],capture_output=True,text=True,env=environment)
        if run.returncode:raise ValueError('Closest live admission failed:\n'+run.stderr[-5000:])
        report['refusals']=json.loads(run.stderr.strip().splitlines()[-1]);report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
        if not report['sources_unchanged']:raise ValueError('Closest routing input drift')
        report['production_bound']=args.require_binding;report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items());args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
