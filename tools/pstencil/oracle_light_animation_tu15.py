#!/usr/bin/env python3
"""Execute the unchanged TU1.5 light-animation routine C2B30 over a native fixture.

Each row of the fixture (written by pstencil_light_animation_tu15_tests) holds
one real animated sphere definition's mapping functions, a light identity,
game tick, control, incoming brightness/diffuse and the native kernel's
outputs. This tool builds the original +C0 animation structure, runs C2B30
with its callees 13DE70/13E6E0 unpatched under Unicorn, and requires the
original brightness and diffuse bits to equal the native ones. No instruction
is patched and no service is replaced; x87 uses the 53-bit precision control.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--dumper',type=Path,help='pstencil_light_animation_tu15_tests; writes the fixture first')
    p.add_argument('maps',nargs='*',type=Path,help='prepared caches for --dumper')
    a=p.parse_args()
    if a.dumper:
        import os,subprocess,tempfile
        scratch=Path(os.environ.get('HALO2_TEST_SCRATCH') or tempfile.mkdtemp(prefix='light-animation-'))
        a.fixture=scratch/'light-animation.tsv'
        subprocess.run([str(a.dumper),str(a.fixture),*map(str,a.maps)],check=True,stdout=subprocess.DEVNULL)
    if not a.fixture:p.error('provide --fixture or --dumper with maps')
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN,'TU1.5 image differs from its pin'
    x=Xbe(NEW_DEFAULT);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    g.mem_map(0x70000000,0x20000)
    stack,stop=0x70010000,0x7001ff00
    anim,brow,crow,bfun,cfun,clock,bright,diffuse,gel=(0x70001000,0x70001100,0x70001110,0x70002000,
        0x70003000,0x70004000,0x70005000,0x70005010,0x70005020)
    pack=lambda *v:struct.pack('<'+'I'*len(v),*(n&0xffffffff for n in v))
    visited=set();g.hook_add(UC_HOOK_CODE,lambda _g,at,_s,_d:visited.add(at))
    g.mem_write(0x51df54,pack(clock))
    rows=mismatches=0;names=set();first=None
    for line in a.fixture.read_text().splitlines():
        f=line.split('\t');name,sync,bhex,chex=f[0],int(f[1]),f[2],f[3]
        identity,tick,control,b0,dx,dy,dz,nb,nx,ny,nz=(int(v,16) for v in f[4:15])
        bbytes=b'' if bhex=='-' else bytes.fromhex(bhex);cbytes=b'' if chex=='-' else bytes.fromhex(chex)
        g.mem_write(bfun,bbytes+bytes(64));g.mem_write(cfun,cbytes+bytes(64))
        g.mem_write(brow,pack(len(bbytes),bfun));g.mem_write(crow,pack(len(cbytes),cfun))
        g.mem_write(anim,pack(sync,1 if bbytes else 0,brow,1 if cbytes else 0,crow,0,0))
        # +2 tick rate, +4 seconds per tick, +8 game tick
        g.mem_write(clock,struct.pack('<HhfI',0,30,struct.unpack('<f',struct.pack('<f',1/30))[0],tick))
        g.mem_write(bright,pack(b0));g.mem_write(diffuse,pack(dx,dy,dz));g.mem_write(gel,pack(0,0))
        g.mem_write(stack,pack(stop,identity,control,bright,gel));g.reg_write(R.UC_X86_REG_ESP,stack)
        g.reg_write(R.UC_X86_REG_ECX,anim);g.reg_write(R.UC_X86_REG_EAX,diffuse)
        g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0xc2b30,stop,count=200000)
        assert g.reg_read(R.UC_X86_REG_EIP)==stop,f'C2B30 did not return for {name}'
        ob,=struct.unpack('<I',g.mem_read(bright,4));ox,oy,oz=struct.unpack('<III',g.mem_read(diffuse,12))
        rows+=1;names.add(name)
        if (ob,ox,oy,oz)!=(nb,nx,ny,nz):
            mismatches+=1
            if first is None:first=dict(name=name,identity=identity,tick=tick,control=control,
                original=[ob,ox,oy,oz],native=[nb,nx,ny,nz])
    report=dict(source_sha256=NEW_PIN,rows=rows,definitions=len(names),mismatches=mismatches,first_mismatch=first,
        visited_instructions=len(visited),intercepted_instructions=0,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest())
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=1)+'\n')
    print(f'{rows} rows over {len(names)} definitions; {mismatches} mismatches; {len(visited)} original instructions visited')
    if not rows or mismatches:
        print(json.dumps(first,indent=1));return 1
    return 0
if __name__=='__main__':sys.exit(main())
