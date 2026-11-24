// Native TU1.5 permutation choice (1700E0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 170930 and 170B40 are hash-driven stubs of their complete entry ABIs:
// each hashes what it reads (its pushed arguments), writes scratch, 170930
// the count and that many candidates, 170B40 answers a candidate or NONE in
// EAX; the caller-saved registers, XMM (the weight in XMM4 among them),
// flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI
// change or the stack is left unbalanced. Fixtures author definitions with
// regions of every permutation count, flagged and not, variants with and
// without region bytes, and held masks; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_permutation_choice.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_permutation_choice_detail;
namespace {
constexpr std::uint32_t definitions=0x10000000u,definition=0x10001000u,regions=0x10002000u,permutations=0x10003000u,
    variants=0x10004000u,candidates=0x10005000u,chosen=0x10006000u,records=0x10007000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=target==0x170930u?6u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x170930u && rng()%4) {
        const auto count=rng()%3?0u:1u+std::uint32_t(rng()%4);
        w(s.memory,s.observe,target+1u,s.gp[2],4,count);
        for(std::uint32_t k=0;k<count;++k)for(std::uint32_t f=0;f<0x1cu;f+=4)
            w(s.memory,s.observe,target+2u,s.gp[7]+0x1cu*k+f,4,std::uint32_t(rng()));
    }
    const auto count=s.gp[2];
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x170b40u)s.gp[0]=rng()%4==0||count==0u||count>32u?0xffffffffu:std::uint32_t(rng()%count);
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0u:std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=regions+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_170930(WorldState &s){stub(s,0x170930u);}
static void run_170b40(WorldState &s){stub(s,0x170b40u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f0414u,definitions);m.put(0x46a0d0u,chance(8)?std::uint32_t(rng()):0x3f800000u);
    for(std::uint32_t k=0;k<4;++k){m.put(definitions+16u*k,std::uint32_t(rng()));m.put(definitions+16u*k+4u,std::uint32_t(rng()));
        m.put(definitions+16u*k+8u,definition);m.put(definitions+16u*k+12u,std::uint32_t(rng()));}
    for(std::uint32_t at=definition;at<definition+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto region_total=chance(10)?0u:chance(12)?0xfffffffeu:1u+pick(6);
    m.put(definition+0x54u,variants);m.put(definition+0x70u,region_total);m.put(definition+0x74u,regions);
    // Regions: every permutation count up to 12, with some or all permutations flagged.
    for(std::uint32_t k=0;k<8;++k) {
        const auto r=regions+0x10u*k;
        m.put(r,std::uint32_t(rng()));m.put(r+4u,std::uint32_t(rng()));
        m.put(r+8u,chance(6)?0u:chance(10)?0xffffffffu:1u+pick(12));m.put(r+0xcu,permutations+0x100u*k);
        const bool all=chance(5);
        for(std::uint32_t p=0;p<12;++p){m.put(permutations+0x100u*k+8u*p,std::uint32_t(rng()));
            m.put(permutations+0x100u*k+8u*p+4u,(std::uint32_t(rng())&~1u)|(all||chance(3)?1u:0u));}
    }
    for(std::uint32_t k=0;k<3;++k) {
        const auto v=variants+0x38u*k;
        for(std::uint32_t at=v;at<v+0x38u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t r=0;r<8;++r)m.put(v+4u+r,chance(2)?0xffu:pick(4),1);
        m.put(v+0x18u,candidates);
    }
    for(std::uint32_t at=candidates;at<candidates+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=chosen;at<chosen+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x440u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    m.put(stack+4u,chance(8)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(stack+8u,chance(3)?0xffffffffu:pick(3));
    m.put(stack+12u,chosen);m.put(stack+16u,records);
    m.put(stack+20u,chance(2)?0u:std::uint32_t(rng()));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{definitions,definition,regions,permutations,variants,candidates,chosen,records};
        m.drop(areas[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1700e0(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0x170930u:original::run_170930(v);break;case 0x170b40u:original::run_170b40(v);break;
            default:throw std::logic_error("Unbound permutation choice child");}});
}
void reference(WorldState &s){original::run_1700e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PERMUTATION_CHOICE");
    std::mt19937 rng(0x1700e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
