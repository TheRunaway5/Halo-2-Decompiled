#!/usr/bin/env python3
"""Private full136230 render-object submission loop proof against the physical original.

Unchanged TU1.5 instructions of 136230 run single-stepped on physical i386.
Each run stops before 136D50 (PRT partition) and 1362A0 (object draw submission);
deterministic stubs supply their effects, including nonzero returns that trigger
a view-kind-1 retry, local-slot writes, queue/count mutations and callee-saved
register changes. See tools/native_shadow_frame_oracle.py for the comparison.
"""
import random,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x61000000,0x10000),(0x62000000,0x4000))
VIEWS=0x61000000;QUEUES=0x61004000;STACK=0x62002000

def partition_stub(ctx):
    """136D50(view): may switch queue or adjust count."""
    view=ctx.get(ctx.entry_gp[4]+4)
    if ctx.mode==1:ctx.put(view+0x14,QUEUES+0x200)
    if ctx.mode==2:ctx.gp[5]=(ctx.gp[5]+4)&0xffffffff          # EBP: parameter changes
    if ctx.mode==3:ctx.gp[6]=(ctx.gp[6]+0x20)&0xffffffff       # ESI: view changes

def submit_stub(ctx):
    """1362A0(index, retry_word, &local_slot, -1, param) -> AL."""
    sp=ctx.entry_gp[4]
    index=ctx.get(sp+4)
    retry_word=ctx.get(sp+8)
    local_slot=ctx.get(sp+0xc)
    ctx.put(local_slot,(0x12340000+ctx.call*17+index)&0xffffffff)
    # Return nonzero on first attempt of certain entries (when retry byte is 0)
    # so view_kind==1 retries once, then succeeds on retry.
    retry_byte=retry_word&0xff
    want_retry=(ctx.row['retry_mask']>>(index&7))&1
    bl=ctx.gp[3]&0xff
    if ctx.mode==4 and ctx.call==1:
        ctx.gp[3]=(ctx.gp[3]&~0xff)|2                          # BL changes to 2
        bl=2
    al=(bl^1) if (want_retry and retry_byte==0) else bl
    ctx.gp[0]=(ctx.gp[0]&~0xff)|al

def build(image,rng,family,v):
    banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked object pass fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked object pass input '+hex(at))
    count=(0,1,2,3,4,5,0xffff,0x8000)[v] if family==0 else (1+(v+family)%5)
    kind=1 if family in(1,3,5,7) else (0,2,3,0xffffffff,1,0,2,1)[v]
    retry_mask=0 if family==2 else (0xff if family==3 else (1<<(v%4))|(1<<((v+2)%5)))
    # Author two views (for mode 3 ESI shift by +0x20) and two queues (mode 1).
    view=VIEWS+v*0x80
    for dv in(0,0x20):
        put(view+dv+0x14,QUEUES)
        put(view+dv+0x2a78,kind)
    for q,cnt in((QUEUES,count),(QUEUES+0x200,max(1,count&3))):
        put(q,32);put(q+4,cnt,2)
    sp=STACK+4*(v%4)
    put(sp,0x63000000);put(sp+4,0xdead0000|((family<<8)|v))
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[0]=view
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x136230,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=v%5,retry_mask=retry_mask,domain=family if family in(0,2)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0x136230);return[build(image,rng,family,v)for family in range(8)for v in range(8)]

LOGIC='src/rendering/native_tu15_object_pass_systems.hpp';ADAPTER='src/rendering/native_tu15_object_pass.hpp'
HELPERS='''static void run_136d50(WorldState&s){s.required(0x13623cu,0x136d50u,false);}
static void run_1362a0(WorldState&s){s.required(0x136264u,0x1362a0u,false);}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}
'''
SPEC=Spec(name='object_pass',label='136230 object pass',entry=0x136230,binding='native_object_pass_replacements',replace='replace_object_pass',
    body_pin='33657e6421c332d59eda9c848e8fb8a195c09693bd5e4ac97da53fcfc7422ffd',owned={0x136230:0x136294},entries=(0x136230,),
    callees={0x136d50:Callee(sites=(0x13623c,),argument_bytes=4,effect=partition_stub),
             0x1362a0:Callee(sites=(0x136264,),argument_bytes=20,effect=submit_stub)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_object_pass_tests.cpp','tools/validate_native_object_pass_tu15.py','tools/native_object_pass_replacements.py'),
    reference_variable='HALO2_REFERENCE_OBJECT_PASS',reference_test_variable='HALO2_OBJECT_PASS_REFERENCE_TEST',census_test_variable='HALO2_OBJECT_PASS_CENSUS_TEST',
    census_range=(0x136230,0x136291),helpers=HELPERS,
    mutants=(
     ('unsigned-initial-count',LOGIC,'if(signed_value(initial_count)<=signed_value(widen16(k.zero)))return frame.finish();','if(initial_count==0u)return frame.finish();'),
     ('skip-retry-check',LOGIC,'if(result!=(k.zero&0xffu)) {','if(false && result!=(k.zero&0xffu)) {'),
     ('wrong-retry-kind',LOGIC,'inline constexpr std::uint32_t retry_view_kind=1u;','inline constexpr std::uint32_t retry_view_kind=2u;'),
     ('no-retry-decrement',LOGIC,'                frame.retry_step(k);\n','                (void)k;\n'),
     ('no-retry-set',LOGIC,'                memory.store8(retry_set,frame.retry_slot(),1u);\n','                memory.store8(retry_set,frame.retry_slot(),0u);\n'),
     ('no-local-clear',LOGIC,'    memory.store32(local_clear,frame.local_slot(),k.zero);\n','    memory.store32(local_clear,frame.local_slot(),1u);\n'),
     ('wrong-loop-bound',LOGIC,'if(signed_value(++k.index)>=signed_value(count))return frame.finish();','if(signed_value(++k.index)>signed_value(count))return frame.finish();'),
     ('kept-view-ignored',ADAPTER,'call_(s_,partition_entry,0x13623cu,before_);\n        k.zero=r(ebx);k.parameter=r(ebp);k.view=r(esi);k.index=r(edi);','call_(s_,partition_entry,0x13623cu,before_);\n        k.zero=r(ebx);k.parameter=r(ebp);k.index=r(edi);'),
     ('kept-zero-ignored',ADAPTER,'k.zero=r(ebx);k.parameter=r(ebp);k.view=r(esi);k.index=r(edi);\n        s_.flags.minus','k.parameter=r(ebp);k.view=r(esi);k.index=r(edi);\n        s_.flags.minus'),
     ('wrong-push-order',ADAPTER,'push(0x136262u,r(edx));\n        push(0x136263u,r(edi));','push(0x136262u,r(edi));\n        push(0x136263u,r(edx));'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x136230u,0x136291u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
