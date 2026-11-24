// Native TU1.5 object root query (BBD50) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author parent chains (none, one to four
// links, ending in NONE) over objects whose headers and words vary, and
// NONE handles (the original's index 0xFFFF lookup is authored too); one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_root_flag.hpp"
using namespace frame_misc_harness;
namespace root=halo2::native_root_flag_detail;
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
    // Objects 0..7 (and the NONE lookup's 0xFFFF) form chains toward higher indices.
    const auto put_object=[&](std::uint32_t index,std::uint32_t parent) {
        const auto object=objects+0x100u*(index&0xffu);
        m.put(header_data+12u*index,std::uint32_t(rng()));m.put(header_data+12u*index+4u,std::uint32_t(rng()));
        m.put(header_data+12u*index+8u,object);
        for(std::uint32_t at=object;at<object+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x14u,parent);
    };
    for(std::uint32_t k=0;k<8;++k)put_object(k,k<7 && pick(2)?(std::uint32_t(rng())<<16)|(k+1u+pick(7u-k)):0xffffffffu);
    put_object(0xffffu,0xffffffffu);
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto handle=pick(8)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(8);
    c.abi.gp[0]=handle;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return root::bridge(s,[](auto &v){original::native_frame_misc_reference_bbd50(v);});}
void reference(WorldState &s){original::run_bbd50(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_ROOT");
    std::mt19937 rng(0xbbd50u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,root::coverage().load(),unsigned(root::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
