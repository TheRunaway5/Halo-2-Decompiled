// Native TU1.5 root point (BBF20) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BBE00 is a hash-driven stub of its complete entry ABI (ESI the point): it
// writes the point, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy ESI changes or the stack is left
// unbalanced. Fixtures author parent chains (forward only, as the game's
// are), roots with and without a cluster, cluster entry bytes negative,
// within and past the count, and points over the source; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_root_point.hpp"
using namespace frame_misc_harness;
namespace point=halo2::native_root_point_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,clusters=0x10180000u,
    cluster_data=0x10190000u,lists=0x10200000u,links=0x10300000u,points=0x10400000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=6;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:s.gp[6]+4u*std::uint32_t(rng()%3),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[6]=points+4u*std::uint32_t(rng()%8);
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
static void run_bbe00(WorldState &s){stub(s,0xbbe00u);}
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
    std::array<std::uint32_t,object_count> handles{};
    for(std::uint32_t k=0;k<object_count;++k)handles[k]=(std::uint32_t(rng()&0x7fffu)<<16)|k;
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,k+1<object_count && pick(3)==0?handles[k+1+pick(object_count-k-1)]:0xffffffffu);
        m.put(object+0xb4u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
    }
    // Index 0xFFFF (the handle NONE) reads the last header entry: author it too.
    m.put(header_data+12u*0xffffu+8u,objects+0x400u);
    for(std::uint32_t k=0;k<4;++k) {
        const auto record=cluster_data+0xa0u*k;
        for(std::uint32_t at=record;at<record+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x18u,pick(5)==0?0x80u+pick(0x80):pick(5),1);
        m.put(record+0x74u,1u+pick(4));
        m.put(record+0x70u,lists+0x1000u*k);
        for(std::uint32_t e=0;e<5;++e) {
            m.put(lists+0x1000u*k+0x60u*e+0x40u,links+0x100u*k+4u*pick(4));
        }
        for(std::uint32_t at=links+0x100u*k;at<links+0x100u*k+0x60u;at+=4)m.put(at,points+0x100u*k+4u*pick(8));
    }
    for(std::uint32_t at=points;at<points+0x500u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(8)==0?0xffffffffu:handles[pick(object_count)];
    c.abi.gp[1]=shape==7?points+0x70u+4u*pick(4):points+0x480u+4u*pick(8);   // the point over a source
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u,clusters,cluster_data,lists+0x1000u,links,points};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s){return point::bridge(s,[](auto &v){original::native_frame_misc_reference_bbf20(v);},[](auto &v){original::run_bbe00(v);});}
void reference(WorldState &s){original::run_bbf20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROOT_POINT");
    std::mt19937 rng(0xbbf20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,point::coverage().load(),unsigned(point::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
