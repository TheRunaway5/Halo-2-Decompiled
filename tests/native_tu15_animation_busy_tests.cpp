// Native TU1.5 animation busy check (113190) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// C9590 is a hash-driven stub of its complete entry ABI: it hashes what it
// reads, writes scratch and answers AL (0 two times in three); the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author units with and without a player, hands with and without
// weapons whose timers are negative, zero or positive, the pose flag set or
// not, and state and channel animations from every class of the original's
// searches and from every leaf between them; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_busy.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_animation_busy_detail;
namespace names=halo2::native_animation_busy;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10001000u,units=0x10010000u,weapons=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffffff00u)|std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?names::weapon_animations[rng()%(sizeof(names::weapon_animations)/4)]:names::pose_animations[rng()%(sizeof(names::pose_animations)/4)];
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
static void run_c9590(WorldState &s){stub(s,0xc9590u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
// Every boundary of the original's two searches (each interval's ends and
// their neighbours) and every name they list, independent of the port's
// name sets.
constexpr std::uint32_t boundaries[]{
    0x00000000u,0x04000049u,0x0400004au,0x0400004bu,0x0400076bu,0x0400076cu,0x0400076du,0x0400076eu,0x05000009u,0x0500000au,0x0500000bu,
    0x05000023u,0x05000024u,0x05000025u,0x050000c2u,0x050000c3u,0x050000c4u,0x05000767u,0x05000768u,0x05000769u,0x0500076au,0x0500076bu,
    0x0500076cu,0x0600005eu,0x0600005fu,0x06000060u,0x0600008bu,0x0600008cu,0x0600008du,0x060000cbu,0x060000ccu,0x060000cdu,0x060000ceu,
    0x060006abu,0x060006acu,0x060006adu,0x0700002bu,0x0700002cu,0x0700002du,0x07000542u,0x07000543u,0x07000544u,0x0700076du,0x0700076eu,
    0x0700076fu,0x080000c3u,0x080000c4u,0x080000c5u,0x0800076eu,0x0800076fu,0x08000770u,0x09000010u,0x09000011u,0x09000012u,0x09000013u,
    0x0a00000fu,0x0a000010u,0x0a000011u,0x0a000012u,0x0a000013u,0x0a000014u,0x0a00002cu,0x0a00002du,0x0a00002eu,0x0a00003du,0x0a00003eu,
    0x0a00003fu,0x0a000040u,0x0a000041u,0x0a00022cu,0x0a00022du,0x0a00022eu,0x0a0005b8u,0x0a0005b9u,0x0a0005bau,0x0a000766u,0x0a000767u,
    0x0a000768u,0x0b00002du,0x0b00002eu,0x0b00002fu,0x0b00022du,0x0b00022eu,0x0b00022fu,0x0b0005b1u,0x0b0005b2u,0x0b0005b3u,0x0b0005b7u,
    0x0b0005b8u,0x0b0005b9u,0x0c000042u,0x0c000043u,0x0c000044u,0x0c000072u,0x0c000073u,0x0c000074u,0x0c000075u,0x0c000076u,0x0c000077u,
    0x0c000078u,0x0c0005b6u,0x0c0005b7u,0x0c0005b8u,0x0c0006b2u,0x0c0006b3u,0x0c0006b4u,0x0c0006ccu,0x0c0006cdu,0x0c0006ceu,0x0d000020u,
    0x0d000021u,0x0d000022u,0x0d00002au,0x0d00002bu,0x0d00002cu,0x0d000041u,0x0d000042u,0x0d000043u,0x0d00022bu,0x0d00022cu,0x0d00022du,
    0x0e000029u,0x0e00002au,0x0e00002bu,0x0e000037u,0x0e000038u,0x0e000039u,0x0e00003au,0x0e00003bu,0x0e00003cu,0x0e0000c2u,0x0e0000c3u,
    0x0e0000c4u,0x0e00067cu,0x0e00067du,0x0e00067eu,0x0f000039u,0x0f00003au,0x0f00003bu,0x1000003bu,0x1000003cu,0x1000003du,0x1000006bu,
    0x1000006cu,0x1000006du,0x11000073u,0x11000074u,0x11000075u,0x11000076u,0x11000077u,0x140005b2u,0x140005b3u,0x140005b4u,0x7fffffffu,
    0x80000000u,0xffffffffu};
std::uint32_t animation(std::mt19937 &rng) {
    switch(rng()%7) {
    case 0:return names::pose_animations[rng()%(sizeof(names::pose_animations)/4)];
    case 1:return names::weapon_animations[rng()%(sizeof(names::weapon_animations)/4)];
    case 2:return names::channel_pose_animations[rng()%(sizeof(names::channel_pose_animations)/4)];
    case 3:return std::uint32_t(rng());
    default:return boundaries[rng()%(sizeof(boundaries)/4)];
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<8;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t k=0;k<4;++k)m.put(datums+12u*k+8u,units+0x800u*(chance(4)?pick(4):k));
    for(std::uint32_t k=4;k<8;++k)m.put(datums+12u*k+8u,weapons+0x400u*pick(4));
    for(std::uint32_t at=units;at<units+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto u=units+0x800u*k;
        const auto state=0x200u+0x10u*pick(4),channels=0x300u+0x10u*pick(4);
        m.put(u+0x12au,state,2);m.put(u+0x33eu,channels,2);
        m.put(u+state+0x7cu,animation(rng));m.put(u+channels+0xa4u,animation(rng));
        m.put(u+state+0x14u,chance(2)?0x10u|std::uint32_t(rng()&0xefu):std::uint32_t(rng())&0xefu,1);
        m.put(u+0x13cu,chance(4)?0xffffffffu:std::uint32_t(rng()));
        // Slots -3 and -2 read +0x20C and +0x210 (whose upper half is the slots themselves).
        m.put(u+0x20cu,(std::uint32_t(rng())&0xffff0000u)|(4u+pick(4)));m.put(u+0x210u,4u+pick(4),2);
        const auto slot=[&](std::uint32_t none_odds){return chance(none_odds)?0xffu:chance(6)?0xfeu-pick(2):pick(4);};
        m.put(u+0x212u,slot(4),1);m.put(u+0x213u,slot(3),1);
        for(std::uint32_t slot=0;slot<4;++slot)m.put(u+0x218u+4u*slot,chance(5)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|(4u+pick(4)));
    }
    for(std::uint32_t at=weapons;at<weapons+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t timers[]{0u,1u,5u,0xffffu,0x8000u,0x7fffu};
    for(std::uint32_t k=0;k<4;++k)m.put(weapons+0x400u*k+0x17cu,timers[pick(6)],2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc8000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,units,weapons};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_113190(v);},[](auto &v){original::run_c9590(v);});
}
void reference(WorldState &s){original::run_113190(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_BUSY");
    std::mt19937 rng(0x113190u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
