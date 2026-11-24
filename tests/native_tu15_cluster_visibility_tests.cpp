// Native TU1.5 cluster visibility (13B610) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1532A0, 140EC0 (direct) and 16FD40 (behind the unit's dispatch) are
// hash-driven stubs of their complete entry ABIs (and the arguments they are
// handed): 16FD40 answers a cluster or none; each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author game kind 3 and others, seats in use or none, players'
// clusters, the second cluster's switches, both string directions, and one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_cluster_visibility.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_cluster_visibility_detail;
namespace {
constexpr std::uint32_t bsp=0x10000000u,bits=0x10010000u,game=0x10020000u,seats=0x10030000u,sets=0x10040000u,second=0x10050000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=2;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x16fd40u?(std::uint32_t(rng())&0xffff0000u)|(rng()%4?std::uint32_t(rng()%4):0xffffu):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=bsp+0x100u;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=sets+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=game+0x2000u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x1532a0u?12u:target==0x140ec0u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1532a0(WorldState &s){stub(s,0x1532a0u);}
static void run_140ec0(WorldState &s){stub(s,0x140ec0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16fd40u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecc18u,bsp);m.put(0x4f3218u,game);m.put(0x4f54f0u,seats);m.put(0x474c24u,pick(4)?pick(4):0xffffu,2);m.put(0x4f5504u,second);
    // The BSP, and a second one a stub may leave in EBX.
    for(std::uint32_t at=bsp;at<bsp+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(bsp+0x58u,bits);m.put(bsp+0x9cu,pick(100));m.put(bsp+0x158u,bits+0x80u);m.put(bsp+0x19cu,pick(100));
    for(std::uint32_t at=bits;at<bits+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k){const auto g=game+0x2000u*k;for(std::uint32_t at=g;at<g+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(g+8u,pick(3)?3u:pick(5));m.put(g+0x11f8u,pick(2),1);}
    for(std::uint32_t at=seats;at<seats+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(seats+0xcu+4u*k,pick(3)?0xffffffffu:std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(0x4f655cu+0x358u*k+0x10u,pick(4)?pick(4):0xffffu,2);
    m.put(0x10u,pick(3),2);
    m.put(second,pick(2),1);
    for(std::uint32_t at=sets;at<sets+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=sets+0x40u*pick(2);
    if(rng()%4==0){c.abi.eflags|=0x400u;c.abi.gp[6]=sets+0x40u;}else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{bsp,bits,game,seats,sets,second};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_13b610(v);},[](auto &v){original::run_1532a0(v);},
        [](auto &v){original::run_140ec0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_13b610(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CLUSTER_VISIBILITY");
    std::mt19937 rng(0x13b610u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
