// Native TU1.5 point blocked (16BDA0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 14CDB0, 16BBC0, 16E360, BA970, BA9E0 and 16BC80 are hash-driven stubs of
// their complete entry ABIs (and the arguments they are handed): 14CDB0
// answers a cluster or none, the iterator walks authored objects (writing
// its cursor), the others answer AL or a mask; each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author points outside and inside, leaves of each kind (none,
// planes the point is behind or not, shapes), surfaces, objects of each type
// near and far (the skips among them), every flag combination, and one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_point_blocked.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_point_blocked_detail;
namespace {
constexpr std::uint32_t bsp=0x10000000u,clusters=0x10010000u,planes=0x10020000u,leaves=0x10030000u,shapes=0x10040000u,
    objects=0x10050000u,points=0x10060000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
std::uint32_t walked=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=6;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    std::uint32_t answer=(std::uint32_t(rng())&0xffffff00u)|(rng()%4==0?1u+std::uint32_t(rng()%255):0u);
    if(target==0x14cdb0u)answer=rng()%6?std::uint32_t(rng()%4):0xffffffffu;
    if(target==0x16e360u){answer=std::uint32_t(rng()%3?0u:rng()%4);s.gp[1]=std::uint32_t(rng());}
    if(target==0xba970u || target==0xba9e0u) {
        if(target==0xba970u)walked=0;
        const auto cursor_slot=r(s.memory,s.gp[4]+(target==0xba970u?8u:4u));   // BA970: the leaf pushed last
        if(walked++<4u && rng()%5){const auto k=std::uint32_t(rng()%6);w(s.memory,s.observe,target+1u,cursor_slot,4,objects+0x20u*k);answer=0x100u+k;}
        else answer=0xffffffffu;
    }
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=answer;s.gp[2]=std::uint32_t(rng());
    if(target!=0x16e360u)s.gp[1]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=points+0x10u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    static constexpr std::pair<std::uint32_t,std::uint32_t> pops[]{{0x14cdb0u,4u},{0x16bbc0u,8u},{0x16e360u,4u},{0xba970u,12u},{0xba9e0u,8u},{0x16bc80u,28u}};
    for(const auto &[t,n]:pops)if(t==target)s.gp[4]+=n+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_14cdb0(WorldState &s){stub(s,0x14cdb0u);}
static void run_16bbc0(WorldState &s){stub(s,0x16bbc0u);}
static void run_16e360(WorldState &s){stub(s,0x16e360u);}
static void run_ba970(WorldState &s){stub(s,0xba970u);}
static void run_ba9e0(WorldState &s){stub(s,0xba9e0u);}
static void run_16bc80(WorldState &s){stub(s,0x16bc80u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecc0cu,std::uint32_t(rng()));m.put(0x4ecc18u,bsp);m.put(0x4ecc20u,shapes);
    for(std::uint32_t at=bsp;at<bsp+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(bsp+0x30u,clusters);m.put(bsp+0x68u,planes);m.put(bsp+0xa0u,leaves);
    for(std::uint32_t k=0;k<4;++k){m.put(clusters+8u*k,(std::uint32_t(rng())&0xffff0000u)|pick(3));m.put(clusters+8u*k+4u,std::uint32_t(rng()));}
    static constexpr float coords[]{-2.f,-1.f,0.f,0.5f,1.f,2.f};
    for(std::uint32_t k=0;k<128;++k) {
        const auto p=planes+24u*k;
        m.put(p,std::uint32_t(rng()));m.put(p+2u,pick(4)?pick(4):0xffffu,2);
        for(std::uint32_t at=p+4u;at<p+0x18u;at+=4)m.put(at,bits_of(coords[pick(6)]));
    }
    for(std::uint32_t k=0;k<3;++k) {
        const auto l=leaves+0xb0u*k;
        for(std::uint32_t at=l;at<l+0xb0u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(l+0x70u,pick(3)==0?0xffu:pick(2)?0x80u|pick(128):pick(128),1);m.put(l+0x98u,pick(4)?pick(4):0u-1u);m.put(l+0x9cu,l+0x80u);
    }
    for(std::uint32_t at=shapes;at<shapes+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(shapes+0x34cu,shapes+0x400u);
    for(std::uint32_t k=0;k<128;++k)m.put(shapes+0x400u+16u*k+0xcu,pick(3)?pick(4):0xffffu,2);
    for(std::uint32_t k=0;k<6;++k) {
        const auto o=objects+0x20u*k;
        m.put(o,pick(16)|(std::uint32_t(rng())&0xffff0000u)|(std::uint32_t(rng())&0xff00u));m.put(o+2u,pick(3)?0u:pick(8),2);
        for(std::uint32_t at=o+4u;at<o+0x10u;at+=4)m.put(at,bits_of(coords[pick(6)]));
        m.put(o+0x10u,bits_of(float(pick(4))));
    }
    for(std::uint32_t at=points;at<points+0x40u;at+=4)m.put(at,bits_of(coords[pick(6)]));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    static constexpr std::uint32_t flag_sets[]{0u,2u,4u,8u,0xcu,6u,0xau,0xeu};
    m.put(stack+4u,flag_sets[pick(8)]|(std::uint32_t(rng())&0x1ffff0u));m.put(stack+8u,points+0x10u*pick(4));
    m.put(stack+12u,0x100u+pick(6));m.put(stack+16u,pick(2)?0x100u+pick(6):0xffffffffu);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{bsp,clusters,planes,leaves,shapes,objects,points};
        m.drop(regions[pick(7)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_16bda0(v);},[](auto &v){original::run_14cdb0(v);},
        [](auto &v){original::run_16bbc0(v);},[](auto &v){original::run_16e360(v);},[](auto &v){original::run_ba970(v);},
        [](auto &v){original::run_16bc80(v);},[](auto &v){original::run_ba9e0(v);});
}
void reference(WorldState &s){original::run_16bda0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POINT_BLOCKED");
    std::mt19937 rng(0x16bda0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
