#!/usr/bin/env python3
"""Run the real tetrahedron and one complete 313BB0 support search iteration."""
import argparse
import copy
import hashlib
import json
import os
import platform
import re
import struct
import subprocess
from pathlib import Path

from validate_native_solver_hull_search_setup_frame_tu15 import (
    BASE, BODY_PIN, ENTRY as SETUP, PIN, REGIONS as SETUP_REGIONS, Xbe, actual_body,
    child_bodies, inputs as setup_inputs, observe_with_fs, sha,
)

ROOT=Path(__file__).resolve().parents[1]
ENTRY,LAST=0x313f63,0x3140d7
EXITS={ENTRY,0x3140dd,0x3141b5,0x3144a0}
CHILDREN={0x313ab0:4,0x314d80:0,0x314a30:0,0x3131b0:0,0x313390:2,0x2e8d20:2,
          0x2e47b0:2,0x314c40:4,0x3148e0:4,0x3147d0:3,0x314ed0:3,
          0x3149b0:2,0x314cb0:4,0x314870:0}
REGIONS=(*SETUP_REGIONS,(0x46e000,0x1000),(0x461000,0x1000))
VECTORS={0x313f94,0x313fa8,0x313fc4,0x313fd7,0x3140ac,0x3140d4}
DRIVER=ROOT/'tests/native_tu15_solver_hull_search_iteration_frame_tests.cpp'


def parent(pc):
    return ENTRY<=pc<=LAST


