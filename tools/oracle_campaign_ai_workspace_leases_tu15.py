#!/usr/bin/env python3
"""Execute original two-slot AI scratch leases and their release lifecycle.

The two12-byte descriptors, four-byte heap-use flag and one-byte startup latch
are authenticated loader-zero storage. Existing1D2D90/1D2E30 produce descriptor
pointers, capacity, occupancy and reset state. Native tests run the same caller
sequence, feeding their own returned leases back to release. A nonnull heap
case stops before the original lock call; no lock service return is supplied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
COLD=((0x50208c,24),(0x52be54,4),(0x52be58,1))
BANKS=(*COLD,(0x486070,4),(0x51df44,4),(0x51df48,1),(BASE,0x10000))
OPERATIONS=(('acquire',0),('acquire',0),('acquire',0),('release',0),('acquire',0),
    ('release_foreign',0),('release',1),('release',3),('acquire',0),('release',4))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    im=Xbe(NEW_DEFAULT);assert hashlib.sha256(im.data).hexdigest()==NEW_PIN
    for address,size in COLD:
        s=next(s for s in im.sections if s.va<=address<address+size<=s.va+s.vsize)
        assert address>=s.va+s.rsize and im.read(address,size)==bytes(size)
    code=_Code(im,'.text');bodies={e:_trace(code,e,{e}) for e in (0x1d2d90,0x1d2e30)}
    allowed=set().union(*(set(b.code) for b in bodies.values()));covered=set();sequences=[]
    for poison,heap in ((0,0),(0xa5,0),(0,BASE+0x6000),(0xa5,BASE+0x6000)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in im.sections:g.mem_write(section.va,im.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([poison])*0x10000);g.mem_map(STOP,4096)
        initial=[]
        for address,size in BANKS:
            raw=bytes([poison])*size
            if address==0x52be58:raw=b'\0'
            g.mem_write(address,raw);initial.append([address,raw.hex()])
        def patch(at,value):g.mem_write(at,struct.pack('<I',value))
        patch(0x486070,heap);patch(0x51df44,BASE+0x1000)
        initial += [[0x486070,struct.pack('<I',heap).hex()],[0x51df44,struct.pack('<I',BASE+0x1000).hex()]]
        cases=[];leases=[]
        for operation,index in (OPERATIONS if heap==0 else OPERATIONS[:1]):
            entry=0x1d2d90 if operation=='acquire' else 0x1d2e30
            initial_gp=[0x11223344,0x22334455,leases[index] if operation=='release' else BASE+0x123,
                0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
            patch(STACK,STOP);xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
            for reg,value in zip(GP,initial_gp):g.reg_write(reg,value)
            for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            trace=[];stores=[];prefix_end=None
            def instruction(_g,pc,size,_u):
                nonlocal prefix_end
                if pc==0x1d2db3:prefix_end=pc;g.emu_stop();return
                assert pc in allowed and bytes(g.mem_read(pc,size))==im.read(pc,size),hex(pc)
                trace.append(pc);covered.add(pc)
            def store(_g,_kind,address,size,value,_u):
                assert any(p<=address<address+size<=p+n for p,n in BANKS),hex(address)
                stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                    (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
            hc=g.hook_add(UC_HOOK_CODE,instruction);hs=g.hook_add(UC_HOOK_MEM_WRITE,store)
            g.emu_start(entry,STOP,count=1000);g.hook_del(hc);g.hook_del(hs)
            assert g.reg_read(R.UC_X86_REG_EIP)==(prefix_end or STOP)
            if operation=='acquire':leases.append(g.reg_read(R.UC_X86_REG_EAX))
            cases.append(dict(operation=operation,lease_index=index,entry=entry,prefix_end=prefix_end,gp=initial_gp,xmm=xmm,
                final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                trace=trace,stores=stores,outputs=[[p,bytes(g.mem_read(p,n)).hex()] for p,n in (*COLD,(0x51df48,1))]))
        sequences.append(dict(poison=poison,heap=heap,initial=initial,cases=cases))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        banks=BANKS,cold=COLD,stack=STACK,return_address=STOP,sequences=sequences)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original workspace lease reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,sequences=len(sequences),cases=sum(len(s['cases']) for s in sequences),
        whole_cases=20,lock_prefix_cases=2,cold_bytes=sum(n for _,n in COLD),
        entry_coverage={f'{p:08x}':dict(observed=len(covered&set(b.code)),static=len(b.code),
            unexecuted=[hex(pc) for pc in sorted(set(b.code)-covered)]) for p,b in bodies.items()},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime_reference_required=False,lock_service_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
