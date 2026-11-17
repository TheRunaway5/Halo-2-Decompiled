#!/usr/bin/env python3
"""Private strict30DC10 source routes and provider-preserving native children."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from native_simplex_provider_policy_probe import check_guard
import native_simplex_boundary_replacements as binding
import native_simplex_triangle_scene_replacements as triangle_binding
import native_closest_segments_replacements as closest_binding
from specialize_native_operands import specialize
from validate_native_simplex_boundary_tu15 import ROOT,PINS,reference
from validate_native_simplex_direction_tu15 import sha
def prepare(scratch,require_binding=False):
    reference(scratch);sources=[(ROOT/name).read_text()for name in PINS];fresh=scratch/'fresh-primary.cpp';command=[os.sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh-primary.json')];run=subprocess.run(command,capture_output=True,text=True)
    if run.returncode:raise ValueError('Genuine freshPrimary generation failed: '+run.stderr[-5000:])
    sources.append(fresh.read_text());bound=[]
    for n,source in enumerate(sources):
        value=binding.replace_simplex_boundary(source)
        if require_binding and n<2 and value!=source:raise ValueError('Complete simplex-boundary production overlay not installed')
        unbound=binding.remove_simplex_boundary(value);rebound=binding.replace_simplex_boundary(unbound);at=value.index(binding.INCLUDE);without=rebound.replace(binding.INCLUDE,'',1);restored=without[:at]+binding.INCLUDE+without[at:]
        if binding.replace_simplex_boundary(value)!=value or binding.remove_simplex_boundary(rebound)!=unbound or restored!=value:raise ValueError('Boundary overlay reversibility/idempotency drift')
        bound.append(value)
    variants=bound+[specialize(value)[0]for value in bound];blocks=[];retained=[];receipts=[]
    for n,value in enumerate(variants):
        if binding.installed_entries(value)!=['30dc10']:raise ValueError('Wrong complete boundary receipt')
        match,resolver,dispatcher=binding.context(value);body=match[0];at=body.index('{')+1+len(binding.PREFIX);body=body[:at]+f'\n++literal_entries[{n}];'+body[at:];body=body.replace('run_30dc10(',f'bound_{n}(',1)
        changed={'body':value[:match.start()]+match[0].replace('0x74u','0x78u',1)+value[match.end():],'include':value.replace(binding.INCLUDE,'',1),'prefix':value.replace(binding.PREFIX,'',1),'missing_body':value[:match.start()]+value[match.end():],'resolver':value.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)};controls={}
        for label,mutated in changed.items():
            if mutated==value:raise ValueError('Boundary strict mutation seam drift: '+label)
            try:binding.installed_entries(mutated)
            except ValueError as error:controls[label]=str(error)
            else:raise ValueError('Complete boundary context mutation accepted: '+label)
        if specialize(value)[0]!=(variants[n+3]if n<3 else value):raise ValueError('Boundary repeated preparation drift')
        scene=n%3==1
        if dispatcher.count('run_30d3f0(s)')!=(1 if scene else 0)or dispatcher.count('run_312620(s)')!=(1 if scene else 0):raise ValueError('Boundary exact local child ownership drift')
        blocks.append(body);retained.append(body.replace(binding.PREFIX,'',1).replace(f'\n++literal_entries[{n}];','',1).replace(f'bound_{n}(',f'retained_{n}(',1));receipts.append(dict(variant=n,scene_local_children=scene,complete_input_sha256=binding.digest(value),bound_body_sha256=binding.digest(match[0]),strict_context_mutants=controls))
    scene=sources[1];triangle_binding.installed_entries(scene);closest_binding.installed_entries(scene);triangle=triangle_binding.BODY.search(scene)[0];closest=closest_binding.BODY.search(scene)[0]
    triangle=triangle.replace('run_30d3f0(','known_triangle(',1);at=triangle.index('{')+1+len(triangle_binding.PREFIX);triangle=triangle[:at]+'\n++child_literals;'+triangle[at:]
    closest=closest.replace('run_312620(','known_closest(',1);at=closest.index('{')+1+len(closest_binding.ACTUAL_PREFIX);closest=closest[:at]+'\n++child_literals;'+closest[at:]
    spill=re.search(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',scene,re.S)[0]
    if binding.digest(spill)!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Actual Scene child spill owner drift')
    prefix='''static unsigned context{},child_literals{};static std::array<unsigned,6>literal_entries{};
using reference::sign_extend;
static std::uint32_t selected(std::uint32_t value,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Wrong boundary selected image");return value;}
'''+spill+'\n'+triangle+'\n'+closest+'''
static void known_child(WorldState&s,std::uint32_t target){if(target==0x30d3f0u)known_triangle(s);else if(target==0x312620u)known_closest(s);else throw std::runtime_error("Unknown boundary child");}
static void run_30d3f0(WorldState&s){known_triangle(s);}
static void run_312620(WorldState&s){known_closest(s);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(tail||!((target==0x30d3f0u&&(site==0x30dc3cu||site==0x30dc71u))||(target==0x312620u&&site==0x30dd31u)))throw std::runtime_error("Boundary exact dispatch target/site/tail changed");if(context%3u==1u)known_child(s,target);else s.required(site,target,tail);}
'''
    suffix='static void run(WorldState&s,unsigned variant,bool retained=false){switch(variant){'+''.join(f'case {n}:if(retained)retained_{n}(s);else bound_{n}(s);return;'for n in range(6))+'default:throw std::runtime_error("Wrong boundary source variant");}}\n'
    text=prefix+'\n'.join(blocks+retained)+suffix;(scratch/'simplex_boundary_routes.inc').write_text(text)
    return dict(source_variants=receipts,source_copy_sha256=binding.digest(text),genuine_fresh_primary_command=command,child_source='Full Scene actual-owner bodies authenticated through their strict original binders',dispatcher_projection='Whole dispatcher pins and exact local-membership checks authorize only the two child targets. Scene retains its two local branches; Primary/fresh retain exact required(site,target,tail). No other native fallback owner is exercised or claimed.',native_composition='Provider-free and exact named readonly-provider parents are admitted. Present providers retain exact child dispatch. Uncertified/mismatched/wrapped/mutating providers take whole-entry retained fallback; actual Primary external-child paths compute genuine child results before mutation. Scene static children do not invoke the provider. Child reference/admission refusal retains dispatch.',no_supplied_child_outcomes=True)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'tools/native_simplex_provider_policy_probe.py','src/physics/runtime/native_tu15_simplex_boundary_source_owner.hpp','tools/native_simplex_boundary_replacements.py','tools/native_simplex_triangle_scene_replacements.py','tools/native_closest_segments_replacements.py','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tools/validate_native_simplex_boundary_tu15.py','tests/native_tu15_simplex_boundary_routes_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((value for value in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if value.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report['source_sha256']=pins
    try:
        report['preparation']=prepare(scratch,args.require_binding);base=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch)]
        if args.sanitizers:base[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        core=scratch/'core';routes=scratch/'routes';commands=[base+[str(ROOT/'tests/native_tu15_simplex_boundary_tests.cpp'),'-o',str(core)],base+[str(ROOT/'tests/native_tu15_simplex_boundary_routes_tests.cpp'),'-o',str(routes)]];report['commands']=commands
        for command in commands:
            result=subprocess.run(command,capture_output=True,text=True)
            if result.returncode:raise ValueError('Boundary source route compile failed:\n'+result.stderr[-12000:])
        inputs=scratch/'inputs.bin'
        with inputs.open('wb')as out:dump=subprocess.run([str(core),'--dump-inputs'],stdout=out,stderr=subprocess.PIPE)
        if dump.returncode:raise ValueError('Boundary genuine input export failed: '+dump.stderr.decode()[-5000:])
        environment=os.environ.copy()
        for name in('SIMPLEX_BOUNDARY','SIMPLEX_TRIANGLE','CLOSEST_SEGMENTS'):environment.pop('HALO2_REFERENCE_'+name,None)
        report['routes']={}
        for mode,provider in[('default-direct',0),('default-provider',1),('parent-reference',1),('triangle-reference',1),('closest-reference',1),('unrecognized-provider-stop',2),('uncertified-computing-provider',3),('wrapped-provider',4),('mismatched-provider-owner',5),('mutating-trace-provider',6),('rebound-instruction-provider',7),('immutable-provider-copy',8),('triangle-reference-direct',0),('closest-reference-direct',0),('unrelated-controls',0)]:
            env=environment.copy()
            if mode=='parent-reference':env['HALO2_REFERENCE_SIMPLEX_BOUNDARY']='1'
            if mode.startswith('triangle-reference'):env['HALO2_REFERENCE_SIMPLEX_TRIANGLE']='1'
            if mode.startswith('closest-reference'):env['HALO2_REFERENCE_CLOSEST_SEGMENTS']='1'
            if mode=='unrelated-controls':
                for name in('SIMPLEX_PAIRED','MATRIX_LEAVES','RAW_ALLOCATOR','DATUM_DELETE'):env['HALO2_REFERENCE_'+name]='1'
            with inputs.open('rb')as inp:probe=subprocess.run([str(routes),str(provider)],stdin=inp,capture_output=True,text=True,env=env)
            if probe.returncode:raise ValueError('Boundary source route failed '+mode+':\n'+probe.stderr[-9000:])
            report['routes'][mode]=json.loads(probe.stderr.strip().splitlines()[-1])
        probe=subprocess.run([str(routes),'--refusals'],capture_output=True,text=True,env=environment)
        if probe.returncode:raise ValueError('Boundary admission/refusal failed:\n'+probe.stderr[-5000:])
        report['provider_guard_mutant']=check_guard(ROOT,scratch,base,'boundary')
        report['refusals']=json.loads(probe.stderr.strip().splitlines()[-1]);report['production_bound']=args.require_binding;report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Boundary route source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
