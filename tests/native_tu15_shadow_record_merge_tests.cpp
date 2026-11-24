// Native TU1.5 shadow record merge (4A7B0) against the unmodified translated
// body of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The routine calls no child. Fixtures author caches of every count up to
// full, records stamped this frame or not, and casters whose keys match
// records, repeat, or are new, in counts below four, at four and past it,
// with the direction flag either way; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_shadow_record_merge.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_shadow_record_merge_detail;
namespace {
constexpr std::uint32_t cache=0x10000000u,casters=0x10001000u;
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
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    const auto stamp=std::uint32_t(rng());
    m.put(0x4c6854u,stamp);
    for(std::uint32_t at=cache;at<cache+0x490u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto records=chance(6)?0u:chance(5)?0x20u:pick(0x21u);
    m.put(cache+0x480u,records);
    for(std::uint32_t k=0;k<0x20u;++k) {
        m.put(cache+0x24u*k+4u,pick(12));
        m.put(cache+0x24u*k+0x1cu,chance(2)?stamp:std::uint32_t(rng()));
    }
    const auto count=chance(8)?0u:chance(3)?pick(4):4u+pick(12);
    for(std::uint32_t at=casters;at<casters+0x14u*16u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<16;++k)m.put(casters+0x14u*k+4u,pick(16));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x4a000u+pick(0x100));m.put(stack+4u,casters);m.put(stack+8u,count);
    random_abi(rng,c,stack);
    c.abi.gp[0]=cache;
    if(shape==15) {
        static constexpr std::uint32_t areas[]{cache,casters};
        m.drop(areas[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_4a7b0(v);});}
void reference(WorldState &s){original::run_4a7b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHADOW_RECORD_MERGE");
    std::mt19937 rng(0x4a7b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
