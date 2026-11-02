#!/usr/bin/env python3
"""Physical whole2E79A0 query0 FP/order diagnostic; run through run_test.py.

Declared bytecode/header/planes are input, never supplied call outcomes. Raw
banks, ELFs, observer and compiler output belong to HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import itertools
import json
import os
import random
import shutil
from pathlib import Path
import struct
import subprocess
import sys
import oracle_native_tu15_model_mask_order as base

ROOT=Path(__file__).resolve().parents[1]
DATA=0x1000000;PH=DATA+0x1000;PLANES=DATA+0x1100;DIRECT=DATA+0x2000;RANGES=DATA+0x4000
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=base.PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    frozen=scratch/'source';frozen.mkdir();shutil.copytree(ROOT/'include',frozen/'include')
    names=['tests/native_tu15_bsp_accelerator_fp_tests.cpp','src/rendering/world/source_bsp_accelerator.cpp','src/assets/cache.cpp']
    pins={name:sha(ROOT/name)for name in names}
    for name in names:
        target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    include_pins={str(p.relative_to(frozen)):sha(p)for p in sorted((frozen/'include').rglob('*'))if p.is_file()}
    include_digest=hashlib.sha256(json.dumps(include_pins,sort_keys=True).encode()).hexdigest()

    base.SECTIONS_MAP['extra_constants']=(0x41c000,4096)
    base.SECTIONS_MAP['absolute_constants']=(0x461000,4096)
    meta=base.prepare(image,scratch,(0x2e79a0,0x2e6290,0x309600,0x3092a0,0x2e3250,0x2e3080,0x2e30e0))
    rows=[]
    parameters=list(itertools.product(((0,),(0x31,),(0x26,1,2,0x31),(0x27,3,7,0x31),(0x28,11,13,0x31),
        (0x10,1,2,1,0x31,0x32),(0x11,3,7,1,0x31,0x32),(0x12,11,13,1,0x31,0x32),
        (0x10,5,7,4,0x26,1,3,0x31,0x27,2,11,0x32),(0x10,5,7,4,0x26,1,3,0x31,0x27,2,7,0x32),(0x10,3,7,6,0x11,5,11,1,0x31,0x32,0x33),
        (0x20,7,1,0x31,0x32),(0x21,7,1,0x31,0x32),(0x22,7,1,0x31,0x32),
        (0x23,1,7,0,0,0,1,0x31,0x32),(0x29,0,0,3,0,0,7,0x31),
        (1,13,17,19,0x10,7,11,1,0x31,0x32),(2,23,29,31,0x20,7,1,0x31,0x32)),
        (3.,7.,65536.),(0x27f,0x67f,0xa7f,0xe7f),((),((1,0,0,-1),),((1,0,0,-5),(-1,0,0,-5),(0,1,0,-5),(0,-1,0,-5),(0,0,1,-5),(0,0,-1,-5)))))
    rng=random.Random(0x2e79a02e6290)
    def node(depth):
        if not depth:
            result=[]
            for axis in range(3):
                lower=rng.randrange(64);result.extend((0x26+axis,lower,lower+rng.randrange(1,190)))
            return result+[0x30+rng.randrange(16)]
        left,right=node(depth-1),node(depth-1);axis=rng.randrange(3)
        if rng.randrange(2):return [0x20+axis,rng.randrange(200),len(left),*left,*right]
        return [0x10+axis,rng.randrange(100),rng.randrange(100,220),len(left),*left,*right]
    for index in range(64):
        cutoff=rng.uniform(10000,1000000)
        planes=((1,0,0,-cutoff),(-1,0,0,-cutoff),(0,1,0,-cutoff),(0,-1,0,-cutoff),(0,0,1,-cutoff),(0,0,-1,-cutoff))
        parameters.append((node(3),rng.choice((3.,7.,11.)),0x27f,planes))
    for code,scale,cw,planes in parameters:
        data=bytearray(0x20000);tree=bytearray(48);struct.pack_into('<f',tree,12,scale);tree+=bytes(code);data[:len(tree)]=tree
        # Zero planes exercises every bound/accept path without inventing
        # a classification result. Actual endpoint owners still run.
        struct.pack_into('<II',data,PH-DATA,len(planes),PLANES)
        for i,plane in enumerate(planes):struct.pack_into('<4f',data,PLANES-DATA+16*i,*plane)
        for header in (DIRECT,RANGES):struct.pack_into('<III',data,header-DATA,header+16,0,0x80000400)
        stack=bytearray(0x20000);struct.pack_into('<II',stack,base.SP-0x62000000+12,DIRECT,RANGES)
        rows.append(dict(id=len(rows),name='accelerator-fp-'+str(len(rows)),entry=0x2e79a0,stop_pc=base.STOP,max_steps=50000,
            gp=[0,0,0,0,base.SP,0,0,0],matrix=DATA,source=PH,before_fp=[cw,0,0x1f80],live_x87=[],tree=tree.hex(),planes=[list(map(bits,p))for p in planes],
            banks=[dict(address=0x46a000,before=image.read(0x46a000,4096).hex()),dict(address=0x41c000,before=image.read(0x41c000,4096).hex()),dict(address=0x461000,before=image.read(0x461000,4096).hex()),
                dict(address=DATA,before=data.hex()),dict(address=0x62000000,before=stack.hex())]))
    (scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=180)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text());assert not proof['instruction_patches']and not proof['supplied_original_outcomes']
    native=scratch/'native-accelerator-fp';command=['g++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I',str(frozen/'include'),
        str(frozen/'tests/native_tu15_bsp_accelerator_fp_tests.cpp'),str(frozen/'src/rendering/world/source_bsp_accelerator.cpp'),str(frozen/'src/assets/cache.cpp'),'-o',str(native)]
    r=subprocess.run(command,capture_output=True,text=True,timeout=90)
    if r.returncode:raise ValueError(r.stderr[-3000:])
    differences=[];compared=0;status_counts={};observations=[]
    process=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    try:
        for row,original in zip(rows,proof['cases'],strict=True):
            assert original['completed'];data=bytes.fromhex(next(b['after']for b in original['banks']if b['address']==DATA))
            def bank(header):
                count=struct.unpack_from('<I',data,header-DATA+4)[0];assert count<1024
                return list(struct.unpack_from('<'+'I'*count,data,header-DATA+16))
            expected=dict(complete=True,direct=bank(DIRECT),ranges=bank(RANGES),fp=original['after_fp'])
            process.stdin.write(json.dumps({k:row[k]for k in ('tree','planes','before_fp')},separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
            if not line:raise ValueError('Native accelerator driver ended')
            actual=json.loads(line);compared+=1
            key=str(expected['fp'][1]);status_counts[key]=status_counts.get(key,0)+1
            observations.append(dict(tree=row['tree'],planes=row['planes'],before_fp=row['before_fp'],expected=expected))
            if actual!=expected:differences.append(dict(case=row['name'],tree=row['tree'],before_fp=row['before_fp'],actual=actual,expected=expected))
        process.stdin.close();assert process.wait(timeout=10)==0
    finally:
        if process.poll()is None:process.terminate();process.wait(timeout=5)
    report=dict(passed=not differences,physical_cases=len(rows),comparisons=compared,original_instructions=proof['original_instructions'],original_xbe_sha256=base.PIN,
        owners=meta['owners'],physical_x87_status_counts=status_counts,observations=observations,differences=differences[:12],difference_count=len(differences),instruction_patches=[],supplied_original_outcomes=False,
        native_source_sha256=pins['src/rendering/world/source_bsp_accelerator.cpp'],source_sha256=pins,frozen_include_inventory_sha256=include_digest,helper_sha256=sha(__file__),source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),command=command,owned_scratch=str(scratch),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('physical accelerator FP',len(rows),'cases,',len(differences),'differences')
    return int(bool(differences))
if __name__=='__main__':raise SystemExit(main())
