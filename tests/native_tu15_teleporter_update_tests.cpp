// Native TU1.5 teleporter update (1A6830) against the unmodified translated
// body of the teleporter unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1A8F10 (through the dispatch), 1A6930 and 1A6A40 are hash-driven stubs of
// their complete entry ABIs: 1A8F10 advances the iterator it is given and
// answers whether a player remains, 1A6930 sets players' inside bits, both
// teleporter children may change the session's count and 1A6A40 the session; each writes scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author players' flags, teleporter counts negative,
// zero and several; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/world/native_tu15_teleporter_update.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_teleporter_update_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,records=0x10010000u,session=0x10020000u,moved=0x10030000u,scratch=0x10600000u;
std::uint32_t limit=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x1a6a40u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1a8f10u) {
        const auto iterator=s.gp[6];
        const auto index=r(s.memory,iterator+0xcu)+1u;
        const bool more=index<limit;
        w(s.memory,s.observe,0x1a8f11u,iterator,4,more?records+0x40u*index:0u);
        w(s.memory,s.observe,0x1a8f12u,iterator+8u,4,more?(std::uint32_t(rng()&0x7fffu)<<16)|index:0xffffffffu);
        w(s.memory,s.observe,0x1a8f13u,iterator+0xcu,4,index);
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(more?1u+std::uint32_t(rng()%255):0u);
    } else {
        if(target==0x1a6930u && limit)w(s.memory,s.observe,0x1a6931u,records+0x40u*std::uint32_t(rng()%limit)+2u,1,r(s.memory,records+2u)|0x80u);
        if(rng()%9==0)w(s.memory,s.observe,target+1u,session+0xbfcu,4,std::uint32_t(rng()%4));
        if(target==0x1a6a40u && rng()%7==0)w(s.memory,s.observe,target+2u,0x4f63b8u,4,moved);
        s.gp[0]=std::uint32_t(rng());
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng())&0xffffff80u;
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng())|0xfe00u;
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[7]=target==0x1a8f10u?std::uint32_t(rng())|0xff00u:session;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x1a6a40u?8u:4u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1a6930(WorldState &s){stub(s,0x1a6930u);}
static void run_1a6a40(WorldState &s){stub(s,0x1a6a40u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1a8f10u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f54f4u,players);m.put(0x4f63b8u,session);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x40u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=session+0xaf0u;at<session+0xc00u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xbfcu,pick(5)?pick(4):0u-pick(3));
    for(std::uint32_t at=moved+0xaf0u;at<moved+0xc00u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(moved+0xbfcu,pick(5));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1a0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,records,session,moved};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1a6830(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_1a6930(v);},[](auto &v){original::run_1a6a40(v);});
}
void reference(WorldState &s){original::run_1a6830(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TELEPORTER_UPDATE");
    std::mt19937 rng(0x1a6830u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        limit=std::uint32_t(rng()%6);
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
