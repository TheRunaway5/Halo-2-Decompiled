// Native TU1.5 seat pair check (D3A80) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 106710 and E8E10 are hash-driven stubs of their complete entry ABIs (EAX
// the occupant; the handle pushed and ECX 0x13): 106710 answers AL zero or
// not, E8E10 pops its argument, both write scratch or the object, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBP or ESI change or the stack is left unbalanced. Fixtures
// author objects whose seat bytes are unnamed (0xFF), named, -1 widened or
// negative; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_seat_pair.hpp"
using namespace frame_misc_harness;
namespace seat_pair=halo2::native_seat_pair_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:objects+0x800u*std::uint32_t(rng()%4)+0x210u,4,
      std::uint32_t(rng()));
    s.gp[0]=rng()%3?std::uint32_t(rng())|1u:std::uint32_t(rng())&~0xffu;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=(std::uint32_t(rng())<<16)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_106710(WorldState &s){stub(s,0x106710u,0);}
static void run_e8e10(WorldState &s){stub(s,0xe8e10u,1);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x800u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object+0x200u;at<object+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t seats[]{0xffu,0u,1u,2u,3u};
        m.put(object+0x212u,pick(6)==0?0xfeu:seats[pick(5)],1);
        m.put(object+0x213u,pick(6)==0?0xfdu:seats[pick(5)],1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())<<16)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return seat_pair::bridge(s,[](auto &v){original::native_frame_misc_reference_d3a80(v);},
        [](auto &v){original::run_106710(v);},[](auto &v){original::run_e8e10(v);});
}
void reference(WorldState &s){original::run_d3a80(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_PAIR");
    std::mt19937 rng(0xd3a80u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,seat_pair::coverage().load(),unsigned(seat_pair::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
