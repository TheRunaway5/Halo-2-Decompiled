#!/usr/bin/env python3
"""Unchanged114F70 transition arbitration across activities, flags and FP modes.

Each argument is an explicit public component input. Actual selected-image
constants are untouched. No dependency calls, instruction patches or native
expected outputs supply the original result; this is not a live pose test.
"""
import argparse,itertools,json,hashlib
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from export_mp_ragdoll_owners import sha

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=False)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    sp,stop=0x200f000,0x3000000;g.mem_map(0x2000000,0x10000);g.mem_map(stop,0x1000)
    def put(p,v):g.mem_write(p,v.to_bytes(4,'little'))
    activities=(0xffffffff,0x7000101,0x400000c,0xa000014,0x9000015,0x9000016,0xa000017,0x900001f,0x9000020,0x90006b2,0xd000042,0xc000043)
    minimum=(0,0x80000000,0x3dcccccd,0x3e800000,0xbf800000,0x7f800000,0xff800000,0x7fc00123,0x7f800123)
    rows=[]
    for rounding,previous,activity,changed,flag,word in itertools.product(range(4),activities,activities,(0,1),(0,1),minimum):
        cw,mx=0x27f+(rounding<<10),0x1f80+(rounding<<13)
        put(sp,stop);put(sp+4,flag);put(sp+8,word)
        for reg,value in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_EAX,activity),(R.UC_X86_REG_ECX,previous),(R.UC_X86_REG_EDX,changed),(R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_MXCSR,mx)):
            g.reg_write(reg,value)
        g.emu_start(0x114f70,stop,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
        result=g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff
        rows.append(f'C {previous:08x} {activity:08x} {changed:02x} {flag:02x} {word:08x} {cw:04x} {mx:08x} {result:08x}')
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    constants={f'{p:08x}':bytes(g.mem_read(p,4)).hex()for p in (0x44c6b4,0x465140)}
    (a.output/'report.json').write_text(json.dumps(dict(source_sha256=PIN,tool_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=len(rows),constants=constants,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False),indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
