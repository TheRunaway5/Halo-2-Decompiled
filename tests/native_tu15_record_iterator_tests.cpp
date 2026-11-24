// Native TU1.5 keyed record iterator (1E5250) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 1E5810 (the record search) is a genuine stand-in whose effects
// are a hash of its complete entry ABI, return address and argument words:
// it returns zero or one of the authored blocks (for the owner's table) or
// records blocks (for a block's table), EAX/ECX/EDX and flags, and
// occasionally clobbers callee-saved registers (EBP the stage, EDI the
// found block) or leaves the stack unbalanced. Fixtures author the stage
// jump table, iterators at every stage (negative, past the last), indices
// (negative, at and past the block counts), keys, blocks with no, one and
// many records; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_record_iterator.hpp"
using namespace frame_misc_harness;
namespace iteration=halo2::native_record_iterator_detail;
namespace {
constexpr std::uint32_t owner=0x10000000u,iterators=0x10100000u,blocks=0x10200000u,records=0x10300000u,
    scratch=0x10600000u;
std::uint32_t stack=0x20010000u;
void search_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x1e5810u);
    for(unsigned k=0;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x1e5811u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto roll=rng()%8;
    s.gp[0]=roll<3?0u:blocks+0x20u*std::uint32_t(rng()%8);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==7?(rng()%2?0u:blocks+0x20u*std::uint32_t(rng()%8)):
            which==5?s.gp[5]+std::uint32_t(rng()%3):std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t cleanup=12u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%5);
    s.gp[4]+=4u+cleanup;
}
std::uint32_t branch_map(std::uint32_t p,SourcePhysicsContactImage){return p;}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){return branch_map(p,image);}
static void run_1e5810(WorldState &s){search_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    // The stage jump table as the image holds it.
    static constexpr std::uint32_t targets[]{0x1e528bu,0x1e52a8u,0x1e52b4u,0x1e52d2u};
    for(unsigned k=0;k<4;++k)m.put(0x1e5350u+4u*k,targets[k]);
    for(std::uint32_t at=owner;at<owner+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    // Blocks: count, records.
    for(std::uint32_t k=0;k<8;++k) {
        const auto block=blocks+0x20u*k;
        for(std::uint32_t at=block;at<block+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        std::uint32_t count=pick(5);
        if(pick(8)==0)count=0x80000000u|pick(3);
        m.put(block+0xcu,count);m.put(block+0x10u,records+0x40u*k);
    }
    for(std::uint32_t at=records;at<records+0x40u*8u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto iterator=iterators+0x20u*pick(4);
    for(std::uint32_t at=iterators;at<iterators+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t index=pick(4);
    if(pick(6)==0)index=0xffffu;
    if(pick(10)==0)index=0x8000u|pick(4);
    std::uint32_t stage=pick(5);
    if(shape==7)stage=0x8000u|pick(4);
    if(shape==8)stage=4u+pick(4);
    m.put(iterator+8u,(stage<<16)|index);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1e0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[3]=owner;c.abi.gp[6]=iterator;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{owner,iterators,blocks,records,0x1e5000u,scratch,records+0x100u,blocks};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return iteration::bridge(s,[](auto &v){original::native_frame_misc_reference_1e5250(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);},
        [](auto &v){original::run_1e5810(v);});
}
void reference(WorldState &s){original::run_1e5250(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_ITERATOR");
    std::mt19937 rng(0x1e5250u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,iteration::coverage().load(),unsigned(iteration::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
