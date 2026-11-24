// Native TU1.5 object type query chain (10DDE0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The queries are hash-driven stubs of their complete entry ABI (target,
// registers, the three pushed arguments): AL answers or not, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy a callee-saved register moves or the stack is left unbalanced.
// Fixtures author objects of every type, type definitions with chains of
// up to four types with and without queries; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_type_query_chain.hpp"
using namespace frame_misc_harness;
namespace query_chain=halo2::native_object_type_query_chain_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,definitions=0x10200000u,
    scratch=0x10600000u,queries=0x330000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?definitions+0x100u*std::uint32_t(rng()%8)+0x84u:std::uint32_t(rng());
    }
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target<queries || target>=queries+0x100u)throw std::runtime_error("Unbound dispatch target");
    stub(s,target);
}
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
    // Types 0..7 name definitions; each definition's chain lists up to four
    // definitions, each with a query (+0x48) or none.
    for(std::uint32_t k=0;k<8;++k) {
        m.put(0x474b90u+4u*k,definitions+0x100u*k);
        const auto definition=definitions+0x100u*k;
        for(std::uint32_t at=definition;at<definition+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x48u,pick(3)?queries+0x10u*pick(16):0u);
        const auto length=pick(5);
        for(std::uint32_t e=0;e<length;++e)m.put(definition+0x84u+4u*e,definitions+0x100u*pick(8));
        m.put(definition+0x84u+4u*length,0u);
    }
    for(std::uint32_t k=0;k<4;++k) {
        // Object addresses with a nonzero low byte show AL's clear.
        const auto object=objects+0x200u*k+0x14u+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0xaau,pick(8),1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,definitions,0x474000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return query_chain::bridge(s,[](auto &v){original::native_frame_misc_reference_10dde0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_10dde0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_TYPE_QUERY_CHAIN");
    std::mt19937 rng(0x10dde0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query_chain::coverage().load(),unsigned(query_chain::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
