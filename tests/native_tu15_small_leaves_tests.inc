// Native TU1.5 small leaves against the unmodified translated bodies of the
// physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first: CB440 child
// unit, 140EC0 bit union, 105FD0 charge gate. There are no children.
// Fixtures author children chains (forward only, as the game's are) of
// every type with matching words or not; bit vectors of every count
// (negative and zero too) with outputs over their inputs; charges below,
// at and above 1 (NaN too) and definitions with and without entries; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0xcb440
#include "gameplay/objects/native_tu15_child_unit.hpp"
namespace leaf=halo2::native_child_unit_detail;
#define SWITCH "HALO2_VERIFY_CHILD_UNIT"
#elif OWNER==0x140ec0
#include "physics/runtime/native_tu15_bit_union.hpp"
namespace leaf=halo2::native_bit_union_detail;
#define SWITCH "HALO2_VERIFY_BIT_UNION"
#else
#include "gameplay/units/native_tu15_charge_gate.hpp"
namespace leaf=halo2::native_charge_gate_detail;
#define SWITCH "HALO2_VERIFY_CHARGE_GATE"
#endif
using namespace frame_misc_harness;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,tags=0x10180000u,
    definitions=0x10200000u,vectors=0x10300000u;
constexpr std::uint32_t object_count=6;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);m.put(0x46a0d0u,0x3f800000u);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    std::array<std::uint32_t,object_count> handles{};
    for(std::uint32_t k=0;k<object_count;++k)handles[k]=(std::uint32_t(rng()&0x7fffu)<<16)|k;
    const auto word=pick(4);
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto object=objects+0x800u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0x10u,k+1<object_count && pick(4)?handles[k+1+pick(object_count-k-1)]:0xffffffffu);
        m.put(object+0xcu,k+1<object_count && pick(3)?handles[k+1+pick(object_count-k-1)]:0xffffffffu);
        static constexpr std::uint32_t types[]{0u,1u,2u,3u,32u,33u,0xffu};
        m.put(object+0xaau,types[pick(7)],1);
        m.put(object+0x1fcu,pick(2)?word:pick(0x10000),2);
        const auto tag_index=0x1000u*pick(2)+k;
        m.put(object,(std::uint32_t(rng())<<16)|tag_index);
        const auto definition=definitions+0x1000u*k+4u*pick(4);
        m.put(tags+16u*tag_index+8u,definition);
        for(std::uint32_t at=definition+0x2b0u;at<definition+0x2d0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x2c0u,pick(4)==0?0u:pick(4)==0?0x80000000u:1u+pick(3));
        const auto list=definitions+0x1000u*k+0x800u;
        m.put(definition+0x2c4u,list);
        for(std::uint32_t at=list;at<list+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(list+0xau,pick(3)==0?0u:pick(3)==0?0x8000u+pick(4):1u+pick(9),2);
        static constexpr std::uint32_t charges[]{0x3f000000u,0x3f800000u,0x40000000u,0x7fc00000u,0xbf800000u,0x3f7fffffu};
        m.put(object+0x184u,charges[pick(6)]);
        m.put(object+0x22au,pick(2)?0u:pick(0x10000),2);m.put(object+0x22cu,pick(2)?0u:pick(0x10000),2);
    }
    for(std::uint32_t at=vectors;at<vectors+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
#if OWNER==0xcb440
    c.abi.gp[0]=handles[pick(object_count)];
    m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|word);
#elif OWNER==0x140ec0
    static constexpr std::uint32_t counts[]{0u,1u,31u,32u,33u,64u,100u,0xffffffffu,0xffffffe1u,0x80000000u};
    c.abi.gp[1]=counts[pick(10)];
    c.abi.gp[0]=vectors+4u*pick(16);c.abi.gp[2]=vectors+0x80u+4u*pick(16);
    m.put(stack+4u,shape==7?vectors+4u*pick(40):vectors+0x100u+4u*pick(8));
#else
    c.abi.gp[0]=handles[pick(object_count)];
#endif
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x1000u,tags,definitions+0x1000u,vectors};
        m.drop(regions[pick(6)]);
    }
    return c;
}
#if OWNER==0xcb440
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_cb440(v);});}
void reference(WorldState &s){original::run_cb440(s);}
#elif OWNER==0x140ec0
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_140ec0(v);});}
void reference(WorldState &s){original::run_140ec0(s);}
#else
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_105fd0(v);});}
void reference(WorldState &s){original::run_105fd0(s);}
#endif
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,SWITCH);
    std::mt19937 rng(OWNER+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,leaf::coverage().load(),unsigned(leaf::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
