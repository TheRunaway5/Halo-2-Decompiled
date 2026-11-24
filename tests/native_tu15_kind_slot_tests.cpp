// Native TU1.5 kind slot (15CAB0) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author sessions with and without a
// variant, counts zero, negative and several, and lists of the kinds the
// compare tree tells apart (0, 1, 9, 0xC, 0xD, 0xE and others, negative
// words too) against kinds of the same set; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_kind_slot.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_kind_slot_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t session=0x10000000u,lists=0x10010000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t kinds[]{0u,1u,9u,0xcu,0xdu,0xeu,2u,5u,0xffffu,0x8009u};
    m.put(0x4f63b8u,session);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(3)?std::uint32_t(rng())|1u:0u);
    for(std::uint32_t at=lists;at<lists+0x40u;at+=2)m.put(at,kinds[pick(10)],2);
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,pick(6)?pick(12):pick(2)?0u:0xffffffffu);
    random_abi(rng,c,stack);
    c.abi.gp[7]=lists+2u*pick(8);
    static constexpr std::uint32_t wanted[]{1u,9u,2u,0xcu,0xdu,0u,0xffffffffu};
    c.abi.gp[2]=wanted[pick(7)];
    if(shape==15)m.drop(pick(2)?session:lists);
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15cab0(v);});}
void reference(WorldState &s){original::run_15cab0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_KIND_SLOT");
    std::mt19937 rng(0x15cab0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
