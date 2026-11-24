// Native TU1.5 animation change (111B10) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1D5320, 115E20, 1D4350, 115E90, BC380 and 115AF0 are hash-driven stubs of
// their complete entry ABIs (and the arguments they are handed): 1D5320 and
// 1D4350 answer AL, 115E90 a blend in XMM0 (0 and negative at times); each
// writes scratch, the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the stack
// is left unbalanced. Fixtures author current names none, equal to the
// request, unset or partners, players in and out of cutscenes, modes unset
// or not (equal to the block's field or not), keep set or not, the still bit,
// and one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_change.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_animation_change_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,units=0x10020000u,scratch=0x10600000u;
constexpr std::uint32_t names[]{0x7000101u,0x1000001u,0x1000002u,0x1000003u,0x1000004u,0x1000005u,0xffffffffu};
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=5;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%4?1u+std::uint32_t(rng()%255):0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=names[rng()%7];
    if(policy&2u && rng()%13==0)s.gp[6]=units+0x400u*std::uint32_t(rng()%2)+0x100u;
    if(policy&2u && rng()%13==0)s.gp[7]=units+0x400u*std::uint32_t(rng()%2)+0x200u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    if(target==0x115e90u){static constexpr float blends[]{0.f,0.5f,1.f,-0.25f};lanes[0]=bits_of(blends[rng()%4]);std::memcpy(&s.xmm[0],lanes.data(),16);}
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    static constexpr std::pair<std::uint32_t,std::uint32_t> pops[]{{0x1d5320u,24u},{0x115e20u,4u},{0x1d4350u,4u},{0x115e90u,12u},{0xbc380u,8u},{0x115af0u,8u}};
    for(const auto &[t,n]:pops)if(t==target)s.gp[4]+=n+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1d5320(WorldState &s){stub(s,0x1d5320u);}
static void run_115e20(WorldState &s){stub(s,0x115e20u);}
static void run_1d4350(WorldState &s){stub(s,0x1d4350u);}
static void run_115e90(WorldState &s){stub(s,0x115e90u);}
static void run_bc380(WorldState &s){stub(s,0xbc380u);}
static void run_115af0(WorldState &s){stub(s,0x115af0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto name=[&]{return names[pick(7)];};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x51daf8u,pick(4)?0u:1u,1);m.put(0x46a0e8u,0u);
    for(std::uint32_t k=0;k<4;++k){m.put(0x474be4u+8u*k,names[1u+k]);m.put(0x474be8u+8u*k,names[1u+pick(5)]);}
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,units+0x400u*k);}
    for(std::uint32_t k=0;k<2;++k) {
        const auto unit=units+0x400u*k;
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x12au,0x100u,2);m.put(unit+0x33eu,0x200u+pick(4),2);m.put(unit+0x13cu,pick(3)?std::uint32_t(rng()):0xffffffffu);
        m.put(unit+0x100u+0x7cu,name());m.put(unit+0x100u+0x70u,name());m.put(unit+0x100u+0x6cu,pick(4)?0u:0x10u,1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(2));m.put(stack+8u,name());m.put(stack+20u,name());
    m.put(stack+28u,(std::uint32_t(rng())&0xffffff00u)|(pick(2)?0u:1u));
    random_abi(rng,c,stack);
    c.abi.gp[0]=std::uint32_t(rng())&0x3fu;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,units};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_111b10(v);},[](auto &v){original::run_1d5320(v);},
        [](auto &v){original::run_115e20(v);},[](auto &v){original::run_1d4350(v);},[](auto &v){original::run_115e90(v);},
        [](auto &v){original::run_bc380(v);},[](auto &v){original::run_115af0(v);});
}
void reference(WorldState &s){original::run_111b10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_CHANGE");
    std::mt19937 rng(0x111b10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
