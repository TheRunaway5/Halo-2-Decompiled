// Native TU1.5 type hooks (10DB40) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Each hook function (through the unit's dispatch) is a hash-driven stub of
// its complete entry ABI and arguments: it answers AL, pops its three
// arguments, writes scratch, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author types with no hooks or up to
// five, hooks with and without functions; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_type_hooks.hpp"
using namespace frame_misc_harness;
namespace hooks=halo2::native_type_hooks_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,objects=0x10020000u,definitions=0x10030000u,hook_records=0x10040000u,
    scratch=0x10600000u,functions=0x00500000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%3==0?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[6]=definitions+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[7]=std::uint32_t(rng()%3);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+12u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target<functions || target>=functions+0x100u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x100u*k);}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=objects+0x100u*k;at<objects+0x100u*k+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(objects+0x100u*k+0xaau,pick(4),1);}
    for(std::uint32_t k=0;k<4;++k)m.put(0x474b90u+4u*k,definitions+0x100u*(k%2));
    for(std::uint32_t d=0;d<2;++d) {
        const auto definition=definitions+0x100u*d;
        for(std::uint32_t at=definition;at<definition+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto count=pick(6);
        for(std::uint32_t k=0;k<6;++k)m.put(definition+0x84u+4u*k,k<count?hook_records+0x40u*(k+6u*d):0u);
    }
    for(std::uint32_t k=0;k<12;++k){for(std::uint32_t at=hook_records+0x40u*k;at<hook_records+0x40u*k+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(hook_records+0x40u*k+0x2cu,pick(3)==0?0u:functions+0x10u*pick(16));}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x10d000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[3]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,objects,definitions,hook_records};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return hooks::bridge(s,[](auto &v){original::native_frame_misc_reference_10db40(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_10db40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TYPE_HOOKS");
    std::mt19937 rng(0x10db40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,hooks::coverage().load(),unsigned(hooks::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
