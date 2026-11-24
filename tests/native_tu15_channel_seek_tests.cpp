// Native TU1.5 channel seek (1CEBD0) against the unmodified translated body of
// the scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E3150 and 1CE950 are hash-driven stubs of their complete entry ABIs (and
// the arguments they are handed) behind the unit's dispatch: 1E3150 answers
// a record; each writes scratch, the caller-saved registers, XMM, flags and
// (by policy) MXCSR are random, and by policy EBX, EBP, ESI or EDI change or
// the stack is left unbalanced. Fixtures author channels in use or not,
// frames (one and zero too), fractions below 0, inside and past 1 (NaN too),
// and one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_channel_seek.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_channel_seek_detail;
namespace {
constexpr std::uint32_t channels=0x10000000u,instances=0x10010000u,records=0x10020000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=5;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x1e3150u?records+0x20u*std::uint32_t(rng()%4):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=channels+0x20u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    std::uint32_t cleanup=0u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%3);
    s.gp[4]+=(target==0x1e3150u?8u:24u)+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1e3150u && target!=0x1ce950u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f0414u,instances);m.put(0x46a0ecu,bits_of(0.001f));m.put(0x14u,pick(5),2);
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=instances+16u*k;at<instances+16u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));}
    for(std::uint32_t at=channels;at<channels+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k){m.put(channels+0x20u*k,(std::uint32_t(rng())&0xffff0000u)|pick(2));m.put(channels+0x20u*k+6u,pick(5)?pick(4):0xffffu,2);}
    for(std::uint32_t at=records;at<records+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(records+0x20u*k+0x14u,pick(5)?pick(40):pick(2)?0u:0xfff0u,2);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr float fractions[]{0.f,0.5f,1.f,-0.5f,1.5f,0.999f};
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,pick(12)?bits_of(fractions[pick(6)]):0x7fc00000u);
    random_abi(rng,c,stack);
    c.abi.gp[6]=channels+0x20u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{channels,instances,records};
        m.drop(regions[pick(3)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1cebd0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1cebd0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_CHANNEL_SEEK");
    std::mt19937 rng(0x1cebd0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
