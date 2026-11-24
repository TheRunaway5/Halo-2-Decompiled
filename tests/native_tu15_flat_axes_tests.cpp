// Native TU1.5 flat axes (152D60) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 30BD0 is a hash-driven stub of its complete entry ABI and the vector it
// is given: it answers a length in XMM0 (zero, minus zero, NaN or random)
// and may rewrite the vector; the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy the stack is left unbalanced.
// Fixtures author facings, fallbacks and default axes of random, zero and
// NaN floats, the zero global zero or not, and axes that overlap the
// facing; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_flat_axes.hpp"
using namespace frame_misc_harness;
namespace axes=halo2::native_flat_axes_detail;
namespace {
constexpr std::uint32_t vectors=0x10000000u,results=0x10010000u,scratch=0x10600000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%6) {
    case 0:return 0u;
    case 1:return 0x80000000u;
    case 2:return 0x7fc00000u|std::uint32_t(rng()%0x1000);
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%2001)-1000)/37.0f);
    }
}
void normalize_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x30bd0u);
    for(std::uint32_t k=0;k<3;++k)h=mix(h,r(s.memory,s.gp[0]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%2)w(s.memory,s.observe,0x30bd1u,s.gp[0]+4u*std::uint32_t(rng()%3),4,random_float(rng));
    else w(s.memory,s.observe,0x30bd1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    for(unsigned k=0;k<8;++k)if(k==0 || rng()%3==0) {
        std::array<std::uint32_t,4> lanes{k==0?random_float(rng):std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[k],lanes.data(),16);
    }
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_30bd0(WorldState &s){normalize_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=vectors;at<vectors+0x40u;at+=4)m.put(at,random_float(floats));
    for(std::uint32_t at=results;at<results+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x474d08u,vectors+0x30u);
    m.put(0x46a0e8u,pick(3)?0u:random_float(floats));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=vectors+4u*pick(4);c.abi.gp[7]=vectors+0x10u+4u*pick(4);c.abi.gp[6]=results+4u*pick(4);
    if(shape==7)c.abi.gp[6]=pick(2)?c.abi.gp[0]:c.abi.gp[7];
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,results};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return axes::bridge(s,[](auto &v){original::native_frame_misc_reference_152d60(v);},[](auto &v){original::run_30bd0(v);});
}
void reference(WorldState &s){original::run_152d60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FLAT_AXES");
    std::mt19937 rng(0x152d60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,axes::coverage().load(),unsigned(axes::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
