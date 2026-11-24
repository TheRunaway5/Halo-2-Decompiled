// Native TU1.5 body matrix (1D8B90) against the unmodified translated body of
// the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 2E4650 (direct) is a hash-driven stub of its complete entry ABI: it
// rewrites the stack matrix (ECX) from a hash of it and of the offset it was
// given, and returns with RET 4; the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBP (another authored frame) or
// ESI (another output) change or the stack is left unbalanced. Fixtures
// author models with and without the offset bit, body indices 0 to 7, rows
// aligned and not, and finite, infinite and NaN components; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_body_matrix.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_body_matrix_detail;
namespace {
constexpr std::uint32_t models=0x10000000u,bodies=0x10010000u,motions=0x10020000u,outputs=0x10030000u,frames=0x10040000u,
    rigid=0x10050000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    for(std::uint32_t k=0;k<16;++k)h=mix(h,r(s.memory,s.gp[1]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    for(std::uint32_t k=0;k<16;++k)if(rng()%3)w(s.memory,s.observe,0x2e4684u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%7==0)s.gp[5]=frames+0x40u+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%7==0)s.gp[6]=outputs+0x40u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%11==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_2e4650(WorldState &s){stub(s,0x2e4650u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t component(std::mt19937 &rng) {
    switch(rng()%12) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    case 2:return 0u;
    default:{float f=float(int(rng()%2001)-1000)/37.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    float one=pick(6)?1.0f:0.5f;std::uint32_t one_bits;std::memcpy(&one_bits,&one,4);m.put(0x46a0d0u,one_bits);
    for(std::uint32_t at=models;at<models+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto model=models+0x80u*k;
        m.put(model+4u,std::uint32_t(rng())&~0x40u|(pick(2)?0x40u:0u));m.put(model+0x70u,bodies+0x100u*pick(2));
    }
    for(std::uint32_t at=bodies;at<bodies+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t table=0;table<2;++table)for(std::uint32_t k=0;k<8;++k)m.put(bodies+0x100u*table+0x60u*k+0x40u,rigid+0x80u*pick(4));
    for(std::uint32_t at=rigid;at<rigid+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(rigid+0x80u*k+0x3cu,motions+0x100u*pick(4)+(pick(6)?0u:4u*pick(4)));
    for(std::uint32_t at=motions;at<motions+0x500u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=outputs;at<outputs+0x140u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=frames;at<frames+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0xc0u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(8);c.abi.gp[1]=models+0x80u*pick(2);c.abi.gp[6]=outputs+0x40u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{models,bodies,rigid,motions,outputs};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1d8b90(v);},[](auto &v){original::run_2e4650(v);});
}
void reference(WorldState &s){original::run_1d8b90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BODY_MATRIX");
    std::mt19937 rng(0x1d8b90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
