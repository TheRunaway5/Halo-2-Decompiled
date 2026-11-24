// Native TU1.5 record cursor step (1E5160) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5C80 is a hash-driven stub of its complete entry ABI and argument: it
// pops the handle, writes the cursor or scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy ESI
// or EDI change or the stack is left unbalanced. Fixtures author cursors at
// and past the list's count (negative indexes too), handles whose high word
// is NONE or not, and cursors over the records; one region removed injects
// a fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_record_cursor.hpp"
using namespace frame_misc_harness;
namespace cursor=halo2::native_record_cursor_detail;
namespace {
constexpr std::uint32_t lists=0x10000000u,records=0x10010000u,cursors=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,rng()%2?scratch+std::uint32_t(rng()%64)*4u:cursors+4u*std::uint32_t(rng()%16),4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[7]=std::uint32_t(rng());
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
static void run_1e5c80(WorldState &s){stub(s,0x1e5c80u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=lists;at<lists+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto list=lists+4u*pick(4);
    m.put(list+0x3cu,pick(5)==0?0xffffffffu:1u+pick(6));
    m.put(list+0x40u,records+4u*pick(4));
    for(std::uint32_t at=records;at<records+0x28u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t e=0;e<8;++e)if(pick(3)==0)m.put(records+0x28u*e+4u+4u*pick(2)+2u,0xffffu,2);
    for(std::uint32_t at=cursors;at<cursors+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    auto cursor_address=cursors+4u*pick(8);
    m.put(cursor_address+0x2au,pick(6)==0?0xffffu-pick(3):pick(7),2);
    if(shape==7)cursor_address=records+4u*pick(40);                 // the cursor over the records
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=cursor_address;c.abi.gp[2]=list;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{lists,records,cursors};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s){return cursor::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5160(v);},[](auto &v){original::run_1e5c80(v);});}
void reference(WorldState &s){original::run_1e5160(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_CURSOR");
    std::mt19937 rng(0x1e5160u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,cursor::coverage().load(),unsigned(cursor::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
