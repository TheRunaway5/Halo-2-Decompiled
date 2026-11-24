// Native TU1.5 object model view (212C70) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// map world support unit's), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BC190 is a hash-driven stub of its complete entry ABI: it fills words at
// its target and pops it, the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy a callee-saved register moves.
// Fixtures author objects (signed block offsets), definitions with and
// without models, models with and without render models; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/models/native_tu15_object_model_view.hpp"
using namespace frame_misc_harness;
namespace model_view=halo2::native_object_model_view_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,instances=0x10200000u,
    tags=0x10210000u,views=0x10300000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto argument=r(s.memory,s.gp[4]+4u);
    h=mix(h,argument);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(rng()%2)w(s.memory,s.observe,target+2u,argument+4u*std::uint32_t(rng()%8),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?views+0x80u*std::uint32_t(rng()%4):which==5?tags+0x100u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbc190u)throw std::runtime_error("Unbound dispatch target");
    stub(s,0xbc190u);
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
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<8;++k) {
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x100u*(k%4));m.put(instances+16u*k+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=tags;at<tags+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        m.put(tags+0x100u*k+0x38u,pick(5)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(8));
        m.put(tags+0x100u*k+0x24u,pick(5)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(8));
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x200u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|pick(8));
    }
    for(std::uint32_t at=views;at<views+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x210000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);c.abi.gp[6]=views+0x80u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x200u*2u,instances,tags,views};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return model_view::bridge(s,[](auto &v){original::native_frame_misc_reference_212c70(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_212c70(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_MODEL_VIEW");
    std::mt19937 rng(0x212c70u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,model_view::coverage().load(),unsigned(model_view::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
