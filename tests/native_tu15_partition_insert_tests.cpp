// Native TU1.5 cluster partition insert (1D2F10) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children are genuine stand-ins whose effects are a hash of their
// complete entry ABI and return address: 14D080 (the cluster set's
// clusters) writes the found count through EDI and fills the list; 14D0E0
// (the sphere query) fills the list from its stack arguments; both return a
// count that may exceed the list (with any upper bits). 16E6A0 (the data
// array allocate) returns a handle in the pool EDX names or NONE, and
// sometimes the other pool in EDX. All of them return EAX/ECX/EDX, flags
// and XMM, write scratch, and occasionally clobber callee-saved registers or
// leave the stack unbalanced. Fixtures author partitions (cluster heads with
// negative clusters, entry and reference pools with several datum sizes),
// locations with and without a cluster, radii around the query floor
// (special floats, NaN payloads), payloads of every size modulo 4 (none,
// overlapping the entries), the object's list head and the overflowed byte;
// one region removed injects a fault; a set direction flag is refused. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_partition_insert.hpp"
using namespace frame_misc_harness;
namespace insert=halo2::native_partition_insert_detail;
namespace {
constexpr std::uint32_t partition=0x10000000u,heads=0x10100000u,entries=0x10200000u,entry_data=0x10210000u,
    references=0x10300000u,reference_data=0x10310000u,arguments=0x10400000u,payloads=0x10500000u,
    scratch=0x10600000u,none=0xffffffffu,capacity=64u;
std::uint32_t list_clusters=16u;
std::uint32_t common(WorldState &s,std::mt19937_64 &rng,std::uint32_t target) {
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%2?scratch+4u*std::uint32_t(rng()%32):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    return policy&2u && rng()%37==0?4u*std::uint32_t(rng()%3):0u;
}
// The list: count clusters (in range, negative, or out of range).
std::uint32_t fill(WorldState &s,std::mt19937_64 &rng,std::uint32_t list,std::uint32_t target) {
    const std::uint32_t found=rng()%8==0?250u+std::uint32_t(rng()%20):std::uint32_t(rng()%6);
    const auto written=std::min(found,256u);
    for(std::uint32_t i=0;i<written;++i) {
        std::uint32_t cluster=std::uint32_t(rng()%list_clusters);
        if(rng()%9==0)cluster=0x10000u-1u-std::uint32_t(rng()%4);
        w(s.memory,s.observe,target+2u,list+2u*i,2,cluster);
    }
    return found;
}
void gather_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x14d080u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto list=r(s.memory,s.gp[4]+8u);
    const auto found=fill(s,rng,list,0x14d080u);
    w(s.memory,s.observe,0x14d083u,s.gp[7],2,found);
    s.gp[0]=(rng()%3==0?std::uint32_t(rng())<<16:0u)|std::min(found,rng()%4==0?300u:256u);
    s.gp[2]=std::uint32_t(rng());
    const auto cleanup=common(s,rng,0x14d080u);
    s.gp[4]+=12u+cleanup;
}
void query_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x14d0e0u);
    for(std::uint32_t k=0;k<6;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto list=r(s.memory,s.gp[4]+0x14u);
    const auto found=fill(s,rng,list,0x14d0e0u);
    s.gp[0]=(rng()%3==0?std::uint32_t(rng())<<16:0u)|(rng()%11==0?0x8000u+std::uint32_t(rng()%16):found);
    s.gp[2]=std::uint32_t(rng());
    const auto cleanup=common(s,rng,0x14d0e0u);
    s.gp[4]+=0x18u+cleanup;
}
void allocate_stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x16e6a0u);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto pool=s.gp[2];
    if(rng()%6==0)s.gp[0]=none;
    else {
        const auto index=std::uint32_t(rng()%capacity);
        s.gp[0]=(std::uint32_t(rng()%0x7fff)<<16)|index;
        if(pool==entries || pool==references) {
            const auto record=index*r(s.memory,pool+0x24u)+r(s.memory,pool+0x44u);
            w(s.memory,s.observe,0x16e6a1u,record,4,s.gp[0]);
        }
    }
    if(rng()%13==0)s.gp[2]=pool==entries?references:entries;
    const auto cleanup=common(s,rng,0x16e6a0u);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_14d080(WorldState &s){gather_stub(s);}
