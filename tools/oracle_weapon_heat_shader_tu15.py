#!/usr/bin/env python3
"""Complete original shader-input owner for raw heat and age, all three images.

Only literal object/definition table and raw state words are supplied. The
complete FF5F0/1037A0/1046C0 owners execute without substituted callees on these
selectors. No rendered pixels, interpolation clock or visual parity is inferred.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from weapon_reference_versions import images
from oracle_weapon_barrel_timing_tu15 import WEAPON,DEF,POOL,HEADERS,TAGS,STACK,STOP,OUT,WI,pack,u32
from functions import _Code,_trace
from research_mp_inventory_switch_u5 import norm
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
CONFIG={'retail':(0xff5f0,0x4e0300,0x4e3b44),'TU1.1':(0x1037a0,0x4e78d0,0x4eb114),
        'TU1.5':(0x1046c0,0x4ecbd0,0x4f0414)}
class Original:
    def __init__(self,label,image):
        self.label,self.image=label,image;self.entry,pool,tags=CONFIG[label]
        g=self.g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(0x1000000,0x100000);g.mem_map(0x2000000,0x20000);g.mem_map(STOP,4096)
        g.mem_write(pool,pack([POOL]));g.mem_write(tags,pack([TAGS]));g.mem_write(POOL+0x44,pack([HEADERS]))
        g.mem_write(HEADERS+8,pack([WEAPON]));g.mem_write(WEAPON,pack([7]));g.mem_write(TAGS+7*16+8,pack([DEF]))
        self.instructions={};g.hook_add(UC_HOOK_CODE,self.audit)
        code=_Code(image,'.text');body=_trace(code,self.entry,{self.entry})
        ins=[code.at(p)for p in sorted(body.code)];positions={i.address:n for n,i in enumerate(ins)}
        self.normalized=[norm(i,image,positions)for i in ins]
        self.complete_body={f'{i.address:08x}':i.bytes.hex()for i in ins}
    def audit(self,g,address,size,_):
        raw=bytes(g.mem_read(address,size));assert raw==self.image.read(address,size)
        self.instructions[f'{address:08x}']=raw.hex()
    def call(self,heat,spent,selector):
        g=self.g;g.mem_write(WEAPON+0x180,pack([heat,spent]));g.mem_write(OUT,bytes(8))
        g.mem_write(STACK,pack([STOP,WI,selector,OUT,OUT+4]));g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(self.entry,STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+20
        assert g.reg_read(UC_X86_REG_EAX)&255==1
        return [u32(g.mem_read(OUT,4)),g.mem_read(OUT+4,1)[0]]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
    originals=[Original(label,image)for label,image,_ in images()]
    assert all(o.normalized==originals[0].normalized for o in originals)
    values=[0,0x80000000,0xbf800000,0x3f000000,0x3f7fffff,0x3f800000,0x3f800001,
        0x40000000,0x7f800000,0xff800000,0x7fc01234,0xffc05678]
    lines=['# raw heat, raw spent, selector, original value, enabled'];count=0
    for heat in values:
        for spent in values:
            for selector in (0x04000567,0x03000577):
                results=[o.call(heat,spent,selector)for o in originals];assert results[0]==results[1]==results[2]
                lines.append(' '.join(f'{v:08x}'for v in(heat,spent,selector,*results[0])));count+=1
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,cases=count,supplied_services=[],instruction_patches=[],
        normalized_complete_bodies_equal=True,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        images=[dict(label=o.label,sha256=hashlib.sha256(o.image.data).hexdigest(),entry=f'{o.entry:08x}',
            complete_body=o.complete_body,executed_instructions=o.instructions)for o in originals])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=count,images=len(originals),services=0)),flush=True)
if __name__=='__main__':main()
