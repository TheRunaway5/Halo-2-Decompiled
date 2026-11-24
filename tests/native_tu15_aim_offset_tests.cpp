// Native TU1.5 aim offset (E06E0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 111F00, BAD60, CDF30 and E05D0 are hash-driven stubs of their complete
// entry ABIs and arguments: 111F00 answers AL, BAD60 AX (and fills the marker
// frame it is given), E05D0 fills the two axes it is given; each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author units with and without a parent, started or
// not, and kept offsets near and far from the new one (each axis below, on,
// inside and above the steps, on a quarter grid so the steps are met
// exactly; NaN too); one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_aim_offset.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_aim_offset_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
std::uint32_t small_float(std::mt19937_64 &rng){return rng()%16==0?0x7fc00000u:bits_of(float(int(rng()%21)-10)*0.25f);}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0xbad60u?3u:target==0xcdf30u?2u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xbad60u)for(std::uint32_t k=0;k<0x6cu;k+=4)w(s.memory,s.observe,0xbad61u,s.gp[3]+k,4,small_float(rng));
    if(target==0xe05d0u)for(std::uint32_t k=0;k<8;k+=4){w(s.memory,s.observe,0xe05d1u,s.gp[1]+k,4,small_float(rng));w(s.memory,s.observe,0xe05d2u,s.gp[7]+k,4,small_float(rng));}
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    if(target==0x111f00u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%4?0u:1u+std::uint32_t(rng()%255));
    if(target==0xbad60u)s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%3?1u+std::uint32_t(rng()%0xfffe):0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=0x20010000u-0x100u+4u*std::uint32_t(rng()%16);
    if(policy&2u && rng()%13==0)s.gp[5]=objects+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%2)+0x200u;
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%2);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_111f00(WorldState &s){stub(s,0x111f00u);}
static void run_bad60(WorldState &s){stub(s,0xbad60u);}
static void run_cdf30(WorldState &s){stub(s,0xcdf30u);}
static void run_e05d0(WorldState &s){stub(s,0xe05d0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x46a558u,bits_of(-0.5f));m.put(0x46a18cu,bits_of(0.5f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x400u*k);}
    for(std::uint32_t k=0;k<2;++k) {
        const auto object=objects+0x400u*k;
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x13cu,pick(6)?std::uint32_t(rng()):0xffffffffu);m.put(object+0x116u,0x200u+pick(8)*4u,2);
        for(std::uint32_t at=object+0x200u;at<object+0x260u;at+=4)m.put(at,bits_of(float(int(pick(9))-4)*0.5f));
        for(std::uint32_t at=object+0x168u;at<object+0x174u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x3c8u,pick(3)?1u:0u,1);
        for(std::uint32_t at=object+0x3ccu;at<object+0x3d8u;at+=4)m.put(at,pick(10)?bits_of(float(int(pick(13))-6)*0.25f):0x7fc00000u);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_e06e0(v);},[](auto &v){original::run_111f00(v);},
        [](auto &v){original::run_bad60(v);},[](auto &v){original::run_cdf30(v);},[](auto &v){original::run_e05d0(v);});
}
void reference(WorldState &s){original::run_e06e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_AIM_OFFSET");
    std::mt19937 rng(0xe06e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
