// Native TU1.5 team active (164890) against the unmodified translated body of
// the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// A leaf: fixtures author engines present and absent, team and free-for-all
// games, teams inside and outside 0 to 7 and team masks, and one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_team_active.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_team_active_detail;
namespace {
constexpr std::uint32_t game=0x10000000u,engine_globals=0x10030000u;
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f3218u,game);m.put(0x4f63b8u,engine_globals);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)?std::uint32_t(rng()):0u);
    for(std::uint32_t at=game;at<game+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0x184u,std::uint32_t(rng())&0xfeu|(pick(5)?1u:0u),1);
    for(std::uint32_t at=engine_globals;at<engine_globals+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(engine_globals+0xc14u,pick(4));
    for(std::uint32_t at=stack-0x20u;at<stack+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xc0000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[1]=pick(6)?pick(8):pick(2)?8u+pick(30):0u-1u-pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{game,engine_globals};
        m.drop(regions[pick(2)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_164890(v);});}
void reference(WorldState &s){original::run_164890(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TEAM_ACTIVE");
    std::mt19937 rng(0x164890u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
