#!/usr/bin/env python3
"""Check native title fog inputs against retail 1301C0/1305D0/130DE0/25A10.

The executable is only a test oracle. This does not certify final frame pixels.
"""
import argparse
import ctypes
import hashlib
import json
import math
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_MXCSR)
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,EYE,DIRECTION,STACK,STOP=0x2100000,0x2101000,0x2101100,0x21fff00,0x3000000

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=ROOT/'build-frontend/libhalo2_kernels.so')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/menu-fog-differential.json')
    args=parser.parse_args()
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED:raise ValueError('unexpected retail executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,_,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        guest.mem_write(address,image[offset:offset+size])
    guest.mem_map(STATE,0x100000);guest.mem_map(0x2300000,0x100000);guest.mem_map(STOP,4096)
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    mapped=cache.magic&~4095;size=(cache.magic+len(cache.data)-mapped+4095)&~4095
    guest.mem_map(mapped,size);guest.mem_write(cache.magic,bytes(cache.data))
    _,scenario=cache.tag(cache.scenario,'scnr')
    bsp=cache.block(scenario+0x210,68)[0]
    file,size,address=struct.unpack_from('<3I',cache.data,bsp)
    guest.mem_write(address,bytes(cache.data[file:file+size]))
    guest.mem_write(0x4e0350,struct.pack('<I',cache.magic+scenario))
    guest.mem_write(0x4e0348,struct.pack('<I',address+16))
    guest.mem_write(0x4e3b44,struct.pack('<I',0x2300000))
    for tag in cache.tags.values():
        guest.mem_write(0x2300000+(tag['datum']&65535)*16+8,struct.pack('<I',tag['address']))
    plane=cache.block(file+16+0x64,24,address-file)[0]
    plane_values=struct.unpack_from('<4f',cache.data,plane+4)
    palette=cache.block(scenario+0x348,16)[cache.u16(plane)]
    fog_datum=cache.u32(palette+8);_,fog=cache.tag(fog_datum,'fog ')
    sky=next(tag for tag in cache.tags.values()if tag['group']=='sky 'and tag['name'].endswith('main_menu'))
    _,sky_root=cache.tag(sky['datum'],'sky ')
    primary=cache.block(sky_root+0x48,24)[0]
    primary_values=struct.unpack_from('<6f',cache.data,primary)
    fog_values=struct.unpack_from('<9f',cache.data,fog+12)
    parameters=(primary_values[4],primary_values[5],primary_values[3],*plane_values,
                fog_values[0],fog_values[1],fog_values[2],fog_values[5])
    native=ctypes.CDLL(str(args.library.resolve())).halo2_menu_fog_constants
    F32=ctypes.c_float;native.argtypes=[ctypes.POINTER(F32)]*4;native.restype=ctypes.c_int
    rng=random.Random(0x25a10)
    cases=[]
    for z in [0,210,plane_values[3]-6.001,plane_values[3]-6,plane_values[3]-.0001,
              plane_values[3],plane_values[3]+.0001,240,1000,plane_values[3]+1023,plane_values[3]+1024]:
        for direction in [(1,0,0),(0,0,1),(0,0,-1),(.6,.8,0)]:cases.append(((23,-41,z),direction))
    for _ in range(3000):
        eye=(rng.uniform(-400,400),rng.uniform(-400,400),rng.uniform(100,350))
        direction=[rng.uniform(-1,1)for _ in range(3)];length=math.sqrt(sum(v*v for v in direction))
        cases.append((eye,tuple(v/length for v in direction)))
    failures=[]
    def run(entry,words=(),registers=(),until=STOP):
        guest.mem_write(STACK,struct.pack('<'+'I'*(len(words)+1),STOP,*words))
        guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for register,value in registers:guest.reg_write(register,value)
        guest.emu_start(entry,until,count=100000)
        if guest.reg_read(UC_X86_REG_EIP)!=until:raise RuntimeError(f'retail {entry:08x} did not finish')
    for index,(eye,direction)in enumerate(cases):
        state=bytearray(288)
        struct.pack_into('<6f',state,0x1c,*primary_values)
        struct.pack_into('<I',state,0x6c,0xffffffff)
        struct.pack_into('<I',state,0x9c,fog_datum)
        struct.pack_into('<3f',state,0xa0,*fog_values[6:9])
        struct.pack_into('<3f',state,0xac,*fog_values[:3])
        struct.pack_into('<2f',state,0xb8,*fog_values[3:5])
        struct.pack_into('<B',state,0xc0,1)
        struct.pack_into('<f',state,0xc4,fog_values[5])
        struct.pack_into('<4f',state,0xf4,*plane_values)
        struct.pack_into('<B',state,0x104,1)
        guest.mem_write(STATE,bytes(state));guest.mem_write(EYE,struct.pack('<3f',*eye));guest.mem_write(DIRECTION,struct.pack('<3f',*direction))
        run(0x1301c0,registers=[(UC_X86_REG_ECX,STATE)])
        run(0x1305d0,[0],[(UC_X86_REG_ECX,0),(UC_X86_REG_EAX,EYE),(UC_X86_REG_ESI,STATE)])
        run(0x130de0,registers=[(UC_X86_REG_ECX,STATE)])
        run(0x25a10,registers=[(UC_X86_REG_ECX,EYE),(UC_X86_REG_EAX,DIRECTION),(UC_X86_REG_EDX,STATE)],until=0x25c6a)
        expected=bytes(guest.mem_read(0x4077f8,96))
        result=(F32*24)();ok=native((F32*3)(*eye),(F32*3)(*direction),(F32*11)(*parameters),result)
        if not ok or bytes(result)!=expected:
            words=[i for i in range(24)if bytes(result)[i*4:i*4+4]!=expected[i*4:i*4+4]]
            failures.append({'case':index,'eye':eye,'direction':direction,'different_words':words,
                'retail':[struct.unpack_from('<f',expected,i*4)[0]for i in words],
                'native':[result[i]for i in words]})
    report={'xbe_sha256':EXPECTED,'library_sha256':hashlib.sha256(args.library.read_bytes()).hexdigest(),
        'cases':len(cases),'failures':len(failures),'examples':failures[:5],
        'entries':['001301c0','001305d0','00130de0','00025a10'],
        'scope':'all 24 title-fog vertex constants, camera-plane boundaries and submerged classification',
        'retail_frame_parity_verified':False}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
