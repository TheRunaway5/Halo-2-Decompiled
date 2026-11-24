// Native TU1.5 object type events (10E340) against the unmodified translated body, extracted
// by tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The callbacks (the unit's dispatch route, at the address a type names)
// are genuine stand-ins whose effects are a hash of their complete entry
// ABI, their four arguments and the return address: they pop the
// arguments, return EAX/ECX/EDX, flags and XMM, write scratch, and
// occasionally clobber callee-saved registers or leave the stack
// unbalanced. Fixtures author objects of every type (negative types too),
// type definitions whose chains are empty or hold several types with and
// without callbacks; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_type_events.hpp"
using namespace frame_misc_harness;
namespace events=halo2::native_object_type_events_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,definitions=0x10a00000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=0;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%29==0) {
        static constexpr unsigned saved[]{3,5,6,7};
        const auto which=saved[rng()%4];
        s.gp[which]=which==6?definitions+0x84u+4u*std::uint32_t(rng()%4):std::uint32_t(rng());
    }
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target,4);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<8;++k) {
        const auto object=objects+0x1000u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
        const std::uint32_t type=pick(6)==0?0x100u-1u-pick(2):pick(8);
        m.put(object+0xa8u,(std::uint32_t(rng())&0xff00ffffu)|(type<<16));
    }
    for(std::uint32_t k=0;k<8;++k) {
        const auto definition=definitions+0x100u*k;
        for(std::uint32_t at=definition;at<definition+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x68u,pick(4)==0?0u:0x300000u+0x10u*k);
        const auto length=shape==5?0u:pick(5);
        for(std::uint32_t i=0;i<length;++i)m.put(definition+0x84u+4u*i,definitions+0x100u*pick(8));
        m.put(definition+0x84u+4u*length,0u);
    }
    for(std::int32_t type=-2;type<8;++type)m.put(0x474b90u+4u*std::uint32_t(type),definitions+0x100u*pick(8));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[7]=(std::uint32_t(rng())<<16)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects,definitions,0x474000u};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return events::bridge(s,[](auto &v){original::native_frame_misc_reference_10e340(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_10e340(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_OBJECT_TYPE_EVENTS");
    std::mt19937 rng(0x10e340u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,events::coverage().load(),unsigned(events::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
