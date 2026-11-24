// Native TU1.5 marked record pass (179A40) against the unmodified translated
// body (the shadow frame unit's copy; the validator repeats it with the lens
// flare unit's, where 17C710 is entered directly), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 16EA60 (the first in-use index), 17C710 and 17CD10 are
// genuine stand-ins whose effects are a hash of their complete entry ABI,
// return address and argument words: they return EAX (an index or NONE for
// 16EA60), write record words (flags, the in-use bits, the bound), return
// ECX/EDX and flags, and occasionally clobber callee-saved registers or
// leave the stack unbalanced. Fixtures author the array (datum size 400,
// bounds zero, negative, past the bits), in-use bits, salts and the flag
// bytes for every filter outcome; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_marked_records.hpp"
using namespace frame_misc_harness;
namespace marked=halo2::native_marked_records_detail;
namespace {
constexpr std::uint32_t array=0x10000000u,bits=0x10010000u,records=0x10100000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    std::uint32_t result=std::uint32_t(rng());
    if(target==0x16ea60u)result=rng()%5==0?0xffffffffu:std::uint32_t(rng()%40);
    else {
        const auto roll=rng()%8;
        if(roll==0)w(s.memory,s.observe,target+2u,records+0x190u*std::uint32_t(rng()%48)+2u,2,std::uint32_t(rng()));
        if(roll==1)w(s.memory,s.observe,target+3u,bits+4u*std::uint32_t(rng()%2),4,std::uint32_t(rng()));
        if(roll==2)w(s.memory,s.observe,target+4u,array+0x38u,4,std::uint32_t(rng()%60));
    }
    s.gp[0]=result;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==3?(rng()%2?array:std::uint32_t(rng())):which==5?(rng()%2?0xffffffffu:std::uint32_t(rng()%48)):std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t cleanup=4u*words;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
[[maybe_unused]] static void run_17c710(WorldState &s){stub(s,0x17c710u,2);}
static void run_17cd10(WorldState &s){stub(s,0x17cd10u,1);}
[[maybe_unused]] static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x16ea60u)stub(s,target,0);
    else if(target==0x17c710u)stub(s,target,2);
    else throw std::runtime_error("Unbound shadow frame dispatch target");
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
    m.put(0x4f720cu,array);
    for(std::uint32_t at=array;at<array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(array+0x24u,0x190u);
    std::uint32_t bound=pick(60);
    if(shape==6)bound=0x80000000u|pick(8);
    m.put(array+0x38u,bound);m.put(array+0x44u,records);m.put(array+0x48u,bits);
    for(std::uint32_t at=bits-0x20u;at<bits+0x20u;at+=4)m.put(at,std::uint32_t(rng()&rng()));
    for(std::uint32_t k=0;k<64;++k) {
        const auto record=records+0x190u*k;
        m.put(record,std::uint32_t(rng()));
        std::uint32_t flags=std::uint32_t(rng())&0xffffu;
        if(pick(2))flags&=~0x4cu;                                  // more records pass the filter
        m.put(record+2u,flags,2);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x17a000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,bits,records,0x4f7000u,records+0x1000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return marked::bridge(s,[](auto &v){original::native_frame_misc_reference_179a40(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_17cd10(v);});
}
void reference(WorldState &s){original::run_179a40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MARKED_RECORDS");
    std::mt19937 rng(0x179a40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,marked::coverage().load(),unsigned(marked::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
