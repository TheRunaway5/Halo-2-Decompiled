// Native TU1.5 director aim (245060) against the unmodified translated body
// of the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 11FA50, CD4A0, BC200, BCDF0, BAD60, BBE00, 144BF0 and 175380 (through the
// unit's dispatch) are hash-driven stubs of their complete entry ABIs: each
// hashes what it reads and writes its outputs (the up vector, a position,
// the marker's frame, the node matrix) on a small grid, now and then with
// NaNs of varied payloads, and BCDF0 answers a weapon or 0, BAD60 AX (RET
// 4/C as the original pops); the caller-saved registers, XMM (XMM6 and XMM7
// among them), flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change. Fixtures author units with and without a parent
// and seats marked or not; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_director_aim.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_director_aim_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,object_datums=0x10001000u,units=0x10010000u,weapons=0x10020000u,instances=0x10030000u,
    definitions=0x10031000u,records=0x10040000u,vectors=0x10050000u,scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t grid(std::mt19937_64 &rng){return rng()%11?bits(float(int(rng()%9)-4)/2.0f):0x7fc00000u|std::uint32_t(rng()%0x100);}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0xbc200u||target==0xbcdf0u?1u:target==0xbad60u?3u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto up_in=s.gp[2];
    const auto write=[&](std::uint32_t at,unsigned words){for(unsigned k=0;k<words;++k)w(s.memory,s.observe,target+1u,at+4u*k,4,grid(rng));};
    if(target==0x11fa50u){write(s.gp[2],3);if(rng()%3==0)write(s.gp[1],3);}
    if(target==0xcd4a0u)write(s.gp[0],3);
    if(target==0xbbe00u)write(s.gp[6],3);
    if(target==0x144bf0u)write(s.gp[2],13);
    if(target==0xbad60u)write(s.gp[3]+0x3cu,0x1bu);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0xbcdf0u)s.gp[0]=rng()%5?weapons+0x100u*std::uint32_t(rng()%2):0u;
    if(target==0xbad60u)s.gp[0]=(s.gp[0]&0xffff0000u)|(rng()%4?1u+std::uint32_t(rng()%0xffff):0u);
    if(target==0x11fa50u)s.gp[2]=rng()%9?up_in:vectors+0x40u;
    if(policy&2u && rng()%13==0)s.gp[3]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=records+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=vectors+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=vectors+0x10u*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{grid(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=4u+4u*args;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    static constexpr std::uint32_t known[]{0x11fa50u,0xcd4a0u,0xbc200u,0xbcdf0u,0xbad60u,0xbbe00u,0x144bf0u,0x175380u};
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
    std::mt19937_64 floats(rng());
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);m.put(0x474d04u,vectors+0x80u);m.put(0x55d6e4u,grid(floats));
    for(std::uint32_t at=vectors;at<vectors+0x100u;at+=4)m.put(at,grid(floats));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x30u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(object_datums+12u*k+8u,units+0x400u*pick(4));
    for(std::uint32_t at=units;at<units+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){const auto u=units+0x400u*k;m.put(u+0x14u,pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);m.put(u+0x1fcu,pick(3),2);}
    for(std::uint32_t at=weapons;at<weapons+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(weapons+0x100u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t at=instances;at<instances+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(instances+16u*k+8u,definitions+0x200u*pick(2));
    for(std::uint32_t at=definitions;at<definitions+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(definitions+0x200u*k+0x1ccu,definitions+0x800u);
    for(std::uint32_t at=definitions+0x800u;at<definitions+0x800u+0xb0u*3u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,grid(floats));
    m.put(stack,0x240000u+pick(0x100));m.put(stack+4u,records+0x100u*pick(2));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(6)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu;c.abi.gp[0]=vectors+0x10u*pick(4);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=grid(floats);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,object_datums,units,weapons,instances,definitions,records};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_245060(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_245060(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_DIRECTOR_AIM");
    std::mt19937 rng(0x245060u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
