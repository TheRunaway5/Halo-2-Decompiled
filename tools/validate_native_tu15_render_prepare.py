#!/usr/bin/env python3
"""Unchanged physical449D0/41C10/41C70 versus native typed preparation.

Physical originals execute real130980 resident/unavailable paths and genuine
D7230/41970 services with declared inactive attachments. No supplied outcomes
or patched instructions. Nonempty leaf work and rendering are separate gates.
Run via run_test.py; transient executables and banks stay in its scratch.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
DATA=0x1000000
POOL,ROWS,TAGS,MODEL,SECTIONS,BSP,INST,DEFS,CLUST,BODY,QUEUE,FRAME,META,STAMPS=range(DATA,DATA+14*4096,4096)
STACK=0x62010000


def cases():
    rows=[]
    def new(entry):
        row=dict(id=len(rows),name=f'{entry:x}-{len(rows)}',entry=entry,gp=[0,0,0,0,base.SP,0,0,0],matrix=0,source=0,flags=0,
            before_fp=[0x27f,0x220,0x1fa0],live_x87=[],max_steps=50000)
        banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
        def put(at,v,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped prepare input '+hex(at))
        put(0x4f0414,TAGS);put(TAGS+8,MODEL);put(MODEL+0x28,SECTIONS);put(0x4ecc18,BSP)
        put(BSP+0xa0,CLUST);put(BSP+0x144,INST);put(BSP+0x13c,DEFS)
        put(0x4ecbd0,POOL);put(POOL+0x44,ROWS)
        for i in range(8):
            put(ROWS+12*i+8,BODY+0x100*i);put(BODY+0x100*i+0xc,0xffffffff);put(BODY+0x100*i+0x10,0xffffffff)
            put(BODY+0x100*i+0xdc,0xffffffff)
        def finish():row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
        return row,put,finish
    for kind,unit,request,resident,case in itertools.product(range(3),(0,1,65535),(0,1,0x100,255),(False,True),range(3)):
        row,put,finish=new(0x449d0);row['gp'][0]=0xbeef0000|unit;row['gp'][2]=request
        owner=(0x101 if kind==2 else 0x100);row['matrix']=owner
        at=0x4c6958+32*(unit if unit<32768 else unit-65536)
        index=(1,511,255)[case]if kind!=2 else(7,2047,1023)[case];datum=(0,7,65535)[case]
        definition=(0,-1,-32768)[case]
        put(at+4,(0xabcd0000|datum)if kind==0 else 0xffffffff)
        put(at+12,index<<(18 if kind==2 else 9))
        if kind==0:
            put(0x4f0414,(TAGS-16*datum)&0xffffffff);put(MODEL+0x28,(SECTIONS-92*index)&0xffffffff)
            section,offset,header=SECTIONS,0x34,SECTIONS+0x38
        elif kind==1:
            put(BSP+0xa0,(CLUST-176*index)&0xffffffff);section,offset,header=CLUST,0x50,CLUST+0x28
        else:
            put(BSP+0x144,(INST-88*index)&0xffffffff);put(INST+0x34,definition,2)
            put(BSP+0x13c,(DEFS-200*definition)&0xffffffff);section,offset,header=DEFS,0x50,DEFS+0x28
        put(section+offset,0x12340000+row['id']);put(header+0x1e,int(resident),1);put(header+0x20,0xabcd0001)
        put(0x4f2d6c,FRAME);put(FRAME+0x64,META);put(FRAME+0x38,0xb16b00b5);put(META+0x44,STAMPS)
        finish()
    for kind,path in itertools.product(range(3),('offset-refusal','publish','inflight-refusal')):
        row,put,finish=new(0x449d0);row['gp'][2]=1;row['matrix']=int(kind==2);row['resource_path']=path
        put(0x4c695c,0 if kind==0 else 0xffffffff);put(0x4c6964,0)
        section=(SECTIONS,CLUST,DEFS)[kind];geometry=section+(0x34 if kind==0 else 0x50);header=section+(0x38 if kind==0 else 0x28)
        put(geometry,0);put(header+8,16);put(header+0xc,0xffffffff if path=='offset-refusal'else 0)
        put(header+0x18,0,2);put(header+0x1c,-8 if kind==0 else 0x24,2);put(header+0x20,0xabcd0001)
        put(TAGS,0x65646f6d if kind==0 else 0x73627370) # mode / sbsp
        flight=DATA+0xf000;flight_rows=flight+0x100
        put(0x4f2d6c,FRAME);put(FRAME+0x64,META);put(FRAME+0x38,0xb16b00b5);put(META+0x44,STAMPS)
        put(0x4f2d5c,flight);put(flight+0x44,flight_rows);put(flight_rows+12+2,int(path=='publish'),1);put(flight_rows+12+3,7,1)
        put(0x4f2d64,DATA+0xf800);put(0x4751ac,int(path=='inflight-refusal'),1)
        finish()
    for entry,mode,shape,flags in itertools.product((0x41c10,0x41c70),(0,1,2,0x10000,0x12340001),(0,1,2),(0,0x8001,0xffff)):
        row,put,finish=new(entry);row['matrix']=mode;row['source']=0xabcd0000;row['flags']=flags;put(base.SP+12,flags)
        if shape>=1:
            put(BODY+0x10,0xbeef0001);put(BODY+0xc,0xcafe0002);put(BODY+0x100+0xc,0x12340003)
        if shape==2:
            put(BODY+0x100+0x10,0x56780004);put(BODY+0x400+0xc,0x10000005)
        context=DATA+0xe000;put(context+0x14,QUEUE);row['gp'][7]=context;row['gp'][3]=mode
        put(QUEUE+4,2,2);put(QUEUE+8,QUEUE+0x100);put(QUEUE+12,QUEUE+0x200)
        put(QUEUE+0x100,flags,2);put(QUEUE+0x102,0x8001,2);put(QUEUE+0x200,0xabcd0000);put(QUEUE+0x204,0xefef0006)
        finish()
    for count in (-32768,-1,0):
        row,put,finish=new(0x41c70);row['gp'][7]=DATA+0xe000;put(DATA+0xe014,QUEUE);put(QUEUE+4,count,2);finish()
    return rows


def expected_calls(gold):
    out=[]
    for c in gold['calls']:
        e=c['entry'];gp=c['gp'];args=c['arguments']
        if e==0x130980:v=dict(entry=e,header=gp[1],flags=gp[0])
        elif e==0xd7230:v=dict(entry=e,object=gp[0],mode=gp[3],flags=args[0])
        else:v=dict(entry=e,mode=args[0],object=args[1])
        out.append(v)
    return out


def compare(rows,proof,native,meta):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);digest=hashlib.sha256()
    parent_sites={s['instruction']for s in meta['sites']if 0x41c10<=s['instruction']<=0x41ca5}
    with (scratch/'native-errors.log').open('w+')as errors:
        p=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                assert gold['completed'];p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line);expected=dict(result=gold['boundary_gp'][0]if row['entry']==0x449d0 else 0,
                    calls=expected_calls(gold),fp=gold['after_fp'],banks=[b for b in gold['banks']if b['address']!=STACK])
                for key,value in expected.items():
                    if actual[key]!=value:raise ValueError(row['name']+' differs in '+key+': '+str(actual[key])[:700]+' vs '+str(value)[:700])
                reads=[[r['address'],r['bytes']]for r in gold['reads']if not STACK<=r['address']<STACK+0x9000 and (row['entry']==0x449d0 or r['instruction']in parent_sites)]
                if actual['reads']!=reads:raise ValueError(row['name']+' changed retained read order '+str(actual['reads'])[:1600]+' vs '+str(reads)[:1600])
                writes=[[w['address'],len(w['written'])//2]for w in gold['writes']if not STACK<=w['address']<STACK+0x9000]
                if actual['writes']!=writes:raise ValueError(row['name']+' changed retained stores')
                digest.update(json.dumps(expected,sort_keys=True).encode())
            p.stdin.close()
            if p.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if p.poll()is None:p.terminate();p.wait(timeout=5)
    return digest.hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    base.SECTIONS_MAP={'units':(0x4c6000,0x2000),'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'resources':(0x4f2000,4096),'policy':(0x475000,4096),
        'data':(DATA,0x10000),'stack':(STACK,0x9000)}
    meta=base.prepare(image,scratch,(0x449d0,0x130980,0x22a680,0x41c70,0x41c10,0xd7230,0x41970,0x32a660))
    meta['trace_calls']=[dict(entry=e,arguments=1 if e==0xd7230 else 2 if e==0x41970 else 0,gp=True)for e in (0x130980,0xd7230,0x41970)]
    rows=cases();(scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=240)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest=compare(rows,proof,a.native,meta)
    report=dict(passed=True,physical_cases=len(rows),geometry_cases=sum(r['entry']==0x449d0 for r in rows),original_instructions=proof['original_instructions'],
        comparison_sha256=digest,original_xbe_sha256=base.PIN,owners=meta['owners'],instruction_patches=proof['instruction_patches'],
        supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        resource_paths=['resident stamp','size refusal','negative-offset refusal','publish geometry during real130980/22A680','nonblocking in-flight refusal and sbsp counter'],
        scope='Whole449D0 with genuine130980/22A680 versus typed admission plus existing resource request/publication kernels on declared no-IO-service paths: exact retained reads/stores, complete persistent bytes, FP and named call arguments. Whole41C10/41C70 with genuine inactive D7230/41970 services: exact parent reads, full datum/mode/flags calls and complete persistent bytes/FP. Controlled native tests independently exercise live mutations. Acquisition/promotion/pump paths and full native130980 are not qualified; World and GPU are separate gates.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical preparation/admission cases');return 0


if __name__=='__main__':raise SystemExit(main())
