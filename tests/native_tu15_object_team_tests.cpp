// Native TU1.5 object team (C8E70) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// D2B60 and D2BD0 are hash-driven stubs of their complete entry ABIs: D2B60
// starts the riders' iterator it is given, D2BD0 advances it (a vehicle, a
// rider and a seat or none) and answers whether one remains; each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author objects held back, with players, with actors,
// vehicles with riders in seats with and without flag bit 11 and teams that
// agree or not, and other objects; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_team.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_team_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,players=0x10030000u,actors=0x10040000u,
    instances=0x10050000u,definitions=0x10060000u,seats=0x10070000u,scratch=0x10600000u;
std::uint32_t limit=0,walked=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xd2b60u){walked=0;for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,0xd2b61u,s.gp[3]+4u*k,4,std::uint32_t(rng()));}
    if(target==0xd2bd0u) {
        const bool more=walked++<limit;
        const auto iterator=s.gp[2];
        w(s.memory,s.observe,0xd2bd1u,iterator+0xcu,4,rng()%6?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu);
        w(s.memory,s.observe,0xd2bd2u,iterator+0x10u,4,(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4));
        w(s.memory,s.observe,0xd2bd3u,iterator+0x14u,2,rng()%5?std::uint32_t(rng()%3):0xffffu);
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(more?1u+std::uint32_t(rng()%255):0u);
    } else s.gp[0]=std::uint32_t(rng());
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=instances;
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?0xffffffffu:std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=datums;
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x200u*std::uint32_t(rng()%4);
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
static void run_d2b60(WorldState &s){stub(s,0xd2b60u);}
static void run_d2bd0(WorldState &s){stub(s,0xd2bd0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f54f4u,players);m.put(0x5020c8u,actors);m.put(0x4f0414u,instances);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x200u*k;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())&0xffff0000u)|pick(2));
        m.put(object+0xaau,pick(3)?1u:pick(4),1);m.put(object+0x10au,pick(6)?0u:4u,1);
        m.put(object+0x13cu,pick(5)?0xffffffffu:pick(2));m.put(object+0x12cu,pick(5)?0xffffffffu:pick(2));
        m.put(object+0x138u,pick(4)?pick(3):0xffffu,2);
    }
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,players+0x100u);
    for(std::uint32_t k=0;k<2;++k)m.put(players+0x100u+0x21cu*k+0xc0u,pick(4),1);
    for(std::uint32_t at=actors;at<actors+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(actors+0x44u,actors+0x100u);
    for(std::uint32_t k=0;k<2;++k)m.put(actors+0x100u+0x888u*k+0x24u,pick(4),2);
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x200u*k);}
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=definitions+0x200u*k;at<definitions+0x200u*k+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definitions+0x200u*k+0x1ccu,seats);}
    for(std::uint32_t k=0;k<3;++k)m.put(seats+0xb0u*k,(std::uint32_t(rng())&~0x800u)|(pick(2)?0x800u:0u));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,players,actors,instances,definitions,seats};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c8e70(v);},[](auto &v){original::run_d2b60(v);},
        [](auto &v){original::run_d2bd0(v);});
}
void reference(WorldState &s){original::run_c8e70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_TEAM");
    std::mt19937 rng(0xc8e70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        limit=std::uint32_t(rng()%6);
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
