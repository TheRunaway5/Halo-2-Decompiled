// Native TU1.5 entry kind check (1D9F50) against the unmodified translated body, extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The virtual kind function (the unit's dispatch route, at the address the
// interface's table names) is a genuine stand-in whose effects are a hash
// of its complete entry ABI and return address: it returns a kind (often 6),
// ECX/EDX, flags and XMM, writes scratch, and occasionally clobbers
// callee-saved registers or leaves the stack unbalanced. Fixtures author
// lists with negative, in-range and past-the-end indices, entries whose
// objects are busy or not; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_entry_kind.hpp"
using namespace frame_misc_harness;
namespace kind=halo2::native_entry_kind_detail;
namespace {
constexpr std::uint32_t lists=0x10000000u,entries=0x10100000u,objects=0x10200000u,interfaces=0x10300000u,
    tables=0x10400000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    
    s.gp[0]=rng()%3==0?6u:std::uint32_t(rng()%8);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==7?lists+0x80u*std::uint32_t(rng()%4):which==6?0x60u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target,0);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t k=0;k<4;++k) {
        const auto list=lists+0x80u*k;
        for(std::uint32_t at=list;at<list+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(list+0x18u,(std::uint32_t(rng())&0xffffff00u)|(pick(5)==0?0x80u|pick(0x80):pick(8)));
        m.put(list+0x70u,entries);m.put(list+0x74u,pick(9));
    }
    for(std::uint32_t k=0;k<8;++k) {
        const auto entry=entries+0x60u*k;
        for(std::uint32_t at=entry;at<entry+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto object=objects+0x80u*k;
        m.put(entry+0x40u,object);
        for(std::uint32_t at=object;at<object+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x3cu,interfaces+0x10u*k);m.put(object+0x40u,(std::uint32_t(rng())&0xffffff00u)|(pick(3)?0u:pick(256)));
        m.put(interfaces+0x10u*k,tables+0x40u*k);
        for(std::uint32_t at=tables+0x40u*k;at<tables+0x40u*(k+1u);at+=4)m.put(at,0x2a0000u+0x10u*pick(16));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=lists+0x80u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lists,entries,objects,interfaces,tables};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return kind::bridge(s,[](auto &v){original::native_frame_misc_reference_1d9f50(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1d9f50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ENTRY_KIND");
    std::mt19937 rng(0x1d9f50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,kind::coverage().load(),unsigned(kind::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
