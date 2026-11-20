#!/usr/bin/env python3
"""Private full136D50 PRT object partition proof against the physical original.

Unchanged TU1.5 instructions of 136D50 run single-stepped on physical i386.
Each run stops before the separately owned queue swap 166C30; a
deterministic stub swaps the two entries in all four queue streams (as
166C30 does) and, per mode, also changes a callee-saved register, the queue
count or the view's queue. The original resumes at 136DED. See
tools/native_shadow_frame_oracle.py for the comparison.
"""
import random,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x4ec000,0x1000),(0x4f0000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
VIEWS=0x61000000;QUEUES=0x61001000;TABLE=0x61003000;ROWS=0x61003100;OBJECTS=0x61004000;TAGS=0x61006000;DEFINITIONS=0x61007000;STACK=0x62002000
NONE=0xffffffff
def sext16(value):value&=0xffff;return value-0x10000 if value&0x8000 else value

def swap(ctx):
    """166C30(EAX, ECX; EDX queue): swap two entries of the four streams."""
    g=ctx.entry_gp;a=sext16(g[0]);c=sext16(g[1]);queue=g[2]
    for offset,width in((0xc,4),(8,2),(0x10,4),(0x14,2)):
        base=ctx.get(queue+offset);x=ctx.get(base+a*width,width);y=ctx.get(base+c*width,width)
        ctx.put(base+a*width,y,width);ctx.put(base+c*width,x,width)
    if ctx.call==0:
        sp=g[4]
        if ctx.mode==2:ctx.gp[6]=(ctx.gp[6]+1)&0xffffffff      # ESI: skip an entry
        if ctx.mode==3:ctx.put(queue+4,max(ctx.get(queue+4,2)-1,0),2)
        if ctx.mode==4:ctx.put(ctx.get(sp+0x18)+0x14,QUEUES+0x800)  # the view's other queue
        if ctx.mode==0:ctx.gp[3]=(ctx.gp[3]+1)&0xffffffff      # EBX: the front moves

KINDS=('prt','prt','none','none_handle','null_object','none_definition','null_definition','none_model','null_model','none_mode','negative','prt')
def build(image,rng,family,v):
    banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked PRT partition fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked PRT partition input '+hex(at))
    # The entries: what each one names.
    length=1+(v*5+family)%12
    if family==0:kinds=[rng.choice(('prt','none'))for _ in range(length)]
    elif family==1:kinds=['prt']*length
    elif family==2:kinds=['none']*length
    elif family==3:kinds=[KINDS[(n+v)%len(KINDS)]for n in range(12)]
    elif family==4:kinds=[('none','prt')[(n+v)%2]for n in range(length)]
    elif family==5:kinds=[('prt','none_handle','prt','null_object')[n%4]for n in range(4+v%5)]
    elif family==6:kinds=['none','none']+['prt']*(1+v%4)+['negative','prt']
    else:kinds=[rng.choice(KINDS)for _ in range(length)]
    count=len(kinds)
    if family==7:count=(0,0xffff,0x8000,1,0,0x8001,2,0)[v]
    view=VIEWS+v*0x40;sp=STACK+4*(v%4)
    # The object header array and the tag table.
    put(0x4ecbd0,TABLE);put(TABLE+0x44,ROWS);put(0x4f0414,TAGS)
    handles=[]
    for slot,kind in enumerate(kinds):
        index=1+slot;obj=OBJECTS+index*0x80;handle=((0x2000+index)<<16)|index
        handles.append(NONE if kind=='none_handle'else handle)
        put(ROWS+index*12+8,0 if kind=='null_object'else obj)
        tags=[3*index+1,3*index+2,3*index+3]
        put(obj,NONE if kind=='none_definition'else(0x6f620000|tags[0]))
        for n,tag in enumerate(tags):put(TAGS+tag*16+8,DEFINITIONS+tag*0x80)
        if kind=='null_definition':put(TAGS+tags[0]*16+8,0)
        if kind=='null_model':put(TAGS+tags[1]*16+8,0)
        put(DEFINITIONS+tags[0]*0x80+0x38,NONE if kind=='none_model'else(0x686c0000|tags[1]))
        put(DEFINITIONS+tags[1]*0x80+4,NONE if kind=='none_mode'else(0x6d6f0000|tags[2]))
        prt={'prt':(1,2,7,0x7fffffff)[(slot+v)%4],'negative':(0xffffffff,0x80000000)[slot%2]}.get(kind,0)
        put(DEFINITIONS+tags[2]*0x80+0x74,prt)
    # Two queues (the stub may switch the view to the second one).
    for queue in(QUEUES,QUEUES+0x800):
        put(queue,32);put(queue+4,count,2);put(queue+8,queue+0x40);put(queue+0xc,queue+0x80);put(queue+0x10,queue+0x100);put(queue+0x14,queue+0x180)
        order=handles if queue==QUEUES else list(reversed(handles))
        for slot in range(32):
            put(queue+0x80+slot*4,order[slot]if slot<len(order)else NONE)
            put(queue+0x40+slot*2,0x100+slot,2);put(queue+0x100+slot*4,0x1000+slot);put(queue+0x180+slot*2,slot*3,2)
    put(view+0x14,QUEUES)
    put(sp,0x63000000);put(sp+4,view)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x136d50,gp=gp,eflags=0x202|(0x400 if v%2 else 0)|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=v%5,domain=family if family in(5,7)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0x136d50);return[build(image,rng,family,v)for family in range(8)for v in range(8)]

