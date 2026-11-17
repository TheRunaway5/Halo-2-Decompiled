#!/usr/bin/env python3
"""Compare the actual 30DE60 paired-reducer dispatch and genuine descendants."""
import argparse
import hashlib
import json
import os
import platform
import random
import re
import struct
import subprocess
from pathlib import Path

from validate_native_simplex_hull_seed_frame_tu15 import BODY_PIN, PIN, ROOT, Xbe, authored, sha
from validate_native_tu15_prt_helpers import physical_original

FIRST,LAST=0x30e25f,0x30e2a9
ENTRIES=(FIRST,0x30e27d)
EXITS={0x30e2af,0x30e8b0,0x30e8b5}
REGIONS=((0x61000000,0x8000),(0x41d000,0x1000),(0x46a000,0x1000),(0x41c000,0x1000),(0x461000,0x1000))
CHILDREN={0x30d830:6,0x30d3f0:3,0x312620:5}
DRIVER=ROOT/'tests/native_tu15_simplex_hull_paired_call_tests.cpp'
CHECK_CALL=(0x30e29c,'e88ff5ffff')
ORIGINAL_ENTRY=0x30de60
EXTRA_INPUTS=()
SCOPE='Declared actual mid-owner paired dispatch with genuine reducer/leaf returns; no source/default or later reduction closure.'


def parent(pc):
    return FIRST<=pc<=LAST


def inputs(image):
    base,size=REGIONS[0];context,sp=base+0x1000,base+0x7000
    constants=[dict(address=address,before=image.read(address,size).hex())for address,size in REGIONS[1:]]
    rng=random.Random(FIRST);rows=[]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    for route,entry in enumerate(ENTRIES):
        for n in range(40):
            raw=bytearray(rng.randbytes(size));gp=[rng.getrandbits(32)for _ in range(8)];gp[4],gp[6],gp[7]=sp,context,context+4
            segment,triangle=(context+0xa0,context+0x20)if route else (context+0x20,context+0xa0)
            def v(p,xyz):struct.pack_into('<4f',raw,p-base,*xyz,0.)
            struct.pack_into('<3I',raw,context-base,3 if route else 2,2 if route else 3,2 if n&1 else 0)
            for i in range(3):v(triangle+16*i,((0.,0.,0.),(1.,0.,0.),(0.,1.,0.))[i])
            if n>=16:
                for i in range(3):v(triangle+16*i,tuple(rng.randrange(-12,13)/8 for _ in range(3)))
            first=((.25,.25,-1.),(2.,2.,-1.),(.25,.25,1.),(.25,.25,-2.))[(n//2)%4]
            second=((.25,.25,1.),(2.,2.,1.),(.25,.25,2.),(.25,.25,-1.))[(n//2)%4]
            if n>=16:first,second=(tuple(rng.randrange(-24,25)/8 for _ in range(3))for _ in range(2))
            v(segment,first);v(segment+16,second)
            for start in (segment,triangle):
                for i in range(4):v(start+0x40+16*i,tuple(rng.randrange(-12,13)/8 for _ in range(3)))
            v(context+0x120,(.25,.25,.5));v(context+0x130,(0.,0.,1.))
            words=struct.unpack_from('<3I',raw,sp-base);k=len(rows)
            rows.append(dict(id=k,name=f'paired-hull-{route}-{n}',entry=entry,gp=gp,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10),0x4100|((k%8)<<11),0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)],
                live_x87=[live[j%4]for j in range((0,1,4,6)[n%4])],return_pc=words[0],matrix=words[1],source=words[2],
                stop_before_entries=sorted(EXITS),max_steps=12000,banks=[dict(address=base,before=raw.hex()),*constants]))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths={DRIVER,Path(__file__),*EXTRA_INPUTS,ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
                                                        ROOT/'tools/validate_native_tu15_prt_helpers.py'};pending=list(paths)
    while pending:
        path=pending.pop()
        for name in re.findall(r'^#include "([^"\n]+)"',path.read_text(),re.M):
            found=next((v for v in(path.parent/name,ROOT/'include'/name,ROOT/'src'/name,ROOT/'tests'/name)if v.is_file()),None)
            if found is not None and found not in paths:paths.add(found);pending.append(found)
    pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(paths)}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        body=authored()
        if sha(a.xbe)!=PIN or hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Actual original image or complete World body drift')
        image=Xbe(a.xbe)
        if image.read(CHECK_CALL[0],5).hex()!=CHECK_CALL[1]:raise ValueError('Actual reduction child CALL drift')
        rows=inputs(image);data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites=set();io=set();vectors=set()
        for line in body.splitlines():
            marker=re.search(r'services.instruction\(world_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u',line)
            if not marker or not parent(pc:=int(marker[1],16)):continue
            sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.push\(',line):io.add(pc)
            if 'vw(s.memory'in line:vectors.add(pc)
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(io)]
        trace.extend(dict(entry=pc,arguments=n,gp=True,xmm=True)for pc,n in CHILDREN.items())
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'hull_paired')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull paired compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull paired stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text());observed,report['original']=physical_original(image,rows,scratch,entries=(ORIGINAL_ENTRY,)+tuple(CHILDREN),
                                    regions=REGIONS,trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        seen=set();exits={};counts={name:0 for name in ('packets','children','reads','writes')};direct_returns=0
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop not in EXITS or not case['live_x87_unchanged']:raise ValueError('Actual paired caller left its private scope')
            packets=[];children=[];returns=[];stores=[]
            for event in case['writes']:
                pc=event['instruction']
                if parent(pc):
                    if pc not in vectors:stores.append(event)
                    else:
                        if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Actual reduction MOVAPS width drift')
                        stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
                if image.read(pc,1)!=b'\xe8':continue
                returned=struct.unpack('<I',bytes.fromhex(event['written']))[0]
                if returned!=pc+5:raise ValueError('Actual paired child return drift')
                returns.append(returned)
            for event in case['calls']:
                pc=event['entry']
                if pc in CHILDREN:children.append(dict(event,return_word=returns[len(children)]))
                else:packets.append({k:v for k,v in event.items()if k!='arguments'})
            if len(returns)!=len(children):raise ValueError('Unobserved genuine paired descendant')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,children=children,
                 reads=[e for e in case['reads']if parent(e['instruction'])],writes=stores,
                 banks=sorted(case['banks'],key=lambda bank:bank['address']))
            if want!=got:
                field=next(key for key in want if want[key]!=got[key]);report['first_difference']=dict(case=row['name'],field=field)
                expected,actual_field=want[field],got[field]
                if isinstance(expected,list) and isinstance(actual_field,list) and field!='banks':
                    first=next((n for n,(a,b) in enumerate(zip(expected,actual_field))if a!=b),min(len(expected),len(actual_field)))
                    expected,actual_field=expected[first:first+1],actual_field[first:first+1]
                if field!='banks':report['first_difference'].update(expected=expected,actual=actual_field)
                raise ValueError('Actual paired dispatch differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            direct_returns+=sum(event['entry']==next(iter(CHILDREN),None)for event in children)
            seen.update(case['executed_sites']);exits[stop]=exits.get(stop,0)+1
        report.update(cases=len(rows),executed_parent_sites=len(seen&sites),missing_sites=sorted(sites-seen),counts=counts,
                      continuations=exits,real_direct_child_returns=direct_returns,original_text_unmodified=True,hostile_provider_comparisons=len(rows))
        if sites-seen or set(exits)!=EXITS:raise ValueError('Incomplete actual paired dispatch continuation coverage')
        report.update(passed=True,scope=SCOPE)
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull paired source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites','continuations')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
