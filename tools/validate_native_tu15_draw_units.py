#!/usr/bin/env python3
"""Compare unchanged TU1.5 draw-unit banks against native owners; run_test.py owns outputs.

The typed append returns consumed AX. Volatile GP/EFLAGS and private caller-stack
aliasing are outside this contract. Whole30270 includes EAX/ECX residues,
epilogue flags, ordered diagnostic stores and guest FP. Resource prefixes stop
before the unresolved service and neither resume nor supply its result.
"""
import argparse
import copy
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base
from validate_native_tu15_render_lighting import word
from native_tu15_sphere_rectangle_cases import append_cases as append_rectangle_cases
from native_tu15_light_parent_cases import append_cases as append_parent_cases

ROOT=Path(__file__).resolve().parents[1]
D=0x1000000;TAGS=D;MODEL=D+0x100;SECTIONS=D+0x1000;OBJ=D+0x3000;GEOM=D+0x4000
BSP=D+0x5000;CLUSTERS=D+0x6000;INST=D+0x7000;DEFINITIONS=D+0x8000;CENTER=D+0x9000
CTX=0x5598f8;NONE=0xffffffff;MODEL_ID=0xabcd0001;BOUNDARIES=[0x130eb0,0x1236a0,0x2dbf74,0x128870]

def f2u(v):return struct.unpack('<I',struct.pack('<f',v))[0]

