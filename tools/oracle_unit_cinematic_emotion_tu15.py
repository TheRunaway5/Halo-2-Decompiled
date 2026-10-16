#!/usr/bin/env python3
"""Unchanged TU1.5 whole emotional-state wrapper/owners over declared unit records.

Only parsed native arguments212010 and published HS result211DA0 are declared
boundaries. Duration conversion and whole111080/1110D0 execute unchanged. Header,
body/record storage, facial-channel admission and clock are explicit inputs.
This does not establish original birth, graph binding, per-tick facial update,
whole HS-thread/ABI or rendered facial pixels.
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
    registration=struct.unpack('<I',x.read(0x480c00+0xdc*4,4))[0]
    if struct.unpack('<I',x.read(registration+4,4))[0]!=0x2ad950:raise ValueError('Original emotional-state registration differs')
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(at,n)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    body,record,clock,packet,stack,stop=0x2100000,0x2100400,0x2102000,0x2105000,0x21fff00,0x3000000
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
    put(0x4ecbd0,0x2103000);put(0x2103044,0x2103100);put(0x2103114,body);put(0x51df54,clock)
    entries=[];executed=set()
    def hook(vm,pc,n,user):
        if bytes(vm.mem_read(pc,n))!=x.read(pc,n):raise ValueError('Original instruction differs')
        executed.add(pc)
        if pc in (0x212010,0x211da0):
            sp=vm.reg_read(R.UC_X86_REG_ESP);vm.reg_write(R.UC_X86_REG_EIP,word(sp));vm.reg_write(R.UC_X86_REG_ESP,sp+(20 if pc==0x212010 else 4))
            if pc==0x212010:vm.reg_write(R.UC_X86_REG_EAX,packet)
        elif pc==0x111080:entries.append([word(vm.reg_read(R.UC_X86_REG_ESP)+4),word(vm.reg_read(R.UC_X86_REG_ESP)+8)])
    g.hook_add(UC_HOOK_CODE,hook);names=list(struct.unpack('<13I',x.read(0x44a9a4,52)))+[0x07000001,0xdeadbeef];rows=[]
    frames=[-32768,-1,0,1,15,30,90,32767];weights=[0.,.5,1.,2.,-1.]
    for n in range(192):
        object=0xbeef0001 if n%29 else 0xffffffff;name=names[n%len(names)];ticks=frames[(n//len(names))%len(frames)];weight=bits(weights[n%len(weights)])
        if n>=184:weight=[0x7f800000,0xff800000,0x7fc00000][n%3]
        g.mem_write(body,bytes((i*13+7)%256 for i in range(0x600)));g.mem_write(body+0x33e,struct.pack('<h',0x400))
        put(record+0x5c,0xffff0002 if n%11==0 else 0x00010002);g.mem_write(clock,bytes(0x24));g.mem_write(clock+2,struct.pack('<h',[15,30,60][n%3]))
        before=bytes(g.mem_read(body,0x600));clock_bytes=bytes(g.mem_read(clock,0x24));put(packet,object,name,weight,ticks)
        put(stack,stop,0xdc,0,0);g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        entries.clear();g.emu_start(0x2ad950,stop,count=100000)
        if g.reg_read(R.UC_X86_REG_EIP)!=stop or len(entries)!=(object!=0xffffffff):raise ValueError('Original emotional-state wrapper incomplete')
        rows.append(dict(object=object,name=name,weight=weight,frames=ticks,clock=clock_bytes.hex(),before=before.hex(),after=bytes(g.mem_read(body,0x600)).hex(),call_arguments=entries.copy()))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[],executed_instructions=len(executed),cases=rows),indent=2)+'\n');print(len(rows),'complete original emotion wrapper/body comparisons')
if __name__=='__main__':main()
