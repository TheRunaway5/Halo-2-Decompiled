// Native TU1.5 seated vector (CDCF0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BBFF0, 30BD0 and 144210 are hash-driven stubs of their complete entry ABIs
// (and the vectors they are handed): BBFF0 writes the forward and up axes,
// 30BD0 normalizes and answers a length in XMM0 (exactly 0 at times), 144210
// writes the matrix; each writes scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI
// change or the stack is left unbalanced. Fixtures author objects without
// players or seats, seats with and without flag bit 4, and one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_seated_vector.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_seated_vector_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    seats=0x10050000u,axes=0x10060000u,outputs=0x10070000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
std::uint32_t some_float(std::mt19937_64 &rng){return rng()%10?bits_of(float(int(rng()%41)-20)*0.125f):std::uint32_t(rng());}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto at=target==0xbbff0u?s.gp[7]:s.gp[0];
    for(std::uint32_t k=0;k<3;++k)h=mix(h,r(s.memory,at+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xbbff0u)for(std::uint32_t k=0;k<3;++k){w(s.memory,s.observe,0xbbff1u,s.gp[6]+4u*k,4,some_float(rng));w(s.memory,s.observe,0xbbff2u,s.gp[7]+4u*k,4,some_float(rng));}
    if(target==0x30bd0u)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,0x30bd1u,s.gp[0]+4u*k,4,some_float(rng));
    if(target==0x144210u)for(std::uint32_t k=0;k<13;++k)w(s.memory,s.observe,0x144211u,s.gp[1]+4u*k,4,some_float(rng));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=outputs+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    if(target==0x30bd0u){lanes[0]=rng()%3?0u:rng()%2?0x80000000u:some_float(rng);std::memcpy(&s.xmm[0],lanes.data(),16);}
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_bbff0(WorldState &s){stub(s,0xbbff0u);}
static void run_30bd0(WorldState &s){stub(s,0x30bd0u);}
static void run_144210(WorldState &s){stub(s,0x144210u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x474d18u,axes);m.put(0x474d1cu,axes+0x10u);m.put(0x46a0e8u,0u);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x200u*k;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(2));
        m.put(object+0x14u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(object+0x13cu,pick(5)?std::uint32_t(rng()):0xffffffffu);m.put(object+0x1fcu,pick(5)?pick(3):0xffffu,2);
    }
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x200u*k);}
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=definitions+0x200u*k;at<definitions+0x200u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definitions+0x200u*k+0x1ccu,seats);}
    for(std::uint32_t k=0;k<3;++k)m.put(seats+0xb0u*k,(std::uint32_t(rng())&~0x10u)|(pick(4)?0u:0x10u));
    for(std::uint32_t k=0;k<8;++k)m.put(axes+4u*k,pick(3)?bits_of(float(pick(3))):bits_of(float(int(pick(41))-20)*0.125f));
    for(std::uint32_t at=outputs;at<outputs+0x100u;at+=4)m.put(at,bits_of(float(int(pick(41))-20)*0.25f));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,outputs+0x10u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions,seats,axes,outputs};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_cdcf0(v);},[](auto &v){original::run_bbff0(v);},
        [](auto &v){original::run_30bd0(v);},[](auto &v){original::run_144210(v);});
}
void reference(WorldState &s){original::run_cdcf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEATED_VECTOR");
    std::mt19937 rng(0xcdcf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
