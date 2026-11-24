// Native TU1.5 unit camouflage (C8CF0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C1E90 and B7E50 are hash-driven stubs of their complete entry ABIs and
// arguments: C1E90 answers AL and pops its argument, B7E50 pops its two;
// both write scratch, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author units with and without players,
// teams NONE, negative, within and past the 16, every game mode and session
// byte, camouflage and second fades near, at and past zero and one (NaN
// too), and attachments or NONE; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_camouflage.hpp"
using namespace frame_misc_harness;
namespace camouflage=halo2::native_unit_camouflage_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,units=0x10020000u,globals=0x10030000u,scratch=0x10600000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%8) {
    case 0:return 0u;
    case 1:return 0x3f800000u;
    case 2:return 0x7fc00000u;
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%3001)-1000)/1000.0f);
    }
}
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[3]=globals+0x100u+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%17==0)s.gp[5]=header;
    if(policy&2u && rng()%17==0)s.gp[6]=units+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[7]=12u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{random_float(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    if(rng()%3==0)std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c1e90(WorldState &s){stub(s,0xc1e90u,1);}
static void run_b7e50(WorldState &s){stub(s,0xb7e50u,2);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,units+0x400u*(k%2));}
    for(std::uint32_t u=0;u<2;++u) {
        const auto unit=units+0x400u*u;
        for(std::uint32_t at=unit;at<unit+0x300u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x12cu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(unit+0x134u,std::uint32_t(rng()));
        static constexpr std::uint32_t teams[]{0xffffu,0x8003u,16u,17u,0u,1u,5u,15u};
        m.put(unit+0x138u,teams[pick(8)],2);
        m.put(unit+0x2b0u,random_float(floats));m.put(unit+0x2b4u,random_float(floats));m.put(unit+0x2b8u,random_float(floats));
        m.put(unit+0xd4u,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
    }
    m.put(0x4f3218u,globals);m.put(globals+8u,pick(3)?1u:pick(4));
    m.put(0x5020b6u,pick(3)?1u+pick(255):0u,1);
    m.put(0x5020c4u,globals+0x40u);
    for(std::uint32_t at=globals+0x40u;at<globals+0x120u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x46a0d0u,pick(8)?0x3f800000u:random_float(floats));
    m.put(0x51df54u,globals+0x100u);m.put(globals+0x104u,random_float(floats));
    for(std::uint32_t at=globals+0x108u;at<globals+0x120u;at+=4)m.put(at,random_float(floats));
    m.put(0x46a174u,random_float(floats));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,units,globals};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return camouflage::bridge(s,[](auto &v){original::native_frame_misc_reference_c8cf0(v);},
        [](auto &v){original::run_c1e90(v);},[](auto &v){original::run_b7e50(v);});
}
void reference(WorldState &s){original::run_c8cf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_CAMOUFLAGE");
    std::mt19937 rng(0xc8cf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,camouflage::coverage().load(),unsigned(camouflage::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
