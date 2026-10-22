#!/usr/bin/env python3
"""Execute unchanged TU1.1/TU1.5 optic magnification for every authored weapon.

The object/tag tables and selected zoom level are declared leaf inputs.
No math service, returned magnification, or executable instruction is replaced.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from inspect_multiplayer import Cache
from weapon_reference_versions import images, unique_entry
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP,
    UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG,
    UC_X86_REG_MXCSR, UC_X86_REG_XMM0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    cache=Cache(args.maps/'shared.map')
    machines=[]
    for label,image,pin in images():
        if label not in ('TU1.1','TU1.5'):
            continue
        entry=unique_entry(image,bytes.fromhex('6683fa010f57c97e1b'),0x49)
        guest=Uc(UC_ARCH_X86,UC_MODE_32)
        guest.mem_map(0x10000,0x800000)
        for section in image.sections:
            guest.mem_write(section.va,image.section_bytes(section))
        guest.mem_map(0x1000000,0x30000)
        def put(at,value,g=guest):g.mem_write(at,struct.pack('<I',value))
        pool=struct.unpack('<I',image.read(entry+2,4))[0]
        table=struct.unpack('<I',image.read(entry+0x20,4))[0]
        put(pool,0x1000000);put(0x1000044,0x1001000)
        put(0x1001008,0x1002000);put(0x1002000,0)
        put(table,0x1003000);put(0x1003008,0x1004000)
        machines.append((label,pin,entry,guest))
    records=[]
    for tag in cache.tags.values():
        if tag['group']!='weap' or tag['offset'] is None:
            continue
        at=tag['offset'];count=struct.unpack_from('<h',cache.data,at+0x1fe)[0]
        if count<=0:
            continue
        if count>2:
            raise ValueError('Authored optic requires general power: '+tag['name'])
        root=bytes(cache.data[at:at+0x310])
        for selector in range(-1,count+1):
            outputs=[]
            for label,pin,entry,guest in machines:
                guest.mem_write(0x1004000,root)
                guest.mem_write(0x1010000,struct.pack('<I',0x1020000))
                for reg,value in ((UC_X86_REG_ESP,0x1010000),(UC_X86_REG_EAX,0x10000),
                        (UC_X86_REG_ECX,selector&65535),(UC_X86_REG_FPCW,0x027f),
                        (UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_MXCSR,0x1f80)):
                    guest.reg_write(reg,value)
                guest.emu_start(entry,0x1020000,count=100000)
                assert guest.reg_read(UC_X86_REG_EIP)==0x1020000
                outputs.append(guest.reg_read(UC_X86_REG_XMM0)&0xffffffff)
            assert outputs[0]==outputs[1],(tag['name'],selector,outputs)
            records.append(dict(weapon=tag['name'],levels=count,selector=selector,
                flags_bits=f'{cache.u(at+0x12c):08x}',magazine_count=cache.u(at+0x2c0),
                minimum_bits=f'{cache.u(at+0x200):08x}',maximum_bits=f'{cache.u(at+0x204):08x}',
                original_magnification_bits=f'{outputs[0]:08x}'))
    globals_tag=next(t for t in cache.tags.values() if t['group']=='matg' and t['offset'] is not None)
    info=cache.u(globals_tag['offset']+0x134)-cache.magic
    cameras=[dict(biped=t['name'],fov_bits=f"{cache.u(t['offset']+0xcc):08x}")
        for t in cache.tags.values() if t['group']=='bipd' and t['offset'] is not None and
        t['name'] in ('objects\\characters\\masterchief\\masterchief',
            'objects\\characters\\dervish\\dervish','objects\\characters\\masterchief\\masterchief_mp',
            'objects\\characters\\elite\\elite_mp')]
    smg=next(t for t in cache.tags.values() if t['name']=='objects\\weapons\\rifle\\smg\\smg')
    at=smg['offset']
    binocular_weapon=dict(weapon=smg['name'],flags_bits=f'{cache.u(at+0x12c):08x}',
        levels=cache.h(at+0x1fe),minimum_bits=f'{cache.u(at+0x200):08x}',
        maximum_bits=f'{cache.u(at+0x204):08x}',magazine_count=cache.u(at+0x2c0))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(scope=__doc__,records=records,
        map_sha256=hashlib.sha256(cache.data).hexdigest(),camera_fields=cameras,binocular_weapon=binocular_weapon,
        binocular=dict(levels=cache.i(info+0xb8),minimum_bits=f'{cache.u(info+0xbc):08x}',
            maximum_bits=f'{cache.u(info+0xc0):08x}'),
        images=[dict(label=m[0],sha256=m[1],entry=f'{m[2]:08x}') for m in machines],
        original_images_equal=True,instruction_patches=[],math_service_replacements=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n')
    print(len(records),'original optic magnification cases matched across TU1.1/TU1.5')


if __name__=='__main__':main()
