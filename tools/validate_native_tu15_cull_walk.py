#!/usr/bin/env python3
"""Unchanged physical44710/44680 versus typed native walk and sphere arithmetic.

The actual44E80/45550 services run with declared empty geometry. Their calls
are observed read-only, never intercepted. This qualifies the parent, not the
geometry implementations. Use run_test.py; raw banks/ELFs remain scratch-owned.
"""
import argparse
import copy
import gzip
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
TRANSFORM=0x100a000
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def cases(image):
    initial=base.inputs(image)[0]
    parameters=[]
    identity=list(map(bits,(1,1,0,0,0,1,0,0,0,1,0,0,0)))
    center=list(map(bits,(1.25,-2.5,3.75)))
    special=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,
        0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321)
    for scale in (bits(1),bits(-1),bits(.5),bits(2),*special):
        t=identity.copy();t[0]=scale;parameters.append((t,center,0x27f,0x1f80))
    for bank,index in [('transform',i)for i in range(1,13)]+[('center',i)for i in range(3)]:
        for value in special:
            t,c=identity.copy(),center.copy();(t if bank=='transform'else c)[index]=value;parameters.append((t,c,0x27f,0x1f80))
    for a,b in itertools.product(special[-4:],repeat=2):
        t=identity.copy();t[0]=a;t[1]=b;parameters.append((t,center,0x27f,0x1f80))
    for cw,mxcsr in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x1fc0,0x9fc0)):
        t=identity.copy();t[0]=bits(1);t[4]=bits(.1);parameters.append((t,center,cw,mxcsr))
    rng=random.Random(0x4471044680)
    for _ in range(40):parameters.append(([rng.getrandbits(32)for _ in range(13)],[rng.getrandbits(32)for _ in range(3)],0x27f,0x1f80))
    def new(n):
        row=copy.deepcopy(initial);row.update(id=n,name='cull-walk-'+str(n),entry=0x44710,max_steps=50000)
        row['banks']=[dict(address=b['address'],before=bytearray.fromhex(b['before']))for b in row['banks']]
        def put(at,value,size=4):
            for b in row['banks']:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    struct.pack_into('<I'if size==4 else'<H',b['before'],at-b['address'],value&((1<<(size*8))-1));return
            raise ValueError('Unmapped input '+hex(at))
        put(base.GEOM+8,0);put(base.BODY+0x114,0,2)
        put(0x4cd39c,1);put(0x4cd398,1)
        return row,put
    n=0
    for transform,c,cw,mxcsr in parameters:
        row,put=new(n);n+=1;row['before_fp']=[cw,0x20,mxcsr]
        put(base.CTX+0x2a7c,1);put(0x4c6958+0x1c,TRANSFORM)
        for i,v in enumerate(transform):put(TRANSFORM+4*i,v)
        for i,v in enumerate(c):put(base.CTX+0x2a68+4*i,v)
        put(base.CTX+0x2a74,0x7fc12345) # unchanged raw radius, even unordered
        for b in row['banks']:b['before']=b['before'].hex()
        yield row
    # Main/secondary selection, full ESI service args, direct low16 ordinal,
    # every association word and signed association address; real empty leaves.
    for mode,query,association in itertools.product((0,1,0xffffffff),(0,2,-1),(False,True)):
        row,put=new(n);n+=1;put(base.CTX+0x2a78,mode);put(base.CTX+0x2a7c,query)
        put(0x4cd39c,0xbeef0001 if mode else 4);put(0x4cd398,0xabcd0004)
        for i in range(4):
            at=0x4c6958+32*i
            put(at,0x48 if association else 0x40);put(at+4,0);put(at+0x10,base.DRAWCONTEXT);put(at+0x14,base.MASK)
            put(at+0x18,0xffff if association else 0x12340000+i)
        # Negative association -1 is distinct from unsigned direct FFFF.
        for i,v in enumerate((0x80000005,0x10,0,0x80000000)):put(base.CTX+0xa50+4*i,v)
        for b in row['banks']:b['before']=b['before'].hex()
        yield row
    for flags in (0,1,0x41,0x40,0x48):
        row,put=new(n);n+=1;put(0x4c6958,flags)
        for b in row['banks']:b['before']=b['before'].hex()
        yield row


