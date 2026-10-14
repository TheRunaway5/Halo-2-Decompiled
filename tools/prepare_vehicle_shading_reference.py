#!/usr/bin/env python3
"""Authenticate captured TU1.5 vehicle inputs; execute unchanged BC200 velocity.

This prepares an offline shader replay. Original gameplay, clocks, other world
objects and first-person presentation are not restored by the fixture.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
TEXT_SHA='d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert not a.output.exists();state=json.loads((a.capture/'state.json').read_text())
    assert state['whole_live_text_matches_original'] and state['text_sha256']==TEXT_SHA
    assert hashlib.sha256((a.capture/'tu15-text.bin').read_bytes()).hexdigest()==TEXT_SHA
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    sites=set(complete_body(code,0xbc200).code)|set(complete_body(code,0x1d8c80).code)
    objects={o['object']:o for o in state['objects']};bodies={o: (a.capture/f'object-{o:08x}.bin').read_bytes() for o in objects}
    banks=json.loads((a.capture/'velocity-banks.json').read_text());rows=[];hashes={}
    for name in ['state.json','camera.bin','next-view-header.bin','original.png','velocity-banks.json',*['object-%08x.bin'%o for o in objects]]:
        hashes[name]=hashlib.sha256((a.capture/name).read_bytes()).hexdigest()
    for object,o in objects.items():
        if o['type']!=1:continue
        root=object;seen=set()
        while True:
            assert root not in seen;seen.add(root);parent=struct.unpack_from('<I',bodies[root],0x14)[0]
            if parent==0xffffffff:break
            root=parent
        rb=bodies[root];row=rb[0xaf]
        if row==255 or 'lighting_words' not in objects[root]:continue
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
        for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
        pages=set(range(0x10000,0x810000,4096))
        for bank in banks:
            address=bank['address'];data=bytes.fromhex(bank['bytes'])
            for page in range(address&~4095,(address+len(data)+4095)&~4095,4096):
                if page not in pages:cpu.mem_map(page,4096);pages.add(page)
            cpu.mem_write(address,data)
        for address in [0x60048000,0x60300000]:cpu.mem_map(address,4096)
        cpu.mem_map(0x600ef000,8192)
        cpu.mem_write(0x600f0000,struct.pack('<II',0x60300000,0));cpu.reg_write(UC_X86_REG_EAX,object);cpu.reg_write(UC_X86_REG_ECX,0x60048000);cpu.reg_write(UC_X86_REG_ESP,0x600f0000)
        trace=[]
        def instruction(uc,pc,size,_):
            assert pc in sites and bytes(uc.mem_read(pc,size))==image.read(pc,size);trace.append(pc)
        cpu.hook_add(UC_HOOK_CODE,instruction);cpu.emu_start(0xbc200,0x60300000,count=10000);assert cpu.reg_read(UC_X86_REG_EIP)==0x60300000
        data=bodies[object];size,offset=struct.unpack_from('<hh',data,0x114);assert size>0 and size%0x34==0 and offset>=0 and offset+size<=len(data)
        cached_path=a.capture/f'cache-{object:08x}.bin'
        if not cached_path.exists():continue
        cached=cached_path.read_bytes();hashes[cached_path.name]=hashlib.sha256(cached).hexdigest()
        rows.append(dict(object=object,definition=o['definition'],netgame_row=row,geometry_flags=struct.unpack_from('<I',data,4)[0],
            geometry_control_flags=struct.unpack_from('<H',data,0x10a)[0],geometry_lods=list(struct.unpack_from('<4b',cached,0xbc)),nodes=[list(struct.unpack_from('<13I',data,i)) for i in range(offset,offset+size,0x34)],
            sphere=list(struct.unpack_from('<4I',data,0x30)),origin=list(struct.unpack_from('<3I',rb,0x30)),flags=struct.unpack_from('<I',rb,4)[0],frozen=bool(rb[0xc1]&1),
            velocity=list(struct.unpack('<3I',cpu.mem_read(0x60048000,12))),velocity_original_trace=trace,expected_lighting=objects[root]['lighting_words']))
    view=(a.capture/'next-view-header.bin').read_bytes()
    a.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5',xbe_sha256=NEW_PIN,capture=str(a.capture.resolve()),sha256=hashes,
        boundary=__doc__,camera=list(struct.unpack_from('<12I',view,0x18)),vehicles=rows),indent=2)+'\n')
    print(f'PASS {len(rows)} captured original vehicle WORLD/lighting inputs; unchanged BC200 velocity, no services replaced')

if __name__=='__main__':main()
