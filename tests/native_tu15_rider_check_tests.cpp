// Native TU1.5 rider check (1AE6F0) against the unmodified translated body
// of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// D2B00 and 1E7810 (through the unit's dispatch) are hash-driven stubs of
// their complete entry ABIs: D2B00 fills the cursor with a unit (seated or
// not) or ends the chain, 1E7810 answers AL; both write scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP or EDI change or the stack is left unbalanced. Fixtures
// author vehicles, units of several teams, and seats with and without flag
// bit 11; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_rider_check.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_rider_check_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,instances=0x10030000u,definitions=0x10040000u,
    seats=0x10050000u,results=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0xd2b00u)for(std::uint32_t k=0;k<16;k+=4)h=mix(h,r(s.memory,s.gp[6]+k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xd2b00u) {
        if(rng()%3) {
            w(s.memory,s.observe,target,s.gp[6]+4u,4,(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%8));
            w(s.memory,s.observe,target,s.gp[6]+8u,2,rng()%4?std::uint32_t(rng()%4):0xffffu);
            w(s.memory,s.observe,target,s.gp[6]+0xcu,4,std::uint32_t(rng()));
            s.gp[0]=objects+0x200u*std::uint32_t(rng()%8);
        } else s.gp[0]=0u;
    } else s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3==0?1u:0u);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=(s.gp[3]&0xffffff00u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%11==0)s.gp[5]=definitions+0x400u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[7]=(s.gp[7]&0xffff0000u)|std::uint32_t(rng()%4);
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
    if(target!=0xd2b00u && target!=0x1e7810u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<8;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t at=objects;at<objects+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k){m.put(objects+0x200u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(objects+0x200u*k+0x138u,pick(4),2);}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x400u*k);}
    for(std::uint32_t at=definitions;at<definitions+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(definitions+0x400u*k+0x1ccu,seats+0x400u*k);
    for(std::uint32_t at=seats;at<seats+0x1000u;at+=4)m.put(at,pick(2)?std::uint32_t(rng())|0x800u:std::uint32_t(rng())&~0x800u);
    for(std::uint32_t at=results;at<results+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1ae000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,results+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[1]=(std::uint32_t(rng())&0xffff0000u)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions,seats,results};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ae6f0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1ae6f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RIDER_CHECK");
    std::mt19937 rng(0x1ae6f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
