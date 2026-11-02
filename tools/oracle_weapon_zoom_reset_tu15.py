#!/usr/bin/env python3
"""Unchanged CABC0 reset, its committed CAC96 decision and actual call ABIs.

BCDF0/106040/18C4B0 are declared callee inputs. No original instructions are
patched; production still calls real owners and retains their actual results.
Run through tools/run_test.py. Only the bounded expected-data fixture survives.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import sys
sys.dont_write_bytecode=True
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_EFLAGS
from oracle_weapon_effect_dispatch_tu15 import GP, STACK, STOP, NONE, pack
from weapon_reference_versions import images

OBJECT_POOL,HEADERS,UNIT,WEAPON,PLAYERS,PLAYER_ROWS,USERS,TAGS,WEAP_DEF,GLOBALS,INFO = [0x1000000+n*0x2000 for n in range(11)]
SERVICES={0xbcdf0:1,0x106040:0,0x18c4b0:1}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5')
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(0x1000000,0x100000);g.mem_map(STACK-4096,8192);g.mem_map(STOP,4096)
    instructions={};calls=[];stores=[];events=[];active=None;observed_player=NONE;observed_scoped=False
    rng=random.Random(0xcabc0)
    def code(g,at,size,_):
        nonlocal observed_player,observed_scoped
        if at in SERVICES:
            sp=g.reg_read(GP[4]);gp=[g.reg_read(r) for r in GP]
            returned=[rng.getrandbits(32) for _ in range(3)]
            if at==0xbcdf0:returned[0]=0 if active['owner_missing'] else UNIT
            elif at==0x106040:returned[0]=(returned[0]&0xffffff00)|active['predicate']
            xmm0=rng.randbytes(16).hex()
            ret=struct.unpack('<I',g.mem_read(sp,4))[0]
            calls.append(dict(target=at,tail=ret==STOP,gp=gp,
                xmm=[g.reg_read(UC_X86_REG_XMM0+n).to_bytes(16,'little').hex() for n in range(8)],
                stack=bytes(g.mem_read(STACK-64,96)).hex(),returned_gp=returned,returned_xmm0=xmm0))
            for reg,value in zip(GP[:3],returned):g.reg_write(reg,value)
            g.reg_write(UC_X86_REG_XMM0,int.from_bytes(bytes.fromhex(xmm0),'little'))
            g.reg_write(GP[4],sp+4+SERVICES[at]*4);g.reg_write(UC_X86_REG_EIP,ret);return
        raw=bytes(g.mem_read(at,size))
        if raw!=image.read(at,size):raise ValueError('Original instruction changed')
        instructions[f'{at:08x}']=raw.hex()
        if at==0xcac17:observed_player=g.reg_read(GP[1])
        if at==0xcac60:observed_scoped=bool(g.reg_read(GP[0])&255)
        if at==0xcac96 and g.reg_read(GP[2])&255:
            events.append(dict(unit=active['gp'][0],player=observed_player,weapon=g.reg_read(GP[7]),
                previous_selector=active['previous_selector'],request=active['request'],weapon_scoped=observed_scoped,
                source_local_user=struct.unpack('<i',pack([g.reg_read(GP[3])]))[0],committed_stores=len(stores)))
    def write(g,access,at,size,value,_):stores.append([g.reg_read(UC_X86_REG_EIP),at,size,value&((1<<(size*8))-1)])
    g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_WRITE,write)
    rows=[]
    configurations=itertools.product((-1,0,1),(-1,0,3),(False,True),(False,True),(0,1,0x80,0xff))
    for n,(slot,local,scoped,argument,previous) in enumerate(configurations):
        gp=[rng.getrandbits(32) for _ in GP];gp[4]=STACK;gp[0]=0xabcd0003
        request=(1,0x80,0xff)[n%3] if argument else 0
        weapon=NONE if n%11==0 else 0x98760005;player=0x76540007
        predicate=(1,0x80,0xff)[(n//3)%3] if scoped else 0
        memory=[[0x4ecbd0,OBJECT_POOL,4],[OBJECT_POOL+0x44,HEADERS,4],
            [HEADERS+3*12+8,UNIT,4],[HEADERS+5*12+8,WEAPON,4],
            [UNIT+0x13c,player if n%17 else NONE,4],[UNIT+0x212,slot&255,1],
            [UNIT+0x218+slot*4,weapon,4],[UNIT+0x240,previous,1],
            [UNIT+0x241,rng.getrandbits(8),1],[UNIT+0x26c,rng.getrandbits(32),4],
            [0x4f54f4,PLAYERS,4],[PLAYERS+0x44,PLAYER_ROWS,4],[PLAYER_ROWS+7*0x21c+0x28,local&65535,2],
            [0x4f9b54,USERS,4],[0x4f0414,TAGS,4],[WEAPON,0x43210009,4],
            [TAGS+9*16+8,WEAP_DEF,4],[WEAP_DEF+0x278,NONE if n%5==0 else 0x5432000b,4],
            [0x4ecc1c,GLOBALS,4],[GLOBALS+0x134,INFO,4],[INFO+0xd0,NONE if n%7==0 else 0x6543000c,4],
            [0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'),4]]
        if local!=-1:memory.extend([[USERS+local*0x94+0x42,rng.getrandbits(16),2],
                                   [USERS+local*0x94+0x9d,rng.getrandbits(8),1]])
        xmm=[rng.randbytes(16).hex() for _ in range(8)]
        stack=rng.randbytes(96);g.mem_write(STACK-64,stack);g.mem_write(STACK,pack([STOP,(rng.getrandbits(24)<<8)|request]))
        for at,value,width in memory:g.mem_write(at,value.to_bytes(width,'little'))
        for reg,value in zip(GP,gp):g.reg_write(reg,value)
        for i,raw in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(bytes.fromhex(raw),'little'))
        g.reg_write(UC_X86_REG_EFLAGS,0x202)
        active=dict(gp=gp,xmm=xmm,memory=memory,initial_stack=bytes(g.mem_read(STACK-64,96)).hex(),
            owner_missing=n%19==0,predicate=predicate,request=request,previous_selector=previous)
        calls.clear();stores.clear();events.clear();observed_player=NONE;observed_scoped=False
        g.emu_start(0xcabc0,STOP,count=1000)
        if g.reg_read(UC_X86_REG_EIP)!=STOP:raise ValueError('Original did not complete')
        active.update(expected_gp=[g.reg_read(r) for r in GP],
            expected_xmm=[g.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little').hex() for i in range(8)],
            expected_stack=bytes(g.mem_read(STACK-64,96)).hex(),calls=list(calls),stores=list(stores),events=list(events))
        rows.append(active)
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_ZOOM_RESET_REFERENCE_V1',original_sha256=pin,rows=rows),separators=(',',':'))+'\n').encode()
    if len(data)>1024*1024:raise ValueError('Fixture exceeds1MiB')
    args.fixture.write_bytes(data);report=dict(passed=True,original_sha256=pin,cases=len(rows),
        committed_reset_events=sum(len(r['events']) for r in rows),tail_sound_calls=sum(c['tail'] for r in rows for c in r['calls']),
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),instructions=instructions,
        instruction_patches=[],declared_service_boundaries=[hex(x) for x in SERVICES],full_audio_or_frame_parity=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','committed_reset_events','tail_sound_calls','fixture_bytes','fixture_sha256')}))


if __name__=='__main__':main()
