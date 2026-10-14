#!/usr/bin/env python3
"""Execute installed MCC's aspect-aware FOV conversion, with declared math services.

The modern-camera flag is supplied as true. This tests that specific camera path,
not a running MCC session, camera timing, or framebuffer parity.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE
from unicorn import x86_const as R

PIN = 'de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'
BASE, STACK, STOP, OUT = 0x180000000, 0x40000000, 0x41000000, 0x42000000

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    raw=args.dll.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==PIN,'MCC binary changed; audit the new instructions first'
    pe=pefile.PE(data=raw)
    assert pe.OPTIONAL_HEADER.ImageBase==BASE
    image=pe.get_memory_mapped_image()
    uc=Uc(UC_ARCH_X86,UC_MODE_64)
    uc.mem_map(BASE,(len(image)+4095)&~4095);uc.mem_write(BASE,image)
    uc.mem_map(STACK,0x10000);uc.mem_map(STOP,4096);uc.mem_map(OUT,4096)
    def xmm(reg,value,fmt):
        uc.reg_write(reg,int.from_bytes(struct.pack(fmt,value),'little'))
    def number(reg,fmt):
        return struct.unpack(fmt,uc.reg_read(reg).to_bytes(16,'little')[:struct.calcsize(fmt)])[0]
    def ret():
        sp=uc.reg_read(R.UC_X86_REG_RSP)
        address=struct.unpack('<Q',uc.mem_read(sp,8))[0]
        uc.reg_write(R.UC_X86_REG_RSP,sp+8);uc.reg_write(R.UC_X86_REG_RIP,address)
    def hook(uc,address,size,data):
        if address==BASE+0x6f4a20:
            uc.reg_write(R.UC_X86_REG_RAX,1);ret()
        elif address==BASE+0x9ed7a5:
            xmm(R.UC_X86_REG_XMM0,math.tan(number(R.UC_X86_REG_XMM0,'<d')),'<d');ret()
        elif address==BASE+0x9ed775:
            xmm(R.UC_X86_REG_XMM0,math.atan2(number(R.UC_X86_REG_XMM0,'<d'),number(R.UC_X86_REG_XMM1,'<d')),'<d');ret()
    uc.hook_add(UC_HOOK_CODE,hook)
    def run(horizontal,width,height):
        uc.mem_write(BASE+0x165c290,struct.pack('<4h',0,0,height,width))
        sp=STACK+0x8008;uc.mem_write(sp,struct.pack('<Q',STOP));uc.reg_write(R.UC_X86_REG_RSP,sp)
        uc.reg_write(R.UC_X86_REG_RCX,0);uc.reg_write(R.UC_X86_REG_R9,OUT)
        xmm(R.UC_X86_REG_XMM1,0.,'<f');xmm(R.UC_X86_REG_XMM2,horizontal,'<f')
        uc.emu_start(BASE+0x7dff10,STOP,count=1000)
        assert uc.reg_read(R.UC_X86_REG_RIP)==STOP
        assert struct.unpack('<f',uc.mem_read(OUT,4))[0]==struct.unpack('<f',struct.pack('<f',.895))[0]
        return number(R.UC_X86_REG_XMM0,'<f')
    f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
    radians=f32(.01745329238474369)
    cases=[]
    for degrees in range(70,121):
        for width,height in ((640,480),(1280,720),(3440,1440),(2560,720),(960,1080),(640,960)):
            horizontal=f32(degrees*radians)
            cases.append((horizontal,width,height,run(horizontal,width,height)))
    for degrees in (70,78,120):
        for zoom in (2,4,10):
            horizontal=f32(2*math.atan(math.tan(f32(degrees*radians)/2)/zoom))
            cases.append((horizontal,1280,720,run(horizontal,1280,720)))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('# MCC aspect-aware FOV: horizontal-radians width height projection-radians\n'+
        ''.join(f'{h:.9g}\t{w}\t{y}\t{v:.9g}\n' for h,w,y,v in cases))
    report=dict(reference_sha256=PIN,entries=['7dff10','6f4490'],modern_camera_flag=True,
        declared_services={'6f4a20':'modern camera enabled','9ed7a5':'host double tan','9ed775':'host double atan2'},
        cases=len(cases),fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        scope=__doc__)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
