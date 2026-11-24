// Native TU1.5 player directors (157FE0) against the unmodified translated
// body of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 158790, 1588B0 (the unit's dispatch route), the directors'
// callbacks (through the route at the address a director names: 245400,
// 16F6A0 or others) and 171FF0 are genuine stand-ins whose effects are a
// hash of their complete entry ABI, their stack and the return address:
// the callbacks pop their three arguments and fill the state (its active
// bit and longest timer); all of them return EAX/ECX/EDX, flags and XMM,
// write scratch, and occasionally clobber callee-saved registers (EBP
// among them) or leave the stack unbalanced. Fixtures author the four
// directors (timers zero, small, large, negative, special floats and NaN
// payloads; callbacks none, default, first-player or other), the local
// player handles, the director mode (in and out of the table, and table
// entries the original does not name), the mode change byte and the
// constants; one region removed injects a fault; a set direction flag is
// refused. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_player_directors.hpp"
using namespace frame_misc_harness;
namespace directors=halo2::native_player_directors_detail;
namespace {
constexpr std::uint32_t players_block=0x10000000u,scratch=0x10600000u,none=0xffffffffu;
std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    return values[rng()%8];
}
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(arguments==3u) {
        // A callback: (target, request, state) fills the state.
        const auto state=r(s.memory,s.gp[4]+12u);
        w(s.memory,s.observe,target+2u,state,1,rng()%4?1u|std::uint32_t(rng()%256):std::uint32_t(rng()%128)*2u);
        const auto longest=rng()%6==0?special_float(rng):std::bit_cast<std::uint32_t>(float(rng()%9)*0.25f);
        w(s.memory,s.observe,target+3u,state+0x88u,4,longest);
    }
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==5?s.gp[5]+4u*std::uint32_t(rng()%3):which==6?std::uint32_t(rng()%5):
            which==3?0x4f556au+0x140u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's mode table targets, as its canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x15807du:case 0x158089u:case 0x1580d0u:case 0x1580dfu:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    stub(s,target,target==0x1588b0u?0u:3u);
}
static void run_158790(WorldState &s){stub(s,0x158790u,0);}
static void run_171ff0(WorldState &s){stub(s,0x171ff0u,0);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    std::mt19937_64 wide(rng());
    m.put(0x4f54f0u,players_block);
    for(std::uint32_t at=players_block;at<players_block+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)
        m.put(players_block+0xcu+4u*k,(shape==1?false:pick(4)!=0)?(std::uint32_t(rng()%0x7fff)<<16)|pick(16):none);
    for(std::uint32_t at=0x4f5500u;at<0x4f5a80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f550cu,pick(7)==0?5u+pick(3):pick(5));
    m.put(0x4f5510u,(std::uint32_t(rng())&0xffffff00u)|(pick(3)?pick(2):pick(256)));
    static constexpr std::uint32_t cases[]{0x15807du,0x158089u,0x1580d0u,0x1580dfu};
    for(std::uint32_t k=0;k<5;++k)m.put(0x158220u+4u*k,shape==13 && pick(4)==0?0x158000u+pick(0x100):cases[pick(4)]);
    for(std::uint32_t k=0;k<4;++k) {
        const auto director=0x4f5518u+0x140u*k;
        static constexpr std::uint32_t callbacks[]{0u,0x245400u,0x16f6a0u,0x2a0000u};
        m.put(director+8u,callbacks[pick(4)]);
        std::uint32_t timer=pick(4)==0?0u:std::bit_cast<std::uint32_t>(float(int(pick(17))-4)*0.125f);
        if(shape==4 && pick(3)==0)timer=special_float(wide);
        m.put(director,timer);
    }
    m.put(0x46a0d0u,std::bit_cast<std::uint32_t>(1.0f));
    m.put(0x46a16cu,pick(4)?std::bit_cast<std::uint32_t>(0.25f):special_float(wide));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    std::uint32_t seconds=std::bit_cast<std::uint32_t>(float(pick(9))*0.0625f);
    if(shape==3 && pick(2)==0)seconds=special_float(wide);
    m.put(stack+4u,seconds);
    random_abi(rng,c,stack);
    if(shape!=14)c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players_block,0x4f5000u,0x158000u,0x46a000u,scratch};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return directors::bridge(s,[](auto &v){original::native_frame_misc_reference_157fe0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_158790(v);},[](auto &v){original::run_171ff0(v);});
}
void reference(WorldState &s){original::run_157fe0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_DIRECTORS");
    std::mt19937 rng(0x157fe0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,directors::coverage().load(),unsigned(directors::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
