// Native TU1.5 object rate (DFE30) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// DFD20, E65A0 and E3310 are hash-driven stubs of their complete entry ABIs:
// DFD20 answers a value in XMM0 (zero, minus zero, NaN or random), the
// others AL; each writes scratch, the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, ESI or EDI change or the
// stack is left unbalanced. Fixtures author definitions with and without
// flag bit 1, spans and tick rates of either sign (NaN too), and the zero
// global zero or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_rate.hpp"
using namespace frame_misc_harness;
namespace object_rate=halo2::native_object_rate_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,objects=0x10020000u,tags=0x10030000u,definitions=0x10040000u,
    outputs=0x10050000u,scratch=0x10600000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%7) {
    case 0:return 0u;
    case 1:return 0x80000000u;
    case 2:return 0x7fc00000u;
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%2001)-1000)/37.0f);
    }
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=definitions+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng())<<16|std::uint32_t(rng()%4);
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
static void run_dfd20(WorldState &s){stub(s,0xdfd20u);}
static void run_e65a0(WorldState &s){stub(s,0xe65a0u);}
static void run_e3310(WorldState &s){stub(s,0xe3310u);}
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
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x40u*(k%2));}
    for(std::uint32_t o=0;o<2;++o){for(std::uint32_t at=objects+0x40u*o;at<objects+0x40u*o+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(objects+0x40u*o,(std::uint32_t(rng())<<16)|pick(4));}
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=tags+16u*k;at<tags+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));m.put(tags+16u*k+8u,definitions+0x400u*(k%2));}
    for(std::uint32_t d=0;d<2;++d) {
        const auto definition=definitions+0x400u*d;
        for(std::uint32_t at=definition+0x260u;at<definition+0x270u;at+=4)m.put(at,random_float(floats));
        m.put(definition+0x264u,pick(3)==0?std::uint32_t(rng()):std::uint32_t(rng())&~2u);
    }
    m.put(0x46a0e8u,pick(4)?0u:random_float(floats));
    m.put(0x51df54u,outputs+0x40u);m.put(outputs+0x40u,(pick(4)?30u:std::uint32_t(rng()))<<16|pick(0x10000));
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xdf0000u+pick(0x100));m.put(stack+4u,outputs+4u*pick(8));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,objects,tags,definitions,outputs};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return object_rate::bridge(s,[](auto &v){original::native_frame_misc_reference_dfe30(v);},[](auto &v){original::run_dfd20(v);},
        [](auto &v){original::run_e65a0(v);},[](auto &v){original::run_e3310(v);});
}
void reference(WorldState &s){original::run_dfe30(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_RATE");
    std::mt19937 rng(0xdfe30u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,object_rate::coverage().load(),unsigned(object_rate::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
