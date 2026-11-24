// Native TU1.5 node matrices (BAC30) against the unmodified translated body, extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author objects with every matrix size
// (negative sizes too, as the original multiplies them unsigned) and both
// offsets; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_node_matrices.hpp"
using namespace frame_misc_harness;
namespace matrices=halo2::native_node_matrices_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,counts=0x10a00000u;
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
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x1000u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        const std::uint32_t size=pick(5)==0?std::uint32_t(rng()&0xffffu):0x34u*pick(40)+(pick(4)==0?pick(0x34):0u);
        m.put(object+0x114u,(std::uint32_t(rng()&0xffffu)<<16)|size);
    }
    for(std::uint32_t at=counts;at<counts+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    m.put(stack+4u,counts+4u*pick(16));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x1000u*3u,counts};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return matrices::bridge(s,[](auto &v){original::native_frame_misc_reference_bac30(v);});}
void reference(WorldState &s){original::run_bac30(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_NODE_MATRICES");
    std::mt19937 rng(0xbac30u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,matrices::coverage().load(),unsigned(matrices::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
