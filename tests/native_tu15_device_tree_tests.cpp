// Native TU1.5 device tree (10C5D0) against the unmodified translated body
// of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// B9390 is a hash-driven stub of its complete entry ABI: it writes scratch,
// the caller-saved registers, XMM, flags and (by policy) MXCSR are random,
// and by policy the stack is left unbalanced. The children's marking is the
// owner again, as the unit's 10C5D0 enters it (native, the translated body
// only where it refuses). Fixtures author trees of up to
// eight objects of every type, children and siblings linked without cycles;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_device_tree.hpp"
using namespace frame_misc_harness;
namespace device_tree=halo2::native_device_tree_detail;
namespace {
constexpr std::uint32_t header=0x10000000u,rows=0x10010000u,objects=0x10020000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
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
static void run_b9390(WorldState &s){stub(s,0xb9390u);}
static void run_10c5d0(WorldState &s);
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(64)*4u;
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    const std::uint32_t count=1u+pick(8);
    for(std::uint32_t k=0;k<8;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x200u*k;
        for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0xaau,pick(3)==0?7u+pick(3):pick(32),1);
        m.put(object+0xcu,0xffffffffu);m.put(object+0x10u,0xffffffffu);
    }
    // Each object after the first hangs under an earlier one: the newest
    // child first, its siblings through +0xC.
    for(std::uint32_t k=1;k<count;++k) {
        const auto parent=objects+0x200u*pick(k);
        m.put(objects+0x200u*k+0xcu,r(m,parent+0x10u));
        m.put(parent+0x10u,(std::uint32_t(rng())<<16)|k);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x10c000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng())<<16);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{header,rows,objects};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return device_tree::bridge(s,[](auto &v){original::native_frame_misc_reference_10c5d0(v);},
        [](auto &v){original::run_b9390(v);},[](auto &v){if(!native(v))original::run_10c5d0(v);});
}
void reference(WorldState &s){original::run_10c5d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DEVICE_TREE");
    std::mt19937 rng(0x10c5d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,device_tree::coverage().load(),unsigned(device_tree::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
