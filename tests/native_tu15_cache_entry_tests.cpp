// Native TU1.5 geometry cache entry check (130D20) against the unmodified
// translated bodies of the physics, model, and shadow frame units, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 22A790 is a genuine stand-in whose effects are a hash of its
// complete entry ABI, its argument and the return address: it pops the
// block, returns AL, ECX/EDX, flags and XMM, writes scratch, and
// occasionally clobbers callee-saved registers or leaves the stack
// unbalanced. Fixtures author entries used this frame, long ago and
// recently (negative ages too), resident or not, with owners ready or not;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/native_tu15_cache_entry.hpp"
using namespace frame_misc_harness;
namespace cache=halo2::native_cache_entry_detail;
namespace {
constexpr std::uint32_t entries_array=0x10000000u,entry_data=0x10010000u,cache_block=0x10100000u,records_array=0x10200000u,
    record_data=0x10210000u,owners=0x10300000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x22a790u);
    for(std::uint32_t k=0;k<2;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x22a791u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
    if(policy&4u)throw std::runtime_error("Interrupted 22A790");
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
[[maybe_unused]] static void run_22a790(WorldState &s){stub(s);}
[[maybe_unused]] static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail) {
    if(target!=0x22a790u || site!=0x130d92u || tail)
        throw std::logic_error("Unexpected geometry cache child");
    stub(s);
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
    m.put(0x4f2d5cu,entries_array);m.put(0x4f2d6cu,cache_block);m.put(0x4f2d64u,0x11000000u+pick(16)*0x100u);
    for(std::uint32_t at=entries_array;at<entries_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(entries_array+0x44u,entry_data);
    for(std::uint32_t at=cache_block;at<cache_block+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t now=1000u+pick(100);
    m.put(cache_block+0x38u,now);m.put(cache_block+0x34u,pick(2)==0?std::uint32_t(rng()):4u+pick(4));
    m.put(cache_block+0x64u,records_array);
    for(std::uint32_t at=records_array;at<records_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records_array+0x44u,record_data);
    for(std::uint32_t k=0;k<16;++k) {
        const auto entry=entry_data+12u*k,record=record_data+24u*k,owner=owners+0x40u*k;
        for(std::uint32_t at=entry;at<entry+12u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(entry,(std::uint32_t(rng())&0xff00ffffu)|((pick(4)?pick(3):0u)<<16));
        m.put(entry+8u,owner);
        for(std::uint32_t at=record;at<record+24u;at+=4)m.put(at,std::uint32_t(rng()));
        const std::uint32_t ages[]{0u,1u,4u,5u,6u,100u,0xffffffffu};
        m.put(record+0x14u,now-ages[pick(7)]);m.put(record+8u,pick(64));
        for(std::uint32_t at=owner;at<owner+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(owner+0x1cu,(std::uint32_t(rng())&0xff00ffffu)|((pick(3)?1u:0u)<<16));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130000u+pick(0x100));
    const auto index=pick(16);
    m.put(stack+4u,(std::uint32_t(rng())<<16)|index);
    random_abi(rng,c,stack);
    if(shape==14) {
        m.put(entry_data+12u*index+2u,1u,1);
        m.put(owners+0x40u*index+0x1eu,1u,1);
        m.put(record_data+24u*index+0x14u,now-1u);
        c.policy|=4u;
    }
    if(shape==15) {
        static constexpr std::uint32_t regions[]{entries_array,entry_data,cache_block,records_array,record_data,owners};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return cache::bridge(s,[](auto &v){original::native_frame_misc_reference_130d20(v);},
        [](auto &v){original::dispatch(v,0x22a790u,0x130d92u,false);});
}
void reference(WorldState &s){original::run_130d20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CACHE_ENTRY");
    std::mt19937 rng(0x130d20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t,false,{},true))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,cache::coverage().load(),unsigned(cache::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