static void run_14d0e0(WorldState &s){query_stub(s);}
static void run_16e6a0(WorldState &s){allocate_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t quiet_nan(std::mt19937 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7fc12345u,0xffc54321u,0xff800003u};
    return payloads[rng()%6];
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t sizes[]{16u,20u,24u,36u};
    const auto entry_size=sizes[pick(4)],reference_size=12u+4u*pick(3);
    list_clusters=4u+pick(12);
    for(std::uint32_t at=partition;at<partition+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(partition,heads);m.put(partition+4u,entries);m.put(partition+8u,references);
    for(const auto pool:{entries,references}) {
        for(std::uint32_t at=pool;at<pool+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(pool+0x24u,pool==entries?entry_size:reference_size);m.put(pool+0x44u,pool==entries?entry_data:reference_data);
    }
    for(std::uint32_t at=entry_data;at<entry_data+capacity*entry_size+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=reference_data;at<reference_data+capacity*reference_size;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=heads-0x40u;at<heads+0x80u;at+=4)m.put(at,pick(3)?(std::uint32_t(rng())<<16)|pick(capacity):none);
    // The arguments' targets: the list head, the center, the location, the overflowed byte.
    for(std::uint32_t at=arguments;at<arguments+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto head_slot=arguments+4u*pick(4),center=arguments+0x20u,location=arguments+0x40u,
        overflowed=arguments+0x60u+pick(4);
    m.put(head_slot,pick(3)?(std::uint32_t(rng())<<16)|pick(capacity):none);
    std::uint32_t cluster=pick(list_clusters);
    if(pick(5)==0)cluster=0xffffu;else if(pick(9)==0)cluster=0x10000u-2u-pick(3);
    m.put(location+4u,(std::uint32_t(rng())<<16)|cluster);
    for(std::uint32_t at=payloads;at<payloads+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t payload=pick(5)==0?0u:payloads+pick(16);
    if(shape==7 && payload)payload=entry_data+pick(capacity)*entry_size;       // overlapping the entries
    const auto payload_size=pick(4)==0?0u:pick(29);
    // The radius around the query floor.
    const auto floor=pick(3)?std::bit_cast<std::uint32_t>(0.5f):std::bit_cast<std::uint32_t>(float(pick(5))*0.25f);
    std::uint32_t radius=std::bit_cast<std::uint32_t>(float(pick(9))*0.125f);
    if(shape==4 && pick(3)==0)radius=quiet_nan(rng);
    if(shape==3 && pick(3)==0) {
        static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x00400000u};
        radius=values[pick(5)];
    }
    m.put(0x46a0e8u,floor);
    m.put(0x4f3ce0u,std::uint32_t(rng()));m.put(0x4f3ce4u,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    const std::uint32_t object=(std::uint32_t(rng()%0x7fff)<<16)|pick(256);
    const std::uint32_t args[]{partition,object,head_slot,center,radius,location,payload_size,payload,overflowed};
    for(std::uint32_t k=0;k<9;++k)m.put(stack+4u+4u*k,args[k]);
    random_abi(rng,c,stack);
    // The direction flag is clear at every call in the game; a set one is
    // refused (shape 14 keeps it to prove that).
    if(shape!=14)c.abi.eflags&=~0x400u;
    c.abi.gp[0]=pick(3)==0?scratch+4u*pick(8):0u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{partition,heads,entries,entry_data,references,reference_data,arguments,
            payloads,0x46a000u,0x4f3000u};
        m.drop(regions[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return insert::bridge(s,[](auto &v){original::native_frame_misc_reference_1d2f10(v);},
        [](auto &v){original::run_14d080(v);},[](auto &v){original::run_14d0e0(v);},[](auto &v){original::run_16e6a0(v);});
}
void reference(WorldState &s){original::run_1d2f10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PARTITION_INSERT");
    std::mt19937 rng(0x1d2f10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,insert::coverage().load(),unsigned(insert::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
