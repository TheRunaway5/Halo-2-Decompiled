// Native TU1.5 shadow schedule (4AC20) against the unmodified translated
// body of the shadow frame unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 4B150 (the record sort) is a hash-driven stub of its complete entry ABI:
// it hashes what it reads (its pushed arguments), writes scratch and now and
// then a record's word; the caller-saved registers, XMM, flags and (by
// policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the
// stack is left unbalanced. Fixtures author caches of every count, records
// with every flag, stale stamps, keys repeating their neighbours', weights
// and rates around zero and their maximum (and NaN), visible counts below,
// at and past the records, paused games, and the direction flag either
// way; one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/shadows/native_tu15_shadow_schedule.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_shadow_schedule_detail;
namespace {
constexpr std::uint32_t cache=0x10000000u,output=0x10001000u,pause_block=0x10002000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=3;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(rng()%3==0)w(s.memory,s.observe,target+1u,cache+0x24u*std::uint32_t(rng()%8)+0x18u,4,std::uint32_t(rng()%3)*0x3f000000u);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=cache;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[3+rng()%5],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+12u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_4b150(WorldState &s){stub(s,0x4b150u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto real=[&](float lo,float hi){return bits(lo+(hi-lo)*float(rng()%10000)/10000.0f);};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    const auto stamp=std::uint32_t(rng());
    m.put(0x4c6854u,stamp);
    m.put(0x51df50u,chance(3)?0u:pause_block);
    for(std::uint32_t at=pause_block;at<pause_block+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(pause_block+5u,chance(5)?1u:0u,1);
    for(std::uint32_t at=cache;at<cache+0x490u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto records=chance(8)?0u:1u+pick(12);
    m.put(cache+0x480u,records);
    std::uint32_t key=pick(4);
    for(std::uint32_t k=0;k<16;++k) {
        const auto r=cache+0x24u*k;
        m.put(r,chance(2)?0u:1u+pick(3),1);m.put(r+1u,chance(2)?0u:1u,1);m.put(r+2u,chance(3)?1u:0u,1);m.put(r+3u,chance(4)?1u:0u,1);
        if(!chance(3))key=chance(6)?0xffffffffu:pick(4);
        m.put(r+8u,key);
        m.put(r+0xcu,real(0.0f,1.0f));
        m.put(r+0x14u,chance(12)?0x7fc00001u:real(-0.5f,0.5f));
        m.put(r+0x18u,chance(6)?0u:chance(12)?0x7fc00002u:real(-0.2f,1.2f));
        m.put(r+0x20u,chance(2)?stamp:std::uint32_t(rng()));
    }
    for(std::uint32_t at=output;at<output+0x14u*32u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x4a000u+pick(0x100));m.put(stack+4u,cache);m.put(stack+8u,output);
    m.put(stack+12u,chance(4)?pick(3):chance(3)?records+pick(3):pick(records+1u));
    m.put(stack+16u,real(0.0f,0.1f));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{cache,output,pause_block};
        m.drop(areas[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_4ac20(v);},
        [](auto &v,std::uint32_t target){if(target!=0x4b150u)throw std::logic_error("Unbound shadow schedule child");original::run_4b150(v);});
}
void reference(WorldState &s){original::run_4ac20(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SHADOW_SCHEDULE");
    std::mt19937 rng(0x4ac20u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
