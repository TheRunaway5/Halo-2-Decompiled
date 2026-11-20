// Native TU1.5 atmospheric fog mixers (131610) against the unmodified
// translated body of the campaign environment unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// The child (the interpolator evaluation 137AC0) is a genuine stand-in whose
// effects are a hash of its complete entry ABI, its return address and its
// two argument words: XMM0 (weights below 0, within [0, 1], above 1, NaN
// payloads, random upper lanes), EAX/ECX/EDX, flags, other XMM registers,
// MXCSR sticky bits, scratch stores and stores into the mixer's frame (the
// weights, the remaining share, the cursors), occasional callee-saved
// clobbers (the scenario and entry registers) and stack imbalance. Fixtures
// author the scenario (atmospheric fog palette, interpolator count), the
// structure BSP's clusters (fog index -1, negative, beyond, valid), palette
// entries with mixers (count 0, negative, 1..3; source and interpolator
// indices negative, beyond and valid), interpolator states (including a
// null state), the unit weight constant, the list and output arrays, the
// list length in EDX (zero and negative), the direction flag (REP STOSD and
// MOVSD stepping downward), plus fault injection (one region removed).
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_fog_mixers.hpp"
using namespace frame_misc_harness;
namespace fog=halo2::native_fog_mixers_detail;
namespace {
constexpr std::uint32_t scenario=0x10000000u,palette=0x10100000u,mixers=0x10200000u,bsp=0x10300000u,clusters=0x10310000u,
    interpolators=0x10400000u,list=0x10500000u,output=0x10600000u,scratch=0x10700000u,stack=0x20010000u;
constexpr std::uint32_t palette_count=12u,cluster_count=24u,interpolator_count=8u;
std::uint32_t quiet_nan(std::mt19937_64 &rng) {
    static constexpr std::uint32_t payloads[]{0x7fc00001u,0xffc00000u,0x7fc12345u,0xffc54321u};
    return payloads[rng()%4];
}
void stub(WorldState &s,std::uint32_t target) {
    std::uint64_t h=entry_hash(s,target);
    for(unsigned k=0;k<=2;++k)h=mix(h,r(s.memory,s.gp[4]+4u*k));
    log_call(h);
    std::mt19937_64 rng(h);
    w(s.memory,s.observe,0x137ac1u,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(rng()%5==0) {
        // The caller's frame: weights, the remaining share, the cursors, or a field size.
        const auto frame=s.gp[4]+12u;
        static constexpr std::uint32_t slots[]{0x2cu,0x38u,0x3cu,0x24u,0x10u,0x14u,0x28u,0x44u};
        const auto slot=slots[rng()%8];
        std::uint32_t value=std::bit_cast<std::uint32_t>(float(int(rng()%21)-5)/10.0f);
        if(slot==0x10u)value=std::uint32_t(rng()%3)*0x10u;
        if(slot==0x14u)value=std::uint32_t(rng()%3);
        if(slot==0x28u)value=palette+std::uint32_t(rng()%palette_count)*0xf4u;
        if(slot==0x44u) {
            const auto field_slot=0x44u+std::uint32_t(rng()%18)*8u;
            value=rng()%2?0u:0xfffffffcu;
            w(s.memory,s.observe,0x137ac2u,frame+field_slot,4,value);
        } else {
            w(s.memory,s.observe,0x137ac2u,frame+slot,4,value);
        }
    }
    if(rng()%17==0) {
        // Occasionally shrink the palette count or invalidate a mixer source after weighing.
        const auto m_idx=std::uint32_t(rng()%palette_count);
        w(s.memory,s.observe,0x137ac3u,mixers+m_idx*0x40u+0xcu,2,rng()%2?0xffffu:palette_count+1u);
    }
    std::uint32_t weight=std::bit_cast<std::uint32_t>(float(int(rng()%31)-10)/20.0f);
    const auto roll=rng()%10;
    if(roll==0)weight=quiet_nan(rng);
    else if(roll==1)weight=0x80000000u;
    else if(roll==2)weight=0x3f800000u;
    std::array<std::uint32_t,4> lanes{weight,std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[0],lanes.data(),16);
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&2u && rng()%23==0) {
        static constexpr unsigned saved[]{3,6,7};
        const auto reg=saved[rng()%3];
        s.gp[reg]=reg==3?palette+std::uint32_t(rng()%palette_count)*0xf4u:reg==7?(rng()%2?scenario:0u):std::uint32_t(rng());
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    if(policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    std::array<std::uint32_t,4> other{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
    std::memcpy(&s.xmm[1+rng()%7],other.data(),16);
    std::uint32_t cleanup=8u;
    if(policy&2u && rng()%37==0)cleanup=4u*std::uint32_t(rng()%4);
    s.gp[4]+=4u+cleanup;
}
}
namespace original {
using namespace halo2::physics_world_runtime_detail;
static void run_137ac0(WorldState &s){stub(s,0x137ac0u);}
#include "body.inc"
#include "reference.inc"
}
namespace {
std::uint32_t field_value(std::mt19937 &rng,unsigned shape) {
    if(shape==3 && rng()%4==0) {
        static constexpr std::uint32_t special[]{0x7fc00001u,0xffc00000u,0x7f800000u,0x00000001u,0x80000000u};
        return special[rng()%5];
    }
    return std::bit_cast<std::uint32_t>(float(int(rng()%2001)-1000)/64.0f);
}
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    m.put(0x4ecc20u,shape==1?0u:scenario);m.put(0x4ecc18u,bsp);m.put(0x4f3010u,interpolators);
    m.put(0x46a0d0u,shape==3&&pick(3)==0?field_value(rng,3):0x3f800000u);
    for(std::uint32_t at=scenario;at<scenario+0x3d0u;at+=4)m.put(at,std::uint32_t(rng()));
    std::uint32_t count=palette_count-pick(3);
    if(shape==4)count=pick(2)?0u:0x80000000u|pick(4);
    m.put(scenario+0x340u,count);m.put(scenario+0x344u,palette);
    m.put(scenario+0x3c0u,shape==5?pick(2):interpolator_count-pick(3));
    // Palette entries and their mixers.
    for(std::uint32_t k=0;k<palette_count+2u;++k) {
        const auto e=palette+k*0xf4u;
        for(std::uint32_t at=0;at<0xf4u;at+=4)m.put(e+at,field_value(rng,shape));
        std::uint32_t mixer_total=pick(4);
        if(pick(9)==0)mixer_total=0x80000000u|pick(3);
        if(shape==6)mixer_total=1u+pick(3);
        m.put(e+0xd8u,mixer_total);m.put(e+0xdcu,mixers+k*0x40u);
        for(std::uint32_t j=0;j<4;++j) {
            const auto x=mixers+k*0x40u+j*0x10u;
            for(std::uint32_t at=0;at<0x10u;at+=4)m.put(x+at,std::uint32_t(rng()));
            std::uint32_t source=pick(palette_count);
            if(pick(8)==0)source=pick(2)?0x8000u|pick(4):palette_count+pick(4);
            std::uint32_t interpolator=pick(interpolator_count);
            if(pick(8)==0)interpolator=pick(2)?0x8000u|pick(4):interpolator_count+pick(4);
            m.put(x+0xcu,source,2);m.put(x+0xeu,interpolator,2);
        }
    }
    if(shape==7)m.put(scenario+0x344u,0u-pick(palette_count)*0xf4u);   // the entry at 0
    if(shape==9)for(std::uint32_t at=0;at<0xf4u;at+=4)m.put(at,field_value(rng,shape)); // non-faulting path for source_missing
    for(std::uint32_t at=interpolators;at<interpolators+(interpolator_count+4u)*0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    if(shape==8)m.put(0x4f3010u,0u-4u-pick(interpolator_count)*0x20u);  // a null interpolator state
    // The structure BSP's clusters and the list.
    m.put(bsp+0xa0u,clusters);
    for(std::uint32_t at=bsp;at<bsp+0xa0u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<cluster_count;++k) {
        for(std::uint32_t at=0;at<0xb0u;at+=4)m.put(clusters+k*0xb0u+at,std::uint32_t(rng()));
        std::uint32_t fog=pick(palette_count);
        const auto roll=pick(10);
        if(roll==0)fog=0xffu;
        else if(roll==1)fog=0x80u|pick(0x7f);
        else if(roll==2)fog=palette_count+pick(3);
        m.put(clusters+k*0xb0u+0x6fu,fog,1);
    }
    std::uint32_t length=1u+pick(6);
    if(shape==2)length=pick(2)?0u:0x80000000u|pick(4);
    for(std::uint32_t k=0;k<8;++k) {
        for(std::uint32_t at=0;at<0x18u;at+=4)m.put(list+k*0x18u+at,std::uint32_t(rng()));
        m.put(list+k*0x18u,pick(cluster_count));
    }
    for(std::uint32_t at=output-0x100u;at<output+8u*0xf4u+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x3000u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    // The return address and the arguments, at either stack alignment.
    const auto top=stack-4u*pick(2);
    m.put(top,0x131b16u);m.put(top+4u,list);m.put(top+8u,output);
    random_abi(rng,c,top);
    c.abi.gp[2]=length;
    if(shape==15) {
        static constexpr std::uint32_t regions[]{scenario,palette,mixers,bsp,clusters,interpolators,list,output};
        m.drop(regions[pick(8)]);
    }
    return c;
}
bool native(WorldState &s) {
    return fog::bridge(s,[](auto &v){original::native_frame_misc_reference_131610(v);},[](auto &v){original::run_137ac0(v);});
}
void reference(WorldState &s){original::run_131610(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_FOG_MIXERS");
    std::mt19937 rng(0x131610u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n)
        if(!compare(n,author(rng,n),native,reference,t))return 1;
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,fog::coverage().load(),unsigned(fog::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
