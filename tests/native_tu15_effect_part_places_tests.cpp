// Native TU1.5 effect part places (17CD10) against the unmodified translated
// body of the shadow frame unit (the validator repeats it with the lens
// flare unit's copy), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). 17DD50, 145560 and 159690 (through the
// dispatch) are hash-driven stubs of their complete entry ABIs and
// arguments: the first two fill the frame buffers they are given, 159690
// may change the event's part count; each writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// events of negative, zero and several parts, scales below, at and above 0
// (NaN too), location chains of several records on nodes or not; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_effect_part_places.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_effect_part_places_detail;
namespace {
constexpr std::uint32_t instances=0x10000000u,definitions=0x10010000u,events=0x10020000u,parts=0x10030000u,effects=0x10040000u,
    pool=0x10050000u,records=0x10060000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0x159690u?3u:target==0x145560u?1u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x17dd50u)for(std::uint32_t k=0;k<13;++k)w(s.memory,s.observe,0x17dd51u,s.gp[1]+4u*k,4,std::uint32_t(rng()));
    if(target==0x145560u)for(std::uint32_t k=0;k<13;++k)w(s.memory,s.observe,0x145561u,r(s.memory,s.gp[4]+4u)+4u*k,4,std::uint32_t(rng()));
    if(target==0x159690u && rng()%9==0)w(s.memory,s.observe,0x159691u,events+0x38u*std::uint32_t(rng()%2)+0x20u,4,std::uint32_t(rng()%4));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=effects+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=records+0x3cu*std::uint32_t(rng()%8);
    if(policy&2u && rng()%13==0)s.gp[7]=parts+0x3cu*std::uint32_t(rng()%8);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*arguments+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x17dd50u && target!=0x145560u && target!=0x159690u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
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
    m.put(0x4f0414u,instances);m.put(0x4f7208u,pool);
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,definitions+0x40u*k);}
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=definitions+0x40u*k;at<definitions+0x40u*k+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definitions+0x40u*k+0x18u,events);}
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=events+0x38u*k;at<events+0x38u*k+0x38u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(events+0x38u*k+0x20u,pick(5)?pick(4):0u-pick(2));m.put(events+0x38u*k+0x24u,parts);}
    for(std::uint32_t at=parts;at<parts+0x3cu*8u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(parts+0x3cu*k+8u,pick(4),2);
    static constexpr float scales[]{0.f,-1.f,0.5f,2.f,1e-30f,3.f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto effect=effects+0x100u*k;
        for(std::uint32_t at=effect;at<effect+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(effect+4u,(std::uint32_t(rng())&0xffff0000u)|pick(2));m.put(effect+0x5cu,pick(2),2);
        m.put(effect+0x60u,bits_of(float(int(pick(9))-4)));m.put(effect+0x64u,pick(10)?bits_of(scales[pick(6)]):0x7fc00000u);
        for(std::uint32_t slot=0;slot<4;++slot)m.put(effect+0x7cu+4u*slot,pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
    }
    for(std::uint32_t at=pool;at<pool+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(pool+0x44u,records);
    for(std::uint32_t k=0;k<8;++k) {
        const auto record=records+0x3cu*k;
        for(std::uint32_t at=record;at<record+0x3cu;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+2u,pick(2)?pick(8):0xffffu,2);
        m.put(record+4u,k<7 && pick(2)?(std::uint32_t(rng())&0xffff0000u)|(k+1u+pick(7-k)):0xffffffffu);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0xc0u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x170000u+pick(0x100));m.put(stack+4u,effects+0x100u*pick(2));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{instances,definitions,events,parts,effects,pool,records};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_17cd10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_17cd10(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EFFECT_PART_PLACES");
    std::mt19937 rng(0x17cd10u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
