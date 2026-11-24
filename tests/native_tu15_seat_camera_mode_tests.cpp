// Native TU1.5 seat camera mode (158450) against the unmodified translated
// body of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 158340 (through the unit's dispatch) and 158900 (direct) are hash-driven
// stubs of their complete entry ABIs and arguments: 158340 writes the word
// (often the camera's remembered one) and answers AX (1 or not), 158900
// writes scratch; each pops its argument, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, ESI or EDI
// change or the stack is left unbalanced. Fixtures author seats NONE and
// 0..3, players NONE or not, cameras in either state or another, and the
// forced byte set or clear; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_seat_camera_mode.hpp"
using namespace frame_misc_harness;
namespace camera_mode=halo2::native_seat_camera_mode_detail;
namespace {
constexpr std::uint32_t seats=0x10000000u,header=0x10010000u,players=0x10020000u,cameras=0x4f5514u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x158340u) {
        const auto slot=r(s.memory,s.gp[4]+4u);
        const auto camera=s.gp[6];
        const bool known=camera>=cameras && camera<cameras+4u*0x140u;
        w(s.memory,s.observe,target+1u,slot,2,known && rng()%2?r(s.memory,camera+0x58u)&0xffffu:std::uint32_t(rng()%4));
    }
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x158340u?(std::uint32_t(rng())&0xffff0000u)|(rng()%2?1u:std::uint32_t(rng()%4)):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=cameras+0x140u*std::uint32_t(rng()%4);
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
static void run_158900(WorldState &s){stub(s,0x158900u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x158340u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
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
    m.put(0x4f54f0u,seats);
    for(std::uint32_t at=seats;at<seats+0x20u;at+=4)m.put(at,pick(3)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
    m.put(0x4f54f4u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,players);
    for(std::uint32_t at=players;at<players+4u*0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto camera=cameras+0x140u*k;
        for(std::uint32_t at=camera&~3u;at<camera+0x140u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t states[]{0x245400u,0x245ea0u,0x247140u};
        m.put(camera+0xcu,states[pick(3)]);
        m.put(camera+0x58u,pick(4),2);
    }
    m.put(0x46a0d0u,0x3f800000u);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x158000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&~0xffu)|(pick(3)==0?1u+pick(255):0u));
    random_abi(rng,c,stack);
    c.abi.gp[7]=pick(6)==0?0xffffffffu:pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{seats,header,players};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return camera_mode::bridge(s,[](auto &v){original::native_frame_misc_reference_158450(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},[](auto &v){original::run_158900(v);});
}
void reference(WorldState &s){original::run_158450(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_CAMERA_MODE");
    std::mt19937 rng(0x158450u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,camera_mode::coverage().load(),unsigned(camera_mode::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
