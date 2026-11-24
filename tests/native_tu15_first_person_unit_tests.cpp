// Native TU1.5 first-person unit (3D2B0) against the unmodified translated
// body of the texture camera producer unit (the first-person light pose
// unit's copy is the same), extracted by tools/validate_native_frame_misc_tu15.py
// into body.inc, plus the verification reference the production binder
// generates (reference.inc). BCDF0, 158290 and 168DED (through the unit's
// dispatch) are hash-driven stubs of their complete entry ABIs and
// arguments: BCDF0 answers an object or 0, 158290 a claim or 0, 168DED a
// count, writing a model; all write scratch, the caller-saved registers,
// XMM, flags and (by policy) MXCSR are random, and by policy ESI or EDI
// change or the stack is left unbalanced. Fixtures author overrides active
// and not, of mode 4 and others, set units, views of NONE and every local
// player, players NONE or not; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/animation/native_tu15_first_person_unit.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_first_person_unit_detail;
namespace {
constexpr std::uint32_t override_at=0x10000000u,mapping=0x10010000u,player_array=0x10020000u,player_data=0x10030000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x168dedu)w(s.memory,s.observe,target,0x4ce3f8u+4u*std::uint32_t(rng()%8),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x168dedu?std::uint32_t(rng()%5):rng()%2?0u:std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%5)-1u;
    if(policy&2u && rng()%13==0)s.gp[7]=std::uint32_t(rng());
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    switch(target) {
    case 0xbcdf0u:stub(s,target,1);return;
    case 0x158290u:stub(s,target,0);return;
    case 0x168dedu:stub(s,target,4);return;
    }
    throw std::runtime_error("Unbound sandbox dispatch target");
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
    for(std::uint32_t at=override_at;at<override_at+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(override_at,pick(3)?1u+pick(255):0u,1);m.put(override_at+2u,pick(3)?4u:pick(8),2);
    for(std::uint32_t at=0x4ce3f0u;at<0x4ce420u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4c3ae0u,pick(3)?0xffffffffu:std::uint32_t(rng()));
    m.put(0x4c66f8u,pick(5)?pick(4):0xffffffffu);
    m.put(0x51df6cu,override_at);m.put(0x4f54f0u,mapping);m.put(0x4f54f4u,player_array);
    for(std::uint32_t at=mapping;at<mapping+0x20u;at+=4)m.put(at,pick(3)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
    for(std::uint32_t at=player_array;at<player_array+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(player_array+0x44u,player_data);
    for(std::uint32_t at=player_data;at<player_data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x3d0000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{override_at,mapping,player_array,player_data};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_3d2b0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_3d2b0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FIRST_PERSON_UNIT");
    std::mt19937 rng(0x3d2b0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
