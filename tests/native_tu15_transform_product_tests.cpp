// Native TU1.5 transform product (145560) against the unmodified translated
// body of the scene device constructor unit (and its variants in physics live
// support, AI frame and effect particle frame), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Fixtures author separate and aliased (out == left, out == right, all three
// equal) 0x34-byte transforms with regular, subnormal, infinite, signed-zero
// and quiet/signaling NaN floats across all MXCSR rounding/FTZ/DAZ modes; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_transform_product.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_transform_product_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
template<class M> __m128 native_load_half(M &memory,std::uint32_t address,__m128 value,unsigned offset) {
    std::array<std::byte,16> bytes;std::memcpy(bytes.data(),&value,16);
    memory.read(address,std::span(bytes).subspan(offset,8));
    std::memcpy(&value,bytes.data(),16);return value;
}
void native_store_half(WorldState &s,std::uint32_t instruction,std::uint32_t address,__m128 value,unsigned offset) {
    std::uint64_t bits;std::memcpy(&bits,reinterpret_cast<const std::byte*>(&value)+offset,8);
    w(s.memory,s.observe,instruction,address,8,bits);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t left_mem=0x10000000u,right_mem=0x10001000u,out_mem=0x10002000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    std::mt19937_64 frng(rng());
    const auto put_transform=[&](std::uint32_t base) {
        for(std::uint32_t off=0;off<0x34u;off+=4) {
            const std::uint32_t v=chance(4)?special_float(frng):std::uint32_t(rng());
            m.put(base+off,v);
        }
    };
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    put_transform(left_mem);put_transform(right_mem);put_transform(out_mem);
    for(std::uint32_t at=stack-0x20u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t left=left_mem,right=right_mem,out=out_mem;
    if(shape==1)out=left;
    else if(shape==2)out=right;
    else if(shape==3){right=left;out=left;}
    m.put(stack,0x145000u+pick(0x100));
    m.put(stack+4u,out);
    random_abi(rng,c,stack);
    c.abi.gp[1]=left;
    c.abi.gp[2]=right;
    if(shape==15) {
        static constexpr std::uint32_t areas[]{left_mem,right_mem,out_mem};
        m.drop(areas[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_145560(v);});}
void reference(WorldState &s){original::run_145560(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TRANSFORM_PRODUCT");
    std::mt19937 rng(0x145560u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
