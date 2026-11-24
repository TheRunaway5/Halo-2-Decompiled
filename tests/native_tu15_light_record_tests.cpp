// Native TU1.5 light record (C3820) against the unmodified translated body of
// the map world support unit, where C3A20 and C3AB0 are direct (the validator
// repeats it with the light portal unit's, where both go through the
// dispatch), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder
// generates (reference.inc). The children are hash-driven stubs of their
// complete entry ABIs: each rewrites the record's kind and scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, ESI or EDI change or the stack is left unbalanced. Fixtures
// author definitions with and without the always bit, split arguments,
// sessions with and without a variant record and the split flag, the game
// enabled or not, lights flagged or not, and records of each kind whose
// floats lie below, on and above the epsilon and the threshold (NaN too);
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "pstencil/native_tu15_light_record.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_light_record_detail;
namespace {
constexpr std::uint32_t lights=0x10000000u,datums=0x10010000u,instances=0x10020000u,definitions=0x10030000u,session=0x10040000u,
    game=0x10050000u,records=0x10060000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0xc3ab0u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%3==0)w(s.memory,s.observe,target,s.gp[7],2,std::uint32_t(rng()%2));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=definitions+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=datums+0x110u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=records+0x80u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0xc3ab0u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_c3a20(WorldState &s){stub(s,0xc3a20u);}
static void run_c3ab0(WorldState &s){stub(s,0xc3ab0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xc3a20u && target!=0xc3ab0u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
void step(WorldState &s,std::uint32_t target,std::uint32_t){stub(s,target);}
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbdcu,lights);m.put(0x4f0414u,instances);m.put(0x4f63b8u,session);m.put(0x51dae8u,game);
    m.put(0x46a0ecu,bits_of(0.001f));m.put(0x46a610u,bits_of(0.25f));
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(2)?0u:std::uint32_t(rng())|1u);
    for(std::uint32_t at=lights;at<lights+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(lights+0x44u,datums);
    for(std::uint32_t at=datums;at<datums+0x110u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(datums+0x110u*k+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x10u*k);}
    for(std::uint32_t k=0;k<4;++k)m.put(definitions+0x10u*k,(std::uint32_t(rng())&~0x100u)|(pick(3)?0u:0x100u));
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t at=game;at<game+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+8u,pick(6)?1u+pick(255):0u,1);
    static constexpr float values[]{0.f,0.001f,0.5f,-1.f,0.3f,0.36f,0.2f,2.f};
    for(std::uint32_t k=0;k<4;++k) {
        const auto record=records+0x80u*k;
        for(std::uint32_t at=record;at<record+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record,pick(2)?0u:1u+pick(3),2);
        for(const auto offset:{0x44u,0x64u,0x68u,0x6cu})m.put(record+offset,pick(12)?bits_of(values[pick(8)]):0x7fc00000u);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffffff00u)|(pick(2)?0u:1u+pick(255)));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records+0x80u*pick(4);c.abi.gp[2]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lights,datums,instances,definitions,session,game,records};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_c3820(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::step(v,target,site);});
}
void reference(WorldState &s){original::run_c3820(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LIGHT_RECORD");
    std::mt19937 rng(0xc3820u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
