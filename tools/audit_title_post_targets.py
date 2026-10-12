#!/usr/bin/env python3
"""Execute retail target allocation and bloom texture-coordinate construction.

Only allocation/submission is stubbed. The target IDs, sizes, coordinate
setup and coordinate callback execute from the supported retail XBE.
"""
import hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
x=(ROOT/'assets/disc/default.xbe').read_bytes()
assert hashlib.sha256(x).hexdigest()==EXPECTED
g=Uc(UC_ARCH_X86,UC_MODE_32)
g.mem_map(0x10000,0x600000);g.mem_map(0x2100000,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(0x3000000,4096)
base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
for i in range(n):
    _,a,_,o,s=struct.unpack_from('<5I',x,table-base+i*56);g.mem_write(a,x[o:o+s])
allocations=[];setup=[]
stubs={0x1db10:16,0x1df20:28,0x14bc0:8,0x142f0:0,0x1c590:4,
       0x3f5cf0:0,0x3f53d0:4,0x3f5590:4,0x3f54f0:4,0x3f59b0:4,0x34420:16}
def hook(guest,address,size,user):
    if address not in stubs:return
    sp=guest.reg_read(UC_X86_REG_ESP)
    if address==0x1df20:
        target=guest.reg_read(UC_X86_REG_EAX)
        args=struct.unpack('<7I',guest.mem_read(sp+4,28))
        allocations.append({'target':target,'width':args[1],'height':args[2]})
        guest.mem_write(0x4b4bd8+target*0x98,struct.pack('<3I',args[1],args[2],0x2200000))
    if address==0x34420:
        rect,callback,_,parameters=struct.unpack('<4I',guest.mem_read(sp+4,16))
        setup.append((bytes(guest.mem_read(rect,16)),bytes(guest.mem_read(parameters,104))))
        assert callback==0x34100
    if address in (0x1db10,0x1df20):guest.reg_write(UC_X86_REG_EAX,1)
    ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
    guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,hook)
def run(address,args=()):
    g.mem_write(0x21fff00,struct.pack('<'+'I'*(len(args)+1),0x3000000,*args))
    g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
    try:g.emu_start(address,0x3000000,count=1000000)
    except Exception:
        print('unhandled guest instruction',hex(g.reg_read(UC_X86_REG_EIP)))
        raise
    assert g.reg_read(UC_X86_REG_EIP)==0x3000000,hex(g.reg_read(UC_X86_REG_EIP))
g.mem_write(0x4b4c78,struct.pack('<I',0x2200000));g.mem_write(0x4b5138,struct.pack('<I',0x2200000))
run(0x1d770)
targets={a['target']:(a['width'],a['height'])for a in allocations}
assert targets[7]==targets[8]==(160,120) and targets[19]==(320,240),targets
g.mem_write(0x407488,struct.pack('<I',0x2350000));g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000))
g.mem_write(0x485a80,struct.pack('<I',0x2110000));g.mem_write(0x211005c,struct.pack('<I',0x2111000))
g.mem_write(0x4b9dd0,struct.pack('<4h',0,0,480,640))
draws=json.loads((ROOT/'analysis/frontend/bloom-differential.json').read_text())['fixtures'][0]['draws']
rng=random.Random(0x34100);maximum=0.;cases=0
def f(v):return struct.unpack('<f',struct.pack('<f',v))[0]
for index,draw in enumerate(draws):
    source=(640,480)if index==0 else(160,120)
    target=(640,480)if index==5 else(160,120)
    g.mem_write(0x485af4,struct.pack('<4I',*([source[0]]*4)))
    g.mem_write(0x485b04,struct.pack('<4I',*([source[1]]*4)))
    radius=struct.unpack('<I',struct.pack('<f',draw['radius']))[0]
    setup.clear()
    run(0x34a90,(draw['target'],0xffffffff,0x1010101,0,0,0x3f800000,radius,0x3f800000,draw['stages'],0,0))
    assert len(setup)==1
    rect,parameters=setup[0]
    g.mem_write(0x2100000,rect);g.mem_write(0x2100100,parameters)
    for _ in range(128):
        pixel=(f(rng.uniform(0,target[0])),f(rng.uniform(0,target[1])))
        g.mem_write(0x2100200,struct.pack('<2f',f(pixel[0]/640.),f(pixel[1]/480.)))
        for stage in range(draw['stages']):
            run(0x34100,(stage+1,0x2100000,0x2100200,0x2100300,0x2100100))
            actual=struct.unpack('<2f',g.mem_read(0x2100300,8))
            for axis in range(2):
                sign=1 if stage&(1<<axis) else -1
                expected=f(f(pixel[axis]/target[axis])+f(sign*f(draw['radius']*.5)/target[axis]))
                maximum=max(maximum,abs(actual[axis]/source[axis]-expected))
            cases+=1
assert maximum<2e-6,maximum
report={'allocations':allocations,'bloom_coordinate_cases':cases,'maximum_normalized_coordinate_error':maximum,
        'entries':['0001d770','00034a90','00034100'],'xbe_sha256':EXPECTED,
        'framebuffer_parity_verified':False}
(ROOT/'analysis/frontend/title-post-targets.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
