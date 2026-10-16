#!/usr/bin/env python3
"""Unchanged TU1.5 complete timed-screen banks across command/update/ack/stop.

Whole HS start/set/set2 wrappers use declared native parsed-argument and
published-result boundaries. Whole1FD00/1FCE0/1FCB0 run without callees or
instruction patches. Clock, bank backing and target-identity/flag bytes are
explicit caller inputs; no GPU, whole HS-thread or render-frame claim is made.
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
    bank,packet,stack,stop=0x2100000,0x2104000,0x21fff00,0x3000000
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*(w&0xffffffff for w in words)))
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    executed=set();boundaries=[]
    def hook(vm,pc,n,user):
        if bytes(vm.mem_read(pc,n))!=x.read(pc,n):raise ValueError('Original instruction differs')
        executed.add(pc)
        if pc in (0x212010,0x211da0):
            sp=vm.reg_read(R.UC_X86_REG_ESP);boundaries.append(pc)
            vm.reg_write(R.UC_X86_REG_EIP,word(sp));vm.reg_write(R.UC_X86_REG_ESP,sp+(20 if pc==0x212010 else 4))
            if pc==0x212010:vm.reg_write(R.UC_X86_REG_EAX,packet)
    g.hook_add(UC_HOOK_CODE,hook);put(0x515ec0,bank);rows=[]
    def action(op,clock=0.,value=0.,strength=1.,reset=False):
        clock_bytes=struct.pack('<d',clock);g.mem_write(0x4920c0,clock_bytes);g.mem_write(0x4c6860,clock_bytes)
        before=bytes(g.mem_read(bank,0x3fc));target_before=bytes(g.mem_read(0x4c1eb0,16))
        if op=='start':put(packet,int(reset));root=0x2b4ef0
        elif op in ('set','set2'):g.mem_write(packet,struct.pack('<ff',value,strength));root=0x2b4fc0 if op=='set'else 0x2b5040
        else:root={'update':0x1fd00,'ack':0x1fce0,'stop':0x1fcb0}[op]
        put(stack,stop,0x2c1 if op=='start'else 0x2c3 if op=='set'else 0x2c4,0,0)
        for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_EBX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI,R.UC_X86_REG_EBP]:g.reg_write(reg,0)
        g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        boundaries.clear();g.emu_start(root,stop,count=100000)
        if g.reg_read(R.UC_X86_REG_EIP)!=stop:raise ValueError('Original screen operation incomplete')
        if boundaries!=([0x212010,0x211da0]if op in ('start','set','set2')else []):raise ValueError('Unexpected original boundaries')
        rows.append(dict(op=op,clock_hex=clock_bytes.hex(),value_hex=struct.pack('<f',value).hex(),strength_hex=struct.pack('<f',strength).hex(),reset=reset,
            before=before.hex(),after=bytes(g.mem_read(bank,0x3fc)).hex(),target_before=target_before.hex(),target_after=bytes(g.mem_read(0x4c1eb0,16)).hex()))
    for epoch in (0.,.1,4096.0003,16777216.25):
        for duration in (-1.,0.,1/30,.5,1.,3.):
            for set_op,strength in [('set',1.),('set2',2.)]:
                g.mem_write(bank,bytes(0x3fc));g.mem_write(0x4c1eb0,bytes(16));put(0x4c1eb0,0x2106000)
                action('stop');action(set_op,epoch,duration,strength);action('update',epoch);action('start',epoch)
                for delta in (0.,1/120,1/30,.25,.5,1.,3.,5.):
                    action('update',epoch+delta)
                    if delta==1/30:action('ack',epoch+delta)
                action(set_op,epoch+6,1.,strength);action('update',epoch+6.125);action('start',epoch+6.125,reset=True);action('update',epoch+6.25);action('stop')
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[],executed_instructions=len(executed),cases=rows),indent=2)+'\n')
    print(len(rows),'complete original screen-bank comparisons')
if __name__=='__main__':main()
