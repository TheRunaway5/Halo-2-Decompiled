// Native TU1.5 unit weapon controls (C7DD0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E8E10, 116480, 113190, CE3A0, CAAA0, 105500 and D3A80 (direct), and
// 1164D0, 116400, 116440 and 18C000 (through the dispatch) are hash-driven
// stubs of their complete entry ABIs: each hashes what it reads (its pushed
// arguments, 18C000 its request) and writes scratch; 116480, 1164D0, 113190
// and CAAA0 answer AL (0 half the time), CE3A0 AX (0xFFFF now and then); the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author units with every counter, stance, stun, weapon, grenade
// and zoom shape, weapons with and without zoom levels, the global player
// information, and action blocks with each blocking action; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_weapon_controls.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_weapon_controls_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10001000u,units=0x10010000u,weapons=0x10020000u,instances=0x10030000u,
    definitions=0x10031000u,globals_block=0x10040000u,information=0x10041000u,clock=0x10042000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0xe8e10u:case 0x116400u:case 0x116440u:case 0x113190u:case 0xd3a80u:return 1u;case 0x105500u:return 3u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x18c000u)for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,s.gp[7]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x116480u||target==0x1164d0u||target==0x113190u||target==0xcaaa0u)
        s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(target==0xce3a0u)s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%4?std::uint32_t(rng()%4):0xffffu);
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffffff00u)|std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=units+0x800u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?definitions+0x400u*std::uint32_t(rng()%8):std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?headers:std::uint32_t(rng()%3);
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
static void run_e8e10(WorldState &s){stub(s,0xe8e10u);}
static void run_116480(WorldState &s){stub(s,0x116480u);}
static void run_113190(WorldState &s){stub(s,0x113190u);}
static void run_ce3a0(WorldState &s){stub(s,0xce3a0u);}
static void run_caaa0(WorldState &s){stub(s,0xcaaa0u);}
static void run_105500(WorldState &s){stub(s,0x105500u);}
static void run_d3a80(WorldState &s){stub(s,0xd3a80u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1164d0u && target!=0x116400u && target!=0x116440u && target!=0x18c000u)
        throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x51df54u,clock);m.put(0x4ecc1cu,globals_block);
    m.put(0x46a0d0u,bits(float(int(pick(5))-1)/2.0f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    // Datums 0-3 the units, 4-7 the weapons.
    for(std::uint32_t k=0;k<8;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t k=0;k<4;++k)m.put(datums+12u*k+8u,units+0x800u*(chance(4)?pick(4):k));
    for(std::uint32_t k=4;k<8;++k)m.put(datums+12u*k+8u,weapons+0x400u*pick(4));
    // Tags 0-3 unit definitions, 4-7 weapon definitions.
    for(std::uint32_t k=0;k<8;++k){m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x400u*(chance(5)?pick(8):k));m.put(instances+16u*k+12u,std::uint32_t(rng()));}
    for(std::uint32_t at=definitions;at<definitions+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k) {
        const auto d=definitions+0x400u*k;
        m.put(d+0xbcu,(std::uint32_t(rng())&~0xc00u)|(chance(4)?0x400u:0u)|(chance(5)?0x800u:0u));
        static constexpr std::uint32_t levels[]{0u,1u,2u,3u,0xffffu,0x8000u,5u};
        m.put(d+0x1feu,levels[pick(7)],2);
        m.put(d+0x270u,chance(5)?0xffffffffu:std::uint32_t(rng()));m.put(d+0x278u,chance(5)?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=weapons;at<weapons+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(weapons+0x400u*k,(std::uint32_t(rng())&0xffff0000u)|(4u+pick(4)));
    for(std::uint32_t at=units;at<units+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto u=units+0x800u*k;
        m.put(u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(u+0x19u,chance(4)?8u:std::uint32_t(rng())&0xf7u,1);
        m.put(u+0xaau,chance(2)?0u:1u+pick(8),1);
        m.put(u+0x10au,chance(5)?4u:std::uint32_t(rng())&0xfbu,1);
        m.put(u+0x134u,std::uint32_t(rng()));
        m.put(u+0x148u,chance(3)?std::uint32_t(rng())&~0x10000000u:std::uint32_t(rng()));
        m.put(u+0x1c0u,bits(float(int(pick(9))-4)/4.0f));m.put(u+0x1c4u,bits(float(int(pick(9))-4)/4.0f));
        m.put(u+0x1ecu,std::uint32_t(int(pick(4))-1));
        static constexpr std::uint32_t counters[]{0u,1u,0x7fu,0x80u,0xffu,5u};
        m.put(u+0x1f5u,counters[pick(6)],1);m.put(u+0x1fau,counters[pick(6)],1);m.put(u+0x1fbu,counters[pick(6)],1);
        for(std::uint32_t hand=0;hand<2;++hand) {
            const auto current=chance(4)?0xffu:pick(4);
            m.put(u+0x212u+hand,current,1);m.put(u+0x216u+hand,chance(2)?current:(chance(4)?0xffu:pick(4)),1);
        }
        for(std::uint32_t slot=0;slot<4;++slot)m.put(u+0x218u+4u*slot,chance(5)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|(4u+pick(4)));
        const auto grenade=pick(3);
        m.put(u+0x23cu,grenade,1);m.put(u+0x23du,chance(2)?grenade:pick(3),1);
        static constexpr std::uint32_t zooms[]{0xffu,0u,1u,2u,0x80u};
        const auto zoom=zooms[pick(5)];
        m.put(u+0x240u,zoom,1);m.put(u+0x241u,chance(3)?zoom:zooms[pick(5)],1);
        const auto block=0x380u+0x20u*pick(4);
        m.put(u+0x346u,block,2);
        static constexpr std::uint32_t actions[]{1u<<0x16,1u<<0x1a,1u<<0x1b,1u<<8,1u<<0x12};
        std::uint32_t active=0u;
        for(auto a:actions)if(chance(6))active|=a;
        m.put(u+block+4u,(std::uint32_t(rng())&~(0x0c440100u))|active);
    }
    for(std::uint32_t at=clock;at<clock+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=globals_block;at<globals_block+0x140u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(globals_block+0x134u,information);
    for(std::uint32_t at=information;at<information+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t global_levels[]{0u,1u,2u,5u,0x80000001u,0x7fffffffu};
    m.put(information+0xb8u,global_levels[pick(6)]);
    m.put(information+0xc8u,chance(5)?0xffffffffu:std::uint32_t(rng()));m.put(information+0xd0u,chance(5)?0xffffffffu:std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc9000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,units,weapons,instances,definitions,globals_block,information,clock};
        m.drop(regions[pick(9)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c7dd0(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0xe8e10u:original::run_e8e10(v);break;case 0x116480u:original::run_116480(v);break;
            case 0x113190u:original::run_113190(v);break;case 0xce3a0u:original::run_ce3a0(v);break;case 0xcaaa0u:original::run_caaa0(v);break;
            case 0x105500u:original::run_105500(v);break;case 0xd3a80u:original::run_d3a80(v);break;
            default:throw std::logic_error("Unbound unit weapon controls child");}},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_c7dd0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_WEAPON_CONTROLS");
    std::mt19937 rng(0xc7dd0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
