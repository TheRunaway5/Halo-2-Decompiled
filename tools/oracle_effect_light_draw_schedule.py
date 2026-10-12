#!/usr/bin/env python3
"""Observe unchanged31C80 receiver collectors and44720 before first31170 draw.

The retained retail RAM supplies the actual main prefix, light, camera,
geometry residency, fog and dynamic object banks. Only a declared call stack
and C15A0 per-view update are supplied. No instruction or callee is replaced.
This records receiver draw-unit/mask ownership; it does not execute GPU draws.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--prepare-commands',action='store_true',help='Include whole40E90/44AC0 part-command preparation; stop before light uniforms/GPU calls')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    s=Snapshot(a.snapshot);digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    g=s.guest();g.mem_map(0x70000000,0x100000);stack,stop=0x700fe000,0x700fff00
    def read(at,n):
        regions=list(g.mem_regions())
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(begin<=page<=end for begin,end,_ in regions):
                g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
        return bytes(g.mem_read(at,n))
    def call(entry,end):
        g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack)
        g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        g.emu_start(entry,end,count=10000000)
        assert g.reg_read(R.UC_X86_REG_EIP)==end,hex(g.reg_read(R.UC_X86_REG_EIP))
    rows=[];calls={};points={0x31c80:'main-input',0x2cb30:'receiver-admission',0x133390:'clusters',
        0x133520:'instances',0x133720:'objects',0x44720:'cull-input'}
    if a.prepare_commands:points[0x31170]='cull-output'
    def capture(label):
        context=u(read(0x4c0b80,4));prefix=u(read(0x4c0b7c,4));count=u(read(0x4c0b78,4))
        used=struct.unpack('<H',read(context+0xa5c,2))[0]
        assert prefix<=count<=850 and used<=512
        units=[list(struct.unpack('<8I',read(0x4ba138+i*32,32))) for i in range(count)]
        arena=read(context+0x25c,512*4)
        (a.output/(label+'-units.bin')).write_bytes(read(0x4ba138,count*32))
        (a.output/(label+'-masks.bin')).write_bytes(arena)
        rows.append(dict(point=label,context=f'{context:08x}',main_prefix=prefix,draw_count=count,mask_used=used,
            units=units,mask_words=list(struct.unpack('<512I',arena))))
    def trace(_guest,at,_size,_data):
        if at in points:capture(points[at])
        calls[f'{at:08x}']=calls.get(f'{at:08x}',0)+1
    for at in (*points,0x2cbf0,0x2d000,0x1332b0,0x45560,0x40e90,0x44ac0,0x45ce0,0x460d0,0x41040,0x1bf50,0x40e30,0x3b9d0):g.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
    call(0xc15a0,stop);call(0x31c80,0x31197 if a.prepare_commands else 0x31170)
    if not a.prepare_commands:capture('cull-output')
    report=dict(boundary=__doc__,snapshot_sha256=digest,intercepted_instructions=0,points=rows,calls=calls,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    if a.prepare_commands:
        header=list(struct.unpack('<6I',read(0x4c1a60,24)))
        assert header[1]<=header[0]<=65536
        command_bytes=read(header[5],header[1]*24)
        (a.output/'part-commands.bin').write_bytes(command_bytes)
        report['part_commands']=dict(header=header,commands=[list(struct.unpack_from('<6I',command_bytes,i*24))for i in range(header[1])],
            stopped_at='00031197',light_uniforms_or_GPU_executed=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(calls=calls,points=[{k:v for k,v in r.items() if k not in ('units','mask_words')} for r in rows]),indent=2))
if __name__=='__main__':main()
