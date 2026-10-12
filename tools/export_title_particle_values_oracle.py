#!/usr/bin/env python3
"""Execute unchanged original 173BA0/248DF0 and export value-cache records.

Owner state is explicit. This does not invent emission, physics, transparent
ordering, or class-8 graphics. Live observations use the separate live exporter.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000
XBE_SHA='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
FORMAT='<5I17f3ff6f2ff9f4H'
assert struct.calcsize(FORMAT)==184
f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
def pack(flags,requested,evaluated,tick,field03,initial,system,location,particle,effects,rate,view,seeds):
    return struct.pack(FORMAT,flags,requested,evaluated,tick&0xffffffff,field03,
        *initial,*system,location,*particle,*effects,rate,*view,*seeds)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--cases',type=int,default=1024)
    args=parser.parse_args();assert 17<=args.cases<=65536
    x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==XBE_SHA
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    g.mem_map(STATE,0x100000);g.mem_map(STOP,4096)
    rng=random.Random(0x248df0);inputs=bytearray(struct.pack('<I',args.cases));outputs=bytearray()
    for n in range(args.cases):
        flags=n%8
        if flags&1 and n%3:flags|=8
        requested=1<<(n%17)if n<34 else 0x1ffff if n%3==0 else rng.randrange(0x20000)
        if not flags&1:requested&=~0x600
        evaluated=0 if n%2==0 else rng.randrange(0x20000)
        tick=(0,1,30,-1,16777217,2147483647,-2147483648)[n%7]
        field03=n%256
        initial=[f32(rng.uniform(-2,2))for _ in range(17)]
        system=[f32(rng.uniform(-10,10))for _ in range(3)]
        location=f32(rng.uniform(-5,5))
        particle=[f32(rng.random()),f32(rng.uniform(-2,2)),f32(rng.uniform(-20,20))]+[f32(rng.uniform(-1000,1000))for _ in range(3)]
        effects=[f32(rng.uniform(-2,3))for _ in range(2)]
        rate=f32((1/30,1/60,.1)[n%3])
        view=[f32(rng.uniform(-1000,1000))for _ in range(3)]+[f32(rng.uniform(-1,1))for _ in range(6)]
        seeds=[(0,1,32768,65535)[(n+i)%4]if n<64 else rng.randrange(65536)for i in range(4)]
        if n%16==0:particle[3:]=view[:3]
        raw=pack(flags,requested,evaluated,tick,field03,initial,system,location,particle,effects,rate,view,seeds)
        inputs+=raw
        system_raw=bytearray(84);struct.pack_into('<f',system_raw,4,system[0]);struct.pack_into('<2f',system_raw,40,*system[1:])
        struct.pack_into('<I',system_raw,24,0x12340003 if flags&8 else 0xffffffff)
        location_raw=bytearray(56);location_raw[3]=field03;struct.pack_into('<f',location_raw,28,location)
        particle_raw=bytearray(64);struct.pack_into('<f',particle_raw,8,particle[0]);struct.pack_into('<f',particle_raw,16,particle[1])
        struct.pack_into('<4H',particle_raw,20,*seeds);struct.pack_into('<3f',particle_raw,28,*particle[3:]);struct.pack_into('<f',particle_raw,56,particle[2])
        g.mem_write(STATE,bytes(system_raw));g.mem_write(STATE+0x100,bytes(location_raw));g.mem_write(STATE+0x200,bytes(particle_raw))
        g.mem_write(STATE+0x300,struct.pack('<Ifi',0,rate,tick));g.mem_write(0x510c54,struct.pack('<I',STATE+0x300))
        g.mem_write(0x485618,struct.pack('<9f',*view));g.mem_write(0x4ea93c,struct.pack('<I',STATE+0x400))
        g.mem_write(STATE+0x444,struct.pack('<I',STATE+0x1000));g.mem_write(STATE+0x1000+3*400+0x6c,struct.pack('<2f',*effects))
        g.mem_write(STATE+0x500,struct.pack('<17f',*initial))
        g.mem_write(STACK,struct.pack('<6I',STOP,requested&~evaluated,STATE if flags&1 else 0,
            STATE+0x100 if flags&2 else 0,STATE+0x200 if flags&4 else 0,STATE+0x500))
        g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0x173ba0,STOP,count=100000);assert g.reg_read(UC_X86_REG_EIP)==STOP
        outputs+=bytes(g.mem_read(STATE+0x500,68))+struct.pack('<I',evaluated|requested)
    fixtures=ROOT/'tests/fixtures';fixtures.mkdir(exist_ok=True)
    input_path=fixtures/'title_particle_values.inputs.bin';expected_path=fixtures/'title_particle_values.original.bin'
    input_path.write_bytes(inputs);expected_path.write_bytes(outputs)
    native=ROOT/'analysis/frontend/title-particle-values.native.bin'
    subprocess.run([str(args.native.resolve()),'--write',str(input_path),str(native)],check=True)
    actual=native.read_bytes();assert len(actual)==len(outputs)
    mismatches=[i for i,(a,b)in enumerate(zip(actual,outputs))if a!=b]
    report={'xbe_sha256':XBE_SHA,'cases':args.cases,'original_instructions':'173BA0 -> unchanged 248DF0, including 4EFB0',
        'intercepted_instructions':[],'different_bytes':len(mismatches),'first_differences':mismatches[:12],
        'original_bytes':len(outputs),'all_bytes_exact':not mismatches,
        'input_sha256':hashlib.sha256(inputs).hexdigest(),'original_sha256':hashlib.sha256(outputs).hexdigest(),
        'scope':'17-slot source getter/masked cache with explicit completed owner state; emission/physics/rendering remain separate',
        'complete_frontend_parity_verified':False}
    (ROOT/'analysis/frontend/title-particle-values.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));assert not mismatches
if __name__=='__main__':main()
