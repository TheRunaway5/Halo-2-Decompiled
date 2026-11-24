// Native TU1.5 record view (304C50) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 2F4990 is a hash-driven stub of its complete entry ABI and arguments
// (cdecl): it reads the view, writes scratch, the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy the stack is
// left unbalanced. Fixtures author records and sources, records that
// overlap the frame or not; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_record_view.hpp"
using namespace frame_misc_harness;
namespace view=halo2::native_record_view_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x2f4990u);
    for(std::uint32_t k=1;k<=4;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,r(s.memory,s.gp[4]+8u)+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x2f4991u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
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
static void run_2f4990(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto record=pick(6)==0?stack-0x10u+4u*pick(4):records+4u*pick(16);
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(record,records+0x80u+4u*pick(8));
    m.put(stack,0x300000u+pick(0x100));m.put(stack+8u,record);
    random_abi(rng,c,stack);
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s){return view::bridge(s,[](auto &v){original::native_frame_misc_reference_304c50(v);},[](auto &v){original::run_2f4990(v);});}
void reference(WorldState &s){original::run_304c50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_VIEW");
    std::mt19937 rng(0x304c50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,view::coverage().load(),unsigned(view::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
