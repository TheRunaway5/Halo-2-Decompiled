// Native TU1.5 multiplayer game engine per-player update (1EEE80) against
// the unmodified translated body of the multiplayer engines unit, extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Every child (1EFD60, 1F0670, 1F1030 and the dispatched 1494F0) is a
// genuine stand-in whose effects are a hash of its complete entry ABI, its
// return address and its argument words: AL/AX results, ECX/EDX, occasional
// EBX/EBP/ESI/EDI clobbers and stack imbalance, flags, XMM, MXCSR sticky
// bits and stores into the player's engine record (slot words, the current
// slot and its counter), so every re-read after a call is exercised. The
// fixtures author the player, campaign, engine, roster and timer globals and
// the jump/byte tables: no player, disabled engines, campaign suppression,
// every countdown byte, every slot state, empty/expired slots, roster
// bounds, negative engine indices, and corrupted tables (unmapped and
// unexpected targets), plus fault injection (one region removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_engine_player_update.hpp"
using namespace frame_misc_harness;
namespace engine=halo2::native_engine_player_update_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,player_records=0x10010000u,globals=0x10100000u,engines=0x10200000u,
    roster=0x10300000u,roster_records=0x10310000u,timer=0x10400000u,scratch=0x10600000u,stack=0x20010000u;
