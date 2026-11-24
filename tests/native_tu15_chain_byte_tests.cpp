// Native TU1.5 chain byte propagation (17FAF0) against the unmodified
// translated body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 17FB60 is a genuine stand-in whose effects are a hash of its
// complete entry ABI and return address: it returns AL (accept or not),
// writes a record word, returns EAX/ECX/EDX and flags, and occasionally
// clobbers callee-saved registers (the byte, the next record) or leaves the
// stack unbalanced. Fixtures author the record array, heads and non-heads,
// chains of every length (NONE at once, long), bytes; one region removed
// injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_chain_byte.hpp"
using namespace frame_misc_harness;
namespace chain=halo2::native_chain_byte_detail;
namespace {
constexpr std::uint32_t array=0x10000000u,records=0x10010000u,scratch=0x10600000u,none=0xffffffffu;
constexpr std::uint32_t record_count=48u;
void accept_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x17fb60u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x17fb61u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    w(s.memory,s.observe,0x17fb62u,records+std::uint32_t(rng()%record_count)*64u+0x20u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3==0?1u:0u);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        const auto which=rng()%2?6u:7u;
        s.gp[which]=which==6?(rng()%2?none:(std::uint32_t(rng()%0x7fff)<<16)|std::uint32_t(rng()%record_count)):std::uint32_t(rng());
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
static void run_17fb60(WorldState &s){accept_stub(s);}
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
    m.put(array+0x44u,records);
    std::vector<std::uint32_t> handle(record_count);
    for(std::uint32_t k=0;k<record_count;++k)handle[k]=(std::uint32_t(rng()%0x7fff)<<16)|k;
    for(std::uint32_t k=0;k<record_count;++k) {
        const auto record=records+64u*k;
        for(std::uint32_t at=record;at<record+64u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x3cu,none);
    }
    // A chain from the head through distinct later records.
    const auto head=pick(record_count/2);
    m.put(records+64u*head+0x38u,pick(6)?handle[head]:handle[pick(record_count)]);
    auto previous=head;
    const unsigned length=shape==5?0u:pick(8);
    for(unsigned i=0;i<length;++i) {
        const auto k=record_count/2+pick(record_count/2);
        if(k==previous)break;
        m.put(records+64u*previous+0x3cu,handle[k]);m.put(records+64u*k+0x3cu,none);
        previous=k;
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x180591u);
    random_abi(rng,c,stack);
    c.abi.gp[2]=handle[head];
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,records,0x4f7000u,scratch};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return chain::bridge(s,[](auto &v){original::native_frame_misc_reference_17faf0(v);},[](auto &v){original::run_17fb60(v);});
}
void reference(WorldState &s){original::run_17faf0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHAIN_BYTE");
    std::mt19937 rng(0x17faf0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,chain::coverage().load(),unsigned(chain::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