def cases(image):
    rows=[]
    def new(entry=0x2cbd0,count=7,used=0,mode=0,flags=0x42,geometry=GEOM,subparts=40,
            coverage=f2u(3.5),context=0,override=NONE,kind='model',section=31,outer=0,inner=0,
            fade_a=f2u(.5),fade_b=f2u(.25),primary=1,override_count=0):
        banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
        def put(at,v,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped draw-unit input '+hex(at))
        for b in banks:
            if b['address']in(0x449000,0x44a000,0x456000,0x46a000):b['before'][:]=image.read(b['address'],len(b['before']))
        put(0x4cd3a0,CTX);put(0x4cd398,count);put(0x4cd39c,3);put(0x4cdd48,override_count)
        put(CTX+0x2a78,mode);put(CTX+0xa5c,used,2);put(0x4f0414,TAGS)
        put(TAGS+16+8,MODEL);put(MODEL+0x28,SECTIONS)
        put(0x4ecc18,BSP);put(BSP+0xa0,CLUSTERS);put(BSP+0x144,INST);put(BSP+0x13c,DEFINITIONS)
        put(0x4f2d6c,D+0xa000);put(D+0xa000+0x64,D+0xb000);put(D+0xb000+0x44,D+0xc000)
        put(D+0xa000+0x38,0xc10c1234)
        put(INST+88*3+0x34,1,2)
        for sec,meta,geom_offset,header in [(SECTIONS+92*5,4,0x34,0x38),(CLUSTERS+176*5,0,0x50,0x28),(DEFINITIONS+200,0,0x50,0x28)]:
            put(sec+meta+0x14,2,2);put(sec+meta+0x24,subparts,2);put(sec+geom_offset,geometry)
            if meta==0:
                h=sec+header;put(h+8,16);put(h+0x1c,0x24,2);put(h+0x1e,1,1);put(h+0x20,1)
        if context:put(context,NONE)
        unit=0x4c6958+32*(count if count<0x80000000 else count-0x100000000)
        if 0<=count<850:
            for i in range(8):put(unit+4*i,0xa5a5a5a5)
            put(0x4cd3a4+count,0x5a,1);put(0x4cd6f6+count,0x5a,1)
        for i,v in enumerate((1.,2.,3.)):put(CENTER+4*i,f2u(v))
        model=MODEL_ID if kind=='model'else NONE
        flags=(flags&~0x1000)|(0x1000 if kind=='instance'else 0)
        args=[flags,model,0x11112222,7,context,0x1234,0xc8,0x22223333,override,coverage,section,
            0x12345678,outer,inner,primary,fade_a,fade_b,f2u(2.),9,CENTER,f2u(.5)]
        for i,a in enumerate(args[2:],2):put(base.SP+4+4*i,a)
        row=dict(id=len(rows),name=f'{entry:x}-{len(rows)}',kind=kind,entry=entry,
            gp=[5,3,0x12,0,base.SP,0,0,0],matrix=args[0],source=args[1],arguments=args,
            stop_pc=base.STOP,stop_before_entries=BOUNDARIES,max_steps=50000,
            before_fp=[0x27f,0,0x1f80],live_x87=[])
        def finish():row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
        return row,put,finish
    # Overlapping forward copy, mode reuse and untouched bytes.
    for mode in (0,1,2,NONE):
        row,put,finish=new(0x2cb10,mode=mode)
        for i in range(288):put(0x4cdd4c+4*i,0xa5a50000+i)
        finish()
    for count,used in itertools.product((0,1,31,32,33,16352,16353,16384,16385,65535,0x7fffffff,0x80000000,NONE),(0,480,511,512,513,65535)):
        row,put,finish=new(0x135dc0,used=used);row['gp'][1:3]=[count,CTX];finish()
    values=[0,0x80000000,1,0x80000001,f2u(3.),f2u(3.5),f2u(-.5),f2u(255.75),
        0x4f000000,0xcf000000,0x7fc00000,0x7f800001,0x7f800000,0xff800000,f2u(1e30),f2u(-1e30)]
    for value,fp in itertools.product(values,([0x27f,0,0x1f80],[0x37f,0x4500,0x1f80],[0x67f,0,0x3f80])):
        row,put,finish=new(0x16ac0);row['matrix']=value;row['before_fp']=fp;finish()
    for kind,mode,flags,subparts,used in itertools.product(('model','cluster','instance'),(0,1),(2,0x42),(0,40,16385),(0,511)):
        row,put,finish=new(kind=kind,mode=mode,flags=flags,subparts=subparts,used=used);finish()
    for value in values+[f2u(2**33),f2u(1023.9)]:
        row,put,finish=new(coverage=value);row['kind']='coverage';finish()
    for primary,value in itertools.product((0,1),values+[f2u(1e10)]):
        row,put,finish=new(context=OBJ,override=0x77,primary=primary,fade_a=value,fade_b=value)
        row['kind']='fade';finish()
    for count,geometry in ((849,GEOM),(850,GEOM),(7,0),(7,NONE)):
        row,put,finish=new(count=count,geometry=geometry);row['kind']='refusal-or-limit';finish()
    row,put,finish=new();row['gp'][0]=NONE;row['kind']='full-geometry-sentinel';finish()
    for section,outer,inner,slot in itertools.product((0,8,31),(0,1),(0,1),(0,191,192)):
        row,put,finish=new(override=0xdeadbeef,section=section,outer=outer,inner=inner,override_count=slot)
        row['kind']='section-rectangle';finish()
    for kind in ('cluster','instance'):
        for phase in ('zero-size','acquire'):
            row,put,finish=new(kind=kind);row['kind']=kind+'-'+phase
            h=(CLUSTERS+176*5 if kind=='cluster'else DEFINITIONS+200)+0x28
            put(h+0x1e,0,1)
            if phase=='zero-size':put(h+8,0)
            else:
                put(0x4f2d6c,D+0xa000);put(D+0xa000+0x64,D+0xb000);put(D+0xb000+0x44,D+0xc000)
                put(D+0xa000+0x38,0xc10c1234);put(0x4f2d70,0);put(TAGS,0x6c746d70)
                put(h+0x20,NONE)
            finish()
    for queue_flags,classification,fraction,planar in itertools.product((0,0xffff,0x400,0x200),(0,3),(0,1,0x7fc00000),(0,1)):
        row,put,finish=new(0x135e00);row['gp'][2]=queue_flags;row['matrix']=CTX
        put(CTX+0x2a60,0x80,1);put(0x4c6710,classification);put(0x4c67ac,MODEL_ID)
        put(0x4c681c,fraction);put(0x4c6824,planar,1);finish()
    def collector(entry,count=2,scale=f2u(50),fp=None,radius=f2u(.5),width=f2u(1)):
        row,put,finish=new(entry);row['gp'][3]=CTX;row['matrix']=CTX;row['before_fp']=fp or [0x27f,0,0x1f80]
        queue=D+0xd000;flags=D+0xd100;ids=D+0xd200;associations=D+0xd300
        put(CTX+(0xc if entry==0x135ea0 else 0x18),queue);put(queue+4,count,2)
        put(queue+8,flags);put(queue+0xc,ids);put(queue+0x14,associations)
        for i in range(2):
            put(flags+2*i,0x800 if i else 0x400,2);put(ids+4*i,5 if entry==0x135ea0 else 3)
            put(associations+2*i,0x1234+i,2)
        put(CTX+0x1c+5,0xfe,1)
        for offset,value in ((0x54,-1.),(0x58,1.),(0x5c,-1.),(0x60,1.),(0x64,2.),(0x68,4.)):
            put(CLUSTERS+176*5+offset,f2u(value))
        put(CLUSTERS+176*5+0x58,width)
        for offset,value in ((0x3c,1.),(0x40,2.),(0x44,3.),(0x48,.5)):
            put(INST+88*3+offset,f2u(value))
        put(INST+88*3+0x48,radius)
        put(0x4c6658,f2u(1.));put(0x4c66f0,scale);put(0x474ce8,CENTER)
        return row,put,finish
    for entry,count,scale,fp in itertools.product((0x135ea0,0x136030),(0,1,2,65535),(f2u(1),f2u(50)),
            ([0x27f,0,0x1f80],[0x37f,0x4500,0x1f80])):
        row,put,finish=collector(entry,count,scale,fp);finish()
    for entry,value in itertools.product((0x135ea0,0x136030),values+[f2u(.129),f2u(.13),f2u(.131)]):
        row,put,finish=collector(entry,radius=value,width=value);row['kind']='collector-fp-edges';finish()
    for entry in (0x135ea0,0x136030):
        row,put,finish=collector(entry);row['kind']='collector-publication-replaces-owner'
        alternative=D+0xe000;flight=D+0xf000;flight_rows=D+0xf100
        put(0x4f2d5c,flight);put(flight+0x44,flight_rows);put(flight_rows+14,1,1)
        if entry==0x136030:
            sec=DEFINITIONS+200;h=sec+0x28;target=BSP+0x144;put(0x4f2d64,alternative-8)
            put(alternative+88*3+0x34,1,2);put(alternative+88*3+0x48,f2u(.75))
            put(alternative+88*3+0x44,f2u(5.))
        else:
            # First cluster publishes a different global BSP root; second
            # iteration must reload it before looking up cluster metadata.
            h=0x4ec40c;sec=h-0x28;put(BSP+0xa0,sec-176*5);target=sec+0x50
            put(sec+0x14,2,2);put(sec+0x24,1,2);put(sec+0x50,GEOM)
            for offset,value in ((0x54,-1.),(0x58,1.),(0x5c,-1.),(0x60,1.),(0x64,2.),(0x68,4.)):put(sec+offset,f2u(value))
            put(alternative+0xa0,D+0xf000)
            # Type0 publication writes a zero count to4ECC14 and its pointer
            # to4ECC18. Unlike a root count1 this leaves a valid lightmap gate
            # for the second iteration. The descriptor is genuine22A680 input.
            data=0x4ecb14;descriptor=D+0xf500;put(0x4f2d64,data-8)
            put(h+0x10,1);put(h+0x14,descriptor);put(descriptor+4,0x100,2)
            put(descriptor+6,1,2);put(descriptor+8,0);put(descriptor+0xc,alternative-data-16)
            second=D+0xf000+176*5;put(second+0x14,1,2);put(second+0x24,3,2);put(second+0x50,GEOM)
            put(second+0x28+0x1e,1,1);put(second+0x28+0x20,1)
            for offset,value in ((0x54,-2.),(0x58,2.),(0x5c,-1.),(0x60,1.),(0x64,2.),(0x68,4.)):put(second+offset,f2u(value))
        put(h+8,16);put(h+0x1e,0,1);put(h+0x20,1);put(h+0x1c,target-4-h,2)
        finish()
    append_rectangle_cases(new,CENTER,CENTER+0x100)
    append_parent_cases(new)
    for original in list(rows):
        if original['entry']!=0x2cbd0:continue
        row=copy.deepcopy(original);row.update(id=len(rows),name=original['name']+'-observed-stack',observe=True,argument_stack=True)
        rows.append(row)
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=base.PIN:raise ValueError('Wrong TU1.5 executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    base.SECTIONS_MAP={'alpha_factor':(0x449000,4096),'low':(0x44a000,4096),'high':(0x456000,4096),'constants':(0x46a000,4096),
        'view':(0x4c6000,0x9000),'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'resources':(0x4f2000,4096),
        'globals':(0x474000,4096),'policy':(0x475000,4096),'force':(0x515000,4096),'contexts':(0x559000,0x4000),
        'data':(D,0x10000),'stack':(0x62000000,0x20000)}
    meta=base.prepare(image,scratch,(0x2cb10,0x2cbd0,0x16ac0,0x135dc0,0x44930,0x449d0,0x2cfe0,0xbcdf0,0x130980,0x22a680,
        0x135e00,0x135ea0,0x136030,0x32cc0c,0x30270,0x31c10,0x166b60,0x31c70))
    meta['trace_calls']=[dict(entry=0x130980,arguments=0,gp=True),dict(entry=0xbcdf0,arguments=1,gp=True),dict(entry=0x30270,arguments=1,gp=True),dict(entry=0x166b60,arguments=4,gp=True)]
    rows=cases(image);(scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=240)
    if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest=hashlib.sha256();counts={};prefixes={}
    with (scratch/'native-errors.log').open('w+')as errors:
        proc=subprocess.Popen([str(a.native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                assert gold['completed'],row['name'];proc.stdin.write(json.dumps(row,separators=(',',':'))+'\n');proc.stdin.flush()
                line=proc.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line);entry=gold.get('stopped_at',base.STOP);prefix=entry!=row.get('stop_pc',base.STOP);calls=[]
                for call in gold['calls']:
                    if call['entry']==0x130980:calls.append(dict(entry=call['entry'],arguments=[call['gp'][0],call['gp'][1]]))
                    elif call['entry']==0xbcdf0:calls.append(dict(entry=call['entry'],arguments=[call['gp'][0],call['arguments'][0]]))
                    elif call['entry']==0x30270 and not row.get('native_rectangle_default'):calls.append(dict(entry=call['entry'],arguments=[call['gp'][0],call['gp'][6],call['arguments'][0]]))
                    elif call['entry']==0x166b60:calls.append(dict(entry=call['entry'],arguments=[call['gp'][1],*call['arguments'],call['gp'][0]]))
                if prefix:
                    gp=gold['boundary_gp'];sp=gp[4]
                    if entry==0x30270:args=[gp[0],gp[6],word(gold['banks'],sp+4)]
                    elif entry==0x130eb0:args=[word(gold['banks'],sp+4),gp[0]&255,word(gold['banks'],sp+8)&255,word(gold['banks'],sp+12)]
                    elif entry==0x1236a0:args=[gp[6],word(gold['banks'],sp+4)]
                    else:args=[]
                    calls.append(dict(entry=entry,arguments=args));prefixes[hex(entry)]=prefixes.get(hex(entry),0)+1
                else:counts[hex(row['entry'])]=counts.get(hex(row['entry']),0)+1
                result=0 if prefix or row['entry']in(0x2cb10,0x135ea0,0x136030,0x31ec2)else gold['boundary_gp'][0]&(65535 if row['entry']==0x2cbd0 else NONE)
                expected=dict(result=result,stopped=prefix,calls=calls,fp=gold['after_fp'],banks=[b for b in gold['banks']if b['address']!=0x62000000])
                if row['entry']==0x30270:
                    expected.update(ecx=gold['boundary_gp'][1],eflags=gold['boundary_eflags'],
                        x87_triples=sum(r['instruction']in(0x3040d,0x30588)for r in gold['reads']))
                    if row.get('observe'):expected['stores']=[w for w in gold['writes']if not 0x62000000<=w['address']<0x62020000]
                if row['entry']==0x166b60:expected.update(ecx=gold['boundary_gp'][1],edx=gold['boundary_gp'][2])
                if row.get('argument_stack'):
                    stack=next(b for b in gold['banks']if b['address']==0x62000000)
                    expected['stack_above']=stack['after'][(row['gp'][4]-stack['address'])*2:]
                    expected['stores']=[w for w in gold['writes']if not 0x62000000<=w['address']<row['gp'][4]]
                    if not prefix:expected['return_word']=gold['boundary_gp'][0]
                for key,value in expected.items():
                    if actual[key]!=value:
                        detail=dict(name=row['name'],kind=row['kind'],key=key,actual=actual[key]if key!='banks'else 'bank bytes',expected=value if key!='banks'else 'bank bytes')
                        if key=='banks':
                            for before,after in zip(actual[key],value,strict=True):
                                if before!=after:
                                    aa=bytes.fromhex(before['after']);bb=bytes.fromhex(after['after']);detail['bank']=before['address'];detail['different_bytes']=[[i,x,y]for i,(x,y)in enumerate(zip(aa,bb,strict=True))if x!=y][:40];break
                        raise ValueError(str(detail)[:3500])
                for key,trace in [('reads',[[r['address'],r['bytes']]for r in gold['reads']]),('writes',[[w['address'],len(w['written'])//2]for w in gold['writes']])]:
                    if key=='reads'and row.get('observe'):continue # declared observer reads/calls are outside30270
                    trace=[v for v in trace if not 0x62000000<=v[0]<0x62020000]
                    actual_trace=[v for v in actual[key]if not 0x62000000<=v[0]<0x62020000]
                    if actual_trace!=trace:raise ValueError(row['name']+' '+row['kind']+' '+key+' order: '+str(actual_trace)[:4000]+' vs '+str(trace)[:4000])
                digest.update(json.dumps(expected,sort_keys=True).encode())
            proc.stdin.close()
            if proc.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if proc.poll()is None:proc.terminate();proc.wait(timeout=5)
    report=dict(passed=True,physical_cases=len(rows),whole_cases=counts,service_prefix_cases=prefixes,original_instructions=proof['original_instructions'],comparison_sha256=digest.hexdigest(),
        original_xbe_sha256=base.PIN,owners=meta['owners'],instruction_patches=proof['instruction_patches'],supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        scope=__doc__)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'native draw-unit physical comparisons',counts,prefixes);return 0

if __name__=='__main__':raise SystemExit(main())
