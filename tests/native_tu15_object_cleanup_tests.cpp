// Native TU1.5 multiplayer object cleanup (15C4E0) against the unmodified
// translated body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children are genuine stand-ins whose effects are a hash of their
// complete entry ABI, return address and argument words: the iterator BCEE0
// steps the frame's iterator through object indices (by one or by the
// 30-tick period, so many land on the current slot) and returns the
// object's data or zero; 105FD0, 164750 and the variant's check return AL;
// BBBC0 and BA570 mark the deleted object; all return EAX/ECX/EDX, flags,
// XMM and MXCSR sticky bits, and occasionally clobber callee-saved
// registers or leave the stack unbalanced. Fixtures author the engine mode
// (quiet or not), the variant (setting, index, the check through its
// table), the game time (ticks negative and positive), object types (items,
// weapons tied to the engine or not, bipeds, others), holders, flags, ticks
// and timeouts (special floats, NaN payloads); one region removed injects a
// fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_object_cleanup.hpp"
using namespace frame_misc_harness;
namespace cleanup=halo2::native_object_cleanup_detail;
namespace {
constexpr std::uint32_t engine_block=0x10000000u,variant_block=0x10010000u,variant_objects=0x10020000u,
    tables=0x10030000u,time_block=0x10040000u,headers=0x10100000u,rows=0x10110000u,objects=0x10200000u,
    tags=0x10300000u,definitions=0x10310000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=48u,object_stride=0x300u,check_base=0x160000u;
std::uint32_t stack=0x20010000u;
void finish(WorldState &s,std::mt19937_64 &rng,std::uint32_t words) {
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%3?std::uint32_t(rng()%64):0x80000000u|std::uint32_t(rng()%64);
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup_bytes=4u*words;
    if(policy&2u && rng()%37==0)cleanup_bytes=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup_bytes;
}
std::mt19937_64 enter(WorldState &s,std::uint32_t target,unsigned words) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=words;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    return rng;
}
void iterate(WorldState &s) {
    auto rng=enter(s,0xbcee0u,1);
    const auto iterator=r(s.memory,s.gp[4]+4u);
    const auto handle=r(s.memory,iterator+8u);
    const auto tick=std::int32_t(r(s.memory,time_block+8u));
    std::uint32_t index;
    if(handle==0xffffffffu)index=std::uint32_t(((tick%30)+30)%30)+(rng()%3==0?1u:0u);
    else index=(handle&0xffffu)+(rng()%4==0?1u:30u);
    std::uint32_t result=0;
    if(index<object_count*2u && rng()%16) {
        w(s.memory,s.observe,0xbcee2u,iterator+8u,4,(std::uint32_t(rng()%0x7fff)<<16)|index);
        result=objects+(index%object_count)*object_stride;
    }
    s.gp[0]=result;finish(s,rng,1);
}
void answer(WorldState &s,std::uint32_t target,unsigned words) {
    auto rng=enter(s,target,words);
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|std::uint32_t(rng()%2);
    if(target==0xbbbc0u||target==0xba570u)w(s.memory,s.observe,target+2u,objects+(r(s.memory,s.gp[4]+4u)&0x3fu)*object_stride+0x200u,4,std::uint32_t(rng()));
    finish(s,rng,words);
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_164750(WorldState &s){answer(s,0x164750u,0);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0xbcee0u)iterate(s);
    else if(target==0x105fd0u)answer(s,target,0);
    else if(target==0xbbbc0u||target==0xba570u)answer(s,target,1);
    else if(target>=check_base && target<check_base+0x100u)answer(s,target,1);
    else throw std::runtime_error("Unbound multiplayer engine dispatch target");
}
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
    const auto real=[&](float low,float high){
        if(shape==4 && pick(6)==0)return quiet_nan(rng);
        if(shape==3 && pick(5)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x00000000u};
            return values[pick(5)];
        }
        return std::bit_cast<std::uint32_t>(low+(high-low)*float(pick(10001))/10000.0f);
    };
    stack=0x20010000u+pick(64)*4u;
    m.put(0x4f3218u,engine_block);
    for(std::uint32_t at=engine_block;at<engine_block+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(engine_block+0xcu,shape==5?4u:pick(4)==0?pick(8):1u,1);
    m.put(0x4f63b8u,variant_block);
    for(std::uint32_t at=variant_block+0xc00u;at<variant_block+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    const std::uint32_t setting=pick(4);
    m.put(variant_block+0xc0cu,setting);m.put(variant_block+0xc14u,pick(4));
    for(std::uint32_t k=0;k<4;++k) {
        m.put(0x56dfa0u+4u*k,variant_objects+0x10u*k);
        m.put(variant_objects+0x10u*k,tables+0x100u*k);
        for(std::uint32_t at=tables+0x100u*k;at<tables+0x100u*k+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(tables+0x100u*k+0x98u,check_base+0x10u*k);
    }
    for(std::uint32_t k=0;k<12;++k)m.put(0x44e9f0u+4u*k,real(0.0f,60.0f));
    m.put(0x51df54u,time_block);
    m.put(time_block,std::uint32_t(rng()));m.put(time_block+4u,real(0.01f,0.1f));
    std::int32_t tick=std::int32_t(pick(5000));
    if(pick(8)==0)tick=-std::int32_t(pick(100));
    m.put(time_block+8u,std::uint32_t(tick));
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,rows);
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k) {
        for(unsigned w=0;w<4;++w)m.put(tags+16u*k+4u*w,std::uint32_t(rng()));
        m.put(tags+16u*k+8u,definitions+0x300u*k);
        m.put(definitions+0x300u*k+0x290u,pick(3)==0?1u+pick(4):0u,2);
    }
    for(std::uint32_t k=0;k<object_count*2u;++k) {
        m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));
        m.put(rows+12u*k+8u,objects+(k%object_count)*object_stride);
    }
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto o=objects+k*object_stride;
        for(std::uint32_t at=o;at<o+object_stride;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(o,(std::uint32_t(rng())<<16)|pick(4));
        static constexpr std::uint32_t types[]{0,0,1,2,2,3,4,5,6,9,12,0xff};
        m.put(o+0xaau,types[pick(12)],1);
        m.put(o+0xafu,pick(2)?0xffu:pick(0xff),1);
        const auto tick_near=[&]{return std::uint32_t(tick-std::int32_t(pick(3000)));};
        m.put(o+0x20u,pick(4)?tick_near():std::uint32_t(tick+1+std::int32_t(pick(50))));
        m.put(o+0xbcu,tick_near());m.put(o+0x150u,tick_near());m.put(o+0x2e0u,tick_near());
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x400u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x15d000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{engine_block,variant_block,tables,time_block,rows,objects,0x44e000u,definitions};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return cleanup::bridge(s,[](auto &v){original::native_frame_misc_reference_15c4e0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_164750(v);});
}
void reference(WorldState &s){original::run_15c4e0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_CLEANUP");
    std::mt19937 rng(0x15c4e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,cleanup::coverage().load(),unsigned(cleanup::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
