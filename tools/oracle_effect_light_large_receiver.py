#!/usr/bin/env python3
"""Compare native large-sphere preparation with original retail BSP prefix.

C15A0 and31C80 execute unchanged over retained real RAM. Each declared radius,
shape radius and portal mask is input data. Whole1320F0 runs through134390 and
stops at132220's object-admission entry, before objects add association rows.
No instruction, callee result or admission outcome is supplied. This proves
the BSP prefix, not dynamic objects, TU1.5 FP state or rendered-frame parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot, u

ROOT=Path(__file__).resolve().parents[1]
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native',type=Path);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--generate-only',action='store_true')
    p.add_argument('--native-banks',action='store_true',help='Feed literal retained BSP inputs to the standalone component probe')
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    if not a.native and not a.generate_only:p.error('--native is required for comparison')
    snapshot=ROOT/'analysis/multiplayer-shading/world-effects-goal-20261011/original-session/frag-light-draw'
    s=Snapshot(snapshot);assert hashlib.sha256(s.ram).hexdigest()==PIN
    g=s.guest();g.mem_map(0x70000000,0x100000);stack,stop=0x700fe000,0x700fff00
    def read(guest,at,n):
        regions=list(guest.mem_regions())
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(begin<=page<=end for begin,end,_ in regions):
                guest.mem_map(page,4096);guest.mem_write(page,s.read(page,4096))
        return bytes(guest.mem_read(at,n))
    def run(guest,entry,end=stop):
        guest.emu_start(entry,end,count=40000000)
        assert guest.reg_read(R.UC_X86_REG_EIP)==end,(hex(entry),hex(guest.reg_read(R.UC_X86_REG_EIP)))
    def call(guest,entry,registers=()):
        guest.mem_write(stack,struct.pack('<I',stop));guest.reg_write(R.UC_X86_REG_ESP,stack)
        guest.reg_write(R.UC_X86_REG_FPCW,0x023f);guest.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        for register,value in registers:guest.reg_write(register,value&0xffffffff)
        run(guest,entry)
    def clone(guest):
        result=s.guest()
        for begin,end,permissions in guest.mem_regions():
            result.mem_map(begin,end-begin+1,permissions);result.mem_write(begin,bytes(guest.mem_read(begin,end-begin+1)))
        result.context_restore(guest.context_save());return result
    def queues(guest,context):
        out=[]
        for offset in (0xc,0x18):
            header=u(read(guest,context+offset,4));count=struct.unpack('<H',read(guest,header+4,2))[0]
            fp,ip,op,ap=struct.unpack('<4I',read(guest,header+8,16))
            out.append([[u(read(guest,ip+4*i,4)),struct.unpack('<H',read(guest,fp+2*i,2))[0],
                u(read(guest,op+4*i,4)),struct.unpack('<H',read(guest,ap+2*i,2))[0]]for i in range(count)])
        return out
    call(g,0xc15a0);g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack)
    run(g,0x31c80,0x1320f0)
    sp=g.reg_read(R.UC_X86_REG_ESP)
    ret,flags,cameras,sphere,radius,auxiliary,clipping,cluster=struct.unpack('<8I',read(g,sp,32))
    assert g.reg_read(R.UC_X86_REG_EAX)==1 and g.reg_read(R.UC_X86_REG_ECX)==6,(g.reg_read(R.UC_X86_REG_EAX),g.reg_read(R.UC_X86_REG_ECX))
    center=read(g,sphere,12);camera_bytes=read(g,cameras,6*0x1bc);main_queues=queues(g,auxiliary)
    live_mask=read(g,u(read(g,0x4f93a4,4)),64)
    bsp=u(s.read(0x4e0348,4));nc=u(s.read(bsp+0x9c,4));datum=u(s.read(0x4b9f8c,4));fog=[];fg=clone(g)
    for i in range(nc):
        call(fg,0x16e210,((R.UC_X86_REG_ECX,i),(R.UC_X86_REG_EDI,datum)));fog.append(fg.reg_read(R.UC_X86_REG_EAX)&255)
    frame=(snapshot/'bindings-entry/light-frame.bin').read_bytes()
    shape=(snapshot/'bindings-entry/shape.bin').read_bytes()
    bank_input=None
    if a.native_banks:
        def ws(at,n):return list(struct.unpack('<'+'I'*n,s.read(at,n*4)))
        def h(at):return struct.unpack('<H',s.read(at,2))[0]
        def block(at,stride):
            n,start=ws(at,2);return [start+i*stride for i in range(n)]
        collision=u(s.read(bsp+0x18,4));planes=[ws(p,4)for p in block(collision+8,16)]
        portals=[dict(back=h(p),front=h(p+2),plane=u(s.read(p+4,4)),flags=u(s.read(p+24,4)),
            vertices=[ws(v,3)for v in block(p+28,12)])for p in block(bsp+0x5c,36)]
        cluster_rows=block(bsp+0x9c,176)
        bank_input=dict(planes=planes,portals=portals,cluster_portals=[[h(q)for q in block(p+0x8c,2)]for p in cluster_rows],
            clusters=[dict(geometry=h(p+0x24),bounds=ws(p+0x54,6),instances=[h(q)for q in block(p+0x98,2)])for p in cluster_rows],
            instances=[dict(definition=struct.unpack('<h',s.read(p+0x34,2))[0],sphere=ws(p+0x3c,4))for p in block(bsp+0x140,88)],
            definitions=[h(p+0x24)for p in block(bsp+0x138,200)])
    def cube(guest,r):
        saved=guest.context_save();base=0x70080000;sh=bytearray(shape)
        struct.pack_into('<3f',sh,0x2c,*struct.unpack('<3f',center));struct.pack_into('<f',sh,0x44,r)
        guest.mem_write(base+0x1000,frame);guest.mem_write(base+0x2000,bytes(sh))
        guest.mem_write(base+0x3000,bytes(6*0x1bc));guest.mem_write(base+0x4000,bytes(4))
        guest.mem_write(base+0xe000,struct.pack('<5I',stop,base+0x1000,base+0x2000,base+0x3000,base+0x4000))
        guest.reg_write(R.UC_X86_REG_ESP,base+0xe000);run(guest,0xc1d90)
        assert struct.unpack('<h',guest.mem_read(base+0x4000,2))[0]==6
        out=bytes(guest.mem_read(base+0x3000,6*0x1bc));guest.context_restore(saved);return out
    rng=random.Random(0x134c00)
    masks=[('live',live_mask),('closed',bytes(64)),('open',b'\xff'*64),('declared-random',struct.pack('<16I',*[rng.getrandbits(32)for _ in range(16)]))]
    cases=[(r,r,label,mask)for r in (3.,3.5,4.,5.,5.625,6.,10.,20.)for label,mask in masks]
    cases.extend((r,2.,'captured-shape-live',live_mask)for r in (3.,4.,6.))
    data=bytearray(struct.pack('<7I',0x3252434c,len(cases),nc,len(camera_bytes)//4,
        struct.unpack('<h',s.read(0x4686c4,2))[0]&0xffffffff,datum,u(s.read(0x4e64c8,4))))
    data+=camera_bytes+bytes(fog);data+=bytes((-len(data))%4)
    for q in main_queues:
        data+=struct.pack('<I',len(q))
        for record in q:data+=struct.pack('<4I',*record)
    calls={};results=[];word_count=0
    for index,(query_radius,shape_radius,label,mask)in enumerate(cases):
        guest=clone(g);cams=cube(guest,shape_radius);guest.mem_write(cameras,cams)
        guest.mem_write(sp+16,struct.pack('<f',query_radius));guest.mem_write(0x70090000,mask)
        guest.mem_write(0x4f93a4,struct.pack('<I',0x70090000))
        def trace(_g,at,_size,_data):calls[f'{at:08x}']=calls.get(f'{at:08x}',0)+1
        for at in (0x1320f0,0x132b80,0x173130,0x134390,0x132220):guest.hook_add(UC_HOOK_CODE,trace,begin=at,end=at)
        run(guest,0x1320f0,0x132220)
        context=u(read(guest,0x4c0b80,4));bank=u(read(guest,context,4));count=struct.unpack('<H',read(guest,bank+0xa6c,2))[0]
        words=[u(read(guest,context+0x2a60,4)),u(read(guest,context+0x2a78,4)),u(read(guest,context+0x2a7c,4)),
            *struct.unpack('<Ihh',read(guest,context+4,8)),struct.unpack('<H',read(guest,bank,2))[0],count]
        words=[v&0xffffffff for v in words];assert words[:3]==[1,1,0]
        for i in range(count):words.extend(struct.unpack('<13H',read(guest,bank+0xa6e+i*26,26)))
        words.extend(struct.unpack('<'+'I'*count,read(guest,bank+6000,count*4)))
        nf=struct.unpack('<H',read(guest,bank+0x1970,2))[0];words.append(nf)
        for i in range(nf):
            row=struct.unpack('<66I',read(guest,bank+0x1974+i*0x108,0x108));words.extend(row[:3]+row[4:])
        words.extend(struct.unpack('<144I',read(guest,context+0x1c,576)))
        qq=queues(guest,context)
        for q in qq:
            words.append(len(q))
            for record in q:words.extend(record)
        associations=struct.unpack('<H',read(guest,context+0xa5e,2))[0];words.append(associations)
        words.extend(struct.unpack('<'+'I'*(associations*4),read(guest,context+0xa60,associations*16)))
        data+=struct.pack('<I',cluster)+center+struct.pack('<f',query_radius)+center+struct.pack('<f',shape_radius)+mask+cams
        data+=struct.pack('<I',len(words))+struct.pack('<'+'I'*len(words),*words);word_count+=len(words)
        results.append(dict(case=index,radius=query_radius,shape_radius=shape_radius,mask=label,constructed=count,frustums=nf,
            clusters=len(qq[0]),instances=len(qq[1]),associations=associations))
    fixture=scratch/'large-receiver.bin';fixture.write_bytes(data)
    if a.generate_only:
        a.report.write_text(json.dumps(dict(passed=False,validation_not_run=True,original_prefixes_completed=len(cases),
            scope=__doc__,results=results,original_calls=calls,compared_words=word_count,snapshot_sha256=PIN,
            fixture_sha256=hashlib.sha256(data).hexdigest(),intercepted_instructions=0,supplied_original_outcomes=False,
            retained_large_artifacts=[]),indent=2)+'\n')
        print('Generated',len(cases),'unchanged original BSP prefixes; native comparison not run',flush=True);return 0
    native_bank_path=scratch/'literal-bsp-input.json'
    if bank_input is not None:native_bank_path.write_text(json.dumps(bank_input,separators=(',',':')))
    native_input=native_bank_path if a.native_banks else(ROOT/'assets/disc/maps/lockout.map').resolve(strict=True)
    result=subprocess.run([str(a.native.resolve()),str(native_input),str(fixture)],capture_output=True,text=True)
    report=dict(passed=result.returncode==0,scope=__doc__,snapshot_sha256=PIN,cases=len(cases),compared_words=word_count,
        results=results,original_calls=calls,intercepted_instructions=0,supplied_original_outcomes=False,
        fixture_sha256=hashlib.sha256(data).hexdigest(),native_binary_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),
        literal_bsp_inputs=a.native_banks,literal_bsp_sha256=hashlib.sha256(native_bank_path.read_bytes()).hexdigest()if a.native_banks else None,
        native_exit_code=result.returncode,native_stdout=result.stdout[-4000:],native_stderr=result.stderr[-4000:],retained_large_artifacts=[])
    a.report.write_text(json.dumps(report,indent=2)+'\n');print(result.stdout,result.stderr,flush=True);return result.returncode


if __name__=='__main__':raise SystemExit(main())
