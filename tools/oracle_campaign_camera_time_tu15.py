#!/usr/bin/env python3
"""Original camera-time values and bounded13AB94..13ABBB tick stores.

Caller director/clock banks and stack backing are explicit inputs. Whole
2B2930 publishes through a captured211DA0 boundary. The game interval ends
before13ABC0; surrounding game-frame owners and caller ABI are not supplied
as completed source execution. No instruction is patched.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise ValueError('Preserve original evidence')
    x=Xbe(a.xbe);pin=hashlib.sha256(x.data).hexdigest()
    if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':raise ValueError('Expected English TU1.5')
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(at,n)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    def put(at,v):g.mem_write(at,struct.pack('<I',v&0xffffffff))
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
    published=[];executed=set()
    def hook(vm,pc,n,user):
        if bytes(vm.mem_read(pc,n))!=x.read(pc,n):raise ValueError('Original instruction differs')
        executed.add(pc)
        if pc==0x211da0:
            published.append(vm.reg_read(R.UC_X86_REG_EAX)&65535);sp=vm.reg_read(R.UC_X86_REG_ESP)
            vm.reg_write(R.UC_X86_REG_EIP,word(sp));vm.reg_write(R.UC_X86_REG_ESP,sp+4)
    g.hook_add(UC_HOOK_CODE,hook)
    director,clock,stack,stop=0x2100000,0x2101000,0x21fff00,0x3000000
    put(0x51df6c,director);put(0x51df54,clock)
    def prepare():
        g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.mem_write(stack,struct.pack('<4I',stop,0,1,0))
    rng=random.Random(0x13ab94);queries=[];ticks=[]
    deltas=[0,1/30,1/60,1/15,-1/30,.5]
    values=[-2147483648,-32768,-2,-1,0,1,2,3,174,280,32767,2147483647]
    for n in range(192):
        frames=values[n%len(values)]if n<72 else rng.randrange(-40000,40000)
        delta=bits(deltas[(n//12)%len(deltas)])
        if n>=174:delta=[0x7f800000,0xff800000,0x7fc00000][n%3]
        g.mem_write(director,bytes([0xa5])*0x78);g.mem_write(clock,bytes(0x24));put(director+0x14,frames);put(clock+4,delta)
        prepare();published.clear();g.emu_start(0x2b2930,stop,count=10000)
        if g.reg_read(R.UC_X86_REG_EIP)!=stop or len(published)!=1:raise ValueError('Original camera query incomplete')
        queries.append(dict(frames=frames,delta=delta,result_short=published[0]))
    for n in range(192):
        frames=values[n%len(values)]if n<72 else rng.randrange(-40000,40000)
        delta=bits(deltas[(n//12)%len(deltas)]);elapsed=bits(rng.uniform(-20,20))
        g.mem_write(director,bytes([0xa5])*0x78);g.mem_write(clock,bytes([0x5a])*0x24)
        put(director+0x14,frames);put(director+4,elapsed);put(clock+4,delta)
        before=bytes(g.mem_read(director,0x78));clock_before=bytes(g.mem_read(clock,0x24))
        prepare();g.emu_start(0x13ab94,0x13abc0,count=10000)
        if g.reg_read(R.UC_X86_REG_EIP)!=0x13abc0:raise ValueError('Original camera tick interval incomplete')
        ticks.append(dict(before=before.hex(),after=bytes(g.mem_read(director,0x78)).hex(),clock=clock_before.hex()))
        if bytes(g.mem_read(clock,0x24))!=clock_before:raise ValueError('Original interval changed its caller clock')
    report=dict(scope=__doc__,xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        instruction_patches=[],query_boundary='00211da0',executed_instructions=len(executed),queries=queries,ticks=ticks)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(len(queries),'original camera-time values;',len(ticks),'bounded original director tick banks')

if __name__=='__main__':main()
