// Native TU1.5 round end (15B740) against the unmodified translated body of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 15B4D0, 15B620 and 161530 (through the dispatch) are hash-driven stubs of
// their complete entry ABIs: 15B4D0 answers a player count (AX), 15B620
// fills its three outputs (the last living player or NONE, the living teams,
// the teams), 161530 answers AL (0 half the time); each writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// sessions with and without a variant record, players' teams, game modes 1,
// 9 and others, the team option and the session's team equal or not, and
// teams_down NULL or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_round_end.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_round_end_detail;
namespace {
constexpr std::uint32_t session=0x10000000u,players=0x10010000u,datums=0x10020000u,game=0x10030000u,winners=0x10040000u,
    outs=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x15b620u)for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x15b620u) {
        w(s.memory,s.observe,0x15b621u,r(s.memory,s.gp[4]+4u),4,rng()%4?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu);
        w(s.memory,s.observe,0x15b622u,r(s.memory,s.gp[4]+8u),4,rng()%3?std::uint32_t(rng()%3):0u-std::uint32_t(rng()%2));
        w(s.memory,s.observe,0x15b623u,r(s.memory,s.gp[4]+12u),4,rng()%3?std::uint32_t(rng()%3):0u-std::uint32_t(rng()%2));
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x15b4d0u)s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%5?std::uint32_t(rng()%4):0xffffu);
    if(target==0x161530u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng())&0xffffff01u;
    if(policy&2u && rng()%13==0)s.gp[5]=session;
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=winners+4u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x15b620u?16u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_15b4d0(WorldState &s){stub(s,0x15b4d0u);}
static void run_15b620(WorldState &s){stub(s,0x15b620u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x161530u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f63b8u,session);m.put(0x4f54f4u,players);m.put(0x4f3218u,game);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)?std::uint32_t(rng())|1u:0u);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));m.put(session+0x2f0u,pick(3)?pick(3):std::uint32_t(rng()));
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,datums);
    for(std::uint32_t at=datums;at<datums+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(datums+0x21cu*k+0xc0u,pick(4)?pick(3):0xffu,1);
    for(std::uint32_t at=game;at<game+0x250u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t modes[]{1u,9u,2u,7u};
    m.put(game+0x180u,modes[pick(4)]);m.put(game+0x240u,pick(3)?1u:pick(2));
    for(std::uint32_t at=winners;at<winners+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=outs;at<outs+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,pick(4)?outs+pick(8):0u);
    random_abi(rng,c,stack);
    c.abi.gp[7]=winners+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{session,players,datums,game,winners,outs};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15b740(v);},[](auto &v){original::run_15b4d0(v);},
        [](auto &v){original::run_15b620(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_15b740(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROUND_END");
    std::mt19937 rng(0x15b740u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
