// Native TU1.5 biped action test (1540D0) against the unmodified translated
// body of the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 1E7810 (through the unit's dispatch), E8110, E79C0 and 153E10 are
// hash-driven stubs of their complete entry ABIs and arguments: 1E7810 and
// E79C0 answer AL, E8110 a target, 153E10 writes the context; all write
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX, EBP, ESI or EDI change or the stack is left
// unbalanced. Fixtures author players whose units are bipeds or not (types
// 16 and 32 too, as SHL masks its count), bipeds
// with and without the ignored bit, and both teams; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_biped_action.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_biped_action_detail;
namespace {
constexpr std::uint32_t players=0x10000000u,player_data=0x10010000u,objects=0x10020000u,object_data=0x10030000u,bipeds=0x10040000u,
    contexts=0x10500000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    if(target==0x153e10u)w(s.memory,s.observe,target,s.gp[7]+4u*std::uint32_t(rng()%3),4,std::uint32_t(rng()));
    w(s.memory,s.observe,target+2u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=target==0x1e7810u || target==0xe79c0u?(std::uint32_t(rng())&0xffffff00u)|(rng()%2?1u:0u):std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%11==0)s.gp[6]=contexts+4u*std::uint32_t(rng()%4);
    if(policy&2u && rng()%11==0)s.gp[7]=player_data+0x21cu*std::uint32_t(rng()%2);
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
static void run_e8110(WorldState &s){stub(s,0xe8110u,2);}
static void run_e79c0(WorldState &s){stub(s,0xe79c0u,1);}
static void run_153e10(WorldState &s){stub(s,0x153e10u,1);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x1e7810u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target,0);
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
    m.put(0x4f54f4u,players);m.put(0x4ecbd0u,objects);
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_data);
    for(std::uint32_t at=player_data;at<player_data+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(player_data+0x21cu*k+0x2cu,(std::uint32_t(rng())&0xffff0000u)|pick(8));
    for(std::uint32_t at=objects;at<objects+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(objects+0x44u,object_data);
    for(std::uint32_t k=0;k<8;++k){m.put(object_data+12u*k,std::uint32_t(rng()));
        m.put(object_data+12u*k+3u,pick(3)?0u:pick(3)==0?(pick(2)?32u:16u):pick(12),1);m.put(object_data+12u*k+4u,std::uint32_t(rng()));
        m.put(object_data+12u*k+8u,bipeds+0x200u*(k%4));}
    for(std::uint32_t at=bipeds;at<bipeds+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(bipeds+0x200u*k+0x10au,pick(3)==0?4u:std::uint32_t(rng())&0xfbu,1);
    for(std::uint32_t at=contexts;at<contexts+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(4));m.put(stack+8u,contexts+4u*pick(4));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(8);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{players,player_data,objects,object_data,bipeds,contexts};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_1540d0(v);},[](auto &v){original::run_e8110(v);},
        [](auto &v){original::run_e79c0(v);},[](auto &v){original::run_153e10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_1540d0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_BIPED_ACTION");
    std::mt19937 rng(0x1540d0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
