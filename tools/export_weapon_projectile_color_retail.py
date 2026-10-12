#!/usr/bin/env python3
"""Unchanged retail projectile selector plus RGB interpolation on Needler data.

Explicit finite travel distances and source speed-override flag controls.
Original shot, motion, cache residency, whole BE8E0 scheduling and rendered
pixels are separate; the native renderer consumes its actual traveled bank.
"""
import hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
from oracle_mp_inventory_1_1 import Cache,ROOT
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    image=Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    c=Cache(ROOT/'assets/disc/maps/lockout.map');datum=0xe3960221;p=c.tags[datum]['offset'];definition=p+c.magic
    channels=c.block(p+0xac,16);assert len(channels)==1 and c.u(channels[0])==0 and c.u(channels[0]+8)==1
    function=c.u(channels[0]+12)-c.magic;raw=bytes(c.data[function:function+40])
    assert struct.unpack_from('<2I',raw)==(0,0) and struct.unpack_from('<2I',raw,32)==(0,0x12000565)
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    base=c.magic&~4095;u.mem_map(base,(c.magic+len(c.data)-base+4095)&~4095);u.mem_write(c.magic,bytes(c.data))
    state=0x2100000;u.mem_map(state,0x200000);stack=state+0x1ff000;stop=state+0x1fe000
    def put(a,*w):u.mem_write(a,struct.pack('<'+'I'*len(w),*(v&0xffffffff for v in w)))
    put(0x4e0300,state);put(state+0x44,state+0x1000);put(state+0x1008,state+0x2000)
    put(0x4e3b44,state+0x10000);put(state+0x10000+(datum&65535)*16+8,definition);put(state+0x2000,datum)
    def run(a,args=(),regs=()):
        put(stack,stop,*args);u.reg_write(UC_X86_REG_ESP,stack)
        u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPCW,0x027f)
        u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
        for r,v in regs:u.reg_write(r,v)
        u.emu_start(a,stop,count=10000);assert u.reg_read(UC_X86_REG_EIP)==stop
    rng=random.Random(0xfbe70);distances=[-1.,-0.,0.,.00001,.1,.2,.5,.9,1.,1.1,30.]
    distances.extend(rng.uniform(-1,3)for _ in range(500));rows=[]
    for flags in (0,0x100):
        for distance in distances:
            encoded=struct.pack('<f',distance);u.mem_write(state+0x2168,encoded);put(state+0x212c,flags)
            run(0xfbe70,(0,0x12000565,state+0x3000,state+0x3004))
            amount=struct.unpack('<I',u.mem_read(state+0x3000,4))[0]
            assert u.mem_read(state+0x3004,1)==bytes([struct.unpack('<f',struct.pack('<I',amount))[0]>0])
            run(0x131c20,regs=((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,c.magic+function+8),
                (UC_X86_REG_ESI,c.magic+function+20),(UC_X86_REG_EDI,state+0x3010),(UC_X86_REG_XMM5,amount)))
            initial=encoded+b''.join(bytes(c.data[p+off:p+off+4])for off in (0x17c,0x180,0x18c,0x194))
            initial+=struct.pack('<I',flags)+raw[8:32]
            output=struct.pack('<I',amount)+bytes(u.mem_read(state+0x3010,12))
            rows.append(' '.join(f'{v:08x}' for v in struct.unpack('<16I',initial+output)))
    path=ROOT/'tests/fixtures/weapon_projectile_color_original.tsv'
    path.write_text('# distance, velocities, range start/inverse, flags, lower/upper, original amount/RGB\n'+'\n'.join(rows)+'\n')
    report={'scope':__doc__,'cases':len(rows),'retail_sha256':hashlib.sha256(image.data).hexdigest(),
        'cache_sha256':hashlib.sha256(Path(c.path).read_bytes()).hexdigest() if hasattr(c,'path')else hashlib.sha256(c.data).hexdigest(),
        'function_hex':raw.hex(),'selector_sha256':hashlib.sha256(image.read(0xfbe70,0x154)).hexdigest(),
        'interpolator_sha256':hashlib.sha256(image.read(0x131c20,0x13b)).hexdigest(),
        'exporter_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
