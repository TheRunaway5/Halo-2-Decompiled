// Native TU1.5 cluster notice (1CD620) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1DC270 is a hash-driven stub of its complete entry ABI and argument: it
// writes scratch or the cluster and pops its argument, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy ESI
// changes or the stack is left unbalanced. Fixtures author objects outside
// and inside clusters with every combination of flag bits 1, 7 and 8 and of
// the two byte arguments; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_cluster_notify.hpp"
using namespace frame_misc_harness;
namespace notice=halo2::native_cluster_notify_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,clusters=0x10180000u,
    cluster_data=0x10190000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:cluster_data+4u*std::uint32_t(rng()%32),4,
      std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[6]=std::uint32_t(rng());
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
static void run_1dc270(WorldState &s){stub(s,0x1dc270u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x52be5cu,clusters);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t at=clusters;at<clusters+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(clusters+0x44u,cluster_data);
    for(std::uint32_t at=cluster_data;at<cluster_data+0xa0u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0xb4u,pick(5)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        // Flag bits 1, 7 and 8 in every combination, the rest random.
        const auto bits=pick(8);
        m.put(cluster_data+0xa0u*k+4u,(std::uint32_t(rng())&~0x182u)|((bits&1u)<<1)|((bits&2u)<<6)|((bits&4u)<<6));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&~0xffu)|(pick(2)?0u:1u+pick(255)));
    m.put(stack+8u,(std::uint32_t(rng())&~0xffu)|(pick(2)?0u:1u+pick(255)));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u*2u,clusters,cluster_data};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return notice::bridge(s,[](auto &v){original::native_frame_misc_reference_1cd620(v);},[](auto &v){original::run_1dc270(v);});}
void reference(WorldState &s){original::run_1cd620(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CLUSTER_NOTIFY");
    std::mt19937 rng(0x1cd620u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,notice::coverage().load(),unsigned(notice::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
