// Native TU1.5 director value (189F20) against the unmodified translated
// body of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// CAD60 is a hash-driven stub of its complete entry ABI: it answers a factor
// in XMM0 (one, zero, NaN or random) and may overwrite a kept float; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX changes or the stack is left unbalanced. Fixtures author
// records with and without vehicles, values of either sign (NaN too) and
// bounds about the quotient; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_director_value.hpp"
using namespace frame_misc_harness;
namespace director=halo2::native_director_value_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,header=0x10010000u,rows=0x10020000u,objects=0x10030000u,tags=0x10040000u,
    definitions=0x10050000u,scratch=0x10600000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%8) {
    case 0:return 0u;
    case 1:return 0x7fc00000u;
    case 2:case 3:return 0x3f800000u;
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%2001)-500)/97.0f);
    }
}
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0xcad60u);
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%5==0)w(s.memory,s.observe,0xcad61u,s.gp[4]+4u+4u*std::uint32_t(rng()%3),4,random_float(rng));
    else w(s.memory,s.observe,0xcad61u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    for(unsigned k=0;k<8;++k)if(k==0 || rng()%4==0) {
        std::array<std::uint32_t,4> lanes{random_float(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[k],lanes.data(),16);
    }
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_cad60(WorldState &s){stub(s);}
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
    m.put(0x4f9b54u,records);
    for(std::uint32_t at=records;at<records+4u*0x94u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(records+0x94u*k+0x14u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
    m.put(0x55d6e4u,random_float(floats));
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x40u*(k%2));}
    for(std::uint32_t o=0;o<2;++o){for(std::uint32_t at=objects+0x40u*o;at<objects+0x40u*o+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(objects+0x40u*o,(std::uint32_t(rng())<<16)|pick(4));}
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=tags+16u*k;at<tags+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));m.put(tags+16u*k+8u,definitions+0x100u*(k%2));}
    for(std::uint32_t d=0;d<2;++d)m.put(definitions+0x100u*d+0xccu,random_float(floats));
    m.put(0x46a0d0u,pick(6)?0x3f800000u:random_float(floats));
    m.put(0x46a7c4u,pick(4)?std::bit_cast<std::uint32_t>(-2.0f):random_float(floats));
    m.put(0x46a7c0u,pick(4)?std::bit_cast<std::uint32_t>(4.0f):random_float(floats));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x180000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,header,rows,objects,tags,definitions};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return director::bridge(s,[](auto &v){original::native_frame_misc_reference_189f20(v);},[](auto &v){original::run_cad60(v);});}
void reference(WorldState &s){original::run_189f20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DIRECTOR_VALUE");
    std::mt19937 rng(0x189f20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,director::coverage().load(),unsigned(director::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
