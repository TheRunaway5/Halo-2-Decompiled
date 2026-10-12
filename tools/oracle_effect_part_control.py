#!/usr/bin/env python3
"""Unchanged17A7E4..17A818 effect-part A/B control production.

Explicit part flags and retained effect A/B values vary; original flag tests,
constant load, scalar multiplication and output store execute unchanged. This
prefix does not establish the caller's admission, location or A/B ownership.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    s=Snapshot(args.snapshot);digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    rng=random.Random(0x17a7e4);cases=2048;fixture=bytearray(struct.pack('<II',0x31504345,cases))
    for n in range(cases):
        g=s.guest();g.mem_map(0x70000000,0x10000)
        flags_a=(rng.getrandbits(32)&~0x20)|(0x20 if n&1 else 0)
        flags_b=(rng.getrandbits(32)&~0x20)|(0x20 if n&2 else 0)
        a,b=(rng.uniform(-8,8) for _ in range(2))
        if n%16<4:a,b=((0.,0.),(.5,2.),(1.,1.),(-0.,-0.))[n%16]
        packet=struct.pack('<IIff',flags_a,flags_b,a,b)
        flags_a,flags_b,a,b=struct.unpack('<IIff',packet)
        g.mem_write(0x70001030,struct.pack('<I',flags_a));g.mem_write(0x70001034,struct.pack('<I',flags_b))
        g.mem_write(0x7000206c,struct.pack('<ff',a,b))
        for reg,value in ((R.UC_X86_REG_ESP,0x70003000),(R.UC_X86_REG_EBP,0x70001000),(R.UC_X86_REG_EDI,0x70002000),(R.UC_X86_REG_MXCSR,0x1fa0)):
            g.reg_write(reg,value)
        g.emu_start(0x17a7e4,0x17a818,count=64);assert g.reg_read(R.UC_X86_REG_EIP)==0x17a818
        fixture+=packet+bytes(g.mem_read(0x70003014,4))
    (args.output/'fixture.bin').write_bytes(fixture)
    report=dict(boundary=__doc__,cases=cases,output_words=cases,snapshot_sha256=digest,
        intercepted_instructions=0,fixture_sha256=hashlib.sha256(fixture).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
