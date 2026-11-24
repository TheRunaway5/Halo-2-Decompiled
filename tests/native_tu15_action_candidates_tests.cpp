// Native TU1.5 action candidates (1AE7F0) against the unmodified translated
// body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1AEA30 and 1AECE0 (direct) and BAD60 (through the unit's dispatch) are
// hash-driven stubs of their complete entry ABIs, each hashing its
// arguments: 1AEA30 writes the candidate and answers AL (RET 14), 1AECE0
// answers AL (RET 4), BAD60 writes the lookup record and answers AX 0, 1 or
// 2 (RET C); the caller-saved registers, XMM, flags and (by policy) MXCSR
// are random, and by policy EBX, EBP, ESI or EDI change. Fixtures author
// units and vehicles with and without the scaled flag, counts near 64,
// definitions with and without actions and their entries; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_action_candidates.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_action_candidates_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,object_datums=0x10001000u,objects=0x10010000u,instances=0x10020000u,definitions=0x10030000u,
    entries_area=0x10040000u,candidates=0x10050000u,games=0x10060000u,counts=0x10070000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x1aea30u?5u:target==0x1aece0u?1u:3u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1aea30u)w(s.memory,s.observe,0x1aea31u,s.gp[6]+4u*std::uint32_t(rng()%24),4,std::uint32_t(rng()));
    if(target==0xbad60u)w(s.memory,s.observe,0xbad61u,s.gp[3]+4u*std::uint32_t(rng()%8),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1aea30u||target==0x1aece0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%4?1u+std::uint32_t(rng()%255):0u);
    if(target==0xbad60u)s.gp[0]=(s.gp[0]&0xffff0000u)|std::uint32_t(rng()%3)|(rng()%9?0u:0x8000u);
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%0x41);
    if(policy&2u && rng()%13==0)s.gp[5]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=candidates+0x60u*std::uint32_t(rng()%0x40);
    if(policy&2u && rng()%13==0)s.gp[7]=entries_area+0x1cu*std::uint32_t(rng()%8);
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
static void run_1aea30(WorldState &s){stub(s,0x1aea30u);}
static void run_1aece0(WorldState &s){stub(s,0x1aece0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbad60u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x46a0d0u,bits(1.0f));m.put(0x4ecc1cu,games);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x30u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(object_datums+12u*k+8u,objects+0x100u*pick(4));
    for(std::uint32_t at=objects;at<objects+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(objects+0x100u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(objects+0x100u*k+0xaau,pick(4),1);}
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x100u*k);
    for(std::uint32_t at=definitions;at<definitions+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto d=definitions+0x100u*k;
        m.put(d+0x38u,pick(3)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
        m.put(d+0x68u,pick(5)?pick(5):0u-pick(2));m.put(d+0x6cu,entries_area+0x20u*pick(2));
        m.put(d+0xbcu,std::uint32_t(rng())&~0x80000u|(pick(2)?0x80000u:0u));
    }
    for(std::uint32_t at=entries_area;at<entries_area+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(entries_area+0x1cu*k+0x10u,bits(float(pick(9))/4.0f));
    for(std::uint32_t at=games;at<games+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(games+0xf4u,games+0x80u);m.put(games+0x88u,bits(float(1+pick(4))/2.0f));
    for(std::uint32_t at=candidates;at<candidates+0x60u*0x42u;at+=4)m.put(at,0u);
    for(std::uint32_t at=counts;at<counts+0x10u;at+=4)m.put(at,pick(4)?pick(6):0x3eu+pick(3));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x180u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1a0000u+pick(0x100));
    m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(stack+0xcu,std::uint32_t(rng()));
    m.put(stack+0x10u,std::uint32_t(rng()));m.put(stack+0x14u,std::uint32_t(rng()));m.put(stack+0x18u,pick(4)?pick(6):0x3eu+pick(3));
    m.put(stack+0x1cu,candidates);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,object_datums,objects,instances,definitions,entries_area,games};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae7f0(v);},[](auto &v){original::run_1aea30(v);},
        [](auto &v){original::run_1aece0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1ae7f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ACTION_CANDIDATES");
    std::mt19937 rng(0x1ae7f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
