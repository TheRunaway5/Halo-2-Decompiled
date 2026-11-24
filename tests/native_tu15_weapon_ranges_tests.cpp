// Native TU1.5 weapon ranges (1ADC90) against the unmodified translated body
// of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 106160 and 1062A0 (direct) and 106230 (through the unit's dispatch) are
// hash-driven stubs of their complete entry ABIs: 106160 answers a scale in
// XMM0 (now and then zero or NaN), 1062A0 and 106230 answer AL; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX (another weapon), EBP (another unit), ESI (another record) or
// EDI (another definition) change. Fixtures author units with primary and
// secondary weapons or none, the word NONE or not, unusable weapons, kinds 1
// and 2, ranges on a small grid; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_weapon_ranges.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_weapon_ranges_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,object_datums=0x10001000u,objects=0x10010000u,instances=0x10020000u,definitions=0x10030000u,
    records=0x10040000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target!=0x106160u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffff0000u)|(4u+std::uint32_t(rng()%4));
    if(policy&2u && rng()%13==0)s.gp[5]=12u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=definitions+0x400u*std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    if(target==0x106160u) {
        static constexpr float scales[]{0.5f,1.0f,2.0f,0.0f,4.0f};
        lanes={rng()%11?bits(scales[rng()%5]):0x7fc00000u|std::uint32_t(rng()%0x100),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[0],lanes.data(),16);
    }
    s.gp[4]+=4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_106160(WorldState &s){stub(s,0x106160u);}
static void run_1062a0(WorldState &s){stub(s,0x1062a0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x106230u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x46a0d0u,bits(1.0f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(object_datums+12u*k+8u,objects+0x400u*k);
    for(std::uint32_t at=objects;at<objects+0x2000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto u=objects+0x400u*k;
        m.put(u+0x212u,pick(5)?pick(4):0xffu,1);m.put(u+0x213u,pick(3)?pick(4):0xffu,1);
        for(std::uint32_t j=0;j<4;++j)m.put(u+0x218u+4u*j,pick(6)?(std::uint32_t(rng())&0xffff0000u)|(4u+pick(4)):0xffffffffu);
    }
    for(std::uint32_t k=4;k<8;++k)m.put(objects+0x400u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x400u*pick(2));
    for(std::uint32_t at=definitions;at<definitions+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto d=definitions+0x400u*k;
        m.put(d+0x12cu,std::uint32_t(rng())&~0x20u|(pick(3)?0u:0x20u));
        for(std::uint32_t j=0;j<5;++j)m.put(d+0x208u+4u*j,pick(9)?bits(float(pick(8))/2.0f):0x7fc00000u|pick(0x100));
        m.put(d+0x294u,pick(3),2);
    }
    for(std::uint32_t at=records;at<records+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1a0000u+pick(0x100));m.put(stack+4u,pick(2)?0xffffu|(std::uint32_t(rng())&0xffff0000u):std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(9)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu;c.abi.gp[6]=records+0x40u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,object_datums,objects,instances,definitions,records};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1adc90(v);},[](auto &v){original::run_106160(v);},
        [](auto &v){original::run_1062a0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1adc90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WEAPON_RANGES");
    std::mt19937 rng(0x1adc90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
