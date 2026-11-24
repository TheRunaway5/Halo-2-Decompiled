// Native TU1.5 record walk (180520) against the unmodified translated body of
// the combat action unit, extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). The children 43980 and 17FAF0 are genuine
// stand-ins whose effects are a hash of their complete entry ABI and return
// address: they write scratch (17FAF0 may clear in-use bits or move the
// bound), return EAX/ECX/EDX and flags, and occasionally clobber
// callee-saved registers (the array, the index) or leave the stack
// unbalanced. Fixtures author the array (valid or not), datum sizes, the
// first unallocated index (zero, negative, past the bits), in-use bits
// (sparse, dense, negative word indices) and salts; one region removed
// injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_record_walk.hpp"
using namespace frame_misc_harness;
namespace walk=halo2::native_record_walk_detail;
namespace {
constexpr std::uint32_t array=0x10000000u,bits=0x10010000u,records=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(target==0x17faf0u && rng()%5==0)w(s.memory,s.observe,0x17faf2u,bits+4u*std::uint32_t(rng()%4),4,std::uint32_t(rng()));
    if(target==0x17faf0u && rng()%9==0)w(s.memory,s.observe,0x17faf3u,array+0x38u,4,std::uint32_t(rng()%120));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        const auto which=rng()%2?6u:7u;
        s.gp[which]=which==6?std::uint32_t(rng()%140)-8u:(rng()%2?array:std::uint32_t(rng()));
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_43980(WorldState &s){stub(s,0x43980u);}
static void run_17faf0(WorldState &s){stub(s,0x17faf0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f7220u,array);
    for(std::uint32_t at=array;at<array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t sizes[]{0x40u,0x20u,0x1cu,0x84u};
    const auto size=sizes[pick(4)];
    m.put(array+0x24u,size);m.put(array+0x29u,shape==5?0u:1u+pick(255),1);
    std::uint32_t bound=pick(120);
    if(shape==6)bound=0x80000000u|pick(8);
    m.put(array+0x38u,bound);m.put(array+0x44u,records);m.put(array+0x48u,bits);
    for(std::uint32_t at=bits-0x40u;at<bits+0x40u;at+=4)m.put(at,shape==7?std::uint32_t(rng()):std::uint32_t(rng()&rng()&rng()));
    for(std::uint32_t at=records;at<records+size*120u+4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x180600u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,bits,records,0x4f7000u};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return walk::bridge(s,[](auto &v){original::native_frame_misc_reference_180520(v);},
        [](auto &v){original::run_43980(v);},[](auto &v){original::run_17faf0(v);});
}
void reference(WorldState &s){original::run_180520(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_WALK");
    std::mt19937 rng(0x180520u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,walk::coverage().load(),unsigned(walk::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
