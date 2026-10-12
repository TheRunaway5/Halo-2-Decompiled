#!/usr/bin/env python3
"""Unchanged standalone sphericalC2D00/C3220 with actual Lockout BSP banks.

Cold named spatial pools and explicitly varied light fields/definitions are
declared inputs. Original collision correction, location queries, geometry
membership, allocators and erasure execute unchanged. The native consumes the
actual16A440 contact bank retained during replay; its production is not proved.
No callee is replaced. This does not supply live light-allocation chronology.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot, u


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    s=Snapshot(args.snapshot);digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    bsp=u(s.read(0x4e0348,4));collision=u(s.read(0x4e033c,4))
    clusters,cluster_ptr=struct.unpack('<II',s.read(bsp+0x9c,8))
    selected=struct.unpack('<h',s.read(0x4686c4,2))[0]
    cases=128;fixture=bytearray(struct.pack('<4I',0x3152474c,cases,selected&65535,clusters))
    def append_bank(at,stride):
        count,ptr=struct.unpack('<II',s.read(at,8))
        fixture.extend(struct.pack('<I',count)+s.read(ptr,count*stride));return count,ptr
    append_bank(collision,8);append_bank(collision+8,16)
    leaves,ptr=struct.unpack('<II',s.read(bsp+0x2c,8))
    fixture+=struct.pack('<I',leaves)
    for n in range(leaves):fixture+=s.read(ptr+n*8,2)
    for n in range(clusters):append_bank(cluster_ptr+n*176+0x8c,2)
    portals,ptr=struct.unpack('<II',s.read(bsp+0x5c,8))
    fixture+=struct.pack('<I',portals)
    for n in range(portals):
        fixture+=s.read(ptr+n*36,24);append_bank(ptr+n*36+0x1c,12)
    light_pool=u(s.read(0x4e030c,4));light=u(s.read(light_pool+0x44,4))+21*272
    before=s.read(light,272);center=struct.unpack_from('<3f',before,0x84)
    spatial_headers=[s.read(u(s.read(a,4)),76) for a in (0x4e0314,0x4e0318)]
    rng=random.Random(0xc2d00);calls={};adjusted=0;nonempty=0
    for case in range(cases):
        g=s.guest();g.mem_map(0x70000000,0x100000)
        def materialize(at,length):
            for page in range(at&~4095,(at+length+4095)&~4095,4096):
                try:g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
                except Exception as e:
                    if 'Invalid memory mapping' not in str(e):raise
        for at,length in ((0x4e0310,12),(light,272)):materialize(at,length)
        heads=0x70010000;pools=[0x70011000,0x70012000]
        records=[0x70020000,0x70030000];bitmaps=[0x70013000,0x70013100]
        stack,stop=0x700fe000,0x700fff00
        g.mem_write(0x4e0310,struct.pack('<3I',heads,*pools));g.mem_write(heads,b'\xff'*(clusters*4))
        contacts=[]
        def trace(guest,address,size,data):
            calls[f'{address:08x}']=calls.get(f'{address:08x}',0)+1
            if address==0x16a994:
                packet=guest.reg_read(R.UC_X86_REG_ESP)+0x168
                a,b,c=struct.unpack('<3h',guest.mem_read(packet,6))
                assert all(0<=v<=256 for v in (a,b,c))
                contacts.append(struct.pack('<3I',a,b,c)+bytes(guest.mem_read(packet+8,a*32))+
                    bytes(guest.mem_read(packet+0x2008,b*44))+bytes(guest.mem_read(packet+0x4c08,c*108)))
        for at in (0xc2d00,0xc3220,0xc28b0,0x16a8e0,0x16a994,0x11bed0,0x1cac60,0x14a5b0,0x1cae40):
            g.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
        def call(address,arguments=(),registers=()):
            g.mem_write(stack,struct.pack('<'+'I'*(1+len(arguments)),stop,*arguments))
            g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
            for reg,value in registers:g.reg_write(reg,value)
            g.emu_start(address,stop,count=100000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==stop,(case,hex(g.reg_read(R.UC_X86_REG_EIP)))
        for i in range(2):
            h=bytearray(spatial_headers[i]);h[0x2a]&=~8
            struct.pack_into('<II',h,0x44,records[i],bitmaps[i]);g.mem_write(pools[i],bytes(h))
            g.mem_write(records[i],bytes(2048*12));call(0x16b7a0,registers=((R.UC_X86_REG_ESI,pools[i]),))
        tag=(0xe63a04c5,0xe694051f)[case&1]
        definition=u(s.read(u(s.read(0x4e3b44,4))+(tag&65535)*16+8,4));materialize(definition,228)
        raw=bytearray(s.read(definition,228))
        if case%4==2:struct.pack_into('<I',raw,0,u(raw)&~0x8000)
        if case%4==3:
            struct.pack_into('<I',raw,0,u(raw)&~2)
            struct.pack_into('<f',raw,0xc,1.5);struct.pack_into('<f',raw,0x1c,3.)
        g.mem_write(definition,bytes(raw));fixture+=raw
        state=bytearray(before);struct.pack_into('<I',state,4,tag)
        struct.pack_into('<H',state,2,[0,1,2,3,0xb,0x13,0x1b,0x11][case%8])
        struct.pack_into('<I',state,0x10,0xffffffff)
        struct.pack_into('<3f',state,0x84,*(x+rng.uniform(-3,3) for x in center))
        struct.pack_into('<h',state,0x48,[-1,0,9][case%3])
        struct.pack_into('<I',state,0x4c,0xffffffff);struct.pack_into('<h',state,0x54,-1)
        struct.pack_into('<I',state,0x58,0xffffffff);struct.pack_into('<h',state,0x5c,-1)
        struct.pack_into('<f',state,0x9c,.5 if case%5==0 else 0)
        struct.pack_into('<3f',state,0xa0,*(x+rng.uniform(-1,1) for x in center))
        g.mem_write(light,bytes(state));fixture+=state
        call(0xc2d00,(0xe9810015,))
        after=bytes(g.mem_read(light,272));fixture+=contacts[0] if contacts else bytes(12)
        fixture+=after;adjusted+=after[0x18:0x24]!=after[0x28:0x34];nonempty+=u(after,0x10)!=0xffffffff
        call(0xc3220,registers=((R.UC_X86_REG_EAX,0xe9810015),))
        fixture+=bytes(g.mem_read(light,272))
        fixture+=bytes(g.mem_read(heads,clusters*4))
        for i in range(2):
            for at in (0x34,0x38,0x3c):fixture+=bytes(g.mem_read(pools[i]+at,4))
            fixture+=struct.pack('<I',struct.unpack('<H',g.mem_read(pools[i]+0x40,2))[0])
            fixture+=bytes(g.mem_read(bitmaps[i],256))+bytes(g.mem_read(records[i],2048*12))
        if case%16==15:print(case+1,'whole registration/unregistration pairs',flush=True)
    (args.output/'fixture.bin').write_bytes(fixture)
    report=dict(boundary=__doc__,cases=cases,registered_nonempty=nonempty,corrected_centers=adjusted,
        calls=calls,snapshot_sha256=digest,intercepted_instructions=0,
        fixture_sha256=hashlib.sha256(fixture).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
