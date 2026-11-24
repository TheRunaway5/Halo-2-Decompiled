// Native TU1.5 player queries against the unmodified translated bodies of the
// physics live support unit (the validator repeats 15B470 with the death
// cycle unit's), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). Shared by one test per owner, which defines OWNER first:
//   C6E50 uncontrolled unit (child C6DD0), 15B470 player live (child 160570).
// The child is a hash-driven stub of its complete entry ABI: it answers AL
// zero or not, writes scratch, the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy BL or ESI change or the stack
// is left unbalanced. Fixtures author every path; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0xc6e50
#include "gameplay/units/native_tu15_uncontrolled_unit.hpp"
namespace query=halo2::native_uncontrolled_unit_detail;
constexpr std::uint32_t child_target=0xc6dd0u;
#define CHILD run_c6dd0
#define VERIFY_SWITCH "HALO2_VERIFY_UNCONTROLLED_UNIT"
#else
#include "gameplay/units/native_tu15_player_live.hpp"
namespace query=halo2::native_player_live_detail;
constexpr std::uint32_t child_target=0x160570u;
#define CHILD run_160570
#define VERIFY_SWITCH "HALO2_VERIFY_PLAYER_LIVE"
#endif
using namespace frame_misc_harness;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,players=0x10180000u,
    player_data=0x10190000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%5==0)s.gp[3]=std::uint32_t(rng());
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
static void CHILD(WorldState &s){stub(s,child_target);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f54f4u,players);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x180u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x13cu,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(object+0x14u,pick(3)==0?std::uint32_t(rng()):0xffffffffu);
        const auto player=player_data+0x21cu*k;
        for(std::uint32_t at=player;at<player+0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(player+2u,(std::uint32_t(rng())&~0x4003u)|(pick(5)?1u:0u)|(pick(5)==0?2u:0u)|(pick(2)?0x4000u:0u),2);
        m.put(player+0xc0u,pick(5)==0?0xffu:pick(255),1);
        m.put(player+0x2cu,pick(2)?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto handle=(std::uint32_t(rng())<<16)|pick(4);
#if OWNER==0xc6e50
    c.abi.gp[1]=handle;
#else
    c.abi.gp[6]=handle;
#endif
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u,players,player_data};
        m.drop(regions[pick(5)]);
    }
    return c;
}
#if OWNER==0xc6e50
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_c6e50(v);},[](auto &v){original::CHILD(v);});}
void reference(WorldState &s){original::run_c6e50(s);}
#else
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_15b470(v);},[](auto &v){original::CHILD(v);});}
void reference(WorldState &s){original::run_15b470(s);}
#endif
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,VERIFY_SWITCH);
    std::mt19937 rng(OWNER+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
