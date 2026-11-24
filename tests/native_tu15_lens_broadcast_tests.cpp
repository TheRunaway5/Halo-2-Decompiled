// Native TU1.5 lens broadcast (20700) against the unmodified translated body
// of the lens flare unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 20E30 is a hash-driven stub of its complete
// entry ABI: it may change the view count, writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy ESI
// changes or the stack is left unbalanced. Fixtures author handles NONE or
// not, deferring on or off, views or none and counts of either sign; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_lens_broadcast.hpp"
using namespace frame_misc_harness;
namespace broadcast=halo2::native_lens_broadcast_detail;
namespace {
constexpr std::uint32_t scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x20e30u);
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%9==0)w(s.memory,s.observe,0x20e31u,0x531fbcu,4,std::uint32_t(rng()%5));
    else w(s.memory,s.observe,0x20e31u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[6]=std::uint32_t(rng()%4);
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
static void run_20e30(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x52dfd8u,pick(2)?0u:1u+pick(255),1);m.put(0x4918e8u,pick(2)?0u:std::uint32_t(rng()));
    m.put(0x532094u,std::uint32_t(rng()));m.put(0x531fbcu,pick(5)==0?0x80000000u+pick(4):pick(5));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x20000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(5)==0?0xffffffffu:std::uint32_t(rng());
    if(shape==15)m.drop(pick(2)?0x532000u:0x531000u);
    return c;
}
bool native(WorldState &s){return broadcast::bridge(s,[](auto &v){original::native_frame_misc_reference_20700(v);},[](auto &v){original::run_20e30(v);});}
void reference(WorldState &s){original::run_20700(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LENS_BROADCAST");
    std::mt19937 rng(0x20700u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,broadcast::coverage().load(),unsigned(broadcast::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
