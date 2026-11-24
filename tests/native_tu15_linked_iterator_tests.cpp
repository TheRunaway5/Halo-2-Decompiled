// Native TU1.5 linked data array iterator (1A8F10) against the unmodified
// translated body of the CTF player iterator unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 16EA60 is a genuine stand-in whose effects are a hash of its
// complete entry ABI and return address: it returns the next in-use index
// at or after EDX (or NONE), EAX/ECX/EDX and flags, and occasionally
// clobbers callee-saved registers (the iterator, the array) or leaves the
// stack unbalanced. Fixtures author arrays (datum sizes, maximum counts,
// first unallocated indices zero, negative, past the bits), in-use bits,
// salts, links (runs of records without one, the last one without) and iterators at every position (NONE, negative, past the end); one
// region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_linked_iterator.hpp"
using namespace frame_misc_harness;
namespace iteration=halo2::native_linked_iterator_detail;
namespace {
constexpr std::uint32_t array=0x10000000u,bits=0x10010000u,records=0x10100000u,iterators=0x10200000u,scratch=0x10600000u;
std::uint32_t datum_size=0x30u;
void first_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x16ea60u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x16ea61u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto start=s.gp[2];
    s.gp[0]=start<60u && rng()%6?start+std::uint32_t(rng()%3):0xffffffffu;
    if(s.gp[0]!=0xffffffffu && s.gp[0]>=60u)s.gp[0]=0xffffffffu;
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        const auto which=rng()%2?6u:7u;
        s.gp[which]=which==6?iterators+0x10u*std::uint32_t(rng()%4):(rng()%2?array:std::uint32_t(rng()));
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x16ea60u)throw std::runtime_error("Unbound dispatch target");
    first_stub(s);
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
    static constexpr std::uint32_t sizes[]{0x30u,0x40u,0x84u};
    datum_size=sizes[pick(3)];
    for(std::uint32_t at=array;at<array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(array+0x20u,64u+pick(8));m.put(array+0x24u,datum_size);
    std::uint32_t bound=pick(64);
    if(shape==6)bound=0x80000000u|pick(8);
    m.put(array+0x38u,bound);m.put(array+0x44u,records);m.put(array+0x48u,bits);
    for(std::uint32_t at=bits-0x20u;at<bits+0x20u;at+=4)m.put(at,std::uint32_t(rng()|rng()));
    for(std::uint32_t k=0;k<64;++k) {
        const auto record=records+datum_size*k;
        for(std::uint32_t at=record;at<record+datum_size;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x2cu,pick(3)==0?std::uint32_t(rng()):0xffffffffu);   // most have no link
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto it=iterators+0x10u*k;
        m.put(it,std::uint32_t(rng()));m.put(it+4u,array);m.put(it+8u,std::uint32_t(rng()));
        std::uint32_t index=pick(64);
        if(pick(6)==0)index=0xffffffffu;
        if(pick(10)==0)index=0x80000000u|pick(4);
        m.put(it+0xcu,index);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1a9000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[6]=iterators+0x10u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,bits,records,iterators,records+0x1000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return iteration::bridge(s,[](auto &v){original::native_frame_misc_reference_1a8f10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1a8f10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_LINKED_ITERATOR");
    std::mt19937 rng(0x1a8f10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,iteration::coverage().load(),unsigned(iteration::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
