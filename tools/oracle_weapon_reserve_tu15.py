#!/usr/bin/env python3
"""Untouched104AA0/1059C0 and104D30/105C50 shared ammunition execution.

Literal magazine definitions, five allocated weapon records and four retained
unit inventory slots are inputs. The original infinite-ammunition predicate
and changed notification are explicit service boundaries. Every magazine byte,
query value, spend result and notification count is checked in both updates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from oracle_weapon_barrel_timing_tu15 import Original as Base
from oracle_weapon_barrel_timing_tu15 import WEAPON,UNIT,DEF,BARRELS,HEADERS,TAGS,STACK,STOP,pack,u32
from weapon_reference_versions import images
from unicorn.x86_const import *

OBJECTS=(WEAPON,0x1040000,0x1042000,0x1044000,0x1046000)
WI,UI=0x12340000,0x12340005
CONFIG={'TU1.1':((0x104aa0,0x104d30),0x10a1d0,0xb8250),
        'TU1.5':((0x1059c0,0x105c50),0x10b0f0,0xb9390)}


class Original(Base):
    def __init__(self,label,image):
        super().__init__(label,image)
        self.reserve_entries,self.infinite_entry,self.changed_entry=CONFIG[label]
        self.changed=0;self.infinite=False

    def audit(self,g,address,size,data):
        super().audit(g,address,size,data)
        if address not in (self.infinite_entry,self.changed_entry):return
        if address==self.infinite_entry:
            assert g.reg_read(UC_X86_REG_EAX)==WI
            result=int(self.infinite)
        else:
            self.changed+=1;result=0
        sp=g.reg_read(UC_X86_REG_ESP);ret=u32(g.mem_read(sp,4))
        g.reg_write(UC_X86_REG_EAX,result);g.reg_write(UC_X86_REG_ESP,sp+4);g.reg_write(UC_X86_REG_EIP,ret)

    def execute(self,root,magazines,words):
        g=self.g;self.changed=0;self.infinite=bool(words[4])
        for index,body in enumerate(OBJECTS):
            g.mem_write(HEADERS+index*12,struct.pack('<HBBII',0x1234,3,2,0,body))
            g.mem_write(body,bytes(0x300))
            definition=8 if index and words[7]&(1<<index) else 7
            g.mem_write(body,pack([definition]));g.mem_write(body+0x154,pack([0xffffffff]))
            g.mem_write(body+0x224,pack(words[14+index*8:22+index*8]))
        g.mem_write(HEADERS+5*12,struct.pack('<HBBII',0x1234,3,0,0,UNIT))
        g.mem_write(WEAPON+0x154,pack([UI if words[3] else 0xffffffff]))
        g.mem_write(UNIT+0x218,pack([WI+value if value!=0xffffffff else value for value in words[8:12]]))
        for index in (0,1):
            g.mem_write(TAGS+(7+index)*16+8,pack([DEF+index*0x1000]))
            g.mem_write(DEF+index*0x1000,root)
            g.mem_write(DEF+index*0x1000+0x2c4,pack([BARRELS+index*0x1000]))
            for number,raw in enumerate(magazines):g.mem_write(BARRELS+index*0x1000+number*0x5c,raw)
        args=[WI,words[5] if words[0]==0 else words[6]]
        g.mem_write(STACK,pack([STOP,*args]));g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_EAX,words[2])
        g.emu_start(self.reserve_entries[words[0]],STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+12
        result=g.reg_read(UC_X86_REG_EAX)
        if words[0]:result&=255
        return [result,self.changed,*(word for body in OBJECTS for word in struct.unpack('<8I',g.mem_read(body+0x224,32)))]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scope',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args();scope=json.loads(args.scope.read_text())
    originals=[Original(label,image) for label,image,_ in images() if label in CONFIG]
    rng=random.Random(0x105c50);seen=set();definitions=[];counts=[0,0]
    lines=['# 54 inputs; query/spend result, changed count and all40 retained magazine words.']
    for map_ in scope['maps']:
        for weapon in map_['weapons']:
            if not weapon['resident'] or not weapon['magazines']:continue
            root=bytes.fromhex(weapon['root_hex']);magazines=[bytes.fromhex(m['raw_hex']) for m in weapon['magazines']]
            assert len(magazines)<=2
            maxima=[struct.unpack_from('<h',raw,0xc)[0] for raw in magazines]
            key=tuple(maxima)
            if key in seen:continue
            seen.add(key);definitions.append(dict(map=map_['path'],tag=weapon['tag'],root_hex=root.hex(),magazines_hex=[raw.hex() for raw in magazines]))
            controls=[]
            for index in range(len(magazines)):
                for has_unit in (0,1):
                    for infinite in (0,1):
                        for slots in ((1,2,3,4),(0,0xffffffff,1,2),(1,1,1,1),(0xffffffff,)*4):
                            for mismatch in (0,0b10100):
                                for reserves in ((0,0,0,0,0),(1,2,3,4,5),(100,0,1,32767,7),(-1,0,1,-1,32767)):
                                    for operation in (0,1):
                                        for amount in (-1,0,1,7,32767):
                                            controls.append((operation,index,has_unit,infinite,amount&1,amount,mismatch,slots,reserves))
                for _ in range(100):
                    controls.append((rng.randrange(2),index,rng.randrange(2),rng.randrange(2),rng.randrange(2),
                        rng.randrange(-100,100000),rng.randrange(32),tuple(rng.choice((0xffffffff,0,1,2,3,4)) for _ in range(4)),
                        tuple(rng.randrange(-32768,32768) for _ in range(5))))
            for operation,index,has_unit,infinite,self_only,amount,mismatch,slots,reserves in controls:
                states=[]
                for reserve in reserves:
                    raw=bytearray(rng.randbytes(32))
                    for magazine in range(2):struct.pack_into('<h',raw,magazine*16+6,reserve)
                    states.extend(struct.unpack('<8I',raw))
                words=[operation,len(magazines),index,has_unit,infinite,self_only,amount&0xffffffff,mismatch,
                    *slots,*(maxima+[0]*(2-len(maxima))),*states]
                words=[value&0xffffffff for value in words];assert len(words)==54
                results=[original.execute(root,magazines,words) for original in originals]
                assert results[0]==results[1],(key,words,results)
                lines.append(' '.join(f'{value:08x}' for value in words+results[0]));counts[operation]+=1
    args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,definitions=definitions,case_counts=counts,instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope_sha256=hashlib.sha256(args.scope.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            entries=[f'{value:08x}' for value in o.reserve_entries],
            supplied_service_addresses=[f'{o.infinite_entry:08x}',f'{o.changed_entry:08x}'],
            reached_instruction_bytes={f'{address:08x}':raw for address,raw in sorted(o.instructions.items())}) for o in originals])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(definitions=len(definitions),case_counts=counts)),flush=True)


if __name__=='__main__':main()
