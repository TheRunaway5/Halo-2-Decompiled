// Native TU1.5 kind entry (11E060) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author owners (counts zero, negative, one,
// more), kinds within and past the table, and the table (unbound targets
// too); one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_kind_entry.hpp"
using namespace frame_misc_harness;
namespace kind_entry=halo2::native_kind_entry_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's kind table targets, as its canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x11e070u:case 0x11e074u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t owners=0x10000000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=owners;at<owners+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto owner=owners+0x10u*pick(16);
    static constexpr std::uint32_t counts[]{0u,1u,2u,3u,0xffffffffu,0x80000000u,7u,1u};
    m.put(owner+0x2a8u,counts[pick(8)]);
    for(std::uint32_t k=0;k<4;++k)m.put(0x11e09cu+4u*k,shape==13 && pick(3)==0?0x11e000u+pick(0x100):pick(2)?0x11e070u:0x11e074u);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=owner;c.abi.gp[2]=pick(3)?pick(4):std::uint32_t(rng());
    if(shape==15)m.drop(pick(2)?owners:0x11e09cu);
    return c;
}
bool native(WorldState &s){return kind_entry::bridge(s,[](auto &v){original::native_frame_misc_reference_11e060(v);});}
void reference(WorldState &s){original::run_11e060(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_KIND_ENTRY");
    std::mt19937 rng(0x11e060u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,kind_entry::coverage().load(),unsigned(kind_entry::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
