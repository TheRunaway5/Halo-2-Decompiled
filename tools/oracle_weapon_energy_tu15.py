#!/usr/bin/env python3
"""Untouched TU1.1/TU1.5 heat-phase and complete battery-owner execution.

Heat enters the declared live registers/stack at102516/103436 and stops at
10275E/10367E, before trigger admission. Real authored root bytes are supplied.
Owning unit, animation-tail and effect services are explicit ABI boundaries;
requests, stores and IEEE arithmetic are compared, not successful effects or a
complete game tick. Battery executes complete105F60/106E80, with declared
player infinite-ammunition and unavailable-effect boundaries. No instructions
are patched and no visual/gameplay result is inferred from supplied services.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from oracle_weapon_barrel_timing_tu15 import Original as TimingOriginal
from oracle_weapon_barrel_timing_tu15 import WEAPON,UNIT,DEF,STACK,STOP,WI,UI,TIME,pack,u32
from weapon_reference_versions import images
from unicorn.x86_const import *

CONFIG={
    'TU1.1': (0x102516,0x10275e,0x105f60,
        (0x1060d0,0xc9a40,0x10a140,0x16a06a,0x107c10,0x179080,0x178cd0,0x15b410,0x107b50)),
    'TU1.5': (0x103436,0x10367e,0x106e80,
        (0x106ff0,0xcabc0,0x10b060,0x16b44a,0x108b30,0x17a4a0,0x17a0f0,0x15c7f0,0x108a70)),
}

class Original(TimingOriginal):
    def __init__(self,label,image):
        super().__init__(label,image)
        self.heat_start,self.heat_stop,self.age_entry,services=CONFIG[label]
        self.services={address:index for index,address in enumerate(services)}
        self.inherited_entries=self.entries
        self.active=False

    def audit(self,g,address,size,_):
        super().audit(g,address,size,_)
        if not self.active or address not in self.services:return
        kind=self.services[address]
        sp=g.reg_read(UC_X86_REG_ESP)
        read=lambda at:u32(g.mem_read(at,4))
        ret,pop,result=read(sp),0,0
        if kind==0:result=UI if self.words[21] else 0xffffffff
        elif kind==1:self.events.append((0,0,0));pop=4
        elif kind==2:self.events.append((1,read(sp+4),0));pop=4
        elif kind==3:
            assert read(sp+4)==0x08000071 and read(sp+8)==1
            self.events.append((2,0,0));result=self.words[22];pop=8
        elif kind==4:self.events.append((3,self.words[7],0));result=self.words[23]
        elif kind==5:self.events.append((4,g.reg_read(UC_X86_REG_EAX),1))
        elif kind==6:
            assert read(sp+4)==1
            self.events.append((4,g.reg_read(UC_X86_REG_EDI),0));pop=4
        elif kind==7:result=self.words[25];pop=4
        elif kind==8:
            assert read(sp+4)==0xffffffff and read(sp+8)==read(sp+12)==0
            self.events.append((5,g.reg_read(UC_X86_REG_EAX),0));result=0xffffffff;pop=12
        g.reg_write(UC_X86_REG_EAX,result)
        g.reg_write(UC_X86_REG_ESP,sp+4+pop)
        g.reg_write(UC_X86_REG_EIP,ret)

    def call_energy(self,root,words):
        g=self.g;self.words=words;self.events=[]
        g.mem_write(DEF,root)
        g.mem_write(WEAPON,pack([words[10]]))
        g.mem_write(WEAPON+0x12c,pack([words[11]]))
        g.mem_write(WEAPON+0x16c,pack([words[12]]))
        g.mem_write(WEAPON+0x180,pack(words[13:16]))
        g.mem_write(WEAPON+0x244,pack([words[16]]))
        g.mem_write(WEAPON+0x17c,struct.pack('<H',words[17]&65535))
        # State word18 is a reserved zero, to preserve an explicit fixed ABI.
        assert words[18]==0
        g.mem_write(WEAPON+0x154,pack([UI if words[21] else 0xffffffff]))
        g.mem_write(UNIT+0x13c,pack([0]))
        g.mem_write(TIME+4,pack([words[19]]))
        g.mem_write(STACK,bytes(0x80))
        if words[0]==0:
            g.mem_write(STACK+0x2c,pack([DEF]))
            g.mem_write(STACK+0x4c,pack([WI]))
            g.reg_write(UC_X86_REG_EBP,WEAPON)
            g.reg_write(UC_X86_REG_ECX,DEF)
            start,stop=self.heat_start,self.heat_stop
        else:
            g.mem_write(STACK,pack([STOP]))
            g.reg_write(UC_X86_REG_EBX,WI)
            g.reg_write(UC_X86_REG_XMM0,words[24])
            start,stop=self.age_entry,STOP
        g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_FPCW,0x027f)
        g.reg_write(UC_X86_REG_FPSW,0)
        g.reg_write(UC_X86_REG_FPTAG,0xffff)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        self.active=True
        g.emu_start(start,stop,count=10000)
        self.active=False
        assert g.reg_read(UC_X86_REG_EIP)==stop
        assert g.reg_read(UC_X86_REG_ESP)==STACK+(4 if words[0] else 0)
        out=[u32(g.mem_read(WEAPON+at,4)) for at in (0,0x12c,0x16c,0x180,0x184,0x188,0x244)]
        out += [int.from_bytes(g.mem_read(WEAPON+0x17c,2),'little'),0]
        # weapon_flags is a retained WORD; input flags occupy the next WORD.
        out[2]&=65535
        return out+[len(self.events),*(v for event in self.events for v in event)]

def f32(value):return struct.unpack('<I',struct.pack('<f',value))[0]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--scope',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--fixture',type=Path,required=True)
    args=ap.parse_args();scope=json.loads(args.scope.read_text())
    originals=[Original(label,image)for label,image,_ in images()if label in CONFIG]
    rng=random.Random(0x103436);seen=set();definitions=[];counts=[0,0]
    lines=['# TU1.1/TU1.5 original heat fragment and complete battery owner. 26 inputs; nine state outputs and variable requests.']
    for map_ in scope['maps']:
        for weapon in map_['weapons']:
            if not weapon['resident']:continue
            root=bytes.fromhex(weapon['root_hex'])
            fields=[u32(root,at)for at in (0x150,0x154,0x160,0x168,0x254)]
            fields += [struct.unpack_from('<H',root,0x292)[0],u32(root,0x170),u32(root,0x2fc),u32(root,0x304)]
            if tuple(fields)in seen:continue
            seen.add(tuple(fields));definitions.append(dict(tag=weapon['tag'],map=map_['path'],fields=fields,root_hex=root.hex()))
            threshold,maximum=[struct.unpack('<f',pack([v]))[0]for v in fields[:2]]
            controls=[(heat,spent,flags,inhib,holstered,tail,handle,blocking,unit,allocation)
                for heat in sorted({0.,threshold,max(0.,threshold-0.00001),maximum,maximum+0.1})
                for flags in (0,2,6,8)
                for spent in (0.,0.75)
                for inhib in (0.,1.)
                for holstered,tail,handle,blocking,unit,allocation in
                    ((0,0,0xffffffff,0,1,0xffffffff),(2,30,0x12340011,5,0,0xffffffff))]
            controls += [(rng.uniform(0,1.2),rng.choice((0.,0.5,1.)),rng.randrange(256),
                rng.choice((0.,1.)),rng.choice((0,2)),rng.choice((-1,1,5,30)),
                rng.choice((0xffffffff,0x12340011)),rng.choice((-1,0,1,9)),rng.randrange(2),
                rng.choice((0xffffffff,0x12340012)))for _ in range(40)]
            for heat,spent,flags,inhib,holstered,tail,handle,blocking,unit,allocation in controls:
                words=[0,*fields,7,9|holstered,flags,f32(heat),f32(spent),f32(inhib),handle,blocking&65535,0,
                    f32(1/30),f32(1),unit,tail&65535,allocation,0,0]
                assert len(words)==26
                results=[o.call_energy(root,words)for o in originals]
                assert results[0]==results[1],('heat',weapon['tag'],words,results)
                lines.append(' '.join(f'{v:08x}'for v in words+results[0]));counts[0]+=1
            amounts=sorted({u32(bytes.fromhex(b['raw_hex']),0xa8)for b in weapon['barrels']}|{0,f32(0.01),f32(0.5),f32(1)})
            for spent in (0.,0.5,0.999,1.,1.1):
                for amount in amounts:
                    for item_flags,unit,infinite in ((0,0,0),(8,0,0),(9,1,0),(9,1,1)):
                        words=[1,*fields,7,item_flags,0,0,f32(spent),0,0xffffffff,0,0,
                            f32(1/30),f32(1),unit,0,0xffffffff,amount,infinite]
                        results=[o.call_energy(root,words)for o in originals]
                        assert results[0]==results[1],('age',weapon['tag'],words,results)
                        lines.append(' '.join(f'{v:08x}'for v in words+results[0]));counts[1]+=1
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(schema='weapon-energy-original-v1',scope=__doc__,definitions=definitions,case_counts=counts,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope_sha256=hashlib.sha256(args.scope.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),instruction_patches=[],
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            heat_range=[f'{o.heat_start:08x}',f'{o.heat_stop:08x}'],battery_entry=f'{o.age_entry:08x}',
            supplied_service_addresses=[f'{v:08x}'for v in o.services],
            executed_instructions={f'{a:08x}':raw for a,raw in sorted(o.instructions.items())})for o in originals])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(definitions=len(definitions),case_counts=counts,fixture=str(args.fixture))),flush=True)

if __name__=='__main__':main()
