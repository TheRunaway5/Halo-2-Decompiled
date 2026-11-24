// Native TU1.5 side count (15B4D0) against the unmodified translated body of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author sessions with and without a
// variant record, team and free-for-all games, team words, and players
// arrays with few, many (past 32) and no players in use, negative first
// unused indices and records at address 0; one region removed injects a
// fault. Modes: <cases> [census|verify]. Prints a JSON summary with branch
// coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_side_count.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_side_count_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t session=0x10000000u,game=0x10010000u,players=0x10020000u,bitmap=0x10030000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4f63b8u,session);m.put(0x4f3218u,game);m.put(0x4f54f4u,players);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(3)?std::uint32_t(rng())|1u:0u);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t at=game;at<game+0x190u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto count=pick(6)?1u+pick(40):pick(2)?0u:0x80000000u+pick(4);
    m.put(players+0x24u,pick(8)?0x40u:0u);m.put(players+0x38u,count);m.put(players+0x44u,pick(8)?0x10040000u:0u);m.put(players+0x48u,bitmap);
    for(std::uint32_t at=bitmap;at<bitmap+8u;at+=4)m.put(at,std::uint32_t(rng())&std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{session,game,players,bitmap};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s){return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15b4d0(v);});}
void reference(WorldState &s){original::run_15b4d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SIDE_COUNT");
    std::mt19937 rng(0x15b4d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
