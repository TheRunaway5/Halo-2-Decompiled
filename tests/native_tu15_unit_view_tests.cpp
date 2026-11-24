// Native TU1.5 unit view (244F80) against the unmodified translated body of
// the combat action unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). CD4A0, BCDF0 and BAD60 (through the unit's
// dispatch) are hash-driven stubs of their complete entry ABIs and
// arguments: CD4A0 writes the position, BCDF0 answers a vehicle or 0, BAD60
// fills the marker's record and answers a count or 0; all write scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy ESI, EDI or EBP change or the stack is left unbalanced. Fixtures
// author units seated or not, seats with and without flag bit 7 (and
// negative seat indices); one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_view.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_view_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,data=0x10010000u,units=0x10020000u,vehicles=0x10030000u,instances=0x10040000u,
    definitions=0x10050000u,seats=0x10060000u,outputs=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xcd4a0u)w(s.memory,s.observe,target,s.gp[0]+4u*std::uint32_t(rng()%3),4,std::uint32_t(rng()));
    if(target==0xbad60u)for(std::uint32_t k=0;k<0x70u;k+=4)if(rng()%2)w(s.memory,s.observe,target,s.gp[3]+k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0xbcdf0u?(rng()%4?vehicles+0x40u*std::uint32_t(rng()%4):0u):std::uint32_t(rng());
    if(target==0xbad60u && rng()%4==0)s.gp[0]&=0xffff0000u;
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=units+0x400u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=outputs+4u*std::uint32_t(rng()%8);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0xcd4a0u:stub(s,target,0);return;
    case 0xbcdf0u:stub(s,target,1);return;
    case 0xbad60u:stub(s,target,3);return;
    }
    throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,objects);m.put(0x4f0414u,instances);
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,data);
    for(std::uint32_t k=0;k<4;++k){m.put(data+12u*k,std::uint32_t(rng()));m.put(data+12u*k+4u,std::uint32_t(rng()));m.put(data+12u*k+8u,units+0x400u*(k%2));}
    for(std::uint32_t u=0;u<2;++u) {
        const auto unit=units+0x400u*u;
        for(std::uint32_t at=unit;at<unit+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(unit+0x14u,pick(4)?std::uint32_t(rng()):0xffffffffu);
        m.put(unit+0x1fcu,pick(8)?pick(4):0xffffu-pick(2),2);
    }
    for(std::uint32_t v=0;v<4;++v){m.put(vehicles+0x40u*v,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(vehicles+0x40u*v+4u,std::uint32_t(rng()));}
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x200u*k);}
    for(std::uint32_t k=0;k<4;++k){m.put(definitions+0x200u*k+0x1ccu,seats+0x200u+0x400u*k);}
    for(std::uint32_t at=seats;at<seats+0x1000u;at+=4)m.put(at,std::uint32_t(rng())&(pick(2)?0xffffff7fu:0xffffffffu));
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x240000u+pick(0x100));m.put(stack+4u,outputs+4u*pick(4));m.put(stack+8u,outputs+0x20u+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[1]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{objects,data,units,instances,definitions,seats,outputs};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_244f80(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_244f80(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_VIEW");
    std::mt19937 rng(0x244f80u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
