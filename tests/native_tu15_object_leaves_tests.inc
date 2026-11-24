// Native TU1.5 object leaves against the unmodified translated bodies of the
// physics live support unit (the validator repeats 172200 with the combat
// action unit's and BCF70 with the AI frame and sandbox units'), extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first:
//   172200 indexed record, BCF70 attachment root, 111F00 ready slot.
// There are no children. Fixtures author every path: indexes NONE and
// others with the structure BSP loaded or not; attachment chains ending
// settled or at the top (forward only, as the game's do); blocks (signed
// offsets) whose two slots are ready or fail each test; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x172200
#include "gameplay/objects/native_tu15_indexed_record.hpp"
namespace leaf=halo2::native_indexed_record_detail;
#elif OWNER==0xbcf70
#include "gameplay/objects/native_tu15_attachment_root.hpp"
namespace leaf=halo2::native_attachment_root_detail;
#else
#include "gameplay/objects/native_tu15_ready_slot.hpp"
namespace leaf=halo2::native_ready_slot_detail;
#endif
using namespace frame_misc_harness;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u;
constexpr std::uint32_t object_count=6;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    std::array<std::uint32_t,object_count> handles{};
    for(std::uint32_t k=0;k<object_count;++k)handles[k]=(std::uint32_t(rng()&0x7fffu)<<16)|k;
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto object=objects+0x1000u*k+0x800u+4u*k;   // nonzero low bytes show AL's clear
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x800u;at<object+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
        // Attachment: bit 26 often set, parents further along or NONE.
        m.put(object+4u,(std::uint32_t(rng())&~(1u<<26))|(pick(3)?1u<<26:0u));
        m.put(object+0x14u,k+1<object_count && pick(4)?handles[k+1+pick(object_count-k-1)]:0xffffffffu);
        // The block and its two slots.
        const auto offset=pick(3)==0?0x10000u-0x400u:0x200u;
        m.put(object+0x33eu,offset,2);
        const auto block=object+std::uint32_t(std::int32_t(std::int16_t(offset)));
        for(std::uint32_t s=0;s<2;++s) {
            const auto slot=block+0x9cu+0x20u*s;
            m.put(slot,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
            m.put(slot+6u,pick(4)==0?0xffffu:pick(0xffff),2);
            m.put(slot+0x12u,pick(4)==0?0u:1u|(pick(128)<<1),1);
            m.put(slot+0x11u,pick(2)?(pick(256)&~9u):pick(256),1);
        }
    }
    m.put(0x474c24u,pick(4)==0?0xffffu:pick(4),2);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
#if OWNER==0x172200
    c.abi.gp[1]=pick(4)==0?0xffffffffu:pick(4)==0?std::uint32_t(rng()):pick(4);
    if(shape==15)c.abi.gp[1]=pick(4);
#elif OWNER==0xbcf70
    c.abi.gp[0]=pick(6)==0?0xffffffffu:handles[pick(object_count)];
#else
    c.abi.gp[0]=handles[pick(object_count)];
#endif
    if(shape==15) {
#if OWNER==0x172200
        m.drop(0x474000u);
#else
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x2000u};
        m.drop(regions[pick(3)]);
#endif
    }
    return c;
}
#if OWNER==0x172200
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_172200(v);});}
void reference(WorldState &s){original::run_172200(s);}
#elif OWNER==0xbcf70
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_bcf70(v);});}
void reference(WorldState &s){original::run_bcf70(s);}
#else
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_111f00(v);});}
void reference(WorldState &s){original::run_111f00(s);}
#endif
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_LEAVES");
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
