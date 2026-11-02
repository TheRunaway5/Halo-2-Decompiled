#!/usr/bin/env python3
"""Unchanged physical TU1.5 command parents and complete part-mask predicate.

The physical parents execute genuine empty preparation/geometry/later services.
Native test services model those declared empty inputs only. Nonempty services,
World publication and GPU acceptance are separate required qualification.
Use tools/run_test.py; all raw banks, ELFs and compiler outputs stay in scratch.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
CTX,BSP,TAGS,MODEL,SECTIONS,INST,DEFS,CLUST,GEOM,PART,MASK=range(0x1000000,0x100b000,0x1000)
STACK=0x62017000


def cases():
    rows=[]
    def new(entry):
        row=dict(id=len(rows),name=f'{entry:x}-{len(rows)}',entry=entry,gp=[0,0,0,0,base.SP,0,0,0],
            matrix=0,source=0,group=31,before_fp=[0x27f,0x220,0x1fa0],live_x87=[],max_steps=50000)
        banks=[dict(address=at,before=bytearray(size))for at,size in base.SECTIONS_MAP.values()]
        def put(at,v,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(v&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped command input '+hex(at))
        put(0x4cd3a0,CTX);put(CTX+0x14,MODEL+0x500);put(0x4f0414,TAGS);put(TAGS+8,MODEL)
        put(MODEL+0x28,SECTIONS);put(SECTIONS+0x34,GEOM);put(0x4ecc18,BSP)
        put(BSP+0xa0,CLUST);put(BSP+0x144,INST);put(BSP+0x13c,DEFS);put(CLUST+0x50,GEOM)
        put(INST+0x34,0,2);put(DEFS+0x50,GEOM);put(GEOM+4,PART);put(0x4f3218,MODEL);put(MODEL+8,3)
        put(0x4cd39c,1);put(0x4cd398,1);put(0x4cd6f6,0xffffffff)
        def finish():
            row['banks']=[dict(address=b['address'],before=b['before'].hex())for b in banks];rows.append(row)
        return row,put,finish
    rng=random.Random(0x460c044ab0)
    for kind,(first,count),(part,unit) in itertools.product(range(4),
            ((0,0),(0,1),(0,32),(31,3),(32,34),(-1,2),(-33,67),(32767,3),(-32768,2),(7,-1),(1,-32768)),
            ((0,0),(0xbeef0001,1),(0xffffffff,0xffff))):
        row,put,finish=new(0x460c0);row['gp'][0]=part;row['gp'][3]=MASK;row['matrix']=unit
        at=0x4c6958+32*(unit if unit<32768 else unit-65536)
        put(at,0 if kind==3 else 0x40|(0x1000 if kind==2 else 0));put(at+4,0 if kind==0 else 0xffffffff)
        # Arbitrary full part indices still consume 32-bit LEA wrapping.
        put(GEOM+4,(PART-72*part)&0xffffffff);put(PART+0xa,first,2);put(PART+0xc,count,2)
        put(MASK-4096,rng.getrandbits(32));put(MASK-8,rng.getrandbits(32));put(MASK-4,1)
        for i in range(8):put(MASK+4*i,rng.getrandbits(32))
        put(MASK+4092,0x80000000);put(MASK+4096,1)
        finish()
    # Resolver indices are source tokens, not bounds checked host-array
    # indices. Place only the selected rows in retained memory, including
    # signed negative instance definitions and wrapping base arithmetic.
    for kind,(index,datum,definition) in itertools.product(range(3),((511,65535,-1),(255,7,-32768),(1,0,32767))):
        row,put,finish=new(0x460c0);row['gp'][3]=MASK
        put(0x4c6958,0x40|(0x1000 if kind==2 else 0));put(0x4c695c,(0xbeef0000|datum)if kind==0 else 0xffffffff)
        put(0x4c6964,index<<(18 if kind==2 else 9));put(PART+0xa,31,2);put(PART+0xc,3,2);put(MASK+4,1)
        if kind==0:
            put(0x4f0414,(TAGS-16*datum)&0xffffffff);put(MODEL+0x28,(SECTIONS-92*index)&0xffffffff)
        elif kind==1:put(BSP+0xa0,(CLUST-176*index)&0xffffffff)
        else:
            put(BSP+0x144,(INST-88*index)&0xffffffff);put(INST+0x34,definition,2)
            put(BSP+0x13c,(DEFS-200*definition)&0xffffffff)
        finish()
    # Special lists use signed counts and bypass every ordinary eligibility
    # filter, including a flag mask of zero and group outside the valid range.
    for mode,count in itertools.product((0,1),(0,1,31,32,33,-1,-32768)):
        if mode and count==-32768:continue # would need an unrelated huge row
        row,put,finish=new(0x44ab0);row['matrix']=mode;row['source']=0;row['group']=0xffffffff
        put(base.SP+12,row['group']);put(0x4cd39c,0 if mode else 1);put(0x4cd398,1);put(0x4c6958,0x400)
        put(0x515f1c,count);put(0x515f20,1);put(0x4d2f1c+0x408,count,2)
        finish()
    # A declared wrapped light index places the destination eight bytes into
    # the source unit. REP MOVSD must observe its own preceding writes.
    row,put,finish=new(0x44ab0);row['matrix']=1;row['source']=0;put(base.SP+12,31)
    put(0x4cd39c,0);put(0x4cd398,1)
    light=((0x4c6960-0x4d2b18)//8*pow(129,-1,1<<29))%(1<<29)
    put(0x515f20,light);put((0x4d2f1c+0x408*light)&0xffffffff,0,2)
    for i in range(8):put(0x4c6958+4*i,0x400 if i==0 else 0x12340000+i)
    finish()
    # Ordinary filters, empty model/cluster geometry, selector masks, CMOV
    # low-half bounds and preparation with genuine zero-count collections.
    for entry,mode,flags,selector in itertools.product((0x44ab0,0x40e80),(0,1),(0,1,0x40,0x41,0x11,0x21,0x400),(0,1,2,0xffffffff)):
        row,put,finish=new(entry);row['matrix']=mode;row['source']=0xffffffff;row['gp'][0]=selector
        put(base.SP+12,31);put(CTX+0x2a78,mode);put(0x4cd39c,0xabcd0000 if mode else 0xabcd0001);put(0x4cd398,0xbeef0001)
        put(0x4c6958,flags);put(0x4c695c,0);put(0x4c6964,31<<4);put(0x4c6970,1<<16)
        # Dirty bank fields detect source reset selection and exact store order.
        for i in range(18):put(0x4ce268+4*i,0xdead0000+i)
        put(0x515f1c,0);put(0x515f20,0)
        finish()
    return rows


def normalize_calls(actual):
    out=[]
    for c in actual['calls']:
        e=c['entry'];v=dict(entry=e)
        if e==0x449d0:v.update(unit=c['gp'][0],owner=c['arguments'][0])
        elif e==0x41c70:v.update(context=c['gp'][7],mode=c['gp'][3])
        elif e==0x42750:v['mask']=c['arguments'][0]
        out.append(v)
    return out


def compare(rows,proof,native):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);digest=hashlib.sha256()
    with (scratch/'native-errors.log').open('w+')as errors:
        p=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,proof['cases'],strict=True):
                assert gold['completed'];p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush()
                line=p.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line);expected=dict(visible=bool(gold['boundary_gp'][0]&255)if row['entry']==0x460c0 else False,
                    calls=normalize_calls(gold),fp=gold['after_fp'],banks=[b for b in gold['banks']if b['address']!=STACK])
                for key in expected:
                    if actual[key]!=expected[key]:raise ValueError(row['name']+' differs in '+key+': '+str(actual[key])[:800]+' vs '+str(expected[key])[:800])
                if row['entry']==0x460c0:
                    original=[[r['address'],r['bytes']]for r in gold['reads']if not STACK<=r['address']<STACK+8192]
                    if actual['reads']!=original:raise ValueError(row['name']+' changed predicate reads '+str(actual['reads'])+' vs '+str(original))
                original_writes=[[w['address'],len(w['written'])//2]for w in gold['writes']if not STACK<=w['address']<STACK+8192]
                if actual['writes']!=original_writes:raise ValueError(row['name']+' changed retained store order '+str(actual['writes'])+' vs '+str(original_writes))
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
    base.SECTIONS_MAP={'units':(0x4c6000,0x3000),'context':(0x4cd000,0x2000),'globals':(0x4c2000,4096),'negative_special':(0x4c4000,4096),
        'specials':(0x4d2000,0x4000),'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'variant':(0x4f3000,4096),
        'lists':(0x515000,4096),'data':(CTX,0x10000),'stack':(STACK,8192)}
    entries=(0x460c0,0x44ab0,0x449d0,0x40e80,0x41c70,0x179b40,0x42090,0x42750,0x41630)
    meta=base.prepare(image,scratch,entries);meta['trace_calls']=[dict(entry=e,arguments=1 if e in (0x449d0,0x42750)else 0,gp=True)for e in (0x449d0,0x41c70,0x179b40,0x42090,0x42750,0x41630)]
    # Capstone's access flags omit this TEST r/m32,r32 read. Authenticate the
    # actual opcode and address expression; observe the operand read-only.
    for site in meta['sites']:
        if site['instruction']==0x46195:
            assert site['bytes']=='85048b' and len(site['operands'])==1
            site['operands'][0]['read']=True
    rows=cases();(scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    run=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=180)
    if run.returncode:raise ValueError((run.stdout+run.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());digest=compare(rows,proof,a.native)
    report=dict(passed=True,physical_cases=len(rows),predicate_cases=sum(r['entry']==0x460c0 for r in rows),original_instructions=proof['original_instructions'],
        comparison_sha256=digest,original_xbe_sha256=base.PIN,owners=meta['owners'],instruction_patches=proof['instruction_patches'],
        supplied_original_outcomes=proof['supplied_original_outcomes'],retained_large_artifacts=[],
        scope='Complete460C0 raw mask result and exact retained read order, complete44AB0/40E80 parents with genuine empty services and special rows: all persistent bytes, FP, named service order/arguments and retained store order. Main40E80 physical cases use variant3 with inactive later producers. Nonempty World services and rendered parity are separate.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical command cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
