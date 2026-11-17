#!/usr/bin/env python3
"""Unchanged physical 45550/458C0 versus the native sphere-cull kernels.

The original runs on physical i386 with its genuine 145560/145200/BAC30/
32A660 children; no instruction is patched and no callee outcome supplied.
Cases cover the per-leaf sphere branch (model, BSP cluster and instance
units, node-space transforms, BAC30 counts, repeated nodes, unbounded and
preselected leaves, special floats, MXCSR modes) and whole 458C0 (bit and
range lists, odd and negative counts). The query-code branch needs 2E7C40,
which is the projection executor's owner, and is exercised live instead.
Use run_test.py; raw banks/ELFs remain scratch-owned.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
CENTER,BSPX,CLUSTERS,INSTANCES,DEFINITIONS,LEAVES,LISTS=0x100c000,0x100d000,0x100e000,0x100f000,0x1011000,0x1005000,0x1018000
UNITS=0x4c6958
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
SPECIAL=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345)


def template(image):
    row=copy.deepcopy(base.inputs(image)[0])
    row['banks']=[dict(address=b['address'],before=bytearray.fromhex(b['before']))for b in row['banks']]
    return row


def new(proto,n,name):
    row=copy.deepcopy(proto);row.update(id=n,name=name,max_steps=100000,live_x87=[])
    def put(at,value,size=4):
        for b in row['banks']:
            if b['address']<=at and at+size<=b['address']+len(b['before']):
                b['before'][at-b['address']:at-b['address']+size]=int(value&((1<<(8*size))-1)).to_bytes(size,'little');return
        raise ValueError('Unmapped sphere-cull input '+hex(at))
    return row,put


def geometry(put,rng,parts,nodes,kind,specials=False):
    """Unit 0 with `parts` leaf spheres; kind is model, cluster or instance."""
    put(base.GEOM+8,parts);put(base.GEOM+0x30,0);put(base.GEOM+0x34,LEAVES)
    for p in range(parts):
        leaf=0xffff if rng.random()<.1 else p
        put(base.SUBPART+8*p+4,leaf,2)
        r=rng.choice((.25,.5,1,2,4))
        values=[bits(rng.uniform(-6,6)),bits(rng.uniform(-6,6)),bits(rng.uniform(-6,6)),bits(r)]
        if specials and rng.random()<.3:values[rng.randrange(4)]=rng.choice(SPECIAL)
        for i,v in enumerate(values):put(base.BOUND+20*p+4*i,v)
        put(base.BOUND+20*p+0x10,rng.choice(range(max(nodes,1))),1)
    for w in range(4):put(base.MASK+4*w,rng.getrandbits(32)&rng.getrandbits(32)&rng.getrandbits(32))
    flags=0x40|(rng.getrandbits(1) if rng.random()<.5 else 0)
    if kind=='model':put(UNITS,flags);put(UNITS+4,0xaabb0000)
    else:
        put(UNITS+4,0xffffffff);put(0x4ecc18,BSPX);put(BSPX+0xa0,CLUSTERS);put(BSPX+0x144,INSTANCES);put(BSPX+0x13c,DEFINITIONS)
        put(CLUSTERS+0x50,base.GEOM);put(INSTANCES+0x34,0,2);put(DEFINITIONS+0x50,base.GEOM)
        put(UNITS,flags|(0x1000 if kind=='instance' else 0))
    put(UNITS+0xc,0)


def matrices(put,rng,nodes,specials=False):
    put(base.BODY+0x114,52*nodes,2);put(base.BODY+0x116,0x200,2)
    for node in range(max(nodes,4)):
        for at in (base.BODY+0x200+0x34*node,base.INV+96*node+0x28):
            scale=rng.choice((1,1,.5,2,0,-1))
            m=[bits(scale)]+[bits(rng.uniform(-1,1))for _ in range(9)]+[bits(rng.uniform(-3,3))for _ in range(3)]
            if specials and rng.random()<.25:m[rng.randrange(13)]=rng.choice(SPECIAL)
            for i,v in enumerate(m):put(at+4*i,v)


def sphere_cases(proto):
    rng=random.Random(0x45550458c0);n=0
    controls=((0x27f,0,0x1f80),(0x27f,0x20,0x1fa0),(0x27f,0,0x3f80),(0x27f,0,0x5f80),(0x27f,0,0x7f80),(0x27f,0,0x9fc0),(0x67f,0,0x1f80))
    shapes=[]
    for kind in ('model','cluster','instance'):
        for parts in (1,3,32,33,40):
            for nodes in (0,1,3):shapes.append((kind,parts,nodes,False))
    for _ in range(160):shapes.append((rng.choice(('model','model','cluster','instance')),rng.randrange(1,64),rng.randrange(0,4),rng.random()<.5))
    for kind,parts,nodes,specials in shapes:
        row,put=new(proto,n,f'sphere-{kind}-{parts}-{nodes}-{n}');n+=1
        geometry(put,rng,parts,nodes,kind,specials);matrices(put,rng,nodes,specials)
        if rng.random()<.15:put(base.DRAWCONTEXT,0xffffffff)
        if rng.random()<.1:put(base.BODY+0x114,rng.choice((0x8000,0xffff,51,53)),2)
        center=[bits(rng.uniform(-4,4))for _ in range(3)]
        if specials and rng.random()<.3:center[rng.randrange(3)]=rng.choice(SPECIAL)
        for i,v in enumerate(center):put(CENTER+4*i,v)
        radius=rng.choice((bits(rng.uniform(0,5)),bits(0),*(SPECIAL if specials else ())))
        row.update(entry=0x45550,gp=[0,0,0,0,base.SP,0,0,0],matrix=CENTER,source=radius,before_fp=list(rng.choice(controls)))
        yield row
    # Boundary equality: (r+R)^2 == d^2 exactly and one ulp either side.
    for delta in (-1,0,1):
        row,put=new(proto,n,f'sphere-boundary-{delta}');n+=1
        geometry(put,random.Random(n),1,0,'cluster');put(base.BOUND,bits(3));put(base.BOUND+4,0);put(base.BOUND+8,0)
        put(base.BOUND+12,bits(1));put(base.MASK,0)
        for i,v in enumerate((0,0,0)):put(CENTER+4*i,v)
        row.update(entry=0x45550,gp=[0,0,0,0,base.SP,0,0,0],matrix=CENTER,source=bits(2)+delta,before_fp=[0x27f,0,0x1f80])
        yield row


def mask_cases(proto,first_id):
    rng=random.Random(0x458c0);n=first_id
    for _ in range(120):
        kind=rng.choice(('model','cluster','instance'))
        row,put=new(proto,n,f'mask-{kind}-{n}');n+=1
        geometry(put,rng,rng.randrange(0,6),0,kind)
        base_offset=rng.randrange(0,8);put(base.GEOM+8,base_offset)
        for i in range(64):put(LEAVES+2*i,rng.randrange(0,128),2)
        for w in range(4):put(base.MASK+4*w,0)
        ranges=rng.random()<.6
        first,second=LISTS,LISTS+0x400
        counts=[rng.choice((0,1,2,3,4,7,8,-1,-5)),rng.choice((0,1,2,5,-2))]
        for header,count in zip((first,second),counts):
            data=header+0x10;put(header,data);put(header+4,count);put(header+8,0x80000200)
            for i in range(max(count,0)+1):
                if ranges and header==first:put(data+4*i,rng.randrange(0,40-base_offset)|(rng.getrandbits(16)<<16))
                else:put(data+4*i,rng.randrange(0,128)|(rng.getrandbits(16)<<16))
        row.update(entry=0x458c0,gp=[0,0,0,0,base.SP,0,0,0],matrix=rng.choice((0,0,0x10000)),source=int(ranges)|(rng.getrandbits(8)<<8 if ranges else 0),
            stack_arguments=[first,second],before_fp=[0x27f,0,0x1f80])
        yield row


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=base.PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);proto=template(image)
    rows=list(sphere_cases(proto));rows+=list(mask_cases(proto,len(rows)))
    for row in rows:
        for b in row['banks']:b['before']=b['before'].hex() if isinstance(b['before'],(bytes,bytearray)) else b['before']
    meta=base.prepare(image,scratch,(0x45550,0x458c0,0x145560,0x145200,0xbac30,0x32a660))
    (scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],
        capture_output=True,text=True,timeout=600)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());assert not proof['instruction_patches'] and not proof['supplied_original_outcomes']
    stack=base.SECTIONS_MAP['stack'][0];digest=hashlib.sha256();changed=0;hits=0;coverage=[0]*13
    with (scratch/'native-errors.log').open('w+')as errors:
        native=subprocess.Popen([str(a.native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,actual in zip(rows,proof['cases'],strict=True):
                if not actual['completed']:raise ValueError(row['name']+' original did not complete')
                native.stdin.write(json.dumps(row,separators=(',',':'))+'\n');native.stdin.flush();line=native.stdout.readline()
                if not line:errors.seek(0);raise ValueError(row['name']+': '+errors.read()[-2500:])
                mine=json.loads(line)
                gold=[b for b in actual['banks'] if b['address']!=stack];ours=[b for b in mine['banks'] if b['address']!=stack]
                for g,o in zip(gold,ours,strict=True):
                    if g!=o:
                        x,y=bytes.fromhex(g['after']),bytes.fromhex(o['after'])
                        first=next(i for i in range(len(x)) if x[i]!=y[i])
                        raise ValueError(f"{row['name']} bank {g['address']:#x}+{first:#x}: original {x[first:first+8].hex()} native {y[first:first+8].hex()}")
                if mine['fp']!=actual['after_fp']:raise ValueError(f"{row['name']} FP original {actual['after_fp']} native {mine['fp']}")
                before={b['address']:b['before']for b in row['banks']}
                changed+=any(before[b['address']]!=b['after']for b in gold)
                hits+=before[UNITS&~0xfff]!=next(b['after']for b in gold if b['address']==UNITS&~0xfff)
                for i,value in enumerate(mine.get('coverage',[])):coverage[i]+=value
                digest.update(line.encode())
            native.stdin.close()
            if native.wait(timeout=30):errors.seek(0);raise ValueError(errors.read()[-2500:])
        finally:
            if native.poll() is None:native.terminate();native.wait(timeout=5)
    names=('calls','no_geometry','queries','parts','already','unbounded','bsp_parts','no_object','same_node','no_nodes','transforms','hits','misses')
    report=dict(passed=True,physical_cases=len(rows),cases_with_persistent_stores=changed,cases_changing_unit_bank=hits,
        native_45550_coverage=dict(zip(names,coverage)),
        original_xbe_sha256=base.PIN,owners=meta['owners'],original_instructions=proof['original_instructions'],
        comparison_sha256=digest.hexdigest(),instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],
        scope='Whole 45550 per-leaf branch with genuine 145560/145200/BAC30 and whole 458C0; every non-stack byte and the x87/MXCSR words. The 2E7C40 query branch is covered live.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(rows),'physical 45550/458C0 cases,',changed,'with persistent stores',dict(zip(names,coverage)))
    return 0


if __name__=='__main__':raise SystemExit(main())
