// Native TU1.5 instance blocks (16BBC0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16B810, 145200 and 14CDB0 are hash-driven stubs of their complete entry
// ABIs (and the flags pushed): 16B810 accepts or not, 14CDB0 answers a
// cluster or none; each writes scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI
// change or the stack is left unbalanced. Fixtures author instances with
// points inside, on and outside their spheres (NaN too), and one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_instance_blocks.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_instance_blocks_detail;
namespace {
constexpr std::uint32_t bsp=0x10000000u,instances=0x10010000u,definitions=0x10020000u,points=0x10030000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t answer=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?1u+std::uint32_t(rng()%255):0u);
    if(target==0x14cdb0u)answer=rng()%2?std::uint32_t(rng()%4):0xffffffffu;
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=points+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=definitions+0xc8u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=instances+0x58u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x16b810u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_16b810(WorldState &s){stub(s,0x16b810u);}
static void run_145200(WorldState &s){stub(s,0x145200u);}
static void run_14cdb0(WorldState &s){stub(s,0x14cdb0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecc18u,bsp);
    for(std::uint32_t at=bsp;at<bsp+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(bsp+0x144u,instances);m.put(bsp+0x13cu,definitions);
    static constexpr float coords[]{-2.f,-1.f,0.f,0.5f,1.f,2.f};
    for(std::uint32_t k=0;k<4;++k) {
        const auto i=instances+0x58u*k;
        for(std::uint32_t at=i;at<i+0x58u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(i+0x34u,pick(2),2);
        for(std::uint32_t at=i+0x3cu;at<i+0x48u;at+=4)m.put(at,pick(12)?bits_of(coords[pick(6)]):0x7fc00000u);
        m.put(i+0x48u,bits_of(float(pick(4))));
    }
    for(std::uint32_t at=definitions;at<definitions+0x190u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=points;at<points+0x40u;at+=4)m.put(at,bits_of(coords[pick(6)]));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(4);c.abi.gp[3]=points+0x10u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{bsp,instances,definitions,points};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_16bbc0(v);},[](auto &v){original::run_16b810(v);},
        [](auto &v){original::run_145200(v);},[](auto &v){original::run_14cdb0(v);});
}
void reference(WorldState &s){original::run_16bbc0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_INSTANCE_BLOCKS");
    std::mt19937 rng(0x16bbc0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
