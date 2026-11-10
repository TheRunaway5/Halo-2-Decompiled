#!/usr/bin/env python3
"""Private complete30D5D0 source/admission and actual bound30D1A0 closure."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_simplex_face_select_replacements as binding
import native_simplex_face_refine_replacements as child_binding
from specialize_native_operands import specialize
from fix_physics_world_observation_tu15 import observation_baseline
from validate_native_simplex_face_select_tu15 import ROOT,PINS,reference
from validate_native_simplex_direction_tu15 import sha

def prepare(scratch,require_binding=False):
    reference(scratch);sources=[(ROOT/n).read_text()for n in PINS];sources.append(observation_baseline(sources[0])[0]);bound=[];blocks=[];receipts=[]
    for index,source in enumerate(sources):
        changed=binding.replace_simplex_face_select(source)
        # The third input is the deliberately stripped unobserved baseline;
        # only the two current production owners must already be installed.
        if require_binding and index<2 and changed!=source:raise ValueError('Complete source face-select overlay not installed')
        bound.append(changed)
    variants=bound+[specialize(v)[0]for v in bound]
    child_source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();child_binding.installed_entries(child_source);child_match,_,_=child_binding.context(child_source);child_body=child_match[0];at=child_body.index('{')+1+len(child_binding.PREFIX);child_body=child_body[:at]+'\n++child_literal_entries;'+child_body[at:];child_body=child_body.replace('run_30d1a0(','child_bound(',1)
    for n,value in enumerate(variants):
        if binding.installed_entries(value)!=['30d5d0']:raise ValueError('Wrong complete face-select binding receipt')
        match=binding.BODY.search(value);body=match[0];at=body.index('{')+1+len(binding.PREFIX);body=body[:at]+f'\n++literal_entries[{n}];'+body[at:];body=body.replace('run_309ae0(',f'bound_{n}(',1).replace('ObservedWorldState &s','ActualState &s').replace('State &s','WorldState &s')if match[1]=='State'else body.replace('run_309ae0(',f'bound_{n}(',1).replace('ObservedWorldState &s','ActualState &s')
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',value)[0];dispatcher=re.search(r'static void dispatch\('+match[1]+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',value)[0]
        changes={'body':value[:match.start()]+match[0].replace('0x10u','0x20u',1)+value[match.end():],'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(binding.PREFIX,'',1),'missing_body':value[:match.start()]+value[match.end():],'resolver':value.replace(resolver,resolver.replace('0x461950u','0x461960u',1),1),'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)};controls={}
        for control,changed in changes.items():
            if changed==value:raise ValueError('Face-select context mutation seam drift: '+control)
            try:binding.installed_entries(changed)
            except ValueError as error:controls[control]=str(error)
            else:raise ValueError('Face-select full context drift accepted: '+control)
        if n<3 and binding.replace_simplex_face_select(value)!=value:raise ValueError('Face-select overlay not idempotent')
        if specialize(value)[0]!=(variants[n+3]if n<3 else value):raise ValueError('Face-select preparation not idempotent')
        blocks.append(body);receipts.append(dict(variant=n,state=match[1],complete_input_sha256=binding.digest(value),body_sha256=binding.digest(match[0]),strict_context_controls=controls))
    prefix='''static std::array<unsigned,6>literal_entries{};static unsigned child_entries{},child_literal_entries,actual_child_site_witnesses{},canonical_child_site_witnesses{};
static std::uint32_t sign_extend(std::uint32_t value,unsigned width){return reference::sign_extend(value,width);}
static std::uint32_t selected(std::uint32_t value,SourcePhysicsContactImage image){return reference::selected(value,image);}
'''+child_body+'''
template<class State>static void dispatch(State&s,std::uint32_t target,std::uint32_t site,bool tail){if(target!=0x30d1a0u||site!=0x309d2fu||tail)throw std::runtime_error("Wrong genuine bound face-refine child");constexpr bool actual=requires{s.selected_observation_pcs;};const auto expected_pc=actual?0x30d81fu:0x309d2fu;if(!s.instruction_cursor||!s.instruction_cursor->position||*s.instruction_cursor->position!=expected_pc)throw std::runtime_error("Face-refine observation and internal dispatch sites conflated");if constexpr(actual)++actual_child_site_witnesses;else ++canonical_child_site_witnesses;++child_entries;child_bound(s);}
'''
    suffix='''static void run_actual(ActualState&s,unsigned n){if(n==0)bound_0(s);else if(n==3)bound_3(s);else throw std::runtime_error("Wrong actual face-select copy");}
static void run_canonical(WorldState&s,unsigned n){switch(n){case 1:bound_1(s);return;case 2:bound_2(s);return;case 4:bound_4(s);return;case 5:bound_5(s);return;default:throw std::runtime_error("Wrong canonical face-select copy");}}
'''
    text=prefix+'\n'.join(blocks)+suffix;(scratch/'simplex_face_select_routes.inc').write_text(text)
    return dict(source_variants=receipts,actual_child_body_sha256=child_binding.digest(child_match[0]),child_source_whole_context_authenticated=True,source_copy_sha256=hashlib.sha256(text.encode()).hexdigest(),child_dispatch_site_contract=dict(internal_site='309d2f',actual_before_and_push_site='30d81f',canonical_before_and_push_site='309d2f',authority='Authenticated unchanged owner bodies call dispatch with canonical309D2F; dispatcher fallback and State::required forward that site unchanged'),test_instrumentation='Only C++ literal-body-entry counters after authenticated prefixes and nonmutating child dispatch/cursor witnesses; original i386 untouched')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'src/physics/physics_live_support_tu15_generated.cpp','tools/native_simplex_face_select_replacements.py','tools/native_simplex_face_refine_replacements.py','tools/specialize_native_operands.py','tools/fix_physics_world_observation_tu15.py','tools/validate_native_simplex_face_select_tu15.py','tests/native_tu15_simplex_face_select_routes_tests.cpp','tests/native_tu15_simplex_face_select_tests.cpp','src/physics/runtime/native_tu15_simplex_face_select_source_owner.hpp','src/physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)}
    try:
        report['preparation']=prepare(scratch,args.require_binding);binary=scratch/'routes';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_face_select_routes_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Face-select route compile failed:\n'+'\n'.join([l for l in result.stderr.splitlines()if 'error:'in l])+result.stderr[-5000:])
        environment=os.environ.copy();environment.pop('HALO2_REFERENCE_SIMPLEX_FACE_SELECT',None);environment.pop('HALO2_REFERENCE_SIMPLEX_FACE_REFINE',None);report['routes']={}
        for mode in('default','parent-reference','child-reference','unrelated-controls'):
            env=environment.copy()
            if mode=='parent-reference':env['HALO2_REFERENCE_SIMPLEX_FACE_SELECT']='1'
            if mode=='child-reference':env['HALO2_REFERENCE_SIMPLEX_FACE_REFINE']='1'
            if mode=='unrelated-controls':
                for name in('MATRIX_LEAVES','SIMPLEX_TRIANGLE','POLYGON_SUPPORT','POLYGON_FEATURES','RAW_ALLOCATOR'):env['HALO2_REFERENCE_'+name]='1'
            probe=subprocess.run([str(binary)],capture_output=True,text=True,env=env)
            if probe.returncode:raise ValueError('Face-select source routing failed '+mode+':\n'+probe.stderr[-7000:])
            report['routes'][mode]=json.loads(probe.stderr.strip().splitlines()[-1])
        probe=subprocess.run([str(binary),'--refusals'],capture_output=True,text=True,env=environment)
        if probe.returncode:raise ValueError('Face-select live admission failed:\n'+probe.stderr[-7000:])
        report['refusals']=json.loads(probe.stderr.strip().splitlines()[-1]);report['source_sha256']=pins;report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
        if not report['sources_unchanged']:raise ValueError('Face-select routing input drift')
        report['production_bound']=args.require_binding;report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
