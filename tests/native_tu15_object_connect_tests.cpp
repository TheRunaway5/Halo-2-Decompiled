// Native TU1.5 object connect (BA630) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 11E600, BA910, 1D2F10, C0F60, B8D90, B92C0, B1870, BA570, B9330, 1CC290
// and (through the dispatch) 1496F0 and 282C10 are hash-driven stubs of
// their complete entry ABIs and arguments: 11E600 fills the location it is
// given (a cluster or NONE), BA910 the summary, B8D90 and B1870 answer AL,
// and some change the object's flags or its header's cluster; each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author objects placed and unplaced, given locations
// or none, every-cluster objects (structures of 0 to 70 clusters, the
// direction flag set too), both partitions and the release bit; one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_object_connect.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_connect_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,locations=0x10030000u,structure=0x10040000u,
    game=0x10050000u,scratch=0x10600000u;
std::uint32_t arguments_of(std::uint32_t target) {
    switch(target){case 0x1d2f10u:return 9u;case 0xc0f60u:return 5u;case 0xb8d90u:case 0xba570u:return 1u;case 0x1cc290u:return 2u;default:return 0u;}
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const auto arguments=arguments_of(target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x11e600u){w(s.memory,s.observe,0x11e601u,s.gp[6],4,std::uint32_t(rng()));
        w(s.memory,s.observe,0x11e602u,s.gp[6]+4u,4,(std::uint32_t(rng())&0xffff0000u)|(rng()%3?std::uint32_t(rng()%8):0xffffu));}
    if(target==0xba910u)for(std::uint32_t k=0;k<5;++k)w(s.memory,s.observe,0xba911u,s.gp[7]+4u*k,4,std::uint32_t(rng()));
    const auto object=objects+0x100u*std::uint32_t(rng()%2);
    if(rng()%8==0)w(s.memory,s.observe,target+1u,object+4u,4,std::uint32_t(rng()));
    if(rng()%8==0)w(s.memory,s.observe,target+2u,datums+12u*std::uint32_t(rng()%2)+4u,2,rng()%2?0xffffu:std::uint32_t(rng()%8));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    if(target==0xb8d90u||target==0xb1870u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=datums+12u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=objects+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=rng()%2?locations+8u*std::uint32_t(rng()%4):(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=locations+8u*std::uint32_t(rng()%4);
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
static void run_11e600(WorldState &s){stub(s,0x11e600u);}
static void run_ba910(WorldState &s){stub(s,0xba910u);}
static void run_1d2f10(WorldState &s){stub(s,0x1d2f10u);}
static void run_c0f60(WorldState &s){stub(s,0xc0f60u);}
static void run_b8d90(WorldState &s){stub(s,0xb8d90u);}
static void run_b92c0(WorldState &s){stub(s,0xb92c0u);}
static void run_b1870(WorldState &s){stub(s,0xb1870u);}
static void run_ba570(WorldState &s){stub(s,0xba570u);}
static void run_b9330(WorldState &s){stub(s,0xb9330u);}
static void run_1cc290(WorldState &s){stub(s,0x1cc290u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1496f0u && target!=0x282c10u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4ecc18u,structure);m.put(0x4f3218u,game);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,(std::uint32_t(rng())&0xffff0000u)|(pick(2)?0xffffu:pick(8)));
        m.put(datums+12u*k+8u,objects+0x100u*k);}
    for(std::uint32_t k=0;k<2;++k) {
        const auto object=objects+0x100u*k;
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+4u,(std::uint32_t(rng())&~0x220200u)|(pick(3)?0u:0x200000u)|(pick(2)?0x200u:0u)|(pick(2)?0x20000u:0u));
    }
    for(std::uint32_t k=0;k<4;++k){m.put(locations+8u*k,std::uint32_t(rng()));m.put(locations+8u*k+4u,(std::uint32_t(rng())&0xffff0000u)|(pick(3)?pick(8):0xffffu));}
    for(std::uint32_t at=structure;at<structure+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(structure+0x9cu,pick(71));
    for(std::uint32_t at=game;at<game+0x11c0u;at+=0x100)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x140u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xb0000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(2));
    m.put(stack+8u,pick(2)?0u:locations+8u*pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,locations,structure};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_ba630(v);},
        [](auto &v,std::uint32_t target) {
            switch(target) {
            case 0x11e600u:original::run_11e600(v);break;case 0xba910u:original::run_ba910(v);break;
            case 0x1d2f10u:original::run_1d2f10(v);break;case 0xc0f60u:original::run_c0f60(v);break;
            case 0xb8d90u:original::run_b8d90(v);break;case 0xb92c0u:original::run_b92c0(v);break;
            case 0xb1870u:original::run_b1870(v);break;case 0xba570u:original::run_ba570(v);break;
            case 0xb9330u:original::run_b9330(v);break;case 0x1cc290u:original::run_1cc290(v);break;
            default:throw std::runtime_error("Unbound child");
            }
        },
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_ba630(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_CONNECT");
    std::mt19937 rng(0xba630u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
