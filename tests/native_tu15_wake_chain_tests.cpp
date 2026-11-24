// Native TU1.5 wake chain (B9390) against the unmodified translated body of
// the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author the object header array (headers
// awake or not), objects whose parent chains are empty, short or long
// (handles with any high word, NONE handles); one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_wake_chain.hpp"
using namespace frame_misc_harness;
namespace wake=halo2::native_wake_chain_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,none=0xffffffffu;
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
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
    // Headers 0..15 and the NONE index's header (index FFFF).
    for(std::uint32_t k=0;k<16;++k) {
        const auto object=objects+0x40u*k;
        m.put(header_data+12u*k,std::uint32_t(rng())&(pick(3)?0xfffdffffu:0xffffffffu));
        m.put(header_data+12u*k+4u,std::uint32_t(rng()));m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,pick(3)==0||k==0?none:pick(8)==0?0u:(std::uint32_t(rng()%0x7fff)<<16)|pick(k));
    }
    const auto far=header_data+12u*0xffffu;
    for(std::uint32_t at=far;at<far+12u;at+=4)m.put(at,std::uint32_t(rng())&(pick(2)?0xfffdffffu:0xffffffffu));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(6)==0?none:(std::uint32_t(rng()%0x7fff)<<16)|pick(16);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return wake::bridge(s,[](auto &v){original::native_frame_misc_reference_b9390(v);});}
void reference(WorldState &s){original::run_b9390(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WAKE_CHAIN");
    std::mt19937 rng(0xb9390u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,wake::coverage().load(),unsigned(wake::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
