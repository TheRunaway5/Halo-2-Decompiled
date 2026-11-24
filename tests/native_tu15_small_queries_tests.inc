// Native TU1.5 small queries against the unmodified translated bodies of
// their units (1E5A50 sandbox special; 335B0 and 226B0 shadow frame; 150990
// combat action, which the validator repeats with the sandbox special
// unit's; 25BC70 multiplayer engines), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first. There are no
// children. Fixtures author every path (states NONE or with the gate byte
// set or clear, seats NONE and 0..3, equal and unequal words, locations
// and structures absent, empty, present, matching or not); one region
// removed injects a fault. Modes: <cases> [census|verify]. Prints a JSON
// summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x1e5a50
#include "gameplay/native_tu15_reference_clear.hpp"
namespace leaf=halo2::native_small_queries_detail::reference_clear;
#elif OWNER==0x335b0
#include "gameplay/native_tu15_shadow_option.hpp"
namespace leaf=halo2::native_small_queries_detail::shadow_option;
#elif OWNER==0x150990
#include "gameplay/native_tu15_seat_player.hpp"
namespace leaf=halo2::native_small_queries_detail::seat_player;
#elif OWNER==0x25bc70
#include "gameplay/native_tu15_word_change.hpp"
namespace leaf=halo2::native_small_queries_detail::word_change;
#else
#include "gameplay/native_tu15_cluster_match.hpp"
namespace leaf=halo2::native_small_queries_detail::cluster_match;
#endif
using namespace frame_misc_harness;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10000000u,states=0x10010000u,places=0x10020000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    for(std::uint32_t at=records;at<records+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=states;at<states+0x40u;at+=4)m.put(at,pick(2)?0u:std::uint32_t(rng()));
    for(std::uint32_t at=places;at<places+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x51df50u,pick(4)==0?0u:states+4u*pick(4));m.put(0x515ef5u,std::uint32_t(rng()),1);
    m.put(0x4f54f0u,records);
    m.put(0x4ecc14u,pick(5)==0?0u:places);m.put(0x4ecc18u,pick(5)==0?0u:places+0x100u);
    static constexpr std::uint32_t counts[]{0u,1u,5u,0x80000000u};
    m.put(places+0x80u,counts[pick(4)]);m.put(places+0x84u,places+0x40u);
    m.put(places+0x5cu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
    m.put(places+0x44u,pick(2)?r(m,places+0x108u):std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));
    const auto word=std::uint32_t(rng());
    m.put(stack+4u,word);m.put(stack+8u,pick(2)?(std::uint32_t(rng())&0xffff0000u)|(word&0xffffu):std::uint32_t(rng()));
    random_abi(rng,c,stack);
    c.abi.gp[0]=records+4u*pick(4);c.abi.gp[1]=pick(5)==0?0xffffffffu:pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,states,places};
        m.drop(regions[pick(3)]);
    }
    return c;
}
#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)
#define REFERENCE CAT(original::native_frame_misc_reference_,OWNER_NAME)
#define TRANSLATED CAT(original::run_,OWNER_NAME)
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){REFERENCE(v);});}
void reference(WorldState &s){TRANSLATED(s);}
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
