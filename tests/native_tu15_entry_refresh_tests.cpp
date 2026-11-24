// Native TU1.5 entry refresh (1E5D60) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 126200 is a hash-driven stub of its complete entry ABI (EAX the entry, CL
// zero): it writes the entry and by policy the record's count, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EDI moves to the other record, EBX or ESI change, or the stack is
// left unbalanced. Fixtures author records with no, negative or a few
// entries, marked or not; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_entry_refresh.hpp"
using namespace frame_misc_harness;
namespace refresh=halo2::native_entry_refresh_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,entries=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,s.gp[0]+4u*std::uint32_t(rng()%5),4,std::uint32_t(rng()));
    // A count rewritten, negative too (the loop's comparison is signed).
    static constexpr std::uint32_t counts[]{0u,1u,2u,3u,5u,0xffffffffu};
    if(policy&2u && rng()%9==0)w(s.memory,s.observe,target+2u,records+0x200u*std::uint32_t(rng()%2)+0xacu,4,counts[rng()%6]);
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=records+0x200u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[3]=std::uint32_t(rng()%6);
    if(policy&2u && rng()%17==0)s.gp[6]=0x14u*std::uint32_t(rng()%6);
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
static void run_126200(WorldState &s){stub(s,0x126200u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t k=0;k<2;++k) {
        const auto record=records+0x200u*k;
        for(std::uint32_t at=record;at<record+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t counts[]{0u,0xffffffffu,0x80000000u,1u,2u,3u,5u};
        m.put(record+0xacu,counts[pick(7)]);
        m.put(record+0xb0u,entries+0x1000u*k+4u*pick(4));
        for(std::uint32_t at=entries+0x1000u*k;at<entries+0x1000u*k+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t e=0;e<8;++e)m.put(entries+0x1000u*k+4u*pick(4)+0x14u*e+0x10u,pick(2)?pick(256):0u,1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=records+0x200u*pick(2);
    if(shape==15)m.drop(pick(2)?records:entries);
    return c;
}
bool native(WorldState &s){return refresh::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5d60(v);},[](auto &v){original::run_126200(v);});}
void reference(WorldState &s){original::run_1e5d60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SMALL_CALLS");
    std::mt19937 rng(0x1e5d60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,refresh::coverage().load(),unsigned(refresh::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