LOGIC='src/rendering/native_tu15_prt_partition_systems.hpp';ADAPTER='src/rendering/native_tu15_prt_partition.hpp'
SPEC=Spec(name='prt_partition',label='136D50 PRT partition',entry=0x136d50,binding='native_prt_partition_replacements',replace='replace_prt_partition',
    body_pin='efe2aecbca25c51a3f70d5469540d86339c780473f321eb21740e4197a8622c6',owned={0x136d50:0x136e09},entries=(0x136d50,),
    callees={0x166c30:Callee(sites=(0x136de8,),argument_bytes=0,effect=swap)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_prt_partition_tests.cpp','tools/validate_native_prt_partition_tu15.py','tools/native_prt_partition_replacements.py'),
    reference_variable='HALO2_REFERENCE_PRT_PARTITION',reference_test_variable='HALO2_PRT_PARTITION_REFERENCE_TEST',census_test_variable='HALO2_PRT_PARTITION_CENSUS_TEST',
    census_range=(0x136d50,0x136e09),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n',
    mutants=(
     ('no-swap',LOGIC,'if(k.index!=k.front)frame.swap(k,queue);','if(k.index==k.front+0x10000u)frame.swap(k,queue);'),
     ('front-not-advanced',LOGIC,'            ++k.front;\n','            k.front+=0u;\n'),
     ('unsigned-prt-count',LOGIC,'return signed_value(memory.load32(prt_info_count,','return 0u!=(memory.load32(prt_info_count,'),
     ('missing-model-test',LOGIC,'    if(!model_data)return false;\n','    if(!model_data&&false)return false;\n'),
     ('stale-queue',LOGIC,'        queue=memory.load32(next_queue,','        memory.load32(next_queue,'),
     ('software-tag-shift',ADAPTER,'void tag_offset(){const auto row=tag_row(r(eax),s_.flags.bits());r(eax)=row.value;s_.flags.load(row.flags);}','void tag_offset(){r(eax)=s_.flags.logic(r(eax)&0xffffu,4);r(eax)=s_.flags.shift(r(eax),4u,4);}'),
     ('kept-index-ignored',ADAPTER,'k.index=r(esi);k.front=r(ebx);','k.front=r(ebx);'),
     ('kept-front-ignored',ADAPTER,'k.index=r(esi);k.front=r(ebx);','k.index=r(esi);'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x136d50u,0x136e09u)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
