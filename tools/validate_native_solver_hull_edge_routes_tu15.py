#!/usr/bin/env python3
"""Private current/fresh raw/prepared313470 source continuations, genuine children."""
import argparse,difflib,hashlib,json,os,platform,re,subprocess,traceback
from pathlib import Path
import native_solver_hull_edge_replacements as binding
from specialize_native_operands import specialize
from validate_native_solver_hull_edge_frame_tu15 import ROOT,inputs
from generated_routine_ownership import function_bodies
import native_solver_hull_support_pair_replacements as pair_binding
import native_polygon_support_replacements as polygon_binding

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def ordered(body):
    for opcode,count,name in [('sub_ps',4,'sub'),('mul_ps',2,'mul'),('add_ss',4,'add_scalar')]:
        body,n=re.subn(r'_mm_'+opcode+r'\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda m:name+'('+m[1]+','+m[2]+')',body)
        if n!=count:raise ValueError('Complete retained hull-edge SIMD seam changed '+opcode)
    return body

def prepare(scratch,discover=False,require_binding=False):
    current=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();fresh=scratch/'fresh-primary.cpp'
    command=[os.sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh-primary.json')]
    run=subprocess.run(command,capture_output=True,text=True)
    if run.returncode:raise ValueError('Genuine full Primary generation failed: '+run.stderr[-7000:])
    fresh_text=fresh.read_text()
    for label,text in [('current',current),('fresh',fresh_text)]:
        match,resolver,dispatcher=binding.context(text)
        canonical=binding.canonical_body(match[0])
        if binding.digest(canonical)not in(binding.BODY_PIN,binding.FRESH_BODY_PIN):
            original=binding.canonical_body(binding.context(current)[0][0])
            diff=''.join(difflib.unified_diff(original.splitlines(True),canonical.splitlines(True),fromfile='current',tofile=label))
            raise ValueError('Exact '+label+' hull-edge body '+binding.digest(canonical)+' differs: '+diff)
        if require_binding and binding.replace_solver_hull_edge(text)!=text:raise ValueError('Exact '+label+' hull-edge source default is not installed')
    raw=[binding.replace_solver_hull_edge(text)for text in(current,fresh_text)]
    for value in raw:
        rebound=binding.replace_solver_hull_edge(binding.remove_solver_hull_edge(value))
        if value.count(binding.INCLUDE)!=1 or rebound.count(binding.INCLUDE)!=1:raise ValueError('Hull-edge exact include ownership drift')
        at=value.index(binding.INCLUDE);without=rebound.replace(binding.INCLUDE,'',1);rebound=without[:at]+binding.INCLUDE+without[at:]
        if binding.replace_solver_hull_edge(value)!=value or rebound!=value:raise ValueError('Hull-edge whole-source reversal/idempotence drift')
    variants=raw+[binding.rebind_prepared(specialize(text)[0])for text in raw];contexts=[]
    for text in variants:
        match,resolver,dispatcher=binding.context(text);contexts.append(tuple(binding.digest(v)for v in(match[0],resolver,dispatcher)))
    report=dict(contexts=contexts,genuine_fresh_command=command,raw_body_pin=binding.BODY_PIN,explicit_source_format_tags=True,source_math_reference='Only10 authored SUBPS/MULPS/ADDSS operations use the ordered opcode projection independently qualified against unchanged i386. Exact raw callback-present fallback uses the unchanged retained body; callback-installation feedback uses finite ordinary-backed rows.')
    if discover:return report
    blocks=[];retained=[];phased=[];controls=[];relocations=[]
    for n,text in enumerate(variants):
        if binding.installed_entries(text)!=['313470']:raise ValueError('Complete313470 context is not authenticated')
        match,resolver,dispatcher=binding.context(text);body=match[0];opening=body.index('{')+1
        active_prefix=binding.RAW_PREFIX if n<2 else binding.PREPARED_PREFIX
        if not body[opening:].startswith(active_prefix):raise ValueError('Candidate prefix changed')
        body=body[:opening+len(active_prefix)]+f'\n++literals[{n}];'+body[opening+len(active_prefix):]
        blocks.append(body.replace('run_313470(',f'bound_{n}(',1));original=binding.canonical_body(match[0]);retained.append(original.replace('run_313470(',f'retained_{n}(',1));phased.append(ordered(original).replace('run_313470(',f'ordered_{n}(',1))
        failures={}
        for label,value in {'body':text[:match.start()]+match[0].replace('0x54u','0x50u',1)+text[match.end():],'include':text.replace(binding.INCLUDE,'',1),'prefix':text.replace(active_prefix,'',1),'wrong-format':text.replace(active_prefix,binding.PREPARED_PREFIX if n<2 else binding.RAW_PREFIX,1),'missing-body':text[:match.start()]+text[match.end():],'resolver':text.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),'dispatcher':text.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)}.items():
            if value==text:raise ValueError('Context defect seam missing '+label)
            try:binding.installed_entries(value)
            except ValueError as error:failures[label]=str(error)
            else:raise ValueError('Changed complete313470 context accepted '+label)
        controls.append(failures);relocated={};first=next(line for line in match[0].splitlines()if '0x313470u'in line)
        for pc,label in binding.LABELS.items():
            changed=match[0].replace(label,'',1).replace(first,label+first,1);value=text[:match.start()]+changed+text[match.end():]
            for method in(binding.authenticate,binding.installed_entries):
                try:method(value)
                except ValueError as error:relocated[hex(pc)+':'+method.__name__]=str(error)
                else:raise ValueError('Relocated hull-edge source frontier accepted '+hex(pc))
        relocations.append(relocated)
    primitive='static constexpr std::array<std::uint32_t,43>frontiers{'+','.join(hex(pc)+'u'for pc,_ in binding.FRONTIERS)+'};\n'+\
