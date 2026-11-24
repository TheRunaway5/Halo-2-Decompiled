// Native TU1.5 weapon trade (E79C0) against the unmodified translated body of
// the combat action unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// CD440, CDCC0 and 11B560 (direct) and 30BD0 and CFC90 (through the unit's
// dispatch) are hash-driven stubs of their complete entry ABIs: CD440,
// CDCC0 and 11B560 write a vector (on a small grid now and then NaN), 30BD0
// hashes the offset and answers a length in XMM0, CFC90 fills the hands
// record and answers AL (RET C); the caller-saved registers, XMM (XMM5 to
// XMM7 among them), flags and (by policy) MXCSR are random, and by policy
// EBX, EBP or EDI (another player) change. Fixtures author busy and idle
// units, units with and without a weapon, the thresholds on the grid;
// one region removed injects a fault. Modes: <cases> [census|verify].
// Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_weapon_trade.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_weapon_trade_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,object_datums=0x10001000u,units=0x10010000u,players=0x10020000u,player_datums=0x10021000u,
    scratch=0x10600000u;
std::uint32_t bits(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
std::uint32_t grid(std::mt19937_64 &rng){return rng()%7?bits(float(int(rng()%5)-2)/2.0f):0x7fc00000u|std::uint32_t(rng()%0x100);}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    const unsigned args=target==0xcfc90u?3u:0u;
    for(std::uint32_t k=1;k<=args;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    if(target==0x30bd0u)for(std::uint32_t k=0;k<3;++k)h=mix(h,r(s.memory,s.gp[0]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    const std::uint32_t out=target==0xcd440u||target==0x11b560u?s.gp[6]:target==0xcdcc0u?s.gp[0]:0u;
    if(out)for(std::uint32_t k=0;k<3;++k)w(s.memory,s.observe,target+1u,out+4u*k,4,grid(rng));
    if(target==0xcfc90u)w(s.memory,s.observe,0xcfc91u,r(s.memory,s.gp[4]+12u),4,std::uint32_t(rng())&(rng()%2?0x00ff0000u:0u));
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0xcfc90u)s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%4?1u+std::uint32_t(rng()%255):0u);
    if(policy&2u && rng()%13==0)s.gp[3]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[5]=std::uint32_t(rng());
    if(policy&2u && rng()%13==0)s.gp[7]=player_datums+0x21cu*std::uint32_t(rng()%4);
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    if(target==0x30bd0u){lanes={rng()%9?bits(float(rng()%5)/2.0f):0x7fc00000u,std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};std::memcpy(&s.xmm[0],lanes.data(),16);}
    s.gp[4]+=4u+4u*args;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_cd440(WorldState &s){stub(s,0xcd440u);}
static void run_cdcc0(WorldState &s){stub(s,0xcdcc0u);}
static void run_11b560(WorldState &s){stub(s,0x11b560u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0x30bd0u && target!=0xcfc90u)throw std::runtime_error("Unbound sandbox dispatch target");
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
    m.put(0x4ecbd0u,headers);m.put(0x4f54f4u,players);
    m.put(0x46a0e8u,bits(float(pick(2))/2.0f));m.put(0x46a0d0u,bits(float(1+pick(4))/2.0f));m.put(0x456910u,bits(float(int(pick(3))-1)/2.0f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,object_datums);
    for(std::uint32_t at=object_datums;at<object_datums+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k)m.put(object_datums+12u*k+8u,units+0x400u*pick(4));
    for(std::uint32_t at=units;at<units+0x1000u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto u=units+0x400u*k;
        m.put(u+0x12cu,pick(5)?std::uint32_t(rng()):0xffffffffu);m.put(u+0x212u,pick(5)?pick(4):0xffu,1);
    }
    for(std::uint32_t at=players;at<players+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(players+0x44u,player_datums);
    for(std::uint32_t at=player_datums;at<player_datums+0x21cu*4u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(player_datums+0x21cu*k+0x2cu,(std::uint32_t(rng())&0xffff0000u)|pick(8));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0xe0000u+pick(0x100));m.put(stack+4u,(std::uint32_t(rng())&0xffff0000u)|pick(8));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,object_datums,units,players,player_datums};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_e79c0(v);},[](auto &v){original::run_cd440(v);},
        [](auto &v){original::run_cdcc0(v);},[](auto &v){original::run_11b560(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_e79c0(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_WEAPON_TRADE");
    std::mt19937 rng(0xe79c0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
