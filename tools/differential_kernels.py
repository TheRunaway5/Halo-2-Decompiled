#!/usr/bin/env python3
"""Compare native scalar crouch port with original retail x86, not Ghidra C.
Requires pip install unicorn==2.1.4 in a separate test environment.
Only a kernel runs under Unicorn; the C++ runtime never executes Xbox code.
"""
import argparse, ctypes, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_XMM0, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def main():
    p=argparse.ArgumentParser();p.add_argument('--library',type=Path,default=ROOT/'build/libhalo2_kernels.so');p.add_argument('--output',type=Path,default=ROOT/'analysis/systems/crouch-differential.json');args=p.parse_args()
    image=(ROOT/'assets/disc/default.xbe').read_bytes()
    assert hashlib.sha256(image).hexdigest()==EXPECTED,'unexpected executable'
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for i in range(count):
        _,address,virtual,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        guest.mem_write(address,image[offset:offset+size])
    guest.mem_map(0x1000000,0x10000);guest.mem_map(0x2000000,0x10000);guest.mem_map(0x3000000,0x1000)
    # Retail tick startup calls CRT__control87(0x9001F, 0xFFFFF): 53-bit x87
    # precision and nearest-even, rather than the emulator's default precision.
    guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
    def u32(at,value):guest.mem_write(at,struct.pack('<I',value))
    def f32(at,value):guest.mem_write(at,struct.pack('<f',value))
    # Synthetic tag/object/time pools exercise the real routine's memory ABI.
    u32(0x4e0300,0x1000000);u32(0x1000044,0x1000100);u32(0x1000108,0x1001000)
    u32(0x1001000,0);u32(0x100112c,0xffffffff)
    u32(0x4e3b44,0x1002000);u32(0x1002008,0x1003000)
    u32(0x510c54,0x1004000)
    native=ctypes.CDLL(str(args.library.resolve())).halo2_crouch_delta
    native.argtypes=[ctypes.c_float]*3+[ctypes.c_int]*2;native.restype=ctypes.c_float
    rng=random.Random(0xDD7D0);cases=[]
    for crouch in [0,.00001,.1,.5,.99999,1]:
        for dt in [0,1/60,1/30,.1]:
            for velocity in [0,.25,2,30]:
                for held in [0,1]:
                    for forced in [0,1]:cases.append((crouch,dt,velocity,held,forced))
    cases += [(rng.random(),rng.random()/10,rng.random()*20,rng.randrange(2),rng.randrange(2)) for _ in range(10000)]
    failures=[]
    for c,dt,v,held,forced in cases:
        f32(0x10012c4,c);f32(0x1003250,v);f32(0x1004004,dt)
        u32(0x1001134,held<<23);u32(0x1001348,(1<<13) if forced else 0)
        guest.mem_write(0x10013dc,bytes([6 if forced else 0]));guest.mem_write(0x10013f8,bytes([forced]))
        stack=0x200fff0;u32(stack,0x3000000);guest.reg_write(UC_X86_REG_ESP,stack);guest.reg_write(UC_X86_REG_EAX,0)
        guest.emu_start(0xdd7d0,0x3000000,count=1000)
        assert guest.reg_read(UC_X86_REG_EIP)==0x3000000,'kernel did not return to sentinel'
        expected=guest.reg_read(UC_X86_REG_XMM0)&0xffffffff
        actual=struct.unpack('<I',struct.pack('<f',native(c,dt,v,held,forced)))[0]
        if expected!=actual:failures.append({'input':[c,dt,v,held,forced],'expected':hex(expected),'actual':hex(actual)})
    cast=ctypes.CDLL(str(args.library.resolve())).halo2_scalar_cast
    cast.argtypes=[ctypes.c_uint32,ctypes.c_uint,ctypes.c_uint];cast.restype=ctypes.c_uint32
    pairs=[(5,6,0x20a000),(5,8,0x20a000),(5,7,0x20a020),(6,7,0x20a070),(6,8,0x20a090)]
    pairs += [(6,s,0x20a0b0) for s in range(44,50)]
    pairs += [(7,6,0x20a0d0),(8,6,0x20a0f0),(8,7,0x20a100),(7,8,0x20a110)]
    values=[0,1,0xffffffff,0xffff0000,0x80000000,0x7fffffff,0x3fc00000,0xbfc00000,0x7fc00000,0xffc00000,0x7f800000,0xcf000000,0x4f000000,0x12345678]+[rng.getrandbits(32) for _ in range(1000)]
    cast_failures=[]
    for target,source,address in pairs:
        for bits in values:
            stack=0x200fff0;u32(stack,0x3000000);u32(stack+4,bits);guest.reg_write(UC_X86_REG_ESP,stack)
            guest.emu_start(address,0x3000000,count=1000)
            assert guest.reg_read(UC_X86_REG_EIP)==0x3000000,'cast did not return'
            expected=guest.reg_read(UC_X86_REG_EAX)&0xffffffff;actual=cast(bits,source,target)
            if expected!=actual:cast_failures.append({'source':source,'target':target,'bits':hex(bits),'expected':hex(expected),'actual':hex(actual)})
    fade=ctypes.CDLL(str(args.library.resolve())).halo2_fade_curve
    fade.argtypes=[ctypes.c_float,ctypes.POINTER(ctypes.c_uint8)];fade.restype=ctypes.c_float
    fade_table=(ctypes.c_uint8*1024).from_buffer_copy(image[0x468fc8:0x4693c8])
    fade_values=[-1,0,1,2]+[i/30 for i in range(31)]+[i/1023 for i in range(1024)]+[rng.random() for _ in range(10000)]
    fade_failures=[]
    guest.reg_write(UC_X86_REG_FPCW,0x027f)
    for x in fade_values:
        xbits=struct.unpack('<I',struct.pack('<f',x))[0]
        stack=0x200fff0;u32(stack,0x3000000)
        guest.reg_write(UC_X86_REG_ESP,stack);guest.reg_write(UC_X86_REG_EAX,5);guest.reg_write(UC_X86_REG_XMM0,xbits)
        guest.emu_start(0x17ca10,0x3000000,count=10000)
        assert guest.reg_read(UC_X86_REG_EIP)==0x3000000,'fade curve did not return'
        expected=guest.reg_read(UC_X86_REG_XMM0)&0xffffffff
        actual=struct.unpack('<I',struct.pack('<f',fade(x,fade_table)))[0]
        if expected!=actual:fade_failures.append({'fraction':x,'expected':hex(expected),'actual':hex(actual)})
    result={'xbe_sha256':EXPECTED,'retail_entry':'000dd7d0','cases':len(cases),'bit_exact_matches':len(cases)-len(failures),'failures':failures[:20],
            'scalar_casts':{'helpers':9,'typed_pairs':len(pairs),'cases':len(pairs)*len(values),'bit_exact_matches':len(pairs)*len(values)-len(cast_failures),'failures':cast_failures[:20]},
            'fade_curve':{'retail_entry':'0017ca10','curve':5,'rounding':'x87 nearest even, 53-bit precision','cases':len(fade_values),'bit_exact_matches':len(fade_values)-len(fade_failures),'failures':fade_failures[:20]},
            'scope':'actor speed override is supplied as input; no full biped, camera, or VM parity claimed'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(failures or cast_failures or fade_failures)
if __name__=='__main__':raise SystemExit(main())
