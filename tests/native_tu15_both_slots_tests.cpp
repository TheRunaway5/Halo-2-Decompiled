// Native TU1.5 object slot query (CFB40) against the unmodified translated body
// of the physics live support unit (the validator repeats it with its other
// unit's copy), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). There are no children. Fixtures author objects with set
// and NONE slot indices and slot handles, and slot indices NONE, negative or
// ordinary; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_both_slots.hpp"
using namespace frame_misc_harness;
namespace query=halo2::native_both_slots_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x400u*k+0x40u;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x40u;at<object+0x300u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x212u,pick(3)==0?0xffu:pick(4),1);m.put(object+0x213u,pick(3)==0?0xffu:pick(4),1);
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xcf000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    (void)0;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u*2u};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return query::bridge(s,[](auto &v){original::native_frame_misc_reference_cfb40(v);});}
void reference(WorldState &s){original::run_cfb40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_SLOTS");
    std::mt19937 rng(0xcfb40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,query::coverage().load(),unsigned(query::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
