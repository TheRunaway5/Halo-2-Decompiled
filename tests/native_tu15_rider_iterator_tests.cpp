// Native TU1.5 rider iterator (D2BD0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The routine calls no child. Fixtures author acyclic object trees (every
// child and sibling of a later index) of every type and riding bit, and
// iterators at the root or past it with pending children or none; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_rider_iterator.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_rider_iterator_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,entries=0x10001000u,bodies=0x10010000u,iterator=0x10002000u;
constexpr std::uint32_t count=10u;
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
    const auto handle=[&](std::uint32_t k){return (std::uint32_t(rng())&0xffff0000u)|k;};
    const auto later=[&](std::uint32_t k){return k+1u<count && !chance(3)?handle(k+1u+pick(count-k-1u)):0xffffffffu;};
    m.put(0x4ecbd0u,objects);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<count;++k) {
        m.put(entries+12u*k,std::uint32_t(rng()));m.put(entries+12u*k+4u,std::uint32_t(rng()));m.put(entries+12u*k+8u,bodies+0x200u*k);
        const auto o=bodies+0x200u*k;
        for(std::uint32_t at=o;at<o+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(o+4u,(std::uint32_t(rng())&~(1u<<26))|(chance(2)?1u<<26:0u));
        m.put(o+0xcu,k==0u?0xffffffffu:later(k));m.put(o+0x10u,later(k));m.put(o+0xaau,pick(5),1);
    }
    for(std::uint32_t at=iterator;at<iterator+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto root=handle(0u);
    m.put(iterator,root);m.put(iterator+4u,chance(2)?root:chance(4)?0xffffffffu:handle(1u+pick(count-1u)));
    m.put(iterator+8u,chance(3)?0xffffffffu:handle(1u+pick(count-1u)));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd2000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=iterator;
    if(shape==15) {
        static constexpr std::uint32_t areas[]{objects,entries,bodies,iterator};
        m.drop(areas[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_d2bd0(v);});}
void reference(WorldState &s){original::run_d2bd0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RIDER_ITERATOR");
    std::mt19937 rng(0xd2bd0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
