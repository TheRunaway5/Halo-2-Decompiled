#!/usr/bin/env python3
"""Whole unchanged retail standalone C0350/C0C80/C1400/C3260 lifetime.

The real350x272 lights header and real Lockout BSP/tag/code banks come from
the pinned original RAM. Declared cold backing, debug mode, control, frame,
clock, epoch and registration-enable inputs exercise chronology, exhaustion,
reuse, generation wrap and tick wrap. Original allocators, registration,
collision, update and release execute unchanged. Word deltas serialize complete
pool checkpoints; the native test reconstructs and compares every word, not
only touched fields. Effect-part A/B and attachments remain outside this test.
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
    header=s.read(u(s.read(0x4e030c,4)),76)
    assert u(header,0x20)==350 and u(header,0x24)==272 and header[:7]==b'lights\0'
    spatial_headers=[s.read(u(s.read(a,4)),76) for a in (0x4e0314,0x4e0318)]
    bsp=u(s.read(0x4e0348,4));clusters=u(s.read(bsp+0x9c,4))
    selected=struct.unpack('<h',s.read(0x4686c4,2))[0]
    original_clock=u(s.read(0x510c54,4));clock_header=s.read(original_clock,16)
    tick_rate=struct.unpack_from('<h',clock_header,2)[0]
    tag_table=u(s.read(0x4e3b44,4));tags=(0xe63a04c5,0xe694051f)
    definitions=[s.read(u(s.read(tag_table+(tag&65535)*16+8,4)),228) for tag in tags]
    frame_source=s.read(u(header,0x44)+21*272,272)
    center=struct.unpack_from('<3f',frame_source,0x84)
    forward=struct.unpack_from('<3f',frame_source,0xac);up=struct.unpack_from('<3f',frame_source,0xb8)
    # Retail curve address, independently pinned by the supported original
    # executable/table fingerprint in OriginalData::load_xbe.
    curve=s.read(0x46d148,1024)
    fixture=bytearray(struct.pack('<6I',0x32464c4c,5,clusters,selected&65535,tick_rate,len(tags)))
    fixture+=curve
    for tag,raw in zip(tags,definitions):fixture+=struct.pack('<I',tag)+raw
    all_calls={};summaries=[];checkpoints=0;failures=0
    rng=random.Random(0xc0350c1400)
    for scenario in range(5):
        g=s.guest();g.mem_map(0x70000000,0x100000)
        def materialize(at,n):
            for page in range(at&~4095,(at+n+4095)&~4095,4096):
                try:g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
                except Exception as e:
                    if 'Invalid memory mapping' not in str(e):raise
        for at,n in ((0x4e0308,20),(0x510c54,4),(0x4de2f4,4)):materialize(at,n)
        case_definitions=[]
        for index,(tag,raw) in enumerate(zip(tags,definitions)):
            raw=bytearray(raw)
            if scenario==4:
                struct.pack_into('<f',raw,0xb0,0. if index==0 else -.25)
                struct.pack_into('<ff',raw,8,.25,.75)
                struct.pack_into('<3f',raw,0x4c,-.5,2.,.25)
                struct.pack_into('<3f',raw,0x64,-2.,.5,3.)
            definition=u(s.read(tag_table+(tag&65535)*16+8,4));materialize(definition,228);g.mem_write(definition,bytes(raw))
            case_definitions.append(bytes(raw))
        light_pool,records,bitmap=0x70010000,0x70020000,0x70011000
        heads=0x70012000;spatial_pools=[0x70014000,0x70015000]
        spatial_records=[0x70040000,0x70050000];spatial_bits=[0x70016000,0x70016100]
        clock,enabled,stack,stop=0x70017000,0x70018000,0x700fe000,0x700fff00
        h=bytearray(header);h[0x2a]=(h[0x2a]&~8)|(8 if scenario==1 else 0)
        struct.pack_into('<II',h,0x44,records,bitmap);g.mem_write(light_pool,bytes(h))
        initial=bytes(rng.randrange(256) for _ in range(350*272))
        g.mem_write(records,initial);g.mem_write(0x4e030c,struct.pack('<I',light_pool))
        g.mem_write(0x4e0310,struct.pack('<3I',heads,*spatial_pools));g.mem_write(heads,b'\xff'*(clusters*4))
        g.mem_write(clock,clock_header);g.mem_write(0x510c54,struct.pack('<I',clock))
        g.mem_write(0x4de2f4,struct.pack('<I',enabled))
        def call(at,args=(),registers=()):
            g.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args))
            g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
            for reg,value in registers:g.reg_write(reg,value)
            g.emu_start(at,stop,count=100000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==stop,(scenario,hex(at),hex(g.reg_read(R.UC_X86_REG_EIP)))
            return g.reg_read(R.UC_X86_REG_EAX)
        def trace(guest,at,size,data):all_calls[f'{at:08x}']=all_calls.get(f'{at:08x}',0)+1
        for at in (0xc0350,0xc0c80,0xc1400,0xc3260,0xc2d00,0xc3220,0x16b840,0x16ba40,0x16a8e0,0x1cac60,0x1cae40):
            g.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
        call(0x16b7a0,registers=((R.UC_X86_REG_ESI,light_pool),))
        initial_salt=0xfffe if scenario==3 else u(bytes(g.mem_read(light_pool+0x40,4)))&65535
        g.mem_write(light_pool+0x40,struct.pack('<H',initial_salt))
        for i in range(2):
            h=bytearray(spatial_headers[i]);h[0x2a]=(h[0x2a]&~8)|(8 if scenario==1 else 0)
            struct.pack_into('<II',h,0x44,spatial_records[i],spatial_bits[i]);g.mem_write(spatial_pools[i],bytes(h))
            call(0x16b7a0,registers=((R.UC_X86_REG_ESI,spatial_pools[i]),))
        fixture+=struct.pack('<2I',scenario==1,initial_salt)
        for tag,raw in zip(tags,case_definitions):fixture+=struct.pack('<I',tag)+raw
        fixture+=initial
        previous=[];steps=bytearray();operations=[]
        def state():
            data=bytes(g.mem_read(light_pool+0x34,16))+bytes(g.mem_read(bitmap,44))+bytes(g.mem_read(records,350*272))
            data+=bytes(g.mem_read(heads,clusters*4))
            for pool,bit,rec in zip(spatial_pools,spatial_bits,spatial_records):
                data+=bytes(g.mem_read(pool+0x34,16))+bytes(g.mem_read(bit,256))+bytes(g.mem_read(rec,2048*12))
            return struct.unpack('<'+'I'*(len(data)//4),data)
        def checkpoint(kind,tick,epoch,admission,identity,tag,control,position,result):
            nonlocal previous,checkpoints
            now=state();old=previous or [0]*len(now)
            delta=[(i,v) for i,v in enumerate(now) if v!=old[i]]
            steps.extend(struct.pack('<7If9fII',kind,tick,epoch,admission,identity,tag,result,control,*position,*forward,*up,len(now),len(delta)))
            for i,v in delta:steps.extend(struct.pack('<II',i,v))
            previous=now;checkpoints+=1;operations.append(kind)
        checkpoint(0,0,0,1,0xffffffff,0,0,center,0xffffffff)
        tick=0xfffffff0 if scenario==3 else 100
        epoch=0xffffffff if scenario==3 else 7
        def ids():
            h=bytes(g.mem_read(light_pool,76));bits=bytes(g.mem_read(bitmap,44));out=[]
            for i in range(u(h,0x38)):
                if u(bits,(i//32)*4)&(1<<(i&31)):
                    salt=struct.unpack('<H',g.mem_read(records+i*272,2))[0];out.append((salt<<16)|i)
            return out
        def operation(kind,identity=0xffffffff,tag=None,control=1.,admission=1):
            nonlocal tick,epoch,failures
            tag=tags[rng.randrange(2)] if tag is None else tag
            position=tuple(x+rng.uniform(-1,1) for x in center)
            g.mem_write(clock+8,struct.pack('<I',tick));g.mem_write(0x4e0308,struct.pack('<I',epoch));g.mem_write(enabled,bytes([admission]))
            result=0xffffffff
            if kind==1:
                g.mem_write(0x70001000,struct.pack('<9f',*position,*forward,*up))
                result=call(0xc0350,(tag,0xffffffff,0xffffffff,0x70001018),
                    ((R.UC_X86_REG_EBX,0x70001000),(R.UC_X86_REG_EDI,0x7000100c),(R.UC_X86_REG_XMM0,u(struct.pack('<f',control)))))
                failures+=result==0xffffffff
            elif kind==2:
                g.mem_write(records+(identity&65535)*272+0xcc,struct.pack('<f',control));call(0xc0c80,(identity,))
            elif kind==3:call(0xc3260,(identity,1))
            elif kind==4:call(0xc1400)
            checkpoint(kind,tick,epoch,admission,identity,tag,control,position,result)
            epoch=(epoch+1)&0xffffffff
            return result
        if scenario<2 or scenario==4:
            for n in range(96):
                live=ids();choice=rng.randrange(8)
                if not live or choice<4:operation(1,control=(0.,.5,1.,2.)[n%4],admission=n%7!=0)
                elif choice<6:operation(2,live[rng.randrange(len(live))],control=(0.,.5,1.,2.)[n%4],admission=n%7!=0)
                elif choice==6:operation(3,live[rng.randrange(len(live))])
                else:tick+=rng.randrange(5);operation(4)
            tick+=1000;operation(4)
            if scenario==4:
                for identity in ids():operation(3,identity)
        elif scenario==2:
            for n in range(351):operation(1,tag=tags[n&1])
            for identity in ids()[::3]:operation(3,identity)
            for n in range(118):operation(1)
            tick+=1000;operation(4)
        else:
            for n in range(8):operation(1,control=(0.,.5,1.,2.)[n%4])
            tick=0;operation(4);tick=1;operation(4);tick=1000;operation(4)
        fixture+=struct.pack('<I',len(operations))+steps
        summaries.append(dict(scenario=scenario,operations=len(operations),final_live=len(ids())))
        print('scenario',scenario,'complete',len(operations),'checkpoints',flush=True)
    (args.output/'fixture.bin').write_bytes(fixture)
    report=dict(boundary=__doc__,snapshot_sha256=digest,cases=summaries,checkpoints=checkpoints,
        allocation_failures=failures,calls=all_calls,intercepted_instructions=0,
        fixture_sha256=hashlib.sha256(fixture).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
