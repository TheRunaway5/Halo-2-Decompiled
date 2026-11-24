// Native TU1.5 cluster visibility update (13B6F0) against the unmodified
// translated body of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 13B610, 1D1E00, 140EC0 and 13B590 (direct) and the listeners' callbacks
// (through the unit's dispatch, at the targets the authored table names) are
// hash-driven stubs of their complete entry ABIs: 13B610 leaves the set or
// changes a word of it (RET 8), 1D1E00 fills the buffer and answers AL (RET
// 4), 140EC0 changes a word of its target or not (RET 4), 13B590 answers a
// cluster or NONE, each callback hashes its arguments and both sets (RET C);
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX, EBP (the other structure) or ESI (the other game, or a
// listener offset) change. Fixtures author cluster counts, listeners with
// either callback, either string direction; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_cluster_visibility_update.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_cluster_visibility_update_detail;
namespace {
constexpr std::uint32_t games=0x10000000u,structures=0x10010000u,bits=0x10020000u,scratch=0x10600000u;
constexpr std::uint32_t callbacks[]{0x2fc000u,0x2fc100u,0x2fc200u};
bool callback_target(std::uint32_t t){for(auto c:callbacks)if(c==t)return true;return false;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x13b610u?2u:target==0x1d1e00u||target==0x140ec0u?1u:callback_target(target)?3u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(callback_target(target))for(std::uint32_t k=0;k<16;++k){h=mix(h,r(s.memory,r(s.memory,s.gp[4]+4u)+4u*k));h=mix(h,r(s.memory,r(s.memory,s.gp[4]+8u)+4u*k));}
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x13b610u && rng()%2)w(s.memory,s.observe,0x13b611u,s.gp[6]+4u*std::uint32_t(rng()%16),4,std::uint32_t(rng()));
    if(target==0x1d1e00u)w(s.memory,s.observe,0x1d1e01u,s.gp[3]+4u*std::uint32_t(rng()%8),4,std::uint32_t(rng()));
    if(target==0x140ec0u && rng()%2)w(s.memory,s.observe,0x140ec1u,r(s.memory,s.gp[4]+4u)+4u*std::uint32_t(rng()%16),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1d1e00u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(target==0x13b590u)s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%3?std::uint32_t(rng()%4):0xffffu);
    if(policy&2u && rng()%13==0)s.gp[3]=games+0x2000u*std::uint32_t(rng()%2)+0x11b8u;
    if(policy&2u && rng()%13==0)s.gp[5]=structures+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=callback_target(target)?0x24u*std::uint32_t(rng()%0x44):games+0x2000u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u+4u*args;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_13b610(WorldState &s){stub(s,0x13b610u);}
static void run_1d1e00(WorldState &s){stub(s,0x1d1e00u);}
static void run_140ec0(WorldState &s){stub(s,0x140ec0u);}
static void run_13b590(WorldState &s){stub(s,0x13b590u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(!callback_target(target))throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f3218u,games+0x2000u*pick(2));m.put(0x4ecc18u,structures+0x100u*pick(2));
    for(std::uint32_t g=0;g<2;++g)for(std::uint32_t at=games+0x2000u*g+0x1100u;at<games+0x2000u*g+0x1240u;at+=4)m.put(at,pick(3)?0u:std::uint32_t(rng()));
    for(std::uint32_t at=structures;at<structures+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k){m.put(structures+0x100u*k+0x9cu,pick(8)?1u+pick(120):std::uint32_t(rng()%0x10000u)|(pick(8)?0u:0x10000u));m.put(structures+0x100u*k+0x58u,bits);}
    for(std::uint32_t at=bits;at<bits+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<0x44u;++k) {
        m.put(0x44aeecu+0x24u*k,pick(8)?0u:callbacks[pick(3)]);m.put(0x44aef0u+0x24u*k,pick(8)?0u:callbacks[pick(3)]);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x140u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130000u+pick(0x100));
    random_abi(rng,c,stack);
    if(rng()%4==0)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{games+0x1000u,structures,bits,0x44a000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_13b6f0(v);},[](auto &v){original::run_13b610(v);},
        [](auto &v){original::run_1d1e00(v);},[](auto &v){original::run_140ec0(v);},[](auto &v){original::run_13b590(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_13b6f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CLUSTER_VISIBILITY_UPDATE");
    std::mt19937 rng(0x13b6f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
