// Native TU1.5 role check (17E100) against the unmodified translated body of
// the death cycle unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). There are no children. Fixtures author owners
// asking for either role, records whose roles match or not, one or several
// local players, and every second role and side; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_role_check.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_role_check_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t owners=0x10000000u,records=0x10010000u,locals=0x10020000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f0u,locals+4u*pick(4));
    for(std::uint32_t at=locals;at<locals+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(r(m,0x4f54f0u)+8u,pick(3)?1u:pick(5),2);
    for(std::uint32_t at=owners;at<owners+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto owner_address=owners+4u*pick(8)+pick(4);
    const auto record=records+4u*pick(16)+2u*pick(2);
    m.put(record+0x10u,pick(4)?1u+pick(2):pick(0x10000u),2);
    m.put(record+0x12u,pick(4)?1u+pick(2):pick(5),2);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=owner_address;c.abi.gp[1]=record;c.abi.gp[6]=pick(3)?pick(2):std::uint32_t(rng());
    if(shape==15) {
        static constexpr std::uint32_t regions[]{owners,records,locals};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_17e100(v);});}
void reference(WorldState &s){original::run_17e100(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROLE_CHECK");
    std::mt19937 rng(0x17e100u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
