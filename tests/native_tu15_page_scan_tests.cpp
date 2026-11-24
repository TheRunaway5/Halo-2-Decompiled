// Native TU1.5 page scan (1F33C0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1F3460 (through the unit's dispatch) is a hash-driven stub of its complete
// entry ABI: it writes the stride word and answers a table (with counts and
// bases of either sign, zero too) and may rewrite the cursor; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy ESI or EDI change or the stack is left unbalanced. Fixtures author
// cursors with every index; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_page_scan.hpp"
using namespace frame_misc_harness;
namespace scan=halo2::native_page_scan_detail;
namespace {
constexpr std::uint32_t cursors=0x10000000u,tables=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1f3460u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1f3461u,s.gp[1],4,rng()%3==0?0u:std::uint32_t(rng()%16));
    if(rng()%6==0)w(s.memory,s.observe,0x1f3462u,cursors+6u,2,std::uint32_t(rng()%4));
    else if(rng()%9==0)w(s.memory,s.observe,0x1f3462u,cursors+4u,2,rng()%2?0xffffu:std::uint32_t(rng()%6));
    else w(s.memory,s.observe,0x1f3462u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=tables+8u*std::uint32_t(rng()%8);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=cursors+0x10u*std::uint32_t(rng()%2)+4u;
    if(policy&2u && rng()%13==0)s.gp[7]=cursors+0x10u*std::uint32_t(rng()%2);
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
    if(target!=0x1f3460u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    for(std::uint32_t at=cursors;at<cursors+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k){m.put(tables+8u*k,pick(4)==0?0x80000000u:pick(4));m.put(tables+8u*k+4u,pick(3)==0?0u:std::uint32_t(rng()));}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1f0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=cursors+0x10u*pick(2);
    if(shape==15)m.drop(pick(2)?cursors:tables);
    return c;
}
bool native(WorldState &s) {
    return scan::bridge(s,[](auto &v){original::native_frame_misc_reference_1f33c0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1f33c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PAGE_SCAN");
    std::mt19937 rng(0x1f33c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,scan::coverage().load(),unsigned(scan::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
