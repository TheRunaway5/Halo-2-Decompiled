// Native TU1.5 placement repair (175380) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 175300, B1720, B16C0 and the tail 1722C0 are hash-driven stubs of their
// complete entry ABIs: each answers sane or not (the tail returns to this
// owner's caller), writes scratch; the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or
// the stack is left unbalanced. Fixtures author points inside, below and
// above the world bounds (NaN too), scalars inside and outside their ranges,
// and one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_placement_repair.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_placement_repair_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,defaults=0x10010000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?1u+std::uint32_t(rng()%255):0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=records+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x200u+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=records+0x200u+0x10u*std::uint32_t(rng()%4);
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
static void run_175300(WorldState &s){stub(s,0x175300u);}
static void run_b1720(WorldState &s){stub(s,0xb1720u);}
static void run_b16c0(WorldState &s){stub(s,0xb16c0u);}
static void run_1722c0(WorldState &s){stub(s,0x1722c0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x474d08u,defaults);m.put(0x474d10u,defaults+0x10u);m.put(0x474d04u,defaults+0x20u);
    m.put(0x44ea9cu,bits_of(-5000.f));m.put(0x44eaa0u,bits_of(5000.f));
    m.put(0x55d6e8u,bits_of(0.001f));m.put(0x55d6ecu,bits_of(100.f));m.put(0x55d6f0u,bits_of(10.f));
    for(std::uint32_t at=defaults;at<defaults+0x30u;at+=4)m.put(at,bits_of(float(pick(3))));
    static constexpr float values[]{0.f,1.f,-1.f,4999.f,5001.f,-5001.f,-5000.f,5000.f,0.0005f,150.f,11.f,-0.f};
    for(std::uint32_t k=0;k<2;++k)for(std::uint32_t at=records+0x100u*k;at<records+0x100u*(k+1u);at+=4)
        m.put(at,pick(12)?bits_of(values[pick(12)]):0x7fc00000u);
    for(std::uint32_t at=records+0x200u;at<records+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records+0x100u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,defaults};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_175380(v);},[](auto &v){original::run_175300(v);},
        [](auto &v){original::run_b1720(v);},[](auto &v){original::run_b16c0(v);},[](auto &v){original::run_1722c0(v);});
}
void reference(WorldState &s){original::run_175380(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLACEMENT_REPAIR");
    std::mt19937 rng(0x175380u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
