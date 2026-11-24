// Native TU1.5 object placement (B95D0) against the unmodified translated
// bodies of the death cycle and physics live support units, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 10E340, B9460, BA570 and B9390 (direct or through the unit's dispatch, as
// each unit calls them) are hash-driven stubs of their complete entry ABIs
// and arguments: B9460 answers AL, 10E340 rewrites the copies; all write
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author every combination of the three vectors given
// or NULL, game modes 4 and others, and objects with and without a link;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_placement.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_placement_detail;
namespace {
constexpr std::uint32_t vectors=0x10000000u,game=0x10010000u,headers=0x10020000u,datums=0x10030000u,objects=0x10040000u,
    scratch=0x10600000u;
std::uint32_t arguments_of(std::uint32_t target){return target==0x10e340u?2u:target==0xb9460u?9u:target==0xba570u?1u:0u;}
void stub(WorldState &s,std::uint32_t target) {
    const auto arguments=arguments_of(target);
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x10e340u && r(s.memory,s.gp[4]+4u) && rng()%2)w(s.memory,s.observe,target,r(s.memory,s.gp[4]+4u)+4u*std::uint32_t(rng()%3),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?1u:0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
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
static void run_10e340(WorldState &s){stub(s,0x10e340u);}
static void run_b9460(WorldState &s){stub(s,0xb9460u);}
static void run_ba570(WorldState &s){stub(s,0xba570u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xb9460u && target!=0xba570u && target!=0xb9390u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target,std::uint32_t){stub(s,target);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f3218u,game);m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=vectors;at<vectors+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=game;at<game+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0xcu,pick(2)?4u:pick(8),1);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x100u*k);}
    for(std::uint32_t at=objects;at<objects+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(objects+0x100u*k+0xd4u,pick(2)?0xffffffffu:std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb80000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    m.put(stack+8u,std::uint32_t(rng()));m.put(stack+12u,std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(2)?vectors:0u;c.abi.gp[1]=pick(2)?vectors+0x10u:0u;c.abi.gp[2]=pick(2)?vectors+0x20u:0u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,game,headers,datums,objects};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_b95d0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::step(v,target,site);});
}
void reference(WorldState &s){original::run_b95d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_PLACEMENT");
    std::mt19937 rng(0xb95d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
