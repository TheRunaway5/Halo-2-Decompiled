// Native TU1.5 object frame (BC190) against the unmodified translated body
// of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 144BF0 and 145560 are hash-driven stubs of their complete entry ABIs and
// arguments: each writes the frame and scratch and pops its arguments (none
// and 4 bytes), the caller-saved registers, XMM, flags and (by policy) MXCSR
// are random, and by policy EBX, EBP or EDI change or the stack is left
// unbalanced. Fixtures author objects with and without parents and node
// indexes and node offsets of either sign; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_frame.hpp"
using namespace frame_misc_harness;
namespace object_frame=halo2::native_object_frame_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,objects=0x10020000u,frames=0x10030000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,frames+4u*std::uint32_t(rng()%32),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=frames+4u*std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[5]=header;
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x200u*std::uint32_t(rng()%3);
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
static void run_144bf0(WorldState &s){stub(s,0x144bf0u,0);}
static void run_145560(WorldState &s){stub(s,0x145560u,1);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x200u*(k%3));}
    for(std::uint32_t o=0;o<3;++o) {
        const auto object=objects+0x200u*o;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,pick(3)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        m.put(object+0x18u,pick(4),1);
        m.put(object+0x116u,pick(4)==0?0x10000u-0x40u:0x40u+4u*pick(8),2);
    }
    for(std::uint32_t at=frames;at<frames+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xbc0000u+pick(0x100));m.put(stack+4u,frames+4u*pick(8));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return object_frame::bridge(s,[](auto &v){original::native_frame_misc_reference_bc190(v);},
        [](auto &v){original::run_144bf0(v);},[](auto &v){original::run_145560(v);});
}
void reference(WorldState &s){original::run_bc190(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_FRAME");
    std::mt19937 rng(0xbc190u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,object_frame::coverage().load(),unsigned(object_frame::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
