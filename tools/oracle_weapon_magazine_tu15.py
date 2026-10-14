#!/usr/bin/env python3
"""Execute untouched original magazine transfer and mode-transition consumers.

Raw owned tag bytes, literal 16-byte magazine states and tick rate are inputs.
Ammunition availability/spend/notification and animation lookup/timing/action
are declared service boundaries. Ordered calls and all retained magazine bytes
are compared. This does not establish reload admission, animation ownership,
reserve/infinite-ammunition stores, or a complete original weapon tick.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct

from oracle_weapon_barrel_timing_tu15 import Original as Base
from oracle_weapon_barrel_timing_tu15 import WEAPON, DEF, BARRELS, TIME, STACK, STOP, WI, pack, u32
from weapon_reference_versions import images
from unicorn.x86_const import *

CONFIG = {
    'TU1.1': ((0x106d40,0x109c30),(0x104aa0,0x104d30,0xa8ab0,0x1680b3,0x16a06a,0x10a140)),
    'TU1.5': ((0x107c60,0x10ab50),(0x1059c0,0x105c50,0xb1f00,0x169493,0x16b44a,0x10b060)),
}


class Original(Base):
    def __init__(self,label,image):
        super().__init__(label,image)
        self.magazine_entries,services=CONFIG[label]
        self.services={address:index for index,address in enumerate(services)}

    def audit(self,g,address,size,data):
        super().audit(g,address,size,data)
        if address not in self.services:return
        kind=self.services[address]
        sp=g.reg_read(UC_X86_REG_ESP)
        read=lambda at:u32(g.mem_read(at,4))
        ret,pop,result=read(sp),0,0
        if kind==0:
            assert g.reg_read(UC_X86_REG_EAX)==0 and read(sp+4)==WI and read(sp+8)==0
            arg,pop,result=0,8,self.words[7]
        elif kind==1:
            assert g.reg_read(UC_X86_REG_EAX)==0 and read(sp+4)==WI
            arg,pop=read(sp+8),8
        elif kind==2:
            assert g.reg_read(UC_X86_REG_EAX)==WI
            arg=0
        elif kind==3:
            arg,result=g.reg_read(UC_X86_REG_ECX),self.words[8]
        elif kind==4:
            assert g.reg_read(UC_X86_REG_ECX)==WI and read(sp+4)==self.words[8]
            arg,pop=read(sp+8),8
            assert arg in (0,3)
            result=self.words[9 if arg==0 else 10]
        else:
            assert g.reg_read(UC_X86_REG_EBX)==WI
            arg,pop=read(sp+4),4
        state=struct.unpack('<4I',g.mem_read(WEAPON+0x224,16))
        self.events.extend((kind,arg,*state))
        g.reg_write(UC_X86_REG_EAX,result)
        g.reg_write(UC_X86_REG_ESP,sp+4+pop)
        g.reg_write(UC_X86_REG_EIP,ret)

    def execute(self,root,magazine,words):
        self.words,self.events=words,[]
        g=self.g
        g.mem_write(DEF,root)
        g.mem_write(DEF+0x2c4,pack([BARRELS]))
        g.mem_write(BARRELS,magazine)
        g.mem_write(WEAPON,pack([7]))
        g.mem_write(WEAPON+0x12c,pack([words[6]]))
        g.mem_write(WEAPON+0x224,pack(words[12:16]))
        g.mem_write(TIME+2,struct.pack('<H',words[5]&65535))
        args=[WI] if words[0]==0 else [WI,0]
        g.mem_write(STACK,pack([STOP,*args]))
        g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_EDI,0)
        g.reg_write(UC_X86_REG_EAX,words[11])
        g.reg_write(UC_X86_REG_FPCW,0x027f)
        g.reg_write(UC_X86_REG_FPSW,0)
        g.reg_write(UC_X86_REG_FPTAG,0xffff)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(self.magazine_entries[words[0]],STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP
        assert g.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4
        return [*struct.unpack('<4I',g.mem_read(WEAPON+0x224,16)),len(self.events)//6,*self.events]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scope',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    scope=json.loads(args.scope.read_text())
    originals=[Original(label,image) for label,image,_ in images() if label in CONFIG]
    rng=random.Random(0x107c60)
    definitions=[];seen=set();rows=['# 16 inputs; four retained magazine words and ordered six-word service events.'];counts=[0,0]
    for map_ in scope['maps']:
        for weapon in map_['weapons']:
            if not weapon['resident']:continue
            root=bytes.fromhex(weapon['root_hex'])
            for authored in weapon['magazines']:
                magazine=bytes.fromhex(authored['raw_hex'])
                flags=u32(magazine)
                capacity,amount=struct.unpack_from('<hh',magazine,0xa)[0],struct.unpack_from('<h',magazine,0x14)[0]
                chamber=u32(magazine,0x18)
                key=flags,capacity,amount,chamber
                if key in seen:continue
                seen.add(key)
                definitions.append(dict(map=map_['path'],tag=weapon['tag'],root_hex=root.hex(),magazine_hex=magazine.hex()))
                controls=[]
                for loaded in sorted({-1,0,1,capacity-1,capacity,32767}):
                    for item in (0,8):
                        for available in (-1,0,7,32767):
                            controls.append((0,0,loaded,item,available,30,-1,-1))
                    for requested in range(7):
                        for total,transfer in ((-1,-1),(1,0),(37,15),(65537,32768)):
                            controls.append((1,requested,loaded,8,7,30,total,transfer))
                for _ in range(100):
                    controls.append((rng.randrange(2),rng.randrange(7),rng.randrange(-32768,32768),
                        rng.choice((0,8,9)),rng.randrange(-65536,65536),rng.choice((1,30,60,32767)),
                        rng.choice((-1,0,1,7,65537)),rng.choice((-1,0,1,7,32768))))
                for operation,requested,loaded,item,available,rate,total,transfer in controls:
                    state=bytearray(rng.randbytes(16))
                    struct.pack_into('<h',state,8,loaded)
                    words=[operation,flags,capacity&65535,amount&65535,chamber,rate,item,available&0xffffffff,
                        0x0b000065,total&0xffffffff,transfer&0xffffffff,requested,*struct.unpack('<4I',state)]
                    results=[original.execute(root,magazine,words) for original in originals]
                    assert results[0]==results[1],(key,words,results)
                    rows.append(' '.join(f'{value:08x}' for value in words+results[0]));counts[operation]+=1
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='weapon-magazine-original-v1',scope=__doc__,definitions=definitions,case_counts=counts,
        instruction_patches=[],scope_sha256=hashlib.sha256(args.scope.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            entries=[f'{value:08x}' for value in o.magazine_entries],
            supplied_service_addresses=[f'{value:08x}' for value in o.services],
            executed_instructions={f'{address:08x}':raw for address,raw in sorted(o.instructions.items())}) for o in originals])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(definitions=len(definitions),case_counts=counts)),flush=True)


if __name__=='__main__':main()
