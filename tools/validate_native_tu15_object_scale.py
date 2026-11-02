#!/usr/bin/env python3
"""Physical unchanged-i386 scaler blocks; all raw output belongs to run_test.py.

Admission stops before BF050; pose starts after BC380; duration stops before
B96B0. No dependency return, instruction patch or resumed prefix is supplied.
Full-world tests separately execute the real interpolation/bounds services.
"""
import argparse
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

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
DATA,BODY,POOL,ROWS=0x1000000,0x1002000,0x1000800,0x1001000
STACK,SP,STOP=0x62000000,0x62000800,0x63000000


def digest(v):return hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def cases():
    special=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x3f000000,0x3f800000,
        0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321)
    parameters=[]
    for scale,obj in itertools.product(special,(0,0xbeef0001,0xffffffff)):
        parameters.append(dict(kind='admit',scale=scale,old_scale=0x3f800000,object=obj))
    for old,scale in itertools.product(special,special):
        parameters.append(dict(kind='pose',old_scale=old,scale=scale,offset=0x200,components=[old,scale,0x3f333333,0x80000000]))
    for offset in (-0x100,-0x10,0,0x84,0xa0,0x200):
        parameters.append(dict(kind='pose',old_scale=0x3f000000,scale=0x40000000,offset=offset,components=[1,0x7f812345,0x7fc54321,0x3f800000]))
    rng=random.Random(0xb96b0)
    for _ in range(48):parameters.append(dict(kind='pose',old_scale=rng.getrandbits(32),scale=rng.getrandbits(32),offset=0x200,components=[rng.getrandbits(32)for _ in range(4)]))
    for ticks in (0,1,2,29,30,31,127,255,0x7fff,0x8000,0xfffe,0xffff):parameters.append(dict(kind='seconds',ticks=ticks))
    for cw,mxcsr in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(0x1f80,0x1fc0,0x9fc0)):
        for sample in (dict(kind='seconds',ticks=0x7fff),dict(kind='pose',old_scale=1,scale=0x3f333333,offset=-0x100,components=[1,0x80000001,0x7f812345,0x7fc54321])):
            parameters.append(dict(sample,cw=cw,mxcsr=mxcsr))
    for n,p in enumerate(parameters):
        banks=[dict(address=0x46a000,before=bytearray(4096)),dict(address=0x4ec000,before=bytearray(4096)),
            dict(address=DATA,before=bytearray((i*13+0x71)&255 for i in range(0x4000))),dict(address=STACK,before=bytearray(4096))]
        def put(at,value,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=int(value&((1<<(size*8))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped input')
        put(0x46a17c,0x3d088889);put(0x4ecbd0,POOL);put(POOL+0x44,ROWS);put(ROWS+8,BODY);put(ROWS+20,BODY)
        put(BODY+0xa0,p.get('old_scale',0x3f800000));put(BODY+0x112,0xffff,2);put(BODY+0x10,0xffffffff)
        put(BODY+0x10e,p.get('offset',0x200),2)
        for at,v in zip((0x1c,0x10,0x14,0x18),p.get('components',[])):put(BODY+p.get('offset',0x200)+at,v)
        put(SP,STOP);put(SP+4,0);put(SP+8,p.get('scale',0));put(SP+12,p.get('ticks',0))
        gp=[p.get('object',0),0x22222222,0x33333333,0x44444444,SP,0x66666666,0x77777777,BODY]
        xmm=[[0]*4 for _ in range(8)];xmm[2][0]=p.get('old_scale',0);xmm[3][0]=p.get('scale',0)
        kind=p['kind'];entry=0xb970d if kind=='pose'else 0x110030 if kind=='seconds'else 0xb96b0
        stop=0x110075 if kind=='seconds'else 0xb9756
        # NONE/rejected admission returns normally, without reaching BF050.
        if kind=='admit'and(p['object']==0xffffffff or p['scale']==0 or p['scale']&0x80000000 or p['scale']>=0x7f800001):stop=STOP
        if kind=='seconds':gp[0]=0;put(SP+4,0)
        yield dict(p,id=n,name=kind+'-'+str(n),body=BODY,entry=entry,stop_pc=stop,gp=gp,xmm=xmm,
            matrix=0,source=0,before_fp=[p.get('cw',0x27f),0x20,p.get('mxcsr',0x1f80)],live_x87=[],
            banks=[dict(address=b['address'],before=b['before'].hex())for b in banks])


def prepare(image,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0xb96b0,0x110030);meta=common.metadata(image);meta['stop']=STOP
    meta['sites']=list({s['instruction']:s for s in meta['sites']}.values())
    d=scratch/'physical-original';d.mkdir();section=image.section('.text');(d/'text.bin').write_bytes(image.section_bytes(section))
    (d/'metadata.json').write_text(json.dumps(meta));sections={'constants':(0x46a000,4096),'global':(0x4ec000,4096),'data':(DATA,0x4000),'stack':(STACK,4096)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{d/"text.bin"}"\n'
    for name,(_,size)in sections.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (d/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker+=' '.join(f'.{name} 0x{address:x}:{{*(.{name})}}'for name,(address,_)in sections.items())
    linker+=f' .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(d/'link.ld').write_text(linker)
    for command in (['as','--32',str(d/'reference.s'),'-o',str(d/'reference.o')],['ld','-melf_i386','-T',str(d/'link.ld'),str(d/'reference.o'),'-o',str(d/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(d/'observer')]):
        r=subprocess.run(command,capture_output=True,text=True)
        if r.returncode:raise ValueError(r.stderr[-3000:])
    return d,meta


def expected(row,original):
    if not original['completed']:raise ValueError('Original stopped early '+row['name'])
    banks=[b for b in original['banks']if b['address']!=STACK]
    reads=[[r['address'],r['bytes']]for r in original['reads']if not STACK<=r['address']<STACK+4096]
    if row['kind']=='seconds':reads=[[0x46a17c,4]]
    return dict(reads=reads,writes=[[w['address'],w['written']]for w in original['writes']if not STACK<=w['address']<STACK+4096],
        banks_sha256=digest(banks),fp=original['after_fp'],seconds=original['boundary_xmm'][0][0]if row['kind']=='seconds'else 0)


def compare(rows,golds,native):
    h=hashlib.sha256();count=0
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native.stderr').open('w+')as errors:
        process=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,golds,strict=True):
                for publisher in (False,True):
                    process.stdin.write(json.dumps(dict(row,publisher=publisher),separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(errors.read()[-3000:])
                    actual=json.loads(line);actual['banks_sha256']=digest(actual.pop('banks'))
                    if actual!=gold:raise ValueError(row['name']+' differs: '+str(dict(native=actual,original=gold)))
                    h.update(json.dumps(actual,sort_keys=True).encode());count+=1
            process.stdin.close()
            if process.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if process.poll()is None:process.terminate();process.wait(timeout=5)
    return dict(comparisons=count,comparison_sha256=h.hexdigest())


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture-output',type=Path);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);rows=list(cases());d,meta=prepare(image,Path(os.environ['HALO2_TEST_SCRATCH']));(d/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(d/'observer'),str(d/'reference'),str(d/'metadata.json'),str(d/'inputs.json'),str(d/'observed.json')],capture_output=True,text=True,timeout=180)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-4000:])
    proof=json.loads((d/'observed.json').read_text());assert not proof['instruction_patches']and not proof['supplied_original_outcomes']
    golds=[expected(row,original)for row,original in zip(rows,proof['cases'],strict=True)];result=compare(rows,golds,a.native)
    names=['include/halo2/native_tu15_object_scale.hpp','src/rendering/native_tu15_object_scale.cpp','tests/native_tu15_object_scale_tests.cpp','tools/validate_native_tu15_object_scale.py']
    report=dict(passed=True,physical_cases=len(rows),**result,source_sha256={n:sha(ROOT/n)for n in names},original_xbe_sha256=PIN,
        owners=meta['owners'],original_instructions=proof['original_instructions'],instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],
        scope='Exact B96B0 admission before BF050; B970D..B9756 pose effect after BC380; signed110030 duration before B96B0. Interpolation/bounds closure and full renderer parity remain separate.')
    if a.fixture_output:
        fixture=dict(schema='TU15_NATIVE_OBJECT_SCALE_V1',original_xbe_sha256=PIN,inputs_sha256=[digest(r)for r in rows],expected=golds,physical_owners=meta['owners'])
        packed=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0);assert len(packed)<1024*1024
        a.fixture_output.parent.mkdir(parents=True,exist_ok=True);a.fixture_output.write_bytes(packed)
        report['required_reference_fixture']=dict(path=str(a.fixture_output),bytes=len(packed),sha256=sha(a.fixture_output))
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical scale cases,',result['comparisons'],'native comparisons')


if __name__=='__main__':main()
