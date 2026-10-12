#!/usr/bin/env python3
"""Compare complete particle push buffers with the unchanged original XBE.

23ED30's geometry operand is EBX, not the order in the recovered C prototype.
Execute its actual instructions with explicit bindings and independently
captured sprite inputs. Only the push allocation is replaced with writable
memory. This proves packet construction, not live emission or GPU pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDI, UC_X86_REG_ESI

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000
PUSH=STATE+0x1000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-binary',type=Path,required=True)
    parser.add_argument('--sprite-submissions',type=Path,
        default=ROOT/'analysis/frontend/title-particle-sprite-differential.retail.bin')
    parser.add_argument('--cases',type=int,default=192)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-submission-differential.json')
    args=parser.parse_args()
    if args.cases<1:parser.error('--cases must be positive')
    image=Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()==EXPECTED
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(STATE,0x100000);guest.mem_map(STOP,4096)
    guest.mem_write(0x470a38,b'\x01')
    guest.mem_write(0x407488,struct.pack('<I',STATE+0x100))
    allocations=[]

    def hook(g,address,size,_):
        if address!=0x15d00:return
        allocations.append(g.reg_read(UC_X86_REG_ESI))
        g.mem_write(g.reg_read(UC_X86_REG_EDI),struct.pack('<I',PUSH))
        sp=g.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',g.mem_read(sp,4))[0]
        g.reg_write(UC_X86_REG_EAX,1);g.reg_write(UC_X86_REG_ESP,sp+4);g.reg_write(UC_X86_REG_EIP,ret)

    guest.hook_add(UC_HOOK_CODE,hook)
    guest.mem_write(STACK,struct.pack('<I',STOP));guest.reg_write(UC_X86_REG_ESP,STACK)
    guest.emu_start(0x23ecd0,STOP,count=10000)
    initial=struct.unpack('<8I',guest.mem_read(0x470a3c,32))
    assert initial==tuple(range(1,9))
    sprites=args.sprite_submissions.read_bytes();assert sprites and len(sprites)%88==0
    rng=random.Random(0x23ed30)
    inputs=bytearray(struct.pack('<I',args.cases));original=bytearray()
    for case in range(args.cases):
        index=case*(len(sprites)//88)//args.cases
        raw=sprites[index*88:index*88+88]
        bindings=list(initial)
        if case%2:rng.shuffle(bindings)
        guest.mem_write(0x470a3c,struct.pack('<8I',*bindings))
        guest.mem_write(STATE,raw[:12]);guest.mem_write(STATE+0x20,raw[12:24])
        guest.mem_write(STATE+0x40,raw[40:56]);guest.mem_write(STATE+0x60,raw[56:72])
        guest.mem_write(STATE+0x80,raw[72:88]);guest.mem_write(PUSH,b'\xcc'*184)
        size,angle,color,blend=struct.unpack_from('<4I',raw,24)
        guest.mem_write(STACK,struct.pack('<9I',STOP,STATE,STATE+0x20,size,angle,color,blend,STATE+0x60,STATE+0x80))
        guest.reg_write(UC_X86_REG_EBX,STATE+0x40);guest.reg_write(UC_X86_REG_ESP,STACK)
        guest.emu_start(0x23ed30,STOP,count=10000)
        assert guest.reg_read(UC_X86_REG_EIP)==STOP and guest.reg_read(UC_X86_REG_ESP)==STACK+36
        end=struct.unpack('<I',guest.mem_read(STATE+0x100,4))[0]
        assert end==PUSH+184
        original+=guest.mem_read(PUSH,184);inputs+=raw+struct.pack('<8I',*bindings)
    assert allocations==[46]*args.cases
    args.output.parent.mkdir(parents=True,exist_ok=True)
    input_path=args.output.with_suffix('.inputs.bin');input_path.write_bytes(inputs)
    retail_path=args.output.with_suffix('.retail.bin');retail_path.write_bytes(original)
    output_path=args.output.with_suffix('.native.bin')
    subprocess.run([str(args.native_binary.resolve()),str(input_path),str(output_path)],check=True)
    native=output_path.read_bytes();assert len(native)==len(original)
    different=[i for i,(a,b)in enumerate(zip(struct.iter_unpack('<I',native),struct.iter_unpack('<I',original)))if a!=b]
    report={'xbe_sha256':EXPECTED,'original_entry':'0023ed30','binding_initializer':'0023ecd0',
        'default_bindings':initial,'replaced_allocator':'00015d00','words_per_submission':46,
        'geometry_operand':'EBX','uv_a_operand':'stack argument 7','uv_b_operand':'stack argument 8',
        'sprite_source_sha256':hashlib.sha256(sprites).hexdigest(),
        'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
        'fixture_sha256':hashlib.sha256(inputs).hexdigest(),'fixtures':args.cases,
        'different_words':len(different),'first_different_words':different[:20],
        'complete_submissions_byte_exact':native==original,
        'particle_simulation_verified':False,'gpu_attribute_conversion_verified':False,
        'gpu_rasterization_verified':False,'whole_frame_parity_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    raise SystemExit(native!=original)


if __name__=='__main__':main()
