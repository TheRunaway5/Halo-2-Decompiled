// Native TU1.5 rigid body build (1DDEA0) against the unmodified translated
// body of the live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The children are hash-driven stubs of their complete entry ABIs: 1DE170
// (RET 3C) writes the shape's outputs (the node, the info block and its
// size, the flags, the value) and answers a shape or none; 1DEC80 (RET 8)
// and 1DF5B0 (RET 8) answer values; 1DF380 (RET 38) hashes its arguments
// and the stack transform and answers a body; 283450 (through the dispatch,
// the caller cleaning) answers AL; the shapes' destructors (through the
// dispatch, RET 4) hash the shape. The caller-saved registers, XMM, flags
// and (by policy) MXCSR are random, and by policy EBX, ESI, EDI or EBP
// (another authored frame) change. Fixtures author entries with their own
// matrix or an indexed one, entry flags, object types, shapes with one to
// three references, scaled builds or not, finite, infinite and NaN
// matrices, either string direction; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_rigid_body_build.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_rigid_body_build_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,headers=0x10010000u,datums=0x10020000u,models=0x10030000u,shapes_area=0x10040000u,
    entries=0x10050000u,matrices=0x10060000u,types=0x10070000u,shape_objects=0x10080000u,vtables=0x10090000u,records=0x100a0000u,
    fourths=0x100b0000u,frames=0x100c0000u,scratch=0x10600000u;
constexpr std::uint32_t destructors[]{0x2fa000u,0x2fa100u};
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0x1de170u?15u:target==0x1df380u?14u:target==0x283450u||target==destructors[0]||target==destructors[1]?1u:2u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x1df380u){const auto t=r(s.memory,s.gp[4]+4u*10u);for(std::uint32_t k=0;k<16;++k)h=mix(h,r(s.memory,t+4u*k));}
    if(target==destructors[0]||target==destructors[1])h=mix(h,r(s.memory,s.gp[1]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    const auto arg=[&](unsigned k){return r(s.memory,s.gp[4]+4u*k);};
    if(target==0x1de170u) {
        const auto info=arg(4);
        for(std::uint32_t k=0;k<16;++k)w(s.memory,s.observe,0x1de171u,info+4u*k,4,std::uint32_t(rng()));
        w(s.memory,s.observe,0x1de172u,info,1,rng()%11?std::uint32_t(rng()%4):0xffu);
        w(s.memory,s.observe,0x1de173u,arg(5),4,rng()%9?std::uint32_t(rng()%0x41):std::uint32_t(rng()%0x40)+0x40u);
        w(s.memory,s.observe,0x1de174u,arg(9),4,std::uint32_t(rng()));
        w(s.memory,s.observe,0x1de175u,arg(12),1,rng()%3?0u:1u+std::uint32_t(rng()%255));
        w(s.memory,s.observe,0x1de176u,arg(14),1,std::uint32_t(rng()%3));
        w(s.memory,s.observe,0x1de177u,arg(15),1,rng()%2?0u:1u+std::uint32_t(rng()%255));
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1de170u)s.gp[0]=rng()%8?shape_objects+0x20u*std::uint32_t(rng()%4):0u;
    if(target==0x283450u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%11==0)s.gp[3]=target==0x1dec80u?entries+0x90u*std::uint32_t(rng()%4):std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=frames+0x80u+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%11==0)s.gp[6]=shape_objects+0x20u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[7]=target==0x1de170u?types+std::uint32_t(rng()%4):target==0x1dec80u?std::uint32_t(rng()%8):std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=target==0x1de170u?0x40u:target==0x1df380u?0x3cu:target==0x283450u?4u:target==destructors[0]||target==destructors[1]?8u:12u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_1de170(WorldState &s){stub(s,0x1de170u);}
static void run_1dec80(WorldState &s){stub(s,0x1dec80u);}
static void run_1df380(WorldState &s){stub(s,0x1df380u);}
static void run_1df5b0(WorldState &s){stub(s,0x1df5b0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x283450u && target!=destructors[0] && target!=destructors[1])throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t component(std::mt19937 &rng) {
    switch(rng()%14) {
    case 0:return 0x7fc00000u|std::uint32_t(rng()%0x100);
    case 1:return 0x7f800000u|(rng()%2?0x80000000u:0u);
    default:{float f=float(int(rng()%2001)-1000)/37.0f;std::uint32_t v;std::memcpy(&v,&f,4);return v;}
    }
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t at=datums;at<datums+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(datums+12u*k+3u,pick(4)?pick(4):std::uint32_t(rng()%256),1);
    for(std::uint32_t at=objects;at<objects+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k)m.put(objects+0x20u*k+8u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t at=models;at<models+0x100u;at+=4)m.put(at,component(rng));
    for(std::uint32_t k=0;k<2;++k){m.put(models+0x80u*k+0x48u,shapes_area+0x40u*pick(2));m.put(models+0x80u*k+0x50u,matrices);}
    for(std::uint32_t at=shapes_area;at<shapes_area+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k){m.put(shapes_area+0x40u*k+0x38u,pick(2)?1u:pick(4));m.put(shapes_area+0x40u*k+0x3cu,entries);}
    for(std::uint32_t at=entries-0x100u;at<entries+0x300u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(entries+0x90u*k,pick(3)?pick(4):0xffffu,2);m.put(entries+0x90u*k+0x18u,std::uint32_t(rng()),2);}
    for(std::uint32_t at=matrices;at<matrices+0x100u;at+=4)m.put(at,component(rng));
    for(std::uint32_t at=types;at<types+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(types+k,pick(3)?pick(8):std::uint32_t(rng()%256),1);
    for(std::uint32_t at=shape_objects;at<shape_objects+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(shape_objects+0x20u*k,vtables+0x10u*pick(2));m.put(shape_objects+0x20u*k+6u,1u+pick(3),2);}
    for(std::uint32_t at=vtables;at<vtables+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(vtables,destructors[0]);m.put(vtables+0x10u,destructors[1]);
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=fourths;at<fourths+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto args=[&](std::uint32_t at) {
        m.put(at+8u,objects+0x20u*pick(2));m.put(at+0xcu,models+0x80u*pick(2));m.put(at+0x10u,pick(2)?0u:1u+pick(255));
        m.put(at+0x14u,std::uint32_t(rng()));m.put(at+0x18u,fourths+4u*pick(4));m.put(at+0x1cu,std::uint32_t(rng()));
        m.put(at+0x20u,std::uint32_t(rng()));m.put(at+0x24u,records+0x80u*pick(2));
    };
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    args(frames+0x80u);args(frames+0x100u);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));
    args(stack-4u);
    random_abi(rng,c,stack);
    c.abi.gp[0]=types;
    if(rng()%4==0)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{objects,headers,datums,models,shapes_area,matrices,shape_objects,records,fourths};
        m.drop(regions[pick(9)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1ddea0(v);},[](auto &v){original::run_1de170(v);},
        [](auto &v){original::run_1dec80(v);},[](auto &v){original::run_1df380(v);},[](auto &v){original::run_1df5b0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1ddea0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RIGID_BODY_BUILD");
    std::mt19937 rng(0x1ddea0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
