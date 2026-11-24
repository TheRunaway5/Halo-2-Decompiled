// Native TU1.5 object reach (154270) against the unmodified translated body
// of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// CFC90 (through the unit's dispatch) and 153E10 and C6920 (direct) are
// hash-driven stubs of their complete entry ABIs: CFC90 fills the hands
// record and answers AL (RET C), 153E10 hashes the action and answers AL
// (RET 4), C6920 answers AL; the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP (another player), ESI or
// EDI change. Fixtures author units and objects near and far (on a small
// grid, so the reach can equal the distance), extended definitions or not,
// unreachable and parented objects, game kinds 2 and others; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_object_reach.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_reach_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,object_datums=0x10001000u,objects=0x10010000u,players=0x10020000u,player_datums=0x10021000u,
    instances=0x10030000u,definitions=0x10031000u,games=0x10040000u,results=0x10050000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0xcfc90u?3u:target==0x153e10u?1u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x153e10u)for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,s.gp[3]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xcfc90u)w(s.memory,s.observe,0xcfc91u,r(s.memory,s.gp[4]+12u),4,std::uint32_t(rng())&(rng()%2?0x01010101u:0x01000100u)*std::uint32_t(rng()%2));
    if(target==0x153e10u)w(s.memory,s.observe,0x153e11u,s.gp[3]+8u+4u*std::uint32_t(rng()%2),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?1u+std::uint32_t(rng()%255):0u);
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=player_datums+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u+4u*args;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_153e10(WorldState &s){stub(s,0x153e10u);}
static void run_c6920(WorldState &s){stub(s,0xc6920u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xcfc90u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4f54f4u,players);m.put(0x4f0414u,instances);m.put(0x4f3218u,games);
    m.put(0x46a178u,bits(float(pick(3))));m.put(0x456918u,bits(float(pick(3))));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(object_datums+12u*k+8u,objects+0x200u*pick(4));
    for(std::uint32_t at=objects;at<objects+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto o=objects+0x200u*k;
        m.put(o,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        for(std::uint32_t a=0;a<3;++a)m.put(o+0x30u+4u*a,pick(9)?bits(float(pick(4))):0x7fc00000u);
        m.put(o+0x3cu,bits(float(pick(3))));m.put(o+0x12cu,pick(4)?0u:1u,1);
        m.put(o+0x14cu,pick(4)?std::uint32_t(rng()):(std::uint32_t(rng())&0xffff0000u)|pick(4));
    }
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_datums);
    for(std::uint32_t at=player_datums;at<player_datums+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(player_datums+0x21cu*k+0x2cu,(std::uint32_t(rng())&0xffff0000u)|pick(8));
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x400u*pick(2));
    for(std::uint32_t k=0;k<2;++k)m.put(definitions+0x400u*k+0x290u,pick(2)?0u:1u+pick(0xffff),2);
    for(std::uint32_t at=games;at<games+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(games+8u,pick(2)?2u:pick(5));
    for(std::uint32_t at=results;at<results+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(stack+8u,results+8u*pick(2));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,object_datums,objects,player_datums,instances,games,results};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_154270(v);},[](auto &v){original::run_153e10(v);},
        [](auto &v){original::run_c6920(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_154270(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_REACH");
    std::mt19937 rng(0x154270u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
