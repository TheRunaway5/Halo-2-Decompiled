// Native TU1.5 record sort (4B150) against the unmodified translated body of
// the shadow frame unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author ranges
// that are empty, single and up to eight records, keys of each mode (float
// keys equal, signed, NaN; integer keys negative), the modes 0, 1 and others,
// and the direction flag set (the copies run backward); one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_record_sort.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_record_sort_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10010100u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr float keys[]{0.f,1.f,-1.f,2.5f,2.5f,-0.f,1e30f,-3.f};
    for(std::uint32_t at=records-0x100u;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<9;++k) {
        const auto record=records+36u*k;
        for(const auto offset:{0x10u,0x18u}) {
            float f=keys[pick(8)];
            std::uint32_t bits;std::memcpy(&bits,&f,4);
            if(pick(12)==0)bits=0x7fc00000u|pick(16);
            m.put(record+offset,bits);
        }
        m.put(record+8u,pick(3)?pick(6):0u-pick(4));
    }
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto first=pick(8)==0?0xffffffffu:pick(6);
    const auto last=pick(8)==0?first-pick(2):first+pick(6);
    m.put(stack,0x40000u+pick(0x100));m.put(stack+4u,records);m.put(stack+8u,pick(4)?pick(3):pick(2)?0xffffffffu:7u);m.put(stack+12u,last);
    random_abi(rng,c,stack);
    c.abi.gp[0]=first;
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_4b150(v);});}
void reference(WorldState &s){original::run_4b150(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_SORT");
    std::mt19937 rng(0x4b150u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
