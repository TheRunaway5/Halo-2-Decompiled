// Native TU1.5 object function scale (C0700) against the unmodified
// translated body of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child BCB70 (the unit's dispatch route) is a genuine stand-in whose
// effects are a hash of its complete entry ABI, its three arguments and
// the return address: it writes the scale through its pointer (special
// floats too), pops its arguments, returns EAX/ECX/EDX, flags and XMM,
// writes scratch, and occasionally clobbers callee-saved registers or
// leaves the stack unbalanced. Fixtures author objects, tags whose
// functions name a function or not, the scaled flag set or clear, and the
// objects' scales; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_function_scale.hpp"
using namespace frame_misc_harness;
namespace scale=halo2::native_function_scale_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,instances=0x10200000u,
    tags=0x10300000u,functions=0x10400000u,scratch=0x10600000u;
std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    return values[rng()%8];
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,r(s.memory,s.gp[4]+12u),4,rng()%6==0?special_float(rng):std::bit_cast<std::uint32_t>(float(rng()%9)*0.125f));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?tags+0x100u*std::uint32_t(rng()%4):which==7?objects+0x100u*std::uint32_t(rng()%8):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=16u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbcb70u)throw std::runtime_error("Unbound shadow frame dispatch target");
    stub(s,target);
}
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
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<4;++k) {
        const auto tag=tags+0x100u*k;
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tag);m.put(instances+16u*k+12u,std::uint32_t(rng()));
        for(std::uint32_t at=tag;at<tag+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tag,(std::uint32_t(rng())&0xfdffffffu)|(pick(2)?0x02000000u:0u));
        m.put(tag+0x98u,functions+0x200u*k);
        for(std::uint32_t i=0;i<16;++i) {
            for(std::uint32_t at=0;at<24u;at+=4)m.put(functions+0x200u*k+24u*i+at,std::uint32_t(rng()));
            m.put(functions+0x200u*k+24u*i+0x10u,pick(3)==0?0u:std::uint32_t(rng()));
        }
    }
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x100u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|pick(4));
        m.put(object+0xa0u,pick(6)==0?special_float(wide):std::bit_cast<std::uint32_t>(float(pick(5))*0.25f));
    }
    m.put(0x46a0d0u,std::bit_cast<std::uint32_t>(1.0f));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x330000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[2]=(std::uint32_t(rng())<<16)|pick(8);c.abi.gp[1]=pick(16);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,instances,tags,functions,0x46a000u};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return scale::bridge(s,[](auto &v){original::native_frame_misc_reference_c0700(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_c0700(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FUNCTION_SCALE");
    std::mt19937 rng(0xc0700u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,scale::coverage().load(),unsigned(scale::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
