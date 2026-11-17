#!/usr/bin/env python3
"""Private full313390 source candidates, genuine children and retained continuations."""
import argparse
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
from pathlib import Path

import native_solver_hull_support_pair_replacements as binding
import native_polygon_support_replacements as polygon
from specialize_native_operands import specialize
from validate_native_solver_hull_support_pair_frame_tu15 import ROOT,inputs
from native_solver_hull_support_pair_zero_inputs import mixed_inputs
from native_solver_hull_support_pair_shape_inputs import box_inputs
from validate_native_zero_shape_support_tu15 import body as zero_body
from validate_native_box_shape_support_tu15 import body as box_body

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def ordered(body):
    for name,count in (('mul',6),('add',5)):
        body,n=re.subn(r'_mm_'+name+r'_ps\((s.xmm\[[0-7]\]),(s.xmm\[[0-7]\])\)',lambda m:name+'('+m[1]+','+m[2]+')',body)
        if n!=count:raise ValueError('Complete retained SIMD seam changed '+name)
    return body

def prepare(scratch,discover=False,require_binding=False):
    primary=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
    fresh=scratch/'fresh-primary.cpp'
    command=[os.sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'fresh-primary.json')]
    run=subprocess.run(command,capture_output=True,text=True)
    if run.returncode:raise ValueError('Genuine full Primary generation failed: '+run.stderr[-7000:])
    raw=[]
    for source in (primary,fresh.read_text()):
        value=binding.replace_solver_hull_support_pair(source)
        if require_binding and value!=source:raise ValueError('Complete313390 current/fresh source default is not installed')
        unbound=binding.remove_solver_hull_support_pair(value);rebound=binding.replace_solver_hull_support_pair(unbound)
        if value.count(binding.INCLUDE)!=1 or rebound.count(binding.INCLUDE)!=1:raise ValueError('Support-pair exact include reversal changed ownership')
        include_at=value.index(binding.INCLUDE);without_include=rebound.replace(binding.INCLUDE,'',1)
        rebound=without_include[:include_at]+binding.INCLUDE+without_include[include_at:]
        if binding.replace_solver_hull_support_pair(value)!=value or rebound!=value:raise ValueError('Complete313390 overlay reversal/idempotence changed')
        raw.append(value)
    variants=raw+[specialize(value)[0]for value in raw]
    contexts=[]
    for source in variants:
        match,resolver,dispatcher=binding.context(source)
        contexts.append(tuple(binding.digest(v)for v in(match[0],resolver,dispatcher)))
    # Whole tuples below are discovery evidence only, never runtime admission.
    report=dict(contexts=contexts,genuine_fresh_command=command,raw_body_pin=binding.BODY_PIN)
    if discover:return report
    blocks=[];retained=[];phased=[];controls=[];relocations=[]
    for n,source in enumerate(variants):
        if binding.installed_entries(source)!=['313390']:raise ValueError('Complete313390 candidate is not authenticated')
        match,resolver,dispatcher=binding.context(source)
        body=match[0];opening=body.index('{')+1
        if not body[opening:].startswith(binding.PREFIX):raise ValueError('Candidate prefix position changed')
        body=body[:opening+len(binding.PREFIX)]+f'\n++literals[{n}];'+body[opening+len(binding.PREFIX):]
        blocks.append(body.replace('run_313390(',f'bound_{n}(',1))
        original=binding.canonical_body(match[0])
        retained.append(original.replace('run_313390(',f'retained_{n}(',1))
        phased.append(ordered(original).replace('run_313390(',f'ordered_{n}(',1))
        failures={}
        mutations={'body':source[:match.start()]+match[0].replace('0x40u','0x3cu',1)+source[match.end():],
                   'include':source.replace(binding.INCLUDE,'',1),
                   'prefix':source.replace(binding.PREFIX,'',1),
                   'missing-body':source[:match.start()]+source[match.end():],
                   'resolver':source.replace(resolver,resolver.replace('switch(p)','p^=1u;switch(p)',1),1),
                   'dispatcher':source.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1),
                   'continuation':source.replace(binding.LABELS[0x3133ae],'',1)}
        for label,value in mutations.items():
            if value==source:raise ValueError('Context mutation seam missing '+label)
            try:binding.installed_entries(value)
            except ValueError as error:failures[label]=str(error)
            else:raise ValueError('Changed complete313390 context was accepted '+label)
        controls.append(failures)
        first=next(line for line in match[0].splitlines()if '0x313390u'in line)
        relocated={}
        for pc,label in binding.LABELS.items():
            changed=match[0].replace(label,'',1).replace(first,first[:len(first)-len(first.lstrip())]+label+first.lstrip(),1)
            if changed==match[0]:raise ValueError('Continuation relocation seam missing '+hex(pc))
            mutation=source[:match.start()]+changed+source[match.end():]
            for method in(binding.authenticate,binding.installed_entries):
                try:method(mutation)
                except ValueError as error:relocated[hex(pc)+':'+method.__name__]=str(error)
                else:raise ValueError('Relocated complete support-pair continuation was accepted '+hex(pc))
        relocations.append(relocated)
    primitive='static constexpr std::array<std::uint32_t,32>frontiers{'+','.join(hex(pc)+'u'for pc,_ in binding.FRONTIERS)+'};\n'+'''static __m128 mul(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static __m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
static std::array<unsigned,4>literals{};
'''
    wrapper='\nstatic void run(WorldState&s,unsigned variant,bool reference,bool ordered_reference){switch(variant){'+''.join(f'case {n}:if(reference){{if(ordered_reference)ordered_{n}(s);else retained_{n}(s);}}else bound_{n}(s);return;'for n in range(4))+'default:throw std::runtime_error("Unknown support-pair context");}}\n'
    text=primitive+'\n'.join(blocks+retained+phased)+wrapper
    (scratch/'solver_hull_support_pair_routes.inc').write_text(text)
    world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
    match,poly=polygon.authenticate(world)
    if match[1]!='ObservedWorldState':raise ValueError('Wrong actual polygon World owner namespace')
    selected=list(re.finditer(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)',poly))
    if len(selected)!=poly.count('world_observation_pc('):raise ValueError('Incomplete polygon observation map')
    poly=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',lambda m:m[1],polygon.ordered_body(poly))
    if poly.count('selected(0x447020u,s.image)')!=1:raise ValueError('Polygon constant mapping changed')
    poly=poly.replace('selected(0x447020u,s.image)','0x44aac8u').replace('ObservedWorldState','WorldState').replace('run_1f1d20(','polygon(',1)
    children=zero_body().replace('run_2e8d20(','zero(',1)+'\n'+box_body().replace('run_306790(','box(',1)+'\n'+poly
    (scratch/'solver_hull_support_pair_child_routes.inc').write_text(children)
    report.update(strict_context_controls=controls,strict_relocated_continuation_controls=relocations,candidate_source_sha256=binding.digest(text),children_source_sha256=binding.digest(children),polygon_whole_body_pin=polygon.BODY_PINS[match[1]],polygon_whole_context_pins=polygon.OWNER_PINS[match[1]],retained_push_unchanged=True,source_math_reference='Exactly6MULPS/5ADDPS operand order, independently established against unchanged i386; fallback uses unchanged retained source.')
    return report

def rows_for(image):
    rows=box_inputs(image,mixed_inputs(image,inputs(image)))
    for row in rows:
        bank=row['banks'][0];data=bytearray.fromhex(bank['before']);base=bank['address'];context=row['gp'][1]
        data[0x1200:0x1248]=data[context-base:context-base+0x48]
        struct.pack_into('<4I',data,0x2200,0x3f800000,0x3f000000,0xbf000000,0)
        bank['before']=data.hex()
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--mutations',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--discover-contexts',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not a.xbe.is_file():
        print('SKIP support-pair source proof requires Linux x86 and its authenticated original image');return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    names=['src/physics/physics_live_support_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp','tests/native_tu15_solver_hull_support_pair_routes_tests.cpp','tools/native_solver_hull_support_pair_replacements.py','tools/native_polygon_support_replacements.py','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tools/validate_native_solver_hull_support_pair_frame_tu15.py','tools/validate_native_zero_shape_support_tu15.py','tools/validate_native_box_shape_support_tu15.py','tools/native_solver_hull_support_pair_shape_inputs.py','tools/native_solver_hull_support_pair_zero_inputs.py',str(Path(__file__).relative_to(ROOT))]
    pending=list(names);seen=set(names)
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
            rows=rows_for(Xbe(a.xbe));path=scratch/'inputs.json';path.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(path)
            command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_solver_hull_support_pair_routes_tests.cpp'),'-o',str(scratch/'routes')]
            if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Support-pair source candidate compile failed: '+built.stderr[-12000:])
            report['routes']={};env=os.environ.copy();env.pop('HALO2_REFERENCE_SOLVER_HULL_SUPPORT_PAIR',None)
            for label,mode,feedback in [('default',0,False),('certified',1,False),('uncertified',2,False),('wrapped',3,False),('mismatched',4,False),('immutable-copy',5,False),('mutating-trace',6,False),('mutating-callback',7,False),('memory-feedback',1,True),('all32-retirement-frontiers',1,'frontiers'),('own-reference',1,False)]:
                environment=env.copy()
                if label=='own-reference':environment['HALO2_REFERENCE_SOLVER_HULL_SUPPORT_PAIR']='1'
                with path.open('rb')as inp:run=subprocess.run([str(scratch/'routes'),str(mode)]+([feedback if isinstance(feedback,str)else'feedback']if feedback else[]),stdin=inp,capture_output=True,text=True,env=environment)
                if run.returncode:raise ValueError('Complete source candidate failed '+label+': '+run.stderr[-10000:])
                report['routes'][label]=json.loads(run.stdout.strip().splitlines()[-1])
            with path.open('rb')as inp:run=subprocess.run([str(scratch/'routes'),'--source-publication'],stdin=inp,capture_output=True,text=True,env=env)
            if run.returncode:raise ValueError('Retained source publication differs: '+run.stderr[-10000:])
            report['source_publication']=json.loads(run.stdout.strip().splitlines()[-1])
            for key,option in [('entry_refusals','--refusals'),('genuine_unknown_owner','--unknown-owner')]:
                with path.open('rb')as inp:run=subprocess.run([str(scratch/'routes'),option],stdin=inp,capture_output=True,text=True,env=env)
                if run.returncode:raise ValueError('Support-pair source guard/owner control failed '+key+': '+run.stderr[-7000:])
                report[key]=json.loads(run.stdout.strip().splitlines()[-1])
            if a.mutations:
                report['source_mutants']={}
                for name,relative,old,new,option in [
                    ('wrong_physical_PUSH_publication','physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp','Publication::retained','Publication::physical','--source-publication'),
                    ('omitted_policy_retirement','physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp','if(!admitted(v))throw Continuation{next,&token};','if(false&&!admitted(v))throw Continuation{next,&token};','frontiers'),
                    ('stale_first_math_XMM','physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','d.z=s_.xmm[3];d.sign=s_.xmm[1];d.input=s_.xmm[0];d.x=s_.xmm[2];d.y=s_.xmm[5];','/* wrongly retained pre-provider DATA */','feedback'),
                    ('missed_true_child_retirement','physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','call_(s_,target,0x3133abu);retired(0x3133abu);','call_(s_,target,0x3133abu);','feedback')]:
                    header=(ROOT/'src'/relative).read_text()
                    if header.count(old)!=1:raise ValueError('Source mutation seam changed '+name)
                    overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'routes')
                    built=subprocess.run(mutated,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Source mutant compile failed '+name+': '+built.stderr[-6000:])
                    with path.open('rb')as inp:run=subprocess.run([str(overlay/'routes')]+([option]if option.startswith('--')else['1',option]),stdin=inp,capture_output=True,text=True,env=env)
                    if not run.returncode:raise ValueError('Source semantic mutant survived '+name)
                    report['source_mutants'][name]=dict(rejected=True,diagnostic=run.stderr[-8000:])
            report.update(passed=True,production_bound=a.require_binding,scope='Current/fresh raw/prepared source candidates; real virtual children, complete retained parent traces. Child-return policy tests use the same computed child on both sides and qualify the parent checkpoint only. Memory host-FP mutation, child-internal policy fidelity and physical page-fault atomicity remain outside this source qualification.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Source candidate dependency drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','validation_not_run','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
