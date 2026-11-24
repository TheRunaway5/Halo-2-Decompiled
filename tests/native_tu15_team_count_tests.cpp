// Native TU1.5 team count (15B620) against the unmodified translated body of
// the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 15B470 (through the unit's dispatch) is a hash-driven stub of its complete
// entry ABI: it answers AL, writes scratch, the caller-saved registers, XMM,
// flags and (by policy) MXCSR are random, and by policy EBX, EBP, ESI or
// EDI change or the stack is left unbalanced. Fixtures author player arrays
// of negative, empty and several counts with sparse bitmaps, players
// inactive or not, of teams -1, 0..15, 16 and others, living or not, and
// outputs given or NULL; one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_team_count.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_team_count_detail;
namespace {
constexpr std::uint32_t arrays=0x10000000u,data=0x10010000u,bitmaps=0x10020000u,outputs=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s) {
    std::uint64_t h=entry_hash(s,0x15b470u);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x15b472u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=(std::uint32_t(rng())&0xffffff00u)|(rng()%2?1u:0u);s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[3]=arrays+0x80u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=1u<<(rng()%16);
    if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=data+0x40u*std::uint32_t(rng()%8);
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
    if(target!=0x15b470u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s);
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
    m.put(0x4f54f4u,arrays+0x80u*pick(2));
    static constexpr std::uint32_t counts[]{0u,1u,5u,16u,40u,0xffffffffu};
    for(std::uint32_t a=0;a<2;++a) {
        const auto array=arrays+0x80u*a;
        for(std::uint32_t at=array;at<array+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(array+0x24u,0x40u);m.put(array+0x38u,pick(3)?pick(20):counts[pick(6)]);m.put(array+0x44u,data);m.put(array+0x48u,bitmaps);
    }
    for(std::uint32_t at=bitmaps;at<bitmaps+0x10u;at+=4)m.put(at,std::uint32_t(rng())&std::uint32_t(rng()));
    static constexpr std::uint32_t teams[]{0u,1u,2u,7u,15u,16u,0xffu,0xfbu,0x40u};
    for(std::uint32_t k=0;k<48;++k) {
        const auto datum=data+0x40u*k;
        for(std::uint32_t at=datum;at<datum+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(datum+2u,pick(3)==0?pick(4):0u,1);
    }
    for(std::uint32_t at=data+0x40u*48u;at<data+0x40u*48u+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<48;++k)m.put(data+0x40u*k+0xc0u,pick(3)?pick(4):teams[pick(9)],1);
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,outputs);
    m.put(stack+8u,pick(4)?outputs+8u:0u);m.put(stack+12u,pick(4)?outputs+0x10u:0u);
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{arrays,data,bitmaps,outputs};
        m.drop(regions[pick(4)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_15b620(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_15b620(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TEAM_COUNT");
    std::mt19937 rng(0x15b620u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
