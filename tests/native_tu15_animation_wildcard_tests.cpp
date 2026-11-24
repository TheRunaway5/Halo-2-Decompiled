// Native TU1.5 animation wildcard search (2A0100) against the unmodified
// translated body of the physics live support unit (the validator repeats
// it with the AI frame and AI actor units'), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E5810 is a hash-driven stub of its complete entry ABI and arguments: it
// answers an entry (or none) and pops its three arguments; it writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy ESI, EBX, EDI or EBP change or the stack is left
// unbalanced. The MCC class symbol is a stub that renames one key; the
// live support copy asks it and the AI copies do not, so the test first
// probes the copy under test (the bridge's symbol follows it). Fixtures
// author searches with keys among a few and the any key, every stage (and
// past the last), and kept entries or none; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "gameplay/units/native_tu15_animation_wildcard.hpp"
using namespace frame_misc_harness;
namespace wildcard=halo2::native_animation_wildcard_detail;
namespace {
constexpr std::uint32_t searches=0x10000000u,outputs=0x10010000u,scratch=0x10600000u;
constexpr std::uint32_t keys[]{0x1000a1u,0x2000b2u,0x3000c3u,0x30000d9u},renamed_from=0x2000b2u,renamed_to=0x2000ffu;
unsigned symbol_calls{};
bool copy_renames{};
void lookup_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1e5810u);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1e5811u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=rng()%3==0?0u:0x30000000u+0x40u*std::uint32_t(rng()%64);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[6]=searches+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[3]=outputs+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%29==0)s.gp[7]=std::uint32_t(rng()%2);
    if(policy&2u && rng()%29==0)s.gp[5]=std::uint32_t(rng()%2)*0x30000040u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+12u+cleanup;
}
}
namespace halo2 {
std::uint32_t source_weapon_animation_scoped_class_symbol_tu15(const SourcePhysicsContactMemory &,std::uint32_t original) {
    ++symbol_calls;
    return original==renamed_from?renamed_to:original;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1e5810(WorldState &s){lookup_stub(s);}
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
        const auto search=searches+0x40u*g;
        for(std::uint32_t at=search;at<search+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(search,0x30100000u+0x100u*pick(4));
        for(std::uint32_t k=0;k<3;++k)m.put(search+4u+4u*k,pick(3)==0?keys[3]:keys[pick(3)]);
        m.put(search+0x10u,pick(6)==0?8u+pick(3):pick(8));
        m.put(search+0x14u,pick(3)?0u:0x30000000u+0x40u*pick(64));
        m.put(search+0x18u,pick(3)?0u:0x30000000u+0x40u*pick(64));
    }
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x2a0000u+pick(0x100));
    m.put(stack+4u,outputs+4u*pick(4));m.put(stack+8u,outputs+0x10u+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[6]=searches+0x40u*pick(2);c.abi.gp[3]=outputs+0x20u+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{searches,outputs};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s) {
    return wildcard::bridge(s,[](auto &v){original::native_frame_misc_reference_2a0100(v);},[](auto &v){original::run_1e5810(v);},
        [](auto &v,std::uint32_t key){return copy_renames?halo2::source_weapon_animation_scoped_class_symbol_tu15(v.memory,key):key;});
}
void reference(WorldState &s){original::run_2a0100(s);}
// Whether this copy asks the class symbol: run its translated body until a
// search reaches the second level.
bool renames() {
    std::mt19937 probe(0x2a0101u);
    for(unsigned n=0;n<400 && !symbol_calls;++n) {
        const auto c=author(probe,n);
        auto memory=c.memory.clone();
        execute(memory,c,false,native,reference);
    }
    return symbol_calls!=0;
}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ANIMATION_WILDCARD");
    copy_renames=renames();
    std::fprintf(stderr,"copy %s the class symbol\n",copy_renames?"asks":"does not ask");
    std::mt19937 rng(0x2a0100u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,wildcard::coverage().load(),unsigned(wildcard::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
