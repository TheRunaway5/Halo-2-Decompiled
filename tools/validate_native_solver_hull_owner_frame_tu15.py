#!/usr/bin/env python3
"""Run complete actual 313BB0 with genuine native children and unchanged i386 authority."""
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

from validate_native_solver_hull_boundary_projection_frame_tu15 import REGIONS as PROJECTION_REGIONS
from validate_native_solver_hull_search_iteration_frame_tu15 import (
    BASE, BODY_PIN, CHILDREN as ITERATION_CHILDREN, PIN, Xbe, actual_body,
    inputs as iteration_inputs, observe_with_fs, sha,
)
from validate_native_solver_hull_search_setup_frame_tu15 import FS_SITES
from validate_native_solver_hull_seed_route_frame_tu15 import DEPTH_ONE

ROOT=Path(__file__).resolve().parents[1]
ENTRY,LAST,STOP=0x313bb0,0x3146f4,0x63000000
TABLE=0x3146f8
CHILDREN={**ITERATION_CHILDREN,0x32a660:0,0x313560:2,0x3136a0:0,0x313790:0,0x313470:3,0x313290:1,
          0x313300:4,0x3122f0:5,0x314b20:1,0x3139f0:0}
REGIONS=(*PROJECTION_REGIONS,(0x41c000,0x1000))
DEPTH={**dict.fromkeys(DEPTH_ONE|{0x313e15,0x3144e9,0x3144fd,0x31450e,0x314650,0x314658,
    0x3141e7,0x314216,0x314230,0x314237,0x31423d,0x314240,0x3142ef,0x3142f3,0x3142f7,0x3142fa,
    0x3142fe,0x314304,0x31430d,0x314311},1),0x3141f2:2,0x3141f6:2,0x3141fc:3}
DRIVER=ROOT/'tests/native_tu15_solver_hull_owner_frame_tests.cpp'


def parent(pc):
    return ENTRY<=pc<=LAST


