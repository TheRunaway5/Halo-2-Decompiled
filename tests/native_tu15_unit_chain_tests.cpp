// Native TU1.5 unit chain step (D2B00) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author chains of objects of every type
// (types past 31 too), ending in NONE, empty cursors, and cursors
// overlapping an object; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_unit_chain.hpp"
using namespace frame_misc_harness;
namespace chain=halo2::native_unit_chain_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,cursors=0x10200000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    constexpr std::uint32_t count=8;
    const auto handle=[&](std::uint32_t k){return (std::uint32_t(rng()&0x7fffu)<<16)|k;};
    for(std::uint32_t k=0;k<count;++k) {
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        // The next object: further along or NONE (the game's chains end).
        const auto next=k+1<count && pick(5)?handle(k+1+pick(count-k-1)):0xffffffffu;
        m.put(object+0xcu,next);
        static constexpr std::uint32_t types[]{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,32u,33u,0xffu};
        m.put(object+0xaau,pick(3)==0?types[pick(2)]:types[pick(15)],1);
    }
    for(std::uint32_t at=cursors;at<cursors+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd0000u+pick(0x100));
    random_abi(rng,c,stack);
    auto cursor=cursors+4u*pick(4);
    if(shape==9)cursor=objects+0x400u*pick(count)+4u*pick(4);   // the cursor inside an object
    c.abi.gp[6]=cursor;
    if(shape!=9)m.put(cursor+0xcu,pick(6)==0?0xffffffffu:handle(pick(count)));
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u*3u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return chain::bridge(s,[](auto &v){original::native_frame_misc_reference_d2b00(v);});}
void reference(WorldState &s){original::run_d2b00(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_CHAIN");
    std::mt19937 rng(0xd2b00u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,chain::coverage().load(),unsigned(chain::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
