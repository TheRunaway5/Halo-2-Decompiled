// Native TU1.5 object ancestry (BCFE0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BCFE0 has no children. Fixtures author object headers and parent chains
// (no parent, deep chains), objects of NONE, ancestors of NONE, the object
// itself, a parent, the top, an unrelated object; one region removed
// injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_ancestry.hpp"
using namespace frame_misc_harness;
namespace ancestry=halo2::native_object_ancestry_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,rows=0x10010000u,objects=0x10100000u,none=0xffffffffu;
constexpr std::uint32_t object_count=64u,object_stride=0x40u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);
    std::vector<std::uint32_t> handle(object_count),parent(object_count,none);
    for(std::uint32_t k=0;k<object_count;++k)handle[k]=(std::uint32_t(rng()%0x7fff)<<16)|k;
    for(std::uint32_t k=0;k<object_count;++k) {
        m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+k*object_stride);
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=o;at<o+object_stride;at+=4)m.put(at,std::uint32_t(rng()));
        if(k && pick(4))parent[k]=handle[pick(k)];
        m.put(o+0x14u,parent[k]);
    }
    const auto object=pick(object_count);
    std::uint32_t eax=handle[object],esi;
    const auto roll=pick(6);
    if(roll==0)esi=none;
    else if(roll==1)esi=eax;
    else if(roll<4) {
        // A parent along the chain.
        auto k=object;unsigned steps=1u+pick(4);
        while(steps-- && parent[k]!=none)k=parent[k]&0xffffu;
        esi=handle[k];
    } else esi=handle[pick(object_count)];
    if(shape==5)eax=none;
    if(shape==6)esi^=0x10000u;                                    // same index, another salt
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xbd100u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=eax;c.abi.gp[6]=esi;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,rows,objects,0x4ec000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return ancestry::bridge(s,[](auto &v){original::native_frame_misc_reference_bcfe0(v);});}
void reference(WorldState &s){original::run_bcfe0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_ANCESTRY");
    std::mt19937 rng(0xbcfe0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,ancestry::coverage().load(),unsigned(ancestry::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
