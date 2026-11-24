// Native TU1.5 action allowed (1EFD60) against the unmodified translated
// body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The eighteen children (direct, or through the dispatch) are hash-driven
// stubs of their complete entry ABIs: each hashes what it reads (its pushed
// arguments), writes scratch and its outputs (D2B00 the rider iterator,
// counting its own calls in the iterator so a walk ends; 1055F0 the weapon
// report), answers AL, AX or EAX; the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or
// the stack is left unbalanced. Fixtures author every action (and past
// 0x1F), local players with and without players and units, actions
// flagged in game mode 2, units seated or not with parents and seats of
// every flag, controls in every mode and state, and the original's jump
// table; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_action_allowed.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_action_allowed_detail;
namespace {
constexpr std::uint32_t controls=0x10000000u,locals=0x10001000u,actions=0x10002000u,game=0x10003000u,players=0x10004000u,
    player_data=0x10005000u,objects=0x10006000u,entries=0x10007000u,bodies=0x10010000u,definitions=0x10008000u,
    definition_bodies=0x10020000u,items=0x10009000u,scratch=0x10600000u;
constexpr std::uint32_t targets[32]{0x1f0160u,0x1f0176u,0x1eff8eu,0x1f05d6u,0x1effdfu,0x1f0516u,0x1f0534u,0x1f0592u,0x1f05b5u,
    0x1f0400u,0x1efe29u,0x1efe8bu,0x1efeedu,0x1eff2bu,0x1eff69u,0x1f00cbu,0x1f0217u,0x1f01a7u,0x1f0118u,0x1f05d6u,0x1f0307u,
    0x1f04ccu,0x1f0484u,0x1f033au,0x1f0456u,0x1f0450u,0x1f045cu,0x1f045cu,0x1f0450u,0x1f045cu,0x1f01d6u,0x1f018eu};
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0x1f1160u:case 0x1055f0u:return 2u;case 0x1efd10u:case 0x1efcc0u:case 0x1ac79bu:case 0x1ac595u:case 0xbcdf0u:return 1u;
        default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    std::uint32_t answer=std::uint32_t(rng());
    if(target==0xd2b00u) {
        const auto it=s.gp[6];
        const auto count=r(s.memory,it+0x14u)&3u;
        w(s.memory,s.observe,target+1u,it+0x14u,4,count+1u);
        if(count<3u && rng()%4) {
            w(s.memory,s.observe,target+2u,it+4u,4,rng()%3?std::uint32_t(rng()%4):r(s.memory,s.gp[4]+4u+0x14u));
            w(s.memory,s.observe,target+3u,it+8u,2,rng()%3?std::uint32_t(rng()%4):0xffffu);
            answer=1u+std::uint32_t(rng()%255);
        } else answer=0u;
    }
    if(target==0x1055f0u)for(std::uint32_t k=0;k<5;++k)w(s.memory,s.observe,target+1u,arg(2)+0x24u+2u*k,2,std::uint32_t(rng()%6)-1u);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    switch(target) {
    case 0x13f620u:case 0x1f1160u:case 0x1efcc0u:case 0x1ac79bu:case 0x1ac595u:case 0x161530u:case 0x536e0u:case 0x1060a0u:case 0x105fd0u:
        s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));break;
    case 0xcad40u:case 0x1060e0u:s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%3?std::uint32_t(rng()%8):0xffffu);break;
    case 0xce230u:case 0x165a50u:s.gp[0]=rng()%3?std::uint32_t(rng()%8):0xffffffffu;break;
    case 0x1efd10u:s.gp[0]=rng()%2?0u:std::uint32_t(rng());break;
    case 0xbcdf0u:s.gp[0]=rng()%3?items:0u;break;
    default:break;
    }
    if(policy&2u && rng()%13==0)s.gp[3]=bodies;
    if(policy&2u && rng()%13==0)s.gp[5]=entries;
    if(policy&2u && rng()%13==0)s.gp[6]=player_data+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=controls+0x1b0u*std::uint32_t(rng()%4);
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
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1f1160(WorldState &s){stub(s,0x1f1160u);}
static void run_1efd10(WorldState &s){stub(s,0x1efd10u);}
static void run_1efcc0(WorldState &s){stub(s,0x1efcc0u);}
static void run_1ac79b(WorldState &s){stub(s,0x1ac79bu);}
static void run_1ac595(WorldState &s){stub(s,0x1ac595u);}
static void run_cad40(WorldState &s){stub(s,0xcad40u);}
static void run_165a50(WorldState &s){stub(s,0x165a50u);}
static void run_536e0(WorldState &s){stub(s,0x536e0u);}
static void run_1060a0(WorldState &s){stub(s,0x1060a0u);}
static void run_1060e0(WorldState &s){stub(s,0x1060e0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0x13f620u:case 0xce230u:case 0xd2ad0u:case 0xd2b00u:case 0x161530u:case 0xbcdf0u:case 0x1055f0u:case 0x105fd0u:stub(s,target);return;
    default:throw std::runtime_error("Unbound sandbox dispatch target");
    }
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto fill=[&](std::uint32_t from,std::uint32_t size){for(std::uint32_t at=from;at<from+size;at+=4)m.put(at,std::uint32_t(rng()));};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x52be64u,controls);m.put(0x4f54f0u,locals);m.put(0x51df94u,chance(12)?0u:actions);m.put(0x4f3218u,game);
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,objects);m.put(0x4f0414u,definitions);m.put(0x46a130u,bits(1.0f));
    for(std::uint32_t k=0;k<32;++k)m.put(0x1f05e4u+4u*k,targets[k]);
    for(std::uint32_t k=0;k<4;++k) {
        const auto ctl=controls+0x1b0u*k;
        fill(ctl,0x1b0u);
        m.put(ctl+0x80u,chance(3)?10u+pick(13):pick(0x20));m.put(ctl+0x140u,pick(6),2);m.put(ctl+0x144u,std::uint32_t(rng()));
        m.put(ctl+0x148u,pick(6),2);m.put(ctl+0x14cu,pick(16),2);m.put(ctl+0x197u,pick(6),1);m.put(ctl+0x19bu,chance(3)?0u:pick(4),1);
    }
    fill(locals,0x20u);
    for(std::uint32_t k=0;k<4;++k)m.put(locals+0xcu+4u*k,chance(6)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(4));
    fill(actions,0x430u);m.put(actions+0x420u,chance(8)?pick(0x20):0x20u);m.put(actions+0x424u,actions);
    for(std::uint32_t k=0;k<0x20u;++k)m.put(actions+20u*k+0x10u,chance(4)?1u:0u,1);
    fill(game,0x20u);m.put(game+8u,chance(2)?2u:pick(4));
    fill(players,0x50u);m.put(players+0x44u,player_data);
    // Units at indices 0-3, and now and then 0x6000-0x6003, where three times the index passes 16 bits.
    const auto unit_index=[&]{return chance(4)?0x6000u+pick(4):pick(4);};
    for(std::uint32_t k=0;k<4;++k) {
        const auto p=player_data+0x21cu*k;
        fill(p,0x21cu);m.put(p+0x2cu,chance(8)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|unit_index());
        m.put(p+0x24u,chance(4)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(4));
    }
    fill(objects,0x50u);m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<4;++k){fill(entries+12u*(0x6000u+k),12u);m.put(entries+12u*(0x6000u+k)+8u,bodies+0x400u*k);}
    for(std::uint32_t k=0;k<8;++k) {
        fill(entries+12u*k,12u);m.put(entries+12u*k+8u,bodies+0x400u*k);
        const auto o=bodies+0x400u*k;
        fill(o,0x400u);
        m.put(o,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(o+0x14u,chance(2)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(8));
        m.put(o+0xf0u,bits(chance(2)?0.5f:2.0f));m.put(o+0x10au,std::uint32_t(rng())&0xffffu,2);
        m.put(o+0x1fcu,chance(2)?0xffffu:pick(4),2);m.put(o+0x212u,pick(4),1);m.put(o+0x213u,chance(2)?0xffu:pick(4),1);
    }
    for(std::uint32_t k=0;k<4;++k) {
        fill(definitions+16u*k,16u);m.put(definitions+16u*k+8u,definition_bodies+0x400u*k);
        const auto d=definition_bodies+0x400u*k;
        fill(d,0x400u);m.put(d+0x1ccu,d+0x200u);m.put(d+0x21au,chance(3)?0u:pick(5),2);
        for(std::uint32_t q=0;q<4;++q)m.put(d+0x200u+0xb0u*q,std::uint32_t(rng())&(chance(2)?0xfffff7f3u:0xffffffffu));
    }
    fill(items,0x20u);m.put(items,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    fill(scratch,0x100u);
    fill(stack-0x100u,0x140u);
    m.put(stack,0x1ef000u+pick(0x100));
    m.put(stack+4u,chance(10)?0xffffffffu:pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[2]=chance(12)?0x20u+pick(4):chance(16)?0xffffffffu:pick(32);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{controls,locals,actions,game,players,player_data,objects,entries,bodies,definitions};
        m.drop(areas[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1efd60(v);},
        [](auto &v,std::uint32_t target){
            switch(target) {
            case 0x1f1160u:original::run_1f1160(v);break;case 0x1efd10u:original::run_1efd10(v);break;
            case 0x1efcc0u:original::run_1efcc0(v);break;case 0x1ac79bu:original::run_1ac79b(v);break;
            case 0x1ac595u:original::run_1ac595(v);break;case 0xcad40u:original::run_cad40(v);break;
            case 0x165a50u:original::run_165a50(v);break;case 0x536e0u:original::run_536e0(v);break;
            case 0x1060a0u:original::run_1060a0(v);break;case 0x1060e0u:original::run_1060e0(v);break;
            default:throw std::logic_error("Unbound action allowed child");
            }},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1efd60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ACTION_ALLOWED");
    std::mt19937 rng(0x1efd60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
