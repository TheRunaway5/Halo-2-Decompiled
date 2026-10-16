#!/usr/bin/env python3
"""Unchanged complete TU1.5 shader-function banks, lookup and32-slot replacement.

Object bodies/header lookup, global storage, clock and caller stack are declared
fixture inputs. Whole10FC30/10FB70/10FD50 and all direct callees run without
stubs or patches. This is not original object birth or a rendered shader test.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise ValueError('Preserve original evidence')
    x=Xbe(a.xbe);pin=hashlib.sha256(x.data).hexdigest()
    if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':raise ValueError('Expected English TU1.5')
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(at,n)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
    def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    put(0x51daf4,0x2100000);put(0x51df54,0x2104000);g.mem_write(0x2104002,struct.pack('<h',30))
    put(0x4ecbd0,0x2105000);put(0x2105044,0x2105100);put(0x2105114,0x2106000);put(0x2105120,0x2107000)
    bank,stack,stop=0x2100000,0x21fff00,0x3000000;rows=[];executed=set()
    def hook(vm,pc,n,user):
        if bytes(vm.mem_read(pc,n))!=x.read(pc,n):raise ValueError('Original instruction differs')
        executed.add(pc)
    g.hook_add(UC_HOOK_CODE,hook)
    def reset():
        g.mem_write(bank,bytes(0x300));g.mem_write(bank+0x280,b'\xff'*128)
        g.mem_write(0x2106000,bytes(0x200));g.mem_write(0x2107000,bytes([0x80])*0x200)
    def action(op,tick,object=0xbeef0001,name=0x0c001b89,value=0.,frames=0.):
        put(0x2104008,tick);before=bytes(g.mem_read(bank,0x300));bodies_before=[bytes(g.mem_read(at,0x200)).hex()for at in [0x2106000,0x2107000]]
        for r in [R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI,R.UC_X86_REG_EBP]:g.reg_write(r,0)
        g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        if op=='set':put(stack,stop,object,name,bits(value));g.reg_write(R.UC_X86_REG_XMM4,bits(frames));root=0x10fc30
        elif op=='clear':put(stack,stop);g.reg_write(R.UC_X86_REG_EDI,object);g.reg_write(R.UC_X86_REG_ESI,name);root=0x10fd50
        else:put(stack,stop,name,0x2108000,0x2108004);put(0x2108000,0x12345678,0x11223344);g.reg_write(R.UC_X86_REG_EBX,object);root=0x10fb70
        g.reg_write(R.UC_X86_REG_ESP,stack);g.emu_start(root,stop,count=100000)
        if g.reg_read(R.UC_X86_REG_EIP)!=stop:raise ValueError('Original function owner incomplete')
        row=dict(op=op,tick=tick,object=object,name=name,value=bits(value),frames=bits(frames),before=before.hex(),after=bytes(g.mem_read(bank,0x300)).hex(),bodies_before=bodies_before,bodies_after=[bytes(g.mem_read(at,0x200)).hex()for at in [0x2106000,0x2107000]])
        if op=='get':row.update(found=g.reg_read(R.UC_X86_REG_EAX)&255,value_out=word(0x2108000),slot_out=word(0x2108004))
        rows.append(row)
    for duration in [0.,1.,1.5,29.999998,30.,30.000002,101.,1024.,-1.]:
        reset();action('set',0);action('set',10,value=1.,frames=duration)
        for tick in [5,10,11,12,15,20,25,40,41,1034,2048]:action('get',tick)
        action('clear',2048);action('get',2048)
    for tied in [False,True]:
        reset()
        for n in range(40):action('set',0 if tied else n,object=0xbeef0001 if n%2 else 0xcafe0002,name=0x04001000+n,value=n/40,frames=30.)
        for n in range(40):action('get',41,object=0xbeef0001 if n%2 else 0xcafe0002,name=0x04001000+n)
        for n in range(40):action('clear',41,object=0xbeef0001 if n%2 else 0xcafe0002,name=0x04001000+n)
    for object,name in [(0xffffffff,0x04001000),(0xbeef0001,0),(0xbeef0001,0xffffffff)]:
        action('set',50,object,name,1.,10.);action('clear',50,object,name)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[],executed_instructions=len(executed),cases=rows),indent=2)+'\n');print(len(rows),'complete original shader-function bank/body comparisons')
if __name__=='__main__':main()
