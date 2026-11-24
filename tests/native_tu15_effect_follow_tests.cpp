// Native TU1.5 effect follow (17C370) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BCDF0, 17A380, BCFB0, 17C330, BCB70, BCD80, 17A0F0, 17A1A0, 17A4A0 and
// 3DDC0 (through the dispatch) are hash-driven stubs of their complete entry
// ABIs: each hashes what it reads (its pushed arguments), writes scratch and
// its outputs (BCB70 a marker, BCD80 a node offset), and BCDF0 answers an
// object or 0, BCFB0 a root, 17C330, BCB70 and 17A4A0 AL, 3DDC0 an entry or
// NONE; the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author effects with and without owners, every flag
// and marker shape, located and unlocated roots, tags with each bit and
// lights active or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_effect_follow.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_effect_follow_detail;
namespace {
constexpr std::uint32_t effects=0x10000000u,effect_data=0x10001000u,instances=0x10002000u,definitions=0x10003000u,headers=0x10004000u,
    datums=0x10005000u,objects=0x10006000u,defaults=0x10007000u,lights=0x10008000u,light_data=0x10009000u,scratch=0x10600000u;
std::uint32_t arguments(std::uint32_t target) {
    switch(target){case 0xbcdf0u:case 0x17c330u:case 0x17a0f0u:case 0x17a1a0u:case 0x3ddc0u:return 1u;case 0xbcb70u:return 3u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xbcb70u && rng()%2)w(s.memory,s.observe,target+1u,r(s.memory,s.gp[4]+12u),4,std::uint32_t(rng()));
    if(target==0xbcd80u)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,s.gp[6]+4u*k,4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0xbcdf0u)s.gp[0]=rng()%4?objects+0x100u*std::uint32_t(rng()%4):0u;
    if(target==0xbcfb0u)s.gp[0]=(s.gp[0]&0xffff0000u)|std::uint32_t(rng()%4);
    if(target==0x17c330u||target==0xbcb70u||target==0x17a4a0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(target==0x3ddc0u)s.gp[0]=rng()%4?(s.gp[0]&0xffff0000u)|std::uint32_t(rng()%4):0xffffffffu;
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffffff00u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=effect_data+0x190u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
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
// Each unit's copy calls some children directly (physics live support:
// 17C330, BCB70, BCD80, 17A1A0, 3DDC0; death cycle: 17C330, 17A380, 17A0F0,
// 17A1A0, 17A4A0) and the rest through the dispatch.
[[maybe_unused]] static void run_17c330(WorldState &s){stub(s,0x17c330u);}
[[maybe_unused]] static void run_bcb70(WorldState &s){stub(s,0xbcb70u);}
[[maybe_unused]] static void run_bcd80(WorldState &s){stub(s,0xbcd80u);}
[[maybe_unused]] static void run_17a1a0(WorldState &s){stub(s,0x17a1a0u);}
[[maybe_unused]] static void run_3ddc0(WorldState &s){stub(s,0x3ddc0u);}
[[maybe_unused]] static void run_17a380(WorldState &s){stub(s,0x17a380u);}
[[maybe_unused]] static void run_17a0f0(WorldState &s){stub(s,0x17a0f0u);}
[[maybe_unused]] static void run_17a4a0(WorldState &s){stub(s,0x17a4a0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    static constexpr std::uint32_t known[]{0xbcdf0u,0x17a380u,0xbcfb0u,0x17c330u,0xbcb70u,0xbcd80u,0x17a0f0u,0x17a1a0u,0x17a4a0u,0x3ddc0u};
    bool ok=false;for(auto k:known)ok|=k==target;
    if(!ok)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4f720cu,effects);m.put(0x4f0414u,instances);m.put(0x4ecbd0u,headers);m.put(0x474c70u,defaults);m.put(0x515f14u,lights);
    m.put(0x474c24u,std::uint32_t(rng()));
    for(const auto base:{effects,headers,lights}){for(std::uint32_t at=base;at<base+0x50u;at+=4)m.put(at,std::uint32_t(rng()));}
    m.put(effects+0x44u,effect_data);m.put(headers+0x44u,datums);m.put(lights+0x44u,light_data);
    for(std::uint32_t at=effect_data;at<effect_data+0x190u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto e=effect_data+0x190u*k;
        m.put(e+2u,std::uint32_t(rng())&0xfdf9u|(chance(2)?2u:0u)|(chance(3)?0x200u:0u)|(chance(2)?4u:0u),2);
        m.put(e+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(e+0x10u,chance(3)?0u:std::uint32_t(rng()));
        m.put(e+0x18u,chance(2)?0u:pick(8),2);
        m.put(e+0x48u,chance(5)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(4));
    }
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x40u*pick(4));
    for(std::uint32_t k=0;k<4;++k)m.put(definitions+0x40u*k,std::uint32_t(rng())&~0x19u|(chance(3)?1u:0u)|(chance(3)?8u:0u)|(chance(3)?0x10u:0u));
    for(std::uint32_t at=datums;at<datums+0x30u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(datums+12u*k+8u,objects+0x100u*pick(4));
    for(std::uint32_t at=objects;at<objects+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(objects+0x100u*k+4u,std::uint32_t(rng())&~0x100u|(chance(2)?0x100u:0u));
    for(std::uint32_t at=defaults;at<defaults+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=light_data;at<light_data+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(light_data+0x100u*k+2u,chance(2)?0u:1u+pick(255),1);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x17d000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{effects,effect_data,instances,definitions,headers,datums,objects,light_data};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_17c370(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_17c370(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_EFFECT_FOLLOW");
    std::mt19937 rng(0x17c370u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
