#!/usr/bin/env python3
"""Qualify the complete light-up kernel against unchanged physical TU1.5 code.

All executable copies, banks, observer binaries and native pipe data belong to
the enclosing run_test.py scratch owner. Only a bounded receipt is retained.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
STACK, STOP, DATA, UP = 0x62000000, 0x63000000, 0x1001000, 0x1001020


def real(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    def row(name, up, forward, *, offset=12, cw=0x27f, mxcsr=0x1f80, status=0):
        bank = bytearray([0xa5] * 128)
        struct.pack_into('<3I', bank, UP-DATA, *up)
        struct.pack_into('<3I', bank, UP-DATA+offset, *forward)
        return dict(name=name, up=list(struct.unpack_from('<3I', bank, UP-DATA)),
                    forward=list(struct.unpack_from('<3I', bank, UP-DATA+offset)),
                    bank=bank.hex(), forward_address=UP+offset, cw=cw, mxcsr=mxcsr, status=status)
    fixture = json.loads((ROOT/'tests/fixtures/first_person_light_up_tu15.json').read_text())
    for n, c in enumerate(fixture['cases']):
        values = struct.unpack('<6I', bytes.fromhex(c['input']))
        yield row('finite-'+str(n), values[:3], values[3:])
    unit = (0x3f800000, 0, 0)
    for epsilon in range(0x38d1b713, 0x38d1b71c):
        for scale in (0x3f800000, real(100000)):
            yield row(f'cross-epsilon-{epsilon:x}-{scale:x}', (0,epsilon,0), unit if scale==0x3f800000 else (scale,0,0))
        yield row(f'tail-epsilon-{epsilon:x}', (0,real(100000),0), (epsilon,0,0))
    previous = struct.unpack('<f', struct.pack('<I', 0x38d1b716))[0]
    epsilon = struct.unpack('<f', struct.pack('<I', 0x38d1b717))[0]
    for fraction in (.25, .5, .75, .9, .99, 1.01, 1.1):
        other = real(math.sqrt((epsilon*epsilon-previous*previous)*fraction))
        yield row('unrounded-epsilon-'+str(fraction), (0,0x38d1b716,other), unit)
    special = (0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,
               0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,
               0x7fc10001,0x7fe54321,0xffc23456,0x7f812345,0xffa54321)
    for side in range(2):
        for axis in range(3):
            for bits in special:
                a,b=[0x3f800000,real(.37),real(-.61)],[real(.13),0x3f800000,real(.71)]
                (a if side==0 else b)[axis]=bits
                yield row(f'special-{side}-{axis}-{bits:x}', a,b)
    for a in special[-5:]:
        for b in special[-5:]:
            yield row(f'paired-nan-{a:x}-{b:x}', (a,b,0x3f800000),(b,0x3f800000,a))
    for value in ((0,0,0),(0x80000000,0,0x80000000),(0x3f800000,0,0),
                  (real(.1),real(.2),real(.3)),(0x7f7fffff,0x7f7fffff,0xff7fffff),
                  (0x7fc12345,0x7f812345,0x3f800000)):
        for offset in (0,4,-4,12,-12):
            yield row('alias-'+str(value)+'-'+str(offset), value,value,offset=offset)
        yield row('parallel-negative-'+str(value), value,tuple(v^0x80000000 for v in value))
    yield row('tiny-cross-tail-normalizes',(0,real(1e-10),0),(real(1e5),0,0))
    yield row('normalized-cross-tiny-tail',(0,real(1e10),0),(real(1e-8),0,0))
    rng=random.Random(0x31b30)
    for n in range(96):
        yield row('raw-bits-'+str(n), [rng.getrandbits(32) for _ in range(3)], [rng.getrandbits(32) for _ in range(3)])
    for cw in (0x27f,0x67f,0xa7f,0xe7f,0x7f,0x37f):
        for mxcsr in (0x1f80,0x1fc0,0x9fc0):
            for n,(a,b) in enumerate([((1,0x38d1b716,real(.3)),unit),
                ((0x7f812345,real(.2),0x7fe54321),(real(.3),0x7fc10001,real(.5))),
                ((0,real(.1),real(.2)),(real(.3),real(.4),real(.5)))]):
                yield row(f'host-controls-{cw:x}-{mxcsr:x}-{n}',a,b,cw=cw,mxcsr=mxcsr,status=0x20)
    # 5788545^2 +15746992^2 ==(2^24+1)^2. The square root of the
    # large pair is an exact REAL midpoint. A tiny third square exposes x87
    # sum-order rounding that ordinary composite light vectors rarely reach.
    for fraction in (.1,.3,.49,.51,.7,.9,1.1,1.3,1.49,1.51,1.7,2.1,2.3,2.49,2.7,3.3):
        tiny=real(math.sqrt(fraction)*2**-26)
        for swap in (False,True):
            large=[real(-5788545/2**24),real(15746992/2**24)]
            if swap:large.reverse()
            item=row(f'normalize-midpoint-{fraction}-{swap}',(*large,tiny),(0,0,0))
            item['entry']=0x30bd0;yield item


def physical(image, rows, scratch):
    sys.path.insert(0,str(ROOT/'tools'))
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0x31b30,0x30bd0);common.geometry.STOP=STOP
    metadata=common.metadata(image)
    sites={}
    for site in metadata['sites']:
        if site['instruction'] in sites and sites[site['instruction']]!=site:
            raise ValueError('Conflicting original metadata')
        sites[site['instruction']]=site
    metadata['sites']=list(sites.values())
    directory=scratch/'physical-original';directory.mkdir()
    section=image.section('.text');(directory/'text.bin').write_bytes(image.section_bytes(section))
    (directory/'metadata.json').write_text(json.dumps(metadata))
    (directory/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{directory/'text.bin'}"
.section .constants,"aw"
.space 4096
.section .data,"aw"
.space 8192
.section .stack,"aw"
.space 4096
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''')
    (directory/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .constants 0x46a000:{{*(.constants)}} .data 0x1000000:{{*(.data)}} .stack 0x{STACK:x}:{{*(.stack)}} .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    commands=[['as','--32',str(directory/'reference.s'),'-o',str(directory/'reference.o')],
        ['ld','-melf_i386','-T',str(directory/'link.ld'),str(directory/'reference.o'),'-o',str(directory/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(directory/'observer')]]
    for command in commands:
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError('Original reference build failed: '+result.stderr[-3000:])
    inputs=[]
    constants=image.read(0x46a000,4096)
    for n,row in enumerate(rows):
        stack=bytearray(4096);struct.pack_into('<I',stack,0x800,STOP)
        gp=[UP,row['forward_address'],0x33333333,0x44444444,STACK+0x800,0x66666666,0x77777777,0x88888888]
        inputs.append(dict(id=n,name=row['name'],entry=row.get('entry',0x31b30),gp=gp,matrix=0,source=0,
            before_fp=[row['cw'],row['status'],row['mxcsr']],live_x87=[],
            banks=[dict(address=0x46a000,before=constants.hex()),dict(address=DATA,before=row['bank']),dict(address=STACK,before=stack.hex())]))
    (directory/'inputs.json').write_text(json.dumps(inputs,separators=(',',':')))
    result=subprocess.run([str(directory/'observer'),str(directory/'reference'),str(directory/'metadata.json'),str(directory/'inputs.json'),str(directory/'observed.json')],capture_output=True,text=True,timeout=150)
    if result.returncode:raise ValueError('Original execution failed: '+(result.stdout+result.stderr)[-4000:])
    proof=json.loads((directory/'observed.json').read_text())
    if proof['instruction_patches'] or proof['supplied_original_outcomes']:
        raise ValueError('Original reference supplied instructions or outcomes')
    return proof,dict(backend='physical-unchanged-i386-offline',cases=len(rows),original_instructions=proof['original_instructions'],owners=metadata['owners'])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    if not args.xbe.is_file():print('SKIP missing owned English TU1.5 executable');return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe)
    if image.read(0x46a0d0,4)!=struct.pack('<I',0x3f800000) or image.read(0x46a0ec,4)!=struct.pack('<I',0x38d1b717):
        raise ValueError('Original light vector constants changed')
    rows=list(cases());proof,receipt=physical(image,rows,Path(os.environ['HALO2_TEST_SCRATCH']))
    digest=hashlib.sha256();stores=0;normalized=0;branches=set()
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native.stderr').open('w+') as errors:
        process=subprocess.Popen([str(args.native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,original in zip(rows,proof['cases'],strict=True):
                if not original['completed']:raise ValueError('Original failed to complete '+row['name'])
                if row.get('entry',0x31b30)==0x31b30:
                    sites=set(original['executed_sites']);branches.add((0x31b9c in sites,0x30c02 in sites))
                process.stdin.write(json.dumps(row,separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
                if not line:errors.seek(0);raise ValueError(row['name']+': native stopped: '+errors.read()[-3000:])
                actual=json.loads(line)
                expected_stores=[[s['instruction'],(s['address']-UP)//4,struct.unpack('<I',bytes.fromhex(s['written']))[0]] for s in original['writes'] if UP<=s['address']<UP+12]
                bank=next(b for b in original['banks'] if b['address']==DATA)
                expected_up=list(struct.unpack_from('<3I',bytes.fromhex(bank['after']),UP-DATA))
                expected_reads=[a['address'] for a in original['reads'] if a['address'] in (0x46a0d0,0x46a0ec)]
                expected=dict(up=expected_up,magnitude=original['result_bits'],fp=original['after_fp'],stores=expected_stores,constant_reads=expected_reads,host_fp_and_stack_restored=True)
                for field,value in expected.items():
                    if actual.get(field)!=value:raise ValueError(row['name']+': '+field+' differs; native='+str(actual.get(field))+' original='+str(value))
                rebuilt=bytearray.fromhex(row['bank'])
                for _,axis,value in actual['stores']:struct.pack_into('<I',rebuilt,UP-DATA+axis*4,value)
                if rebuilt.hex()!=bank['after']:raise ValueError(row['name']+': complete vector bank differs')
                stores+=len(expected_stores);normalized+=len(expected_stores)==6
                digest.update(json.dumps(expected,sort_keys=True,separators=(',',':')).encode())
            process.stdin.close()
            if process.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if process.poll() is None:
                process.terminate()
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:process.kill();process.wait()
    if branches!={(False,False),(False,True),(True,False),(True,True)}:raise ValueError('Missing complete light normalization branch combination')
    names=['include/halo2/native_tu15_light_up_vector.hpp','src/rendering/effects/native_tu15_light_up_vector.cpp',
        'include/halo2/detail/native_tu15_arithmetic_environment.hpp','src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp',
        'include/halo2/native_campaign_ai_vector_math_tu15.hpp','include/halo2/native_tu15_normalize_vector.hpp',
        'tests/native_tu15_light_up_vector_tests.cpp',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/validate_native_tu15_light_up_vector.py']
    report=dict(passed=True,cases=len(rows),complete_light_up_cases=sum(r.get('entry',0x31b30)==0x31b30 for r in rows),
        standalone_tail_rounding_cases=sum(r.get('entry')==0x30bd0 for r in rows),
        nonstack_ordered_stores=stores,normalized_tail_cases=normalized,
        comparison_sha256=digest.hexdigest(),original_xbe_sha256=PIN,physical_original=receipt,
        source_sha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in names},
        normalization_branch_combinations=[list(pair) for pair in sorted(branches)],
        instruction_patches=[],supplied_outcomes=[],host_fp_stack_restoration_cases=len(rows),
        scope='Complete31B30 plus30BD0 values, ordered vector publications, magnitude, x87 status and MXCSR; private guest stack residue is intentionally elided.',whole_renderer_visual_parity_verified=False)
    args.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'complete physical original light-vector cases;',stores,'ordered vector stores')
    return 0


if __name__=='__main__':raise SystemExit(main())
