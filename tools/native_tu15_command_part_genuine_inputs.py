#!/usr/bin/env python3
"""Discover data pages read by genuine45CD0 on pinned retained TU1.5 inputs.

Unicorn is used only to discover input pages. Acceptance executes the unchanged
instructions again on physical i386; no emulator result supplies an outcome.
Original snapshot allocations, geometry, palettes and populated hash tables
retain their addresses. Only the caller stack/packet and FP controls are declared.
"""
import hashlib
import json
from pathlib import Path
import struct
import re
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot

ROOT=Path(__file__).resolve().parents[1]
SNAPSHOTS=(
    ('lockout','analysis/multiplayer-shading/world-effects-goal-20261011/xbox-tu15-frag-capture-v7/physical-ram.bin',
     '1120cd1b7c4838c7b30fc3504a0c233d537e03147a7b5c368a0a4b4fa7b5ff5c'),
    ('waterworks','analysis/vehicles/shading-tu15-20261013/original-session-v2/waterworks-state-v1/physical-ram.bin',
     '4fcfddf1ed195500d999794e3dc0cf875e33b7ab9b42e36bec148ef3dc47898a'),
    ('00a','analysis/opening-cinematic-20261015/original-camera-observer-v1/target-1558/observer-entry-physical-ram.bin',
     '0e2e01629f679b0aac9888bd1acaf74c733467cfaa0a1c17a74fb236ed6b9ee6'))


def collect(image,sp,stop,packet):
    rows=[];witnesses=[];pages=set()
    for name,path,pin in SNAPSHOTS:
        source=ROOT/path;snapshot=Snapshot.__new__(Snapshot)
        snapshot.ram=source.read_bytes();snapshot.cr3=0xf000;snapshot.registers=''
        state=source.parent/'source-state.json'
        if state.is_file():
            snapshot.registers=json.loads(state.read_text()).get('registers_before','')
            cr3=re.search(r'CR3=([0-9a-fA-F]+)',snapshot.registers)
            if cr3:snapshot.cr3=int(cr3[1],16)
        if hashlib.sha256(snapshot.ram).hexdigest()!=pin:raise ValueError('Wrong retained45CD0 snapshot '+name)
        def w(at):return struct.unpack('<I',snapshot.read(at,4))[0]
        def h(at):return struct.unpack('<h',snapshot.read(at,2))[0]
        guest=Uc(UC_ARCH_X86,UC_MODE_32);section=image.section('.text');text=image.section_bytes(section)
        text_first=section.va&~4095;text_last=(section.va+len(text)+4095)&~4095
        guest.mem_map(text_first,text_last-text_first);guest.mem_write(section.va,text)
        guest.mem_map(0x62000000,0x20000);guest.mem_map(stop,4096)
        seen={}
        def unmapped(g,access,at,n,value,data):
            first=at&~4095
            raw=snapshot.read(first,4096);g.mem_map(first,4096);g.mem_write(first,raw);return True
        def read(g,access,at,n,value,data):
            for page in range(at&~4095,(at+n+4095)&~4095,4096):
                if text_first<=page<text_last or 0x62000000<=page<0x62020000:continue
                seen.setdefault(page,snapshot.read(page,4096))
        guest.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);guest.hook_add(UC_HOOK_MEM_READ,read)
        tags,bsp=w(0x4f0414),w(0x4ecc18);count=w(0x4cd398)&65535;counts={}
        for unit in range(min(count,850)):
            at=0x4c6958+32*unit;flags,model,_,packed=struct.unpack('<4I',snapshot.read(at,16))
            if flags&0x400 or not flags&0x1fffff or not flags&1 or (packed>>4)&31!=31 or not snapshot.read(at+0x1a,1)[0]:continue
            index=(packed>>9)&511
            if model!=0xffffffff:
                kind='model';definition=w(tags+16*(model&65535)+8);geometry=w(w(definition+0x28)+92*index+0x34)
            elif flags&0x1000:
                kind='instance';inst=w(bsp+0x144)+88*((packed>>18)&2047);geometry=w(w(bsp+0x13c)+200*h(inst+0x34)+0x50)
            else:kind='cluster';geometry=w(w(bsp+0xa0)+176*index+0x50)
            if not geometry:continue
            parts=struct.unpack('<i',snapshot.read(geometry,4))[0]
            if not 0<=parts<=4096:raise ValueError('Unbounded retained part count')
            for part in range(parts):
                seen.clear();guest.mem_write(sp,struct.pack('<III',stop,part,packed&15))
                initial=bytes((i*37+19)&255 for i in range(44));guest.mem_write(packet,initial)
                guest.reg_write(R.UC_X86_REG_ESP,sp);guest.reg_write(R.UC_X86_REG_EAX,packet);guest.reg_write(R.UC_X86_REG_ECX,unit)
                guest.reg_write(R.UC_X86_REG_FPCW,0x27f);guest.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                guest.emu_start(0x45cd0,stop,count=10000)
                if guest.reg_read(R.UC_X86_REG_EIP)!=stop:raise ValueError('Genuine input discovery escaped whole45CD0')
                stack=bytearray(0x20000);stack[packet-0x62000000:packet-0x62000000+44]=initial
                banks=[dict(address=page,before=raw.hex())for page,raw in sorted(seen.items())]
                banks.append(dict(address=0x62000000,before=stack.hex()));pages.update(seen)
                for cw in (0x27f,0x23f):
                    rows.append(dict(name=f'genuine-{name}-{kind}-u{unit}-p{part}-cw{cw:x}',entry=0x45cd0,
                        gp=[packet,unit,0,0,sp,0,0,0],matrix=part,source=packed&15,packet=packet,
                        before_fp=[cw,0,0x1f80],live_x87=[],max_steps=10000,banks=banks,
                        genuine=dict(snapshot=path,snapshot_sha256=pin,kind=kind,unit=unit,part=part)))
                counts[kind]=counts.get(kind,0)+1
        witnesses.append(dict(name=name,path=path,sha256=pin,parts=counts,cr3=snapshot.cr3))
    return rows,witnesses,pages
