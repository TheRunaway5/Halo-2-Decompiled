// Native TU1.5 object flag bit (E9530) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author the object header array and
// objects whose flag bit blocks lie before or after them (negative and
// positive int16 offsets), bits across several words (negative bits too, as
// the original shifts arithmetically), and handles with any high word; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_flag_bit.hpp"
using namespace frame_misc_harness;
namespace flag_bit=halo2::native_object_flag_bit_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u;
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
    for(std::uint32_t k=0;k<16;++k) {
        const auto object=objects+0x1000u*k+0x800u;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x800u;at<object+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
        const std::int32_t offset=pick(3)==0?-std::int32_t(0x40u+4u*pick(0x100)):std::int32_t(0x350u+4u*pick(0x80));
        m.put(object+0x344u,(std::uint32_t(std::uint16_t(offset))<<16)|(std::uint32_t(rng())&0xffffu));
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(16);
    // Bits in the first three words, negative bits, and far bits (faults).
    c.abi.gp[1]=shape==3?std::uint32_t(-std::int32_t(1u+pick(8))):pick(4)==0?std::uint32_t(rng())&0x80000fffu:pick(96);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x1000u*3u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return flag_bit::bridge(s,[](auto &v){original::native_frame_misc_reference_e9530(v);});}
void reference(WorldState &s){original::run_e9530(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_FLAG_BIT");
    std::mt19937 rng(0xe9530u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,flag_bit::coverage().load(),unsigned(flag_bit::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
