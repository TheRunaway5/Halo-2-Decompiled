#!/usr/bin/env python3
"""Unchanged TU1.5 15CC70 choices over owned Xbox cache globals and rows."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache, file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert not a.output.exists();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');sites=set(complete_body(code,0x15cc70).code);result=[];covered=set()
    for name,folder in [('containment','assets/map-pack/maps'),('waterworks','assets/disc/maps'),('coagulation','assets/disc/maps'),('zanzibar','assets/disc/maps')]:
        c=Cache(Path(folder)/(name+'.map'));_,g=c.tag(next(t['datum'] for t in c.tags.values() if t['group']=='matg'),'matg');_,m=c.tag(c.u32(g+0x16c),'mulg')
        runtime=c.block(m+8,0x568)[0];choices=c.block(runtime+0x70,8);raw_choices=bytes(c.data[choices[0]:choices[0]+len(choices)*8])
        _,s=c.tag(c.scenario,'scnr');rows=[]
        for ordinal,r in enumerate(c.block(s+0x120,0x90)):
            if c.data[r+0x14]!=3:continue
            default=0xffffffff;variant=0;collection=c.u32(r+0x5c)
            if collection!=0xffffffff:
                _,collection_at=c.tag(collection,'vehc');items=c.block(collection_at,0x10);assert len(items)==1
                variant=c.u32(items[0]+12);assert c.u32(items[0]+4)==0x76656869;default=c.u32(items[0]+8)
            for mode in [0,1,2,4]:
                cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
                for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
                cpu.mem_map(0x61000000,0x6000);cpu.mem_map(0x600ef000,0x2000);cpu.mem_map(0x60300000,0x1000)
                cpu.mem_write(0x4ecc1c,struct.pack('<I',0x61000000));cpu.mem_write(0x6100016c,struct.pack('<I',0xeeff0001))
                cpu.mem_write(0x4f0414,struct.pack('<I',0x61001000));cpu.mem_write(0x61001018,struct.pack('<I',0x61002000))
                cpu.mem_write(0x6100200c,struct.pack('<I',0x61003000));cpu.mem_write(0x61003074,struct.pack('<I',0x61004000));cpu.mem_write(0x61004000,raw_choices)
                cpu.mem_write(0x61005000,struct.pack('<I',variant));cpu.mem_write(0x600f0000,struct.pack('<4I',0x60300000,default,mode,0x61005000))
                cpu.reg_write(UC_X86_REG_ESP,0x600f0000);cpu.reg_write(UC_X86_REG_EFLAGS,0x202);trace=[]
                def instruction(uc,pc,size,_):
                    assert pc in sites and bytes(uc.mem_read(pc,size))==image.read(pc,size)
                    trace.append(pc);covered.add(pc)
                cpu.hook_add(UC_HOOK_CODE,instruction);cpu.emu_start(0x15cc70,0x60300000,count=1000);assert cpu.reg_read(UC_X86_REG_EIP)==0x60300000
                rows.append(dict(row=ordinal,mode=mode,definition=cpu.reg_read(UC_X86_REG_EAX),variant=struct.unpack('<I',cpu.mem_read(0x61005000,4))[0],
                    position_words=list(struct.unpack_from('<3I',c.data,r+0x40)),rotation_words=list(struct.unpack_from('<3I',c.data,r+0x4c)),trace=trace))
        result.append(dict(map=name,cache_path=str(c.path.resolve()),cache_sha256=file_sha(c.path),cases=rows))
    a.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5',xbe_sha256=NEW_PIN,xbe_path=str(NEW_DEFAULT.resolve()),shared_maps=str(Path('assets/disc/maps').resolve()),entry=0x15cc70,scope='Fixed/default/none heavy choices only; original input globals and singleton collection identities. No service replacements or RNG/random claims.',maps=result,oracle_sha256=file_sha(Path(__file__))),indent=2)+'\n')
    print(f'PASS {sum(len(m["cases"]) for m in result)} original heavy choices, {len(covered)}/{len(sites)} instructions; random branch excluded')

if __name__=='__main__':main()
