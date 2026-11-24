// Native TU1.5 seat mode (158340) against the unmodified translated body of
// the physics live support unit (the validator repeats it with the campaign
// camera and map world support units', where 111CD0 and C9550 go through the
// dispatch), extracted by tools/validate_native_frame_misc_tu15.py into
// body.inc, plus the verification reference the production binder generates
// (reference.inc). BCDF0, 111CD0 and C9550 are hash-driven stubs of their
// complete entry ABIs: BCDF0 answers a seat record (or none) and pops its
// kind, 111CD0 a name (often 400004A), C9550 AL; each writes scratch, the
// caller-saved registers, XMM, flags and (by policy) MXCSR are random, and by
// policy EBX, EBP, ESI or EDI change or the stack is left unbalanced.
// Fixtures author records with and without vehicles, vehicles of every type
// and seats with every flag; one region removed injects a fault. Modes:
// <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_seat_mode.hpp"
using namespace frame_misc_harness;
namespace seat_mode=halo2::native_seat_mode_detail;
namespace {
constexpr std::uint32_t records=0x10000000u,header=0x10010000u,rows=0x10020000u,objects=0x10030000u,tags=0x10040000u,
    definitions=0x10050000u,seats=0x10060000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target,std::uint32_t arguments) {
    std::uint64_t h=entry_hash(s,target);
    for(std::uint32_t k=1;k<=arguments;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(target==0xbcdf0u)s.gp[0]=rng()%5==0?0u:records+0x200u*std::uint32_t(rng()%2);
    else if(target==0x111cd0u)s.gp[0]=rng()%2?0x400004au:std::uint32_t(rng());
    else s.gp[0]=rng()%2?std::uint32_t(rng())&~0xffu:std::uint32_t(rng());
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[5]=rng()%2?0x400004au:std::uint32_t(rng());
    if(policy&2u && rng()%17==0)s.gp[6]=objects+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%17==0)s.gp[7]=records+0x200u*std::uint32_t(rng()%2);
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
static void run_111cd0(WorldState &s){stub(s,0x111cd0u,0);}
static void run_c9550(WorldState &s){stub(s,0xc9550u,0);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0xbcdf0u)stub(s,target,1);
    else if(target==0x111cd0u || target==0xc9550u)stub(s,target,0);
    else throw std::runtime_error("Unbound sandbox dispatch target");
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
    for(std::uint32_t g=0;g<2;++g) {
        const auto record=records+0x200u*g;
        for(std::uint32_t at=record;at<record+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(record+0x14u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())<<16)|pick(4));
        m.put(record+0x1fcu,pick(4),2);
    }
    m.put(0x4ecbd0u,header);
    for(std::uint32_t at=header;at<header+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(header+0x44u,rows);
    for(std::uint32_t k=0;k<4;++k){m.put(rows+12u*k,std::uint32_t(rng()));m.put(rows+12u*k+4u,std::uint32_t(rng()));m.put(rows+12u*k+8u,objects+0x100u*(k%2));}
    for(std::uint32_t o=0;o<2;++o){for(std::uint32_t at=objects+0x100u*o;at<objects+0x200u*o+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(objects+0x100u*o,(std::uint32_t(rng())<<16)|pick(4));m.put(objects+0x100u*o+0xaau,pick(3)?pick(2):pick(32),1);}
    m.put(0x4f0414u,tags);
    for(std::uint32_t k=0;k<4;++k){for(std::uint32_t at=tags+16u*k;at<tags+16u*k+16u;at+=4)m.put(at,std::uint32_t(rng()));m.put(tags+16u*k+8u,definitions+0x200u*(k%2));}
    for(std::uint32_t d=0;d<2;++d){for(std::uint32_t at=definitions+0x200u*d;at<definitions+0x200u*d+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
        m.put(definitions+0x200u*d+0x1ccu,seats+0x400u*d);}
    for(std::uint32_t at=seats;at<seats+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x150000u+pick(0x100));m.put(stack+4u,scratch+0x80u+4u*pick(8));
    random_abi(rng,c,stack);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,header,objects,tags,definitions,seats};
        m.drop(regions[pick(6)]);
    }
    return c;
}
bool native(WorldState &s) {
    return seat_mode::bridge(s,[](auto &v){original::native_frame_misc_reference_158340(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);},
        [](auto &v){original::run_111cd0(v);},[](auto &v){original::run_c9550(v);});
}
void reference(WorldState &s){original::run_158340(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_SEAT_MODE");
    std::mt19937 rng(0x158340u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,seat_mode::coverage().load(),unsigned(seat_mode::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
