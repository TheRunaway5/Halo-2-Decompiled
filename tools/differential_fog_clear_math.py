#!/usr/bin/env python3
"""Unchanged original12E68F RGB tail and131ED0 differential fixtures."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_bsp_clear_color import (Cache,setup,invoke,STATE,CONTROL,STACK,
    UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_MXCSR,UC_X86_REG_FPCW)
from unicorn.x86_const import UC_X86_REG_ESI

def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def scalar(word):return struct.unpack('<f',struct.pack('<I',word))[0]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--native-binary',type=Path)
    parser.add_argument('--include-reserved-bit-zero',action='store_true',
        help='Also exercise the reserved-bit6 encoding observed in original live FCW023F')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    cache=Cache(ROOT/'assets/map-pack/maps/backwash.map');g,_,_=setup(cache)
    rng=random.Random(0x12e68f);rows=[];tail_count=0;pack_count=0
    controls=[(pc<<8)|(rc<<10)|mask
        for mask in ((0x3f,0x7f) if args.include_reserved_bit_zero else (0x7f,))
        for pc in (0,2,3) for rc in range(4)]
    # All meaningful ordered branch boundaries, plus nonboolean low bytes,
    # signed zero, retained RGB and unclamped copied colors.
    for model,planar,p,s,t,a in itertools.product((0,1,2,255),(0.,.5),
            (0.,.25,1.),(0.,.5,1.),(0.,.3,1.),(0.,.5,1.,scalar(0x3f800001))):
        color=[rng.choice((-2.,-0.,0.,.2,.9,1.,2.)) for _ in range(12)]
        prior,sky,primary,secondary=[color[i:i+3] for i in range(0,12,3)]
        state=bytearray(0x120)
        for offset,values in ((4,prior),(0x5c,sky),(0x68,[a]),
            (0x1c,primary+[p]),(0x34,secondary+[s]),(0xac,[planar]),(0x11c,[t])):
            struct.pack_into('<'+'f'*len(values),state,offset,*values)
        g.mem_write(STATE,bytes(state));g.mem_write(STACK,bytes(32))
        g.mem_write(STACK+0x1c,bytes([model]));g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_ESI,STATE);g.reg_write(UC_X86_REG_EAX,0)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
        g.emu_start(0x12e68f,0x12e8aa,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==0x12e8aa
        expected=struct.unpack('<3I',g.mem_read(STATE+4,12))
        words=[model,*map(bits,prior+sky+[a]+primary+[p]+secondary+[s,planar,t]),*expected]
        rows.append('T tail-'+str(tail_count)+' '+' '.join(f'{v:08X}' for v in words))
        tail_count+=1
    values=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,
        0xbf800000,0x40000000,0xc0000000,0x7f7fffff,0xff7fffff,
        0x7f800000,0xff800000,0x7fc00000,0x7f800001]
    for n in range(-256,512):
        word=bits((n+.5)/255.)
        values.extend(((word-1)&0xffffffff,word,(word+1)&0xffffffff))
    values.extend(rng.getrandbits(32) for _ in range(512))
    for control in controls:
        for i,word in enumerate(values):
            rgb=[word,values[(i+37)%len(values)],values[(i+137)%len(values)]]
            g.mem_write(CONTROL,struct.pack('<3I',*rgb))
            invoke(g,0x131ed0,(CONTROL,))
            # invoke supplies the ordinary helper context; rerun with the
            # explicit test CW so every precision and rounding case is real.
            g.reg_write(UC_X86_REG_FPCW,control)
            g.reg_write(UC_X86_REG_ESP,STACK)
            g.emu_start(0x131ed0,0x3000000,count=10000)
            assert g.reg_read(UC_X86_REG_EIP)==0x3000000
            expected=g.reg_read(UC_X86_REG_EAX)
            rows.append('P pack-'+str(pack_count)+' '+' '.join(f'{v:08X}' for v in [control,*rgb,expected]))
            pack_count+=1
    fixture=out/'source_fog_clear.tsv';fixture.write_text('\n'.join(rows)+'\n')
    cache.close()
    inputs=[ROOT/'assets/disc/default.xbe',ROOT/'assets/map-pack/maps/backwash.map',
        Path(__file__).resolve(),ROOT/'tools/differential_bsp_clear_color.py']
    report={'tail_cases':tail_count,'packing_cases':pack_count,'x87_controls':[f'{v:04X}' for v in controls],
        'original_instruction_replacements':[],'fixture_sha256':digest(fixture),
        'inputs':{str(p):digest(p) for p in inputs},'framebuffer_parity':False}
    if args.native_binary:
        result=subprocess.run([str(args.native_binary.resolve()),'--math',str(fixture)],cwd=ROOT,text=True,capture_output=True)
        (out/'native.log').write_text(result.stdout+result.stderr)
        report.update(native_binary=str(args.native_binary.resolve()),native_sha256=digest(args.native_binary),
            passed=result.returncode==0,exit_code=result.returncode)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    return 0 if report.get('passed',True) else 1

if __name__=='__main__':raise SystemExit(main())
