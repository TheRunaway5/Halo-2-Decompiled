// Native TU1.5 datum claim (16E7F0) against the unmodified translated body of
// the UI script startup unit (the validator repeats it with the campaign
// runtime unit's, where 16E860 goes through the dispatch), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16E860 is a hash-driven stub of its complete entry ABI: it writes the
// datum's salt and scratch, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy ESI or EDI change or the stack is
// left unbalanced. Fixtures author indexes negative, within and past the
// limit (indices past 16 and 32 under the limit 0x40; the limit 0x80000000
// is negative), free and taken datums,
// and high marks below and above the index;
// one region removed injects a fault. Modes: <cases> [census|verify]. Prints
// a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/native_tu15_datum_claim.hpp"
using namespace frame_misc_harness;
namespace claim=halo2::native_datum_claim_detail;
namespace {
constexpr std::uint32_t arrays=0x10000000u,datums=0x10010000u,bitmaps=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x16e860u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x16e861u,s.gp[6],2,std::uint32_t(rng()));
    w(s.memory,s.observe,0x16e862u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=datums+0x20u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%8);
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
static void run_16e860(WorldState &s){stub(s);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16e860u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
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
    for(std::uint32_t at=arrays;at<arrays+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(arrays+0x20u,pick(3)==0?0x40u:pick(4)==0?0x80000000u:8u);m.put(arrays+0x24u,0x20u);m.put(arrays+0x38u,pick(10));
    m.put(arrays+0x3cu,pick(8));m.put(arrays+0x44u,datums);m.put(arrays+0x48u,bitmaps);
    for(std::uint32_t at=datums;at<datums+0x500u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<40;++k)m.put(datums+0x20u*k,pick(2)?0u:std::uint32_t(rng()),2);
    for(std::uint32_t at=bitmaps;at<bitmaps+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    // Indices past 16 and 32 too, under the large limit.
    c.abi.gp[2]=arrays;c.abi.gp[7]=pick(6)==0?0xfffffff0u+pick(16):pick(3)==0?pick(40):pick(10);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{arrays,datums,bitmaps};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return claim::bridge(s,[](auto &v){original::native_frame_misc_reference_16e7f0(v);},[](auto &v){original::run_16e860(v);});}
void reference(WorldState &s){original::run_16e7f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DATUM_CLAIM");
    std::mt19937 rng(0x16e7f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,claim::coverage().load(),unsigned(claim::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
