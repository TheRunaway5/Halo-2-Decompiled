// Native TU1.5 index marks (3F3E0) against the unmodified translated body of
// the effect particle frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 3F3E0 has no children. Fixtures author the list owner, counts (zero,
// negative, small, a count that changes under the walk because an entry
// overlaps it), indices (negative, within and past the table, the same word
// twice) and the bit table; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_index_marks.hpp"
using namespace frame_misc_harness;
namespace marks=halo2::native_index_marks_detail;
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
    // Shape 7 places the list over the bit table: a mark can change the count.
    const std::uint32_t owner=shape==7?0x4d1ee0u+4u*pick(8)-0xa6cu:0x10000000u;
    m.put(0x556e18u,owner);
    for(std::uint32_t at=owner+0xa40u;at<owner+0xa6eu+26u*40u+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t count=pick(12);
    if(shape==5)count=0;
    if(shape==6)count=0x8000u|pick(4);
    m.put(owner+0xa6cu,count,2);
    for(std::uint32_t k=0;k<40;++k) {
        std::uint32_t index=shape==7?pick(256):pick(512);
        if(pick(8)==0)index=0x10000u-1u-pick(64);                 // negative
        m.put(owner+0xa6eu+26u*k,index,2);
    }
    for(std::uint32_t at=0x4d1ee0u-0x100u;at<0x4d1ee0u+0x100u;at+=4)if(shape!=7 || at<owner+0xa6cu || at>=owner+0xa70u)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x3e000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        const std::uint32_t regions[]{owner,owner+0x1000u,0x4d1000u,0x556000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return marks::bridge(s,[](auto &v){original::native_frame_misc_reference_3f3e0(v);});}
void reference(WorldState &s){original::run_3f3e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_INDEX_MARKS");
    std::mt19937 rng(0x3f3e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,marks::coverage().load(),unsigned(marks::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
