// Native TU1.5 element search (1E5360) against the unmodified translated
// body of the AI frame unit (the physics live support unit's copy is the
// same), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). 1E5C80 is a hash-driven stub of its complete entry ABI
// and argument: it writes scratch, the caller-saved registers, XMM, flags
// and (by policy) MXCSR are random, and by policy ESI changes or the stack
// is left unbalanced. Fixtures author definitions (or none) with negative,
// empty and several references, references without a tag or a block, and
// blocks of negative, empty and several elements holding the value once,
// twice or not at all; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/ai/native_tu15_element_search.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_element_search_detail;
namespace {
constexpr std::uint32_t definitions=0x10000000u,references=0x10010000u,instances=0x10020000u,blocks=0x10030000u,
    elements=0x10100000u,outputs=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%7==0)s.gp[6]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%17==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1e5c80(WorldState &s){stub(s,0x1e5c80u,1);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const std::uint32_t sought=pick(4)?0x1234u+pick(3):std::uint32_t(rng());
    static constexpr std::uint32_t counts[]{0u,1u,2u,3u,5u,0xffffffffu,0x80000000u};
    m.put(0x4f0414u,instances);
    // Eight blocks: block 0 is the definition's own.
    for(std::uint32_t b=0;b<8;++b) {
        const auto block=blocks+0x40u*b,array=elements+0x400u*b;
        for(std::uint32_t at=block;at<block+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t at=definitions+0x40u*b;at<definitions+0x40u*b+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto count=pick(3)?pick(6):counts[pick(7)];
        m.put(block+0x2cu,count);m.put(block+0x30u,array);
        for(std::uint32_t e=0;e<6;++e)m.put(array+0x6cu*e,pick(5)==0?sought:std::uint32_t(rng()));
    }
    const auto definition=definitions+0x200u;
    for(std::uint32_t at=definition;at<definition+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto own=blocks;
    for(std::uint32_t at=own;at<own+0x40u;at+=4)m.put(definition+(at-own),r(m,at));
    m.put(definition+0x4cu,pick(4)?pick(6):counts[pick(7)]);m.put(definition+0x50u,references);
    for(std::uint32_t k=0;k<6;++k) {
        for(std::uint32_t at=references+0x20u*k;at<references+0x20u*k+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(references+0x20u*k+4u,pick(8)?(std::uint32_t(rng())&0xffff0000u)|pick(8):0xffffffffu);
    }
    for(std::uint32_t k=0;k<8;++k) {
        for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,pick(8)?blocks+0x40u*(1u+pick(7)):0u);
    }
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));m.put(stack+4u,outputs+4u*pick(8));m.put(stack+8u,sought);
    random_abi(rng,c,stack);
    c.abi.gp[2]=shape==3?0u:definition;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{definitions,references,instances,blocks,elements,outputs};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5360(v);},[](auto &v){original::run_1e5c80(v);});}
void reference(WorldState &s){original::run_1e5360(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ELEMENT_SEARCH");
    std::mt19937 rng(0x1e5360u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
