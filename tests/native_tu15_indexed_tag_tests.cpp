// Native TU1.5 indexed tag reference (1CEE60) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author references with the index NONE,
// within, at and past the count (negative too), and entries naming a tag or
// NONE; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_indexed_tag.hpp"
using namespace frame_misc_harness;
namespace indexed=halo2::native_indexed_tag_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t references=0x10000000u,tags=0x10180000u,definitions=0x10200000u,blocks=0x10300000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k) {
        const auto tag_index=0x1000u*pick(2)+k;
        const auto definition=definitions+0x1000u*k+4u*pick(4);
        m.put(tags+16u*tag_index+8u,definition);
        m.put(tags+16u*(tag_index^0x1000u)+8u,definition+0x100u);
        for(std::uint32_t at=definition;at<definition+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x4cu,pick(5)==0?0xffffffffu:pick(6));
        const auto block=blocks+0x1000u*k+4u*pick(4);
        m.put(definition+0x50u,block);
        for(std::uint32_t e=0;e<6;++e) {
            for(std::uint32_t at=block+32u*e;at<block+32u*e+32u;at+=4)m.put(at,std::uint32_t(rng()));
            m.put(block+32u*e+4u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())<<16)|(0x1000u*pick(2)+pick(4)));
        }
    }
    for(std::uint32_t at=references;at<references+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto reference_address=references+4u*pick(8);
    m.put(reference_address,(std::uint32_t(rng())<<16)|(0x1000u*pick(2)+pick(4)));
    m.put(reference_address+4u,pick(4)==0?0xffffu:pick(6)==0?0x8000u+pick(4):pick(7),2);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=reference_address;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{references,tags,definitions+0x1000u,blocks+0x2000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return indexed::bridge(s,[](auto &v){original::native_frame_misc_reference_1cee60(v);});}
void reference(WorldState &s){original::run_1cee60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_INDEXED_TAG");
    std::mt19937 rng(0x1cee60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,indexed::coverage().load(),unsigned(indexed::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
