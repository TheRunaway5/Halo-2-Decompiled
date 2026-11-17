#!/usr/bin/env python3
"""Private exact30D830 source overlays/admission and genuine native child routes."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from native_simplex_provider_policy_probe import check_guard
import native_simplex_paired_replacements as binding
import native_simplex_triangle_replacements as triangle_binding
import native_closest_segments_replacements as closest_binding
from specialize_native_operands import specialize
from fix_physics_world_observation_tu15 import observation_baseline
from validate_native_simplex_paired_tu15 import ROOT,PINS,reference
from validate_native_simplex_direction_tu15 import sha
def prepare(scratch,require_binding=False):
    reference(scratch);sources=[(ROOT/name).read_text()for name in PINS];sources.append(observation_baseline(sources[0])[0]);bound=[]
    for n,source in enumerate(sources):
        value=binding.replace_simplex_paired(source)
        if require_binding and n<2 and value!=source:raise ValueError('Complete simplex-paired production overlay is not installed')
        unbound=binding.remove_simplex_paired(value);rebound=binding.replace_simplex_paired(unbound);include_at=value.index(binding.INCLUDE);without_include=rebound.replace(binding.INCLUDE,'',1);restored=without_include[:include_at]+binding.INCLUDE+without_include[include_at:]
        if binding.replace_simplex_paired(value)!=value or binding.remove_simplex_paired(rebound)!=unbound or restored!=value:raise ValueError('Private paired overlay is not reversible/idempotent')
        bound.append(value)
    variants=bound+[specialize(v)[0]for v in bound];blocks=[];retained_blocks=[];receipts=[]
    for n,value in enumerate(variants):
        if binding.installed_entries(value)!=['30d830']:raise ValueError('Wrong paired source receipt')
        match,resolver,dispatcher=binding.context(value);body=match[0];at=body.index('{')+1+len(binding.PREFIX);body=body[:at]+f'\n++literal_entries[{n}];'+body[at:];body=body.replace('run_309d40(',f'bound_{n}(',1).replace('ObservedWorldState &s','ActualPairedState &s')
        if match[1]=='State':body=body.replace('State &s','WorldState &s',1)
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body).replace('run_309900(s);','triangle_bound(s);').replace('run_30eb30(s);','closest_bound(s);')
        changes={'body':value[:match.start()]+match[0].replace('0xa4u','0xa8u',1)+value[match.end():],'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(binding.PREFIX,'',1),'missing_body':value[:match.start()]+value[match.end():],'resolver':value.replace(resolver,resolver.replace('0x41c500u','0x41c510u',1),1),'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)};controls={}
        for control,changed in changes.items():
            if changed==value:raise ValueError('Paired strict context mutation seam drift: '+control)
            try:binding.installed_entries(changed)
            except ValueError as error:controls[control]=str(error)
            else:raise ValueError('Complete paired context drift accepted: '+control)
        if specialize(value)[0]!=(variants[n+3]if n<3 else value):raise ValueError('Paired repeated preparation drift')
        blocks.append(body);retained_blocks.append(body.replace(binding.PREFIX,'',1).replace(f'\n++literal_entries[{n}];','',1).replace(f'bound_{n}(',f'retained_{n}(',1));receipts.append(dict(variant=n,state=match[1],complete_input_sha256=binding.digest(value),body_sha256=binding.digest(match[0]),strict_context_controls=controls))
    world=sources[0];triangle_binding.installed_entries(world);closest_binding.installed_entries(world)
    triangle=triangle_binding.BODY.search(world)[0];closest=closest_binding.BODY.search(world)[0];child_receipts=[]
    def child_body(body,symbol,name,prefix):
        original=body;at=body.index('{')+1;body=body[:at]+f'\nchild_packets.emplace_back(0x{symbol:x}u,s.save());'+body[at:];at=body.index(prefix)+len(prefix);body=body[:at]+'\n++child_literals;'+body[at:];body=body.replace('run_'+('309900'if symbol==0x30d3f0 else'30eb30')+'(',name+'(',1).replace('ObservedWorldState &s','SolverState &s',1);body=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),(0x[0-9a-f]+u),s\)',r'observation(s,\1,\2)',body);child_receipts.append(dict(actual_entry=f'{symbol:x}',original_bound_body_sha256=binding.digest(original)));return 'template<class SolverState>'+body
    children=[child_body(triangle,0x30d3f0,'triangle_bound',triangle_binding.PREFIX),child_body(closest,0x312620,'closest_bound',closest_binding.PREFIX)]
    resolver=binding.context(world)[1];mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)};operands={int(v,16)for text in blocks+children for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',text)}
    selected='static std::uint32_t selected(std::uint32_t value,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Wrong paired route image");switch(value){'+''.join(f'case 0x{v:x}u:return 0x{mapping[v]:x}u;'for v in sorted(operands))+'default:throw std::runtime_error("Wrong paired route selected operand");}}\n'
    prefix='''static std::array<unsigned,6>literal_entries{};static unsigned child_literals{},site_witnesses{};static std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>child_packets;
template<class State>static std::uint32_t observation(const State&,std::uint32_t canonical,std::uint32_t actual){if constexpr(requires(const State&s){s.selected_observation_pcs;})return actual;else return canonical;}
'''+selected+'\n'.join(children)+'''
template<class State>static void known_child(State&s,std::uint32_t target){if(target==0x30d3f0u)triangle_bound(s);else if(target==0x312620u)closest_bound(s);else throw std::runtime_error("Wrong genuine paired child");}
template<class State>static void dispatch(State&s,std::uint32_t target,std::uint32_t site,bool tail){const bool triangle=target==0x30d3f0u&&(site==0x309e35u||site==0x309e95u||site==0x309fe9u||site==0x309ffbu);const bool closest=target==0x312620u&&(site==0x309ecfu||site==0x309ee8u);if(tail||(!triangle&&!closest))throw std::runtime_error("Wrong paired internal callsite");const auto observed=observation(s,site,site+0x3af0u);if(!s.instruction_cursor||!s.instruction_cursor->position||*s.instruction_cursor->position!=observed)throw std::runtime_error("Paired actual/canonical observation and internal callsite conflated");++site_witnesses;known_child(s,target);}
'''
    suffix='''static void run_actual(ActualPairedState&s,unsigned variant,bool retained=false){if(variant==0u){if(retained)retained_0(s);else bound_0(s);}else if(variant==3u){if(retained)retained_3(s);else bound_3(s);}else throw std::runtime_error("Wrong actual paired variant");}
static void run_canonical(WorldState&s,unsigned variant,bool retained=false){switch(variant){case 1:if(retained)retained_1(s);else bound_1(s);return;case 2:if(retained)retained_2(s);else bound_2(s);return;case 4:if(retained)retained_4(s);else bound_4(s);return;case 5:if(retained)retained_5(s);else bound_5(s);return;default:throw std::runtime_error("Wrong canonical paired variant");}}
'''
    text=prefix+'\n'.join(blocks+retained_blocks)+suffix;(scratch/'simplex_paired_routes.inc').write_text(text)
    return dict(source_variants=receipts,qualified_child_sources=child_receipts,source_copy_sha256=hashlib.sha256(text.encode()).hexdigest(),source_contexts_authenticated=True,cached_intersection_handoff='The literal fallback retains the real actual30DAD9 handoff. Its admitted native/reference deferral reaches the genuine whole Triangle owner. Native paired dispatch uses the six explicit local canonical internal sites, separately witnessed against actual/canonical Before/PUSH cursors; no external required-owner site is inferred',test_instrumentation='Nonmutating entry counters/full callee ABI snapshots and cursor witnesses only; no original i386 patch or child outcomes',provider_scope='Both children are genuinely local in all six source copies; required_provider_calls must be zero. Named readonly provider certification and immutable copies admit the parent; uncertified, wrapped, mismatched and mutating-provider presence require exact whole-entry fallback. Actual post-child provider mutations are separately exercised through Boundary Primary external-owner routes.')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'tools/native_simplex_provider_policy_probe.py','tools/native_simplex_paired_replacements.py','tools/native_simplex_triangle_replacements.py','tools/native_closest_segments_replacements.py','tools/specialize_native_operands.py','tools/fix_physics_world_observation_tu15.py','tools/validate_native_simplex_paired_tu15.py','tests/native_tu15_simplex_paired_routes_tests.cpp','src/physics/runtime/native_tu15_simplex_paired_source_owner.hpp','src/physics/runtime/native_tu15_simplex_triangle_source_owner.hpp','src/physics/runtime/native_tu15_closest_segments_source_owner.hpp','src/physics/native_tu15_simplex_helpers_handoff.cpp','src/physics/native_tu15_simplex_triangle.cpp','src/physics/native_tu15_closest_segments.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)};report['source_sha256']=pins
    try:
        report['preparation']=prepare(scratch,args.require_binding);base=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch)]
        if args.sanitizers:base[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        core=scratch/'core';routes=scratch/'routes';commands=[base+[str(ROOT/'tests/native_tu15_simplex_paired_tests.cpp'),'-o',str(core)],base+[str(ROOT/'tests/native_tu15_simplex_paired_routes_tests.cpp')]+[str(ROOT/'src/physics'/n)for n in('native_tu15_simplex_helpers_handoff.cpp','native_tu15_simplex_triangle.cpp','native_tu15_closest_segments.cpp')]+['-o',str(routes)]];report['commands']=commands
        for command in commands:
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Paired source route compile failed:\n'+built.stderr[-12000:])
        inputs=scratch/'inputs.bin'
        with inputs.open('wb')as out:dump=subprocess.run([str(core),'--dump-inputs'],stdout=out,stderr=subprocess.PIPE)
        if dump.returncode:raise ValueError('Paired declared input export failed: '+dump.stderr.decode()[-5000:])
        environment=os.environ.copy()
        for name in('SIMPLEX_PAIRED','SIMPLEX_TRIANGLE','CLOSEST_SEGMENTS'):environment.pop('HALO2_REFERENCE_'+name,None)
        report['routes']={}
        for mode,provider in [('default',0),('certified-provider',1),('uncertified-provider',2),('wrapped-provider',3),('mismatched-provider-owner',4),('mutating-provider-presence',5),('immutable-provider-copy',6),('parent-reference',0),('triangle-reference',0),('closest-reference',0),('unrelated-controls',0)]:
            env=environment.copy()
            if mode=='parent-reference':env['HALO2_REFERENCE_SIMPLEX_PAIRED']='1'
            if mode=='triangle-reference':env['HALO2_REFERENCE_SIMPLEX_TRIANGLE']='1'
            if mode=='closest-reference':env['HALO2_REFERENCE_CLOSEST_SEGMENTS']='1'
            if mode=='unrelated-controls':
                for name in('MATRIX_LEAVES','SIMPLEX_FACE_SELECT','POLYGON_SUPPORT','POLYGON_FEATURES','RAW_ALLOCATOR'):env['HALO2_REFERENCE_'+name]='1'
            with inputs.open('rb')as inp:probe=subprocess.run([str(routes),str(provider)],stdin=inp,capture_output=True,text=True,env=env)
            if probe.returncode:raise ValueError('Paired source route failed '+mode+':\n'+probe.stderr[-8000:])
            report['routes'][mode]=json.loads(probe.stderr.strip().splitlines()[-1])
        refusal=subprocess.run([str(routes),'--refusals'],capture_output=True,text=True,env=environment)
        if refusal.returncode:raise ValueError('Paired live admission failed:\n'+refusal.stderr[-5000:])
        report['provider_guard_mutant']=check_guard(ROOT,scratch,base,'paired')
        report['refusals']=json.loads(refusal.stderr.strip().splitlines()[-1]);report['production_bound']=args.require_binding;report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Paired route input drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
