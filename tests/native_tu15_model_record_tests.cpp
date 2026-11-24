// Native TU1.5 first-person model record (168D76) against the unmodified
// translated body of the sandbox special unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16B240 and 145560 (through the unit's dispatch) are hash-driven stubs of
// their complete entry ABIs and arguments: 16B240 writes a matrix, 145560
// its destination; both write scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy ESI changes, the
// direction flag flips or the stack is left unbalanced. Fixtures author node
// maps or none, transforms or none, counts of 0, several, 64 and past it,
// and both copy directions; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_model_record.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_model_record_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,matrices=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto destination=r(s.memory,s.gp[4]+(target==0x16b240u?8u:4u));
    w(s.memory,s.observe,target+1u,destination+4u*std::uint32_t(rng()%13),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());
    if(target==0x16b240u || rng()%4==0)s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x40u+4u*std::uint32_t(rng()%64);
    if(policy&2u && rng()%17==0)s.other_flags^=0x400u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_16b240(WorldState &s){stub(s,0x16b240u,5);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x145560u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,1);
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
    const bool backward=shape==5 || shape==9;
    for(std::uint32_t at=records;at<records+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=matrices;at<matrices+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t counts[]{0u,1u,2u,3u,7u,0x40u,0x41u,0x80u,0xffffffffu};
    m.put(stack,0x168000u+pick(0x100));
    m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,pick(2));
    m.put(stack+12u,pick(2)?0u:records+0x1f00u);m.put(stack+16u,pick(3)?0u:records+0x1e00u);
    m.put(stack+20u,backward?records+0x1000u:records+4u*pick(16));
    random_abi(rng,c,stack);
    c.abi.gp[0]=std::uint32_t(rng());c.abi.gp[1]=pick(3)?pick(9):counts[pick(9)];
    c.abi.gp[2]=backward?matrices+0x1000u:matrices+4u*pick(16);
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,matrices};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_168d76(v);},[](auto &v){original::run_16b240(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_168d76(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MODEL_RECORD");
    std::mt19937 rng(0x168d76u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
