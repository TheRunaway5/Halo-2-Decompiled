// Native TU1.5 ray hit accept (185820) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 186C10 (direct) and 1D7530 (behind the unit's dispatch) are hash-driven
// stubs of their complete entry ABIs: 186C10 answers a body entry, 1D7530 an
// owner (the ignored one at times); each writes scratch, the caller-saved
// registers, XMM, flags and (by policy) MXCSR are random, and by policy EBX,
// EBP, ESI or EDI change or the stack is left unbalanced. Fixtures author
// hits before and past the end, nearer and farther than the best, ignored
// objects or none, normals facing and not (NaN too), and one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_ray_hit_accept.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_ray_hit_accept_detail;
namespace {
constexpr std::uint32_t queries=0x10000000u,bests=0x10010000u,hits=0x10020000u,bodies=0x10030000u,scratch=0x10600000u;
std::uint32_t bits_of(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x186c10u?bodies+0x40u*std::uint32_t(rng()%2):std::uint32_t(rng()%3);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=hits+0x20u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%3);
    if(policy&2u && rng()%13==0)s.gp[6]=queries+0x20u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[7]=bests+0x40u*std::uint32_t(rng()%2);
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
static void run_186c10(WorldState &s){stub(s,0x186c10u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1d7530u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x46a0d0u,bits_of(1.f));
    static constexpr float fractions[]{0.f,0.25f,0.5f,0.75f,1.f,1.5f};
    static constexpr float coords[]{-1.f,-0.5f,0.f,0.5f,1.f};
    for(std::uint32_t k=0;k<2;++k) {
        const auto q=queries+0x20u*k,b=bests+0x40u*k,h=hits+0x20u*k;
        for(std::uint32_t at=q;at<q+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(q+8u,b);m.put(q+0xcu,pick(2)?0xffffffffu:pick(3));
        for(std::uint32_t at=b;at<b+0x40u;at+=4)m.put(at,bits_of(coords[pick(5)]));
        m.put(b+0x18u,bits_of(fractions[pick(6)]));
        for(std::uint32_t at=h;at<h+0x20u;at+=4)m.put(at,pick(16)?bits_of(coords[pick(5)]):0x7fc00000u);
        m.put(h+0x14u,bits_of(fractions[pick(6)]));
    }
    for(std::uint32_t k=0;k<2;++k){for(std::uint32_t at=bodies+0x40u*k;at<bodies+0x40u*(k+1u);at+=4)m.put(at,std::uint32_t(rng()));
        m.put(bodies+0x40u*k+0x18u,pick(3)?1u:pick(4));m.put(bodies+0x40u*k+0x20u,pick(4)?1u+pick(4):0u);}
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));m.put(stack+4u,std::uint32_t(rng()));m.put(stack+8u,hits+0x20u*pick(2));
    random_abi(rng,c,stack);
    c.abi.gp[1]=queries+0x20u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{queries,bests,hits,bodies};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_185820(v);},[](auto &v){original::run_186c10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_185820(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_RAY_HIT_ACCEPT");
    std::mt19937 rng(0x185820u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
