// Native TU1.5 object axes (BBFF0) against the unmodified translated body of
// the physics live support unit (the validator repeats it with the AI frame
// unit's), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author objects with and
// without parents (node indices, signed node offsets), axes and matrices
// (special floats too), outputs present or not and overlapping the object;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_axes.hpp"
using namespace frame_misc_harness;
namespace axes=halo2::native_object_axes_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,outputs=0x10200000u;
std::uint32_t real(std::mt19937 &rng) {
    static constexpr std::uint32_t special[]{0u,0x80000000u,0x7fc00000u,0x7f800000u,0x00000001u,0x3f800000u};
    return rng()%7==0?special[rng()%6]:std::bit_cast<std::uint32_t>(float(int(rng()%4001)-2000)/1000.0f);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x1000u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,real(rng));
        m.put(object+0x14u,pick(3)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        m.put(object+0x18u,pick(8)==0?0xffu:pick(6),1);
        m.put(object+0x116u,0x200u+4u*pick(16),2);
    }
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));
    random_abi(rng,c,stack);
    const std::uint32_t object=pick(4);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|object;
    c.abi.gp[6]=pick(4)==0?0u:shape==7?objects+0x1000u*object+0x7cu:outputs+4u*pick(4);
    c.abi.gp[7]=pick(4)==0?0u:outputs+0x20u+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x1000u*2u,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return axes::bridge(s,[](auto &v){original::native_frame_misc_reference_bbff0(v);});}
void reference(WorldState &s){original::run_bbff0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_AXES");
    std::mt19937 rng(0xbbff0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,axes::coverage().load(),unsigned(axes::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
