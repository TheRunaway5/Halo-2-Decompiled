// Native TU1.5 unit speed scale (DFD20) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 2647E0 is a hash-driven stub: it may rewrite the scale through the
// pushed slot address, returns random caller-saved registers, XMM, flags
// and (by policy) MXCSR, a changed callee-saved register or an unbalanced
// cleanup. Fixtures author units (with and without controllers, live and
// idle records, both limit directions, every override condition), tags,
// the speed globals, one and the fractions (special floats too); one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_speed_scale.hpp"
using namespace frame_misc_harness;
namespace speed=halo2::native_unit_speed_scale_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,units=0x10100000u,tags=0x10200000u,
    definitions=0x10210000u,records=0x10300000u,record_data=0x10310000u,speed_globals=0x10400000u;
std::uint32_t real(std::mt19937 &rng) {
    static constexpr std::uint32_t special[]{0u,0x80000000u,0x7fc00000u,0x7f800000u,0x3f800000u,0x3f000000u,0x00000001u,0xbf800000u};
    return rng()%5==0?special[rng()%8]:std::bit_cast<std::uint32_t>(float(int(rng()%4001)-1000)/2000.0f);
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto slot=r(s.memory,s.gp[4]+4u,4);
    h=mix(h,slot);h=mix(h,r(s.memory,slot,4));
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%2)w(s.memory,s.observe,target+1u,slot,4,std::bit_cast<std::uint32_t>(float(int(rng()%4001)-2000)/1000.0f));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=units+0x400u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_2647e0(WorldState &s){stub(s,0x2647e0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<8;++k) {
        m.put(tags+16u*k,std::uint32_t(rng()));m.put(tags+16u*k+4u,std::uint32_t(rng()));
        m.put(tags+16u*k+8u,definitions+0x400u*(k%4));m.put(tags+16u*k+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t k=0;k<4;++k)m.put(definitions+0x400u*k+0x250u,real(rng));
    for(std::uint32_t k=0;k<4;++k) {
        const auto unit=units+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,unit);
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit,(std::uint32_t(rng())<<16)|pick(8));
        m.put(unit+0x12cu,pick(3)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        m.put(unit+0x2c4u,real(rng));
        const auto put8=[&](std::uint32_t at,std::uint32_t v){m.put(at,v,1);};
        put8(unit+0x3dcu,pick(2)?6u:pick(8));
        put8(unit+0x3f8u,pick(3)?1u+pick(255):0u);
        m.put(unit+0x348u,std::uint32_t(rng()),2);
    }
    m.put(0x5020c8u,records);
    for(std::uint32_t at=records;at<records+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records+0x44u,record_data);
    for(std::uint32_t k=0;k<4;++k)m.put(record_data+0x888u*k+0x858u,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
    m.put(0x51df54u,speed_globals);
    m.put(speed_globals+4u,pick(4)?0x3f800000u:real(rng));
    m.put(0x46a0d0u,pick(6)?0x3f800000u:real(rng));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xd0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,units+0x400u*2u,tags,definitions,records,record_data,
            speed_globals,0x46a000u};
        m.drop(regions[pick(9)]);
    }
    return c;
}
bool native(WorldState &s) {
    return speed::bridge(s,[](auto &v){original::native_frame_misc_reference_dfd20(v);},[](auto &v){original::run_2647e0(v);});
}
void reference(WorldState &s){original::run_dfd20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_SPEED_SCALE");
    std::mt19937 rng(0xdfd20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,speed::coverage().load(),unsigned(speed::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
