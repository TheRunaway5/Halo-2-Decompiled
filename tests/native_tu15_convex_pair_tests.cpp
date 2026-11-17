// Native TU1.5 convex pair owners: the 2F8630 closest point (with 2E4550 and
// 2E4760 inline) and the 2E4FE0 swept pair (with 2E4550 inline) against the
// unmodified translated canonical bodies run_2f4bd0 and run_2e14e0, extracted
// by tools/validate_native_convex_pair_tu15.py into pair_bodies.inc (with the
// translated 2E4550 and 2E4760 the references run) with the unit's selected()
// operand cases (pair_selected.inc) and the binder's verification references
// (pair_reference.inc).
//
// The translated children (30E930, 30D350, 283380, 309060, and 2F8630 under
// 2E4FE0) and the virtual targets (the shapes' getType(), 309150) are
// deterministic stand-ins, entered by both routes with the same state. Each
// appends its complete entry state (target, canonical site, general
// registers, flags, x87 depth/status/TOP, an XMM digest) to a log in the
// fixture and reads its stack arguments, so a wrong register, flag, x87 state
// or argument at a call reaches the store stream. They keep EBX, ESI, EDI and
// EBP, and change the scratch registers, flags and XMM registers. 30E930
// writes the separation and answers apart or not; 30D350 writes one of the
// case's planned outputs; 283380 answers from the owner; the cache set-ups
// write the cache; the 2F8630 stand-in reads the settings' byte and both
// sweeps, and answers through the hkBool.
//
// Seeded fixtures: separating planes that stay apart, close or are not
// trusted; outputs whose surfaces' distance is below, at or above the
// tolerance; planes cached or not (the agent's flag, the settings' byte);
// the profiler stream with and without room, and no RDTSC service; swept
// shapes (type 17) or not on either body, parent chains of 0-3 levels, speeds
// around the threshold, the predicate's answer, the previous step's flags,
// shape B of type 5 or not; random and special floats (NaN payloads, SNaN,
// infinities, denormals, signed zeros; NaN payloads in both operands of the
// rotations' products), random register files, x87 depths
// 0-7 (7 overflows the x87 stack, as in the original), an active MMX state,
// every MXCSR mode, an unbound virtual target, and fault injection with one
// fixture page removed. Each case compares the ordered store events (pc,
// address, preimage, bytes), the final bank, registers, flags, XMM, x87,
// MXCSR and the cursor; fault cases compare the exception, the cursor and the
// bank.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace leaves=halo2::native_world_leaves_detail;

