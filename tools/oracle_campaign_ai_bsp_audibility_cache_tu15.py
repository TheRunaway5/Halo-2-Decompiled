#!/usr/bin/env python3
"""Execute the original BSP audibility cache and all real pair-query dependencies.

Declared BSP/audibility records and poison-filled prior cache storage are inputs.
The unchanged TU1.5 instructions alone produce masks, keys, registers and writes.
Cache hits deliberately omit a BSP pointer, proving that the early return does
not query or replace the prior mask. No captured runtime banks or supplied results.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

BASE=0x2000000
STACK,BSP,AUD,BITS,PAIRS=(BASE+n for n in (0xf000,0x10000,0x11000,0x20000,0x30000))
STOP=0x3000000
ENTRIES={0x203c00,0x252f30,0x253070,0x2531e0}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert image.read(0x502200,0x42)==bytes(0x42)
    code=_Code(image,'.text');bodies={e:_trace(code,e,ENTRIES) for e in ENTRIES}
    sites=set().union(*(set(body.code) for body in bodies.values()))
    banks=[[BASE,0x60000],[0x502200,64],[0x502240,2],[0x4ecc18,4]]
    specs=[dict(count=n,cluster=q,present=p,profile=v,hit=False)
        for n in (0,1,2,3,31,32,33) for q in sorted({0,max(n-1,0)})
        for p,v in itertools.product((False,True),range(4))]
    specs += [dict(count=n,cluster=n-1,present=p,profile=3,hit=False)
        for n,p in ((127,False),(511,True),(512,False),(512,True))]
    specs += [dict(count=n,cluster=q,present=False,profile=0,hit=True)
        for n,q in ((0,-1),(1,0),(32,31),(33,32),(512,511))]
    cases=[];covered=set();points=set()
    for ordinal,spec in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x60000);g.mem_write(BASE,b'\xa5'*0x60000);g.mem_map(STOP,4096)
        for at,size in banks[1:]:g.mem_write(at,b'\xa5'*size)
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(size*8))-1)).to_bytes(size,'little'))
        n,q=spec['count'],spec['cluster']
        put(STACK,STOP);put(STACK+4,q,2);put(0x502240,q if spec['hit'] else 0xffff,2)
        # Cached early-return inputs deliberately leave4ECC18 poisoned.
        if not spec['hit']:
            put(0x4ecc18,BSP);put(BSP+0x9c,n);put(BSP+0x224,int(spec['present']))
            put(BSP+0x228,AUD);put(BSP+0xe8,PAIRS)
            put(AUD+4,0);raw(AUD+8,struct.pack('<f',32.0))
            put(AUD+0x20,BITS);put(AUD+0x28,PAIRS)
            pair_count=n*(n-1)//2
            # Populate only the queried row's component inputs; expected masks
            # and branch outcomes are never calculated here.
            words={}
            for peer in range(n):
                if peer==q:continue
                low,high=sorted((q,peer));index=(n-1)*low-(low+1)*low//2+high-1
                assert 0<=index<pair_count
                v=spec['profile']
                distance=(0,255,1,(0,1,2,127,254,255,128)[peer%7])[v]
                audible=(False,True,False,peer%3==1)[v]
                if spec['present']:
                    word=index//32;words.setdefault(word,0xa5a5a5a5)
                    bit=1<<(index%32)
                    words[word]=(words[word]|bit) if audible else (words[word]&~bit)
                else:distance=(distance&0x7f)|(int(audible)<<7)
                put(PAIRS+index,distance,1)
            for word,value in sorted(words.items()):put(BITS+word*4,value)
        initial=[0x12345678,0x87654321,0x11223344,0x55667788,STACK,0x99aabbcc,0x13572468,0x24681357]
        xmm=[[0x3f800000+i,0x40000000+i,0x40400000+i,0x40800000+i] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];pending=[]
        def flush():
            for pc,at,prior in pending:stores.append([pc,at,prior.hex(),bytes(g.mem_read(at,len(prior))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(spec,hex(pc))
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,at,size,_value,_u):
            assert any(p<=at<at+size<=p+s for p,s in banks),(spec,hex(at))
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size))))
        def read(_g,_kind,at,size,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+size))
            elif sec:assert any(p<=at<at+size<=p+s for p,s in banks),('Undeclared field',spec,hex(at))
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        g.emu_start(0x203c00,STOP,count=1000000);flush()
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP,(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        cases.append(dict(spec,entry=0x203c00,poison=0xa5,prefix_end=None,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            outputs=[[at,bytes(g.mem_read(at,size)).hex()] for at,size in ((0x502200,64),(0x502240,2),(STACK-64,72))]))
    fixture=dict(scope=__doc__,executor='bsp_cache',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=banks,immutable_operands=[[at,image.read(at,size).hex()] for at,size in regions(points)],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original BSP-cache reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases),cache_hit_cases=sum(c['hit'] for c in cases),
        entry_coverage={f'{e:08x}':dict(observed=len(covered&set(b.code)),static=len(b.code),unexecuted=sorted(set(b.code)-covered))
            for e,b in sorted(bodies.items())},loader_zero_bytes=64,separate_key_bytes=2,live_cached_results_seeded=False,
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
