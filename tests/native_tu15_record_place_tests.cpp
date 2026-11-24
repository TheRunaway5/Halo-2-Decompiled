// Native TU1.5 record placement (1DC680) against the unmodified translated
// body of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// E32C0 and 145560 are hash-driven stubs of their complete entry ABIs and
// arguments: E32C0 answers AL and writes an offset (NaN too) into its slot,
// 145560 writes the placement; both write scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX
// changes, the direction flag flips or the stack is left unbalanced.
// Fixtures author records flagged bit 9 or not, objects of every type,
// markers negative, within and past the count, nodes and 0xFF, and both
// copy directions; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/objects/native_tu15_record_place.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_record_place_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,frames=0x10010000u,placements=0x10020000u,headers=0x10030000u,datums=0x10040000u,
    objects=0x10050000u,instances=0x10060000u,tags=0x10070000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    static constexpr std::uint32_t floats[]{0x3f800000u,0xbf000000u,0x41200000u,0u,0x7fc00000u};
    const auto slot=r(s.memory,s.gp[4]+4u);
    if(target==0xe32c0u)w(s.memory,s.observe,target,slot,4,floats[rng()%5]);
    else for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target,slot+4u*std::uint32_t(rng()%13),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?1u:0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=placements+0x80u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%17==0)s.other_flags^=0x400u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=8u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_e32c0(WorldState &s){stub(s,0xe32c0u);}
static void run_145560(WorldState &s){stub(s,0x145560u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    const bool backward=shape==5 || shape==11;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,instances);
    for(std::uint32_t at=records;at<records+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t markers[]{0u,1u,2u,0xffu,0x80u,5u};
    for(std::uint32_t k=0;k<4;++k) {
        const auto rc=records+0x100u*k;
        m.put(rc+4u,(std::uint32_t(rng())&~(1u<<9))|(pick(3)==0?1u<<9:0u));m.put(rc+8u,(std::uint32_t(rng())&0xffff0000u)|pick(8));
        m.put(rc+0x18u,pick(3)?pick(3):markers[pick(6)],1);m.put(rc+0x19u,pick(4)?pick(4):0xffu,1);m.put(rc+0x74u,pick(4));
    }
    for(std::uint32_t at=frames;at<frames+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=placements;at<placements+0x200u;at+=4)m.put(at,pick(4)?0x3f800000u:std::uint32_t(rng()));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<8;++k){m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+3u,pick(3)?0u:pick(40),1);
        m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x40u*k);}
    for(std::uint32_t k=0;k<8;++k)m.put(objects+0x40u*k,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(instances+16u*k+8u,tags+0x400u*k);}
    for(std::uint32_t at=tags;at<tags+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(tags+0x400u*k+0x38u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
        m.put(tags+0x400u*k+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(tags+0x400u*k+0x4cu,tags+0x400u*k+0x100u);}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));m.put(stack+4u,backward?placements+0x100u:placements+4u*pick(16));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records+0x100u*pick(4);c.abi.gp[1]=backward?frames+0x100u:frames+4u*pick(16);
    if(backward)c.abi.eflags|=0x400u;else c.abi.eflags&=~0x400u;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,frames,placements,headers,datums,objects,instances,tags};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1dc680(v);},[](auto &v){original::run_e32c0(v);},
        [](auto &v){original::run_145560(v);});
}
void reference(WorldState &s){original::run_1dc680(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RECORD_PLACE");
    std::mt19937 rng(0x1dc680u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
