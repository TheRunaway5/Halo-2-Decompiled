// Native TU1.5 channel fades (C8390) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// D3150, D05A0 and E8E10 are hash-driven stubs of their complete entry ABIs
// (and argument): each writes scratch, the caller-saved registers, XMM, flags
// and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change
// or the stack is left unbalanced. Fixtures author fixed definitions, the
// seated bit with and without a parent and a seat, game kind 4, objects with
// and without targets, held or not, channels with and without durations, and
// weights at, near and past their ends (NaN too); one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_channel_fades.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_channel_fades_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    channels=0x10050000u,game=0x10060000u,clock=0x10070000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0xe8e10u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=definitions+0x200u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0xe8e10u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_d3150(WorldState &s){stub(s,0xd3150u);}
static void run_d05a0(WorldState &s){stub(s,0xd05a0u);}
static void run_e8e10(WorldState &s){stub(s,0xe8e10u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x4f3218u,game);m.put(0x51df54u,clock);m.put(0x46a0d0u,bits_of(1.f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x400u*k);}
    static constexpr float weights[]{0.f,1.f,0.5f,0.99f,0.01f,1.5f,-0.5f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(2));m.put(object+0x14u,pick(4)?std::uint32_t(rng()):0xffffffffu);
        m.put(object+0x10au,pick(5)?0u:4u,1);m.put(object+0x134u,std::uint32_t(rng())&(pick(2)?0x4000002u:0u));
        m.put(object+0x1fcu,pick(4)?pick(4):0xffffu,2);
        m.put(object+0x248u,pick(2)?0xffffffffu:pick(3));m.put(object+0x24cu,pick(2)?0xffffffffu:pick(3));
        for(std::uint32_t c2=0;c2<4;++c2)m.put(object+0x25cu+4u*c2,pick(12)?bits_of(weights[pick(7)]):0x7fc00000u);
    }
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x200u*k);}
    for(std::uint32_t k=0;k<2;++k){const auto definition=definitions+0x200u*k;for(std::uint32_t at=definition;at<definition+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0xbcu,pick(6)?0u:0x800u);m.put(definition+0x1b8u,pick(6)?pick(5):0u-1u);m.put(definition+0x1bcu,channels);}
    for(std::uint32_t k=0;k<4;++k){m.put(channels+8u*k,pick(4)?bits_of(float(pick(4))*0.5f):bits_of(-1.f));m.put(channels+8u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t at=game;at<game+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0xcu,pick(6)?pick(4):4u,1);
    for(std::uint32_t at=clock;at<clock+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(clock+2u,pick(4)?30u:pick(70),2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions,channels,game,clock};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c8390(v);},[](auto &v){original::run_d3150(v);},
        [](auto &v){original::run_d05a0(v);},[](auto &v){original::run_e8e10(v);});
}
void reference(WorldState &s){original::run_c8390(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHANNEL_FADES");
    std::mt19937 rng(0xc8390u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
