// Native TU1.5 animation frame clamp (1CE810) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 1E3150 (returns an animation, popping its argument) and
// 1CEC60 are genuine stand-ins whose effects are a hash of their complete
// entry ABI, their stack and the return address: they return EAX/ECX/EDX,
// flags and XMM, write scratch, and occasionally clobber callee-saved
// registers or leave the stack unbalanced. Fixtures author states (NONE
// animations, playing or not, held bits, rates), animations with several
// frame counts, frames below zero, within and past the end, special floats
// and NaN payloads, and the game's seconds per tick; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_clamp.hpp"
using namespace frame_misc_harness;
namespace clamp=halo2::native_animation_clamp_detail;
namespace {
constexpr std::uint32_t states=0x10000000u,instances=0x10100000u,animations=0x10200000u,time_block=0x10300000u,
    scratch=0x10600000u;
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
    s.gp[0]=target==0x1e3150u?animations+0x40u*std::uint32_t(rng()%8):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?states+0x40u*std::uint32_t(rng()%8):std::uint32_t(rng());
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
static void run_1e3150(WorldState &s){stub(s,0x1e3150u,1);}
static void run_1cec60(WorldState &s){stub(s,0x1cec60u,0);}
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
    m.put(0x4f0414u,instances);
    for(std::uint32_t at=instances;at<instances+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k) {
        const auto animation=animations+0x40u*k;
        for(std::uint32_t at=animation;at<animation+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(animation+0x14u,(std::uint32_t(rng())&0xffff0000u)|(pick(6)==0?0u:pick(40)));
        const auto state=states+0x40u*k;
        for(std::uint32_t at=state;at<state+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(state,(std::uint32_t(rng())<<16)|pick(16));
        m.put(state+4u,(pick(5)==0?0xffffu:pick(30))<<16|pick(64));
        m.put(state+0x10u,(std::uint32_t(rng())&0xff0000ffu)|((std::uint32_t(rng())&0xf6u)|(pick(3)?0u:pick(16)))<<8|
                          (pick(3)?1u:0u)<<16);
        m.put(state+0x18u,pick(6)==0?special_float(wide):std::bit_cast<std::uint32_t>(float(pick(9))*0.5f));
    }
    m.put(0x51df54u,time_block);m.put(time_block+4u,std::bit_cast<std::uint32_t>(1.0f/30.0f));
    m.put(0x46a0ecu,std::bit_cast<std::uint32_t>(0.001f));
    m.put(0x46a20cu,pick(6)?std::bit_cast<std::uint32_t>(30.0f):special_float(wide));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    std::uint32_t frame=std::bit_cast<std::uint32_t>(float(int(pick(50))-5)*0.75f);
    if(shape==4 && pick(2)==0)frame=special_float(wide);
    m.put(stack+4u,frame);
    random_abi(rng,c,stack);
    c.abi.gp[0]=states+0x40u*pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{states,instances,animations,time_block,0x46a000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return clamp::bridge(s,[](auto &v){original::native_frame_misc_reference_1ce810(v);},
        [](auto &v){original::run_1e3150(v);},[](auto &v){original::run_1cec60(v);});
}
void reference(WorldState &s){original::run_1ce810(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_CLAMP");
    std::mt19937 rng(0x1ce810u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,clamp::coverage().load(),unsigned(clamp::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
