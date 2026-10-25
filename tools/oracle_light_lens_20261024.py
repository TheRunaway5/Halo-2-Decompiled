#!/usr/bin/env python3
"""Execute unchanged TU1.5 C5370/31500 and2DB80 for declared standalone light callers.

Light records, typed tag directory and request ring are explicit caller inputs.
Lens metadata and every executed instruction are original; no service is replaced.
This qualifies the request boundary, not live allocation, PVS or pixels.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    path=Path(__file__).resolve().parents[1]/'assets/disc/maps/coagulation.map';cache=Cache(path)
    try:
        tag=next(t for t in cache.tags.values() if t['group']=='lens' and t['address'])
        _,at=cache.tag(tag['datum'],'lens');metadata=bytes(cache.data[at:at+tag['size']])
    finally:cache.close()
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    initial=bytes(cpu.mem_read(0x10000,0x800000));cpu.mem_map(0x2400000,0x300000);cpu.mem_map(0x2800000,0x1000)
    table,root,lens_root,body,pool,stack=0x2400000,0x2510000,0x2510800,0x2513000,0x2514000,0x2604000
    definition,identity=0xa100fffe,0xe9810000
    def word(at,v):cpu.mem_write(at,struct.pack('<I',v&0xffffffff))
    requests=[];sites=set();trace=[]
    def code(c,pc,n,_):
        assert bytes(c.mem_read(pc,n))==image.read(pc,n);sites.add(pc);trace.append(pc)
        if pc==0x2db80:
            sp=c.reg_read(R.UC_X86_REG_ESP);args=struct.unpack('<9I',c.mem_read(sp+4,36))
            requests.append(dict(priority=args[0],category=args[1],index=args[2],ordinal=args[3],
                position=bytes(c.mem_read(args[4],12)).hex(),forward=bytes(c.mem_read(c.reg_read(R.UC_X86_REG_EDI),12)).hex(),
                color=bytes(c.mem_read(args[5],12)).hex(),brightness_bits=args[6],opacity_bits=args[7],scale_bits=args[8],lens=c.reg_read(R.UC_X86_REG_ECX)))
    cpu.hook_add(UC_HOOK_CODE,code);rows=[]
    registers=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    for flags in (0,0x20,0x40,0x60,0x40000):
      for active in (0,2,3):
       for fade in (-.25,0.,.125,1.,1.25):
        for opacity in (0.,.25,1.):
            cpu.mem_write(0x10000,initial);cpu.mem_write(0x2510000,b'\0'*0x6000)
            word(0x4f0414,table);cpu.mem_write(table+(definition&65535)*16,struct.pack('<4I',0x6c696768,definition,root,228))
            cpu.mem_write(table+(tag['datum']&65535)*16,struct.pack('<4I',0x6c656e73,tag['datum'],lens_root,len(metadata)));cpu.mem_write(lens_root,metadata)
            word(root,flags);word(root+0x94,tag['datum']);word(root+0xa0,0xffffffff)
            word(0x4ecbdc,pool);word(pool+0x44,body);word(body,0xe981|(active<<16));word(body+4,definition)
            word(body+0x4c,0xffffffff);word(body+0x54,0xffff)
            cpu.mem_write(body+0x84,struct.pack('<3f',3.,.25,.5));cpu.mem_write(body+0xac,struct.pack('<3f',1.,0.,0.))
            cpu.mem_write(body+0xc8,struct.pack('<f',fade));cpu.mem_write(body+0xd0,struct.pack('<f',opacity));cpu.mem_write(body+0xd4,struct.pack('<3f',.25,.5,1.5))
            word(0x4c66f4,0);word(0x4c66f8,0);cpu.mem_write(0x4c65c0,struct.pack('<6f',0.,0.,0.,1.,0.,0.))
            ring=bytearray(0x6000);ring[0xfd8]=1;cpu.mem_write(0x52d000,bytes(ring))
            cpu.mem_write(stack,struct.pack('<3I',0x2800000,identity,0xffff))
            for reg in registers:cpu.reg_write(reg,0)
            cpu.reg_write(R.UC_X86_REG_ESP,stack)
            for n in range(8):cpu.reg_write(R.UC_X86_REG_XMM0+n,0)
            for reg,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(reg,value)
            requests.clear();trace.clear();cpu.emu_start(0xc5370,0x2800000,count=100000)
            assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2800000 and cpu.reg_read(R.UC_X86_REG_ESP)==stack+12
            assert len(requests)==int(bool(active&2) and not(flags&0x20))
            rows.append(dict(flags=flags,active=active,lens=tag['datum'],identity=identity,fade_bits=struct.unpack('<I',struct.pack('<f',fade))[0],opacity_bits=struct.unpack('<I',struct.pack('<f',opacity))[0],requests=list(requests),trace_sha256=hashlib.sha256(b''.join(struct.pack('<I',pc) for pc in trace)).hexdigest()))
    fixture=dict(scope=__doc__,original_sha256=NEW_PIN,cases=rows)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    report=dict(passed=True,cases=len(rows),requests=sum(len(row['requests']) for row in rows),original_sha256=NEW_PIN,instruction_patches=[],replaced_services=[],fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),fixture_bytes=a.fixture.stat().st_size,instruction_sites=len(sites),scope=__doc__)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
