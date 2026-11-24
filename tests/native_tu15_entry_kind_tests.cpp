// Native TU1.5 entry kind query (1062A0) against the unmodified translated
// body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author objects whose definitions list no
// (zero or negative counts) or a few entries of kind 5 and others, with
// states idle and busy; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_entry_kind.hpp"
using namespace frame_misc_harness;
namespace kind=halo2::native_entry_kind_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,tags=0x10180000u,
    definitions=0x10200000u,lists=0x10300000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x800u*k+4u*k;
        const auto tag_index=0x1000u*pick(2)+k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x300u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|tag_index);
        const auto definition=definitions+0x1000u*k+4u*k;
        m.put(tags+16u*tag_index+8u,definition);
        for(std::uint32_t at=definition+0x2c0u;at<definition+0x2d0u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t counts[]{0u,0xffffffffu,0x80000000u,1u,2u,3u,4u,6u};
        m.put(definition+0x2c8u,counts[pick(8)]);
        const auto list=lists+0x1000u*k+4u*pick(4);
        m.put(definition+0x2ccu,list);
        for(std::uint32_t e=0;e<6;++e) {
            for(std::uint32_t at=list+0x40u*e;at<list+0x40u*e+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
            m.put(list+0x40u*e+6u,pick(3)?5u:(pick(2)?pick(8):0x10005u),2);
            m.put(object+0x20cu+12u*e,pick(2)?0u:1u+pick(255),1);
        }
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u*2u,tags,definitions+0x1000u,lists+0x2000u};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return kind::bridge(s,[](auto &v){original::native_frame_misc_reference_1062a0(v);});}
void reference(WorldState &s){original::run_1062a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ENTRY_KIND");
    std::mt19937 rng(0x1062a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,kind::coverage().load(),unsigned(kind::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
