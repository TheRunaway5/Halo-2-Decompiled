#!/usr/bin/env python3
"""Recreate local caller goldens from owned unchanged TU1.5 and pinned captures.

The candidate native implementation is never imported or executed. Original
2B868..2B8AB and2BA96 produce all252 caller controls. Genuine completed original
main banks are authenticated inputs and are copied, never synthesized.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from oracle_smg_firing_1_1 import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ECX
XBE_SHA='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MAP_SHA='5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
CAPTURES={
    'fleet':('e80fc2d489e0fcc5794376b58457d060712035af03f16c4af4bbc7d4c258c20d',1558,32800),
    'destruction':('fbef1d9f6671be6ab102051b4797f3816ef42893b5e253acb4d7c11dcd2c7091',2800,34081),
}
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def require(value,why):
    if not value:raise ValueError(why)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--map',type=Path,required=True)
    p.add_argument('--fleet-bank',type=Path,required=True);p.add_argument('--destruction-bank',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    require(not a.output.exists(),'Preserve existing local fixture evidence')
    require(sha(a.xbe)==XBE_SHA,'Expected owned English OG Xbox TU1.5 executable')
    require(sha(a.map)==MAP_SHA,'Expected authenticated original00a_introduction cache')
    frame_rows={}
    for name,path in [('fleet',a.fleet_bank),('destruction',a.destruction_bank)]:
        pin,tick,frame=CAPTURES[name];raw=path.read_bytes()
        require(len(raw)==288 and sha(path)==pin,'Expected genuine completed original '+name+' bank')
        frame_rows[name]={'source':str(path),'source_sha256':pin,'bytes_hex':raw.hex(),'game_tick':tick,'frame':frame,
            'map_sha256':MAP_SHA,'capture':'xemu0.8.136 / Complex4627 / actual completed original main publication; not physical Xbox or final framebuffer'}
    x=Xbe(a.xbe);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    stack,camera=0x600f0000,0x600f2000;g.mem_map(0x60000000,0x200000)
    f32=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
    depth=[(.015625,1024.,v)for v in [-1.,-0.,0.,.015625,.01562500186264515,.03125,10.,1024.,2048.]]
    depth += [(.015625,1024.,v)for v in [float('nan'),float('inf'),float('-inf')]]
    depth += [(0.,1.,v)for v in [-0.,0.,.5,1.,2.]]
    depth += [(.5,1.,.25),(.5,1.,.5),(.5,1.,.75),(.5,1.,1.)]
    rows=[];random_source=random.Random(0x2b868)
    for near,far,limit in depth:
        for clear in [0,1,255]:
            for enabled in [0,1,2,255]:
                bank=bytearray(random_source.randbytes(0x120));bank[0x18]=enabled;struct.pack_into('<f',bank,0x14,limit)
                rgb=random_source.randbytes(12)
                g.mem_write(stack+0x58,bytes(bank));g.mem_write(stack+0x17,bytes([clear]));g.mem_write(stack+0x28,rgb)
                g.mem_write(camera+0x4c,struct.pack('<2f',near,far));g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_EBP,camera)
                g.emu_start(0x2b868,0x2b8ab,count=1000);require(g.reg_read(UC_X86_REG_EIP)==0x2b8ab,'Original caller did not complete')
                g.reg_write(UC_X86_REG_ESI,stack+0x58);g.reg_write(UC_X86_REG_EDI,0x4c6710);g.reg_write(UC_X86_REG_ECX,0x48)
                g.emu_start(0x2ba96,0x2ba98,count=1000);require(g.reg_read(UC_X86_REG_EIP)==0x2ba98,'Original full publication did not complete')
                rows.append({'input':bank.hex(),'rgb':rgb.hex(),'clear':clear,'near_bits':f32(near),'far_bits':f32(far),
                    'output':bytes(g.mem_read(0x4c6710,0x120)).hex(),'far_after_bits':struct.unpack('<I',g.mem_read(camera+0x50,4))[0]})
    result={'status':'PASS_UNCHANGED_ORIGINAL_CALLER_EXECUTION','authority':'OG Xbox TU1.5','xbe_sha256':XBE_SHA,
        'original_code_patches':[],'segments':[{'first':first,'last_exclusive':last,'bytes_hex':x.read(first,last-first).hex(),
            'sha256':hashlib.sha256(x.read(first,last-first)).hexdigest()}for first,last in [(0x2b868,0x2b8ab),(0x2ba96,0x2ba98)]],
        'scope':'Explicit caller-stage bank, RGB and near/far controls. Full original producer verified separately; no authored scene/GPU claim.',
        'cases':rows,'genuine_main_frames':frame_rows,'generator_sha256':sha(Path(__file__))}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS252 unchanged-original caller controls and2 authenticated genuine main banks:',sha(a.output))
if __name__=='__main__':main()
