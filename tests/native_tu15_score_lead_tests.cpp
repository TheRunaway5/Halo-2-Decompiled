// Native TU1.5 score lead (15BE80) against the unmodified translated body of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 15B210, 1648E0 and 1A8E50 (through the dispatch) are hash-driven stubs of
// their complete entry ABIs: 15B210 answers a team's score, 1648E0 whether
// it is active, 1A8E50 advances the iterator it is given; each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author team and free-for-all games, sessions with and
// without a variant record, and scores below, equal to and above the
// player's; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_score_lead.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_score_lead_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,data=0x10010000u,session=0x10020000u,game=0x10030000u,outs=0x10040000u,scratch=0x10600000u;
std::uint32_t limit=0;
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x1a8e50u) {
        const auto iterator=s.gp[6];
        const auto index=r(s.memory,iterator+0xcu)+1u;
        const bool more=index<limit;
        w(s.memory,s.observe,0x1a8e51u,iterator,4,more?data+0x21cu*index:0u);
        w(s.memory,s.observe,0x1a8e52u,iterator+8u,4,more?(std::uint32_t(rng()&0x7fffu)<<16)|index:0xffffffffu);
        w(s.memory,s.observe,0x1a8e53u,iterator+0xcu,4,index);
        s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(more?1u+std::uint32_t(rng()%255):0u);
    } else if(target==0x1648e0u)s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%3?1u:0u);
    else s.gp[0]=std::uint32_t(rng()%4);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[6]=data+0x21cu*std::uint32_t(rng()%4);
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?session:std::uint32_t(rng()%9);
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
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x15b210u && target!=0x1648e0u && target!=0x1a8e50u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4f54f4u,players);m.put(0x4f63b8u,session);m.put(0x4f3218u,game);
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)?std::uint32_t(rng())|1u:0u);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t at=data;at<data+0x21cu*6u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<6;++k)m.put(data+0x21cu*k+0xc0u,pick(8),1);
    for(std::uint32_t at=session;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t k=0;k<8;++k)m.put(session+0x304u+0x1cu*k+4u,pick(4),2);
    for(std::uint32_t at=game;at<game+0x190u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=outs;at<outs+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(6));
    m.put(stack+8u,outs+pick(4));m.put(stack+12u,outs+8u+pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,data,session,game,outs};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15be80(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_15be80(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SCORE_LEAD");
    std::mt19937 rng(0x15be80u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        limit=std::uint32_t(rng()%6);
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
