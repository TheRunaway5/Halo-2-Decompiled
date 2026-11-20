#!/usr/bin/env python3
"""Private full135690 render-view visibility setup proof against the physical original.

Unchanged TU1.5 instructions of 135690 run single-stepped on physical i386.
Each run stops before 11E600 when [ESP+0x1C] is -1; a deterministic stub
supplies its effect. See tools/native_shadow_frame_oracle.py for the
comparison.
"""
import random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from native_shadow_frame_oracle import Spec,Callee,LIVE,main

REGIONS=((0x46a000,0x1000),(0x474000,0x1000),(0x61000000,0x10000),(0x62000000,0x4000))
IMAGE_BANKS=(0x46a000,)
VIEWS=0x61000000;NODES_SRC=0x61004000;NODES_DST=0x61006000;ORIGINS=0x61008000;STACK=0x62002000
NONE=0xffffffff
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
QNAN=0x7fc00011;SNAN=0x7f800019;DENORMAL=0x00000005;NEGZERO=0x80000000

def init_handle_stub(ctx):
    """11E600(ESI=&view->first_handle, EDX=bias_bits)."""
    handle_ptr=ctx.entry_gp[6]
    ctx.put(handle_ptr,(0x77000000|(ctx.entry_gp[2]&0xffff))&0xffffffff)

def build(image,rng,family,v):
    banks=[bytearray(image.read(base,size))if base in IMAGE_BANKS else bytearray(rng.randbytes(size))for base,size in REGIONS]
    def put(at,value,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
        raise ValueError('Unbacked view setup fixture '+hex(at))
    def get(at,width=4):
        for(base,size),bank in zip(REGIONS,banks):
            if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
        raise ValueError('Unbacked view setup input '+hex(at))

    put(0x474c24,0x4321+family*17+v,2)
    view=VIEWS+v*0x100
    src_nodes=NODES_SRC+v*0x200
    dst_buf=NODES_DST+v*0x200
    origin=ORIGINS+v*0x20

    use_plane=1 if (family in(0,2,4,6) or v%2==0) else 0
    if family==1:use_plane=0
    same_buffer=(family==2 and v<4) or (family==5 and v%2==1)
    if same_buffer:src_nodes=dst_buf+4
    put(view+0x00,dst_buf)

    node_count=(0,1,2,3,1,2,0,3)[v]
    if family==3:node_count=0
    elif family==4:node_count=1+(v%2)

    kind=(0,1,2,0,1,0,2,0)[v]
    view_flags=(0x0000,0x0080,0x0100,0x0180,0x0200,0x0000,0x0080,0x0100)[v]
    threshold=bits((0.5,1.0,1.5,2.0,0.25,1.0,0.75,3.0)[v])
    bias=bits((0.125,-0.5,0.0,1.25,-1.0,2.5,0.5,-0.25)[v])
    flag_ptr=(0,0x61009000+v*16,0,0x61009100,0,0,0x61009200,0)[v]

    if family==0:
        # Exercise 11E600 call when threshold slot ([ESP+0x1C] before push -> [ESP+0x18] on entry) is -1 (0xFFFFFFFF = QNaN).
        threshold=NONE if v<4 else bits(1.5)
        flag_ptr=0x61009000 if v%2 else 0
    elif family==1:
        # Exercise status=2 exit (kind==0 and flags low byte & 0x80).
        kind=0 if v<6 else 1
        view_flags=0x80 if v%2==0 else 0x7f
    elif family==2:
        # Exercise status=0 vs status=1 via threshold and bit 8 (0x100).
        kind=1
        node_count=1+(v%2)
        threshold=(bits(0.5),bits(1.0),bits(1.5),QNAN,SNAN,DENORMAL,NEGZERO,bits(0.25))[v]
        view_flags=0x0100 if v in(0,6,7) else 0x0000
    elif family==3:
        # node_count == 0 -> status=1 (unless kind==0 and flags & 0x80).
        kind=1
        node_count=0
    elif family==4:
        # Special floats in plane distance + bias addss.
        use_plane=1
        put(src_nodes+0x80,(bits(2.0),QNAN,DENORMAL,NEGZERO,bits(-1.5),SNAN,bits(0.0),bits(4.0))[v])
        bias=(bits(0.5),bits(1.0),QNAN,NEGZERO,DENORMAL,bits(-2.0),bits(0.0),bits(3.5))[v]

    sp=STACK+4*(v%4)
    put(sp+0x00,0x63000000)         # return address
    put(sp+0x04,view)               # [ESP+0x08] after PUSH EBP
    put(sp+0x08,kind)               # [ESP+0x0C] after PUSH EBP
    put(sp+0x0c,0x1100+family*16+v) # [ESP+0x10] at entry (tag_high word)
    put(sp+0x10,src_nodes)          # [ESP+0x18] after PUSH EBP,ESI,EDI
    put(sp+0x14,threshold)          # [ESP+0x1C] after PUSH EBP,ESI,EDI (note: at entry [ESP+0x1C] is sp+0x1c!)
    put(sp+0x18,origin)             # [ESP+0x20] after PUSH EBP,ESI (2 pushes -> sp+0x18)
    put(sp+0x1c,threshold)          # [ESP+0x1C] at entry AND [ESP+0x24] after 2 pushes
    put(sp+0x20,flag_ptr)           # [ESP+0x20] at entry
    put(sp+0x24,bias)               # [ESP+0x24] at entry AND [ESP+0x2C] after 3 pushes
    # Note on stack offsets as ESP changes across the 3 pushes:
    #   0 pushes (entry):
    #     [ESP+0x10] = sp+0x10 (tag_high)
    #     [ESP+0x1C] = sp+0x1C (threshold)
    #     [ESP+0x24] = sp+0x24 (bias)
    #   1 push (PUSH EBP):
    #     [ESP+0x08] = sp+0x04 (view)
    #     [ESP+0x0C] = sp+0x08 (kind)
    #   2 pushes (PUSH EBP, ESI):
    #     [ESP+0x20] = sp+0x18 (origin)
    #   3 pushes (PUSH EBP, ESI, EDI):
    #     [ESP+0x2C] = sp+0x20 (use_plane byte)
    #     [ESP+0x18] = sp+0x0C (src_nodes)
    #     [ESP+0x1C] = sp+0x10 (sentinel -1 check for 11E600)
    #     [ESP+0x24] = sp+0x18 (EDX arg to 11E600)
    #     [ESP+0x20] = sp+0x14 (flag_ptr stored to +0x2AD0)
    sentinel=NONE if (family==0 and v<4) else (0x1100+family*16+v)
    put(sp+0x04,view)
    put(sp+0x08,kind)
    put(sp+0x0c,src_nodes)
    put(sp+0x10,sentinel)
    put(sp+0x14,flag_ptr)
    put(sp+0x18,origin)
    put(sp+0x1c,threshold)
    put(sp+0x20,(flag_ptr&~0xff)|(use_plane&0xff))
    put(sp+0x24,bias)

    gp=[rng.getrandbits(32)for _ in range(8)]
    gp[4]=sp
    gp[1]=view_flags
    gp[3]=node_count
    depth=(v+family)%4;cw=(0x7f,0x27f,0x37f)[v%3]|((v%4)<<10);status=((v%8)<<11)|0x4500
    return dict(name=f'family{family}-variant{v}',entry=0x135690,gp=gp,eflags=0x202|(rng.getrandbits(12)&0x8d5),matrix=get(sp+4),source=get(sp+8),max_steps=100000,
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[v%4]],live_x87=list(LIVE[:depth]),
        stub=0,domain=family if family in(0,1,2)else'ordinary',banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)])
