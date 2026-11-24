// Native TU1.5 rotation product (1459B0) against the unmodified translated
// body of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author factors apart from the output,
// either or both the output itself, overlapping, with small, large,
// denormal, infinite and NaN elements, and the direction flag set; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_rotation_product.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_rotation_product_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t matrices=0x10000000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    static constexpr std::uint32_t specials[]{0x7fc00001u,0xffc00002u,0x7f800000u,0x00000003u,0x80000000u,0x7f7fffffu};
    for(std::uint32_t at=matrices-0x40u;at<matrices+0x100u;at+=4)
        m.put(at,pick(10)?bits_of(float(int(pick(41))-20)*0.125f):specials[pick(6)]);
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x140000u+pick(0x100));
    random_abi(rng,c,stack);
    const auto out=matrices+0x24u*pick(4);
    c.abi.gp[0]=out;
    c.abi.gp[2]=pick(3)==0?out:matrices+0x24u*pick(4)+4u*pick(2);
    c.abi.gp[1]=pick(3)==0?out:matrices+0x24u*pick(4);
    if(shape==15)m.drop(matrices);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1459b0(v);});}
void reference(WorldState &s){original::run_1459b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROTATION_PRODUCT");
    std::mt19937 rng(0x1459b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