"""static __m128 sub(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
static std::array<unsigned,4>literals{};
"""
    wrapper='\nstatic void run(WorldState&s,unsigned variant,bool reference,bool ordered_reference){switch(variant){'+''.join(f'case {n}:if(reference){{if(ordered_reference)ordered_{n}(s);else retained_{n}(s);}}else bound_{n}(s);return;'for n in range(4))+'default:throw std::runtime_error("Unknown hull-edge context");}}\n'
    (scratch/'solver_hull_edge_routes.inc').write_text(primitive+'\n'.join(blocks+retained+phased)+wrapper)
    # Compile the actual installed local Pair/Zero/Box source bodies, keeping
    # their existing native admission and exact fallback dispatch in the probe.
    children=[];child_pins=[]
    for n,text in enumerate(variants):
        selected={sig.symbol:body for sig,body in function_bodies(text)if sig.symbol in('313390','2e8d20','306790')}
        if len(selected)!=3:raise ValueError('Incomplete actual hull-edge child source set')
        if pair_binding.installed_entries(text)!=['313390']:raise ValueError('Actual support-pair child context drift')
        child_pins.append({name:binding.digest(body)for name,body in selected.items()})
        for name,label in [('2e8d20','zero'),('306790','box'),('313390','pair')]:children.append(selected[name].replace('run_'+name+'(',label+'_'+str(n)+'(',1))
    world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text();match,poly=polygon_binding.authenticate(world)
    if match[1]!='ObservedWorldState':raise ValueError('Wrong actual polygon child owner')
    opening=match[0].index('{')+1
    if not match[0].startswith(polygon_binding.PREFIX,opening):raise ValueError('Missing actual polygon default owner')
    installed_tail=match[0][:opening]+match[0][opening+len(polygon_binding.PREFIX):]
    if installed_tail!=polygon_binding.ordered_body(poly):raise ValueError('Actual polygon retained child tail drift')
    # Keep its actual ordered-store helper and16-byte child event. The
    # canonical extractor's8+8 spelling is a different retained protocol.
    poly=installed_tail
    poly=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',lambda m:m[1],poly)
    if poly.count('selected(0x447020u,s.image)')!=1 or 'world_observation_pc('in poly:raise ValueError('Incomplete actual polygon source operand mapping')
    poly=poly.replace('selected(0x447020u,s.image)','0x44aac8u').replace('ObservedWorldState','WorldState').replace('run_1f1d20(','polygon_source(',1)
    children.append(poly)
    for label in('zero','box','pair'):
        children.append('static void '+label+'(WorldState&s,unsigned variant){switch(variant){'+''.join('case '+str(n)+':'+label+'_'+str(n)+'(s);return;'for n in range(4))+'default:throw std::runtime_error("Unknown genuine child source context");}}')
    (scratch/'solver_hull_edge_children.inc').write_text('\n'.join(children))
    report.update(actual_child_body_pins=child_pins,polygon_actual_source_body_pin=polygon_binding.BODY_PINS[match[1]],strict_context_controls=controls,strict_relocated_frontiers=relocations)

    return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--discover-contexts',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--modes',nargs='+',choices=('baseline','frontiers','feedback','observer-objects','provider-controls','refusals','own-reference','raw-free','raw-install','raw-publication','source-publication'));a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    names=['src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','tests/native_tu15_solver_hull_edge_routes_tests.cpp','tools/native_solver_hull_edge_replacements.py','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tools/native_solver_hull_edge_source_mutants.py','tools/validate_native_solver_hull_edge_frame_tu15.py','tools/generated_routine_ownership.py','tools/native_solver_hull_support_pair_replacements.py','tools/native_polygon_support_replacements.py','src/physics/runtime/physics_world_runtime_generated.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(names);seen=set(names)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header,ROOT/'tests'/header)if v.is_file()),None)
            if target is not None:
                relative=str(target.relative_to(ROOT))
                if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report['source_sha256']=pins
    try:
        from research_mp_sandbox_update_functions import Xbe
        from validate_native_tu15_prt_helpers import PIN
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original image')
        report['preparation']=prepare(scratch,a.discover_contexts,a.require_binding)
        if a.discover_contexts:report.update(passed=True,validation_not_run=True)
        else:
            rows=inputs(Xbe(a.xbe));path=scratch/'inputs.json';path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(path)
            command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_edge_routes_tests.cpp'),'-o',str(scratch/'routes')]
            if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Hull-edge source candidate compile failed: '+built.stderr[-12000:])
            report['routes']={};env=os.environ.copy();env.pop('HALO2_REFERENCE_SOLVER_HULL_EDGE',None)
            for mode in a.modes or ['baseline','frontiers','feedback','observer-objects','provider-controls','refusals','own-reference','raw-free','raw-install','raw-publication','source-publication']:
                environment=env.copy()
                if mode=='own-reference':environment['HALO2_REFERENCE_SOLVER_HULL_EDGE']='1'
                with path.open('rb')as inp:run=subprocess.run([str(scratch/'routes'),mode],stdin=inp,capture_output=True,text=True,env=environment)
                if run.returncode:raise ValueError('Hull-edge source candidate failed '+mode+': '+run.stderr[-12000:])
                report['routes'][mode]=json.loads(run.stdout.strip().splitlines()[-1])
                # A timeout must retain completed-mode diagnostics without
                # advertising an incomplete registered gate as passed.
                a.report.parent.mkdir(parents=True,exist_ok=True)
                pending=a.report.with_suffix('.pending');pending.write_text(json.dumps(report,indent=2)+'\n');pending.replace(a.report)
                print(json.dumps(dict(completed_mode=mode,passed=False,validation_incomplete=True)),flush=True)
            if a.mutations:
                from native_solver_hull_edge_source_mutants import run_mutants
                report['compiled_source_defects']=run_mutants(ROOT,scratch,command,path,env)
            report.update(passed=True,production_bound=a.require_binding,scope='Current/fresh/raw/prepared source routes, genuine computed pair+shape children. Whole-instruction policy frontier and retained provider publication only; further arbitrary feedback/child/fault domains remain open.')
    except Exception as error:report.update(error=repr(error),failure_traceback=traceback.format_exc()[-12000:])
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,source_stability_error='Private hull-edge source dependency drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
