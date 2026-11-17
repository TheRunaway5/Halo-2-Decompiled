#!/usr/bin/env python3
"""Replay actual 30E89B..30E8D8 output and saved-register ordering on i386."""
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

ENTRIES=(0x30e89b,0x30e8a9,0x30e8b0,0x30e8b5)
FIRST,LAST,STOP=ENTRIES[0],0x30e8d8,0x63000000
STACK={0x30e8c8,0x30e8d3,0x30e8d4,0x30e8d7,0x30e8d8}
VECTORS={0x30e8a9,0x30e8cc}
REGIONS=((0x61000000,0x8000),(0x461000,0x1000))
DRIVER=ROOT/'tests/native_tu15_simplex_hull_finish_frame_tests.cpp'


def inputs(image):
    base,size=REGIONS[0];constant=dict(address=0x461000,before=image.read(0x461000,0x1000).hex())
    rng=random.Random(FIRST);rows=[]
    live=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
    for entry in ENTRIES:
        for n in range(16):
            raw=bytearray(rng.randbytes(size));gp=[rng.getrandbits(32)for _ in range(8)];sp=base+0x4000
            context=(base+0x1000,sp-0x120,base+0x1000,sp-0x10)[n%4]
            bp=sp+8 if n%4==2 else base+0x7200
            gp[4],gp[5],gp[6],gp[7]=sp,bp,context,context+(8 if n&4 else 4)
            struct.pack_into('<I',raw,bp+4-base,STOP);words=struct.unpack_from('<3I',raw,sp-base)
            i=len(rows);rows.append(dict(id=i,name=f'hull-finish-{entry:x}-{n}',entry=entry,gp=gp,
                eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x7f,0x27f,0x37f)[i%3]|((i%4)<<10),0x4100|((i%8)<<11),0x1f80|((i%4)<<13)|(0x8040 if i&4 else 0)],
                live_x87=[live[j%4]for j in range((0,1,4,7)[n%4])],return_pc=words[0],matrix=words[1],source=words[2],
                stop_before_entries=[],max_steps=80,banks=[dict(address=base,before=raw.hex()),constant]))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);paths={DRIVER,Path(__file__),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
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
        if image.read(0x30e8b5,7).hex()!='0f280560194600':raise ValueError('Actual plane sign vector operand drift')
        rows=inputs(image);data=scratch/'inputs.json';data.write_text(json.dumps(rows,separators=(',',':')));report['input_sha256']=sha(data)
        sites=set();io=set();reads=set()
        for line in body.splitlines():
            marker=re.search(r'services.instruction\(world_observation_pc\(0x[0-9a-f]+u,0x([0-9a-f]+)u',line)
            if not marker or not FIRST<=(pc:=int(marker[1],16))<=LAST:continue
            sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory|s.pop\(',line):io.add(pc)
            if re.search(r'\b(?:r|v)\(s.memory',line):reads.add(pc)
        trace=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in sorted(io)]
        command=[a.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(DRIVER),'-o',str(scratch/'hull_finish')]
        if a.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Private hull finish compile failed: '+built.stderr[-10000:])
        out=scratch/'native.json'
        with data.open('rb') as inp,out.open('wb') as output:
            done=subprocess.run([command[-1]],stdin=inp,stdout=output,stderr=subprocess.PIPE)
        if done.returncode:raise ValueError('Native hull finish stopped: '+done.stderr.decode()[-6000:])
        actual=json.loads(out.read_text());observed,report['original']=physical_original(image,rows,scratch,entries=(0x30de60,),regions=REGIONS,
                                                      trace_calls=trace,trace_extended=True,preserve_x87_prefix=True)
        seen=set();counts={name:0 for name in ('packets','reads','writes')}
        for case,got in zip(observed['cases'],actual,strict=True):
            row=rows[case['id']];stop=case.get('stopped_at')
            if stop!=STOP or not case['live_x87_unchanged']:raise ValueError('Original hull finish did not return')
            packets=[];ordered=[];stores=[];original_reads=iter(case['reads'])
            for event in case['calls']:
                pc=event['entry']
                if pc in STACK:ordered.append(dict(instruction=pc,address=event['gp'][4],bytes=4))
                elif pc in reads:
                    item=next(original_reads)
                    if item['instruction']!=pc:raise ValueError('Actual hull finish read order differs')
                    ordered.append(item)
                packets.append({k:v for k,v in event.items()if k!='arguments'})
            if next(original_reads,None)is not None:raise ValueError('Unclassified original finish read')
            for event in case['writes']:
                if event['instruction'] not in VECTORS:stores.append(event);continue
                if len(event['before'])!=32 or len(event['written'])!=32:raise ValueError('Original finish MOVAPS width drift')
                stores.extend(dict(event,address=event['address']+n,before=event['before'][n*2:n*2+16],written=event['written'][n*2:n*2+16])for n in(0,8))
            want=dict(abi=dict(entry=stop,gp=case['boundary_gp'],eflags=case['boundary_eflags'],xmm=case['boundary_xmm'],
                 fp=case['after_fp'],live_x87=row['live_x87']),packets=packets,reads=ordered,writes=stores,
                 banks=sorted(case['banks'],key=lambda bank:bank['address']))
            if want!=got:
                field=next(key for key in want if want[key]!=got[key]);report['first_difference']=dict(case=row['name'],field=field)
                raise ValueError('Actual hull finish differs: '+row['name']+' '+field)
            for key in counts:counts[key]+=len(want[key])
            seen.update(case['executed_sites'])
        if seen!=sites:raise ValueError('Private hull finish site inventory drift')
        report.update(passed=True,cases=len(rows),executed_parent_sites=len(seen),counts=counts,real_ret16=len(rows),
                      original_text_unmodified=True,hostile_provider_comparisons=len(rows),
                      scope='Actual childless output and RET16 with backed saved-slot and plane aliases; no source/default closure.')
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/name)==value for name,value in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Private hull finish source drift')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error','first_difference','executed_parent_sites')if key in report}))
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
