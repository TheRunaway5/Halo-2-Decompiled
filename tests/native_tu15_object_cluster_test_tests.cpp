// Native TU1.5 object cluster test (B8D90) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 10DDE0 is a hash-driven stub of its complete entry ABI and arguments: AL
// answers or not, it may write the result byte through EBX and pops the
// cluster set, the caller-saved registers, XMM, flags and (by policy) MXCSR
// are random, and by policy a callee-saved register moves or the stack is
// left unbalanced. Fixtures author headers (machines and other types,
// clusters NONE, negative, ordinary), objects with and without the always
// flag and cluster bit sets; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_cluster_test.hpp"
using namespace frame_misc_harness;
namespace cluster_test=halo2::native_object_cluster_test_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,clusters=0x10200000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(rng()%2)w(s.memory,s.observe,target+2u,s.gp[3],1,std::uint32_t(rng()%3));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[rng()%2?6:5]=rng()%2?header_data+12u*std::uint32_t(rng()%4):clusters+4u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_10dde0(WorldState &s){stub(s,0x10dde0u);}
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
        const auto object=objects+0x100u*k;
        const auto header=header_data+12u*k;
        m.put(header,std::uint32_t(rng()));
        static constexpr std::uint32_t types[]{7u,7u,0u,1u,2u,15u,39u,8u};
        m.put(header+3u,types[pick(8)],1);
        m.put(header+4u,pick(4)==0?0xffffu:pick(5)==0?0x8000u|pick(0x100):pick(64),2);
        m.put(header+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+4u,std::uint32_t(rng())&(pick(4)?~2u:~0u));
    }
    for(std::uint32_t at=clusters-0x400u;at<clusters+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));m.put(stack+4u,clusters);
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,clusters};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return cluster_test::bridge(s,[](auto &v){original::native_frame_misc_reference_b8d90(v);},[](auto &v){original::run_10dde0(v);});
}
void reference(WorldState &s){original::run_b8d90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_CLUSTER_TEST");
    std::mt19937 rng(0xb8d90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,cluster_test::coverage().load(),unsigned(cluster_test::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
