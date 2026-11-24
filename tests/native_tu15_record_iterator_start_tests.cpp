// Native TU1.5 record iterator start (1D3F90) against the unmodified
// translated body of the sandbox special unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5250 is a hash-driven stub of its complete entry ABI: it writes the
// iterator, returns random caller-saved registers, XMM, flags and (by
// policy) MXCSR, a moved callee-saved register or an unbalanced stack.
// Fixtures author sources and iterators (overlapping too); one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_record_iterator_start.hpp"
using namespace frame_misc_harness;
namespace start=halo2::native_record_iterator_start_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,tags=0x10010000u,sources=0x10100000u,iterators=0x10200000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,iterators+4u*std::uint32_t(rng()%32),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[rng()%2?3:6]=std::uint32_t(rng());
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1e5250u)throw std::runtime_error("Unbound dispatch target");
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
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<8;++k) {
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x100u*(k%4));m.put(instances+16u*k+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=sources;at<sources+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(sources+0x80u*k+0x68u,(std::uint32_t(rng())<<16)|pick(8));
    for(std::uint32_t at=iterators;at<iterators+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto source=sources+0x80u*pick(4);
    c.abi.gp[0]=source;c.abi.gp[1]=shape==7?source+0x6cu:iterators+4u*pick(16);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,sources,iterators};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return start::bridge(s,[](auto &v){original::native_frame_misc_reference_1d3f90(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1d3f90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_ITERATOR_START");
    std::mt19937 rng(0x1d3f90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,start::coverage().load(),unsigned(start::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
