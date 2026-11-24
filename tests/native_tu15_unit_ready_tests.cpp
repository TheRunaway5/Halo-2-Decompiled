// Native TU1.5 unit readiness (E65A0) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author units with every flag
// combination, counts around the rounded threshold (negative counts too),
// every state, parents NONE or not, tick rates and threshold seconds that
// round both ways (and special floats), every MXCSR mode and x87 control
// word the harness sets, up to seven live x87 registers (eight are
// refused); one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_ready.hpp"
using namespace frame_misc_harness;
namespace ready=halo2::native_unit_ready_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,units=0x10100000u,time_block=0x10200000u,
    none=0xffffffffu;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<8;++k) {
        const auto unit=units+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,unit);
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0xc0u,(std::uint32_t(rng())&0xffff00ffu)|((pick(3)?0u:1u)|(std::uint32_t(rng())&0xfeu))<<8);
        m.put(unit+0x348u,(std::uint32_t(rng())&0xfffffffeu)|(pick(4)?1u:0u));
        m.put(unit+0x398u,(std::uint32_t(rng())&0xffff00ffu)|(std::uint32_t(std::uint8_t(int(pick(80))-10))<<8));
        m.put(unit+0x3dcu,(std::uint32_t(rng())&0xffffff00u)|(pick(3)?(pick(2)?1u:3u):pick(6)));
        m.put(unit+0x14u,pick(4)?none:std::uint32_t(rng()));
    }
    m.put(0x51df54u,time_block);
    m.put(time_block,(std::uint32_t(pick(2)?30u:60u)<<16)|std::uint32_t(rng()&0xffffu));
    static constexpr float seconds[]{1.0f,0.5f,0.25f,0.75f,1.5f,0.1f,0.0f,-1.0f};
    std::uint32_t settle=std::bit_cast<std::uint32_t>(seconds[pick(8)]);
    if(shape==3)settle=pick(2)?0x7fc00001u:0x7f800000u;
    m.put(0x46a458u,settle);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==14)c.abi.x87_depth=8u;
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,units+0x400u*2u,time_block,0x46a000u,0x51d000u};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return ready::bridge(s,[](auto &v){original::native_frame_misc_reference_e65a0(v);});}
void reference(WorldState &s){original::run_e65a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_READY");
    std::mt19937 rng(0xe65a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>7u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,ready::coverage().load(),unsigned(ready::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
