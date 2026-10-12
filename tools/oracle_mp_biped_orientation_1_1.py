#!/usr/bin/env python3
"""Untouched 1F0940 physical orientation output with declared API records.

These are component inputs: physical packet forward/up and current up, output
storage, flag40, and FP controls. No pose, CRT result, instruction, dependency
return, or native expected value is supplied. The actual x87 CRT and vector
normalization execute. This does not establish a live solved corpse pose.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from export_mp_ragdoll_owners import spans

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    base,stack,sp,stop=0x1000000,0x2000000,0x200f000,0x3000000
    packet,up,forward=base,base+0x1000,base+0x1020
    norm=lambda v:tuple(x/math.sqrt(sum(y*y for y in v)) for x in v)
    pairs=[((0,0,1),(0,0,1)),((0,0,1),(1,0,0)),((0,0,1),(0,1,0)),
        ((0,0,1),(0,0,-1)),((0,0,0),(0,0,1)),((0,0,1),(0,0,0)),
        ((0,0,1),(0,0,1.0001)),((0,0,1),(0,0,.9999999)),
        ((0,0,1),(0,0,-1.0001)),((0,0,1),(.00001,0,1)),
        ((0,0,1),(.1,.2,.97)),((.2,.3,.9),(.8,-.4,.1))]
    for i in range(12):
        pairs.append((norm((math.sin(i+.3),math.cos(i*.71),.1+i*.03)),
            norm((math.sin(i*.93),math.cos(i*.19),-.3+i*.1))))
    rows=[];cases=[]
    for mode in range(4):
        for flags in (0,0x40):
            for index,(current,target) in enumerate(pairs):
                name=f'round{mode}_flag{flags:x}_basis{index}'
                g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
                for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
                g.mem_map(base,0x2000);g.mem_map(stack,0x10000);g.mem_map(stop,0x1000)
                g.mem_write(packet+0x14,bytes([flags]))
                g.mem_write(packet+0xac,struct.pack('<3f',*current))
                g.mem_write(packet+0xf4,struct.pack('<3f',.91,-.37,.13))
                g.mem_write(packet+0x100,struct.pack('<3f',*target))
                g.mem_write(up,bytes([0x71])*12);g.mem_write(forward,bytes([0x93])*12)
                g.mem_write(sp,struct.pack('<I',stop))
                cw,mx=0x27f+(mode<<10),0x1f80+(mode<<13)
                for reg,val in ((R.UC_X86_REG_ESP,sp),(R.UC_X86_REG_EAX,packet),
                    (R.UC_X86_REG_ESI,up),(R.UC_X86_REG_EDI,forward),
                    (R.UC_X86_REG_FPCW,cw),(R.UC_X86_REG_MXCSR,mx),
                    (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
                    g.reg_write(reg,val)
                initial={};current_bytes={};writes=[];instruction=0
                def touch(p,raw):
                    for j,v in enumerate(raw):
                        if p+j not in current_bytes:initial[p+j]=current_bytes[p+j]=v
                        assert current_bytes[p+j]==v,(name,hex(p+j))
                def code(uc,p,size,user):
                    nonlocal instruction
                    instruction=p
                def read(uc,access,p,size,value,user):touch(p,bytes(uc.mem_read(p,size)))
                def write(uc,access,p,size,value,user):
                    before=bytes(uc.mem_read(p,size));touch(p,before)
                    raw=(value&((1<<(size*8))-1)).to_bytes(size,'little')
                    for j,v in enumerate(raw):current_bytes[p+j]=v
                    if not stack<=p<stack+0x10000:writes.append((instruction,p,before,raw))
                for p in (up,forward):touch(p,bytes(g.mem_read(p,12)))
                g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
                g.emu_start(0x1f0940,stop,count=100000)
                assert g.reg_read(R.UC_X86_REG_EIP)==stop,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
                rows.append(f'C {name} {packet:08x} {up:08x} {forward:08x} {sp:08x} {cw:04x} {mx:08x}')
                for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
                for site,p,before,raw in writes:rows.append(f'W {site:08x} {p:08x} {before.hex()} {raw.hex()}')
                for p in (up,forward):rows.append(f'A {p:08x} '+bytes(g.mem_read(p,12)).hex())
                cases.append(dict(name=name,stores=len(writes),up=bytes(g.mem_read(up,12)).hex(),forward=bytes(g.mem_read(forward,12)).hex()))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(tool_sha256=sha(__file__),source_sha256=PIN,fixture_sha256=sha(fixture),
        cases=cases,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],
        native_expected_data_used=False,live_gameplay_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),stores=sum(c['stores']for c in cases),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
