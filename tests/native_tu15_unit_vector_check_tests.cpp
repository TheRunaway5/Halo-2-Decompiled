// Native TU1.5 unit vector check (B1720) against the unmodified translated
// body (the physics live support unit's copy; the validator repeats it with
// the AI frame unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). The children AA200 and AA270 are genuine
// stand-ins whose effects are a hash of their complete entry ABI, return
// address and argument words: they return AL, ECX/EDX, flags and XMM, write
// scratch, and occasionally clobber callee-saved registers or leave the
// stack unbalanced. Fixtures author vectors near and far from unit length,
// non-finite lengths, the 1.0 and tolerance globals (special floats, NaN
// payloads) and the other vector; at most six live x87 registers; one
// region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_unit_vector_check.hpp"
using namespace frame_misc_harness;
namespace vector_check=halo2::native_unit_vector_check_detail;
namespace {
constexpr std::uint32_t vectors=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%4?1u:0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=vectors+4u*std::uint32_t(rng()%8);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_aa200(WorldState &s){stub(s,0xaa200u,0);}
static void run_aa270(WorldState &s){stub(s,0xaa270u,1);}
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
    const auto stack=0x20010000u+pick(64)*4u;
    const auto value=[&](float v) {
        if(shape==4 && pick(5)==0)return quiet_nan(rng);
        if(shape==3 && pick(5)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x7f7fffffu};
            return values[pick(5)];
        }
        return std::bit_cast<std::uint32_t>(v);
    };
    // A direction near unit length (sometimes far).
    const float scale=pick(3)?1.0f+float(int(pick(201))-100)/20000.0f:float(pick(300))/100.0f;
    const float a=float(int(pick(2001))-1000)/1000.0f,b=float(int(pick(2001))-1000)/1000.0f;
    const float norm=std::sqrt(a*a+b*b+1.0f);
    const float v[3]{a/norm*scale,b/norm*scale,1.0f/norm*scale};
    for(unsigned k=0;k<3;++k)m.put(vectors+4u*k,value(v[k]));
    for(unsigned k=0;k<3;++k)m.put(vectors+0x10u+4u*k,value(float(int(pick(2001))-1000)/1000.0f));
    m.put(0x46a0d0u,pick(8)?std::bit_cast<std::uint32_t>(1.0f):value(float(pick(200))/100.0f));
    m.put(0x46a180u,value(float(pick(100))/10000.0f));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb1800u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=vectors;c.abi.gp[7]=vectors+0x10u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{vectors,0x46a000u,scratch};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return vector_check::bridge(s,[](auto &v){original::native_frame_misc_reference_b1720(v);},
        [](auto &v){original::run_aa200(v);},[](auto &v){original::run_aa270(v);});
}
void reference(WorldState &s){original::run_b1720(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_VECTOR_CHECK");
    std::mt19937 rng(0xb1720u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,vector_check::coverage().load(),unsigned(vector_check::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
