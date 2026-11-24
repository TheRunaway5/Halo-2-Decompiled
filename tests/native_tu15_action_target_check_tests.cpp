// Native TU1.5 action target check (1AE500) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1AE5E0 and 1E7810 are hash-driven stubs of their complete entry ABI: AL
// random, 1AE5E0 writing the result byte and popping its three arguments,
// the caller-saved registers, XMM, flags and (by policy) MXCSR random, and
// by policy EBX or EBP moved. Fixtures author headers (every type), objects
// (flagged, kinds, links), tags (flagged or not) and arguments (the object
// acting on itself too); one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_action_target_check.hpp"
using namespace frame_misc_harness;
namespace target_check=halo2::native_action_target_check_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,instances=0x10200000u,
    tags=0x10210000u,results=0x10300000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(arguments && rng()%2)w(s.memory,s.observe,target+2u,r(s.memory,s.gp[4]+12u),1,std::uint32_t(rng()%2));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0) {
        if(rng()%2)s.gp[3]=std::uint32_t(rng());
        else s.gp[5]=results+std::uint32_t(rng()%16);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1ae5e0(WorldState &s){stub(s,0x1ae5e0u,3);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1e7810u)throw std::runtime_error("Unbound dispatch target");
    stub(s,0x1e7810u,0);
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
    for(std::uint32_t k=0;k<4;++k)m.put(tags+0x100u*k+0xbcu,std::uint32_t(rng())&(pick(3)?~(1u<<21):~0u));
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x200u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));
        m.put(header_data+12u*k+3u,pick(3)?pick(2):pick(16),1);
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|pick(8));
        m.put(object+0xaau,pick(2)?1u:pick(8),1);
        m.put(object+0x10au,std::uint32_t(rng())&(pick(3)?~4u:~0u),1);
        m.put(object+0x138u,pick(2)?0xffffu:std::uint32_t(rng()),2);
    }
    for(std::uint32_t at=results;at<results+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x60u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1a0000u+pick(0x100));
    const auto acting=(std::uint32_t(rng())<<16)|pick(4);
    m.put(stack+4u,acting);m.put(stack+8u,pick(8)==0?acting:std::uint32_t(rng()));
    m.put(stack+12u,std::uint32_t(rng()));m.put(stack+16u,results+pick(16));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x200u*2u,instances,tags,results};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return target_check::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae500(v);},
        [](auto &v){original::run_1ae5e0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1ae500(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ACTION_TARGET_CHECK");
    std::mt19937 rng(0x1ae500u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,target_check::coverage().load(),unsigned(target_check::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
