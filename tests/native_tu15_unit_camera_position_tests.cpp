// Native TU1.5 unit camera position (CD4A0) against the unmodified
// translated body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BBE00, E3310, 11FA50, E18D0, 30BD0, 16C620, BAD60 and BBF20 (through the
// dispatch, or direct in the unit's copy) are hash-driven stubs of their
// complete entry ABIs: each hashes what it reads (its pushed arguments),
// writes scratch and its outputs on a small grid with NaNs of varied
// payloads (BBE00 and BBF20 a position, 11FA50 the up, E18D0 the moved
// camera, 16C620 a distance and a hit, BAD60 a marker's position), and E3310
// and 16C620 answer AL, 30BD0 XMM0; the caller-saved registers, XMM, flags
// and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change
// or the stack is left unbalanced. Fixtures author seated units in vehicles
// and bipeds, controlling units, and bipeds crouched or not with every ray
// outcome; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_unit_camera_position.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_unit_camera_position_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10001000u,objects=0x10010000u,instances=0x10020000u,definitions=0x10021000u,
    seats=0x10030000u,outputs=0x10040000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t grid(std::mt19937_64 &rng){return rng()%13?bits(float(int(rng()%9)-4)/2.0f):0x7fc00000u|std::uint32_t(rng()%0x100);}
std::uint32_t arguments(std::uint32_t target){switch(target){case 0xe18d0u:case 0xbad60u:return 3u;case 0x16c620u:return 6u;default:return 0u;}}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto args=arguments(target);
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto write=[&](std::uint32_t at,unsigned words){for(unsigned k=0;k<words;++k)w(s.memory,s.observe,target+1u,at+4u*k,4,grid(rng));};
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(target==0xbbe00u||target==0xbbf20u)write(target==0xbbe00u?s.gp[6]:s.gp[1],3);
    if(target==0x11fa50u)write(s.gp[2],3);
    if(target==0xe18d0u && rng()%4)write(arg(1),3);
    if(target==0x16c620u && rng()%2){write(arg(6)+4u,1);write(arg(6)+8u,3);}
    if(target==0xbad60u)write(s.gp[3]+0x60u,3);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0xe3310u||target==0x16c620u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=rng()%2?datums:definitions+0x400u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=outputs+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x400u*std::uint32_t(rng()%8);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{grid(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[target==0x30bd0u?0u:rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// Each unit's copy calls its children directly or through the dispatch.
[[maybe_unused]] static void run_bbe00(WorldState &s){stub(s,0xbbe00u);}
[[maybe_unused]] static void run_e3310(WorldState &s){stub(s,0xe3310u);}
[[maybe_unused]] static void run_11fa50(WorldState &s){stub(s,0x11fa50u);}
[[maybe_unused]] static void run_e18d0(WorldState &s){stub(s,0xe18d0u);}
[[maybe_unused]] static void run_30bd0(WorldState &s){stub(s,0x30bd0u);}
[[maybe_unused]] static void run_16c620(WorldState &s){stub(s,0x16c620u);}
[[maybe_unused]] static void run_bad60(WorldState &s){stub(s,0xbad60u);}
[[maybe_unused]] static void run_bbf20(WorldState &s){stub(s,0xbbf20u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    static constexpr std::uint32_t known[]{0xbbe00u,0xe3310u,0x11fa50u,0xe18d0u,0x30bd0u,0x16c620u,0xbad60u,0xbbf20u};
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
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);
    m.put(0x46a0d0u,bits(1.0f));m.put(0x46a0e8u,bits(float(pick(3))/4.0f));m.put(0x4502c8u,grid(floats));m.put(0x46a194u,grid(floats));
    m.put(0x46a12cu,grid(floats));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    // Datums 0-3 units, 4-7 parents and controlled objects.
    for(std::uint32_t k=0;k<8;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));
        m.put(datums+12u*k+8u,objects+0x400u*(chance(5)?pick(8):k));}
    for(std::uint32_t at=objects;at<objects+0x2000u;at+=4)m.put(at,grid(floats));
    const auto handle=[&](std::uint32_t lo,std::uint32_t span){return (std::uint32_t(rng())&0xffff0000u)|(lo+pick(span));};
    for(std::uint32_t k=0;k<8;++k) {
        const auto o=objects+0x400u*k;
        m.put(o,handle(0,4));
        m.put(o+0x14u,chance(3)?handle(4,4):0xffffffffu);
        m.put(o+0xaau,chance(2)?0u:1u+pick(3),1);
        m.put(o+0x10au,chance(5)?4u:std::uint32_t(rng())&0xfbu,1);
        m.put(o+0x1fcu,chance(4)?0xffffu:pick(3),2);
        m.put(o+0x24cu,chance(3)?0xffffffffu:handle(4,4));
    }
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x400u*pick(4));
    for(std::uint32_t at=definitions;at<definitions+0x1000u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t k=0;k<4;++k)m.put(definitions+0x400u*k+0x1ccu,seats+0x400u*pick(2));
    for(std::uint32_t at=seats;at<seats+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<6;++k){m.put(seats+0xb0u*k+0x60u,chance(3)?0u:std::uint32_t(rng()));m.put(seats+0x400u+0xb0u*k+0x60u,chance(3)?0u:std::uint32_t(rng()));}
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x180u;at<stack+0x40u;at+=4)m.put(at,grid(floats));
    m.put(stack,0xc8000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=handle(0,4);c.abi.gp[0]=outputs+0x10u*pick(4);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=grid(floats);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,instances,definitions,seats,outputs};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_cd4a0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_cd4a0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_UNIT_CAMERA_POSITION");
    std::mt19937 rng(0xcd4a0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
