// Native TU1.5 shadow caster collection (333F0) against the unmodified
// translated body of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 3D7E0 (the shadow fade), 3E9B0 (the class), 4A7B0 and 4AC20
// are genuine stand-ins whose effects are a hash of their complete entry
// ABI, return address and argument words: 3D7E0 writes the fade and shown
// outputs and returns AL, 3E9B0 returns AL, 4A7B0 returns a table, 4AC20
// writes a record word; all return EAX/ECX/EDX, flags, XMM and MXCSR sticky
// bits, and occasionally clobber callee-saved registers or leave the stack
// unbalanced. Fixtures author the candidate lists (counts zero, negative,
// past 32 shown), flags for every mode (modes 0, 1, 2 and others), object
// headers and bounding spheres, the view depth row (depths behind, near
// the floor, far), the projection and floor constants with special floats
// and NaN payloads, and the shadow quality (negative, within, past 3); one
// region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_shadow_casters.hpp"
using namespace frame_misc_harness;
namespace casters=halo2::native_shadow_casters_detail;
namespace {
constexpr std::uint32_t candidates=0x10000000u,handles=0x10010000u,flag_table=0x10020000u,headers=0x10100000u,
    rows=0x10110000u,objects=0x10200000u,records=0x10400000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=48u,object_stride=0x40u;
std::uint32_t stack=0x20010000u;
void stub(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    if(target==0x3d7e0u) {
        w(s.memory,s.observe,0x3d7e1u,r(s.memory,s.gp[4]+8u),4,std::bit_cast<std::uint32_t>(float(rng()%101)/100.0f));
        w(s.memory,s.observe,0x3d7e2u,r(s.memory,s.gp[4]+12u),1,std::uint32_t(rng()%2));
        result=(result&0xffffff00u)|(rng()%4?1u:0u);
    } else if(target==0x3e9b0u)result=(result&0xffffff00u)|std::uint32_t(rng()%3);
    else if(target==0x4a7b0u)result=scratch+std::uint32_t(rng()%16)*4u;
    else w(s.memory,s.observe,0x4ac21u,r(s.memory,s.gp[4]+8u)+4u*std::uint32_t(rng()%8),4,std::uint32_t(rng()));
    s.gp[0]=result;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%5);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_3d7e0(WorldState &s){stub(s,0x3d7e0u,3);}
static void run_3e9b0(WorldState &s){stub(s,0x3e9b0u,0);}
static void run_4a7b0(WorldState &s){stub(s,0x4a7b0u,2);}
static void run_4ac20(WorldState &s){stub(s,0x4ac20u,4);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto real=[&](float low,float high){
        if(shape==4 && pick(6)==0)return quiet_nan(rng);
        if(shape==3 && pick(5)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x00000000u};
            return values[pick(5)];
        }
        return std::bit_cast<std::uint32_t>(low+(high-low)*float(pick(10001))/10000.0f);
    };
    stack=0x20010000u+pick(64)*4u;
    std::uint32_t count=pick(12);
    if(shape==9)count=40u+pick(10);                                 // past 32 records
    if(shape==10)count=0x8000u|pick(4);                             // negative
    for(std::uint32_t at=candidates;at<candidates+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(candidates+4u,count,2);m.put(candidates+8u,flag_table);m.put(candidates+0xcu,handles);
    for(std::uint32_t k=0;k<64;++k) {
        m.put(handles+4u*k,(std::uint32_t(rng()%0x7fff)<<16)|pick(object_count));
        m.put(flag_table+2u*k,(pick(2)?0x4000u:0u)|(pick(2)?0x8000u:0u)|pick(0x4000),2);
    }
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);
    for(std::uint32_t k=0;k<object_count;++k) {
        m.put(rows+k*12u,std::uint32_t(rng()));m.put(rows+k*12u+4u,std::uint32_t(rng()));m.put(rows+k*12u+8u,objects+k*object_stride);
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=o;at<o+object_stride;at+=4)m.put(at,std::uint32_t(rng()));
        for(unsigned a=0;a<3;++a)m.put(o+0x30u+4u*a,real(-40.0f,40.0f));
        m.put(o+0x3cu,real(0.05f,3.0f));
    }
    std::uint32_t quality=pick(4);
    if(pick(6)==0)quality=0x80000000u|pick(4);
    if(pick(6)==0)quality=4u+pick(4);
    m.put(0x4c66f4u,quality);
    m.put(0x4c6640u,real(-1.0f,1.0f));m.put(0x4c664cu,real(-1.0f,1.0f));m.put(0x4c6658u,real(-1.0f,1.0f));
    m.put(0x4c6664u,real(-20.0f,20.0f));m.put(0x4c66f0u,real(100.0f,1000.0f));
    m.put(0x46a178u,real(0.01f,5.0f));m.put(0x46a11cu,real(0.5f,2.5f));
    for(std::uint32_t at=records;at<records+0x20u*0x14u+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x330000u+pick(0x100));
    m.put(stack+4u,records);
    static constexpr std::uint32_t modes[]{0,0,1,1,2,2,3,0xffffffffu};
    m.put(stack+8u,modes[pick(8)]);
    random_abi(rng,c,stack);
    c.abi.gp[3]=candidates;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{candidates,handles,flag_table,headers,rows,objects,0x4c6000u,0x46a000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return casters::bridge(s,[](auto &v){original::native_frame_misc_reference_333f0(v);},
        [](auto &v){original::run_3d7e0(v);},[](auto &v){original::run_3e9b0(v);},
        [](auto &v){original::run_4a7b0(v);},[](auto &v){original::run_4ac20(v);});
}
void reference(WorldState &s){original::run_333f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHADOW_CASTERS");
    std::mt19937 rng(0x333f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,casters::coverage().load(),unsigned(casters::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
