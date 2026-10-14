#!/usr/bin/env python3
"""Execute the complete original vector-angle routine and its real initializer.

Inputs are finite unit collision normals/directions, including grazing and
response thresholds. No game or CRT service is replaced. Outputs retain the
original binary32 result before FEB40 subtracts its half-pi operand.
"""
import argparse, hashlib, json, math, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--xbe',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();x=Xbe(args.xbe)
    text=next(s for s in x.sections if s.name=='.text');raw=x.section_bytes(text)
    anchor=bytes.fromhex('83ec08f30f107204f30f106a08f30f1021')
    hits=[i for i in range(len(raw)) if raw.startswith(anchor,i)]
    assert len(hits)==1,hits
    entry=text.va+hits[0];cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    ins=list(cs.disasm(x.read(entry,0x130),entry))
    pi_global=next(i.operands[1].mem.disp for i in ins if i.mnemonic=='movss' and i.op_str.startswith('xmm0, dword ptr [0x') and i.address>entry+0xf0)
    # Locate the real static initializer that writes this exact global.
    tail=b'\xf3\x0f\x11\x05'+struct.pack('<I',pi_global)+b'\xc3'
    at=raw.find(tail);assert at>=8 and raw.find(tail,at+1)<0
    init=text.va+at-8;assert raw[at-8:at-4]==bytes.fromhex('f30f1005')
    pi_operand=struct.unpack_from('<I',raw,at-4)[0]
    assert struct.unpack('<I',x.read(pi_operand,4))[0]==0x40490fdb
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    g.mem_map(0x1000000,0x10000);g.mem_map(0x2000000,4096)
    stack,a,b=0x100f000,0x1000000,0x1001000
    def call(address):
        g.mem_write(stack,struct.pack('<I',0x2000000));g.reg_write(UC_X86_REG_ESP,stack)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_FPSW,0);g.reg_write(UC_X86_REG_FPTAG,0xffff);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(address,0x2000000,count=100000)
        assert g.reg_read(UC_X86_REG_EIP)==0x2000000
    call(init);assert bytes(g.mem_read(pi_global,4))==x.read(pi_operand,4)
    rng=random.Random(0x11ce20)
    controls=[((-1.,0.,0.),(math.sin(v),math.cos(v),0.)) for v in (0,1e-6,math.pi/6,math.pi/4,math.pi/2,-math.pi/6)]
    controls += [((0.,0.,1.),(0.,0.,-1.)),((0.,0.,1.),(0.,0.,1.))]
    for _ in range(512):
        pair=[]
        for _ in range(2):
            v=[rng.uniform(-1,1) for _ in range(3)];l=math.sqrt(sum(t*t for t in v));pair.append(tuple(t/l for t in v))
        controls.append(pair)
    lines=[]
    for n,d in controls:
        inputs=struct.pack('<6f',*n,*d);g.mem_write(a,inputs[:12]);g.mem_write(b,inputs[12:]);g.reg_write(UC_X86_REG_ECX,a);g.reg_write(UC_X86_REG_EDX,b)
        call(entry);result=g.reg_read(UC_X86_REG_XMM0)&0xffffffff
        lines.append(' '.join(f'{v:08x}' for v in (*struct.unpack('<6I',inputs),result)))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# Complete original vector-angle; six input words and one output word.\n'+'\n'.join(lines)+'\n')
    args.output.with_suffix('.provenance.json').write_text(json.dumps(dict(image=str(args.xbe),sha256=hashlib.sha256(x.data).hexdigest(),entry=hex(entry),initializer=hex(init),pi_operand=hex(pi_operand),pi_bits='40490fdb',cases=len(lines)),indent=2)+'\n')
    print('PASS original angle cases',len(lines),'entry',hex(entry))
if __name__=='__main__':main()
