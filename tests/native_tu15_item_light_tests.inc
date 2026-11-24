// Native TU1.5 item light against the unmodified translated bodies of the
// scene device constructor unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first: 1D9080 place,
// 1D9100 aim. 315C90, 315CB0 and the light objects' functions (through the
// unit's dispatch) are hash-driven stubs of their complete entry ABIs and
// arguments: 315C90 writes the settled byte, answers its address and pops
// it; the functions pop the vector's address; all write scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy ESI changes or the stack is left unbalanced. Fixtures author
// records with and without the aiming flag, lights on and off, pending or
// not, and stacks of every alignment; one region removed injects a fault.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x1d9080
#include "rendering/renderer/native_tu15_item_light_place.hpp"
namespace leaf=halo2::native_item_light_detail::item_light_place;
#else
#include "rendering/renderer/native_tu15_item_light_aim.hpp"
namespace leaf=halo2::native_item_light_detail::item_light_aim;
#endif
using namespace frame_misc_harness;
namespace {
constexpr std::uint32_t records=0x10000000u,items=0x10010000u,lights=0x10020000u,objects=0x10030000u,vectors=0x10040000u,
    scratch=0x10600000u,functions=0x00700000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target>=functions)for(std::uint32_t k=0;k<4;++k)h=mix(h,r(s.memory,r(s.memory,s.gp[4]+4u)+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x315c90u)w(s.memory,s.observe,target+1u,r(s.memory,s.gp[4]+4u),1,rng()%2?0u:1u);
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x315c90u?(rng()%6==0?scratch+4u*std::uint32_t(rng()%8):r(s.memory,s.gp[4]+4u)):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=lights+0x80u*std::uint32_t(rng()%2);
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
static void run_315c90(WorldState &s){stub(s,0x315c90u,1);}
static void run_315cb0(WorldState &s){stub(s,0x315cb0u,0);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target<functions || target>=functions+0x100u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,1);
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
    for(std::uint32_t at=records;at<records+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(records+0x70u,items);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=items+0x60u*k;at<items+0x60u*k+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(items+0x60u*k+0x40u,lights+0x80u*(k%2));}
    for(std::uint32_t l=0;l<2;++l) {
        const auto light=lights+0x80u*l;
        for(std::uint32_t at=light;at<light+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(light+0x3cu,objects+0x40u*l);m.put(light+0x40u,pick(4)==0?1u+pick(255):0u,1);m.put(light+0x44u,pick(2)?0u:std::uint32_t(rng()));
        m.put(objects+0x40u*l,objects+0x100u);
    }
    for(std::uint32_t at=objects+0x100u;at<objects+0x160u;at+=4)m.put(at,functions+4u*pick(16));
    for(std::uint32_t at=vectors;at<vectors+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1d0000u+pick(0x100));m.put(stack+4u,records);
    random_abi(rng,c,stack);
    c.abi.gp[0]=OWNER==0x1d9080?vectors+4u*pick(4):pick(4);c.abi.gp[1]=OWNER==0x1d9080?pick(4):vectors+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,items,lights,objects,vectors};
        m.drop(regions[pick(5)]);
    }
    return c;
}
#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)
bool native(WorldState &s) {
    return leaf::bridge(s,[](auto &v){CAT(original::native_frame_misc_reference_,OWNER_NAME)(v);},[](auto &v){original::run_315c90(v);},
        [](auto &v){original::run_315cb0(v);},[](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){CAT(original::run_,OWNER_NAME)(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,VERIFY_SWITCH);
    std::mt19937 rng(OWNER+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,leaf::coverage().load(),unsigned(leaf::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
