// Native TU1.5 lens request (2DB80) against the unmodified translated body
// of the lens flare unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 2E210, 20990 and 20B20 are hash-driven stubs of
// their complete entry ABIs and arguments: 2E210 writes the projected lens
// and pops four arguments, 20990 answers a queued lens (or NONE) and pops
// two, 20B20 pops none; all write scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX or ESI change
// or the stack is left unbalanced. Fixtures author views and lenses (NONE
// too), brightness and opacity at, below and above zero (NaN too), every
// kind and kind flag, ranges equal or not with positions within and past
// them, and reflections or none; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_lens_request.hpp"
using namespace frame_misc_harness;
namespace lens=halo2::native_lens_request_detail;
namespace {
constexpr std::uint32_t tags=0x10000000u,definitions=0x10010000u,vectors=0x10020000u,scratch=0x10600000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%8) {
    case 0:return 0u;
    case 1:return 0x80000000u;
    case 2:return 0x7fc00000u;
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%4001)-1000)/100.0f);
    }
}
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x2e210u)for(std::uint32_t k=0;k<6;++k)w(s.memory,s.observe,target+1u,s.gp[6]+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x20990u?(rng()%4==0?0xffffffffu:std::uint32_t(rng()%64)):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[3]=definitions+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[6]=std::uint32_t(rng()%64);
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
static void run_2e210(WorldState &s){stub(s,0x2e210u,4);}
static void run_20990(WorldState &s){stub(s,0x20990u,2);}
static void run_20b20(WorldState &s){stub(s,0x20b20u,0);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4c66f4u,pick(8)==0?0xffffffffu:pick(4));
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k) {
        for(std::uint32_t at=tags+16u*k;at<tags+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tags+16u*k+8u,definitions+0x40u*(k%2));
    }
    for(std::uint32_t d=0;d<2;++d) {
        const auto definition=definitions+0x40u*d;
        for(std::uint32_t at=definition;at<definition+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x16u,pick(2)?0u:1u+pick(3),2);
        const auto end=random_float(floats);
        m.put(definition+0x18u,pick(3)==0?end:random_float(floats));m.put(definition+0x1cu,end);
        m.put(definition+0x28u,pick(3)==0?std::uint32_t(rng()):0u,1);
        m.put(definition+0x2au,pick(2)?std::uint32_t(rng()):0u,1);
    }
    for(std::uint32_t at=vectors;at<vectors+0x40u;at+=4)m.put(at,random_float(floats));
    for(std::uint32_t at=0x4c65c0u;at<0x4c65d8u;at+=4)m.put(at,random_float(floats));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x2d0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())&~0xffu)|(pick(2)?0u:1u+pick(255)));
    m.put(stack+0x14u,vectors+4u*pick(4));m.put(stack+0x18u,vectors+0x20u+4u*pick(4));
    for(std::uint32_t k=7;k<=9;++k)m.put(stack+4u*k,pick(4)==0?random_float(floats):std::bit_cast<std::uint32_t>(0.5f+float(pick(8))));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(8)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4);c.abi.gp[7]=vectors+0x30u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{tags,definitions,vectors,0x4c6000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return lens::bridge(s,[](auto &v){original::native_frame_misc_reference_2db80(v);},[](auto &v){original::run_2e210(v);},
        [](auto &v){original::run_20990(v);},[](auto &v){original::run_20b20(v);});
}
void reference(WorldState &s){original::run_2db80(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LENS_REQUEST");
    std::mt19937 rng(0x2db80u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,lens::coverage().load(),unsigned(lens::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
