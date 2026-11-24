// Native TU1.5 player unit search (153360) against the unmodified translated
// body of the multiplayer engines unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BD080 (through the unit's dispatch) and 1538B0 are hash-driven stubs of
// their complete entry ABIs and arguments: BD080 fills the found list and
// answers its count (negative too), 1538B0 writes scratch; both make the
// caller-saved registers, XMM, flags and (by policy) MXCSR random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author players with and without units, riding or not, and found
// objects of every type with child lists (naming later objects only);
// one region removed injects a fault. Modes: <cases> [census|verify]. Prints
// a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/multiplayer/native_tu15_player_unit_search.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_player_unit_search_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,data=0x10010000u,headers=0x10020000u,datums=0x10030000u,objects=0x10040000u,
    scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0xbd080u) {
        const auto list=r(s.memory,s.gp[4]+24u);
        const auto count=std::uint32_t(rng()%6);
        for(std::uint32_t k=0;k<count;++k)w(s.memory,s.observe,target,list+4u*k,4,(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%8));
        s.gp[0]=(std::uint32_t(rng())&0xffff0000u)|(rng()%9==0?0xfffeu:count);
    } else s.gp[0]=std::uint32_t(rng());
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x1538b0u) {
        if(policy&2u && rng()%13==0)s.gp[3]=headers;
        if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng()%4);
        if(policy&2u && rng()%13==0)s.gp[6]=std::uint32_t(rng()%8);
        if(policy&2u && rng()%13==0)s.gp[7]=objects+0x100u*std::uint32_t(rng()%8);
    }
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
static void run_1538b0(WorldState &s){stub(s,0x1538b0u,2);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbd080u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,7);
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
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,headers);m.put(0x456918u,0x3ecccccdu);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,data);
    for(std::uint32_t at=data;at<data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(data+0x21cu*k+0x2cu,pick(5)?(std::uint32_t(rng())&0xffff0000u)|pick(8):0xffffffffu);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    static constexpr std::uint32_t types[]{0u,1u,2u,3u,4u,7u,11u,0xffu};
    for(std::uint32_t k=0;k<8;++k) {
        m.put(datums+12u*k,std::uint32_t(rng()));m.put(datums+12u*k+4u,std::uint32_t(rng()));m.put(datums+12u*k+8u,objects+0x100u*k);
        const auto object=objects+0x100u*k;
        for(std::uint32_t at=object;at<object+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto later=[&](){return k==7 || pick(3)==0?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|(k+1u+pick(7u-k));};
        m.put(object+0xcu,later());m.put(object+0x10u,later());m.put(object+0x14u,pick(4)?0xffffffffu:std::uint32_t(rng()));
        m.put(object+0x3cu,0x3f800000u);m.put(object+0xaau,types[pick(8)],1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,data,headers,datums,objects};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_153360(v);},[](auto &v){original::run_1538b0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_153360(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_PLAYER_UNIT_SEARCH");
    std::mt19937 rng(0x153360u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
