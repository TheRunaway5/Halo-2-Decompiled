// Native TU1.5 animation record (1E5DF0) against the unmodified translated
// body of the physics live support unit (the validator repeats it with the
// AI frame unit's), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 1E3150 and 1E5A70 are hash-driven stubs of
// their complete entry ABIs and arguments: 1E3150 answers an entry address,
// 1E5A70 a value, each pops the reference and writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author references whose high word is NONE or not and records
// over the entries; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_record.hpp"
using namespace frame_misc_harness;
namespace record=halo2::native_animation_record_detail;
namespace {
constexpr std::uint32_t entries=0x10000000u,records=0x10010000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:entries+4u*std::uint32_t(rng()%32),4,std::uint32_t(rng()));
    s.gp[0]=target==0x1e3150u?entries+4u*std::uint32_t(rng()%16):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=entries+4u*std::uint32_t(rng()%16);
    if(policy&2u && rng()%13==0)s.gp[7]=records+4u*std::uint32_t(rng()%8);
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
static void run_1e3150(WorldState &s){stub(s,0x1e3150u);}
static void run_1e5a70(WorldState &s){stub(s,0x1e5a70u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=entries;at<entries+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    m.put(stack+4u,std::uint32_t(rng()));
    m.put(stack+8u,((pick(4)==0?0xffffu:pick(0xffff))<<16)|pick(0x10000));
    random_abi(rng,c,stack);
    c.abi.gp[7]=shape==7?entries+4u*pick(16):records+4u*pick(8);   // the record over the entries
    if(shape==15)m.drop(pick(2)?entries:records);
    return c;
}
bool native(WorldState &s) {
    return record::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5df0(v);},
        [](auto &v){original::run_1e3150(v);},[](auto &v){original::run_1e5a70(v);});
}
void reference(WorldState &s){original::run_1e5df0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_RECORD");
    std::mt19937 rng(0x1e5df0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,record::coverage().load(),unsigned(record::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
