// Native TU1.5 platform carry (1F8710) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1FA110, 1FA1A0 and 1F8B30 are hash-driven stubs of their complete entry
// ABIs: each hashes what it reads (its pushed arguments and the vectors and
// rotation they point at), now and then writes its outputs (the attached
// position, offset and rotation; the surface offset and rotation; the
// displacement) on a small grid with NaNs of varied payloads, and 1FA110 and
// 1FA1A0 answer AL; the caller-saved registers, XMM, flags and (by policy)
// MXCSR are random, and by policy EBX, EBP, ESI or EDI change or the stack
// is left unbalanced. Fixtures author grounds with every tick count, moving
// and still surfaces, and bodies, characters and the identity rotation on
// the same grid; the direction flag is set now and then; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_platform_carry.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_platform_carry_detail;
namespace {
constexpr std::uint32_t surfaces=0x10000000u,surface_data=0x10001000u,objects=0x10002000u,identity=0x10003000u,zero=0x10003100u,
    grounds=0x10010000u,bodies=0x10020000u,characters=0x10030000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t grid(std::mt19937_64 &rng){return rng()%13?bits(float(int(rng()%9)-4)/2.0f):0x7fc00000u|std::uint32_t(rng()%0x100);}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x1f8b30u?2u:3u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto write=[&](std::uint32_t at,unsigned words){for(unsigned k=0;k<words;++k)w(s.memory,s.observe,target+1u,at+4u*k,4,grid(rng));};
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(target==0x1fa110u){if(rng()%2){write(arg(1),3);write(arg(2),3);}if(rng()%3==0)write(arg(3),9);}
    if(target==0x1fa1a0u){if(rng()%2)write(arg(2),3);if(rng()%3==0)write(arg(3),9);}
    if(target==0x1f8b30u && rng()%2)write(arg(2),3);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1fa110u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3?0u:1u+std::uint32_t(rng()%255));
    if(target==0x1fa1a0u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=surface_data+0xa0u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=bodies+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{grid(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+4u*args+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1fa110(WorldState &s){stub(s,0x1fa110u);}
static void run_1fa1a0(WorldState &s){stub(s,0x1fa1a0u);}
static void run_1f8b30(WorldState &s){stub(s,0x1f8b30u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x52be5cu,surfaces);m.put(0x474d30u,identity);m.put(0x474d04u,zero);
    for(std::uint32_t at=surfaces;at<surfaces+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(surfaces+0x44u,surface_data);
    for(std::uint32_t at=surface_data;at<surface_data+0xa0u*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(surface_data+0xa0u*k+0x70u,objects+0x80u*pick(4));
    for(std::uint32_t at=objects;at<objects+0x200u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t k=0;k<4;++k)m.put(objects+0x80u*k+0x44u,pick(2)?0u:1u+pick(255),1);
    for(std::uint32_t at=identity;at<identity+0x28u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t at=zero;at<zero+0x10u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t at=grounds;at<grounds+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t ticks[]{0u,1u,2u,0xfdu,0xfeu,0xffu};
    m.put(grounds+0x62u,ticks[pick(6)],1);
    for(std::uint32_t at=bodies;at<bodies+0x200u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t k=0;k<2;++k)m.put(bodies+0x100u*k,std::uint32_t(rng()));
    for(std::uint32_t at=characters;at<characters+0x140u;at+=4)m.put(at,grid(floats));
    m.put(characters+0x18u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,grid(floats));
    m.put(stack,0x1f0000u+pick(0x100));m.put(stack+4u,grounds);m.put(stack+8u,bodies+0x100u*pick(2));m.put(stack+12u,characters);
    random_abi(rng,c,stack);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=grid(floats);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{surfaces,surface_data,objects,identity,grounds,bodies,characters};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1f8710(v);},
        [](auto &v,std::uint32_t target){
            switch(target){case 0x1fa110u:original::run_1fa110(v);break;case 0x1fa1a0u:original::run_1fa1a0(v);break;
            case 0x1f8b30u:original::run_1f8b30(v);break;default:throw std::logic_error("Unbound platform carry child");}});
}
void reference(WorldState &s){original::run_1f8710(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLATFORM_CARRY");
    std::mt19937 rng(0x1f8710u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
