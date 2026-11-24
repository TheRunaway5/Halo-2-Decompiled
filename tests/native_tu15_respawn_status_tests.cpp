// Native TU1.5 respawn status (1618B0) against the unmodified translated
// body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The engines' method +0x94 and 160570 (through the dispatch) and 15BE80
// are hash-driven stubs of their complete entry ABIs: the method answers
// NONE or a code and now and then sets the override byte (RET 8), 160570
// answers AL, 15BE80 now and then writes the decided and second bytes (RET
// C); each writes scratch, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author sessions, engines, games and
// players reaching every status; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_respawn_status.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_respawn_status_detail;
namespace {
constexpr std::uint32_t session=0x10000000u,engines=0x10001000u,tables=0x10002000u,game=0x10003000u,players=0x10010000u,
    player_data=0x10011000u,clock=0x10020000u,scratch=0x10600000u,method_base=0x170000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const bool method=target>=method_base && target<method_base+4u;
    const unsigned args=method?2u:target==0x15be80u?3u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(method && rng()%3==0)w(s.memory,s.observe,target+1u,arg(2),1,1u+std::uint32_t(rng()%255));
    if(target==0x15be80u){if(rng()%2)w(s.memory,s.observe,target+1u,arg(2),1,rng()%2?0u:1u);if(rng()%2)w(s.memory,s.observe,target+2u,arg(3),1,rng()%2?0u:1u);}
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(method)s.gp[0]=rng()%2?0xffffffffu:std::uint32_t(rng()%40);
    if(target==0x160570u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0xffffffffu:std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=player_data+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_15be80(WorldState &s){stub(s,0x15be80u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x160570u && !(target>=method_base && target<method_base+4u))throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f63b8u,session);m.put(0x4f3218u,game);m.put(0x4f54f4u,players);m.put(0x51df54u,clock);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,engines+0x10u*k);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    static constexpr std::uint32_t modes[]{0xffffu,0u,1u,4u,5u,6u,0xfffeu,0x8000u};
    m.put(session+0xe0u,modes[pick(8)],2);
    for(std::uint32_t k=0;k<4;++k){m.put(engines+0x10u*k,tables+0x100u*k);m.put(engines+0x10u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t at=tables;at<tables+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(tables+0x100u*k+0x94u,method_base+k);
    for(std::uint32_t at=game;at<game+0x1130u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0x1128u,chance(4)?1u+pick(255):0u,1);m.put(game+0x1b8u,chance(2)?0u:std::uint32_t(rng()));m.put(game+0x18cu,chance(2)?0u:std::uint32_t(rng()));
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_data);
    for(std::uint32_t at=player_data;at<player_data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    // The clock: ticks per second and the time; the session stamp within or beyond three seconds.
    const auto ticks=chance(6)?std::uint32_t(0xfff0u+pick(16)):1u+pick(40);
    const auto now=std::uint32_t(rng()%100000u);
    m.put(clock,std::uint32_t(rng()));m.put(clock+2u,ticks,2);m.put(clock+4u,std::uint32_t(rng()));m.put(clock+8u,now);m.put(clock+12u,std::uint32_t(rng()));
    m.put(session+0x70u,chance(4)?now+pick(50):now-pick(200));
    for(std::uint32_t k=0;k<4;++k) {
        const auto p=player_data+0x21cu*k;
        m.put(p+3u,chance(3)?0x40u:0u,1);
        m.put(p+0x2cu,chance(2)?0xffffffffu:std::uint32_t(rng()));
        m.put(p+0xc0u,chance(4)?0xffu:pick(8),1);
        m.put(p+0x164u,chance(2)?std::uint32_t(0u-1u-pick(9)):pick(9));
        m.put(p+0x1a8u,std::uint32_t(int(pick(200))-100),1);
        m.put(p+0x1acu,pick(4),2);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{session,engines,tables,game,players,player_data,clock};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1618b0(v);},
        [](auto &v,std::uint32_t target){if(target!=0x15be80u)throw std::logic_error("Unbound respawn status child");original::run_15be80(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1618b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RESPAWN_STATUS");
    std::mt19937 rng(0x1618b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
