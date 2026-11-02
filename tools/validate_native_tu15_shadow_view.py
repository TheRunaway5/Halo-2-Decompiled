#!/usr/bin/env python3
"""Whole unchanged physical-i386 320A0/1681C0 versus native typed value kernel.

Use run_test.py. The real box-edge producer and clipping/projection leaf run
without patched instructions or supplied callees. Raw banks/ELFs stay scratch.
"""
import argparse
import gzip
import hashlib
import itertools
import json
import os
import random
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
DATA,CONTEXT,BOX,EMPTY=0x1000000,0x1000000,0x1001000,0x1001800
STACK,SP,STOP=0x62000000,0x62001000,0x63000000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def digest(v):return hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]


def cases(constants,pointers):
    camera=[0]*111
    for at in (1,2,6,10,14,15,19,23):camera[at]=bits(1.)
    camera[28]=bits(.1);camera[40:44]=list(map(bits,(-1,1,-1,1)))
    box=[0]*16
    for at in (0,1,5,9):box[at]=bits(1.)
    box[12]=bits(-2);box[13:]=list(map(bits,(.25,.5,.75)))
    parameters=[]
    for z,scale in itertools.product((-10,-2,-1,-.1,-.05,0,.1,1,10),(0,1,-1,2)):
        b=box.copy();b[12]=bits(z);b[0]=bits(scale);parameters.append((camera.copy(),b,0x27f,0x1f80))
    special=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321)
    for kind,index in [('box',i)for i in (0,1,5,9,10,12,13,14,15)]+[('camera',i)for i in (1,2,6,10,11,13,21,22,23,28,40,41,44,57,58,59)]:
        for v in special:
            c,b=camera.copy(),box.copy();(b if kind=='box'else c)[index]=v;parameters.append((c,b,0x27f,0x1f80))
    for cw,mxcsr in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x1fc0,0x9fc0)):
        for z in (-.1,-2):
            c,b=camera.copy(),box.copy();b[12]=bits(z);b[0]=1;parameters.append((c,b,cw,mxcsr))
    for a,bits_b in itertools.product(special[-4:],repeat=2):
        for kind in ('box','box_extent','camera','camera_point'):
            c,b=camera.copy(),box.copy()
            if kind=='box':b[0],b[1]=a,bits_b
            elif kind=='box_extent':b[0],b[13]=a,bits_b
            elif kind=='camera':c[1],c[2]=a,bits_b
            else:c[1],b[10]=a,bits_b
            parameters.append((c,b,0x27f,0x1f80))
    rng=random.Random(0x320a01681c0)
    for index in range(4):
        c,b=camera.copy(),box.copy();b[12]=bits(10)
        c[40:44]=[0xff7fffff,0x7f7fffff,0xff7fffff,0x7f7fffff];c[40+index]=0x7fc12345
        parameters.append((c,b,0x27f,0x1f80))
    for _ in range(32):
        c,b=camera.copy(),box.copy()
        for index in range(1,13):b[index]=bits(rng.uniform(-3,3))
        for index in range(2,14):c[index]=bits(rng.uniform(-3,3))
        parameters.append((c,b,0x27f,0x1f80))
    for _ in range(64):
        c,b=camera.copy(),box.copy()
        for index in (1,2,6,10,11,13,21,22,23,28,40,41,44,57,58,59):c[index]=rng.getrandbits(32)
        b=[rng.getrandbits(32)for _ in b];parameters.append((c,b,0x27f,0x1f80))
    for n,(c,b,cw,mxcsr)in enumerate(parameters):
        const=bytearray.fromhex(constants);pointer=bytearray.fromhex(pointers);struct.pack_into('<I',pointer,0xd40,EMPTY)
        data=bytearray(0x2000);struct.pack_into('<111I',data,4,*c);struct.pack_into('<16I',data,BOX-DATA,*b)
        struct.pack_into('<6I',data,EMPTY-DATA,0x7f7fffff,0xff7fffff,0x7f7fffff,0xff7fffff,0x7f7fffff,0xff7fffff)
        binding=bytearray(4096);struct.pack_into('<I',binding,0xe18,CONTEXT)
        yield dict(id=n,name='view-'+str(n),camera=c,box=b,entry=0x320a0,stop_pc=STOP,max_steps=20000,
            gp=[0,0,0,0,SP,0,BOX,0],matrix=0,source=0,before_fp=[cw,0x20,mxcsr],live_x87=[],
            banks=[dict(address=0x46a000,before=const.hex()),dict(address=0x474000,before=pointer.hex()),dict(address=0x556000,before=binding.hex()),
                dict(address=DATA,before=data.hex()),dict(address=STACK,before=bytes(8192).hex())])


