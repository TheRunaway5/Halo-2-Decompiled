// Native TU1.5 data array restore (16E510) against the unmodified translated
// body of the campaign runtime unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// strncpy (32A4E0, behind the unit's dispatch) is a hash-driven stub of its
// complete entry ABI: it copies the two bytes and returns with the caller
// cleaning; the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change. Fixtures author arrays
// with counts none, few and many, elements used and unused of each size
// (sizes not a multiple of 4 too), fill mode on and off, both string
// directions, and one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "runtime/native_tu15_data_array_restore.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_data_array_restore_detail;
namespace {
constexpr std::uint32_t arrays=0x10000000u,elements=0x10010000u,bitmaps=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto dest=r(s.memory,s.gp[4]+4u),src=r(s.memory,s.gp[4]+8u);
    w(s.memory,s.observe,0x32a4e1u,dest,2,r(s.memory,src,2));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=dest;s.gp[1]=0u;s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[6]=arrays+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=arrays+0x80u*std::uint32_t(rng()%2)+0x40u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x32a4e0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto size=4u*(1u+pick(6))+(pick(4)?0u:pick(4));
    for(std::uint32_t k=0;k<2;++k) {
        const auto a=arrays+0x80u*k;
        for(std::uint32_t at=a;at<a+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(a+0x24u,size);m.put(a+0x2au,(std::uint32_t(rng())&0xf7u)|(pick(2)?8u:0u),1);m.put(a+0x48u,bitmaps);
    }
    for(std::uint32_t at=elements;at<elements+0x400u;at+=4)m.put(at,pick(3)?std::uint32_t(rng()):0u);
    for(std::uint32_t at=bitmaps;at<bitmaps+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=arrays;c.abi.gp[0]=pick(8)?pick(20):0u-pick(3);c.abi.gp[1]=elements+0x100u;
    if(rng()%4==0)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{arrays,elements,bitmaps};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_16e510(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_16e510(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DATA_ARRAY_RESTORE");
    std::mt19937 rng(0x16e510u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
