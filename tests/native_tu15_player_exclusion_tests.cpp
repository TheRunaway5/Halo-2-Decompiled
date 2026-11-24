// Native TU1.5 player exclusion (1617A0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1A8E50, 1601B0 and 160360 are hash-driven stubs of their complete entry
// ABIs: 1A8E50 advances the iterator it is given (record, handle, index) and
// answers whether one remains, the others answer AL; each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP or EDI change or the stack is left unbalanced. Fixtures
// author game kind 4 and others, players waiting or not, sessions with and
// without a variant record, slots NONE or not, network modes 3, 4, 5 and
// others and replicated slots; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_player_exclusion.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_exclusion_detail;
namespace {
constexpr std::uint32_t game=0x10000000u,players=0x10010000u,records=0x10020000u,session=0x10030000u,network=0x10040000u,
    state=0x10050000u,table=0x10060000u,scratch=0x10600000u;
std::uint32_t limit=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1a8e50u) {
        const auto iterator=s.gp[6];
        const auto index=r(s.memory,iterator+0xcu)+1u;
        const bool more=index<limit;
        w(s.memory,s.observe,0x1a8e51u,iterator,4,more?records+0x40u*index:0u);
        w(s.memory,s.observe,0x1a8e52u,iterator+8u,4,more?(std::uint32_t(rng()&0x7fffu)<<16)|index:0xffffffffu);
        w(s.memory,s.observe,0x1a8e53u,iterator+0xcu,4,index);
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(more?1u+std::uint32_t(rng()%255):0u);
    } else s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng())&0xffffff01u;
    if(policy&2u && rng()%13==0)s.gp[5]=session+4u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%8);
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
static void run_1a8e50(WorldState &s){stub(s,0x1a8e50u);}
static void run_1601b0(WorldState &s){stub(s,0x1601b0u);}
static void run_160360(WorldState &s){stub(s,0x160360u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f3218u,game);m.put(0x4f54f4u,players);m.put(0x4f63b8u,session);m.put(0x4dc04cu,network);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)?std::uint32_t(rng())|1u:0u);
    for(std::uint32_t at=game;at<game+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0xcu,pick(6)?pick(4):4u,1);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x40u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t k=0;k<8;++k)m.put(session+0x2cu+4u*k,pick(4)?(std::uint32_t(rng())&0xfc00u)|pick(8):0xffffffffu);
    for(std::uint32_t at=network;at<network+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t modes[]{4u,4u,5u,3u,0u,6u};
    m.put(network+4u,state);m.put(network+8u,modes[pick(6)]);
    for(std::uint32_t at=state+0x2090u;at<state+0x20b0u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=state+0x2098u+0x14u;at<state+0x2098u+0x14u+36u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(state+0x20a4u,table);
    for(std::uint32_t at=table;at<table+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{game,players,records,session,network,state,table};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1617a0(v);},[](auto &v){original::run_1a8e50(v);},
        [](auto &v){original::run_1601b0(v);},[](auto &v){original::run_160360(v);});
}
void reference(WorldState &s){original::run_1617a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_EXCLUSION");
    std::mt19937 rng(0x1617a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        limit=std::uint32_t(rng()%7);
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
