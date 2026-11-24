// Native TU1.5 object allocation (BE2B0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16E6A0, 1910A0 and 16E8A0 (through the unit's dispatch) are hash-driven
// stubs of their complete entry ABIs and arguments: 16E6A0 answers a handle
// or NONE, 1910A0 fills the slot with memory and answers AL; all write
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author sizes of every remainder and both clear
// directions; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_allocate.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_allocate_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,pool=0x10020000u,memory=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const bool granted=rng()%4!=0;
    if(target==0x1910a0u && granted)w(s.memory,s.observe,target,r(s.memory,s.gp[4]+4u),4,memory+0x100u*std::uint32_t(rng()%4)+(rng()%5==0?0x80u:0u));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x16e6a0u?(rng()%5?(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu):
            target==0x1910a0u?(std::uint32_t(rng())&0xffffff00u)|(granted?1u:0u):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());if(target!=0x16e6a0u || rng()%5==0)s.gp[2]=std::uint32_t(rng());
    if(target==0x1910a0u) {
        if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng()%64);
        if(policy&2u && rng()%11==0)s.gp[5]=datums+12u*std::uint32_t(rng()%4)+8u;
        if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
        if(policy&2u && rng()%11==0)s.gp[7]=datums+12u*std::uint32_t(rng()%4);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(target!=0x16e6a0u && policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_16e6a0(WorldState &s){stub(s,0x16e6a0u,0);}
static void run_1910a0(WorldState &s){stub(s,0x1910a0u,3);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16e8a0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,0);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const bool backward=shape==6;
    m.put(0x4ecbd0u,headers);m.put(0x4eabbcu,pool);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t at=datums;at<datums+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=pool;at<pool+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=memory;at<memory+0x600u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb80000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|(pick(8)?pick(0x60):0x8000u+pick(4)));
    random_abi(rng,c,stack);
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,pool,memory};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_be2b0(v);},[](auto &v){original::run_16e6a0(v);},
        [](auto &v){original::run_1910a0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_be2b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_ALLOCATE");
    std::mt19937 rng(0xbe2b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
