// Native TU1.5 marker chains (116060) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children are genuine stand-ins whose effects are a hash of their
// complete entry ABI, their stack arguments and the return address: 1D4140
// returns a node count; 1E5250 and 1E51D0 step the record iterator (a few
// records, markers NONE, zero or real) and return AL; BAD60 writes the
// pose's node (negative and repeated nodes too) and returns AX; 1E1390 pops
// its arguments. All of them return ECX/EDX, flags and XMM, write scratch,
// and occasionally clobber callee-saved registers or leave the stack
// unbalanced. Fixtures author objects, their tags and models, definitions
// whose tags are missing, without nodes or with nodes, weights around the
// floor (special floats, NaN payloads) and both walk kinds; one region
// removed injects a fault; a set direction flag is refused.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_marker_chains.hpp"
using namespace frame_misc_harness;
namespace chains=halo2::native_marker_chains_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,instances=0x10200000u,
    tags=0x10300000u,records=0x10400000u,scratch=0x10600000u,none=0xffffffffu;
std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,0x00000001u,
        0x80000000u,0x00400000u};
    return values[rng()%8];
}
void common(WorldState &s,std::mt19937_64 &rng,std::uint32_t target,std::uint32_t arguments) {
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==5?records+0x80u:which==6?records+0x40u:which==3?instances:std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
std::mt19937_64 begin(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    return std::mt19937_64(h);
}
void count_stub(WorldState &s) {
    auto rng=begin(s,0x1d4140u,0);
    s.gp[0]=rng()%9==0?std::uint32_t(rng()%300):std::uint32_t(rng()%64);
    common(s,rng,0x1d4140u,0);
}
void walk_stub(WorldState &s,std::uint32_t target) {
    auto rng=begin(s,target,0);
    const auto it=s.gp[6];
    const auto step=r(s.memory,it+0x14u);
    w(s.memory,s.observe,target+2u,it+0x14u,4,step+1u);
    if(step<1u+std::uint32_t(rng()%5)) {
        static constexpr std::uint32_t kinds[]{0u,none};
        w(s.memory,s.observe,target+3u,it,4,rng()%6==0?kinds[rng()%2]:std::uint32_t(rng()%64)+1u);
        w(s.memory,s.observe,target+4u,it+4u,4,rng()%6==0?kinds[rng()%2]:std::uint32_t(rng()%64)+1u);
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|1u;
    } else s.gp[0]=std::uint32_t(rng())&0xffffff00u;
    common(s,rng,target,0);
}
void marker_stub(WorldState &s) {
    auto rng=begin(s,0xbad60u,3);
    const auto out=s.gp[3];
    const auto node=rng()%7==0?0x10000u-1u-std::uint32_t(rng()%3):std::uint32_t(rng()%12);
    w(s.memory,s.observe,0xbad61u,out,2,node);
    for(std::uint32_t k=1;k<8;++k)
        w(s.memory,s.observe,0xbad62u,out+4u*k,4,rng()%9==0?special_float(rng):std::bit_cast<std::uint32_t>(float(rng()%17)*0.5f));
    s.gp[0]=(std::uint32_t(rng())&0xffff0000u)|(rng()%5==0?0u:std::uint32_t(rng()%0xffff)+1u);
    common(s,rng,0xbad60u,3);
}
void chain_stub(WorldState &s){auto rng=begin(s,0x1e1390u,4);s.gp[0]=std::uint32_t(rng());common(s,rng,0x1e1390u,4);}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1d4140(WorldState &s){count_stub(s);}
static void run_1e5250(WorldState &s){walk_stub(s,0x1e5250u);}
static void run_1e51d0(WorldState &s){walk_stub(s,0x1e51d0u);}
static void run_bad60(WorldState &s){marker_stub(s);}
static void run_1e1390(WorldState &s){chain_stub(s);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    std::mt19937_64 wide(rng());
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    m.put(0x4f0414u,instances);
    for(std::uint32_t k=0;k<8;++k) {
        const auto tag=tags+0x100u*k;
        m.put(instances+16u*k,std::uint32_t(rng()));m.put(instances+16u*k+4u,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,pick(8)==0?0u:tag);m.put(instances+16u*k+12u,std::uint32_t(rng()));
        for(std::uint32_t at=tag;at<tag+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tag+0xcu,(std::uint32_t(rng())&0xffff0000u)|(pick(5)==0?0u:pick(20)));
        m.put(tag+0x38u,(std::uint32_t(rng())<<16)|pick(8));
    }
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x100u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|pick(8));
    }
    // The definition and the record iterator.
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records+0x14u,0u);m.put(records+0x94u,0u);                              // a second iterator
    const auto definition=records+0x40u;
    m.put(definition+0x68u,(std::uint32_t(rng())<<16)|pick(8));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    m.put(0x46a0e8u,std::bit_cast<std::uint32_t>(0.0f));
    std::uint32_t weight=std::bit_cast<std::uint32_t>(float(int(pick(9))-2)*0.25f);
    if(shape==4 && pick(2)==0)weight=special_float(wide);
    const std::uint32_t object_handle=pick(9)==0?none:(std::uint32_t(rng()%0x7fff)<<16)|pick(8);
    const std::uint32_t other_handle=pick(9)==0?none:(std::uint32_t(rng()%0x7fff)<<16)|pick(8);
    const std::uint32_t args[]{object_handle,other_handle,(std::uint32_t(rng())&0xffffff00u)|pick(2),weight,definition,
        records,scratch+0x40u,scratch+0x80u};
    for(std::uint32_t k=0;k<8;++k)m.put(stack+4u+4u*k,args[k]);
    random_abi(rng,c,stack);
    if(shape!=14)c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,instances,tags,records,0x46a000u};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return chains::bridge(s,[](auto &v){original::native_frame_misc_reference_116060(v);},
        [](auto &v){original::run_1d4140(v);},[](auto &v){original::run_1e5250(v);},[](auto &v){original::run_1e51d0(v);},
        [](auto &v){original::run_bad60(v);},[](auto &v){original::run_1e1390(v);});
}
void reference(WorldState &s){original::run_116060(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_MARKER_CHAINS");
    std::mt19937 rng(0x116060u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t,(c.abi.eflags&0x400u)!=0))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,chains::coverage().load(),unsigned(chains::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
