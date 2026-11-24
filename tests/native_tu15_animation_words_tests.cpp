// Native TU1.5 animation words (111D10) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// AI frame unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author objects
// whose animation blocks (signed offsets) are in use or lack a graph, handle
// or index, and outputs (overlapping the block too); one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_words.hpp"
using namespace frame_misc_harness;
namespace words=halo2::native_animation_words_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,outputs=0x10200000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        // Object addresses with a nonzero low byte show AL's clear.
        const auto object=objects+0x800u*k+0x204u+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x200u;at<object+0x600u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto offset=pick(4)==0?0x10000u-0x100u:0x300u;
        m.put(object+0x12au,offset,2);
        const auto block=object+std::uint32_t(std::int32_t(std::int16_t(offset)));
        m.put(block+0x68u,pick(5)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(block,pick(5)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(block+6u,pick(5)==0?0xffffu:pick(64),2);
    }
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);c.abi.gp[6]=outputs+4u*pick(8);c.abi.gp[2]=outputs+0x20u+4u*pick(8);
    if(shape==7)c.abi.gp[2]=c.abi.gp[6];
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u*2u,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return words::bridge(s,[](auto &v){original::native_frame_misc_reference_111d10(v);});}
void reference(WorldState &s){original::run_111d10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_WORDS");
    std::mt19937 rng(0x111d10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,words::coverage().load(),unsigned(words::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