def inputs(image,smoke):
    rows=setup_inputs(image,False)
    if smoke:rows=rows[:18]
    for k in range(10):
        row=copy.deepcopy(rows[k]);row['id']=len(rows);rows.append(row)
    for row in rows:
        k=row['id'];gp=row['gp'];context=gp[6]
        raw=bytearray.fromhex(row['banks'][2]['before'])
        def words(address,*bits):struct.pack_into('<'+'I'*len(bits),raw,address-BASE,*bits)
        def vector(address,*values):struct.pack_into('<4f',raw,address-BASE,*values)
        vertex=BASE+0x4500;payload=BASE+0x4800
        words(context+0x40,BASE+0x5000,BASE+0x6000,payload,0,vertex)
        words(BASE+0x5000,0x4613a0);words(BASE+0x6000,0x4613a0)
        for n in range(3):vector(context+16*n,*(1. if j==n else 0. for j in range(4)))
        p=k%12
        shift=((-4,-2,-1),(-2,0,0),(-1,-1,-1),(-1,0,0),(-.1,0,0),(0,0,0),(.1,0,0),
               (1,0,0),(1,1,1),(2,0,0),(4,2,1),(4,4,4))[p]
        vector(context+0x30,*shift,0.)
        hull=((-1,-1,-1),(1,1,-1),(1,-1,1),(-1,1,1))
        if (k//12)&1:hull=((-1,0,0),(0,-1,0),(0,0,-1),(2,2,2))
        if (k//12)&2:hull=tuple(tuple(v*.05 for v in point)for point in hull)
        if k>=len(rows)-10:
            hull=((0.,0.,0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.))
            j=k-len(rows)+10
            if j<8:
                e=(0.,1e-7,1e-6,1e-5,1e-4,0.001,0.01,0.1)[j]
                vector(context+0x30,e,e,e,0.)
            else:
                # The original SIMD gate is ordered less-than, whereas the
                # actual Clip child handles unordered x87 projections. Give
                # the genuine Zero/Pair path a backed masked-NaN translation.
                words(context+0x30,0x7fc12345 if j==8 else 0xff812345,0,0,0)
        for i,point in enumerate(hull):
            vector(vertex+16*i,*point,0.);vector(payload+16*i,*point,0.)
            vector(payload+0x40+16*i,0.,0.,0.,0.)
        row.update(name=f'first-search-iteration-{k}',stop_before_entries=sorted(EXITS),max_steps=10000,live_x87=row['live_x87'][:5])
        row['banks'][2]['before']=raw.hex()
        for address,size in REGIONS[-2:]:row['banks'].append(dict(address=address,before=image.read(address,size).hex()))
    return rows


def capacity_rows(image,rows,scratch):
    """Use actual setup's backed continuation, then declare an exactly full edge array."""
    directory=scratch/'seeded-capacity';directory.mkdir()
    seeds=copy.deepcopy(rows[:12])
    for seed in seeds:seed['stop_before_entries']=[ENTRY]
    observed,proof=observe_with_fs(image,seeds,directory,[],regions=REGIONS)
    result=[]
    for row,case in zip(seeds,observed['cases'],strict=True):
        if case.get('stopped_at')!=ENTRY or not case['live_x87_unchanged']:
            raise ValueError('Capacity input did not reach genuine search iteration entry')
        sp=case['boundary_gp'][4];banks=[dict(address=b['address'],before=b['after'])for b in case['banks']]
        main=next(b for b in banks if b['address']==BASE);raw=bytearray.fromhex(main['before'])
        count,capacity=struct.unpack_from('<2I',raw,sp+0x2924-BASE)
        if (count,capacity)!=(4,0x80000037):raise ValueError('Real initial inline edge capacity drift')
        struct.pack_into('<I',raw,sp+0x2928-BASE,0x80000000|count)
        main['before']=raw.hex();words=struct.unpack_from('<3I',raw,sp-BASE)
        row.update(id=len(rows)+len(result),name=f'full-inline-edge-capacity-{row["id"]}',entry=ENTRY,gp=case['boundary_gp'],
                   eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],before_fp=case['after_fp'],
                   return_pc=words[0],matrix=words[1],source=words[2],stop_before_entries=sorted(EXITS),banks=banks)
        result.append(row)
    return result,proof


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths={DRIVER,Path(__file__),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'};pending=list(paths)
    while pending:
        path=pending.pop();text=path.read_text();names=re.findall(r'^#include "([^"\n]+)"',text,re.M)
        if path.suffix=='.py':names+=['tools/'+m+'.py'for m in re.findall(r'^(?:from|import) (\w+)',text,re.M)]
        for name in names:
            found=next((v for v in(path.parent/name,ROOT/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,
                complete_owner=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original 313BB0 body drift')
        child_bodies();image=Xbe(a.xbe);rows=inputs(image,a.smoke)
        extra,report['real_setup_for_capacity_rows']=capacity_rows(image,rows,scratch);rows+=extra
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites={int(m[1],16)for m in re.finditer(r'services.instruction\(0x([0-9a-f]+)u\)',body)if parent(int(m[1],16))}
        io={int(m[1],16)for line in body.splitlines()if (m:=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line))
            and parent(int(m[1],16))and re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line)}
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'search_iteration')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull search iteration compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull search iteration stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        observed,report['original']=observe_with_fs(image,rows,scratch,trace,children=CHILDREN,regions=REGIONS,pass_first_stop=ENTRY)
        seen=set();exits={};target_counts={};counts={name:0 for name in('packets','children','reads','writes')}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in EXITS or not case['live_x87_unchanged']:raise ValueError('Original escaped private search iteration scope')
            calls=[];stores=[];packets=[];children=[]
            for event in case['writes']:
                pc=event['instruction'];opcode=image.read(pc,1)[0]
                if opcode==0xe8 or pc in(0x3133ab,0x313425):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    width=3 if pc in(0x3133ab,0x313425)else 5
                    if returned!=pc+width:raise ValueError('Original search iteration child return instruction drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in VECTORS:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original search iteration MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                target=event['entry']
                if target in CHILDREN:
                    children.append(dict(event,return_word=calls[len(children)]));target_counts[target]=target_counts.get(target,0)+1
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            if len(children)!=len(calls):raise ValueError('Unobserved genuine search iteration descendant CALL')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                               fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=children,
                      banks=sorted(case['banks'],key=lambda bank:bank['address']),
                      reads=[event for event in case['reads']if parent(event['instruction'])],writes=stores)
            for field,expected in want.items():
                if expected==got[field]:continue
                received=got[field];where=None
                if isinstance(expected,list):
                    where=next((n for n,(x,y)in enumerate(zip(expected,received))if x!=y),min(len(expected),len(received)))
                    expected,received=expected[where:where+1],received[where:where+1]
                if field=='banks' and expected and received:
                    x,y=expected[0]['after'],received[0]['after'];at=next(n for n,(a,b)in enumerate(zip(x,y))if a!=b)//2
                    expected,received=dict(address=expected[0]['address']+at,bits=x[at*2:at*2+16]),dict(bits=y[at*2:at*2+16])
                report['first_difference']=dict(case=row['name'],field=field,ordinal=where,expected=expected,actual=received)
                raise ValueError('Actual search iteration differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites']);exits[stop]=exits.get(stop,0)+1
        report.update(cases=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),missing_sites=sorted(sites-seen),
                      counts=counts,original_text_unmodified=True,supplied_child_outcomes=False,continuations=exits,
                      executed_child_targets=target_counts)
        if not a.smoke and (sites-seen or set(exits)!=EXITS):raise ValueError('Incomplete actual search iteration coverage')
        report.update(passed=True,scope='Actual search setup and first iteration through genuine original/native child bodies; declared backed inline storage, real Zero support. No retained source, whole owner, allocator or fault closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private search iteration source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','missing_sites','continuations')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
