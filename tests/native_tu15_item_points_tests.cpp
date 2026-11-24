// Native TU1.5 item points (15D890) against the unmodified translated body
// of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The engine's first method, 15CAB0, D9110 and 15D4F0 (through the
// dispatch) and 15BA10, 15D670 and 161AE0 (direct) are hash-driven stubs of
// their complete entry ABIs: each hashes what it reads (its pushed
// arguments), writes scratch, answers EAX (the method and 15CAB0 NONE or a
// value, 15D670 a time) or AL; the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or
// the stack is left unbalanced. Fixtures author points of every mode, limit,
// kind and flag, records with and without objects of every type and state,
// timers idle, running and ending, positions near and far (and NaN), game
// settings of every scale, and tick rates; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_item_points.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_item_points_detail;
namespace {
constexpr std::uint32_t game=0x10000000u,tags=0x10001000u,definitions=0x10002000u,globals_definition=0x10003000u,block=0x10004000u,
    respawn=0x10005000u,clock=0x10006000u,scenario=0x10007000u,points=0x10008000u,session=0x10009000u,engines=0x1000a000u,
    objects=0x1000b000u,entries=0x1000c000u,bodies=0x10010000u,records=0x1000d000u,scratch=0x10600000u,method_base=0x170000u;
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0x15cab0u:case 0xd9110u:return 1u;case 0x15d4f0u:return 2u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target>=method_base && target<method_base+4u)s.gp[0]=rng()%3?std::uint32_t(rng()%8):0xffffffffu;
    if(target==0x15cab0u)s.gp[0]=rng()%4?std::uint32_t(rng()%8):0xffffffffu;
    if(target==0x15d670u)s.gp[0]=rng()%4?std::uint32_t(rng()%400):std::uint32_t(rng());
    if(target==0x15ba10u||target==0x161ae0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=rng()%2?0xffffffffu:std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=points+0x90u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=records+8u*std::uint32_t(rng()%4)+4u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_15ba10(WorldState &s){stub(s,0x15ba10u);}
static void run_15d670(WorldState &s){stub(s,0x15d670u);}
static void run_161ae0(WorldState &s){stub(s,0x161ae0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x15cab0u && target!=0xd9110u && target!=0x15d4f0u && !(target>=method_base && target<method_base+4u))
        throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto real=[&](float lo,float hi){return lo+(hi-lo)*float(rng()%10000)/10000.0f;};
    const auto fill=[&](std::uint32_t from,std::uint32_t size){for(std::uint32_t at=from;at<from+size;at+=4)m.put(at,std::uint32_t(rng()));};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f3218u,game);m.put(0x4ecc1cu,tags);m.put(0x4f0414u,definitions);m.put(0x51df54u,clock);m.put(0x4ecc20u,scenario);
    m.put(0x4f63b8u,session);m.put(0x4ecbd0u,objects);
    m.put(0x4500b8u,bits(chance(8)?real(0.0f,100.0f):4.0f));m.put(0x46a0d0u,0x3f800000u);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,chance(4)?0u:engines+0x10u*k);
    for(std::uint32_t k=0;k<4;++k){m.put(engines+0x10u*k,engines+0x100u+4u*k);m.put(engines+0x100u+4u*k,method_base+k);}
    fill(game,0x220u);
    m.put(game+0xcu,chance(10)?4u:pick(4),1);m.put(game+0x208u,pick(5),1);m.put(game+0x211u,pick(5),1);
    fill(tags,0x180u);m.put(tags+0x16cu,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t k=0;k<4;++k){fill(definitions+16u*k,16u);m.put(definitions+16u*k+8u,globals_definition);}
    fill(globals_definition,0x20u);m.put(globals_definition+0xcu,block);
    fill(block+0x530u,0x10u);m.put(block+0x534u,respawn);
    fill(respawn,0x40u);m.put(respawn+0x28u,bits(real(0.5f,3.0f)));
    fill(clock,0x10u);m.put(clock+2u,1u+pick(3),2);m.put(clock+8u,pick(1000));
    fill(scenario,0x130u);m.put(scenario+0x120u,chance(10)?0u:1u+pick(4));m.put(scenario+0x124u,points);
    fill(session,0xc20u);m.put(session+0xc14u,pick(4));
    // Points and their records' objects.
    fill(objects,0x50u);m.put(objects+0x44u,entries);
    for(std::uint32_t k=0;k<4;++k) {
        const auto p=points+0x90u*k;
        fill(p,0x90u);
        m.put(p,std::uint32_t(rng())&0xffu,1);m.put(p+0x10u,chance(3)?0u:pick(4),2);m.put(p+0x12u,pick(3),2);m.put(p+0x14u,chance(3)?0u:pick(9),1);
        const float px=real(-20.0f,20.0f),py=real(-20.0f,20.0f),pz=real(-5.0f,5.0f);
        m.put(p+0x40u,bits(px));m.put(p+0x44u,bits(py));m.put(p+0x48u,bits(pz));
        fill(entries+12u*k,12u);m.put(entries+12u*k+8u,bodies+0x200u*k);
        const auto o=bodies+0x200u*k;
        fill(o,0x200u);
        const float spread=chance(2)?0.5f:6.0f;
        m.put(o+0x30u,bits(chance(12)?std::numeric_limits<float>::quiet_NaN():px+real(-spread,spread)));
        m.put(o+0x34u,bits(py+real(-spread,spread)));m.put(o+0x38u,bits(pz+real(-spread,spread)));
        m.put(o+0xaau,pick(5),1);m.put(o+0x10au,std::uint32_t(rng())&0xffu,1);m.put(o+0x12cu,chance(2)?0u:pick(4),1);
        m.put(records+8u*k,chance(4)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|k);
        m.put(records+8u*k+4u,chance(2)?0u:chance(2)?1u:pick(6),2);m.put(records+8u*k+6u,pick(5),2);
    }
    fill(scratch,0x100u);
    fill(stack-0x80u,0xc0u);
    m.put(stack,0x15d000u+pick(0x100));m.put(stack+4u,records);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t areas[]{game,tags,definitions,block,clock,scenario,points,session,objects,records};
        m.drop(areas[pick(10)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15d890(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0x15ba10u:original::run_15ba10(v);break;case 0x15d670u:original::run_15d670(v);break;
            case 0x161ae0u:original::run_161ae0(v);break;default:throw std::logic_error("Unbound item points child");}},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_15d890(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ITEM_POINTS");
    std::mt19937 rng(0x15d890u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