namespace t_pair {
struct ObservedWorldState : halo2::physics_world_runtime_detail::WorldState {
    using halo2::physics_world_runtime_detail::WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image) {
    if(image==SourcePhysicsContactImage::MapPack11)return p;
    switch(p) {
#include "pair_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses (the globals at their TU1.5 literals).
inline constexpr std::uint32_t agent=0x10100000u,body_a=0x10110000u,body_b=0x10110100u,parents=0x10110200u,
    motion_base=0x10120000u,shape_a=0x10130000u,shape_b=0x10130100u,vtables=0x10131000u,
    input=0x10140000u,settings=0x10141000u,output=0x10150000u,owner_a=0x10160000u,owner_b=0x10160100u,
    moving_base=0x1016ff00u,plan=0x10170000u,stream=0x10600000u,log_base=0x10900000u,
    fs=0x10700000u,constants=0x46a000u,monitor=0x48c000u;
inline constexpr std::uint32_t type_functions=0x7e0000u,cache_5_target=0x309150u;
// Each case places the motions and the owners' motion objects at varied
// offsets (the registers that carry their addresses, and their flags, are
// part of the comparison; the owners' objects may cross a 64K boundary).
inline std::uint32_t motion_a=motion_base,motion_b=motion_base+0x100u,moving_a=moving_base,moving_b=moving_base+0x100u;
inline void log_entry(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,std::uint32_t extra=0) {
    const auto count=r(s.memory,log_base);
    const auto at=log_base+0x10u+(count%250u)*0x48u;
    std::uint32_t xmm_digest=0;
    for(unsigned i=0;i<8;++i) {
        std::uint32_t lanes[4];std::memcpy(lanes,&s.xmm[i],16);
        for(const auto lane:lanes)xmm_digest=(xmm_digest*0x9e3779b1u)^lane;
    }
    const std::uint32_t words[]{target,site,s.gp[0],s.gp[1],s.gp[2],s.gp[3],s.gp[4],s.gp[5],s.gp[6],s.gp[7],
        s.flags.bits(),std::uint32_t(s.fp.count),std::uint32_t(s.fp.status),std::uint32_t(s.fp.top),xmm_digest,count,extra};
    for(unsigned i=0;i<17;++i)w(s.memory,s.observe,0x7a0000u,at+4u*i,4,words[i]);
    w(s.memory,s.observe,0x7a0004u,log_base,4,count+1u);
}
inline std::uint32_t argument(ObservedWorldState &s,unsigned n){return r(s.memory,s.gp[4]+4u*n);}
inline std::uint32_t fold(ObservedWorldState &s,std::uint32_t seed,std::uint32_t address,unsigned words) {
    for(unsigned i=0;i<words;++i)seed=seed*0x9e3779b1u^r(s.memory,address+4u*i);
    return seed;
}
// Scratch registers, flags and XMM change, as a real callee may.
inline void scratch(ObservedWorldState &s,std::uint32_t seed) {
    s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    for(unsigned i=1;i<8;i+=1+(seed>>i&1u))
        s.xmm[i]=_mm_castsi128_ps(_mm_set_epi32(int(seed*i),int(seed*3u+i),int(seed^0x80000000u),int(seed+7u*i)));
}
// 30E930 (thiscall on the GSK object; shape A, shape B, the relative
// transform, the cache, the separation; RET 14): writes the separation and
// answers non-zero when the shapes are apart.
static void run_30ae40(ObservedWorldState &s) {
    const auto gsk=s.gp[1];
    auto seed=fold(s,gsk^r(s.memory,gsk+0xcu),argument(s,3),16);
    seed=fold(s,seed,argument(s,1),4);seed=fold(s,seed,argument(s,2),4);seed=fold(s,seed,argument(s,4),3);
    log_entry(s,0x30e930u,0,seed);
    const auto separation=argument(s,5);
    for(unsigned i=0;i<4;++i)w(s.memory,s.observe,0x30e940u,separation+4u*i,4,seed*(i+3u));
    s.gp[0]=seed%4u==0u?seed|1u:0u;scratch(s,seed);s.gp[4]+=0x18u;
}
// 30D350 (thiscall on the GSK object; the cache, the separation, the
// output; RET C): one of the case's planned outputs.
static void run_309860(ObservedWorldState &s) {
    const auto seed=fold(s,s.gp[1],argument(s,2),4)^r(s.memory,argument(s,1));
    log_entry(s,0x30d350u,0,seed);
    const auto chosen=plan+(seed>>3)%4u*0x30u,out=argument(s,3);
    for(unsigned i=0;i<12;++i)w(s.memory,s.observe,0x30d360u,out+4u*i,4,r(s.memory,chosen+4u*i));
    s.gp[0]=seed;scratch(s,seed);s.gp[4]+=0x10u;
}
// 283380 (cdecl; the owner): the game's predicate.
static void run_27f920(ObservedWorldState &s) {
    const auto owner=argument(s,1);
    const auto seed=fold(s,owner,owner,2);
    log_entry(s,0x283380u,0,seed);
    s.gp[0]=(seed&0xffffff00u)|((seed>>5)%3u!=0u?1u:0u);scratch(s,seed);s.gp[4]+=4u;
}
// 309060 and 309150 (thiscall on the cache; shape A, shape B, the relative
// transform; RET C): the cache is set up.
inline void set_up_cache(ObservedWorldState &s,std::uint32_t target,std::uint32_t site) {
    const auto cache=s.gp[1];
    auto seed=fold(s,cache^target,argument(s,3),16);
    seed=seed^argument(s,1)*7u^argument(s,2)*13u;
    log_entry(s,target,site,seed);
    for(unsigned i=0;i<3;++i)w(s.memory,s.observe,0x309070u,cache+4u*i,4,seed+i);
    s.gp[0]=seed;scratch(s,seed);s.gp[4]+=0x10u;
}
static void run_305560(ObservedWorldState &s){set_up_cache(s,0x309060u,0);}
// 2F8630 under 2E4FE0 (thiscall on the agent; the hkBool, body A, body B,
// the input, the output; RET 14): reads the settings' byte, the plane and
// both shapes' sweeps, and answers.
static void stand_in_2f8630(ObservedWorldState &s) {
    const auto self=s.gp[1],result=argument(s,1),a=argument(s,2),b=argument(s,3),in=argument(s,4);
    auto seed=fold(s,self,self+0x20u,4)^r(s.memory,r(s.memory,in+0xcu))*0x1000193u;
    seed=fold(s,seed,r(s.memory,a)+0x10u,4);seed=fold(s,seed,r(s.memory,b)+0x10u,4);seed=fold(s,seed,self+0xcu,3);
    log_entry(s,0x2f8630u,0,seed);
    w(s.memory,s.observe,0x2f8640u,result,1,(seed>>7)%3u==0u?0u:(seed>>9)&0xffu);
    w(s.memory,s.observe,0x2f8644u,argument(s,5),4,seed);
    s.gp[0]=result;scratch(s,seed);s.gp[4]+=0x18u;
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    if(target>=type_functions && target<type_functions+0x100u) {
        // shape->getType() (thiscall, RET).
        log_entry(s,target,site,r(s.memory,s.gp[1]+0x20u));
        s.gp[0]=target-type_functions;scratch(s,target*0x2545f491u^s.gp[1]);s.gp[4]+=4u;return;
    }
    if(target==cache_5_target){set_up_cache(s,target,site);return;}
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "pair_bodies.inc"
#include "pair_reference.inc"
} // namespace t_pair
using t_pair::ObservedWorldState;

namespace {
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    std::array<std::byte,4096> &page(std::uint32_t at,bool create) {
        auto found=pages.find(at>>12);
        if(found==pages.end()) {
            if(!create){char t[64];std::snprintf(t,sizeof t,"Access escaped the authored fixture at %x",at);throw std::runtime_error(t);}
            found=pages.emplace(at>>12,std::make_shared<std::array<std::byte,4096>>()).first;
        }
        return *found->second;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(std::size_t n=0;n<out.size();++n)out[n]=const_cast<Memory*>(this)->page(at+std::uint32_t(n),false)[(at+n)&4095u];
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(std::size_t n=0;n<in.size();++n)page(at+std::uint32_t(n),false)[(at+n)&4095u]=in[n];
    }
    void put(std::uint32_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));}
    void put8(std::uint32_t at,std::uint32_t value){page(at,true)[at&4095u]=std::byte(value);}
    void putf(std::uint32_t at,float value){put(at,std::bit_cast<std::uint32_t>(value));}
    void pages_at(std::uint32_t at,unsigned count){for(unsigned n=0;n<count;++n)page(at+n*4096u,true);}
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};
std::uint32_t special(std::mt19937 &rng) {
    static constexpr std::uint32_t values[]{0x3f800000u,0xbf800000u,0u,0x80000000u,0x7fc00000u,0xffc12345u,
        0x7f812345u,0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,0x7f7fffffu};
    if(rng()%3==0)return values[rng()%std::size(values)];
    return std::uint32_t(rng());
}
float small(std::mt19937 &rng,float scale){return (float(rng()%2001u)-1000.0f)/1000.0f*scale;}
constexpr std::uint32_t stack=0x20010000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};bool no_counter{};};
// A random register file, x87 stack and MXCSR mode.
void registers(Case &c,std::mt19937 &rng,std::uint32_t entry,unsigned kind) {
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=entry;c.abi.fs_base=t_pair::fs;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=special(rng);
    c.abi.x87_depth=kind==8?6u+pick(2):pick(4);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=static_cast<long double>(i)+0.5L;
    c.abi.x87_status=std::uint16_t((((8u-c.abi.x87_depth)&7u)<<11)|(pick(2)?0x4100u:0u)|(pick(2)?0x200u:0u));
    if(kind==9) {
        c.abi.x87_depth=0;c.abi.x87_status=0;
        SourcePhysicsMmxAlias alias{};alias.known_mask=std::uint8_t(pick(256));
        c.abi.active_mmx=alias;
    }
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
}
// A transform (three columns, the translation) at address: near a rotation,
// or random.
void transform(Memory &m,std::mt19937 &rng,std::uint32_t address,bool random) {
    for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row) {
        const auto at=address+col*16u+row*4u;
        if(random || rng()%9==0)m.put(at,special(rng));
        else m.putf(at,col==3?small(rng,50.0f):row==col?1.0f:rng()%4?0.0f:small(rng,0.5f));
    }
}
// The common world: bodies, motions, shapes, the input, the constants and
// the profiler stream.
void world(Case &c,std::mt19937 &rng,unsigned kind) {
    using namespace t_pair;
    auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    motion_a=motion_base+4u*pick(32);motion_b=motion_base+0x200u+4u*pick(32);
    moving_a=moving_base+4u*pick(64);moving_b=moving_base+0x300u+4u*pick(64);
    m.pages_at(motion_base,1);m.pages_at(moving_base,2);
    m.pages_at(agent,1);m.pages_at(body_a,1);m.pages_at(shape_a,2);m.pages_at(input,2);
    m.pages_at(output,1);m.pages_at(owner_a,1);m.pages_at(plan,1);m.pages_at(log_base,5);m.pages_at(constants,1);
    m.pages_at(monitor,1);m.pages_at(stream,1);
    for(std::uint32_t at=0;at<0x100u;at+=4)m.put(agent+at,special(rng));
    for(const auto motion:{motion_a,motion_b}) {
        for(unsigned k=0;k<2;++k)for(unsigned i=0;i<4;++i)m.putf(motion+k*16u+4u*i,kind==10?std::bit_cast<float>(special(rng)):small(rng,20.0f));
        m.putf(motion+0xcu,pick(4)?std::abs(small(rng,0.2f)):std::bit_cast<float>(special(rng)));  // the movement bound
        transform(m,rng,motion+0x20u,kind==10);
    }
    m.put(body_a,shape_a);m.put(body_a+4u,std::uint32_t(rng()));m.put(body_a+8u,motion_a);m.put(body_a+12u,0u);
    m.put(body_b,shape_b);m.put(body_b+4u,std::uint32_t(rng()));m.put(body_b+8u,pick(5)?motion_b:motion_a);m.put(body_b+12u,0u);
    for(const auto shape:{shape_a,shape_b}) {
        for(std::uint32_t at=0;at<0x30u;at+=4)m.put(shape+at,special(rng));
        m.putf(shape+0xcu,pick(5)?std::abs(small(rng,0.5f)):std::bit_cast<float>(special(rng)));
    }
    for(std::uint32_t at=0;at<0x20u;at+=4)m.put(input+at,special(rng));
    m.put(input+0xcu,settings);m.put8(settings,pick(3)?1u:0u);
    m.putf(input+8u,pick(6)?0.05f+small(rng,0.04f):std::bit_cast<float>(special(rng)));
    m.putf(input+0x10u,pick(6)?1.0f/30.0f:std::bit_cast<float>(special(rng)));
    m.putf(0x46a0e8u,pick(4)?0.0f:pick(2)?0.01f:std::bit_cast<float>(special(rng)));  // the plane's bound
    m.putf(0x46a190u,pick(4)?0.5f:std::bit_cast<float>(special(rng)));                // the cached tolerance fraction
    const bool room=kind!=12 && pick(4)!=0;
    const auto next=stream+0x10u*pick(8);
    m.put(0x48c974u,next);m.put(0x48c978u,room?next+0x100u:next-pick(2)*12u);
    m.put(t_pair::fs,0x2001ff00u);
}

