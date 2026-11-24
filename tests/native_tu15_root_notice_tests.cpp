// Native TU1.5 root notice (1FA110) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 184EC0 is reached through the unit's dispatch, a hash-driven stub of its
// complete entry ABI and six arguments: it answers AL, pops them, writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy the stack is left unbalanced. Fixtures author
// records with and without an object, root lists empty, holding the object
// or not; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_root_notice.hpp"
using namespace frame_misc_harness;
namespace notice=halo2::native_root_notice_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,roots=0x10200000u,records=0x10300000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=6;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+24u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x184ec0u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x51daf0u,roots);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    std::array<std::uint32_t,4> handles{};
    for(std::uint32_t k=0;k<4;++k) {
        handles[k]=(std::uint32_t(rng())<<16)|k;
        const auto object=objects+0x400u*k+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object+0xb0u;at<object+0xc0u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    for(std::uint32_t at=roots+0x1d00u;at<roots+0x1d84u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto count=pick(5)==0?0u:pick(6)==0?0xffffffffu:1u+pick(6);
    m.put(roots+0x1d80u,count);
    for(std::uint32_t k=0;k<8;++k)if(pick(3)==0)m.put(roots+0x1d00u+4u*k,handles[pick(4)]);
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto record=records+4u*pick(8);
    m.put(record+0x10u,pick(5)==0?0xffffffffu:handles[pick(4)]);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1f0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=record;c.abi.gp[1]=records+0x100u+4u*pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x400u,roots+0x1000u,records};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return notice::bridge(s,[](auto &v){original::native_frame_misc_reference_1fa110(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1fa110(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ROOT_NOTICE");
    std::mt19937 rng(0x1fa110u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,notice::coverage().load(),unsigned(notice::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
