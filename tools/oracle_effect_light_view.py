#!/usr/bin/env python3
"""Unchanged C15A0, query epoch transition/132220, and whole31910 selection.

The pinned retail RAM supplies all code and literal banks. Declared occupied
350-slot light records, two tag definitions, spatial lists, constructed cluster
frustums, camera, clock and local-view inputs exercise ordered multi-view history.
The query replay starts at13219A after caller-supplied BSP admission and stops
at1321CD; it is not a whole1320F0/portal replay. Object branches are disabled.
No instruction or callee is replaced. Complete record, queue and four-cache
checkpoints are serialized as deltas and compared in full by the native test.
This is CPU service evidence, not a matched original gameplay framebuffer.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
from oracle_effect_light_visibility import cube


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    s=Snapshot(a.snapshot);digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    pool=u(s.read(0x4e030c,4));link_pool=u(s.read(0x4e0314,4))
    table=u(s.read(0x4e3b44,4));tags=(0xe63a04c5,0xe694051f)
    definitions=[s.read(u(s.read(table+(tag&65535)*16+8,4)),228) for tag in tags]
    render=u(s.read(0x5107e8,4));debug=u(s.read(0x4e9ae8,4));debug_index=u(s.read(debug+0xc14,4))
    entries=[[c,1,0,0,0,0,0,0,0xffff,0xffff,0xffff,0xffff,0xffff] for c in (3,1,7,0,6,2,5,4)]
    frustums=[cube((0.,0.,0.),12.)]
    scenarios,steps=8,32
    data=bytearray(struct.pack('<5I',0x31564c45,scenarios,1,len(entries),len(frustums)))
    for row in entries:data+=struct.pack('<13I',*row)
    for f in frustums:data+=struct.pack('<30f',*f)
    for tag,raw in zip(tags,definitions):data+=struct.pack('<I',tag)+raw
    rng=random.Random(0xc15a031910);calls={};checkpoints=0;selected=0;appended=0;word_count=0
    for scenario in range(scenarios):
        g=s.guest();g.mem_map(0x70000000,0x100000);mapped=set()
        def write(at,raw):
            for page in range(at&~4095,(at+len(raw)+4095)&~4095,4096):
                if page not in mapped:
                    g.mem_map(page,4096);g.mem_write(page,s.read(page,4096));mapped.add(page)
            g.mem_write(at,raw)
        def put(at,*v):write(at,struct.pack('<'+'I'*len(v),*v))
        records,bitmap=0x70010000,0x70028000
        context,bank,main_queue,light_queue=0x70030000,0x70034000,0x70038000,0x70039000
        columns=[0x7003a000+i*0x1000 for i in range(8)]
        heads,links=0x70044000,0x70045000
        stack,stop,out=0x700fe000,0x700fff00,0x70050000
        raw=bytearray(rng.randbytes(350*272));occupied=[i for i in range(96) if rng.randrange(4)]
        bits=[sum(1<<(i&31) for i in occupied if i//32==j) for j in range(11)]
        for i in occupied:
            r=bytearray(272);struct.pack_into('<HHII',r,0,0xe900+i,0x1b if i%5 else 1,tags[i&1],0xffffffff)
            struct.pack_into('<I',r,12,0xffffffff)
            pos=tuple(rng.uniform(-16,16) for _ in range(3));radius=rng.choice([.125,1.,2.,8.])
            struct.pack_into('<4f',r,0x18,*pos,radius)
            struct.pack_into('<4f',r,0x28,pos[0]+.25,pos[1]-.5,pos[2]+.125,radius)
            struct.pack_into('<h',r,0x48,i%8)
            struct.pack_into('<I',r,0x4c,0xffffffff);struct.pack_into('<h',r,0x54,-1)
            struct.pack_into('<I',r,0x58,0xffffffff);struct.pack_into('<h',r,0x5c,-1)
            struct.pack_into('<3f',r,0x84,*pos);struct.pack_into('<3f',r,0xac,1.,0.,0.);struct.pack_into('<3f',r,0xb8,0.,0.,1.)
            struct.pack_into('<f',r,0xc4,1.);struct.pack_into('<2f',r,0xcc,1.,1.)
            struct.pack_into('<3f',r,0xd4,1.,.5,.25);struct.pack_into('<3f',r,0xe0,.25,.5,1.)
            struct.pack_into('<f',r,0xec,1.);struct.pack_into('<5f',r,0xf8,.13,.27,.39,.51,.63)
            raw[i*272:(i+1)*272]=r
        put(pool+0x34,0,96,len(occupied),0xe981,records,bitmap)
        g.mem_write(records,bytes(raw));g.mem_write(bitmap,struct.pack('<11I',*bits))
        lists=[[0xe9000000+(i<<16)+i for i in occupied if rng.randrange(4)==0] for _ in range(8)]
        link_count=0
        put(0x4e0310,heads);put(link_pool+0x44,links)
        for c,ids in enumerate(lists):
            g.mem_write(heads+c*4,struct.pack('<I',0xec630000|link_count if ids else 0xffffffff))
            for j,identity in enumerate(ids):
                g.mem_write(links+link_count*12,struct.pack('<III',0xec63,identity,0xec630000|(link_count+1) if j+1<len(ids) else 0xffffffff));link_count+=1
        for q,cols,capacity in ((main_queue,columns[:4],64),(light_queue,columns[4:],1)):
            g.mem_write(q,struct.pack('<6I',capacity,0,cols[1],cols[0],cols[2],cols[3]))
            for address,width in zip(cols,(4,2,4,2)):g.mem_write(address,bytes(width*capacity))
        g.mem_write(context,struct.pack('<I',bank));g.mem_write(context+0x10,struct.pack('<I',main_queue))
        g.mem_write(bank,struct.pack('<h',1));g.mem_write(bank+0xa6c,struct.pack('<h',len(entries)))
        for i,row in enumerate(entries):g.mem_write(bank+0xa6e+i*26,struct.pack('<13H',*row))
        for i,f in enumerate(frustums):g.mem_write(bank+0x1974+i*264+96,struct.pack('<30f',*f))
        for at in (0x4e0308,0x4de2fc,0x4ba034,0x4b9ed4,0x4c0b80,0x547f98,0x510c50):put(at,0)
        for at in (0x4e0304,0x4de2f8):write(at,b'\0')
        write(0x5234e8,bytes(4*0x484))
        write(render+8,b'\1');put(debug,0);put(0x55e4d0+debug_index*4,0);write(0x485602,struct.pack('<h',2))
        # Bit6 suppresses object branches. Independent sky scan is disabled.
        put(0x4b9f8c,0x70048000);g.mem_write(0x70048000,bytes(64))
        initial_epoch=(0xfffffffc if scenario&1 else 0);put(0x4e0308,initial_epoch)
        data+=struct.pack('<I11I',initial_epoch,*bits)+raw
        for ids in lists:data+=struct.pack('<I',len(ids))+struct.pack('<'+'I'*len(ids),*ids)
        data+=struct.pack('<I',steps)
        def trace(guest,at,n,d):calls[f'{at:08x}']=calls.get(f'{at:08x}',0)+1
        for at in (0xc15a0,0xc12d0,0x132220,0x132a30,0xc3110,0x31910,0x4a7c0,0x4ac30):g.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
        def run(at,args=(),until=stop):
            g.mem_write(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args));g.reg_write(R.UC_X86_REG_ESP,stack)
            g.reg_write(R.UC_X86_REG_FPCW,0x023f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
            g.emu_start(at,until,count=10000000);assert g.reg_read(R.UC_X86_REG_EIP)==until,(scenario,hex(at),hex(g.reg_read(R.UC_X86_REG_EIP)))
        previous=[]
        for step in range(steps):
            eye=tuple(rng.uniform(-24,24) for _ in range(3));mode=step%4
            kind=int(step%7==6);local_player=step%4 if scenario<6 else (-1 if step%2 else 7)
            flags=0x40|(0x10 if step%11==10 else 0);pass_mode=int(step%13==12)
            frame=step//4+1;cinematic=int(step%17==16)
            plane=(.25,-.125,.875,-.5);pixel_scale=(120.,240.,360.,540.)[step%4]
            write(0x4b9da0,struct.pack('<3f',*eye));run(0xc15a0)
            q=light_queue if kind else main_queue
            g.mem_write(context+0x10,struct.pack('<I',q));g.mem_write(q+4,b'\0\0')
            g.mem_write(context+8,struct.pack('<h',local_player));g.mem_write(context+0x2a60,struct.pack('<I',flags))
            g.mem_write(context+0x2a68,struct.pack('<4f',*eye,8.));g.mem_write(context+0x2a78,struct.pack('<II',pass_mode,mode))
            put(0x4b9ed4,local_player&0xffffffff);put(0x4c0b80,context)
            g.reg_write(R.UC_X86_REG_ESI,context);g.reg_write(R.UC_X86_REG_EBX,1)
            run(0x13219a,until=0x1321cd)
            count=struct.unpack('<H',g.mem_read(q+4,2))[0];appended+=count
            for at in (0x4e0304,0x4de2f8):write(at,b'\0')
            #31910 always consumes the main context queue, never nested light queue.
            put(0x547f98,main_queue);put(0x4ba034,frame)
            for at,v in zip((0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0),(*plane,pixel_scale)):write(at,struct.pack('<f',v))
            put(0x510c50,0x70049000 if cinematic else 0);g.mem_write(0x70049005,bytes([cinematic]))
            if not kind:
                run(0x31910,(out,));selected+=g.reg_read(R.UC_X86_REG_EAX)
            words=[u(g.mem_read(0x4e0308,4))]
            words+=list(struct.unpack('<'+('I'*(350*68)),g.mem_read(records,350*272)))
            for queue,cols,capacity in ((main_queue,columns[:4],64),(light_queue,columns[4:],1)):
                words.append(struct.unpack('<H',g.mem_read(queue+4,2))[0])
                for i in range(capacity):
                    for at,fmt,width in zip(cols,('I','H','I','H'),(4,2,4,2)):words.append(struct.unpack('<'+fmt,g.mem_read(at+i*width,width))[0])
            # Inactive cache entries are retained source bytes too.
            words+=list(struct.unpack('<'+('I'*(4*0x484//4)),g.mem_read(0x5234e8,4*0x484)))
            old=previous or [0]*len(words);delta=[(i,v) for i,v in enumerate(words) if old[i]!=v]
            data+=struct.pack('<7I9fII',kind,local_player&0xffffffff,mode,flags,pass_mode,frame,cinematic,*eye,*plane,pixel_scale,8.,len(words),len(delta))
            for i,v in delta:data+=struct.pack('<II',i,v)
            previous=words;checkpoints+=1;word_count+=len(words)
        print('scenario',scenario,'complete',flush=True)
    (a.output/'fixture.bin').write_bytes(data)
    report=dict(boundary=__doc__,snapshot_sha256=digest,scenarios=scenarios,checkpoints=checkpoints,words=word_count,
        appends=appended,selected=selected,calls=calls,intercepted_instructions=0,
        fixture_sha256=hashlib.sha256(data).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
