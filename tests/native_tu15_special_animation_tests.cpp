// Native TU1.5 special animation query (C9590) against the unmodified
// translated body of the physics live support unit (the validator repeats it
// with the AI frame unit's), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C9550 is a hash-driven stub of its complete entry ABI: it answers AL zero
// or not, writes scratch, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy ESI moves to another object or the
// stack is left unbalanced. Fixtures author objects whose animation (signed
// block offsets) is each special code or another; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_special_animation.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_special_animation_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%3?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%7==0)s.gp[6]=(std::uint32_t(rng())<<16)|std::uint32_t(rng()%4);
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
static void run_c9550(WorldState &s){stub(s,0xc9550u);}
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
    static constexpr std::uint32_t codes[]{0x080000c4u,0x0400004au,0x050000c3u};
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x800u*k+0x204u+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x204u;at<object+0x5f0u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto offset=pick(4)==0?0x10000u-0x100u:0x300u;
        m.put(object+0x12au,offset,2);
        const auto block=object+std::uint32_t(std::int32_t(std::int16_t(offset)));
        m.put(block+0x7cu,pick(4)?codes[pick(3)]+(pick(4)==0?1u:0u):std::uint32_t(rng()));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u*2u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_c9590(v);},[](auto &v){original::run_c9550(v);});}
void reference(WorldState &s){original::run_c9590(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SMALL_CALLS");
    std::mt19937 rng(0xc9590u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