// ---- 2F8630
Case author_closest(std::mt19937 &rng,unsigned n) {
    using namespace t_pair;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;c.no_counter=kind==11;
    world(c,rng,kind);
    // The separating plane: a unit normal and a distance that is not trusted
    // (at or below the bound), stays apart, or closes under the movement.
    const auto axis=pick(3);
    for(unsigned i=0;i<3;++i)m.putf(agent+0x20u+4u*i,i==axis?(pick(2)?1.0f:-1.0f):kind==10?std::bit_cast<float>(special(rng)):0.0f);
    const float distance=kind==6?-small(rng,0.1f)*float(pick(2)):kind==4?5.0f+small(rng,1.0f):kind==5?0.01f+std::abs(small(rng,0.02f)):
        pick(3)?small(rng,2.0f):std::bit_cast<float>(special(rng));
    m.putf(agent+0x2cu,distance);
    if(kind==3) {
        m.put(0x46a0e8u,0x3f800001u);m.put(agent+0x2cu,0x3f800002u);  // the bound 1 + 2^-23, the plane 1 + 2^-22
        m.put(body_b+8u,motion_b);
        for(const auto motion:{motion_a,motion_b})for(unsigned i=0;i<4;++i)m.put(motion+0x10u+4u*i,r(m,motion+4u*i));
        m.put(motion_a+0xcu,0x34000000u);m.put(motion_b+0xcu,0xa1800000u);m.put(motion_b+0x1cu,0xa1800000u);  // 2^-23, -2^-60
        for(unsigned i=0;i<3;++i)m.putf(agent+0x20u+4u*i,i==axis?1.0f:0.0f);
    }
    m.put(agent+0x18u,kind==13?1u+pick(4):pick(6)?0u:std::uint32_t(rng()));
    if(kind==14)m.put8(settings,0u);
    // The planned outputs of 30D350: a unit normal, a point and the cores'
    // distance around the tolerance plus both radii.
    const float radii=std::bit_cast<float>(r(m,shape_a+0xcu))+std::bit_cast<float>(r(m,shape_b+0xcu));
    const float tolerance=std::bit_cast<float>(r(m,input+8u));
    for(unsigned k=0;k<4;++k) {
        const auto p=plan+k*0x30u;
        for(unsigned i=0;i<12;++i)m.put(p+4u*i,special(rng));
        if(pick(5)==0 && kind!=7)continue;
        const auto along=pick(3);
        for(unsigned i=0;i<4;++i)m.putf(p+4u*i,i==along?(pick(2)?1.0f:-1.0f):i==3?small(rng,1.0f):0.0f);
        for(unsigned i=0;i<4;++i)m.putf(p+0x10u+4u*i,small(rng,3.0f));
        const float offsets[]{-0.2f,-0.01f,0.0f,0.01f,0.3f};
        const float core=kind==7?radii+tolerance*0.5f:radii+tolerance+offsets[pick(5)]*(pick(3)?1.0f:0.0f);
        m.putf(p+0x20u,pick(8)?core:std::bit_cast<float>(special(rng)));
    }
    // NaN payloads in both operands of every product of the rotations (A's
    // columns and the planned normals): the operand order decides the result.
    if(kind==2) {
        for(unsigned i=0;i<16;++i)m.put(motion_a+0x20u+4u*i,0x7fc00000u|(std::uint32_t(rng())&0x3fffffu));
        for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)m.put(plan+k*0x30u+4u*i,0xffc00000u|(std::uint32_t(rng())&0x3fffffu));
    }
    // The stack: the return address and five arguments, random below.
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x2e52edu);m.put(entry+4u,stack-0x400u+pick(64));m.put(entry+8u,body_a);m.put(entry+12u,body_b);
    m.put(entry+16u,input);m.put(entry+20u,output);
    registers(c,rng,entry,kind);
    c.abi.gp[1]=agent;
    if(kind==15) {
        static constexpr std::uint32_t regions[]{agent,body_a,motion_base,shape_a,input,settings,output,constants,monitor,stream,
            plan,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    return c;
}

// ---- 2E4FE0
Case author_swept(std::mt19937 &rng,unsigned n) {
    using namespace t_pair;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    world(c,rng,kind);
    // The shapes' types through their vtables: swept (17), 5 or another.
    const auto type_of=[&](bool b){
        if(kind==11)return 0x17u;
        if(kind==12)return 1u;
        const auto k=pick(5);return k<2?0x17u:k==2?5u:k==3?(b?5u:1u):std::uint32_t(pick(0x30));
    };
    const std::uint32_t types[2]{type_of(false),type_of(true)};
    for(unsigned k=0;k<2;++k) {
        const auto vtable=vtables+k*0x100u,shape=k?shape_b:shape_a;
        for(std::uint32_t at=0;at<0x40u;at+=4)m.put(vtable+at,std::uint32_t(rng()));
        m.put(vtable+0x14u,kind==13 && k==pick(2)?0x7f0000u:type_functions+types[k]);
        m.put(shape,vtable);
        // The sweep (overwritten) and the squared speed threshold.
        m.putf(shape+0x20u,pick(6)?std::abs(small(rng,4.0f)):std::bit_cast<float>(special(rng)));
    }
    // Parent chains of 0-3 levels to the collidable, whose owner moves.
    for(unsigned k=0;k<2;++k) {
        const auto body=k?body_b:body_a;
        auto at=body;
        const auto levels=pick(4);
        for(unsigned level=0;level<levels;++level) {
            const auto parent=parents+(k*4u+level)*0x40u;
            m.put(at+12u,parent);
            for(std::uint32_t i=0;i<0x40u;i+=4)m.put(parent+i,std::uint32_t(rng()));
            m.put(parent+12u,0u);
            at=parent;
        }
        const auto owner=k?owner_b:owner_a,moving=k?moving_b:moving_a;
        m.put(at+0x20u,owner);
        for(std::uint32_t i=0;i<0x40u;i+=4)m.put(owner+i,std::uint32_t(rng()));
        m.put(owner+0x3cu,moving);
        for(unsigned i=0;i<4;++i)m.putf(moving+0x40u+4u*i,pick(5)?small(rng,3.0f):std::bit_cast<float>(special(rng)));
    }
    // The agent: the previous step's flags, its plane and cache.
    m.put(agent+0xf0u,pick(4)?pick(4):std::uint32_t(rng()));
    // The stack: the return address and five arguments, random below.
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x2e5c00u);m.put(entry+4u,stack-0x400u+pick(64));m.put(entry+8u,body_a);m.put(entry+12u,body_b);
    m.put(entry+16u,input);m.put(entry+20u,output);
    registers(c,rng,entry,kind);
    c.abi.gp[1]=agent;
    if(kind==15) {
        static constexpr std::uint32_t regions[]{agent,body_a,motion_base,shape_a,vtables,input,settings,owner_a,moving_base,
            moving_base+0x1000u,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
template<class Native,class Translated> Result execute(Memory &memory,const Case &c,bool is_native,Native native,Translated translated) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    std::uint64_t counter=0x123456789abcull;
    if(!c.no_counter)services.timestamp_counter=[&counter]{counter+=0x10203u;return counter;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native convex pair owner delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try{s.load(c.abi);}catch(const std::exception &e){result.fault=e.what();return result;}
    s.selected_observation_pcs=true;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        if(is_native)result.admitted=native(s);else translated(s);
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.fs_base==b.fs_base && a.active_mmx.has_value()==b.active_mmx.has_value();
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    return same;
}
struct Totals {std::uint64_t compared{},stores{},faults{},children{};};
template<class Author,class Native,class Translated>
int compare(const char *owner,unsigned cases,std::uint32_t seed,Totals &totals,Author author,Native native,Translated translated) {
    std::mt19937 rng(seed);
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true,native,translated);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"%s case %u refused\n",owner,n);return 1;}
        const auto reference=execute(left,c,false,native,translated);
        ++totals.compared;totals.stores+=reference.stores.size();
        for(const auto &e:reference.stores)totals.children+=e.instruction==0x7a0004u;
        if(!reference.fault.empty())++totals.faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"%s case %u (kind %u) fault mismatch: native '%s' translated '%s'\n",owner,n,c.kind,
                native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"%s case %u (kind %u) store mismatch at %zu of %zu/%zu",owner,n,c.kind,at,native_result.stores.size(),
                reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size()) {
                const auto &a=native_result.stores[at],&b=reference.stores[at];
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",a.instruction,a.address,b.instruction,b.address);
                if(a.instruction==b.instruction && a.address==b.address && a.written!=b.written) {
                    std::fprintf(stderr," bytes");
                    for(std::size_t i=0;i<a.written.size();++i)std::fprintf(stderr," %02x/%02x",unsigned(a.written[i]),unsigned(b.written[i]));
                }
            } else if(at<reference.stores.size())std::fprintf(stderr," original pc=%x",reference.stores[at].instruction);
            else if(at<native_result.stores.size())std::fprintf(stderr," native pc=%x",native_result.stores[at].instruction);
            std::fprintf(stderr," (%s)\n",reference.fault.c_str());return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"%s case %u (kind %u) cursor %x vs %x (%s)\n",owner,n,c.kind,native_result.cursor,reference.cursor,
                reference.fault.c_str());
            return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || native_result.mxcsr!=reference.mxcsr || !left.same(right))) {
            std::fprintf(stderr,"%s case %u (kind %u) mismatch:%s%s%s",owner,n,c.kind,native_result.mxcsr!=reference.mxcsr?" mxcsr":"",
                left.same(right)?"":" bank",native_result.abi.eflags!=reference.abi.eflags?" eflags":"");
            if(native_result.abi.eflags!=reference.abi.eflags)std::fprintf(stderr," %x/%x",native_result.abi.eflags,reference.abi.eflags);
            for(unsigned i=0;i<8;++i)if(native_result.abi.gp[i]!=reference.abi.gp[i])
                std::fprintf(stderr," gp%u %x/%x",i,native_result.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native_result.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr," xmm%u",i);
            if(native_result.abi.x87_status!=reference.abi.x87_status || native_result.abi.x87_depth!=reference.abi.x87_depth)
                std::fprintf(stderr," x87 %x/%x depth %u/%u",native_result.abi.x87_status,reference.abi.x87_status,
                    native_result.abi.x87_depth,reference.abi.x87_depth);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"%s case %u fault bank mismatch\n",owner,n);return 1;}
    }
    return 0;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    const std::uint32_t variant=(census_mode?1u:0u)+(verify?2u:0u);
    Totals closest;
    if(compare("2F8630",cases,0x2f8630u+variant,closest,author_closest,
        [](ObservedWorldState &s){return leaves::bridge_2f8630(s,[](auto &v){t_pair::native_world_leaves_reference_2f4bd0(v);},
            [](auto &v){t_pair::run_30ae40(v);},[](auto &v){t_pair::run_309860(v);});},
        [](ObservedWorldState &s){t_pair::run_2f4bd0(s);}))return 1;
    Totals swept;
    if(compare("2E4FE0",cases,0x2e4fe0u+variant,swept,author_swept,
        [](ObservedWorldState &s){return leaves::bridge_2e4fe0(s,[](auto &v){t_pair::native_world_leaves_reference_2e14e0(v);},
            [](auto &v){t_pair::run_27f920(v);},[](auto &v){t_pair::run_305560(v);},[](auto &v){t_pair::stand_in_2f8630(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_pair::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_pair::run_2e14e0(s);}))return 1;
    const auto owner=[](const Totals &t){
        char text[160];std::snprintf(text,sizeof text,"{\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"child_calls\":%llu}",
            (unsigned long long)t.compared,(unsigned long long)t.faults,(unsigned long long)t.stores,(unsigned long long)t.children);
        return std::string(text);
    };
    std::printf("{\"cases\":%u,\"closest\":%s,\"swept\":%s,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"mode\":\"%s\"}\n",
        cases,owner(closest).c_str(),owner(swept).c_str(),(unsigned long long)(closest.compared+swept.compared),
        (unsigned long long)(closest.stores+swept.stores),(unsigned long long)(closest.faults+swept.faults),
        (unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
