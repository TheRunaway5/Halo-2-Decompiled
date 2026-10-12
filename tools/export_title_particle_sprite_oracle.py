#!/usr/bin/env python3
"""Compare title sprite calculation with unchanged original Xbox instructions.

Supplies the same completed 17-value cache and initialized sprite context to
both implementations. Only the final 23ED30 GPU submission is intercepted.
This does not establish emission, simulation, transparent sorting, or pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_FPCW, UC_X86_REG_MXCSR

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-binary',type=Path,required=True)
    parser.add_argument('--cases',type=int,default=1024,help='fixtures per actual particle tag')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-sprite-differential.json')
    args=parser.parse_args()
    if args.cases<1:parser.error('--cases must be positive')
    image=Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()==EXPECTED
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(STATE,0x100000);guest.mem_map(STOP,4096)
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    base=cache.magic&~4095
    guest.mem_map(base,(cache.magic+len(cache.data)-base+4095)&~4095)
    guest.mem_write(cache.magic,bytes(cache.data))
    captured=[]

    def hook(g,address,size,_):
        if address!=0x23ed30:return
        sp=g.reg_read(UC_X86_REG_ESP);words=struct.unpack('<9I',g.mem_read(sp,36))
        raw=bytes(g.mem_read(words[1],12))+bytes(g.mem_read(words[2],12))
        raw+=struct.pack('<4I',*words[3:7])
        for pointer in (g.reg_read(UC_X86_REG_EBX),*words[7:9]):raw+=bytes(g.mem_read(pointer,16))
        captured.append(raw)
        # Original 224CD2/23EF7A: geometry in EBX, eight stack arguments,
        # callee cleanup. No property evaluation is replaced.
        g.reg_write(UC_X86_REG_ESP,sp+36);g.reg_write(UC_X86_REG_EIP,words[0])

    guest.hook_add(UC_HOOK_CODE,hook)
    rng=random.Random(0x224550)
    f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    fixtures=bytearray(struct.pack('<I',args.cases*3));original=bytearray();metadata=[]
    tags=(0xe21a00a5,0xe21f00aa,0xe22100ac)
    for index,datum in enumerate(tags):
        tag,root=cache.tag(datum,'prt3');parameter=cache.block(root+0x14,0x28)[0]
        bitmap_datum=cache.u32(parameter+12);bitmap,bitmap_root=cache.tag(bitmap_datum,'bitm')
        sequences=cache.block(bitmap_root+0x3c,0x3c)
        metadata.append({'particle':tag,'bitmap':bitmap,
            'flags':cache.u32(root),'atlas_frames':[len(cache.block(s+0x34,0x20))for s in sequences]})
        for case in range(args.cases):
            values=[f32(rng.random())for _ in range(17)]
            if case<8:values[0]=(0.,.0001,.1,.25,.5,.75,.9999,1.)[case]
            seed1,seed2=rng.getrandbits(16),rng.getrandbits(16);seed_color=rng.getrandbits(32)
            position=[f32(rng.uniform(-250,250))for _ in range(3)]
            direction=[f32(rng.uniform(-5,5))for _ in range(3)]
            scale=f32(rng.uniform(.01,10));width=f32(rng.uniform(.25,2))
            color=[f32(rng.uniform(0,1.5))for _ in range(4)]
            # Tests caller-provided null and selected contexts. No choice of
            # original runtime sequence is inferred from this fixture.
            selected=-1 if case%7==0 else 0
            seed=bytearray(64);struct.pack_into('<2H',seed,0x14,seed1,seed2)
            struct.pack_into('<I',seed,0x3c,seed_color);guest.mem_write(STATE,bytes(seed))
            guest.mem_write(STATE+0x100,struct.pack('<17f4I',*values,0xffffffff,0,0,0))
            guest.mem_write(STATE+0x200,struct.pack('<6f',*position,*direction))
            # Native public input is RGBA; retail color4f stores alpha first.
            guest.mem_write(STATE+0x300,struct.pack('<4f',color[3],*color[:3]))
            guest.mem_write(STATE+0x400,struct.pack('<3If',tag['address'],bitmap['address'],
                sequences[selected]+cache.magic if selected>=0 else 0,width))
            scale_bits=struct.unpack('<I',struct.pack('<f',scale))[0]
            guest.mem_write(STACK,struct.pack('<8I',STOP,STATE,STATE+0x100,
                STATE+0x200,STATE+0x20c,scale_bits,STATE+0x300,STATE+0x400))
            guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_FPCW,0x027f)
            guest.reg_write(UC_X86_REG_MXCSR,0x1f80);captured.clear()
            try:guest.emu_start(0x224550,STOP,count=100000)
            except Exception as error:raise RuntimeError(f'retail sprite fault at {guest.reg_read(UC_X86_REG_EIP):08x}, tag {datum:08x}, case {case}')from error
            assert guest.reg_read(UC_X86_REG_EIP)==STOP and len(captured)==1
            original+=captured[0]
            fixtures+=struct.pack('<I17fHHI6f2f4fi',index,*values,seed1,seed2,seed_color,
                *position,*direction,scale,width,*color,selected)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    inputs=args.output.with_suffix('.inputs.bin');outputs=args.output.with_suffix('.native.bin')
    retail=args.output.with_suffix('.retail.bin')
    inputs.write_bytes(fixtures);retail.write_bytes(original)
    subprocess.run([str(args.native_binary.resolve()),str(cache.path.resolve()),str(inputs),str(outputs)],check=True)
    native=outputs.read_bytes();assert len(native)==len(original)==args.cases*3*88
    mismatch=[];maximum=0.;color_errors=0;float_errors=0
    for case in range(args.cases*3):
        a,b=original[case*88:(case+1)*88],native[case*88:(case+1)*88]
        if a==b:continue
        original_words=struct.unpack('<22I',a);native_words=struct.unpack('<22I',b)
        for component,(x,y)in enumerate(zip(original_words,native_words)):
            if x==y:continue
            if component==8:color_errors+=1;error=None
            else:
                error=abs(struct.unpack('<f',struct.pack('<I',x))[0]-struct.unpack('<f',struct.pack('<I',y))[0])
                maximum=max(maximum,error);float_errors+=1
            if len(mismatch)<50:mismatch.append({'fixture':case,'component':component,
                'retail_bits':f'{x:08x}','native_bits':f'{y:08x}','absolute_error':error})
    report={'xbe_sha256':EXPECTED,'original_entry':'00224550','intercepted_submission':'0023ed30',
        'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
        'fixture_sha256':hashlib.sha256(fixtures).hexdigest(),'particle_definitions':metadata,
        'fixtures':args.cases*3,'submission_bytes':len(original),
        'maximum_float_error':maximum,'float_components_not_byte_exact':float_errors,
        'packed_color_errors':color_errors,'first_mismatches':mismatch,
        'sprite_parameters_byte_exact':native==original,
        'particle_simulation_verified':False,'gpu_shader_verified':False,'whole_frame_parity_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    raise SystemExit(native!=original)


if __name__=='__main__':main()