def corpus(image):
    rng=random.Random(0x135690);return[build(image,rng,family,v)for family in range(6)for v in range(8)]

LOGIC='src/rendering/native_tu15_view_setup_systems.hpp';ADAPTER='src/rendering/native_tu15_view_setup.hpp'
SPEC=Spec(name='view_setup',label='135690 view setup',entry=0x135690,binding='native_view_setup_replacements',replace='replace_view_setup',
    body_pin='237100b008c386cdf99cf2eb224a3751d56076cfbf8f85b076df31a7cae479c1',owned={0x135690:0x13586d},entries=(0x135690,),
    callees={0x11e600:Callee(sites=(0x135784,),argument_bytes=0,effect=init_handle_stub)},regions=REGIONS,
    inputs=(LOGIC,ADAPTER,'tests/native_tu15_view_setup_tests.cpp','tools/validate_native_view_setup_tu15.py','tools/native_view_setup_replacements.py'),
    reference_variable='HALO2_REFERENCE_VIEW_SETUP',reference_test_variable='HALO2_VIEW_SETUP_REFERENCE_TEST',census_test_variable='HALO2_VIEW_SETUP_CENSUS_TEST',
    census_range=(0x135690,0x13586a),
    helpers='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){s.required(site,target,tail);}\n',
    mutants=(
     ('wrong-plane-flag',LOGIC,'inline constexpr std::uint32_t has_clip_plane=0x0200u;','inline constexpr std::uint32_t has_clip_plane=0x0400u;'),
     ('wrong-bit8-flag',LOGIC,'inline constexpr std::uint32_t force_zero_result=0x0100u;','inline constexpr std::uint32_t force_zero_result=0x0080u;'),
     ('wrong-ptr-flag',LOGIC,'inline constexpr std::uint32_t has_flag_ptr=0x0001u;','inline constexpr std::uint32_t has_flag_ptr=0x0002u;'),
     ('skip-plane-bias',LOGIC,'const auto biased_d=frame.add_plane_bias(pd,bias);','const auto biased_d=pd;'),
     ('skip-handle-call',LOGIC,'if(frame.read_arg32(check_flag_sentinel,0x1cu)==none) {','if(false && frame.read_arg32(check_flag_sentinel,0x1cu)==none) {'),
     ('skip-flag-ptr-clear',LOGIC,'if(flag_ptr!=0u) {','if(false && flag_ptr!=0u) {'),
     ('wrong-status-two-sign',LOGIC,'if(low_byte&0x80u) {','if(low_byte&0x40u) {'),
     ('invert-threshold-branch',LOGIC,'if(frame.one_not_above_threshold(one,threshold)) {','if(!frame.one_not_above_threshold(one,threshold)) {'),
     ('ucomiss-instead-of-comiss',ADAPTER,'s_.flags.scalar_compare(s_.xmm[0],s_.xmm[3],false);','s_.flags.scalar_compare(s_.xmm[0],s_.xmm[3],true);'),
     ('software-imul-shr',ADAPTER,'const auto mul=multiply_node_stride(r(ebx),s_.flags.bits());\n        r(edx)=total;r(ecx)=mul.value;s_.flags.load(mul.flags);','r(ecx)=s_.flags.product(r(ebx),0x1bcu);r(edx)=r(ecx);r(ecx)=s_.flags.right(r(ecx),2u,4,false);'),
     ('uncertified-required-owner',ADAPTER,'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(!s.services.required_owner)return false;'),
     ('census-without-switch',ADAPTER,'if(native_census_detail::uncursored(s,0x135690u,0x13586au)){','if(!s.instruction_cursor&&s.services.instruction){'),
    ),guard_mutants=('uncertified-required-owner','census-without-switch'))
if __name__=='__main__':raise SystemExit(main(SPEC,corpus))