std::uint32_t record_base{};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    const unsigned words=target==0x1f1030u?4u:(target==0x1f0670u?0u:1u);
    for(unsigned k=1;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1f1031u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(rng()%3==0) {
        const auto roll=rng()%8;
        if(roll==0)w(s.memory,s.observe,0x1f1032u,record_base+0x80u,4,std::uint32_t(rng()%40)-4u);
        else if(roll==1)w(s.memory,s.observe,0x1f1033u,record_base+0x84u,4,std::uint32_t(rng()));
        else if(roll==2 || roll==3) {
            // The current slot or the offered one (EDX), which becomes
            // current when picked: any state with a small or negative timer,
            // so the state cases see wrapped timers.
            const auto slot=roll==2?r(s.memory,record_base+0x80u):s.gp[2];
            if(slot<32u) {
                w(s.memory,s.observe,0x1f1035u,record_base+slot*4u,2,std::uint32_t(rng()%5));
                w(s.memory,s.observe,0x1f1036u,record_base+slot*4u+2u,2,rng()%3?std::uint32_t(rng()%3):0x8000u|std::uint32_t(rng()%4));
            }
        }
        else w(s.memory,s.observe,0x1f1034u,record_base+std::uint32_t(rng()%32)*4u+std::uint32_t(rng()%2)*2u,2,
               std::uint32_t(rng()%6));
    }
    const auto al=std::uint32_t(rng()%3==0?0u:1u+rng()%255);
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|((policy&1u)?al:(rng()%2?al:0u));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng()%64);}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
constexpr std::uint32_t mapped[]{0x1ef036u,0x1ef055u,0x1ef0c1u,0x1ef114u,0x1ef187u,0x1ef1feu,0x1ef23au,0x1efe29u};
std::uint32_t branch_map(std::uint32_t p,SourcePhysicsContactImage image) {
    if(image==SourcePhysicsContactImage::MapPack11)return p;
    for(const auto v:mapped)if(v==p)return p;
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){return branch_map(p,image);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target);}
static void run_1efd60(WorldState &s){stub(s,0x1efd60u);}
static void run_1f0670(WorldState &s){stub(s,0x1f0670u);}
static void run_1f1030(WorldState &s){stub(s,0x1f1030u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4f54f4u,players);m.put(players+0x44u,player_records);
    m.put(0x4f3218u,globals);m.put(0x52be64u,engines);
    m.put(0x51df94u,shape==3?0u:roster);m.put(0x51df54u,timer);
    for(std::uint32_t at=players;at<players+0x44u;at+=4)m.put(at,std::uint32_t(rng()));
    // Engine index: mostly 0..7, sometimes no player (-1) or negative.
    const std::uint32_t player=pick(16);
    std::int32_t index=std::int32_t(pick(8));
    if(shape==1)index=-1;
    if(shape==2)index=-2-std::int32_t(pick(2));
    const std::uint32_t handle=(shape==4?0x8000u:0u)|player;
    for(std::uint32_t k=0;k<16;++k)for(std::uint32_t at=0;at<0x21cu;at+=4)m.put(player_records+k*0x21cu+at,std::uint32_t(rng()));
    m.put(player_records+handle*0x21cu+0x28u,std::uint32_t(index),2);
    // Campaign globals (+8 == 1) with the +0x132 suppression word.
    for(std::uint32_t at=globals;at<globals+0x140u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(globals+8u,shape==5||shape==6?1u:(pick(5)==0?1u:pick(4)+2u));
    static constexpr std::uint32_t words[]{0,1,2,3,4,0x10002u,0xffff0003u};
    m.put(globals+0x132u,words[pick(7)]&0xffffu,2);
    // Engine records, including those of negative indices.
    for(std::uint32_t at=engines-0x400u;at<engines+8u*0x1b0u+0x6d0u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(engines+0x6c5u,shape==7?0u:1u+pick(255),1);
    record_base=engines+std::uint32_t(index)*0x1b0u;
    if(index>=0 || shape==2) {
        const auto rec=record_base;
        for(std::uint32_t slot=0;slot<32;++slot) {
            std::uint32_t state=pick(6);
            if(shape==8)state=pick(2)?0u:4u;
            m.put(rec+slot*4u,state,2);
            std::uint32_t timer_word=pick(6);
            if(pick(5)==0)timer_word=0x8000u|pick(0x8000u);
            if(pick(7)==0)timer_word=0x7fffu;
            m.put(rec+slot*4u+2u,timer_word,2);
        }
        std::uint32_t current=pick(40)-4u;
        if(shape==9)current=0xffffffffu;
        m.put(rec+0x80u,current);
        for(std::uint32_t at=0x195u;at<=0x19bu;++at)m.put(rec+at,pick(3)==0?0u:pick(3)==0?1u:pick(256),1);
        m.put(rec+0x170u,shape==10?0xffffffffu:std::uint32_t(rng()));
        m.put(rec+0x174u,shape==10?0xffffffffu:std::uint32_t(rng()));
    }
    // Roster: 20-byte records; count around the slot range.
    std::uint32_t count=pick(40);
    if(shape==11)count=0x80000000u|pick(4);
    m.put(roster+0x420u,count);m.put(roster+0x424u,roster_records);
    for(std::uint32_t at=roster;at<roster+0x420u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=roster_records;at<roster_records+40u*20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=timer;at<timer+8u;at+=4)m.put(at,std::uint32_t(rng()));
    // Jump tables (state 0..4 and the award selector) and the byte table.
    static constexpr std::uint32_t states[]{0x1ef036u,0x1ef055u,0x1ef0c1u,0x1ef114u,0x1ef187u};
    for(std::uint32_t k=0;k<5;++k)m.put(0x1ef260u+k*4u,states[k]);
    m.put(0x1ef274u,0x1ef1feu);m.put(0x1ef278u,0x1ef23au);
    for(std::uint32_t k=0;k<16;++k)m.put(0x1ef27cu+k,pick(2),1);
    for(std::uint32_t at=0x1ef28cu;at<0x1ef2a0u;at+=4)m.put(at,std::uint32_t(rng()));
    if(shape==12) {
        const auto roll=pick(4);
        if(roll==0)m.put(0x1ef260u+pick(5)*4u,0x1ef1feu);            // mapped, not a state case
        else if(roll==1)m.put(0x1ef260u+pick(5)*4u,0x1ef037u);       // unmapped
        else if(roll==2)m.put(0x1ef274u+pick(2)*4u,0x1ef036u);       // mapped, not an award case
        else m.put(0x1ef27cu+pick(16),2u+pick(3),1);                 // into the byte table
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1f4000u+pick(0x40));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|handle;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,player_records,globals,engines,roster,roster_records,timer,0x1ef260u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return engine::bridge(s,[](auto &v){original::native_frame_misc_reference_1eee80(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);},
        [](auto &v){original::run_1efd60(v);},[](auto &v){original::run_1f0670(v);},[](auto &v){original::run_1f1030(v);});
}
void reference(WorldState &s){original::run_1eee80(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ENGINE_PLAYER_UPDATE");
    std::mt19937 rng(0x1eee80u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,engine::coverage().load(),unsigned(engine::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
