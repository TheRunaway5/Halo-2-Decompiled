// Native TU1.5 object origin (30C40) against the unmodified translated body
// of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author objects and outputs (the point
// overlapping the object or the word pointer's target, the word pointer
// anywhere); one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_object_origin.hpp"
using namespace frame_misc_harness;
namespace origin=halo2::native_object_origin_detail;
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
        const auto object=objects+0x100u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x30000u+pick(0x100));
    const std::uint32_t object=pick(4);
    const auto point=shape==7?objects+0x100u*object+0x34u:outputs+4u*pick(8);
    m.put(stack+4u,shape==8?point+4u:outputs+0x20u+4u*pick(8));
    random_abi(rng,c,stack);
    c.abi.gp[1]=(std::uint32_t(rng())<<16)|object;c.abi.gp[0]=point;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x100u*2u,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return origin::bridge(s,[](auto &v){original::native_frame_misc_reference_30c40(v);});}
void reference(WorldState &s){original::run_30c40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_ORIGIN");
    std::mt19937 rng(0x30c40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,origin::coverage().load(),unsigned(origin::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
