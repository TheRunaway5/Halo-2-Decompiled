// Native TU1.5 unit reaction (E71A0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E72E0 is a hash-driven stub of its complete entry ABI (and the value, kind
// and unit it is handed): it writes scratch; the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI
// change or the stack is left unbalanced. Fixtures author every mode,
// counters below, at and past the limit, blocks with either id or another,
// references usable or not with weights zero (and -0, NaN) or not, levels
// below 0, inside and past the clamp, and one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_reaction.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_reaction_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,units=0x10020000u,clock=0x10030000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=units+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
static void run_e72e0(WorldState &s){stub(s,0xe72e0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x51df54u,clock);
    m.put(0x46a178u,bits_of(0.5f));m.put(0x46a0d0u,bits_of(1.f));m.put(0x456690u,bits_of(0.125f));m.put(0x46a17cu,bits_of(pick(4)?2.f:0.f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,units+0x400u*k);}
    for(std::uint32_t at=clock;at<clock+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(clock+4u,bits_of(1.f/30.f));
    static constexpr std::uint32_t ids[]{0x900000eu,0xa00000fu,0x900000fu};
    static constexpr float weights[]{0.f,-0.f,0.5f,1.f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto unit=units+0x400u*k;
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto offset=0x100u+4u*pick(8);
        m.put(unit+0x12au,offset,2);
        m.put(unit+offset,pick(4)?pick(4):0xffffffffu);m.put(unit+offset+6u,pick(4)?pick(4):0xffffu,2);
        m.put(unit+offset+0x1cu,pick(10)?bits_of(weights[pick(4)]):0x7fc00000u);m.put(unit+offset+0x7cu,ids[pick(3)]);
        m.put(unit+0x34au,pick(3)?0u:pick(2)?1u:pick(256),1);m.put(unit+0x39bu,pick(3)?pick(20):pick(256),1);
        m.put(unit+0x39fu,pick(4)?pick(12):pick(256),1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())&0xffff0000u)|pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,units,clock};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_e71a0(v);},[](auto &v){original::run_e72e0(v);});}
void reference(WorldState &s){original::run_e71a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_REACTION");
    std::mt19937 rng(0xe71a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
