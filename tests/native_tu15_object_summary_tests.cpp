// Native TU1.5 object summary (BA910) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 16E290 is a hash-driven stub of its complete entry ABI (EAX the handle):
// it answers AX, writes scratch or the summary, the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy ESI or EDI move
// or the stack is left unbalanced. Fixtures author objects with and without
// state bits (negative types too) and summaries overlapping the object; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_summary.hpp"
using namespace frame_misc_harness;
namespace summary=halo2::native_object_summary_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,outputs=0x10200000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:s.gp[7]+4u*std::uint32_t(rng()%5),4,
      std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=objects+0x400u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[7]=outputs+4u*std::uint32_t(rng()%8);
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
static void run_16e290(WorldState &s){stub(s,0x16e290u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto index=pick(4);
    c.abi.gp[2]=(std::uint32_t(rng())<<16)|index;
    c.abi.gp[7]=shape==7?objects+0x400u*index+4u*index+0x40u:outputs+4u*pick(8);   // over the object's vector
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u*2u,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return summary::bridge(s,[](auto &v){original::native_frame_misc_reference_ba910(v);},[](auto &v){original::run_16e290(v);});}
void reference(WorldState &s){original::run_ba910(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_SUMMARY");
    std::mt19937 rng(0xba910u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,summary::coverage().load(),unsigned(summary::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
