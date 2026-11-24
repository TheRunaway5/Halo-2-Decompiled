// Native TU1.5 player biped query (152E00) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E65A0 is a hash-driven stub of its complete entry ABI (EAX the unit): it
// answers AL zero or not, writes scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX or ESI change or
// the stack is left unbalanced. Fixtures author players without units and
// with units of every type (types past 31 too), parented or not, in state 1
// or another; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_player_biped.hpp"
using namespace frame_misc_harness;
namespace biped=halo2::native_player_biped_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,player_data=0x10010000u,headers=0x10100000u,header_data=0x10110000u,
    objects=0x10200000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[6]=std::uint32_t(rng());
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
static void run_e65a0(WorldState &s){stub(s,0xe65a0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_data);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        for(std::uint32_t at=player_data+0x21cu*k;at<player_data+0x21cu*(k+1);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(player_data+0x21cu*k+0x2cu,pick(5)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        const auto object=objects+0x1000u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));
        static constexpr std::uint32_t types[]{0u,1u,2u,3u,7u,9u,32u,0xffu};
        m.put(header_data+12u*k+3u,pick(2)?0u:types[pick(8)],1);
        m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,pick(4)==0?std::uint32_t(rng()):0xffffffffu);
        m.put(object+0x3dcu,pick(4)==0?pick(8):1u,1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,player_data,headers,header_data,objects+0x2000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return biped::bridge(s,[](auto &v){original::native_frame_misc_reference_152e00(v);},[](auto &v){original::run_e65a0(v);});}
void reference(WorldState &s){original::run_152e00(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_BIPED");
    std::mt19937 rng(0x152e00u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,biped::coverage().load(),unsigned(biped::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
