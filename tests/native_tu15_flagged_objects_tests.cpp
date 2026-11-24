// Native TU1.5 flagged object visit (102910) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BCEE0 and 102980 are reached through the unit's dispatch, hash-driven
// stubs of their complete entry ABIs and arguments: BCEE0 answers the next
// object (or 0, ending the walk within a few steps) and writes the
// iterator's current handle, 102980 writes scratch; each pops its argument,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy the stack is left unbalanced. Fixtures author objects
// flagged or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_flagged_objects.hpp"
using namespace frame_misc_harness;
namespace flagged=halo2::native_flagged_objects_detail;
namespace {
constexpr std::uint32_t objects=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto argument=r(s.memory,s.gp[4]+4u);
    if(target==0xbcee0u)w(s.memory,s.observe,target+1u,argument+8u,4,std::uint32_t(rng()));
    else w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0xbcee0u?(rng()%4==0?0u:objects+0x400u*std::uint32_t(rng()%4)):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbcee0u && target!=0x102980u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    for(std::uint32_t k=0;k<4;++k)
        for(std::uint32_t at=objects+0x400u*k+0x160u;at<objects+0x400u*k+0x180u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15)m.drop(objects+0x400u*pick(4));
    return c;
}
bool native(WorldState &s) {
    return flagged::bridge(s,[](auto &v){original::native_frame_misc_reference_102910(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_102910(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FLAGGED_OBJECTS");
    std::mt19937 rng(0x102910u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,flagged::coverage().load(),unsigned(flagged::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
