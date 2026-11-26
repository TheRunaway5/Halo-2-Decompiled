// Native TU1.5 zoom factor (CAD60) against the unmodified translated body of
// the vehicle director unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// 106040 (dispatched) and 106160 (direct tail call) are hash-driven stubs of
// their complete entry ABIs: 106040 returns AL (0 or nonzero) and randomizes
// caller-saved registers, XMM, flags and (by policy) MXCSR, and by policy EBX,
// ESI or EDI change or ESP is left unbalanced; 106160 returns a random scalar
// zoom in XMM0. Fixtures author units with and without weapon slots, weapons
// with and without zoom ownership and flag bit 0, unzoomed (BX == -1) and
// zoomed (levels == 1 and levels > 1) states, and special float constants;
// one region removed injects a fault. Modes: <cases> [census|verify]. Prints a
// JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_zoom_factor.hpp"
using namespace frame_misc_harness;
namespace owner=halo2::native_zoom_factor_detail;
namespace {
constexpr std::uint32_t objects=0x10000000u,headers=0x10001000u,units=0x10002000u,weapons=0x10003000u,
    definitions=0x10004000u,def_bodies=0x10005000u,game_globals=0x10006000u,camera_globals=0x10007000u,scratch=0x10600000u;
void stub(WorldState &s,std::uint32_t target) {
    const std::uint64_t h=entry_hash(s,target);
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,target+4u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(target==0x106040u) {
        s.gp[0]=(s.gp[0]&0xffffff00u)|(rng()%3==0?1u+std::uint32_t(rng()%255):0u);
        if(policy&2u && rng()%11==0)s.gp[3]=(s.gp[3]&0xffff0000u)|(rng()%3==0?0xffffu:std::uint32_t(rng()%4));
        if(policy&2u && rng()%11==0)s.gp[6]=(std::uint32_t(rng())&0xffff0000u)|std::uint32_t(rng()%4);
        if(policy&2u && rng()%11==0)s.gp[7]=objects;
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    for(unsigned i=0;i<2;++i) {
        std::array<std::uint32_t,4> lanes{special_float(rng),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        std::memcpy(&s.xmm[i],lanes.data(),16);
    }
    std::uint32_t cleanup=0u;
    if(policy&2u && target==0x106040u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%2);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
static void run_106160(WorldState &s){stub(s,0x106160u);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool) {
    if(target==0x106040u)stub(s,target);
    else throw std::runtime_error("Unbound vehicle director dispatch target");
}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t fbits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const auto chance=[&](std::uint32_t k){return pick(k)==0u;};
    const auto fill=[&](std::uint32_t from,std::uint32_t size){for(std::uint32_t at=from;at<from+size;at+=4)m.put(at,std::uint32_t(rng()));};
    const unsigned shape=n%16;
    const auto stack=0x20020000u+pick(16)*4u;
    m.put(0x46a0d0u,fbits(1.0f));
    m.put(0x4ecbd0u,objects);
    m.put(0x4f0414u,definitions);
    m.put(0x4ecc1cu,game_globals);
    fill(objects,0x50u);m.put(objects+0x44u,headers);
    for(std::uint32_t k=0;k<4;++k) {
        fill(headers+12u*k,12u);m.put(headers+12u*k+8u,weapons+0x100u*k);
        fill(headers+12u*(0x8000u+k),12u);m.put(headers+12u*(0x8000u+k)+8u,units+0x300u*k);
        fill(headers+12u*(4u+k),12u);m.put(headers+12u*(4u+k)+8u,units+0x300u*k);
        const auto u=units+0x300u*k;
        fill(u,0x300u);
        const std::uint32_t slot=chance(4)?0xffu:pick(4);
        m.put(u+0x212u,slot,1);
        for(std::uint32_t s=0;s<4;++s)
            m.put(u+0x218u+4u*s,chance(4)?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(4));
        const auto w=weapons+0x100u*k;
        fill(w,0x100u);
        m.put(w,(std::uint32_t(rng())&0xffff0000u)|(chance(4)?0x8000u+pick(4):pick(4)));
    }
    for(std::uint32_t k=0;k<4;++k) {
        for(const auto idx:{k,0x8000u+k}) {
            fill(definitions+16u*idx,16u);m.put(definitions+16u*idx+8u,def_bodies+0x200u*k);
        }
        const auto d=def_bodies+0x200u*k;
        fill(d,0x200u);
        m.put(d+0x12eu,chance(3)?(std::uint32_t(rng())|1u):(std::uint32_t(rng())&0xfeu),1);
    }
    fill(game_globals,0x150u);m.put(game_globals+0x134u,camera_globals);
    fill(camera_globals,0xe0u);
    m.put(camera_globals+0xb8u,chance(3)?1u:2u+pick(5));
    std::mt19937_64 frng(rng());
    m.put(camera_globals+0xbcu,chance(4)?special_float(frng):fbits(1.0f+float(pick(4))));
    m.put(camera_globals+0xc0u,chance(4)?special_float(frng):fbits(2.0f+float(pick(8))));
    fill(scratch,0x100u);
    fill(stack-0x80u,0xc0u);
    m.put(stack,0xca000u+pick(0x100));
    random_abi(rng,c,stack);
    c.abi.gp[0]=(std::uint32_t(rng())&0xffff0000u)|(chance(4)?0x8000u+pick(4):4u+pick(4));
    c.abi.gp[3]=(std::uint32_t(rng())&0xffff0000u)|(chance(4)?0xffffu:(chance(6)?0xfffeu:pick(6)));
    if(shape==15) {
        static constexpr std::uint32_t areas[]{objects,headers,units,weapons,definitions,def_bodies,game_globals,camera_globals};
        m.drop(areas[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return owner::bridge(s,[](auto &v){original::native_frame_misc_reference_cad60(v);},
        [](auto &v){original::run_106160(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
void reference(WorldState &s){original::run_cad60(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_ZOOM_FACTOR");
    std::mt19937 rng(0xcad60u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,owner::coverage().load(),unsigned(owner::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
