// Native TU1.5 object lights (1181F0) against the unmodified translated body
// of the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 212C70, the owners' virtual method and 1D8ED0 (through the dispatch) and
// 1D9080, 1D9100 and 1D9190 are hash-driven stubs of their complete entry
// ABIs and arguments: 212C70 fills the model state it is given and answers
// AL, the virtual method answers a type (often 6); each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and
// by policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author objects with and without a light block, blocks of
// negative, zero and several lights, entries with and without markers, and
// markers on nodes or none, active or not; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/renderer/native_tu15_object_lights.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_object_lights_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,objects=0x10020000u,blocks=0x10030000u,entries=0x10040000u,
    lights=0x10050000u,owners=0x10060000u,models=0x10070000u,markers=0x10080000u,scratch=0x10600000u;
constexpr std::uint32_t type_method=0x1e0000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const std::uint32_t arguments=target==0x1d9080u||target==0x1d9100u||target==0x1d8ed0u?1u:0u;
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x212c70u){w(s.memory,s.observe,0x212c71u,s.gp[6]+0x48u,4,models);w(s.memory,s.observe,0x212c72u,s.gp[6]+0x50u,4,std::uint32_t(rng()));}
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());
    if(target==0x212c70u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%5?1u:0u);
    if(target==type_method)s.gp[0]=rng()%3?6u:std::uint32_t(rng()%8);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=0x60u*std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[5]=objects+0x200u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=blocks+0xa0u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng()%4);
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
static void run_1d9080(WorldState &s){stub(s,0x1d9080u);}
static void run_1d9100(WorldState &s){stub(s,0x1d9100u);}
static void run_1d9190(WorldState &s){stub(s,0x1d9190u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x212c70u && target!=0x1d8ed0u && target!=type_method)throw std::runtime_error("Unbound sandbox dispatch target");
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
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x52be5cu,blocks+0x400u);m.put(0x474d04u,std::uint32_t(rng()));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<2;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x200u*k);}
    for(std::uint32_t k=0;k<2;++k){const auto object=objects+0x200u*k;for(std::uint32_t at=object;at<object+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object+0xb4u,pick(5)?(std::uint32_t(rng())&0xffff0000u)|pick(2):0xffffffffu);}
    for(std::uint32_t at=blocks+0x400u;at<blocks+0x450u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(blocks+0x444u,blocks);
    for(std::uint32_t k=0;k<2;++k){const auto block=blocks+0xa0u*k;for(std::uint32_t at=block;at<block+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(block+0x70u,entries+0x200u*k);m.put(block+0x74u,pick(6)?pick(4):0u-pick(2));}
    for(std::uint32_t k=0;k<8;++k) {
        const auto entry=entries+0x60u*k;
        for(std::uint32_t at=entry;at<entry+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(entry+0x40u,lights+0x40u*(k%4));m.put(entry+0x48u,markers+0x400u+pick(4));m.put(entry+0x4cu,pick(4)?pick(3):0u-1u);
    }
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=lights+0x40u*k;at<lights+0x40u*k+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(lights+0x40u*k+0x3cu,owners+0x10u*k);m.put(owners+0x10u*k,owners+0x100u);}
    for(std::uint32_t at=owners+0x100u;at<owners+0x120u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(owners+0x118u,type_method);
    for(std::uint32_t at=models;at<models+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(models+0x3cu,markers);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=markers+0x90u*k;at<markers+0x90u*k+0x90u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(markers+0x90u*k,pick(3)?pick(8):0xffffu,2);}
    for(std::uint32_t at=markers+0x400u;at<markers+0x408u;at+=4)m.put(at,std::uint32_t(rng())&0x03030303u);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));m.put(stack+4u,pick(2)?0u:1u+pick(255));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,objects,blocks,entries,lights,owners,models,markers};
        m.drop(regions[pick(9)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1181f0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_1d9080(v);},[](auto &v){original::run_1d9100(v);},[](auto &v){original::run_1d9190(v);});
}
void reference(WorldState &s){original::run_1181f0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_LIGHTS");
    std::mt19937 rng(0x1181f0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
