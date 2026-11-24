// Native TU1.5 model switch (1D3930) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author records with each model present
// or not (handle or index NONE), random flags, distances below, at and past
// the threshold and the near bound (NaN too), and the switch on or off; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_model_switch.hpp"
using namespace frame_misc_harness;
namespace model_switch=halo2::native_model_switch_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10000000u;
std::uint32_t random_float(std::mt19937_64 &rng) {
    switch(rng()%7) {
    case 0:return 0u;
    case 1:return 0x7fc00000u;
    case 2:return std::bit_cast<std::uint32_t>(4.0f);
    case 3:return std::bit_cast<std::uint32_t>(1.0f);
    default:return std::bit_cast<std::uint32_t>(float(std::int32_t(rng()%1001))/100.0f);
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records+0x0u,pick(4)==0?0xffffffffu:std::uint32_t(rng()));m.put(records+0x6u,pick(4)==0?0xffffu:pick(8),2);
    m.put(records+0x20u,pick(4)==0?0xffffffffu:std::uint32_t(rng()));m.put(records+0x26u,pick(4)==0?0xffffu:pick(8),2);
    m.put(records+0x80u,random_float(floats));
    m.put(0x47c1b4u,pick(4)==0?0u:1u+pick(255),1);
    m.put(0x46a330u,pick(4)?std::bit_cast<std::uint32_t>(4.0f):random_float(floats));
    m.put(0x46a234u,pick(4)?std::bit_cast<std::uint32_t>(1.0f):random_float(floats));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records;
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s){return model_switch::bridge(s,[](auto &v){original::native_frame_misc_reference_1d3930(v);});}
void reference(WorldState &s){original::run_1d3930(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MODEL_SWITCH");
    std::mt19937 rng(0x1d3930u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,model_switch::coverage().load(),unsigned(model_switch::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
