#!/usr/bin/env python3
"""Run unchanged TU1.5 17C710 through every request and its actual callees.

The single WORLD location, effect/event/part rows, controls and request-pool
state are declared caller fixtures. Lens definitions and transition tables
are authenticated original data. No source instruction or callee is replaced.
This qualifies request arithmetic, not allocation, network or gameplay.
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
    path=Path(__file__).resolve().parents[1]/'assets/disc/maps/coagulation.map';cache=Cache(path);lenses={}
    try:
        for tag in cache.tags.values():
            if tag['group']!='lens' or not tag['address']:continue
            _,at=cache.tag(tag['datum'],'lens');curve=cache.u16(at+0x3c)
            assert curve<16
            lenses.setdefault(curve,(tag,bytes(cache.data[at:at+tag['size']])))
    finally:cache.close()
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    initial=bytes(cpu.mem_read(0x10000,0x800000));cpu.mem_map(0x2400000,0x300000);cpu.mem_map(0x2800000,0x1000)
    table,root,lens_root,event,part,body,pool,locations,stack=0x2400000,0x2510000,0x2510800,0x2511000,0x2512000,0x2513000,0x2514000,0x2515000,0x2604000
    definition,instance=0xa100fffe,0xe672000d
    def word(at,v):cpu.mem_write(at,struct.pack('<I',v&0xffffffff))
    requests=[];trace=[];sites=set()
    def code(c,pc,n,_):
        assert bytes(c.mem_read(pc,n))==image.read(pc,n)
        trace.append(pc);sites.add(pc)
        if pc==0x2db80:
            sp=c.reg_read(R.UC_X86_REG_ESP);args=struct.unpack('<9I',c.mem_read(sp+4,36))
            requests.append(dict(category=args[1],index=args[2],ordinal=args[3],opacity_bits=args[7],
                color=bytes(c.mem_read(args[5],12)).hex(),position=bytes(c.mem_read(args[4],12)).hex()))
    cpu.hook_add(UC_HOOK_CODE,code);rows=[]
    registers=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    for curve,(tag,metadata) in sorted(lenses.items()):
      for target in (0.,.1,.5):
       for elapsed in (-.025,0.,.025,1/30,.066,.1,.2):
        for a_flags,b_flags in ((0,0),(32,0),(0,32),(32,32)):
            cpu.mem_write(0x10000,initial);cpu.mem_write(0x2510000,b'\0'*0x6000)
            word(0x4f0414,table);cpu.mem_write(table+(definition&65535)*16,struct.pack('<4I',0x65666665,definition,root,0x1c))
            cpu.mem_write(table+(tag['datum']&65535)*16,struct.pack('<4I',0x6c656e73,tag['datum'],lens_root,len(metadata)));cpu.mem_write(lens_root,metadata)
            word(root+0xc,1);word(root+0x14,1);word(root+0x18,event);word(event+0x18,1);word(event+0x1c,part)
            word(part+8,0x6c656e73);word(part+12,0x6c656e73);word(part+16,tag['datum']);word(part+0x30,a_flags);word(part+0x34,b_flags)
            word(body+4,definition);cpu.mem_write(body+0x24,struct.pack('<3f',1.,.5,.25));word(body+0x48,0xffffffff);word(body+0x58,0xffffffff)
            cpu.mem_write(body+0x60,struct.pack('<4f',elapsed,target,0.,.25));cpu.mem_write(body+0x70,struct.pack('<f',.75));word(body+0x7c,0xa2000000)
            word(0x4f7208,pool);word(pool+0x20,512);word(pool+0x24,60);word(pool+0x44,locations)
            cpu.mem_write(locations,struct.pack('<HHI13f',0xa200,0xffff,0xffffffff,1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,3.,0.,0.))
            word(0x4c66f4,0);word(0x4c66f8,0);cpu.mem_write(0x4c65c0,struct.pack('<6f',0.,0.,0.,1.,0.,0.))
            ring=bytearray(0x6000);ring[0xfd8]=1;cpu.mem_write(0x52d000,bytes(ring))
            cpu.mem_write(stack,struct.pack('<3I',0x2800000,body,instance))
            for r in registers:cpu.reg_write(r,0)
            cpu.reg_write(R.UC_X86_REG_ESP,stack)
            for n in range(8):cpu.reg_write(R.UC_X86_REG_XMM0+n,0)
            for reg,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:cpu.reg_write(reg,value)
            requests.clear();trace.clear();cpu.emu_start(0x17c710,0x2800000,count=100000)
            assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2800000 and cpu.reg_read(R.UC_X86_REG_ESP)==stack+12
            assert len(requests)==1,(curve,target,elapsed,requests)
            assert requests[0]['category']==1 and requests[0]['index']==(instance&65535) and requests[0]['ordinal']==0
            rows.append(dict(curve=curve,lens=tag['name'],scale_a=a_flags,scale_b=b_flags,controls=[.25,.75],
                elapsed_bits=struct.unpack('<I',struct.pack('<f',elapsed))[0],target_bits=struct.unpack('<I',struct.pack('<f',target))[0],
                expected=requests[0],trace_sha256=hashlib.sha256(b''.join(struct.pack('<I',pc) for pc in trace)).hexdigest()))
    fixture=dict(scope=__doc__,original_sha256=NEW_PIN,original_map_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        transitions=[image.read(0x4782a8+n*1024,1024).hex() for n in range(16)],cases=rows)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    report=dict(passed=True,cases=len(rows),curves=sorted(lenses),original_sha256=NEW_PIN,instruction_patches=[],replaced_services=[],
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),fixture_bytes=a.fixture.stat().st_size,instruction_sites=len(sites),scope=__doc__)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
