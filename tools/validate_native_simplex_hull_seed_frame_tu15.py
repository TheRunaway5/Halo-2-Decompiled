#!/usr/bin/env python3
"""Qualify actual 30DE60 seed construction through genuine 314710 and 313BB0."""
import argparse
import copy
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
import types
from pathlib import Path

from generated_routine_ownership import function_bodies
from validate_native_solver_hull_context_composed_tu15 import (
    ALL_CHILDREN, BASE, BODY_PIN as INNER_PIN, DEPTH, DRIVER as BASE_DRIVER, ENTRY as SOLVER,
    LAST as SOLVER_LAST, OUTER, OUTER_LAST, OUTER_PIN, PIN, REGIONS, TABLE, Xbe,
    actual_body as solver_body, inputs as context_inputs, observe_original, outer_body, prepare_driver, sha,
)

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
FRAME=ROOT/'src/physics/runtime/native_tu15_simplex_hull_seed_frame.hpp'
ENTRY,LAST,STOP=0x30de60,0x30e013,0x30e018
BODY_PIN='08aea2d6c59de447d6dcebd6c3bc4182e5b47b25da1d1b8d28e20d1d7503abf7'
CHILDREN={**ALL_CHILDREN,OUTER:6}
FORMS=((1,1),(3,1),(4,1),(3,2),(1,3),(2,3),(1,4),(4,4),(-1,4),(4,-1),(-1,-1))
ACCEPTED_STOPS={STOP}
EXTRA_INPUTS=()
REQUIRE_PARENT_SITES=True
READONLY_HULL_REDUCTION=False
READONLY_SIMPLEX_REDUCTION=False
ORIGINAL_ENTRY=None
REFERENCE_REGIONS=None
EXTRA_IO=()
INDIRECT_CALLS={0x3133ab:3,0x313425:3}
WITNESS_PCS=()
SCOPE='Actual 30DE60 seed through genuine context and full backed inline hull solver. Profiles off; no later 30DE60, retained source or default claim.'


def authored():
    return next(body for sig,body in function_bodies(SOURCE.read_text())if sig.symbol=='30a370')


def parent(pc):
    return ENTRY<=pc<=LAST or SOLVER<=pc<=SOLVER_LAST or OUTER<=pc<=OUTER_LAST


def inputs(image,smoke):
    bases=context_inputs(image,False);rows=[]
    for form,(first_count,second_count)in enumerate(FORMS):
        for variant in range(1 if smoke else 4):
            row=copy.deepcopy(bases[form*3+variant]);sp=row['gp'][4];transform=row['gp'][1];context=BASE+0x2000
            row['gp'][1]=context
            main=next(bank for bank in row['banks']if bank['address']==BASE);raw=bytearray.fromhex(main['before'])
            a,b=struct.unpack_from('<2I',raw,transform+0x40-BASE)
            struct.pack_into('<4I',raw,sp-BASE,0x63000000,a,b,transform)
            struct.pack_into('<2i',raw,context-BASE,first_count,second_count)
            tetra=((-1.,-1.,-1.),(1.,1.,-1.),(1.,-1.,1.),(-1.,1.,1.))
            if variant&1:tetra=((-1.,0.,0.),(0.,-1.,0.),(0.,0.,-1.),(2.,2.,2.))
            def v(address,xyz):struct.pack_into('<4f',raw,address-BASE,*xyz,0.)
            for n,point in enumerate(tetra):
                first=(0.,0.,0.)if second_count>=3 else point
                second=tuple(-x for x in point)if second_count>=3 else (0.,0.,0.)
                if first_count<3 and second_count<3:first=(2.,0.,0.)if n==0 else first
                v(context+0x20+n*16,first);v(context+0xa0+n*16,second);v(context+0xe0+n*16,second)
            v(transform+0x30,((-1.,0.,0.),(1.,-1.,0.),(2.,1.,0.),(-2.,1.,1.))[variant])
            main['before']=raw.hex();page=next(bank for bank in row['banks']if bank['address']==0x48c000)
            values=bytearray.fromhex(page['before']);struct.pack_into('<2I',values,0x974,BASE+0x5700,BASE+0x5700);page['before']=values.hex()
            row.update(id=len(rows),name=f'hull-seed-{first_count}-{second_count}-{variant}',entry=ENTRY,matrix=a,source=b,
                       stop_before_entries=[STOP],observed_clocks=[],max_steps=80000)
            rows.append(row)
    return rows