def expected(actual):
    assert actual['completed']
    calls=[]
    for c in actual['calls']:
        if c['entry']==0x44e80:calls.append(dict(kind='frustum',unit=c['arguments'][0],entry=c['arguments'][1]))
        else:calls.append(dict(kind='sphere',unit=c['ecx'],center=c['indirect'],radius=c['arguments'][1]))
    return dict(calls=calls,fp=actual['after_fp'])


def compare(rows,golds,native):
    digest=hashlib.sha256()
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native-errors.log').open('w+')as errors:
        p=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,golds,strict=True):
                p.stdin.write(json.dumps(row,separators=(',',':'))+'\n');p.stdin.flush();line=p.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-2500:])
                actual=json.loads(line)
                if actual!=gold:raise ValueError(row['name']+' differs: '+str(actual)+' vs '+str(gold))
                digest.update(json.dumps(actual,sort_keys=True).encode())
            p.stdin.close()
            if p.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-2500:])
        finally:
            if p.poll()is None:p.terminate();p.wait(timeout=5)
    return digest.hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture-output',type=Path);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=base.PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);rows=list(cases(image))
    # Reuse only the owned ELF layout, never the earlier proof's outcomes.
    meta=base.prepare(image,scratch,(0x44710,0x44680,0x44e80,0x45550,0x32a660))
    meta['trace_calls']=[dict(entry=0x44e80,arguments=2),dict(entry=0x45550,arguments=2,ecx=True,indirect=[dict(argument=0,words=3)])]
    (scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=180)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());assert not proof['instruction_patches']and not proof['supplied_original_outcomes']
    # Empty geometry services perform no persistent stores. Genuine caller
    # stack writes differ by implementation and are excluded from this check.
    for row,actual in zip(rows,proof['cases'],strict=True):
        for before,after in zip(row['banks'],actual['banks'],strict=True):
            if before['address']!=base.SECTIONS_MAP['stack'][0]:assert before['before']==after['after']
    golds=[expected(c)for c in proof['cases']];digest=compare(rows,golds,a.native)
    report=dict(passed=True,physical_cases=len(rows),original_xbe_sha256=base.PIN,owners=meta['owners'],
        original_instructions=proof['original_instructions'],service_calls=sum(len(g['calls'])for g in golds),
        comparison_sha256=digest,instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],
        scope='Whole44710/44680 parent ordering, exact raw inverse-point/radius arguments, FP status, complete unchanged persistent banks with actual empty44E80/45550 geometry services. Native core also checks live service mutations. Nonempty geometry/full World and rendered acceptance are separate.')
    if a.fixture_output:
        # Required reference input contains compact original declarations and
        # outcomes; the multi-megabyte raw transient banks are not retained.
        fixture=dict(schema='TU15_NATIVE_CULL_WALK_V1',original_xbe_sha256=base.PIN,constants=image.read(0x46a000,4096).hex(),
            expected=golds,input_sha256=[hashlib.sha256(json.dumps(row,sort_keys=True).encode()).hexdigest()for row in rows],owners=meta['owners'])
        packed=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0);assert len(packed)<1024*1024
        a.fixture_output.parent.mkdir(parents=True,exist_ok=True);a.fixture_output.write_bytes(packed)
        report['required_reference_fixture']=dict(path=str(a.fixture_output),bytes=len(packed),sha256=sha(a.fixture_output))
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(rows),'physical whole44710 cases,',report['service_calls'],'genuine calls')
    return 0


if __name__=='__main__':raise SystemExit(main())
