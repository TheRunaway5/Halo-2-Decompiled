// Native TU1.5 respawn wait (1601B0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1A8E50 and 160570 are hash-driven stubs of their complete entry ABIs (and
// the iterator's state): 1A8E50 walks the authored players from the state's
// index, 160570 sets a player aside or not; each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author living limits (none, reached, not), players alive and dead
// on teams, team and free-for-all games, tied orders, and one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_respawn_wait.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_respawn_wait_detail;
namespace {
constexpr std::uint32_t game=0x10000000u,players=0x10010000u,data=0x10020000u,engine_globals=0x10030000u,scratch=0x10600000u;
std::uint32_t player_count=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x1a8e50u)for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,s.gp[6]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t answer=std::uint32_t(rng())&0xffffff00u;
    if(target==0x1a8e50u) {
        const auto state=s.gp[6];
        const auto next=r(s.memory,state+0xcu)+1u;
        if(next<player_count) {
            w(s.memory,s.observe,0x1a8e51u,state,4,data+0x21cu*next);
            w(s.memory,s.observe,0x1a8e52u,state+8u,4,(std::uint32_t(rng())&0xffff0000u)|next);
            w(s.memory,s.observe,0x1a8e53u,state+0xcu,4,next);
            answer|=1u+std::uint32_t(rng()%255);
        }
    } else if(rng()%4==0)answer|=1u+std::uint32_t(rng()%255);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?players:0xffffffffu;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=data+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=data+0x21cu*std::uint32_t(rng()%4);
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
static void run_1a8e50(WorldState &s){stub(s,0x1a8e50u);}
static void run_160570(WorldState &s){stub(s,0x160570u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    player_count=1u+pick(6);
    m.put(0x4f3218u,game);m.put(0x4f54f4u,players);m.put(0x4f63b8u,engine_globals);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(3)?std::uint32_t(rng()):0u);
    for(std::uint32_t at=game;at<game+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0x1b4u,pick(8)?pick(5):0u-pick(3));m.put(game+0x184u,std::uint32_t(rng())&0xfeu|pick(2),1);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t k=0;k<6;++k) {
        const auto player=data+0x21cu*k;
        for(std::uint32_t at=player;at<player+0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(player+0x2cu,pick(2)?0xffffffffu:std::uint32_t(rng()));
        m.put(player+0xc0u,pick(3),1);m.put(player+0x1b4u,pick(4)?pick(3):std::uint32_t(rng()));
    }
    for(std::uint32_t at=engine_globals;at<engine_globals+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(engine_globals+0xc14u,pick(4));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(player_count);
    if(pick(3))m.put(data+0x21cu*(c.abi.gp[0]&0xffffu)+0x2cu,0xffffffffu);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{game,players,data,engine_globals};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1601b0(v);},[](auto &v){original::run_1a8e50(v);},
        [](auto &v){original::run_160570(v);});
}
void reference(WorldState &s){original::run_1601b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RESPAWN_WAIT");
    std::mt19937 rng(0x1601b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
