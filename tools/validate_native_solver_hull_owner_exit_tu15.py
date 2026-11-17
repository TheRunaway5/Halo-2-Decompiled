#!/usr/bin/env python3
"""Compare childless 313BB0 capacity/limit and degenerate return stages on i386."""
import argparse
import hashlib
import json
import os
import platform
import random
import re
import struct
import subprocess
import types
from pathlib import Path

from validate_native_solver_hull_search_setup_frame_tu15 import (
    BASE, BODY_PIN, FS, FS_SITES, PIN, Xbe, actual_body, observe_with_fs, sha,
)

ROOT=Path(__file__).resolve().parents[1]
STOPS=(0x3144a0,0x63000000)
ENTRIES=(0x3141b5,0x314494,0x314478)
REGIONS=((BASE,0x8000),(FS,0x1000))
STACK_READS={0x31448b,0x31448c,0x31448d,0x314490,0x314491}
IO=STACK_READS|{0x3141b5,0x314494,0x31447d,0x314484}
SITES=IO|{0x3141bd,0x31449c,0x314478,0x31448e}
DRIVER=ROOT/'tests/native_tu15_solver_hull_owner_exit_tests.cpp'


def inputs():
    rng=random.Random(0x313bb0);rows=[]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    for route,entry in enumerate(ENTRIES):
        for n in range(16):
            raw=bytearray(rng.randbytes(0x8000));fs=bytearray(rng.randbytes(0x1000));gp=[rng.getrandbits(32)for _ in range(8)]
            sp=BASE+0x4000+(n%4)*4;bp=BASE+0x7200+(n%4)*4
            if n%4==3:bp=sp+8
            gp[4],gp[5]=sp,bp
            struct.pack_into('<I',raw,bp+4-BASE,STOPS[1])
            struct.pack_into('<I',raw,sp+0x3014-BASE,FS+0x100+n*4)
            values=struct.unpack_from('<3I',raw,sp-BASE)
            rows.append(dict(id=len(rows),name=f'hull-exit-{route}-{n}',entry=entry,gp=gp,fs_base=FS,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),0x4100|((n%8)<<11),0x1f80|(n%4)<<13],
                live_x87=[live[j%4]for j in range((0,1,4,7)[n%4])],return_pc=values[0],matrix=values[1],source=values[2],
                stop_before_entries=[STOPS[0]]if route<2 else [],max_steps=80,
                banks=[dict(address=BASE,before=raw.hex()),dict(address=FS,before=fs.hex())]))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
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
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),source_sha256=pins,retained_large_artifacts=[])
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong actual TU1.5 original image')
        if hashlib.sha256(actual_body().encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original 313BB0 body drift')
        image=Xbe(a.xbe);rows=inputs();data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(IO)]
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'hull_exit')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull exit compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull exit stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text())
        globals_=observe_with_fs.__globals__.copy();globals_['FS_SITES']=tuple(pc for pc in FS_SITES if pc not in (0x313ab7,0x313abe,0x313b9c))
        scoped=types.FunctionType(observe_with_fs.__code__,globals_,observe_with_fs.__name__,
                                  observe_with_fs.__defaults__,observe_with_fs.__closure__)
        scoped.__kwdefaults__=observe_with_fs.__kwdefaults__.copy()
        observed,report['original']=scoped(image,rows,scratch,trace,children=(),regions=REGIONS)
        seen=set();counts={name:0 for name in('packets','reads','writes')}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at');expected_stop=STOPS[1] if row['entry']==ENTRIES[2] else STOPS[0]
            if stop!=expected_stop or not case['live_x87_unchanged']:raise ValueError('Original exited private result control scope')
            packets=[];reads=[];original_reads=iter(case['reads'])
            for packet in case['calls']:
                pc=packet['entry']
                if pc in STACK_READS:reads.append(dict(instruction=pc,address=packet['gp'][4],bytes=4))
                elif pc==0x31447d:reads.append(next(original_reads))
                packets.append({k:v for k,v in packet.items()if k!='arguments'})
            if next(original_reads,None)is not None:raise ValueError('Unexpected private exit read')
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,reads=reads,writes=case['writes'],
                 banks=sorted(case['banks'],key=lambda bank:bank['address']))
            if want!=got:
                field=next(key for key in want if want[key]!=got[key])
                report['first_difference']=dict(case=row['name'],field=field)
                raise ValueError('Actual private hull exit differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites'])
        if seen!=SITES:raise ValueError('Actual private hull exit site inventory drift: '+str(sorted(seen^SITES)))
        report.update(passed=True,cases=len(rows),executed_parent_sites=len(seen),executed_sites=sorted(seen),counts=counts,
                      real_ret4=16,original_text_unmodified=True,hostile_provider_comparisons=len(rows),
                      scope='Childless actual capacity/limit publications and degenerate RET4. Declared backed mid-owner frames; no whole-domain or source closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull exit source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