def prepare(image,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0x320a0,0x1681c0,0x122400,0x122360);meta=common.metadata(image);meta['stop']=STOP
    meta['sites']=list({s['instruction']:s for s in meta['sites']}.values())
    d=scratch/'physical-original';d.mkdir();section=image.section('.text');(d/'text.bin').write_bytes(image.section_bytes(section));(d/'metadata.json').write_text(json.dumps(meta))
    sections={'constants':(0x46a000,4096),'pointers':(0x474000,4096),'binding':(0x556000,4096),'data':(DATA,8192),'stack':(STACK,8192)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{d/"text.bin"}"\n'
    for name,(_,size)in sections.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';(d/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker+=' '.join(f'.{name} 0x{address:x}:{{*(.{name})}}'for name,(address,_)in sections.items())
    linker+=f' .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(d/'link.ld').write_text(linker)
    for command in (['as','--32',str(d/'reference.s'),'-o',str(d/'reference.o')],['ld','-melf_i386','-T',str(d/'link.ld'),str(d/'reference.o'),'-o',str(d/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(d/'observer')]):
        r=subprocess.run(command,capture_output=True,text=True)
        if r.returncode:raise ValueError(r.stderr[-3000:])
    return d,meta


def expected(original):
    if not original['completed']:raise ValueError('Whole original shadow view did not complete')
    stack=bytes.fromhex(next(b['after']for b in original['banks']if b['address']==STACK))
    return dict(admitted=bool(original['boundary_gp'][0]&255),bounds=list(struct.unpack_from('<6I',stack,SP-STACK-0x258)),
        endpoints=list(struct.unpack_from('<72I',stack,SP-STACK-0x120)),fp=original['after_fp'])


def compare(rows,golds,native):
    h=hashlib.sha256()
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native.stderr').open('w+')as errors:
        process=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,golds,strict=True):
                process.stdin.write(json.dumps(row,separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
                if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                actual=json.loads(line)
                if actual!=gold:
                    differences={k:[(n,a,b)for n,(a,b)in enumerate(zip(actual[k],gold[k]))if a!=b][:8]if isinstance(actual[k],list)else(actual[k],gold[k])for k in gold if actual[k]!=gold[k]}
                    raise ValueError(row['name']+' differs: '+str(differences)+' box='+str(row['box'])+' camera='+str(row['camera']))
                h.update(json.dumps(actual,sort_keys=True).encode())
            process.stdin.close()
            if process.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if process.poll()is None:process.terminate();process.wait(timeout=5)
    return dict(comparisons=len(rows),comparison_sha256=h.hexdigest())


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture-output',type=Path);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);constants=image.read(0x46a000,4096).hex();pointers=image.read(0x474000,4096).hex();rows=list(cases(constants,pointers))
    d,meta=prepare(image,Path(os.environ['HALO2_TEST_SCRATCH']));(d/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(d/'observer'),str(d/'reference'),str(d/'metadata.json'),str(d/'inputs.json'),str(d/'observed.json')],capture_output=True,text=True,timeout=240)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-4000:])
    proof=json.loads((d/'observed.json').read_text());assert not proof['instruction_patches']and not proof['supplied_original_outcomes']
    golds=[expected(original)for original in proof['cases']];result=compare(rows,golds,a.native)
    report=dict(passed=True,physical_cases=len(rows),**result,original_xbe_sha256=PIN,owners=meta['owners'],original_instructions=proof['original_instructions'],instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],scope='Whole320A0 receiver transform plus complete1681C0 near-only clip/projection; raw endpoints, all bounds, BOOL and FP status. No GPU or whole renderer parity claim.')
    if a.fixture_output:
        fixture=dict(schema='TU15_NATIVE_SHADOW_VIEW_V1',original_xbe_sha256=PIN,constants=constants,pointers=pointers,inputs_sha256=[digest(row)for row in rows],expected=golds,owners=meta['owners'])
        packed=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0);assert len(packed)<1024*1024
        a.fixture_output.parent.mkdir(parents=True,exist_ok=True);a.fixture_output.write_bytes(packed);report['required_reference_fixture']=dict(path=str(a.fixture_output),bytes=len(packed),sha256=sha(a.fixture_output))
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'whole original shadow view cases')


if __name__=='__main__':main()
