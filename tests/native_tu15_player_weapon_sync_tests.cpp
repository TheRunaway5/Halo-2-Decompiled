// Native TU1.5 player weapon sync (168CA1) against the unmodified translated
// body of the sandbox special unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 150990 (direct), 16B16A, CE230 and 16AD45 (through the unit's dispatch)
// and 169C46 (the World's required owner) are hash-driven stubs of their
// complete entry ABIs: 150990 answers a player or NONE, 16B16A rewrites the
// record's unit and state (RET 8), CE230 answers one of a few weapons,
// 16AD45 rewrites the slot (RET 8) and 169C46 returns with RET 4; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX (the slot), ESI (another record), EDI (another mask) or EBP
// (another authored frame) change. Fixtures author records whose unit and
// state match the player's or not, records without a unit, slots holding
// the unit's weapon or another; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_player_weapon_sync.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_weapon_sync_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,players=0x10020000u,player_datums=0x10021000u,headers=0x10030000u,object_datums=0x10031000u,
    units=0x10040000u,frames=0x10050000u,scratch=0x10600000u;
std::uint32_t weapon_of(std::mt19937_64 &rng){return rng()%5?0x10000u*std::uint32_t(rng()%3)+std::uint32_t(rng()%3):0xffffffffu;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x16b16au||target==0x16ad45u?2u:target==0x169c46u?1u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto record=[&](std::uint32_t index){return records+0x20ccu*(index&3u);};
    if(target==0x16b16au) {
        const auto at=record(r(s.memory,s.gp[4]+4u));
        if(rng()%2)w(s.memory,s.observe,0x16b16bu,at+4u,4,rng()%4?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu);
        if(rng()%2)w(s.memory,s.observe,0x16b16cu,at+8u,4,std::uint32_t(rng()%3)-1u);
    }
    if(target==0x16ad45u)w(s.memory,s.observe,0x16ad46u,record(r(s.memory,s.gp[4]+4u))+0x10u+0x1010u*(r(s.memory,s.gp[4]+8u)&1u),4,weapon_of(rng));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x150990u)s.gp[0]=rng()%4?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu;
    if(target==0xce230u)s.gp[0]=weapon_of(rng);
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=frames+0x40u+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x20ccu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?0xffffu:3u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u+4u*args;
}
const SourcePhysicsContactRuntimeServices::RequiredOwner required=[](SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsContactRuntimeAbi &a,const SourcePhysicsContactRecordObserver &o) {
    if(call.target!=0x169c46u)throw std::runtime_error("Unowned required target");
    SourcePhysicsWorldRuntimeServices none;
    WorldState s{m,o,none,SourcePhysicsContactImage::TitleUpdate15};s.load(a);
    stub(s,call.target);
    a=s.save();
};
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_150990(WorldState &s){stub(s,0x150990u);}
static void run_169c46(WorldState &s){stub(s,0x169c46u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16b16au && target!=0xce230u && target!=0x16ad45u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    std::mt19937_64 weapons(rng());
    m.put(0x4f6498u,records);m.put(0x4f54f4u,players);m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_datums);
    for(std::uint32_t at=player_datums;at<player_datums+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        m.put(player_datums+0x21cu*k+0x2cu,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(player_datums+0x21cu*k+0x88u,pick(3)?pick(3):0xffu,1);
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto record=records+0x20ccu*k,player=player_datums+0x21cu*k;
        for(std::uint32_t at=record;at<record+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+4u,pick(3)?m.get(player+0x2cu):pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
        m.put(record+8u,pick(3)?std::uint32_t(std::int32_t(std::int8_t(m.get(player+0x88u,1)))):pick(3)-1u);
        m.put(record+0x10u,weapon_of(weapons));m.put(record+0x10u+0x1010u,weapon_of(weapons));
    }
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x30u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(object_datums+12u*k+8u,units+0x400u*pick(4));
    for(std::uint32_t at=units;at<units+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=frames;at<frames+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=1;k<=2;++k){const auto f=frames+0x40u*k;m.put(f-4u,pick(4));m.put(f-8u,0x20ccu*pick(4));m.put(f-12u,records+0x10u);}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,players,player_datums,headers,object_datums,units};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_168ca1(v);},[](auto &v){original::run_150990(v);},
        [](auto &v){v.required(0x168d55u,0x169c46u,false);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_168ca1(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_WEAPON_SYNC");
    std::mt19937 rng(0x168ca1u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,false,required))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
