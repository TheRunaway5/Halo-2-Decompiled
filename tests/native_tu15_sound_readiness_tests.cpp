// Native TU1.5 sound readiness (1718C0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 21F260 is a hash-driven stub of its complete entry ABI and arguments: it
// answers AL (bit 1 or not), writes scratch, the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or
// EDI change or the stack is left unbalanced. Fixtures author sounds of
// negative, zero and several permutations, and permutations of negative,
// zero and several chunks; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "audio/native_tu15_sound_readiness.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_sound_readiness_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,definitions=0x10010000u,sound_globals=0x10020000u,permutations=0x10030000u,
    chunks=0x10040000u,samples=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x21f260u);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x21f262u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xfffffff0u)|(rng()%5?2u:0u)|std::uint32_t(rng()%2);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=(s.gp[3]&0xffffff00u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[7]=permutations+12u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=16u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_21f260(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,instances);m.put(0x52d664u,sound_globals);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x20u*k);}
    for(std::uint32_t at=definitions;at<definitions+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(definitions+0x20u*k+8u,pick(4),2);m.put(definitions+0x20u*k+0xau,pick(5)?pick(4):pick(2)?0u:0xffu,1);}
    for(std::uint32_t at=sound_globals;at<sound_globals+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(sound_globals+0x24u,permutations);m.put(sound_globals+0x2cu,chunks);m.put(sound_globals+0x44u,samples);
    for(std::uint32_t at=permutations;at<permutations+12u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k){m.put(permutations+12u*k+8u,pick(4),2);m.put(permutations+12u*k+0xau,pick(5)?pick(4):0xffffu,2);}
    for(std::uint32_t at=chunks;at<chunks+16u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(chunks+16u*k+0xcu,pick(4),2);
    for(std::uint32_t at=samples;at<samples+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,definitions,sound_globals,permutations,chunks};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1718c0(v);},[](auto &v){original::run_21f260(v);});}
void reference(WorldState &s){original::run_1718c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SOUND_READINESS");
    std::mt19937 rng(0x1718c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
