#!/usr/bin/env python3
"""Physical unchanged130980 policy/publication and explicit service prefixes.

Prefixes stop before genuine acquisition, promotion or yield, never resume,
and never supply a callee outcome. Whole cases include complete22A680 fixups.
Real service execution is a separate World/live-runtime qualification gate.
Use run_test.py; all ELFs, raw banks and compiler outputs stay in scratch.
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
DATA=0x1000000;H=DATA+0x100;FRAME=DATA+0x1000;META=DATA+0x2000;RECORDS=DATA+0x3000
FLIGHT=DATA+0x4000;ROWS=DATA+0x5000;TAGS=DATA+0x6000;ALLOCATION=DATA+0x8000;DESC=DATA+0xa000
STACK=0x62010000;BOUNDARIES=[0x130eb0,0x1236a0,0x2dbf74,0x128870]


def cases():
    rows=[]
    def new(entry=0x130980):
        row=dict(id=len(rows),name=f'{entry:x}-{len(rows)}',entry=entry,gp=[0,H,0,0,base.SP,0,0,H],
            matrix=ALLOCATION,source=0,before_fp=[0x27f,0x220,0x1fa0],live_x87=[],max_steps=50000)
        if entry==0x130980:row['stop_before_entries']=BOUNDARIES
        banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
        def put(at,v,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped resource input '+hex(at))
        put(0x4f0414,TAGS);put(0x4f2d6c,FRAME);put(FRAME+0x64,META);put(META+0x44,RECORDS);put(FRAME+0x38,0xabcd1234)
        put(0x4f2d5c,FLIGHT);put(FLIGHT+0x44,ROWS);put(0x4f2d64,ALLOCATION)
        put(H+8,16);put(H+0x20,1);put(H+0x1c,0x24,2);put(ROWS+14,1,1);put(ROWS+15,7,1)
        def finish():row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
        return row,put,finish
    flags=(*range(16),0x10003,0xffff0000,0xffffffff)
    for f,group,policy,fil,disabled in itertools.product(flags,(0x65646f6d,0x73627370,0x6c746d70),(-1,0,1,2),(0,1,2),(0,1)):
        row,put,finish=new();row['gp'][0]=f;row['kind']='no-token-policy'
        put(H+0x20,0xffffffff);put(TAGS,group);put(0x4f2d70,policy);put(0x4f2d74,fil);put(0x4751ac,disabled,1);finish()
    for f,group,policy,disabled,ready,priority in itertools.product(range(16),(0x65646f6d,0x73627370),(0,1),(0,1),(0,1),(0,255)):
        row,put,finish=new();row['gp'][0]=f;row['kind']='inflight-policy'
        put(TAGS,group);put(0x4f2d70,policy);put(0x4f2d74,1);put(0x4751ac,disabled,1)
        put(ROWS+14,ready,1);put(ROWS+15,priority,1);put(ROWS+16,0x1234);finish()
    for f,size,offset in itertools.product((0,3,15),(-1,0,1),(-1,0)):
        if size>0 and offset>=0:continue
        row,put,finish=new();row['gp'][0]=f;row['kind']='invalid';put(H+8,size);put(H+0xc,offset);finish()
    for hint in (False,True):
        row,put,finish=new();row.pop('stop_before_entries');row['kind']='genuine-acquire-refusal'
        row.update(genuine_acquire_failure=True,limit_hint=hint,physical=FRAME);row['gp'][0]=3
        put(H+4,0x3000);put(H+0x20,0xffffffff);put(TAGS,0x73627370)
        put(FRAME+0x30,0 if hint else 0x40);put(FRAME+0x34,12);put(FRAME+0x3c,0xffffffff);put(FRAME+0x40,0xffffffff)
        for i in range(8):put(FRAME+0x44+4*i,3 if i==1 and not hint else 0x100)
        put(FRAME+0x64,META);put(META+0x20,16);put(META+0x24,24);put(META+0x44,RECORDS);put(META+0x48,META+0x80)
        finish()
    for kind,stride,negative,index,absent in itertools.product((0,1,2,3,255),(1,7,20,255),(False,True),(65535,3),(False,True)):
        row,put,finish=new(0x22a680);row['kind']='publication';put(H+0x10,1);put(H+0x14,DESC)
        put(DESC,kind,1);put(DESC+4,0x80,2);put(DESC+6,stride if kind<=1 else index,2)
        put(DESC+8,0xffffffe7 if negative else 103);put(DESC+12,0xffffffff if absent else 0x200)
        put(ALLOCATION+8+0x80+4,ALLOCATION+0x800)
        buffer=ALLOCATION+8+0x80 if index==65535 else ALLOCATION+0x800+index*32
        put(buffer+1,stride,1);put(buffer+4,0xe1234567);put(buffer+8,7);finish()
    for count,root in itertools.product((0,-1,-2147483648),(-4,0x24)):
        row,put,finish=new(0x22a680);row['kind']='publication-signed-count';put(H+0x10,count);put(H+0x1c,root,2);finish()
    # Publishing the root overwrites the descriptor count/pointer. The source
    # rereads both from the newly published bytes rather than the old header.
    row,put,finish=new(0x22a680);row['kind']='publication-alias';put(H+0x1c,0x10,2)
    put(ALLOCATION+8,0,1);put(ALLOCATION+8+4,0x80,2);put(ALLOCATION+8+6,7,2)
    put(ALLOCATION+8+8,103);put(ALLOCATION+8+12,0x200);finish()
    return rows


def word(banks,at):
    for b in banks:
        raw=bytes.fromhex(b['after'])
        if b['address']<=at and at+4<=b['address']+len(raw):return struct.unpack_from('<I',raw,at-b['address'])[0]
    raise ValueError('Unmapped original service argument')


def compare(rows,proof,native):
    digest=hashlib.sha256();counts={};prefixes={}
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native-errors.log').open('w+')as errors:
        p=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                assert gold['completed'];p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line);entry=gold.get('stopped_at',base.STOP);prefix=entry!=base.STOP;calls=[]
                for call in gold['calls']:
                    if call['entry']==0x130eb0:
                        args=call['arguments'];calls.append(dict(entry=call['entry'],arguments=[args[0],call['gp'][0]&255,args[1]&255,args[2]]))
                if prefix:
                    gp=gold['boundary_gp'];sp=gp[4]
                    if entry==0x130eb0:args=[word(gold['banks'],sp+4),gp[0]&255,word(gold['banks'],sp+8)&255,word(gold['banks'],sp+12)]
                    elif entry==0x1236a0:args=[gp[6],word(gold['banks'],sp+4)]
                    else:args=[]
                    calls=[dict(entry=entry,arguments=args)];prefixes[hex(entry)]=prefixes.get(hex(entry),0)+1
                else:counts[row['kind']]=counts.get(row['kind'],0)+1
                expected=dict(result=bool(gold['boundary_gp'][0]&255)if row['entry']==0x130980 and not prefix else False,
                    stopped=prefix,calls=calls,fp=gold['after_fp'],banks=[b for b in gold['banks']if b['address']!=STACK])
                for key,value in expected.items():
                    if actual[key]!=value:raise ValueError(row['name']+' differs in '+key+': '+str(actual[key])[:700]+' vs '+str(value)[:700])
                reads=[[r['address'],r['bytes']]for r in gold['reads']if not STACK<=r['address']<STACK+0x9000 and
                    (not row.get('genuine_acquire_failure')or 0x130980<=r['instruction']<=0x130bce)]
                if actual['reads']!=reads:raise ValueError(row['name']+' changed retained read order '+str(actual['reads'])[:2000]+' vs '+str(reads)[:2000])
                writes=[[w['address'],len(w['written'])//2]for w in gold['writes']if not STACK<=w['address']<STACK+0x9000]
                if actual['writes']!=writes:raise ValueError(row['name']+' changed retained store order '+str(actual['writes'])+' vs '+str(writes))
                digest.update(json.dumps(expected,sort_keys=True).encode())
            p.stdin.close()
            if p.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if p.poll()is None:p.terminate();p.wait(timeout=5)
    return digest.hexdigest(),counts,prefixes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    base.SECTIONS_MAP={'tags':(0x4f0000,4096),'globals':(0x4f2000,4096),'policy':(0x475000,4096),
        'data':(DATA,0x10000),'stack':(STACK,0x9000)}
    meta=base.prepare(image,scratch,(0x130980,0x22a680,0x130eb0,0x13fe50,0x32a660));rows=cases()
    meta['trace_calls']=[dict(entry=0x130eb0,arguments=3,gp=True)]
    (scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=240)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest,counts,prefixes=compare(rows,proof,a.native)
    report=dict(passed=True,physical_cases=len(rows),whole_cases=counts,service_prefix_cases=prefixes,
        original_instructions=proof['original_instructions'],comparison_sha256=digest,original_xbe_sha256=base.PIN,owners=meta['owners'],
        instruction_patches=proof['instruction_patches'],supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        scope='Exact retained reads/stores, full non-stack banks, FP, results and boundary arguments across TU1.5 policy combinations and complete22A680 descriptor fixups. Service prefixes stop before the genuine call and are not whole service/IO execution. Genuine World and live resource acceptance are separate.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical whole/prefix resource cases',counts,prefixes);return 0


if __name__=='__main__':raise SystemExit(main())
