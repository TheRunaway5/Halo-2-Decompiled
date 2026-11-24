// Native TU1.5 engine queries against the unmodified translated bodies of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first: 25BCC0 option
// flag, 18A240 director player. There are no children. Fixtures author
// every kind, and players NONE, past the limit, of zero or other salts,
// with and without units; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x25bcc0
#include "gameplay/multiplayer/native_tu15_option_flag.hpp"
namespace leaf=halo2::native_engine_queries_detail::option_flag;
#else
#include "gameplay/multiplayer/native_tu15_director_player.hpp"
namespace leaf=halo2::native_engine_queries_detail::director_player;
#endif
using namespace frame_misc_harness;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t globals=0x10000000u,records=0x10010000u,header=0x10020000u,players=0x10030000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4f3218u,globals);
    for(std::uint32_t at=globals;at<globals+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f9b54u,records);
    for(std::uint32_t at=records;at<records+4u*0x94u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4f54f4u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x24u,0x21cu);m.put(header+0x38u,pick(4)==0?0x80000004u:4u);m.put(header+0x44u,players);
    for(std::uint32_t at=players;at<players+4u*0x21cu;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t salts[4];
    for(std::uint32_t k=0;k<4;++k) {
        salts[k]=pick(4)==0?0u:pick(4)==0?0x8000u+pick(16):0x1000u+pick(16);
        m.put(players+0x21cu*k,salts[k],2);
        m.put(players+0x21cu*k+0x2cu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
    }
    for(std::uint32_t k=0;k<4;++k) {
        const auto index=pick(6);
        const auto salt=index<4 && pick(3)?salts[index]:pick(0x10000);
        m.put(records+0x94u*k+0x60u,pick(5)==0?0xffffffffu:(salt<<16)|index);
    }
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x180000u+pick(0x100));m.put(stack+8u,pick(6));
    random_abi(rng,c,stack);
    c.abi.gp[0]=pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{globals,records,header,players};
        m.drop(regions[pick(4)]);
    }
    return c;
}
#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){CAT(original::native_frame_misc_reference_,OWNER_NAME)(v);});}
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
