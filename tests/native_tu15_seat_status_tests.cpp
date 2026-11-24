// Native TU1.5 seat status (161610) against the unmodified translated body
// of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1618B0 is a hash-driven stub of its complete entry ABI and argument: it
// answers a status code from a small set, writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX
// changes or the stack is left unbalanced. Fixtures author seats with and
// without players, records whose code matches or not, and statuses of
// negative, empty and several counts holding the code once, twice or not at
// all (and codes whose int16 sign-extends); one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_seat_status.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_seat_status_detail;
namespace {
constexpr std::uint32_t seats=0x10000000u,globals_tag=0x10010000u,instances=0x10020000u,multiplayer=0x10030000u,runtime=0x10040000u,
    statuses=0x10050000u,scratch=0x10600000u;
constexpr std::uint32_t codes[]{0u,1u,2u,5u,0xffffffffu,0xffff8001u,0x8001u};
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1618b0u);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1618b2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=codes[rng()%7];s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=rng()%3?std::uint32_t(rng()%4):std::uint32_t(rng());
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
static void run_1618b0(WorldState &s){stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f54f0u,seats);m.put(0x4ecc1cu,globals_tag);m.put(0x4f0414u,instances);
    for(std::uint32_t at=seats;at<seats+0x20u;at+=4)m.put(at,pick(5)<2?0xffffffffu:std::uint32_t(rng()));
    if(shape==4)for(std::uint32_t k=0;k<4;++k)m.put(seats+0xcu+4u*k,0xffffffffu);
    for(std::uint32_t k=0;k<12;++k)m.put(0x4f63d8u+4u*k,k%3==0?codes[pick(7)]:std::uint32_t(rng()));
    for(std::uint32_t at=globals_tag;at<globals_tag+0x180u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(globals_tag+0x16cu,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,multiplayer+0x40u*k);}
    for(std::uint32_t at=multiplayer;at<multiplayer+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(multiplayer+0x40u*k+0xcu,runtime+0x600u*(k%2));
    for(std::uint32_t r=0;r<2;++r) {
        const auto block=runtime+0x600u*r;
        for(std::uint32_t at=block+0x530u;at<block+0x540u;at+=4)m.put(at,std::uint32_t(rng()));
        static constexpr std::uint32_t counts[]{0u,1u,3u,6u,0xffffffffu};
        m.put(block+0x538u,counts[pick(5)]);m.put(block+0x53cu,statuses+0x100u*r);
    }
    for(std::uint32_t at=statuses;at<statuses+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<16;++k)m.put(statuses+0x1cu*k+4u,codes[pick(7)],2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{seats,globals_tag,instances,multiplayer,runtime,statuses};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_161610(v);},[](auto &v){original::run_1618b0(v);});}
void reference(WorldState &s){original::run_161610(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_STATUS");
    std::mt19937 rng(0x161610u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
