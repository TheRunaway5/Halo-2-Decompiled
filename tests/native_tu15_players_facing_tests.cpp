// Native TU1.5 players facing (164750) against the unmodified translated body
// of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BBE00, 1A8F10 and 11F7E0 (through the unit's dispatch) are hash-driven
// stubs of their complete entry ABIs: BBE00 writes a position at ESI, the
// iterator (ESI) answers AL and a player datum for the first few calls,
// 11F7E0 hashes both vectors and answers an angle in XMM0; the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX
// or EDI (other object headers) change. Fixtures author players near and far,
// aims long and short, finite, infinite and NaN components; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_players_facing.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_players_facing_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,datums_area=0x10010000u,headers=0x10020000u,object_datums=0x10021000u,units=0x10030000u,
    scratch=0x10600000u;
std::uint32_t component(std::mt19937_64 &rng) {
    switch(rng()%14) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    case 2:return 0u;
    default:{float f=float(int(rng()%201)-100)/7.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0x11f7e0u)for(std::uint32_t k=0;k<3;++k){h=mix(h,r(s.memory,s.gp[1]+4u*k));h=mix(h,r(s.memory,s.gp[2]+4u*k));}
    if(target==0x1a8f10u)h=mix(h,r(s.memory,s.gp[6]+8u));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto grid=[&]{float f=float(rng()%4);std::uint32_t v;std::memcpy(&v,&f,4);return rng()%5?v:component(rng);};
    if(target==0xbbe00u)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,0xbbe01u,s.gp[6]+4u*k,4,grid());
    std::uint32_t more=0u;
    if(target==0x1a8f10u) {
        const auto index=r(s.memory,s.gp[6]+8u);
        more=(index==0xffffffffu||index<3u)&&rng()%5?1u+std::uint32_t(rng()%255):0u;
        w(s.memory,s.observe,0x1a8f11u,s.gp[6]+8u,4,index+1u);
        w(s.memory,s.observe,0x1a8f12u,s.gp[6],4,datums_area+0x40u*std::uint32_t(rng()%4));
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1a8f10u)s.gp[0]=(s.gp[0]&0xffffff00u)|more;
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=headers+0x80u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    if(target==0x11f7e0u){float f=float(rng()%400)/100.0f;std::uint32_t v;std::memcpy(&v,&f,4);lanes={rng()%9?v:0x7fc00002u,std::uint32_t(rng()),0u,0u};std::memcpy(&s.xmm[0],lanes.data(),16);}
    s.gp[4]+=4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbbe00u && target!=0x1a8f10u && target!=0x11f7e0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,headers);
    float range=pick(3)?float(pick(19)):float(pick(500))/3.0f,shortest=float(pick(10))/100.0f,facing=float(pick(300))/100.0f;
    std::uint32_t v;
    std::memcpy(&v,&range,4);m.put(0x4500b8u,v);std::memcpy(&v,&shortest,4);m.put(0x46a19cu,v);std::memcpy(&v,&facing,4);m.put(0x46a1ecu,v);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=datums_area;at<datums_area+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(datums_area+0x40u*k+0x2cu,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t at=headers;at<headers+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);m.put(headers+0x80u+0x44u,object_datums+0x40u);
    for(std::uint32_t at=object_datums;at<object_datums+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(object_datums+12u*k+8u,units+0x100u*pick(4));m.put(object_datums+0x40u+12u*k+8u,units+0x100u*pick(4)+0x10u);}
    for(std::uint32_t at=units;at<units+0x400u;at+=4)m.put(at,component(floats));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x160000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,datums_area,headers,object_datums,units};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_164750(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_164750(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYERS_FACING");
    std::mt19937 rng(0x164750u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
