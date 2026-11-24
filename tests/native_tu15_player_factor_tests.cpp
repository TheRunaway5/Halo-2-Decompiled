// Native TU1.5 player factor (15B2D0) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author kinds 2 and others, game modes 7
// and others, player masks with the player's bit set or not, game flags and
// settings 0..3, players of classes -1..5, multipliers (NaN too) under the
// random MXCSR, and the original jump table's four cases (now and then an
// unbound entry, or a bound one that is no case); one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/players/native_tu15_player_factor.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_factor_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
// The unit's table targets (and one bound target that is no case), as its
// canonical_branch names them.
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x15b377u:case 0x15b383u:case 0x15b38fu:case 0x15b397u:case 0x1d4a9u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t players=0x10000000u,data=0x10010000u,game=0x10020000u,masks=0x10030000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    static constexpr std::uint32_t floats[]{0x3f800000u,0x3f000000u,0x40400000u,0x3e4ccccdu,0x7fc00000u,0x80000000u,0x00000001u};
    m.put(0x4f54f4u,players);m.put(0x4f3218u,game);m.put(0x51df9cu,masks+2u*pick(4));
    for(const auto at:{0x46a0d0u,0x46a0e4u,0x46a13cu,0x46a0ccu,0x46a130u})m.put(at,pick(3)?floats[pick(4)]:floats[pick(7)]);
    static constexpr std::uint32_t targets[]{0x15b377u,0x15b383u,0x15b38fu,0x15b397u};
    for(std::uint32_t k=0;k<4;++k)m.put(0x15b39cu+4u*k,shape==9 && pick(4)==0?(pick(2)?0x15b300u:0x1d4a9u):targets[pick(4)]);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t at=data;at<data+0x21cu*0x14u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t classes[]{0u,1u,2u,3u,4u,0xffu,0x80u};
    for(std::uint32_t k=0;k<0x14u;++k)m.put(data+0x21cu*k+0xc1u,pick(3)?pick(4):classes[pick(7)],1);
    for(std::uint32_t at=game;at<game+0x240u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(game+0x180u,pick(2)?7u:pick(9));m.put(game+0x1c4u,pick(5)?pick(4):std::uint32_t(rng()));
    for(std::uint32_t at=masks;at<masks+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,pick(3)?2u:pick(5));
    random_abi(rng,c,stack);
    c.abi.gp[1]=(std::uint32_t(rng())&0xffff0000u)|pick(4)|(pick(4)==0?0x10u:0u);   // indices past 16 too
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,data,game,masks};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15b2d0(v);},
        [](std::uint32_t p,SourcePhysicsContactImage image){return original::canonical_branch(p,image);});
}
void reference(WorldState &s){original::run_15b2d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_FACTOR");
    std::mt19937 rng(0x15b2d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
