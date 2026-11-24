// Native TU1.5 player input (1EEDB0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 1F0F90 is a genuine stand-in whose effects are a hash of its
// complete entry ABI and return address: it returns EAX/ECX/EDX, flags and
// XMM, writes scratch, and occasionally clobbers callee-saved registers or
// leaves the stack unbalanced. Fixtures author players with and without
// local slots and units, units with pending choices or not, inputs choosing
// nothing, the same or another choice; one region removed injects a fault;
// a set direction flag is refused. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_player_input.hpp"
using namespace frame_misc_harness;
namespace input=halo2::native_player_input_detail;
namespace {
constexpr std::uint32_t players_array=0x10000000u,player_data=0x10010000u,locals=0x10100000u,headers=0x10200000u,
    header_data=0x10210000u,units=0x10300000u,inputs=0x10400000u,time_block=0x10500000u,scratch=0x10600000u,
    none=0xffffffffu;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1f0f90u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1f0f91u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1f0f90(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f4u,players_array);m.put(0x52be64u,locals);m.put(0x4ecbd0u,headers);m.put(0x51df54u,time_block);
    for(std::uint32_t at=players_array;at<players_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players_array+0x44u,player_data);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto player=player_data+0x21cu*k;
        for(std::uint32_t at=player;at<player+0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(player+0x28u,(std::uint32_t(rng())<<16)|(pick(4)==0?0xffffu:pick(4)));
        m.put(player+0x2cu,pick(4)==0?none:(std::uint32_t(rng())<<16)|pick(4));
        const auto unit=units+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,unit);
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x1fcu,(std::uint32_t(rng())&0xffff0000u)|(pick(3)?0xffffu:pick(4)));
        m.put(unit+0x214u,(std::uint32_t(rng())&0xff00ffffu)|(pick(4)<<16));
    }
    for(std::uint32_t at=locals;at<locals+0x1b0u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=inputs;at<inputs+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(inputs+0x20u,(std::uint32_t(rng())&0xffffff00u)|(pick(4)==0?0xffu:pick(4)));
    m.put(time_block,(std::uint32_t(30u+pick(40))<<16)|(std::uint32_t(rng())&0xffffu));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));m.put(stack+4u,inputs);
    random_abi(rng,c,stack);
    if(shape!=14)c.abi.eflags&=~0x400u;
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players_array,player_data,locals,headers,header_data,units,inputs,time_block};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s){return input::bridge(s,[](auto &v){original::native_frame_misc_reference_1eedb0(v);},[](auto &v){original::run_1f0f90(v);});}
void reference(WorldState &s){original::run_1eedb0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_INPUT");
    std::mt19937 rng(0x1eedb0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,input::coverage().load(),unsigned(input::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
