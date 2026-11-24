// Native TU1.5 record frame (1DC610) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1D8B90 and BC190 (through the unit's dispatch) and 1DC680 (direct) are
// hash-driven stubs of their complete entry ABIs and arguments: each fills
// part of the frame and scratch, pops its arguments (none, 4 and 4 bytes),
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EDI or ESI change or the stack is left unbalanced.
// Fixtures author records whose marker is negative, one of their markers or
// past them; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_record_frame.hpp"
using namespace frame_misc_harness;
namespace record_frame=halo2::native_record_frame_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto frame=target==0x1d8b90u?s.gp[6]:target==0xbc190u?r(s.memory,s.gp[4]+4u):s.gp[1];
    w(s.memory,s.observe,target+1u,frame+4u*std::uint32_t(rng()%13),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=records+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
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
static void run_1dc680(WorldState &s){stub(s,0x1dc680u,1);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x1d8b90u)stub(s,target,0);
    else if(target==0xbc190u)stub(s,target,1);
    else throw std::runtime_error("Unbound sandbox dispatch target");
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
        static constexpr std::uint32_t markers[]{0xffu,0x80u,0u,1u,3u,7u};
        m.put(record+0x18u,markers[pick(6)],1);
        m.put(record+0x74u,pick(4)==0?0xffffffffu:pick(6));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=records+0x80u*pick(2);
    if(shape==15)m.drop(records);
    return c;
}
bool native(WorldState &s) {
    return record_frame::bridge(s,[](auto &v){original::native_frame_misc_reference_1dc610(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},[](auto &v){original::run_1dc680(v);});
}
void reference(WorldState &s){original::run_1dc610(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_FRAME");
    std::mt19937 rng(0x1dc610u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,record_frame::coverage().load(),unsigned(record_frame::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
