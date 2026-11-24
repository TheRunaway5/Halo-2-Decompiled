// Native TU1.5 player timers (154D00) against the unmodified translated body
// of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// D3340 (through the unit's dispatch) is a hash-driven stub of its complete
// entry ABI (XMM0 and XMM1 too): it writes the player's timers and scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author timers negative, zero, one, several and the
// int16 extremes; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_player_timers.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_timers_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,data=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0xd3340u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0xd3340u,data+0x21cu*std::uint32_t(rng()%4)+0x198u+2u*std::uint32_t(rng()%2),2,std::uint32_t(rng()%3));
    w(s.memory,s.observe,0xd3342u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng()%3)-1u;
    if(policy&2u && rng()%9==0)s.gp[5]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%9==0)s.gp[6]=data+0x21cu*std::uint32_t(rng()%4)+0x198u;
    if(policy&2u && rng()%9==0)s.gp[7]=0x21cu*std::uint32_t(rng()%4);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xd3340u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f4u,players);m.put(0x46a0d0u,0x3f800000u);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t at=data;at<data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t timers[]{0u,1u,2u,30u,0x7fffu,0x8000u,0xffffu,0x8001u};
    for(std::uint32_t k=0;k<8;++k)m.put(data+0x21cu*(k/2)+0x198u+2u*(k%2),pick(3)?pick(3):timers[pick(8)],2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,data};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_154d00(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_154d00(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_TIMERS");
    std::mt19937 rng(0x154d00u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
