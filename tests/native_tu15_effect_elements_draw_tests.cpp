// Native TU1.5 effect elements draw (1770C0) against the unmodified
// translated body of the effect particle draw unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16EA60, 178070, 3EB10 and 158290 (through the unit's dispatch) and 17DE60
// and 1772C0 (direct) are hash-driven stubs of their complete entry ABIs:
// 16EA60 answers the next authored element from EDX (NONE past them),
// 178070 one of the authored definitions, 3EB10 and 17DE60 (RET 4) AL,
// 158290 EAX, and 1772C0 (RET 10) hashes its arguments; the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change. Fixtures author elements with and without a
// cluster, visible and hidden clusters, parents and locations, effects whose
// owner is the local unit or not, definitions always drawn or not, both
// first person bytes; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_effect_elements_draw.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_effect_elements_draw_detail;
namespace {
constexpr std::uint32_t elements_array=0x10000000u,element_records=0x10001000u,definitions=0x10010000u,effects_array=0x10020000u,
    effect_records=0x10021000u,players=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x17de60u?1u:target==0x1772c0u?4u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t next=0xffffffffu;
    if(target==0x16ea60u)for(std::uint32_t i=s.gp[2];i<6u;++i)if(r(s.memory,element_records+0x54u*i,2)){next=i;break;}
    if(target==0x1772c0u)w(s.memory,s.observe,0x1772c1u,scratch+0x80u+4u*std::uint32_t(rng()%16),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%32)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x16ea60u)s.gp[0]=next;
    if(target==0x178070u)s.gp[0]=definitions+0x20u*std::uint32_t(rng()%4);
    if(target==0x3eb10u||target==0x17de60u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(target==0x158290u)s.gp[0]=rng()%2?0u:std::uint32_t(rng())|1u;
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?0xffffffffu:std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=target==0x178070u?element_records+0x54u*std::uint32_t(rng()%6):std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=target==0x16ea60u?elements_array:(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
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
static void run_17de60(WorldState &s){stub(s,0x17de60u);}
static void run_1772c0(WorldState &s){stub(s,0x1772c0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16ea60u && target!=0x178070u && target!=0x3eb10u && target!=0x158290u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x51df74u,elements_array);m.put(0x4f720cu,effects_array);m.put(0x4f9b54u,players);
    m.put(0x4c66f8u,pick(3)?pick(4):0xffffffffu);
    for(std::uint32_t k=0;k<8;++k)m.put(0x557034u-8u+4u*k,std::uint32_t(rng()));
    for(std::uint32_t at=elements_array;at<elements_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(elements_array+0x24u,0x54u);m.put(elements_array+0x44u,element_records);
    for(std::uint32_t at=element_records;at<element_records+0x54u*6u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<6;++k) {
        const auto e=element_records+0x54u*k;
        m.put(e,pick(4)?1u+pick(0x7fff):0u,2);
        m.put(e+0x18u,pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
        m.put(e+0x20u,pick(4)?std::uint32_t(int(pick(96))-32):0xffffu,2);
        m.put(e+0x30u,pick(5)?std::uint32_t(rng()):0xffffffffu);m.put(e+0x4cu,pick(5)?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t at=definitions;at<definitions+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){const auto d=definitions+0x20u*k;m.put(d+0xcu,pick(2)?0u:1u+pick(5),2);m.put(d+0x12u,pick(3),2);}
    for(std::uint32_t at=effects_array;at<effects_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(effects_array+0x44u,effect_records);
    for(std::uint32_t at=effect_records;at<effect_records+0x190u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(effect_records+0x190u*k+0x58u,pick(3)?pick(4):0xffffffffu);
    for(std::uint32_t at=players;at<players+0x94u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(players+0x94u*k+0x42u,pick(3)?pick(4):0xffffu,2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffffff00u)|pick(2),4);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{elements_array,element_records,definitions,effect_records,players};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1770c0(v);},[](auto &v){original::run_17de60(v);},
        [](auto &v){original::run_1772c0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1770c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EFFECT_ELEMENTS_DRAW");
    std::mt19937 rng(0x1770c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
