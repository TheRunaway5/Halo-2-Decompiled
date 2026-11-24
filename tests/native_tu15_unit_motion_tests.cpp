// Native TU1.5 unit motion check (E0500) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child E65A0 is a genuine stand-in whose effects are a hash of its
// complete entry ABI and return address: it returns AL, ECX/EDX, flags and
// XMM, writes scratch, and occasionally clobbers callee-saved registers (BL
// among them) or leaves the stack unbalanced. Fixtures author units frozen
// or not, in clusters (none, unflagged, detached, supporting), in every
// state, with velocities still, slow and fast (special floats, NaN
// payloads) and up to three live x87 registers (four are refused); one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_motion.hpp"
using namespace frame_misc_harness;
namespace motion=halo2::native_unit_motion_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,units=0x10100000u,clusters_array=0x10200000u,
    cluster_data=0x10210000u,surfaces=0x10300000u,scratch=0x10600000u,none=0xffffffffu;
std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    return values[rng()%8];
}
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0xe65a0u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0xe65a1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?0u:1u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_e65a0(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    std::mt19937_64 wide(rng());
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    m.put(0x52be5cu,clusters_array);
    for(std::uint32_t at=clusters_array;at<clusters_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(clusters_array+0x44u,cluster_data);
    for(std::uint32_t k=0;k<8;++k) {
        const auto cluster=cluster_data+0xa0u*k;
        for(std::uint32_t at=cluster;at<cluster+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(cluster+0x70u,surfaces+0x80u*k);
        for(std::uint32_t at=surfaces+0x80u*k;at<surfaces+0x80u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(surfaces+0x80u*k+0x44u,(std::uint32_t(rng())&0xffffff00u)|(pick(3)?1u:0u));
    }
    for(std::uint32_t k=0;k<8;++k) {
        const auto unit=units+0x400u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,unit);
        for(std::uint32_t at=unit;at<unit+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x348u,(std::uint32_t(rng())&0x00ffffffu)|((pick(5)==0?1u:0u)<<24));
        m.put(unit+0xb4u,pick(4)==0?none:(std::uint32_t(rng())<<16)|pick(8));
        m.put(unit+0x3dcu,(std::uint32_t(rng())&0xffffff00u)|pick(8));
        for(std::uint32_t i=0;i<3;++i) {
            const float speeds[]{0.0f,0.001f,0.01f,0.1f,1.0f,-0.5f};
            m.put(unit+0x1b0u+4u*i,shape==4 && pick(3)==0?special_float(wide):std::bit_cast<std::uint32_t>(speeds[pick(6)]));
        }
    }
    m.put(0x46a0ecu,pick(5)?std::bit_cast<std::uint32_t>(0.0001f):special_float(wide));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>3u)c.abi.x87_depth=3u;
    if(shape==14)c.abi.x87_depth=4u;
    c.abi.gp[6]=(std::uint32_t(rng())<<16)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,units+0x400u*3u,clusters_array,cluster_data,surfaces,0x46a000u};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return motion::bridge(s,[](auto &v){original::native_frame_misc_reference_e0500(v);},[](auto &v){original::run_e65a0(v);});
}
void reference(WorldState &s){original::run_e0500(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_MOTION");
    std::mt19937 rng(0xe0500u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,c.abi.x87_depth>3u))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,motion::coverage().load(),unsigned(motion::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
