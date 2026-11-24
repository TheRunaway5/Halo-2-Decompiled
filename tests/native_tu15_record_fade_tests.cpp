// Native TU1.5 record fade (17FB60) against the unmodified translated body
// of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children 140310 (the expiry) and 32CC0C (the CRT float-to-integer
// conversion, which pops the logical x87 value it is given) are genuine
// stand-ins whose effects are a hash of their complete entry ABI and return
// address: they return EAX (32CC0C a byte-range integer), ECX/EDX, flags and
// XMM, write scratch, and occasionally clobber callee-saved registers or
// leave the stack unbalanced. Fixtures author the record array, records
// (held, counted, chains of counted and uncounted records), start ticks,
// lifetimes (zero, negative, reached, not reached), fade times, the game
// time and the fade constants (special floats, NaN payloads); at most seven
// live x87 registers; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_record_fade.hpp"
using namespace frame_misc_harness;
namespace record_fade=halo2::native_record_fade_detail;
namespace {
constexpr std::uint32_t array=0x10000000u,records=0x10010000u,counters=0x10100000u,time_block=0x10200000u,
    scratch=0x10600000u,none=0xffffffffu;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(target==0x32cc0cu) {
        s.fp.pop();s.gp[0]=std::uint32_t(rng()%256);
        if(policy&2u && rng()%7==0)s.gp[6]=records+64u*std::uint32_t(rng()%32);   // another record in ESI
    }
    else s.gp[0]=std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        s.gp[saved[rng()%4]]=rng()%2?records+64u*std::uint32_t(rng()%32):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_140310(WorldState &s){stub(s,0x140310u);}
static void run_32cc0c(WorldState &s){stub(s,0x32cc0cu);}
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
    const auto real=[&](float low,float high){
        if(shape==4 && pick(6)==0)return quiet_nan(rng);
        if(shape==3 && pick(6)==0) {
            static constexpr std::uint32_t values[]{0x7f800000u,0xff800000u,0x00000001u,0x80000000u,0x00000000u};
            return values[pick(5)];
        }
        return std::bit_cast<std::uint32_t>(low+(high-low)*float(pick(10001))/10000.0f);
    };
    m.put(0x4f7220u,array);
    for(std::uint32_t at=array;at<array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(array+0x44u,records);
    const std::int32_t now=std::int32_t(pick(3000));
    for(std::uint32_t k=0;k<32;++k) {
        const auto record=records+64u*k;
        for(std::uint32_t at=record;at<record+64u;at+=4)m.put(at,std::uint32_t(rng()));
        std::uint32_t flags=std::uint32_t(rng())&0xffu;
        if(pick(3))flags&=~2u;
        m.put(record+2u,flags,1);
        m.put(record+0x18u,std::uint32_t(now-std::int32_t(pick(600))));
        m.put(record+0x1cu,pick(6)==0?0u:real(-1.0f,15.0f));
        m.put(record+0x20u,real(-1.0f,10.0f));
        m.put(record+0x3cu,none);
    }
    // A chain from record 0 through a few others.
    std::uint32_t previous=0;
    for(unsigned i=0,length=pick(5);i<length;++i) {
        const auto k=1u+pick(31);
        if(k==previous)break;
        m.put(records+64u*previous+0x3cu,(std::uint32_t(rng()%0x7fff)<<16)|k);m.put(records+64u*k+0x3cu,none);
        previous=k;
    }
    m.put(0x4f721cu,counters);m.put(counters+0x3804u,pick(20));
    m.put(0x51df54u,time_block);m.put(time_block,std::uint32_t(rng()));m.put(time_block+4u,real(0.01f,0.05f));
    m.put(time_block+8u,std::uint32_t(now));
    m.put(0x515f28u,std::uint32_t(rng()));
    m.put(0x46a26cu,pick(6)?std::bit_cast<std::uint32_t>(255.0f):real(-300.0f,300.0f));
    m.put(0x46a148u,pick(6)?std::bit_cast<std::uint32_t>(255.0f):real(0.0f,300.0f));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x17fb1bu);
    random_abi(rng,c,stack);
    if(c.abi.x87_depth>7u)c.abi.x87_depth=7u;
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|(pick(4)==0?0u:pick(32));
    if(shape==15) {
        static constexpr std::uint32_t regions[]{array,records,counters,time_block,0x46a000u,0x51d000u};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return record_fade::bridge(s,[](auto &v){original::native_frame_misc_reference_17fb60(v);},
        [](auto &v){original::run_140310(v);},[](auto &v){original::run_32cc0c(v);});
}
void reference(WorldState &s){original::run_17fb60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_FADE");
    std::mt19937 rng(0x17fb60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,record_fade::coverage().load(),unsigned(record_fade::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
