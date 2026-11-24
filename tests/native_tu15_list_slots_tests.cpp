// Native TU1.5 list slots (1AFA90) against the unmodified translated body of
// the AI actor unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). A leaf with a jump table: fixtures author lists
// (none, empty, short and long) of entries of every kind (and kinds past 4),
// ids that match the current id or the next default's, defaults plenty and
// few, table entries for the five cases, a bound entry that is no case and
// an unbound one, and one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/ai/native_tu15_list_slots.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_list_slots_detail;
namespace {
constexpr std::uint32_t lists=0x10000000u,entries=0x10010000u,defaults=0x10020000u,slots=0x10030000u;
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's table targets (and one bound target that is no case), as its canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    using namespace halo2::native_list_slots::targets;
    for(const auto t:{same_id,never,default_id,when_spent,always})if(t==p)return p;
    if(p==0x1afb07u)return p;
    throw std::runtime_error("Unbound TU1.5 contact dispatch target");
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    using namespace halo2::native_list_slots::targets;
    static constexpr std::uint32_t targets[]{same_id,never,default_id,when_spent,always};
    for(std::uint32_t k=0;k<5;++k)m.put(0x1afb48u+4u*k,pick(40)?targets[pick(5)]:pick(2)?0x1afb07u:0x1234u);
    m.put(0x47c1a4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    const auto count=pick(4)?pick(60):0u;
    for(std::uint32_t at=lists;at<lists+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(lists+4u,entries);m.put(lists+8u,count|(std::uint32_t(rng())&0xffff0000u));
    for(std::uint32_t k=0;k<64;++k) {
        const auto e=entries+24u*k;
        for(std::uint32_t at=e;at<e+24u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(e,pick(12)?pick(5):pick(2)?0xffffu:5u+pick(4),2);m.put(e+2u,pick(4),2);
    }
    const auto default_count=pick(3)?pick(60):pick(4);
    for(std::uint32_t k=0;k<64;++k){for(std::uint32_t at=defaults+20u*k;at<defaults+20u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(defaults+20u*k,pick(4),2);}
    for(std::uint32_t at=slots;at<slots+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    m.put(stack+4u,slots);m.put(stack+8u,pick(8)?lists:0u);m.put(stack+12u,defaults);
    m.put(stack+16u,default_count|(std::uint32_t(rng())&0xffff0000u));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lists,entries,defaults,slots};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1afa90(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);});
}
void reference(WorldState &s){original::run_1afa90(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LIST_SLOTS");
    std::mt19937 rng(0x1afa90u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
