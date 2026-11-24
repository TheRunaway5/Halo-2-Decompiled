// Native TU1.5 stamps against the unmodified translated bodies of the combat
// action unit, extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). Shared by one test per owner, which defines OWNER first:
// 43980 stamp advance, 12F0C0 framed stamp advance. 13FD10 and 12F040 are
// hash-driven stubs of their complete entry ABIs: each may rewrite a stamp,
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy ESI or EDI change or the stack is left
// unbalanced. Fixtures author stamps at and below the limit and frame counts
// that wrap or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x43980
#include "gameplay/native_tu15_stamp_advance.hpp"
namespace leaf=halo2::native_stamps_detail::stamp_advance;
#else
#include "gameplay/native_tu15_stamp_framed.hpp"
namespace leaf=halo2::native_stamps_detail::stamp_framed;
#endif
using namespace frame_misc_harness;
namespace {
constexpr std::uint32_t records=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%5==0)w(s.memory,s.observe,target+1u,records+0x100u*std::uint32_t(rng()%2)+0x38u,4,std::uint32_t(rng()%4));
    else w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
static void run_13fd10(WorldState &s){stub(s,0x13fd10u);}
static void run_12f040(WorldState &s){stub(s,0x12f040u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t g=0;g<2;++g)m.put(records+0x100u*g+0x38u,pick(3)==0?0x7fffffffu:std::uint32_t(rng()));
    m.put(0x515f28u,records+0x100u*pick(2));m.put(0x4f2d34u,records+0x100u*pick(2));
    m.put(0x4f2d58u,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x120000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15)m.drop(records);
    return c;
}
#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)
bool native(WorldState &s) {
    return leaf::bridge(s,[](auto &v){CAT(original::native_frame_misc_reference_,OWNER_NAME)(v);},
        [](auto &v){original::run_13fd10(v);},[](auto &v){original::run_12f040(v);});
}
void reference(WorldState &s){CAT(original::run_,OWNER_NAME)(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,VERIFY_SWITCH);
    std::mt19937 rng(OWNER+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,leaf::coverage().load(),unsigned(leaf::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
