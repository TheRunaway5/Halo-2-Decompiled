// Native TU1.5 seat sum (1059C0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 10B0F0 is a hash-driven stub of its complete entry ABI (EAX the handle,
// EBX zero): it answers AL zero or not, writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// objects whose definitions list entries (indexes within and past their
// counts, negative too), vehicles with every kind of seat (empty, the object
// itself, other definitions, the same one) and the skip byte set or not;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_seat_sum.hpp"
using namespace frame_misc_harness;
namespace seats=halo2::native_seat_sum_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,tags=0x10180000u,
    definitions=0x10200000u,lists=0x10300000u,scratch=0x10600000u;
constexpr std::uint32_t object_count=6;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    // Mostly answering, so the definition words (and the last seat's own epilogue) come up often.
    s.gp[0]=rng()%4?std::uint32_t(rng())|1u:std::uint32_t(rng())&~0xffu;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%9==0)s.gp[3]=std::uint32_t(rng()%64);
    if(policy&2u && rng()%13==0)s.gp[5]=headers;
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=objects+0x800u*std::uint32_t(rng()%object_count);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_10b0f0(WorldState &s){stub(s,0x10b0f0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    // Two definitions (tag indexes 1 and 2), shared by the objects.
    for(std::uint32_t d=1;d<=2;++d) {
        const auto definition=definitions+0x1000u*d;
        m.put(tags+16u*d+8u,definition);
        for(std::uint32_t at=definition+0x2b0u;at<definition+0x2d0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definition+0x2c0u,pick(6)==0?0xffffffffu:1u+pick(5));
        const auto list=lists+0x1000u*d+4u*pick(4);
        m.put(definition+0x2c4u,list);
        for(std::uint32_t at=list;at<list+0x5cu*5u;at+=4)m.put(at,std::uint32_t(rng()));
    }
    std::array<std::uint32_t,object_count> handles{};
    for(std::uint32_t k=0;k<object_count;++k)handles[k]=(std::uint32_t(rng()&0x7fffu)<<16)|k;
    for(std::uint32_t k=0;k<object_count;++k) {
        const auto object=objects+0x800u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object;at<object+0x300u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(object,(std::uint32_t(rng())<<16)|(pick(4)?1u:2u));   // mostly one definition
        m.put(object+0x154u,pick(4)==0?0xffffffffu:handles[pick(object_count)]);
        for(std::uint32_t seat=0;seat<4;++seat)
            m.put(object+0x218u+4u*seat,pick(4)==0?0xffffffffu:handles[pick(object_count)]);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x100000u+pick(0x100));
    m.put(stack+4u,handles[pick(object_count)]);
    m.put(stack+8u,(std::uint32_t(rng())&~0xffu)|(pick(4)==0?1u+pick(255):0u));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(8)==0?0x80000000u+pick(4):pick(8)==0?5u:pick(3);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x800u*2u,tags,definitions+0x1000u,lists+0x2000u};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s){return seats::bridge(s,[](auto &v){original::native_frame_misc_reference_1059c0(v);},[](auto &v){original::run_10b0f0(v);});}
void reference(WorldState &s){original::run_1059c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_SUM");
    std::mt19937 rng(0x1059c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,seats::coverage().load(),unsigned(seats::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
