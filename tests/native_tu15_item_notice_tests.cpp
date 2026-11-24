// Native TU1.5 item notice (1D9F10) against the unmodified translated body of
// the death cycle unit (the validator repeats it with the physics live
// support unit's), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). 315CB0 (through the unit's dispatch) is a hash-driven stub
// of its complete entry ABI: it may change the count, writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy ESI, EDI or EBX change or the stack is left unbalanced. Fixtures
// author records with and without flag bit 5 and counts of either sign;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_item_notice.hpp"
using namespace frame_misc_harness;
namespace notice=halo2::native_item_notice_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,items=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x315cb0u);
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%9==0)w(s.memory,s.observe,0x315cb1u,records+0x74u,4,std::uint32_t(rng()%5));
    else w(s.memory,s.observe,0x315cb1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[6]=records+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[7]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%17==0)s.gp[3]=0x60u*std::uint32_t(rng()%4);
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
    if(target!=0x315cb0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    for(std::uint32_t g=0;g<2;++g) {
        const auto record=records+0x80u*g;
        for(std::uint32_t at=record;at<record+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+4u,pick(4)?std::uint32_t(rng())|0x20u:std::uint32_t(rng())&~0x20u);
        m.put(record+0x70u,items+0x200u*g);m.put(record+0x74u,pick(5)==0?0x80000000u+pick(4):pick(5));
    }
    for(std::uint32_t at=items;at<items+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=records+0x80u*pick(2);
    if(shape==15)m.drop(pick(2)?records:items);
    return c;
}
bool native(WorldState &s) {
    return notice::bridge(s,[](auto &v){original::native_frame_misc_reference_1d9f10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1d9f10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ITEM_NOTICE");
    std::mt19937 rng(0x1d9f10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,notice::coverage().load(),unsigned(notice::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
