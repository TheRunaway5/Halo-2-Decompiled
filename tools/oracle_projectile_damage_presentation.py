#!/usr/bin/env python3
"""Execute unchanged TU1.5 parentless damage material/scaleB consumer segments.

Explicit caller inputs supply material/region selection and before/after
vitality and normalized requests. No damage, collision or attachment service
is replaced. Execution stops before the unrelated B9390 notification call.
"""
import argparse, hashlib, itertools, json, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--xbe',type=Path,required=True)
    ap.add_argument('--fixture',type=Path,required=True)
    args=ap.parse_args();x=Xbe(args.xbe)
    assert hashlib.sha256(x.data).hexdigest()==PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    g.mem_map(0x1000000,0x10000)
    sp,body,info,event=0x1000000,0x1001000,0x1002000,0x1003000
    def write(at,fmt,*v):g.mem_write(at,struct.pack('<'+fmt,*v))
    stops={0xda40d,0xda769,0xda777}
    g.hook_add(UC_HOOK_CODE,lambda uc,at,size,data:uc.emu_stop() if at in stops else None)
    def execute(at):
        g.reg_write(UC_X86_REG_ESP,sp);g.reg_write(UC_X86_REG_EBP,event)
        g.reg_write(UC_X86_REG_EDI,0);g.reg_write(UC_X86_REG_EDX,info)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(at,0,count=1000);assert g.reg_read(UC_X86_REG_EIP) in stops
    cases=[];rng=random.Random(0xda6dd)
    threshold=struct.unpack('<f',x.read(0x46a0ec,4))[0]
    values=(0.,threshold/2,threshold,threshold*2,.25,1.,2.)
    for a,b,c in itertools.product(values,repeat=3):
        cases.append((86,91,rng.choice((0.,1.,2.)),rng.choice((0.,.25,1.)),
            rng.choice((-.5,0.,.25,1.,2.)),rng.choice((0.,.25,1.,2.)),a,b,c,rng.randrange(2)))
    for absorbs,pre,amount in itertools.product((0,1),(0.,.25,1.),(0.,1.)):
        cases.append((86,-1,amount,pre,.5,pre,0.,0.,0.,absorbs))
    output=[]
    for bm,sm,amount,pre,post_body,post_shield,body_request,shield_request,region_request,absorbs in cases:
        write(event+0x78,'f',0.);write(event+0x7c,'h',bm);write(info+0xcc,'h',sm)
        write(sp+0x18,'I',body);write(sp+0x28,'f',amount);write(body+0xf0,'f',pre)
        g.reg_write(UC_X86_REG_EBX,absorbs)
        execute(0xda3ce)
        write(body+0xec,'2f',post_body,post_shield);write(sp+0x38,'I',info)
        write(sp+0x54,'3f',shield_request,region_request,body_request);write(sp+0x15,'B',0)
        execute(0xda6dd)
        inputs=struct.pack('<2h7fI',bm,sm,amount,pre,post_body,post_shield,body_request,shield_request,region_request,absorbs)
        result=bytes(g.mem_read(event+0x7c,2))+bytes(2)+bytes(g.mem_read(event+0x78,4))
        output.append(inputs+result)
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_bytes(b'PDP1'+struct.pack('<II',len(output),36)+b''.join(output))
    args.fixture.with_suffix('.provenance.json').write_text(json.dumps(dict(scope=__doc__,
        image=str(args.xbe.resolve()),sha256=PIN,cases=len(output),input_extent=36,
        segments=[['0xda3ce','0xda40d'],['0xda6dd',['0xda769','0xda777']]],
        threshold_bits=x.read(0x46a0ec,4).hex(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n')
    print('PASS original TU1.5 damage presentation cases',len(output))
if __name__=='__main__':main()
