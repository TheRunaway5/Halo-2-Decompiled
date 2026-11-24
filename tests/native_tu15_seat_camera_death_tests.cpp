// Native TU1.5 seat camera death (158790) against the unmodified translated
// body of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 158450, 246CF0 and 158900 are hash-driven stubs of their complete entry
// ABIs and arguments: 158450 rewrites the camera's state, 246CF0 the view,
// 158900 the state; all write scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, ESI or EDI
// change or the stack is left unbalanced. Fixtures author resets, seats of
// NONE and every local seat, players NONE or not with and without a unit
// and flag bit 3, both game modes, scripted camera control on and off, game
// times above, at and below zero, and cameras in and out of the dead state;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_seat_camera_death.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_seat_camera_death_detail;
namespace {
constexpr std::uint32_t seats=0x10000000u,players=0x10010000u,data=0x10020000u,game=0x10030000u,control=0x10040000u,
    times=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto camera=0x4f5514u+0x140u*std::uint32_t(rng()%4);
    if(target!=0x246cf0u && rng()%2)w(s.memory,s.observe,target,camera+0xcu,4,rng()%2?0x247140u:0x245400u);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=0x4f5514u+0x140u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_158450(WorldState &s){stub(s,0x158450u);}
static void run_246cf0(WorldState &s){stub(s,0x246cf0u);}
static void run_158900(WorldState &s){stub(s,0x158900u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=0x4f5300u;at<0x4f5a20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(0x4f5514u+0x140u*k+0xcu,pick(3)==0?0x247140u:pick(2)?0x245400u:std::uint32_t(rng()));
    m.put(0x4f54f0u,seats);m.put(0x4f54f4u,players);m.put(0x4f3218u,game);m.put(0x4f5504u,control+pick(4));m.put(0x51df54u,times);
    m.put(0x46a0d0u,0x3f800000u);m.put(0x46a0e8u,0u);
    for(std::uint32_t at=seats;at<seats+0x20u;at+=4)m.put(at,pick(5)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t at=data;at<data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(data+0x21cu*k+0x2cu,pick(2)?0xffffffffu:std::uint32_t(rng()));m.put(data+0x21cu*k+2u,pick(2)?8u:0u,1);}
    for(std::uint32_t at=game;at<game+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+8u,pick(2)?2u:pick(4));
    for(std::uint32_t at=control;at<control+8u;at+=4)m.put(at,pick(3)?0u:std::uint32_t(rng()));
    static constexpr std::uint32_t floats[]{0x3c888889u,0u,0x80000000u,0xbf800000u,0x7fc00000u,0x00000001u};
    for(std::uint32_t at=times;at<times+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(times+0xcu,pick(3)?floats[0]:floats[pick(6)]);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(c.abi.gp[0]&0xffffff00u)|(pick(5)==0?1u+pick(255):0u);
    c.abi.gp[1]=pick(8)?pick(4):0xffffffffu;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{seats,players,data,game,control,times};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_158790(v);},[](auto &v){original::run_158450(v);},
        [](auto &v){original::run_246cf0(v);},[](auto &v){original::run_158900(v);});
}
void reference(WorldState &s){original::run_158790(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_CAMERA_DEATH");
    std::mt19937 rng(0x158790u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
