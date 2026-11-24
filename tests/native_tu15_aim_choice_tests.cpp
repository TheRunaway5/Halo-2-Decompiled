// Native TU1.5 aim choice (1ADF60) against the unmodified translated body
// of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The stack probe 32A660 moves ESP below the caller's by EAX and reads the
// probed words, as the CRT's does. 14CDB0, 1AE090, qsort 32B560 and 1AF3E0
// are hash-driven stubs of their complete entry ABIs: each hashes what it
// reads (its pushed arguments), writes scratch and its outputs (1AE090 its
// count of candidates into the list, qsort some of the list's words),
// answers EAX (14CDB0 a leaf or NONE, 1AE090 the count) or AL; the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author leaves with and without clusters, counts of every sign
// and candidates accepted early, late or never; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_aim_choice.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_aim_choice_detail;
namespace {
constexpr std::uint32_t query=0x10000000u,structure=0x10001000u,leaves=0x10002000u,ray=0x10003000u,chosen=0x10004000u,
    scratch=0x10600000u;
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0x1ae090u:return 7u;case 0x32b560u:return 4u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    std::uint32_t count=0u;
    if(target==0x1ae090u) {
        count=rng()%5?std::uint32_t(rng()%5):0u-std::uint32_t(rng()%3);
        for(std::uint32_t k=0;k<count&&k<6u;++k)for(std::uint32_t f=0;f<0x60u;f+=4)
            w(s.memory,s.observe,target+1u,arg(7)+0x60u*k+f,4,std::uint32_t(rng()));
    }
    if(target==0x32b560u && rng()%2)w(s.memory,s.observe,target+1u,arg(1)+4u*std::uint32_t(rng()%24),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x14cdb0u)s.gp[0]=rng()%5?std::uint32_t(rng()%4):0xffffffffu;
    if(target==0x1ae090u)s.gp[0]=count;
    if(target==0x1af3e0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?0u:std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%5);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+(target==0x32b560u?0u:4u*args)+cleanup;
}
// 32A660: ESP moves below the caller's by EAX; the probed words are read.
void probe(WorldState &s) {
    log_call(entry_hash(s,0x32a660u));
    const auto ret=r(s.memory,s.gp[4]),top=s.gp[4]+4u-s.gp[0];
    for(auto at=s.gp[4]+4u-0x1000u;at>top;at-=0x1000u)(void)r(s.memory,at);
    (void)r(s.memory,top);
    w(s.memory,s.observe,0x32a69fu,top-4u,4,ret);
    s.gp[4]=top;s.gp[0]=ret;
    s.flags.logic(ret&0x1808u,4);
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1ae090(WorldState &s){stub(s,0x1ae090u);}
static void run_32b560(WorldState &s){stub(s,0x32b560u);}
static void run_1af3e0(WorldState &s){stub(s,0x1af3e0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x32a660u)probe(s);
    else if(target==0x14cdb0u)stub(s,target);
    else throw std::runtime_error("Unbound combat action dispatch target");
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecc0cu,query);m.put(0x4ecc18u,structure);m.put(0x52be40u,std::uint32_t(rng()));
    for(std::uint32_t at=query;at<query+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=structure;at<structure+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(structure+0x30u,leaves);
    for(std::uint32_t k=0;k<4;++k){m.put(leaves+8u*k,chance(4)?0xffffu:pick(8),2);m.put(leaves+8u*k+2u,std::uint32_t(rng()),2);
        m.put(leaves+8u*k+4u,std::uint32_t(rng()));}
    for(std::uint32_t at=ray;at<ray+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=chosen;at<chosen+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x1900u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ad000u+pick(0x100));
    for(std::uint32_t k=1;k<=4;++k)m.put(stack+4u*k,std::uint32_t(rng()));
    m.put(stack+20u,chosen);
    random_abi(rng,c,stack);
    c.abi.gp[1]=ray;
    if(shape==15) {
        static constexpr std::uint32_t areas[]{query,structure,leaves,chosen};
        m.drop(areas[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1adf60(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0x1ae090u:original::run_1ae090(v);break;case 0x32b560u:original::run_32b560(v);break;
            case 0x1af3e0u:original::run_1af3e0(v);break;default:throw std::logic_error("Unbound aim choice child");}},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1adf60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_AIM_CHOICE");
    std::mt19937 rng(0x1adf60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