def driver(scratch):
    path=prepare_driver(scratch);text=path.read_text()
    def replace(old,new):
        nonlocal text
        if text.count(old)!=1:raise ValueError('Private 30DE60 proof overlay anchor drift')
        text=text.replace(old,new,1)
    replace('#include "physics/runtime/native_tu15_solver_hull_context_frame.hpp"',
            '#include "physics/runtime/native_tu15_solver_hull_context_frame.hpp"\n#include "physics/runtime/native_tu15_simplex_hull_seed_frame.hpp"')
    replace('return (pc>=0x313bb0u&&pc<=0x3146f4u)||(pc>=0x314710u&&pc<=0x3147cdu);',
            'return (pc>=0x30de60u&&pc<=0x30e013u)||(pc>=0x313bb0u&&pc<=0x3146f4u)||(pc>=0x314710u&&pc<=0x3147cdu);')
    replace('std::array<std::uint32_t,6> words{}','std::array<std::uint32_t,7> words{}')
    replace('    native_solver_hull_context_detail::run_private_owner(state,actual_child,before);','''    const auto actual_context=[&](WorldState& next,std::uint32_t target,std::uint32_t site){
        check(target==0x314710u&&site==0x30e013u,"Wrong real simplex hull context target");enter(next,target,site,6u);
        native_solver_hull_context_detail::run_private_owner(next,actual_child,before);
    };
    native_simplex_hull_seed_detail::run_private_hull_seed(state,actual_context,before);''')
    replace('constexpr auto exit=0x63000000u;','constexpr auto exit=0x30e018u;')
    path.write_text(text);return path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths={BASE_DRIVER,FRAME,Path(__file__),*EXTRA_INPUTS,ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
           ROOT/'src/physics/runtime/native_tu15_solver_hull_context_frame.hpp'};pending=list(paths)
    while pending:
        path=pending.pop();text=path.read_text();names=re.findall(r'^#include "([^"\n]+)"',text,re.M)
        if path.suffix=='.py':names+=['tools/'+m+'.py'for m in re.findall(r'^(?:from|import) (\w+)',text,re.M)]
        for name in names:
            found=next((v for v in(path.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        bodies=(authored(),solver_body(),outer_body())
        if tuple(hashlib.sha256(body.encode()).hexdigest()for body in bodies)!=(BODY_PIN,INNER_PIN,OUTER_PIN):
            raise ValueError('Complete actual composed seed source owner drift')
        image=Xbe(a.xbe);rows=inputs(image,a.smoke)
        if image.read(0x313bf5,7).hex()!='ff2485f8463100':raise ValueError('Original solver table opcode drift')
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites=set();io=set(EXTRA_IO);read_sites=set();stack=set();vectors=set()
        for line in '\n'.join(bodies).splitlines():
            marker=re.search(r'services.instruction\((?:world_observation_pc\(0x[0-9a-f]+u,)?0x([0-9a-f]+)u',line)
            if not marker or not parent(pc:=int(marker[1],16)):continue
            sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(|s.pop\(',line):io.add(pc)
            if re.search(r'\b(?:r|v)\(s.memory',line):read_sites.add(pc)
            if 's.pop()'in line:stack.add(pc)
            if 'vw(s.memory'in line:vectors.add(pc)
        traces=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=DEPTH.get(pc,0))for pc in sorted(io)]
        traces.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        globals_=observe_original.__globals__.copy()
        globals_.update(OUTER=ORIGINAL_ENTRY or ENTRY,ALL_CHILDREN=CHILDREN,READONLY_HULL_REDUCTION=READONLY_HULL_REDUCTION,
                        READONLY_SIMPLEX_REDUCTION=READONLY_SIMPLEX_REDUCTION,REGIONS=REFERENCE_REGIONS or REGIONS)
        original=types.FunctionType(observe_original.__code__,globals_,observe_original.__name__,
                                    observe_original.__defaults__,observe_original.__closure__)
        observed,report['original']=original(image,rows,scratch,traces)
        path=driver(scratch);report['owned_driver_overlay_sha256']=sha(path)
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(path),'-o',str(scratch/'simplex_hull_seed')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private simplex hull seed compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE,timeout=120)
        if done.returncode:raise ValueError('Native simplex hull seed stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text());seen=set();counts={name:0 for name in('packets','children','reads','writes')};exits={};owners={};witnesses={}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in ACCEPTED_STOPS or not case['live_x87_unchanged']:raise ValueError('Original simplex seed did not reach an authenticated continuation')
            if WITNESS_PCS:
                picked={}
                for event in case['calls']:
                    if event['entry'] in WITNESS_PCS:
                        picked.setdefault(f"{event['entry']:x}",dict(eax=event['gp'][0],ecx=event['gp'][1],flags=event['eflags'],xmm4=event['xmm'][4][0]))
                if picked:witnesses[row['name']]=picked
            exits[stop]=exits.get(stop,0)+1
            calls=[];stores=[];packets=[];children=[];reads=[]
            original_reads=iter(v for v in case['reads']if parent(v['instruction']))
            for event in case['writes']:
                pc=event['instruction'];opcode=image.read(pc,1)[0]
                if opcode==0xe8 or pc in INDIRECT_CALLS:
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0];width=INDIRECT_CALLS.get(pc,5)
                    if returned!=pc+width:raise ValueError('Actual simplex hull seed return drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in vectors:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Actual composed MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                pc=event['entry'];arity=len(event['arguments'])
                if pc in CHILDREN and arity==CHILDREN[pc]:
                    owners[pc]=owners.get(pc,0)+1
                    children.append(dict(event,return_word=calls[len(children)]));continue
                if pc in stack:reads.append(dict(instruction=pc,address=event['gp'][4],bytes=4))
                elif pc==0x313bf5:
                    address=TABLE+event['gp'][0]*4
                    if address>=TABLE+20:raise ValueError('Original table index escaped actual table')
                    reads.append(dict(instruction=pc,address=address,bytes=4))
                elif pc in read_sites:
                    read=next(original_reads)
                    if read['instruction']!=pc:raise ValueError(f'Actual composed read differs at {pc:x}')
                    reads.append(read)
                packets.append({k:v for k,v in event.items()if k!='arguments'})
            if next(original_reads,None)is not None or len(children)!=len(calls):raise ValueError('Unclassified actual simplex hull seed transport')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=children,
                 banks=sorted(case['banks'],key=lambda bank:bank['address']),reads=reads,writes=stores)
            for field,expected in want.items():
                if expected==got[field]:continue
                received=got[field];where=None
                if isinstance(expected,list):
                    where=next((n for n,(x,y) in enumerate(zip(expected,received))if x!=y),min(len(expected),len(received)))
                    expected,received=expected[where:where+1],received[where:where+1]
                if field=='banks' and expected and received:
                    x,y=expected[0]['after'],received[0]['after'];at=next(n for n,(a,b)in enumerate(zip(x,y))if a!=b)//2
                    expected,received=dict(address=expected[0]['address']+at,bits=x[at*2:at*2+16]),dict(bits=y[at*2:at*2+16])
                report['first_difference']=dict(case=row['name'],field=field,ordinal=where,expected=expected,actual=received)
                raise ValueError('Actual simplex hull seed differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites'])
        outer_sites={pc for pc in sites if ENTRY<=pc<=LAST};missing=outer_sites-seen
        report.update(cases=len(rows),parent_sites=len(outer_sites),executed_parent_sites=len(outer_sites&seen),missing_sites=sorted(missing),
                      counts=counts,real_314710_returns=owners.get(OUTER,0),executed_child_entries=owners,continuations=exits,
                      declared_mid_owner_cases=sum(row['entry']!=ENTRY for row in rows),
                      original_text_unmodified=True,supplied_child_outcomes=False)
        if missing and REQUIRE_PARENT_SITES:raise ValueError('Incomplete actual simplex seed site coverage')
        if WITNESS_PCS:report['original_site_witnesses']=witnesses
        report.update(passed=True,scope=SCOPE)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private simplex hull seed source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites','missing_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