def inputs(image,smoke):
    seeds=iteration_inputs(image,False);rows=[]
    table=image.read(TABLE,20)
    if struct.unpack('<5I',table)!=(0x313db2,0x313d69,0x313d14,0x313c89,0x313bfc):
        raise ValueError('Complete original solver jump table drift')
    for count in range(6):
        for variant in range(3 if smoke else 12):
            row=copy.deepcopy(seeds[(count*5+variant)%48]);gp=row['gp'];context=gp[6]
            raw=bytearray.fromhex(row['banks'][2]['before']);sp=BASE+0xb700+(variant%8)*4
            def put(address,*words):struct.pack_into('<'+'I'*len(words),raw,address-BASE,*words)
            def vector(address,xyz):struct.pack_into('<4f',raw,address-BASE,*xyz,0.)
            gp[1],gp[4]=context,sp
            config,vertices,out=BASE+0x4800,BASE+0x4500,BASE+0x7000
            put(context+0x48,config,0,vertices);put(config+0x80,count)
            put(sp,STOP,out,0x734521ab);put(sp+12,0x13452789)
            if count<3:
                point=((2.,0.,0.),(0.,2.,0.),(0.,0.,2.),(1.,2.,3.),(-2.,-3.,-1.),(0.,0.,0.))[variant%6]
                vector(vertices,point);vector(vertices+16,(-point[0],-point[1],-point[2]))
                vector(config,point);vector(config+16,(-point[0],-point[1],-point[2]))
                if variant%6==5:vector(context+0x30,(0.,0.,0.))
            if count==0:
                # Zero support shapes plus zero translation are the genuine
                # initialized-origin return exercised by the actual child.
                vector(context+0x30,(0.,0.,0.))
            if count==3:
                triangle=((1.,0.,0.),(0.,1.,0.),(-1.,-1.,0.))if variant%2 else ((1.,0.,1.),(0.,1.,1.),(-1.,-1.,1.))
                for n,v in enumerate(triangle):vector(vertices+16*n,v);vector(config+16*n,v)
                if variant%3==0:vector(context+0x30,(0.,0.,0.))
            row.update(id=len(rows),name=f'full-hull-{count}-{variant}',entry=ENTRY,return_pc=STOP,matrix=out,source=0x734521ab,
                       stop_before_entries=[],max_steps=80000,live_x87=row['live_x87'][:4],actual_table=table.hex())
            row['banks'][2]['before']=raw.hex()
            for address,size in REGIONS[-2:]:row['banks'].append(dict(address=address,before=image.read(address,size).hex()))
            rows.append(row)
    return rows


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
                allocator_returns_qualified=False,complete_owner_domain=False,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        body=actual_body()
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original 313BB0 body drift')
        image=Xbe(a.xbe);rows=inputs(image,a.smoke)
        if image.read(0x313bf5,7).hex()!='ff2485f8463100':raise ValueError('Original switch table opcode drift')
        data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites=set();io=set();read_sites=set();stack=set();vectors=set()
        for line in body.splitlines():
            found=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not found:continue
            pc=int(found[1],16);sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(|s.pop\(',line):io.add(pc)
            if re.search(r'\b(?:r|v)\(s.memory',line):read_sites.add(pc)
            if 's.pop()'in line:stack.add(pc)
            if 'vw(s.memory' in line:vectors.add(pc)
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=DEPTH.get(pc,0))for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'hull_owner')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull owner compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE,timeout=120)
        if done.returncode:raise ValueError('Native hull owner stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        globals_=observe_with_fs.__globals__.copy();globals_['FS_SITES']=tuple(sorted((*FS_SITES,0x3139f7,0x3139fe,0x313a9b)))
        scoped=types.FunctionType(observe_with_fs.__code__,globals_,observe_with_fs.__name__,
                                  observe_with_fs.__defaults__,observe_with_fs.__closure__)
        scoped.__kwdefaults__=observe_with_fs.__kwdefaults__.copy()
        observed,report['original']=scoped(image,rows,scratch,trace,children=CHILDREN,regions=REGIONS)
        seen=set();codes={};counts={name:0 for name in('packets','children','reads','writes')};branches={}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop!=STOP or not case['live_x87_unchanged']:raise ValueError('Original full hull did not return')
            calls=[];stores=[];packets=[];children=[];reads=[]
            original_reads=iter(v for v in case['reads']if parent(v['instruction']))
            for event in case['writes']:
                pc=event['instruction'];opcode=image.read(pc,1)[0]
                if opcode==0xe8 or pc in(0x3133ab,0x313425):
                    returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                    width=3 if pc in(0x3133ab,0x313425)else 5
                    if returned!=pc+width:raise ValueError('Original complete hull child return drift')
                    calls.append(returned)
                if not parent(pc):continue
                if pc not in vectors:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original complete hull MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            for event in case['calls']:
                pc=event['entry']
                if pc in CHILDREN:
                    children.append(dict(event,return_word=calls[len(children)]));continue
                if pc in stack:reads.append(dict(instruction=pc,address=event['gp'][4],bytes=4))
                elif pc==0x313bf5:
                    address=TABLE+event['gp'][0]*4
                    if address>=TABLE+20:raise ValueError('Original table index escaped actual table')
                    reads.append(dict(instruction=pc,address=address,bytes=4))
                elif pc in read_sites:
                    read=next(original_reads)
                    if read['instruction']!=pc:raise ValueError(f'Original ordered read differs at {pc:x}: {read}')
                    reads.append(read)
                packets.append({k:v for k,v in event.items()if k!='arguments'})
            if next(original_reads,None)is not None or len(children)!=len(calls):raise ValueError('Unclassified original complete hull transport')
            abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                     fp=case['after_fp'],live_x87=row['live_x87'])
            want=dict(abi=abi,packets=packets,children=children,banks=sorted(case['banks'],key=lambda bank:bank['address']),reads=reads,writes=stores)
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
                raise ValueError('Actual complete hull differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            executed=set(case['executed_sites']);seen.update(executed);code=abi['gp'][0];codes[code]=codes.get(code,0)+1
            for pc in (0x314350,0x314475,0x314491,0x3146f4):
                if pc in executed:branches[hex(pc)]=branches.get(hex(pc),0)+1
        report.update(complete_original_calls=len(rows),parent_sites=len(sites),executed_parent_sites=len(seen&sites),
                      missing_sites=sorted(sites-seen),counts=counts,return_codes=codes,returns=branches,
                      original_text_unmodified=True,supplied_child_outcomes=False)
        report.update(passed=True,scope='Complete actual 313BB0 entry-to-return for admitted backed inline cases; no dynamic allocator body, retained source, default, or complete-domain claim.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull owner source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites','returns')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
