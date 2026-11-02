#!/usr/bin/env python3
"""Whole unchanged TU1.5 render section/object/lighting resource admission.

Use run_test.py. Physical services run genuine130980/22A680 and BCDF0.
Acquisition/promote/yield prefixes stop before the real service, never resume
and never supply an original callee outcome. Raw banks and ELFs stay scratch.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
D=0x1000000;LM=D+0x100;GROUP=D+0x200;REFS=D+0x400;SCENE=D+0x500;CHECKS=D+0x600
BUCKETS=D+0x800;BSP=D+0x1000;OTHER=D+0x2000;INST=D+0x3000;ALTINST=D+0x4000
TAGS=D+0x5000;MODEL=D+0x6000;PHYS=D+0x7000;META=D+0x8000;RECORDS=D+0x9000
FLIGHT=D+0xa000;ROWS=D+0xb000;POOL=D+0xc000;OBJROWS=D+0xd000;BODY=D+0xe000
NONE=0xffffffff;OWNER=0x80010002;MODEL_ID=0xabcd0001;STACK=0x62010000
BOUNDARIES=[0x130eb0,0x1236a0,0x2dbf74,0x128870]


def cases():
    rows=[]
    def new(entry=0x2cfe0,instance=1):
        row=dict(id=len(rows),name=f'{entry:x}-{len(rows)}',kind='lighting',entry=entry,
            gp=[OWNER,instance,MODEL_ID,0,base.SP,0,0,0],matrix=0,source=0,
            before_fp=[0x27f,0x220,0x1fa0],live_x87=[],max_steps=50000)
        if entry==0x2cfe0:row['stop_before_entries']=BOUNDARIES
        banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
        def put(at,v,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped render-lighting input '+hex(at))
        put(0x4ecc14,LM);put(0x4ecc18,BSP);put(0x4f0414,TAGS);put(0x4ecbd0,POOL)
        put(LM+0x80,1);put(LM+0x84,GROUP);put(GROUP+0x1c,0);put(GROUP+4,0x12345678);put(BSP+8,0x12345678)
        put(GROUP+0x54,REFS);put(GROUP+0x5c,CHECKS);put(GROUP+0x64,SCENE);put(GROUP+0x44,BUCKETS)
        put(BSP+0x144,INST);put(OTHER+0x144,ALTINST);put(BSP+0xa0,D+0x10000);put(BSP+0x13c,D+0x11000)
        put(TAGS,0x6c746d70);put(TAGS+16+8,MODEL);put(MODEL+8,0x87654321);put(MODEL+0x28,D+0x12000)
        if instance!=0x7ff:
            put(CHECKS+12*instance+8,0x87654321);put(INST+88*instance+0x56,1,2)
        put(POOL+0x38,16);put(POOL+0x24,12);put(POOL+0x44,OBJROWS);put(OBJROWS+24,0x8001,2);put(OBJROWS+24+8,BODY);put(BODY+7,1,1)
        put(0x4f2d6c,PHYS);put(PHYS+0x64,META);put(META+0x44,RECORDS);put(PHYS+0x38,0xc10c1234)
        put(0x4f2d5c,FLIGHT);put(FLIGHT+0x44,ROWS);put(0x4f2d64,D+0xf000)
        h=BUCKETS+0xc;put(h+8,16);put(h+0x1c,0x24,2);put(h+0x1e,1,1);put(h+0x20,1)
        put(ROWS+14,1,1);put(ROWS+15,255,1)
        def finish():row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
        return row,put,finish
    def owner(put,row,variant):
        if variant=='none':row['gp'][0]=NONE
        elif variant=='past':put(POOL+0x38,2)
        elif variant=='negative-count':put(POOL+0x38,0x80000000)
        elif variant=='zero-salt':put(OBJROWS+24,0,2)
        elif variant=='stale-salt':put(OBJROWS+24,0x8002,2)
        elif variant=='clear-body':put(BODY+7,0,1)
        elif variant=='null-body':put(OBJROWS+24+8,0)
    gates=('no-lightmaps','zero-groups','negative-groups','no-bsp','no-bitmap','checksum')
    owners=('valid','none','past','negative-count','zero-salt','stale-salt','clear-body','null-body')
    for gate,model,variant in itertools.product(gates,(NONE,MODEL_ID),owners):
        row,put,finish=new();row['kind']='gate-'+gate;row['gp'][2]=model;owner(put,row,variant)
        if gate=='no-lightmaps':put(0x4ecc14,0)
        elif gate=='zero-groups':put(LM+0x80,0)
        elif gate=='negative-groups':put(LM+0x80,0x80000000)
        elif gate=='no-bsp':put(0x4ecc18,0)
        elif gate=='no-bitmap':put(GROUP+0x1c,NONE)
        elif gate=='checksum':put(GROUP+4,0)
        finish()
    for model,index,policy,phase in itertools.product((NONE,MODEL_ID),(0,1,0x7ff,0x107ff),(0,0xffff),('resident','size-refusal','offset-refusal','publish','inflight-refusal')):
        row,put,finish=new(instance=index);row['kind']='request-'+phase;row['gp'][2]=model
        if index!=0x7ff:put(INST+88*index+0x56,policy,2)
        h=BUCKETS+0xc
        if phase!='resident':put(h+0x1e,0,1)
        if phase=='size-refusal':put(h+8,0)
        elif phase=='offset-refusal':put(h+0xc,NONE)
        elif phase=='inflight-refusal':put(0x4751ac,1,1);put(ROWS+14,0,1)
        finish()
    for variant in owners:
        row,put,finish=new();row['kind']='model-checksum-refusal';owner(put,row,variant);put(MODEL+8,0);finish()
    for model in (0x1234ffff,0xffff0000,0xfffffffe):
        row,put,finish=new(instance=0x7ff);row['kind']='full-model-datum-sentinel';row['gp'][2]=model;finish()
    for group,disabled,policy,filter_,model in itertools.product((0x6c746d70,0x73627370,0x65646f6d),(0,1),(0,1,2),(0,1,2),(NONE,MODEL_ID)):
        row,put,finish=new();row['kind']='acquire-policy-prefix';row['gp'][2]=model;h=BUCKETS+0xc
        put(h+0x1e,0,1);put(h+0x20,NONE);put(TAGS,group);put(0x4751ac,disabled,1);put(0x4f2d70,policy);put(0x4f2d74,filter_);finish()
    for change_root in (False,True):
        row,put,finish=new();row['kind']='publication-replaces-bsp-root'if change_root else'publication-replaces-instance-table';row['gp'][2]=NONE
        h=0x4ec40c if change_root else BUCKETS+0xc
        if change_root:put(GROUP+0x44,h-0xc)
        put(h+8,16);put(h+0x1c,(0x4ecc14 if change_root else BSP+0x140)-h,2);put(h+0x1e,0,1);put(h+0x20,1)
        put(INST+88+0x56,0,2);put(ALTINST+88+0x56,1,2);put(0x4f2d64,(OTHER if change_root else ALTINST)-8)
        finish()
    # Original reads the unbound unsigned bucket address; it never substitutes3.
    for model in (NONE,MODEL_ID):
        row,put,finish=new();row['kind']='unbound-bucket';row['gp'][2]=model
        put((REFS if model==NONE else SCENE)+12+2,65535,2);finish()
    for variant,kind,mask in itertools.product(owners,(0,31,32,255),(0,1,0x80000000,NONE)):
        row,put,finish=new(0xbcdf0);row['kind']='object';row['matrix']=mask;owner(put,row,variant);put(OBJROWS+24+3,kind,1);finish()
    for unit,path,selector,definition in itertools.product((0,1,0x7fff,0x8000,0x1ffff,0xffff8000),('model','cluster','instance'),(0,1,0x100,0xff01),(0,-1,-32768)):
        if path!='instance'and definition:continue
        row,put,finish=new(0x44930);row['kind']='section-'+path;row['gp'][0]=unit;row['matrix']=selector
        low=unit&65535;at=(0x4c6958+32*(low if low<32768 else low-65536))&NONE
        put(at+4,MODEL_ID if path=='model'else NONE);put(at+0xc,(1<<18)|(511<<9))
        if path=='instance':put(INST+88+0x34,definition,2)
        finish()
    return rows


def word(banks,at):
    for b in banks:
        data=bytes.fromhex(b['after'])
        if b['address']<=at and at+4<=b['address']+len(data):return int.from_bytes(data[at-b['address']:at-b['address']+4],'little')
    raise ValueError('Unmapped boundary argument '+hex(at))


def compare(rows,proof,native):
    digest=hashlib.sha256();counts={};prefixes={}
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native-errors.log').open('w+')as errors:
        p=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                assert gold['completed'];p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line);entry=gold.get('stopped_at',base.STOP);prefix=entry!=base.STOP;calls=[]
                if row['entry']==0x2cfe0:
                    for call in gold['calls']:
                        if call['entry']==0x130980:calls.append(dict(entry=call['entry'],arguments=[call['gp'][0],call['gp'][1]]))
                        elif call['entry']==0xbcdf0:calls.append(dict(entry=call['entry'],arguments=[call['gp'][0],call['arguments'][0]]))
                if prefix:
                    gp=gold['boundary_gp'];sp=gp[4]
                    if entry==0x130eb0:args=[word(gold['banks'],sp+4),gp[0]&255,word(gold['banks'],sp+8)&255,word(gold['banks'],sp+12)]
                    elif entry==0x1236a0:args=[gp[6],word(gold['banks'],sp+4)]
                    else:args=[]
                    calls.append(dict(entry=entry,arguments=args));prefixes[hex(entry)]=prefixes.get(hex(entry),0)+1
                else:counts[row['kind']]=counts.get(row['kind'],0)+1
                expected=dict(result=0 if prefix else gold['boundary_gp'][0],stopped=prefix,calls=calls,fp=gold['after_fp'],
                    banks=[b for b in gold['banks']if b['address']!=STACK])
                for key,value in expected.items():
                    if actual[key]!=value:raise ValueError(row['name']+' '+row['kind']+' differs in '+key+': '+str(actual[key])[:900]+' vs '+str(value)[:900])
                reads=[[r['address'],r['bytes']]for r in gold['reads']if not STACK<=r['address']<STACK+0x9000]
                writes=[[w['address'],len(w['written'])//2]for w in gold['writes']if not STACK<=w['address']<STACK+0x9000]
                if actual['reads']!=reads:raise ValueError(row['name']+' read order '+str(actual['reads'])[:2000]+' vs '+str(reads)[:2000])
                if actual['writes']!=writes:raise ValueError(row['name']+' store order '+str(actual['writes'])+' vs '+str(writes))
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
    high=(REFS+12*0x107ff)&~4095;high_instance=(INST+88*0x107ff)&~4095
    base.SECTIONS_MAP={'negative_units':(0x3c6000,8192),'units':(0x4c6000,8192),'positive_units':(0x5c6000,8192),
        'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'resources':(0x4f2000,4096),'policy':(0x475000,4096),
        'data':(D,0x10000),'high_refs':(high,8192),'high_instances':(high_instance,4096),
        'unbound_bucket':(D+0x380000,4096),'stack':(STACK,0x9000)}
    meta=base.prepare(image,scratch,(0x2cfe0,0x44930,0xbcdf0,0x130980,0x22a680))
    meta['trace_calls']=[dict(entry=0x130980,arguments=0,gp=True),dict(entry=0xbcdf0,arguments=1,gp=True)]
    rows=cases();(scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=240)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest,counts,prefixes=compare(rows,proof,a.native)
    report=dict(passed=True,physical_cases=len(rows),whole_cases=counts,service_prefix_cases=prefixes,original_instructions=proof['original_instructions'],
        comparison_sha256=digest,original_xbe_sha256=base.PIN,owners=meta['owners'],instruction_patches=proof['instruction_patches'],
        supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        scope='Whole44930, BCDF0 and2CFE0 through genuine resource policy/publication on no-IO paths; exact retained reads/writes, full persistent banks, return values, named calls and x87 control/status/MXCSR. Two genuine22A680 publications replace the current BSP root or instance table. Acquisition prefixes stop before130EB0 and never supply or resume its outcome.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'render section/object/lighting cases',counts,prefixes)
    return 0


if __name__=='__main__':raise SystemExit(main())
