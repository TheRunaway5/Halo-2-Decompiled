// Native TU1.5 player activation (14FDC0) against the unmodified translated
// body of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 69AD0, B7E50 and the variant's method (through the dispatch) are
// hash-driven stubs of their complete entry ABIs (and arguments): 69AD0
// answers AL (0, 1 or another byte), each writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// game kind 4 and others, players in use or not (in-use bits past 32 too),
// passed over or not, active bits set or clear, sessions with and without a
// variant record and network slots NONE or not; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_player_activation.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_activation_detail;
namespace {
constexpr std::uint32_t game=0x10000000u,players=0x10010000u,data=0x10020000u,bitmap=0x10030000u,session=0x10040000u,
    variant=0x10050000u,vtable=0x10060000u,scratch=0x10600000u;
constexpr std::uint32_t method=0x170000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0xb7e50u?2u:target==method?1u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    if(target==0x69ad0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%5?std::uint32_t(rng()%2):std::uint32_t(rng()%256));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=players;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%6)-1u;
    if(policy&2u && rng()%13==0)s.gp[6]=data+0x40u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%8);
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
static void run_69ad0(WorldState &s){stub(s,0x69ad0u);}
static void run_b7e50(WorldState &s){stub(s,0xb7e50u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=method)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f3218u,game);m.put(0x4f54f4u,players);m.put(0x4f63b8u,session);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)?variant+0x10u*k:0u);
    for(std::uint32_t at=game;at<game+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0xcu,pick(6)?pick(4):4u,1);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto count=pick(6)?1u+pick(40):pick(2)?0u:0x80000000u+pick(4);
    m.put(players+0x24u,0x40u);m.put(players+0x38u,count);m.put(players+0x44u,data);m.put(players+0x48u,bitmap);
    for(std::uint32_t at=bitmap;at<bitmap+8u;at+=4)m.put(at,std::uint32_t(rng())&std::uint32_t(rng()));
    for(std::uint32_t at=data;at<data+0x40u*48u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<48;++k)m.put(data+0x40u*k+2u,(std::uint32_t(rng())&~2u)|(pick(5)?0u:2u),2);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t k=0;k<48;++k)m.put(session+0x2cu+4u*k,pick(4)?pick(16):0xffffffffu);
    for(std::uint32_t k=0;k<4;++k){m.put(variant+0x10u*k,vtable);m.put(variant+0x10u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t at=vtable;at<vtable+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtable+0x18u,method);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{game,players,data,bitmap,session,variant,vtable};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_14fdc0(v);},[](auto &v){original::run_69ad0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},[](auto &v){original::run_b7e50(v);});
}
void reference(WorldState &s){original::run_14fdc0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_ACTIVATION");
    std::mt19937 rng(0x14fdc0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
