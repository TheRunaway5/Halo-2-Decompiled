#!/usr/bin/env python3
"""Original whole unit-time evaluator with declared completed query seconds.

212010 supplies parsed native-call arguments, 11DD90 supplies completed query
seconds, and 211DA0 captures the published value. The original float32 multiply,
x87 conversion, clamp and signed-short publication execute without patches.
This fixture does not establish object lookup, controller state or playback.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():raise ValueError('Preserve original evidence')
    x=Xbe(a.xbe);pin=hashlib.sha256(x.data).hexdigest()
    if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':raise ValueError('Expected English TU1.5')
    definition=struct.unpack('<I',x.read(0x480c00+0xc7*4,4))[0]
    if struct.unpack('<I',x.read(definition+4,4))[0]!=0x2ad170:raise ValueError('Original evaluator differs')
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(at,n)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    active=0;published=[];executed=set()
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    def ret(pop):
        sp=g.reg_read(R.UC_X86_REG_ESP);pc=word(sp)
        g.reg_write(R.UC_X86_REG_ESP,sp+4+pop);g.reg_write(R.UC_X86_REG_EIP,pc)
    def hook(vm,pc,n,user):
        if bytes(vm.mem_read(pc,n))!=x.read(pc,n):raise ValueError('Original instruction differs')
        executed.add(pc)
        if pc==0x212010:vm.reg_write(R.UC_X86_REG_EAX,0x2100100);ret(16)
        elif pc==0x11dd90:vm.reg_write(R.UC_X86_REG_XMM0,active);ret(0)
        elif pc==0x211da0:published.append(vm.reg_read(R.UC_X86_REG_EAX)&65535);ret(0)
    g.hook_add(UC_HOOK_CODE,hook);g.mem_write(0x2100100,struct.pack('<I',0x12340001))
    bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    values=[bits(x)for x in [0,1/30,2/30,2.5/30,3/30,3.5/30,-1/30,-1,1092.23,1e20,-1e20]]
    values += [0x7f800000,0xff800000,0x7fc00000,0x7fa12345,0x80000000,1,0x80000001]
    rng=random.Random(0x2ad170)
    values += [bits((rng.randrange(-8,32768)+rng.choice([0,.4999,.5,.5001]))/30)for _ in range(174)]
    rows=[]
    for active in values:
        published.clear();g.mem_write(0x21fff00,struct.pack('<4I',0x3000000,0xc7,1,0))
        g.reg_write(R.UC_X86_REG_ESP,0x21fff00);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(R.UC_X86_REG_FPCW,0x027f)
        g.emu_start(0x2ad170,0x3000000,count=10000)
        if g.reg_read(R.UC_X86_REG_EIP)!=0x3000000 or len(published)!=1:raise ValueError('Original evaluator incomplete')
        rows.append(dict(seconds=active,result_short=published[0]))
    report=dict(scope=__doc__,xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        entry='002ad170',instruction_patches=[],boundaries=['00212010','0011dd90','00211da0'],
        executed_instructions=len(executed),cases=rows)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(len(rows),'unchanged original unit-time conversion cases')

if __name__=='__main__':main()
