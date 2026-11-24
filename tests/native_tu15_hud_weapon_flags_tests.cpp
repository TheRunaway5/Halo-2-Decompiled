// Native TU1.5 HUD weapon flags (13DC44) against the unmodified translated
// body of the weapon HUD unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// BCDF0 and 1E7810 (through the unit's dispatch) and 10B3D0 (direct) are
// hash-driven stubs of their complete entry ABIs: BCDF0 answers a weapon or
// 0 (RET 4), 1E7810 and 10B3D0 answer AL (0 half the time); each writes
// scratch, the caller-saved registers, XMM, flags and (by policy) MXCSR are
// random, and by policy EBX (the other record), EBP (another authored
// frame), ESI (another slot) or EDI (another mask) change. Fixtures author
// units with and without a weapon, handles past the low indices, weapon
// types 0 to 33, magazines none, inside and past the count, charges below,
// at, above and NaN against one, and records with each target shape; one
// region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/weapons/native_tu15_hud_weapon_flags.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_hud_weapon_flags_detail;
namespace {
constexpr std::uint32_t headers=0x10000000u,datums=0x10010000u,units=0x10030000u,weapons=0x10040000u,tags=0x10050000u,
    defs=0x10070000u,entries=0x10080000u,records=0x10090000u,frames=0x100a0000u,scratch=0x10600000u;
constexpr std::uint32_t far_index=0x1000u;
std::uint32_t slot_of(std::uint32_t k){return units+0x200u*k+0x1c8u;}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    if(target==0xbcdf0u)h=mix(h,r(s.memory,s.gp[4]+4u));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0xbcdf0u)s.gp[0]=rng()%5?weapons+0x200u*std::uint32_t(rng()%4):0u;
    else s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%2?0u:1u+std::uint32_t(rng()%255));
    if(policy&2u && rng()%13==0)s.gp[3]=records+0x100u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[5]=frames+0x40u+0x40u*std::uint32_t(rng()%2);
    if(policy&2u && rng()%13==0)s.gp[6]=slot_of(std::uint32_t(rng()%4));
    if(policy&2u && rng()%13==0)s.gp[7]=rng()%2?0xffffu:0x3u;
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[rng()%8],lanes.data(),16);
    s.gp[4]+=target==0xbcdf0u?8u:4u;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_10b3d0(WorldState &s){stub(s,0x10b3d0u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target!=0xbcdf0u && target!=0x1e7810u)throw std::runtime_error("Unbound sandbox dispatch target");
    stub(s,target);
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t charge(std::mt19937 &rng) {
    static constexpr float values[]{0.5f,1.0f,1.5f,2.0f,-1.0f};
    if(rng()%9==0)return 0x7fc00000u;
    float f=values[rng()%5];std::uint32_t v;std::memcpy(&v,&f,4);return v;
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto index=[&](std::uint32_t k){return pick(k)+(pick(4)?0u:far_index);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(16)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);
    float one=pick(6)?1.0f:1.5f;std::uint32_t one_bits;std::memcpy(&one_bits,&one,4);m.put(0x46a0d0u,one_bits);
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,datums);
    for(std::uint32_t k=0;k<4;++k)for(std::uint32_t base:{0u,far_index})m.put(datums+12u*(base+k)+8u,units+0x200u*pick(4));
    for(std::uint32_t at=units;at<units+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k) {
        const auto slot=slot_of(k);
        m.put(slot,pick(4)?(std::uint32_t(rng())&0xffff0000u)|pick(4):0xffffffffu);
        m.put(slot+4u,pick(4)==0?0xffffffffu:pick(8)==0?0x80000000u|pick(4):pick(6));
        m.put(slot+0x1cu,charge(rng));
    }
    for(std::uint32_t at=weapons;at<weapons+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    static constexpr std::uint32_t types[]{0u,1u,2u,3u,32u,33u};
    for(std::uint32_t k=0;k<4;++k) {
        const auto weapon=weapons+0x200u*k;
        m.put(weapon,(std::uint32_t(rng())&0xffff0000u)|index(8));m.put(weapon+0xaau,types[pick(6)],1);
    }
    for(std::uint32_t k=0;k<8;++k)for(std::uint32_t base:{0u,far_index}) {
        const auto t=tags+16u*(base+k);
        m.put(t,std::uint32_t(rng()));m.put(t+4u,std::uint32_t(rng()));m.put(t+8u,defs+0x100u*pick(8));m.put(t+12u,std::uint32_t(rng()));
    }
    for(std::uint32_t at=defs;at<defs+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<8;++k) {
        const auto d=defs+0x100u*k;
        m.put(d+0x38u,pick(4)?(std::uint32_t(rng())&0xffff0000u)|index(8):0xffffffffu);
        m.put(d+0x68u,pick(6));m.put(d+0x6cu,entries+0x100u*pick(8));
    }
    for(std::uint32_t at=entries;at<entries+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x200u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<2;++k) {
        const auto record=records+0x100u*k;
        m.put(record+0xcu,pick(3)?pick(16):0xffffffffu);m.put(record+0xabu,pick(2)?0u:1u+pick(255),1);
        m.put(record+0x10u,pick(3)?pick(16):0xffffffffu);m.put(record+0xe3u,pick(2)?0u:1u+pick(255),1);
    }
    for(std::uint32_t at=frames;at<frames+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=1;k<=2;++k) {
        const auto f=frames+0x40u*k;
        m.put(f-4u,slot_of(pick(4)));m.put(f-8u,units+0x200u*pick(4));m.put(f-12u,defs+0x100u*pick(8));
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x130000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|index(4);c.abi.gp[3]=records+0x100u*pick(2);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,datums,units,weapons,tags,defs,entries,records};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_13dc44(v);},[](auto &v){original::run_10b3d0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_13dc44(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_HUD_WEAPON_FLAGS");
    std::mt19937 rng(0x13dc44u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
