#!/usr/bin/env python3
"""Validate retained original-player bindings against retail instructions/GPU bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, default=ROOT/'tests/fixtures/retail_ivory_player.reference')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer-shading/original-xemu/reference-player-differential.json')
    args = parser.parse_args()
    fields = {}
    for line in args.fixture.read_text().splitlines():
        name, *values = line.split()
        fields[name] = values
    words = lambda name: [int(x,16) for x in fields[name]]
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    xbe=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(xbe.data).hexdigest()!=EXPECTED:raise ValueError('unexpected source executable')
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in xbe.sections:g.mem_write(section.va,xbe.section_bytes(section))
    g.mem_map(0x1000000,0x200000);g.mem_map(0x3000000,4096)
    base=0x1000000;stack=0x11fff00;stop=0x3000000
    def write(at,values):g.mem_write(at,struct.pack('<'+'I'*len(values),*values))
    def read(at,n):return list(struct.unpack('<'+'I'*n,bytes(g.mem_read(at,n*4))))
    def call(at,registers,arguments=()):
        write(stack,[stop,*arguments]);g.reg_write(UC_X86_REG_ESP,stack)
        for register,value in registers.items():g.reg_write(register,value)
        g.emu_start(at,stop,count=20000)
        if g.reg_read(UC_X86_REG_EIP)!=stop:raise RuntimeError(f'original{at:x} failed to return')
    # Original33980 consumes the packed object context via131BB0. Its float
    # reciprocal precedes the second byte packing; GPU then expands bytes.
    write(0x485a58,[base]);write(base+0x64,words('object_packed'));g.mem_write(base+0x74,b'\x04')
    original_colors=[]
    for selector in [3,4,21,22]:
        call(0x33980,{UC_X86_REG_EAX:selector,UC_X86_REG_ESI:base+0x200})
        original_colors+=read(base+0x200,3)
    original_packed=[]
    for i in range(4):
        write(base+0x200,original_colors[i*3:i*3+3]);call(0x131ed0,{},[base+0x200])
        original_packed.append(g.reg_read(UC_X86_REG_EAX))
    # Actual lighting GPU directions corroborate the retained lighting record.
    directions=words('draw_light_directions');write(base+0x3c,directions[:3]);write(base+0x54,directions[3:])
    original_directions=[]
    for selector in [14,16]:
        call(0x33980,{UC_X86_REG_EAX:selector,UC_X86_REG_ESI:base+0x200})
        original_directions+=read(base+0x200,3)
    write(base+0x200,words('aiming'));call(0x1d9370,{UC_X86_REG_EAX:base+0x200})
    original_weight=g.reg_read(UC_X86_REG_XMM0)&0xffffffff
    native=subprocess.run([str(args.probe.resolve()),'--probe',str(args.fixture.resolve())],check=True,text=True,capture_output=True)
    got=[int(x,16)for x in native.stdout.split()]
    expected=original_colors+original_packed+original_directions+[original_weight]
    mismatches=[dict(word=i,original=f'{a:08X}',native=f'{b:08X}')for i,(a,b)in enumerate(zip(expected,got))if a!=b]
    if len(got)!=len(expected):raise RuntimeError('native fixture word count mismatch')
    capture=ROOT/'analysis/multiplayer-shading/original-xemu'
    map_name=fields['map'][0]
    if map_name=='cyclotron':
        directions_events=[848,856,871,879,887];arms_events=[830,838];lighting_events=[848,856,879,887]
        source_file=lambda event,stage:capture/f'fp-{event}-{stage}-0.bin'
    elif map_name=='zanzibar':
        directions_events=[8823,8831,8846,8854,8862];arms_events=[8805,8813];lighting_events=[8823,8831,8854,8862]
        source_file=lambda event,stage:capture/'prt-output-audit'/f'original20-fp-{event}-{stage}-0.bin'
    else:
        raise ValueError('No independently captured GPU binding bank for reference map '+map_name)
    gpu_checks=[]
    def raw(event,stage,at,n):return list(struct.unpack_from('<'+'I'*n,source_file(event,stage).read_bytes(),at))
    for event in directions_events:
        gpu_checks.append(directions==raw(event,0,26*16,3)+raw(event,0,27*16,3))
    for event in arms_events:
        for index,color in [(2,0),(4,1)]:
            rgb=struct.unpack('<3f',struct.pack('<3I',*raw(event,4,544+index*16,3)))
            packed=round(rgb[0]*255)<<16|round(rgb[1]*255)<<8|round(rgb[2]*255)
            gpu_checks.append(packed==original_packed[color])
    lighting=words('lighting')
    def packed_source(offset):
        write(base+0x200,lighting[offset:offset+3]);call(0x131ed0,{},[base+0x200])
        return g.reg_read(UC_X86_REG_EAX)
    for event in lighting_events:
        for index,source in [(4,8),(5,14),(10,0)]:
            rgb=struct.unpack('<3f',struct.pack('<3I',*raw(event,4,544+index*16,3)))
            gpu_checks.append((round(rgb[0]*255)<<16|round(rgb[1]*255)<<8|round(rgb[2]*255))==packed_source(source))
    result=dict(executable_sha256=EXPECTED,fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        source_calls={'33980/131BB0_color':4,'131ED0_pack':4,'33980_direction':2,'1D9370_aiming':1},
        compared_words=len(expected),bit_mismatched_words=mismatches,gpu_binding_checks=len(gpu_checks),
        gpu_binding_matches=sum(gpu_checks),scope='Retained source-player diagnostic inputs and actual GPU lighting/color bindings; no simulation/controller restore')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    if mismatches or not all(gpu_checks):raise SystemExit(1)

if __name__=='__main__':main()
