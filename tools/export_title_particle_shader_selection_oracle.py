#!/usr/bin/env python3
"""Trace original particle render setup through its original virtual methods.

Execute 2243C0, 137BD0, the particle object methods, and 23EA80 using original
cache metadata. Stop at the actual 1C590 vertex binding. Six render setup
services listed in the report are intercepted. No declaration, style or dependency result is
substituted. This validates draw setup for supplied layer/pass, not live emitter
identity, simulation, pixel shader or rasterization.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ECX

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x21fff00,0x3000000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-shader-selection.json')
    args=parser.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==EXPECTED
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(STATE,0x100000);guest.mem_map(STOP,4096)
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    base=cache.magic&~4095;guest.mem_map(base,(cache.magic+len(cache.data)-base+4095)&~4095)
    guest.mem_write(cache.magic,bytes(cache.data))
    index=cache.u32(16);header_magic=cache.u32(index)-index-32
    table=cache.u32(index+8)-header_magic+cache.magic
    guest.mem_write(0x4e3b44,struct.pack('<I',table))
    guest.mem_write(STATE,struct.pack('<13f',1,1,0,0,0,1,0,0,0,1,0,0,0))
    guest.mem_write(0x4687d0,struct.pack('<I',STATE))
    # Cleanup sizes are from the original instructions at each call boundary.
    replaced={0x2c3b0:0,0x1bd50:0,0x3f5cf0:0,0x3f7330:4,0x1bbf0:20,0x1cf50:0}
    current={};bound=[];calls=[]

    def u32(address):return struct.unpack('<I',guest.mem_read(address,4))[0]

    def hook(g,address,size,_):
        if address==0x23eace:current['template']=g.reg_read(UC_X86_REG_EAX)
        if address==0x23eb0a:current['pass']=g.reg_read(UC_X86_REG_EAX)
        if address==0x1c590:
            sp=g.reg_read(UC_X86_REG_ESP)
            vertex=g.reg_read(UC_X86_REG_EAX);classification=u32(sp+4)
            assert classification==g.reg_read(UC_X86_REG_ECX)
            root=u32(table+(vertex&65535)*16+8)
            at=u32(root+8)+classification*28
            code=bytes(g.mem_read(u32(at+16),u32(at+12)))
            bound.append({'vertex':vertex,'classification':classification,'program':code})
            g.emu_stop();return
        if address not in replaced:return
        calls.append(address)
        sp=g.reg_read(UC_X86_REG_ESP);ret=u32(sp)
        g.reg_write(UC_X86_REG_ESP,sp+4+replaced[address]);g.reg_write(UC_X86_REG_EIP,ret)

    guest.hook_add(UC_HOOK_CODE,hook)
    inputs=bytearray(struct.pack('<I',3));original=bytearray();rows=[]
    for datum in (0xe21a00a5,0xe21f00aa,0xe22100ac):
        tag,root=cache.tag(datum,'prt3');current.clear();bound.clear();calls.clear()
        guest.mem_write(STACK,struct.pack('<6I',STOP,0,0,datum,15,0))
        guest.reg_write(UC_X86_REG_EAX,tag['address']);guest.reg_write(UC_X86_REG_ESP,STACK)
        try:guest.emu_start(0x2243c0,STOP,count=100000)
        except Exception as error:raise RuntimeError(f'original particle setup fault at {guest.reg_read(UC_X86_REG_EIP):08x}')from error
        assert len(bound)==1 and guest.reg_read(UC_X86_REG_EIP)==0x1c590
        result=bound[0];code=result['program']
        original+=struct.pack('<5I',current['template'],current['pass'],result['vertex'],result['classification'],len(code))+code
        inputs+=struct.pack('<3I',datum,15,0)
        rows.append({'particle_datum':f'{datum:08x}','authored_style':cache.u16(root+4),
            'template_datum':f'{current["template"]:08x}','pass_datum':f'{current["pass"]:08x}',
            'vertex_datum':f'{result["vertex"]:08x}','classification':result['classification'],
            'program_sha256':hashlib.sha256(code).hexdigest(),'replaced_services':[f'{a:08x}'for a in calls]})
    args.output.parent.mkdir(parents=True,exist_ok=True)
    input_path=args.output.with_suffix('.inputs.bin');input_path.write_bytes(inputs)
    retail_path=args.output.with_suffix('.retail.bin');retail_path.write_bytes(original)
    output_path=args.output.with_suffix('.native.bin')
    subprocess.run([str(args.native_binary.resolve()),str(cache.path.resolve()),str(input_path),str(output_path)],check=True)
    native=output_path.read_bytes()
    report={'xbe_sha256':EXPECTED,'original_entry':'002243c0','draw_setup_entry':'0023ea80',
        'stopped_at_original_vertex_binding':'0001c590','layer':15,'pass_in_layer':0,'selections':rows,
        'native_binary_sha256':hashlib.sha256(args.native_binary.read_bytes()).hexdigest(),
        'fixture_sha256':hashlib.sha256(inputs).hexdigest(),'selection_and_program_bytes_exact':native==original,
        'original_style_program_selection_verified':native==original,'live_emitter_identity_verified':False,
        'native_particle_rasterization_verified':False,'whole_frame_parity_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    raise SystemExit(native!=original)


if __name__=='__main__':main()
