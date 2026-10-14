#!/usr/bin/env python3
"""Original preparation misfire/detonation fragments and exact shared RNG.

Live stack/register bindings and raw authored roots are explicit ABI inputs.
The complete misfire/detonation decision fragments execute unchanged in both
images. Successful self-detonation only records1081E0/109100 as a required
owner boundary; no death/effect result is supplied or claimed.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from oracle_weapon_barrel_timing_tu15 import Original as TimingOriginal
from oracle_weapon_barrel_timing_tu15 import WEAPON,DEF,STACK,pack,u32,WI
from weapon_reference_versions import images
from unicorn.x86_const import *
RANDOM=0x1038000
CONFIG={'TU1.1':((0x10756a,0x1075f0),(0x1079ca,0x107a36),0x4ee9d8,0x4650c0,0x465030,0x1081e0),
        'TU1.5':((0x10848a,0x108510),(0x1088ea,0x108956),0x4f3cd8,0x46a160,0x46a0d0,0x109100)}
class Original(TimingOriginal):
    def __init__(self,label,image):
        super().__init__(label,image)
        self.ranges,self.random_global,self.scale,self.one,self.detonate=CONFIG[label][:2],*CONFIG[label][2:]
        self.g.mem_write(self.random_global,pack([RANDOM]));self.detonation=False
    def audit(self,g,address,size,_):
        super().audit(g,address,size,_)
        if address==self.detonate:
            self.detonation=True;sp=g.reg_read(UC_X86_REG_ESP)
            assert g.reg_read(UC_X86_REG_ESI)==WI
            g.reg_write(UC_X86_REG_EIP,u32(g.mem_read(sp,4)));g.reg_write(UC_X86_REG_ESP,sp+4)
    def decision(self,root,operation,value,seed):
        g=self.g;self.detonation=False;g.mem_write(DEF,root);g.mem_write(STACK,bytes(0x1c80))
        g.mem_write(STACK+0x14,pack([DEF]));g.mem_write(STACK+0x1c74,pack([WI]))
        g.mem_write(WEAPON+(0x184 if operation==0 else 0x180),pack([value]));g.mem_write(RANDOM,pack([seed]))
        g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EBP,WEAPON)
        g.reg_write(UC_X86_REG_XMM2,0);g.reg_write(UC_X86_REG_XMM3,u32(self.image.read(self.one,4)))
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        start,stop=self.ranges[operation];g.emu_start(start,stop,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==stop and g.reg_read(UC_X86_REG_ESP)==STACK
        result=int(g.mem_read(STACK+0x12,1)[0]!=0)if operation==0 else int(self.detonation)
        return [result,u32(g.mem_read(RANDOM,4))]
def f32(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--scope',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
    originals=[Original(label,image)for label,image,_ in images()if label in CONFIG]
    constants=[(u32(o.image.read(o.one,4)),u32(o.image.read(o.scale,4)))for o in originals];assert constants[0]==constants[1]
    seen=set();definitions=[];lines=['# op threshold chance one random_scale energy seed original_result original_seed'];counts=[0,0]
    rng=random.Random(0x1084b3)
    for map_ in json.loads(a.scope.read_text())['maps']:
        for weapon in map_['weapons']:
            if not weapon['resident']:continue
            root=bytes.fromhex(weapon['root_hex'])
            for operation,at in ((0,0x25c),(1,0x158)):
                threshold,chance=u32(root,at),u32(root,at+4)
                if (operation,threshold,chance)in seen:continue
                seen.add((operation,threshold,chance));definitions.append(dict(tag=weapon['tag'],operation=operation,
                    threshold=threshold,chance=chance,root_hex=root.hex()))
                threshold_f=struct.unpack('<f',pack([threshold]))[0]
                for value in sorted({0.,0.5,0.99,1.,1.25,threshold_f,max(0.,threshold_f-0.0001),threshold_f+0.0001}):
                    for seed in (0,1,0x12345678,0xffffffff,*(rng.getrandbits(32)for _ in range(24))):
                        outputs=[o.decision(root,operation,f32(value),seed)for o in originals];assert outputs[0]==outputs[1]
                        words=[operation,threshold,chance,*constants[0],f32(value),seed,*outputs[0]]
                        lines.append(' '.join(f'{v:08x}'for v in words));counts[operation]+=1
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,case_counts=counts,definitions=definitions,instruction_patches=[],
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),
            reached_instruction_bytes={f'{p:08x}':raw for p,raw in sorted(o.instructions.items())})for o in originals])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(case_counts=counts,contracts=len(seen))),flush=True)
if __name__=='__main__':main()
