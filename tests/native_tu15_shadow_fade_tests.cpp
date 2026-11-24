// Native TU1.5 object shadow fade (3D7E0) against the unmodified translated
// body of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 30C40 (the object's position), 4BAE0 (the distance level)
// and BCFB0 (the parent) are genuine stand-ins whose effects are a hash of
// their complete entry ABI, return address and argument words: 30C40 writes
// a position near the camera, 4BAE0 writes its level bytes (zero, low,
// high), BCFB0 returns a parent with a lower index (or another handle);
// all return EAX/ECX/EDX, flags, XMM (whole registers, so the upper lanes
// the original keeps through CVTSI2SS/MULSS are observed) and MXCSR sticky
// bits, write scratch, and occasionally clobber callee-saved registers or
// leave the stack unbalanced. The parent's fade runs through the bridge
// first, as the bound unit does. Fixtures author the tag instances and
// definitions (distance modes 0, 2 and others), object headers (bipeds,
// vehicles, other types, types past 31), objects with the not-measured
// flag, selectors (0, 1, 12, others, negative), parents, ages, the age
// flag and camouflage below zero, within, at and above one (special floats
// and NaN payloads), the camera, scales and constants, the game time and
// the x87 control word (at most three live registers); one region removed
// injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_shadow_fade.hpp"
using namespace frame_misc_harness;
namespace fade=halo2::native_shadow_fade_detail;
namespace {
constexpr std::uint32_t tags=0x10000000u,definitions=0x10100000u,headers=0x10200000u,rows=0x10210000u,
    objects=0x10300000u,time_block=0x10400000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=48u,object_stride=0x300u,definition_count=8u,none=0xffffffffu;
std::uint32_t stack=0x20010000u;
std::uint32_t real_bits(std::mt19937_64 &rng,float low,float high) {
    return std::bit_cast<std::uint32_t>(low+(high-low)*float(rng()%10001)/10000.0f);
}
void stub(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    if(target==0x30c40u) {
        const auto output=s.gp[0];
        for(unsigned k=0;k<3;++k)w(s.memory,s.observe,0x30c41u,output+4u*k,4,real_bits(rng,-30.0f,30.0f));
    } else if(target==0x4bae0u) {
        const auto first=r(s.memory,s.gp[4]+4u),second=r(s.memory,s.gp[4]+8u);
        static constexpr std::uint32_t levels[]{0,0,1,7,64,128,200,255};
        w(s.memory,s.observe,0x4bae1u,second,1,levels[rng()%8]);
        if(rng()%2)w(s.memory,s.observe,0x4bae2u,first,1,std::uint32_t(rng()%3));
    } else if(target==0xbcfb0u) {
        const auto index=s.gp[1]&0xffffu;
        result=none;
        if(index && index<object_count) {
            const auto parent=r(s.memory,objects+index*object_stride+0x14u);
            result=parent!=none && rng()%4?parent:(std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%index);
        }
    }
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
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%9);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static void run_30c40(WorldState &s){stub(s,0x30c40u,1);}
static void run_4bae0(WorldState &s){stub(s,0x4bae0u,2);}
static void run_bcfb0(WorldState &s){stub(s,0xbcfb0u,0);}
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
    const auto real=[&](float low,float high){return std::bit_cast<std::uint32_t>(low+(high-low)*float(pick(10001))/10000.0f);};
    const unsigned shape=n%16;
    const auto special=[&](std::uint32_t value) {
        if(shape==4 && pick(6)==0)return quiet_nan(rng);
        if(shape==3 && pick(5)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,0x80000000u,0x00000000u};
            return values[pick(6)];
        }
        return value;
    };
    stack=0x20010000u+pick(64)*4u;
    // Tag instances and definitions (distance modes).
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<definition_count;++k) {
        m.put(tags+k*16u,std::uint32_t(rng()));m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,definitions+k*0x40u);m.put(tags+k*16u+12u,std::uint32_t(rng()));
        for(std::uint32_t at=definitions+k*0x40u;at<definitions+k*0x40u+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t modes[]{0,0,2,2,2,1,3,0xffff};
        m.put(definitions+k*0x40u+0x18u,modes[pick(8)],2);
    }
    // Object headers and objects.
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);
    std::vector<std::uint32_t> salt(object_count);
    for(auto &v:salt)v=std::uint32_t(rng()%0x7fff);
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto row=rows+k*12u;
        m.put(row,std::uint32_t(rng()));
        static constexpr std::uint32_t types[]{0,1,0,1,2,3,7,32,33,40};
        m.put(row+3u,types[pick(10)],1);
        m.put(row+4u,std::uint32_t(rng()));m.put(row+8u,objects+k*object_stride);
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=o;at<o+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(o,(std::uint32_t(rng()%0x7fff)<<16)|pick(definition_count));
        const auto flags=std::uint32_t(rng())&0xffu;
        m.put(o+6u,pick(6)==0?flags|1u:flags&~1u,1);
        m.put(o+0x14u,k && pick(2)?(salt[0]<<16)|pick(k):none);
        for(unsigned a=0;a<3;++a)m.put(o+0x30u+4u*a,special(real(-40.0f,40.0f)));
        static constexpr std::uint32_t selectors[]{0,0,0,1,1,12,3,0xff,0x80};
        m.put(o+0xa8u,std::uint32_t(rng()));m.put(o+0xaau,selectors[pick(9)],1);
        m.put(o+0xbcu,std::uint32_t(rng()%400));
        m.put(o+0x108u,std::uint32_t(rng()));
        const auto fade_flags=std::uint32_t(rng())&0xffu;
        m.put(o+0x10au,pick(2)?fade_flags|4u:fade_flags&~4u,1);
        static constexpr float camouflage[]{0.0f,0.0f,0.25f,0.5f,1.0f,1.5f,-0.5f};
        m.put(o+0x2b0u,special(pick(4)?std::bit_cast<std::uint32_t>(camouflage[pick(7)]):real(-0.5f,1.5f)));
    }
    // The camera, scales, constants and the game time.
    for(unsigned a=0;a<3;++a)m.put(0x4c65c0u+4u*a,special(real(-10.0f,10.0f)));
    m.put(0x4c65ecu,special(real(0.2f,3.0f)));
    m.put(0x46a0c4u,special(real(0.001f,0.02f)));
    m.put(0x46a0e8u,pick(8)?0u:real(-0.2f,0.2f));
    m.put(0x46a0d0u,pick(8)?std::bit_cast<std::uint32_t>(1.0f):special(real(0.5f,1.5f)));
    m.put(0x46a11cu,special(real(0.0f,3.0f)));
    m.put(0x51df54u,time_block);
    m.put(time_block,std::uint32_t(rng()));m.put(time_block+4u,special(real(0.01f,0.05f)));
    m.put(time_block+8u,std::uint32_t(rng()%500));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x2000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x3e000u+pick(0x100));
    const auto object=pick(object_count);
    m.put(stack+4u,shape==12 && pick(2)?(salt[0]<<16)|(object_count+pick(4)):(salt[object]<<16)|object);
    m.put(stack+8u,scratch+0x80u);m.put(stack+0xcu,scratch+0x90u+pick(4));
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>3u)c.abi.x87_depth=3u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{tags,definitions,headers,rows,objects,time_block,0x46a000u,0x4c6000u};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s);
void chain(WorldState &s){if(!native(s))original::run_3d7e0(s);}
bool native(WorldState &s) {
    return fade::bridge(s,[](auto &v){original::native_frame_misc_reference_3d7e0(v);},
        [](auto &v){original::run_30c40(v);},[](auto &v){original::run_4bae0(v);},[](auto &v){original::run_bcfb0(v);},
        [](auto &v){chain(v);});
}
void reference(WorldState &s){original::run_3d7e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHADOW_FADE");
    std::mt19937 rng(0x3d7e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,fade::coverage().load(),unsigned(fade::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
