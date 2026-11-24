// Native TU1.5 cluster partition removal (1D30F0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child 16E8A0 (the data array free, the unit's dispatch route) is a
// genuine stand-in whose effects are a hash of its complete entry ABI and
// return address: it writes the freed datum's first word, returns
// EAX/ECX/EDX and flags, and occasionally clobbers callee-saved registers
// or leaves the stack unbalanced. Fixtures author partitions (cluster heads,
// entry and reference pools with several datum sizes), objects with no,
// one and many references (clusters with no chain, the object's entry
// first, later or missing, other owners, negative cluster indices), and a
// head slot that overlaps the pools; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_partition_removal.hpp"
using namespace frame_misc_harness;
namespace removal=halo2::native_partition_removal_detail;
namespace {
constexpr std::uint32_t partition=0x10000000u,heads=0x10100000u,entries=0x10200000u,entry_data=0x10210000u,
    references=0x10300000u,reference_data=0x10310000u,head_slots=0x10400000u,scratch=0x10600000u;
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t stack=0x20010000u;
void free_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x16e8a0u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x16e8a1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    const auto pool=s.gp[2];
    const auto record=(s.gp[0]&0xffffu)*r(s.memory,pool+0x24u)+r(s.memory,pool+0x44u);
    w(s.memory,s.observe,0x16e8a2u,record,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64);
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
    if(target!=0x16e8a0u)throw std::runtime_error("Unbound live support dispatch target");
    free_stub(s);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t sizes[]{12u,16u,20u};
    const auto entry_size=sizes[pick(3)],reference_size=sizes[pick(3)];
    const std::uint32_t clusters=4u+pick(12),capacity=96u;
    const std::uint32_t object=(std::uint32_t(rng()%0x7fff)<<16)|pick(256);
    for(std::uint32_t at=partition;at<partition+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(partition,heads);m.put(partition+4u,entries);m.put(partition+8u,references);
    for(const auto pool:{entries,references}) {
        for(std::uint32_t at=pool;at<pool+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(pool+0x24u,pool==entries?entry_size:reference_size);m.put(pool+0x44u,pool==entries?entry_data:reference_data);
    }
    for(std::uint32_t at=entry_data;at<entry_data+capacity*entry_size;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=reference_data;at<reference_data+capacity*reference_size;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=heads-0x40u;at<heads+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    // Cluster chains with other owners, the object's entry inserted in some.
    std::uint32_t next_entry=0;
    const auto handle=[&](std::uint32_t index){return (std::uint32_t(rng()%0x7fff)<<16)|index;};
    std::vector<std::uint32_t> mine;
    // Clusters -4..-1 (negative as 16 bits) have chains before the heads.
    for(std::int32_t signed_cluster=-4;signed_cluster<std::int32_t(clusters);++signed_cluster) {
        const auto k=std::uint32_t(signed_cluster)&0xffffu;
        const unsigned length=pick(5);
        const bool owned=pick(3)!=0;
        const unsigned position=length?pick(length+1u):0u;
        std::uint32_t first=none,previous=none;
        for(unsigned i=0;i<length+(owned?1u:0u) && next_entry<capacity;++i) {
            const auto index=next_entry++;const auto record=entry_data+index*entry_size;
            const bool is_mine=owned && i==position;
            m.put(record+4u,is_mine?object:object^(1u+pick(4)));m.put(record+8u,none);
            const auto h=handle(index);
            if(previous==none)first=h;else m.put(entry_data+previous*entry_size+8u,h);
            previous=index;
            if(is_mine)mine.push_back(k);
        }
        m.put(heads+4u*std::uint32_t(signed_cluster),first);
    }
    // The object's references: clusters it is in (and a few it is not).
    std::uint32_t next_reference=0,first_reference=none,previous_reference=none;
    const unsigned count=shape==5?0u:1u+pick(6);
    for(unsigned i=0;i<count;++i) {
        const auto index=next_reference++;const auto record=reference_data+index*reference_size;
        std::uint32_t cluster=!mine.empty() && pick(4)?mine[pick(std::uint32_t(mine.size()))]:pick(clusters);
        if(shape==6 && pick(4)==0)cluster=0x10000u-1u-pick(4);                  // negative as 16 bits, maybe unowned
        m.put(record+4u,(std::uint32_t(rng())<<16)|cluster);m.put(record+8u,none);
        const auto h=handle(index);
        if(previous_reference==none)first_reference=h;else m.put(reference_data+previous_reference*reference_size+8u,h);
        previous_reference=index;
    }
    const auto head_slot=head_slots+4u*pick(8);
    for(std::uint32_t at=head_slots;at<head_slots+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(head_slot,first_reference);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));m.put(stack+4u,object);m.put(stack+8u,head_slot);
    random_abi(rng,c,stack);
    c.abi.gp[7]=partition;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{partition,heads,entries,entry_data,references,reference_data,head_slots,scratch};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return removal::bridge(s,[](auto &v){original::native_frame_misc_reference_1d30f0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1d30f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PARTITION_REMOVAL");
    std::mt19937 rng(0x1d30f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,removal::coverage().load(),unsigned(removal::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
