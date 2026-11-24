// Native TU1.5 flagged slot (D2330) against the unmodified translated body
// of the weapon HUD unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author objects
// whose four slots are empty or hold objects with flagged or unflagged tags;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/weapons/native_tu15_flagged_slot.hpp"
using namespace frame_misc_harness;
namespace flagged=halo2::native_flagged_slot_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,instances=0x10200000u,
    tags=0x10210000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<8;++k) {
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x200u*(k%4));m.put(instances+16u*k+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=tags;at<tags+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(tags+0x200u*k+0x12cu,std::uint32_t(rng())&(pick(3)?~8u:~0u));
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|pick(8));
        for(std::uint32_t s=0;s<4;++s)m.put(object+0x218u+4u*s,pick(3)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(8));
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=(std::uint32_t(rng())<<16)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u*3u,instances,tags};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return flagged::bridge(s,[](auto &v){original::native_frame_misc_reference_d2330(v);});}
void reference(WorldState &s){original::run_d2330(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FLAGGED_SLOT");
    std::mt19937 rng(0xd2330u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,flagged::coverage().load(),unsigned(flagged::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
