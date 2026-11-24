// Native TU1.5 director bind (171FF0) against the unmodified translated body
// of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 175380 (through the unit's dispatch) is a hash-driven stub of its complete
// entry ABI: it may rewrite the record's started byte or its object, writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy ESI or EDI change or the stack is left unbalanced.
// Fixtures author records started or not and objects that alias the
// record's or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_director_bind.hpp"
using namespace frame_misc_harness;
namespace bind=halo2::native_director_bind_detail;
namespace {
constexpr std::uint32_t records=0x4f64a4u,objects=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x175380u);
    log_call(h);
    std::mt19937_64 rng(h);
    switch(rng()%4) {
    case 0:w(s.memory,s.observe,0x175381u,s.gp[6]+0xb5u,1,std::uint32_t(rng()%2));break;
    case 1:w(s.memory,s.observe,0x175381u,s.gp[6]+4u,4,objects+0x100u*std::uint32_t(rng()%4));break;
    default:w(s.memory,s.observe,0x175381u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));break;
    }
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x358u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x100u*std::uint32_t(rng()%4);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x175380u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
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
    for(std::uint32_t at=records&~3u;at<records+4u*0x358u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(records+0x358u*k+0xb5u,pick(2)?0u:1u+pick(255),1);
    for(std::uint32_t at=objects;at<objects+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=objects+0x100u*pick(4);c.abi.gp[1]=pick(4);
    if(shape==15)m.drop(objects);
    return c;
}
bool native(WorldState &s) {
    return bind::bridge(s,[](auto &v){original::native_frame_misc_reference_171ff0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_171ff0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DIRECTOR_BIND");
    std::mt19937 rng(0x171ff0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,bind::coverage().load(),unsigned(bind::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
