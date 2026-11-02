#!/usr/bin/env python3
"""Whole unchanged44E80 model cull: original shared-mask/flag chronology.

Use run_test.py. Complete genuine pose composition and frustum predicates run
on physical i386, without patched instructions or supplied callee outcomes.
All executable copies, raw banks and compiler outputs belong to test scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
POOL,OBJROWS,TAGROWS,MODEL,INV,SECTIONS=0x1000000,0x1000800,0x1001000,0x1002000,0x1002800,0x1003000
BODY,GEOM,SUBPART,BOUND,DRAWCONTEXT,MASK,CTX,COLLECT=0x1004000,0x1006000,0x1006800,0x1007000,0x1008000,0x1009000,0x1010000,0x1014000
SP,STOP=0x62018000,0x63000000
SECTIONS_MAP={'constant':(0x46a000,4096),'units':(0x4c6000,8192),'context':(0x4cd000,4096),
    'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),'data':(0x1000000,0x20000),'stack':(0x62000000,0x20000)}


def inputs(image):
    rows=[]
    for n,mask in enumerate((0,1)):
        banks=[dict(address=at,before=bytearray(size))for at,size in SECTIONS_MAP.values()]
        def put(at,value,size=4):
            for bank in banks:
                if bank['address']<=at and at+size<=bank['address']+len(bank['before']):
                    bank['before'][at-bank['address']:at-bank['address']+size]=int(value&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Unmapped model-cull input '+hex(at))
        def floats(at,values):
            for i,v in enumerate(values):put(at+4*i,struct.unpack('<I',struct.pack('<f',v))[0])
        for bank in banks:
            if bank['address']==0x46a000:bank['before'][:]=image.read(0x46a000,4096)
        put(0x4ecbd0,POOL);put(POOL+0x44,OBJROWS);put(OBJROWS+8,BODY);put(0x4f0414,TAGROWS);put(TAGROWS+8,MODEL)
        put(MODEL+0x28,SECTIONS);put(MODEL+0x4c,INV);put(SECTIONS+0x34,GEOM)
        put(BODY+0x114,52,2);put(BODY+0x116,0x200,2)
        identity=[1,1,0,0,0,1,0,0,0,1,0,0,0];floats(BODY+0x200,identity);floats(INV+0x28,identity)
        put(GEOM+8,1);put(GEOM+0xc,SUBPART);put(GEOM+0x14,BOUND);floats(BOUND,[0,0,-2,.5]);put(BOUND+0x10,0,1)
        put(DRAWCONTEXT,0xbeef0000);put(MASK,mask);put(0x4c6958,0x40);put(0x4c695c,0xaabb0000);put(0x4c6968,DRAWCONTEXT);put(0x4c696c,MASK)
        put(0x4cd3a0,CTX);put(CTX,COLLECT);put(COLLECT,1,2);put(COLLECT+0xa70,1,2);put(COLLECT+0xa7c,0,2)
        frustum=COLLECT+0x1974;floats(frustum+0x60,[-5,5,-5,5,-5,5])
        for i,plane in enumerate(((1,0,0,5),(-1,0,0,5),(0,1,0,5),(0,-1,0,5),(0,0,1,5),(0,0,-1,5))):floats(frustum+0x78+16*i,plane)
        rows.append(dict(id=n,name='model-mask-'+str(mask),entry=0x44e80,stop_pc=STOP,max_steps=20000,
            gp=[0,0,0,0,SP,0,0,0],matrix=0,source=0,before_fp=[0x27f,0,0x1f80],live_x87=[],
            banks=[dict(address=b['address'],before=b['before'].hex())for b in banks]))
    return rows


def prepare(image,scratch,entries=(0x44e80,0x145560,0x167ef0,0x184690,0x32a660)):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=entries;meta=common.metadata(image);meta['stop']=STOP
    meta['sites']=list({s['instruction']:s for s in meta['sites']}.values());(scratch/'metadata.json').write_text(json.dumps(meta))
    section=image.section('.text');(scratch/'text.bin').write_bytes(image.section_bytes(section))
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{scratch/"text.bin"}"\n'
    for name,(_,size)in SECTIONS_MAP.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';(scratch/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+ ' '.join(f'.{name} 0x{at:x}:{{*(.{name})}}'for name,(at,_)in SECTIONS_MAP.items())+f' .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(scratch/'link.ld').write_text(linker)
    for cmd in (['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(scratch/'observer')]):
        r=subprocess.run(cmd,capture_output=True,text=True)
        if r.returncode:raise ValueError(r.stderr[-2000:])
    return meta


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if not a.xbe.is_file():return 77
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);meta=prepare(image,scratch);rows=inputs(image)
    (scratch/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=120)
    if r.returncode:raise ValueError((r.stdout+r.stderr)[-2500:])
    proof=json.loads((scratch/'observed.json').read_text());result=[]
    for row,actual in zip(rows,proof['cases'],strict=True):
        def read(at):
            for b in actual['banks']:
                after=bytes.fromhex(b['after'])
                if b['address']<=at and at+4<=b['address']+len(after):return int.from_bytes(after[at-b['address']:at-b['address']+4],'little')
            raise ValueError(hex(at))
        result.append(dict(input_mask=row['id'],completed=actual['completed'],unit_flags=read(0x4c6958),mask=read(MASK),original_instructions=actual['original_instructions']))
    passed=all(c['completed']for c in result)and result[0]['unit_flags']==0x41 and result[1]['unit_flags']==0x40
    out=dict(passed=passed,original_xbe_sha256=PIN,owners=meta['owners'],cases=result,instruction_patches=proof['instruction_patches'],
        supplied_original_outcomes=proof['supplied_original_outcomes'],declared_inputs_sha256=[hashlib.sha256(json.dumps(row,sort_keys=True).encode()).hexdigest()for row in rows],
        owned_scratch=str(scratch),retained_large_artifacts=[],
        finding='Whole44E80 posed-model linear path tests the incoming shared mask. An already-selected bit skips the bound and leaves flag1 unset; an empty mask admits the same genuine sphere and sets flag1. Deferring native BSP mask writes until after the residual model loop therefore needs full source-order qualification; OR mask equality alone cannot establish equivalence.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));return int(not passed)


if __name__=='__main__':raise SystemExit(main())
