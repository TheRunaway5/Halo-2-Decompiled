#!/usr/bin/env python3
"""Replay the actual31C80→1320F0 light receiver query from pinned retail RAM.

C15A0 and31C80 run unchanged up to their first1320F0 call. The captured
registers, six cameras, sphere, containing cluster and auxiliary main queues
are the original caller's inputs. Whole1320F0 then executes unchanged.
Additional declared sphere/cluster probes below the original3-unit mode
threshold retain that caller's cameras and
main-view queues. All consumed BSP queue and association-mask words are saved;
dynamic object queues execute but are not compared by this fixture. This is
CPU receiver preparation evidence, not geometry draws or framebuffer parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot, u

PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    s=Snapshot(a.snapshot);assert hashlib.sha256(s.ram).hexdigest()==PIN
    g=s.guest();g.mem_map(0x70000000,0x100000)
    stack,stop=0x700fe000,0x700fff00
    def read(guest,at,n):
        regions=list(guest.mem_regions())
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(begin<=page<=end for begin,end,_ in regions):
                guest.mem_map(page,4096);guest.mem_write(page,s.read(page,4096))
        return bytes(guest.mem_read(at,n))
    def run(guest,entry,end=stop):
        guest.emu_start(entry,end,count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==end,(hex(entry),hex(guest.reg_read(R.UC_X86_REG_EIP)))
    def call(guest,entry,registers=()):
        guest.mem_write(stack,struct.pack('<I',stop));guest.reg_write(R.UC_X86_REG_ESP,stack)
        guest.reg_write(R.UC_X86_REG_FPCW,0x023f);guest.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        for register,value in registers:guest.reg_write(register,value&0xffffffff)
        run(guest,entry)
    call(g,0xc15a0)
    g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack)
    run(g,0x31c80,0x1320f0)
    registers={name:g.reg_read(getattr(R,'UC_X86_REG_'+name)) for name in ('EAX','EBX','ECX','EDX','ESI','EDI','ESP')}
    sp=registers['ESP'];args=struct.unpack('<8I',read(g,sp,32))
    ret,flags,cameras,sphere,radius,auxiliary,clipping,cluster=args
    assert registers['EAX']==1 and registers['ECX']==6 and auxiliary==0x547f88
    original_sphere=read(g,sphere,12)+struct.pack('<I',radius)
    camera_bytes=read(g,cameras,6*0x1bc)
    bsp=u(s.read(0x4e0348,4));nc=u(s.read(bsp+0x9c,4));clusters=u(s.read(bsp+0xa0,4))
    # A new Unicorn guest is required after stopping at its previous until
    # address. Copy every materialized page plus CPU context; no code changes.
    def clone(guest):
        result=s.guest()
        for begin,end,permissions in guest.mem_regions():
            result.mem_map(begin,end-begin+1,permissions)
            result.mem_write(begin,bytes(guest.mem_read(begin,end-begin+1)))
        result.context_restore(guest.context_save());return result
    def queues(guest,context):
        out=[]
        for offset in (0xc,0x18):
            header=u(read(guest,context+offset,4));count=struct.unpack('<H',read(guest,header+4,2))[0]
            flagptr,idptr,otherptr,assocptr=struct.unpack('<4I',read(guest,header+8,16))
            out.append([list((u(read(guest,idptr+i*4,4)),struct.unpack('<H',read(guest,flagptr+i*2,2))[0],
                u(read(guest,otherptr+i*4,4)),struct.unpack('<H',read(guest,assocptr+i*2,2))[0])) for i in range(count)])
        return out
    main_queues=queues(g,auxiliary)
    fog_guest=clone(g);fog=[];datum=u(s.read(0x4b9f8c,4))
    for i in range(nc):
        call(fog_guest,0x16e210,((R.UC_X86_REG_ECX,i),(R.UC_X86_REG_EDI,datum)))
        fog.append(fog_guest.reg_read(R.UC_X86_REG_EAX)&255)
    cases=[(cluster,original_sphere,'actual31C80')]
    for i in range(-1,nc):
        bounds=struct.unpack('<6f',s.read(clusters+max(i,0)*0xb0+0x54,24))
        position=[(bounds[j]+bounds[j+1])*.5 for j in (0,2,4)]
        for r in (0.,.371,2.,struct.unpack('<f',struct.pack('<I',0x403fffff))[0]):
            cases.append((i,struct.pack('<4f',*position,r),'declared-sphere'))
    data=bytearray(struct.pack('<7I',0x3152434c,len(cases),nc,len(camera_bytes)//4,
        struct.unpack('<h',s.read(0x4686c4,2))[0]&0xffffffff,datum,u(s.read(0x4e64c8,4))))
    data+=camera_bytes+bytes(fog)
    data+=bytes((-len(data))%4)
    for q in main_queues:
        data+=struct.pack('<I',len(q))
        for record in q:data+=struct.pack('<4I',*record)
    calls={};word_count=0;results=[]
    for index,(containing,volume,label) in enumerate(cases):
        guest=clone(g)
        guest.mem_write(sphere,volume[:12]);guest.mem_write(sp+16,volume[12:])
        guest.mem_write(sp+28,struct.pack('<I',containing&0xffffffff))
        def trace(_guest,at,_size,_data):calls[f'{at:08x}']=calls.get(f'{at:08x}',0)+1
        for at in (0x1320f0,0x132b80,0x132d60,0x134390,0x132220,0x132e60,0x134300):
            guest.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
        run(guest,0x1320f0,ret)
        context=u(read(guest,0x4c0b80,4));assert context==0x54aa68
        bank=u(read(guest,context,4));count=struct.unpack('<H',read(guest,bank+0xa6c,2))[0]
        words=[u(read(guest,context+0x2a60,4)),u(read(guest,context+0x2a78,4)),u(read(guest,context+0x2a7c,4)),
            *struct.unpack('<Ihh',read(guest,context+4,8)),
            struct.unpack('<H',read(guest,bank,2))[0],count]
        words=[v&0xffffffff for v in words]
        assert words[:3]==[1,1,1]
        #132D60 writes only the cluster ID and first count, leaving other
        # range fields retained. Sphere-mode consumers do not read those.
        for i in range(count):words.extend(struct.unpack('<2H',read(guest,bank+0xa6e+i*26,4)))
        words.extend(struct.unpack('<'+'I'*count,read(guest,bank+6000,count*4)))
        words.extend(struct.unpack('<144I',read(guest,context+0x1c,576)))
        qq=queues(guest,context)
        for q in qq:
            words.append(len(q))
            for record in q:words.extend(record)
        associations=struct.unpack('<H',read(guest,context+0xa5e,2))[0]
        words.append(associations)
        words.extend(struct.unpack('<'+'I'*(associations*4),read(guest,context+0xa60,associations*16)))
        assert read(guest,bank+4,len(camera_bytes))==camera_bytes
        data+=struct.pack('<I',containing&0xffffffff)+volume+struct.pack('<I',len(words))
        data+=struct.pack('<'+'I'*len(words),*words)
        word_count+=len(words)
        results.append(dict(case=index,label=label,cluster=containing,constructed=count,
            clusters=len(qq[0]),instances=len(qq[1]),associations=associations))
        if index%32==0:print('receiver',index,'/',len(cases),flush=True)
    (a.output/'fixture.bin').write_bytes(data)
    report=dict(boundary=__doc__,snapshot_sha256=PIN,registers={k:f'{v:08x}' for k,v in registers.items()},
        stack_words=[f'{v:08x}' for v in args],camera_count=registers['ECX'],camera_sha256=hashlib.sha256(camera_bytes).hexdigest(),
        sphere_hex=original_sphere.hex(),main_queues=main_queues,fog_datum=f'{datum:08x}',fog=fog,
        context='0054aa68',flags=1,pass_mode=1,query_mode=1,cases=len(cases),compared_words=word_count,
        results=results,original_calls=calls,intercepted_instructions=0,
        correction='Earlier light-receiver-call-original-v1.json incorrectly read ECX=6 as a pointer for camera_bank_count. ECX is the actual camera count.',
        fixture_sha256=hashlib.sha256(data).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(cases),'whole original receiver queries',word_count,'owned words',flush=True)

if __name__=='__main__':main()
