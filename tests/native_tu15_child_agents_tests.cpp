// Native TU1.5 child agents (2F20F0) against the unmodified translated body
// of the map world support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children (through the unit's dispatch: 2E8FC0, 2E8CF0, 2E12A0 and, at
// the targets the authored vtables and dispatcher name, the other's type and
// the agents) are hash-driven stubs of their complete entry ABIs: the
// wrapper's constructor fills it (its owned bit set or not, RET 4), the type
// queries answer types 0 to 3, each agent hashes its arguments, the stack
// body and the copied motion (the caller cleaning), and 2E12A0 returns with
// RET 4; the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX (the other dispatcher), ESI, EDI (the other
// list) or EBP (another authored frame) change. Fixtures author lists of -1
// to 4 children, finite, infinite and NaN motions and offsets, motions
// aligned and not, and an exception list at fs_base; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_child_agents.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_child_agents_detail;
namespace {
constexpr std::uint32_t bodies=0x10000000u,lists=0x10010000u,motions=0x10020000u,others=0x10030000u,vtables=0x10040000u,
    dispatchers=0x10050000u,frames=0x10060000u,teb=0x10070000u,shapes=0x10080000u,scratch=0x10600000u;
constexpr std::uint32_t types[]{0x2f8000u,0x2f8100u},agents[]{0x2f9000u,0x2f9100u,0x2f9200u,0x2f9300u};
bool agent_target(std::uint32_t t){for(auto a:agents)if(a==t)return true;return false;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const bool agent=agent_target(target);
    if(agent) {
        for(std::uint32_t k=1;k<=4;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
        const auto body=r(s.memory,s.gp[4]+4u);
        for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,body+4u*k));
        const auto motion=r(s.memory,body+8u);
        for(std::uint32_t k=0;k<24;++k)h=mix(h,r(s.memory,motion+4u*k));
        h=mix(h,r(s.memory,r(s.memory,body)+0xcu));
    }
    if(target==0x2e8fc0u||target==0x2e12a0u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x2e8fc0u) {
        w(s.memory,s.observe,0x2e8fc1u,s.gp[1],4,0x45e000u+std::uint32_t(rng()%0x100)*4u);
        w(s.memory,s.observe,0x2e8fc2u,s.gp[1]+4u,4,std::uint32_t(rng())&(rng()%2?0xffffffffu:0xffff7fffu));
        w(s.memory,s.observe,0x2e8fc3u,s.gp[1]+0xcu,4,std::uint32_t(rng()));
    }
    if(agent)w(s.memory,s.observe,target+1u,r(s.memory,s.gp[4]+8u)+4u,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==types[0]||target==types[1]||target==0x2e8cf0u)s.gp[0]=rng()%41?std::uint32_t(rng()%4):std::uint32_t(rng()%4)|0x100u;
    if(policy&2u && rng()%11==0)s.gp[3]=dispatchers+0x2400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%11==0)s.gp[5]=frames+0x80u+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%11==0)s.gp[7]=lists+0x100u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=(target==0x2e8fc0u||target==0x2e12a0u)?8u:4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x2e8fc0u && target!=0x2e8cf0u && target!=0x2e12a0u && target!=types[0] && target!=types[1] && !agent_target(target))
        throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t component(std::mt19937 &rng) {
    switch(rng()%14) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    case 2:return 0u;
    default:{float f=float(int(rng()%2001)-1000)/37.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(teb,0x20030000u+4u*pick(64));
    for(std::uint32_t at=bodies;at<bodies+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k){m.put(bodies+0x40u*k,lists+0x100u*pick(2));m.put(bodies+0x40u*k+8u,motions+0x80u*pick(4)+(pick(6)?0u:4u*pick(4)));}
    for(std::uint32_t at=lists;at<lists+0x200u;at+=4)m.put(at,component(rng));
    for(std::uint32_t k=0;k<2;++k) {
        m.put(lists+0x100u*k+0xcu,pick(8)?pick(5):0u-pick(2));
        for(std::uint32_t j=0;j<8;++j)m.put(lists+0x100u*k+0x10u+0x10u*j+0xcu,shapes+0x10u*pick(8));
    }
    for(std::uint32_t at=shapes;at<shapes+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=motions;at<motions+0x240u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=others;at<others+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(others+0x40u*k,others+0x20u+0x40u*k);
    for(std::uint32_t k=0;k<2;++k)m.put(others+0x20u+0x40u*k,vtables+0x40u*pick(2));
    for(std::uint32_t at=vtables;at<vtables+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtables+0x14u,types[0]);m.put(vtables+0x54u,types[1]);
    for(std::uint32_t d=0;d<2;++d) {
        const auto base=dispatchers+0x2400u*d;
        m.put(base,base+0x10u*pick(2));
        for(std::uint32_t at=base+0x2180u;at<base+0x2400u;at+=4)m.put(at,agents[pick(4)]);
    }
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=1;k<=2;++k) {
        const auto f=frames+0x80u*k;
        m.put(f+8u,bodies+0x40u*pick(2));m.put(f+0xcu,others+0x40u*pick(2));m.put(f+0x10u,dispatchers+0x2400u*pick(2));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x140u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x2f0000u+pick(0x100));
    m.put(stack+4u,bodies+0x40u*pick(2));m.put(stack+8u,others+0x40u*pick(2));m.put(stack+12u,dispatchers+0x2400u*pick(2));
    m.put(stack+16u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.fs_base=teb;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{bodies,lists,motions,others,vtables,dispatchers,teb};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_2f20f0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_2f20f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHILD_AGENTS");
    std::mt19937 rng(0x2f20f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
