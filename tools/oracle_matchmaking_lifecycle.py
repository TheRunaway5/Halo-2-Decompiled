#!/usr/bin/env python3
"""Record unchanged retail lifecycle outputs with declared synthetic operands.

Whole 70190, 75890, 193C70/getters, 91290/912D0 and prepared 70EA0 run
unchanged. Role entries, growth/countdown/timeout blocks, successful initializer
tail and query resubmission stop at explicit boundaries before platform/launch
work. No instruction, allocator, callback, NAT or Live callee is substituted.
Run through tools/run_test.py. Only bounded numerical reference JSON is retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_game_variants import Source,VERSIONS,STACK,STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
PARTY,GAME,STATE,OWNER,RESULTS,SEEN,HOPPER,DESC,SEARCH,RNG=(
    0x1100000,0x1110000,0x1120000,0x1130000,0x1140000,0x1150000,
    0x1151000,0x1154000,0x1160000,0x1170000)
CONFIG=0x4ce040


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if sys.flags.optimize or hashlib.sha256((ROOT/VERSIONS['retail']['path']).read_bytes()).hexdigest()!=PIN:
        raise RuntimeError('Pinned executable and enabled original guards required')
    source=Source(VERSIONS['retail']);g=source.g;g.mem_map(PARTY,0x80000)
    def w(at,value):g.mem_write(at,struct.pack('<I',value&0xffffffff))
    def r(at):return struct.unpack('<i',g.mem_read(at,4))[0]
    def u(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    def b(at,value):g.mem_write(at,bytes([value]))
    def reset(now=10000):
        source.call(0x66330);g.mem_write(PARTY,bytes(0x80000))
        b(0x510548,1);w(0x51054c,now);w(0x467214,-1)
        b(0x4cf8d4,0);w(0x55e6fc,0)
        w(STATE+8,OWNER);w(OWNER+0x30,PARTY);w(OWNER+0x38,GAME);w(STATE+0x14,DESC)
        w(STATE+0x978,3);w(PARTY+0x741c,5);w(PARTY+0x4980,7)
    def boundary(address,stops,registers=(),argv=()):
        g.mem_write(STACK,struct.pack('<I',STOP))
        for index,value in enumerate(argv):w(STACK+4+index*4,value)
        for reg,value in registers:g.reg_write(reg,value&0xffffffff)
        g.reg_write(UC_X86_REG_ESP,STACK)
        hit=[];before=set(source.executed)
        def stop(_,at,size,data):
            if at in stops:hit.append(at);g.emu_stop()
        hook=g.hook_add(UC_HOOK_CODE,stop)
        try:g.emu_start(address,STOP,count=200000)
        finally:g.hook_del(hook)
        if not hit and g.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Original boundary not reached')
        if hit and hit[0] not in before:source.executed.discard(hit[0])
        return hit[0] if hit else STOP
    fixture=dict(original_sha256=PIN,scope=__doc__,jitter=[],elapsed=[],roles=[],progress=[],growth=[],
                 hoppers=[],countdown_gates=[],gather_timeouts=[],search_timeouts=[],seen=[])
    rng=random.Random(0x7019071730)
    controls=[(seed,60000,60000)for seed in (0,1,1234,0xffffffff)]
    spreads=[-0x80000000,-65537,-32768,-1,0,1,32767,32768,60000,65535,65536,0x7fffffff]
    controls += [(rng.getrandbits(32),rng.choice(spreads),rng.choice(spreads))for _ in range(64)]
    for seed,search,gather in controls:
        reset();w(CONFIG+0x19c,search);w(CONFIG+0x194,gather);w(0x4e7408,RNG);w(RNG+4,seed)
        source.call(0x70190,registers=((UC_X86_REG_EAX,STATE),(UC_X86_REG_ECX,OWNER)))
        fixture['jitter'].append(dict(seed=seed,search_spread=search,gather_spread=gather,
            expected=[u(RNG+4),r(STATE+0x96c),r(STATE+0x974)]))
    if fixture['jitter'][0]['expected'][1:]!=[-1307,-1543]:raise RuntimeError('Original initializer fixture failed')
    ticks=[(0x100,0xffffff00),(0x80000010,0x7ffffff0),(0x80000000,0),(0,0xffffffff),(0x7fffffff,0)]
    ticks += [(rng.getrandbits(32),rng.getrandbits(32))for _ in range(32)]
    for now,start in ticks:
        reset(now);value=source.call(0x75890,(start,))
        fixture['elapsed'].append(dict(now=now,start=start,expected=value))
    roles=[(10000,0,0,8),(10000,9000,8000,8),(10000,8000,9000,8),
           (10000,9000,9000,8),(10000,9000,8000,-1),
           (0x100,0xffffff00,0xffffff80,8),(0x100,0xffffff80,0xffffff00,8),
           (0x100,0x80000100,0xfffffff0,8)]
    roles += [(rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.choice([-1,8,16]))for _ in range(64)]
    for now,search,gather,capacity in roles:
        reset(now);w(PARTY+0x4980,6);b(STATE+0x954,1)
        w(STATE+0xa7c,search);w(STATE+0xa68,gather);w(STATE+0x938,capacity)
        hit=boundary(0x70d60,{0x70c00,0x70c90},registers=((UC_X86_REG_ESI,STATE),))
        if hit not in (0x70c00,0x70c90):raise RuntimeError('Role entry missing')
        fixture['roles'].append(dict(now=now,search_start=search,gather_start=gather,capacity=capacity,
            expected='search'if hit==0x70c00 else'gather'))
    def progress_setup(maximum,ranked,radius,total,count,pending,selected):
        reset();b(SEARCH,1);w(SEARCH+8,1)
        for at,active in zip((0x70,0x74,0x78),pending):w(SEARCH+at,3 if active else -1)
        w(SEARCH+0x8c,RESULTS);w(SEARCH+0x90,RESULTS+0x7000);w(SEARCH+0x88,50*0x204)
        w(SEARCH+0x1c,radius);w(SEARCH+0x20,maximum);w(SEARCH+0xc,total);w(SEARCH+0x84,count)
        b(SEARCH+0x14,int(ranked));w(SEARCH+0x60,0);w(SEARCH+0x64,127);w(SEARCH+0x68,10);w(SEARCH+0x50,10)
        w(SEARCH+0x34,100)
        if selected:
            b(RESULTS,1);b(RESULTS+1,1);b(RESULTS+0x74,1);w(RESULTS+0x1f8,100)
    cases=[(4,True,0,0,0),(4,True,4,0,0),(4,False,4,0,0),(4,True,4,0,50),
           (4,False,-1,0,49),(4,True,0,100,49),(4,True,0,100,50)]
    cases += [(maximum,ranked,rng.choice(range(-int(not ranked),maximum+1)),
        rng.choice([0,49,100,149,150,0x7fffffff,-0x80000000]),rng.randrange(51))
        for maximum,ranked in [(rng.randrange(16),bool(rng.getrandbits(1)))for _ in range(192)]]
    for index,(maximum,ranked,radius,total,count) in enumerate(cases):
        pending=[False]*3;selected=False
        if index>=7 and index%9 in (0,1,2):pending[index%9]=True
        if index>=7 and index%9==3:selected=True
        progress_setup(maximum,ranked,radius,total,count,pending,selected)
        hit=boundary(0x90f70,{0x8fa80},registers=((UC_X86_REG_EAX,SEARCH),))
        fixture['progress'].append(dict(maximum=maximum,ranked=ranked,radius=radius,total=total,count=count,
            pending=[*pending,selected],expected=[r(SEARCH+0x1c),r(SEARCH+0xc),bool(r(SEARCH+8)),hit==0x8fa80]))
    for peak in range(17):
        for players in range(17):
            for now,start in ((10000,5000),(16,0xfffffff0)):
                reset(now);w(STATE+0xa6c,peak);w(STATE+0xa70,start);w(GAME+0x1118,players);w(STACK+0x14,GAME)
                boundary(0x7182f,{0x718a9},registers=((UC_X86_REG_EBP,STATE),(UC_X86_REG_EDX,peak),(UC_X86_REG_ECX,0)))
                value=g.reg_read(UC_X86_REG_EAX);value=value if value<0x80000000 else value-0x100000000
                fixture['growth'].append(dict(now=now,start=start,peak=peak,players=players,
                    expected=[r(STATE+0xa6c),u(STATE+0xa70),value]))
    for index in range(7):
        reset();source.call(0x193c70,registers=((UC_X86_REG_EAX,index),(UC_X86_REG_EDX,HOPPER)))
        minimum=source.call(0x1932c0,registers=((UC_X86_REG_ECX,HOPPER),))
        maximum=source.call(0x193300,registers=((UC_X86_REG_ECX,HOPPER),))
        countdown=[source.call(0x1931d0,registers=((UC_X86_REG_EAX,HOPPER),(UC_X86_REG_ESI,count)))for count in range(18)]
        timeout=r(HOPPER+0x59c)
        fixture['hoppers'].append(dict(index=index,minimum=minimum,maximum=maximum,timeout=timeout,countdown=countdown))
        for count in (0,minimum,maximum):
            delay=countdown[count]
            elapsed_values=[0x7fffffff]if delay==0x7fffffff else[delay-1,delay,delay+1]
            for waited in elapsed_values:
                w(GAME+0x1118,count);w(STACK+0x14,GAME);w(STACK+0x28,waited)
                hit=boundary(0x719e9,{0x71a07,0x71a30},registers=((UC_X86_REG_EDI,HOPPER),))
                fixture['countdown_gates'].append(dict(index=index,players=count,waited=waited,expected=hit==0x71a30))
        for offset in (-1543,0,32767):
            for delta in (-1,0,1):
                waited=timeout+offset+delta;w(STACK+0x28,waited);w(STATE+0x974,offset)
                hit=boundary(0x71c7a,{0x71cc8,0x71cfd},registers=((UC_X86_REG_EDI,HOPPER),(UC_X86_REG_EBP,STATE)))
                fixture['gather_timeouts'].append(dict(index=index,offset=offset,waited=waited,expected=hit==0x71cfd))
    for offset in (-5536,-1,0,32767):
        for delta in (-1,0,1):
            start=0xffff0000;now=(start+240000+offset+delta)&0xffffffff;reset(now)
            active=STATE+0x97c;b(active,1);w(active+8,1)
            for at in (0x70,0x74,0x78):w(active+at,-1)
            w(active+0x8c,RESULTS);w(active+0x98,150);w(active+0xa0,SEEN)
            w(STATE+0xa7c,start);w(STATE+0x96c,offset)
            source.call(0x70ea0,registers=((UC_X86_REG_EAX,STATE),))
            fixture['search_timeouts'].append(dict(now=now,start=start,offset=offset,expected=r(PARTY+0x4980)==6))
    reset();w(SEARCH+0x98,150);w(SEARCH+0xa0,SEEN)
    identifiers=[1,(1<<32)|1,2,1,*range(3,153)]
    for identity in identifiers:
        g.mem_write(RESULTS,bytes(0x204));b(RESULTS,1);b(RESULTS+1,1)
        g.mem_write(RESULTS+0x1c,struct.pack('<Q',identity))
        source.call(0x912d0,registers=((UC_X86_REG_EAX,RESULTS),(UC_X86_REG_ECX,SEARCH)))
        fixture['seen'].append(dict(identity=identity,count=r(SEARCH+0x94),eligible=g.mem_read(RESULTS+1,1)[0],seen=g.mem_read(RESULTS+2,1)[0]))
    fixture['seen_final']=list(struct.unpack('<150Q',g.mem_read(SEEN,1200)))
    fixture['seen_lookups']=[]
    for identity in (1,2,149,150,151,152,(1<<32)|1):
        g.mem_write(DESC,struct.pack('<Q',identity))
        found=bool(source.call(0x91290,(DESC,),registers=((UC_X86_REG_EBX,SEARCH),))&0xff)
        fixture['seen_lookups'].append(dict(identity=identity,expected=found))
    boundary(0x90a4c,{0x90b30},registers=((UC_X86_REG_ESI,SEARCH),(UC_X86_REG_EDX,0)))
    fixture['new_search_seen_count']=r(SEARCH+0x94)
    cs=Cs(CS_ARCH_X86,CS_MODE_32);trace=[]
    for at in sorted(source.executed):
        instruction=next(cs.disasm(bytes(g.mem_read(at,15)),at))
        if instruction.bytes!=source.image.read(at,instruction.size):raise RuntimeError('Original code bytes changed')
        trace.append(f'{at:08x} {instruction.bytes.hex()}')
    fixture['oracle_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    fixture['executed_instruction_sha256']=hashlib.sha256(('\n'.join(trace)+'\n').encode()).hexdigest()
    raw=(json.dumps(fixture,indent=2)+'\n').encode()
    if len(raw)>1024*1024:raise RuntimeError('Numerical reference exceeded small-fixture budget')
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(passed=True,original_sha256=PIN,oracle_sha256=fixture['oracle_sha256'],fixture_sha256=hashlib.sha256(raw).hexdigest(),
        fixture_bytes=len(raw),scope=__doc__,counts={key:len(value)for key,value in fixture.items()if isinstance(value,list)},
        original_instructions=len(trace),executed_instruction_sha256=fixture['executed_instruction_sha256'],instruction_patches=[],replaced_callees=[],
        scratch=str(scratch),scratch_retained=False,whole_session_or_live_flow_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
