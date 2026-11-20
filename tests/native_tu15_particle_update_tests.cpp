// Native TU1.5 particle location update (251A60 with 250450 inline) against
// the unmodified translated bodies of the particle frame unit, extracted by
// tools/validate_native_effect_owners_tu15.py into particle.inc, plus the
// verification references the production binder generates
// (particle_references.inc). The children are genuine stand-ins: 13A6B0 (the
// particle definition interface), 251650 (a particle's release), 16E8A0 (its
// deletion), 2C40D0 (the emitter's physics), 2516C0 (an emitter's particles
// released) and the interface methods reached through the unit's dispatch
// (+34 a resting particle's check, +50 the tag selection, +4C the radius
// scale in ST(0)). Each stand-in's results (registers, flags, XMM, MXCSR
// sticky bits, x87 push, one store) are a hash of its complete entry ABI (GP,
// flags, XMM, x87 depth/status, MXCSR) and of the memory it sees, so any entry
// difference diverges.
// Seeded fixtures author complete locations: emitter chains, particle chains
// with every flag combination, ages around 1 with NaN payloads, infinities,
// denormals and -0, landed particles around the rest speed, system and
// definition flags, size functions (unit range, lower > upper, NaN), the
// frame stamp around the location's drawn frame, the three tag groups,
// x87 depths 0..7 with any precision/rounding control, every MXCSR mode, and
// fault injection (one fixture page removed). Each case compares ordered
// stores (pc, address, preimage, bytes), the child log, bank, registers,
// flags, XMM, x87 lanes/status, MXCSR and the cursor; faults compare the
// exception, cursor and store prefix.
// Modes: <cases> [census|verify|reference]. Prints a JSON summary with
// branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "rendering/effects/native_tu15_particle_update.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <random>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace pu=halo2::native_particle_update_detail;
namespace {
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}
constexpr std::uint32_t location=0x16000000u,system_record=0x16100000u,definition=0x16200000u,emitter_definitions=0x16300000u,
    functions=0x16400000u,emitter_array=0x16500000u,emitter_data=0x16600000u,particle_array=0x16700000u,
    particle_data=0x16800000u,tags=0x13000000u,physics=0x13800000u,vtables=0x17000000u,scratch=0x14000000u,
    stack=0x20010000u;
constexpr std::uint32_t rest_method=0x00a00034u,radius_method=0x00a0004cu,select_method=0x00a00050u;
std::vector<std::uint64_t> *call_log{};
struct Child {std::uint32_t policy{},interface{};} child;
std::uint32_t special(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0xff800005u,0x7f800000u,0xff800000u,
        0x00000001u,0x807fffffu,0x80000000u,0x00000000u,0x3f800000u,0x447a0000u,0x00800000u,0x7f7fffffu};
    return values[rng()%14];
}
std::uint64_t entry_hash(WorldState &s,std::uint32_t tag) {
    std::uint64_t h=tag;
    for(const auto v:s.gp)h=mix(h,v);
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,_mm_getcsr());h=mix(h,s.fp.count);h=mix(h,s.fp.status);
    for(unsigned i=0;i<s.fp.count;++i){std::uint64_t lane[2]{};std::memcpy(lane,&s.fp.lanes[i],10);h=mix(mix(h,lane[0]),lane[1]);}
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    return h;
}
std::mt19937_64 enter(WorldState &s,std::uint32_t tag,std::uint32_t back,std::initializer_list<std::uint32_t> words) {
    if(r(s.memory,s.gp[4])!=back){char t[96];std::snprintf(t,sizeof t,"%x entered without its return address %x",tag,back);
        throw std::runtime_error(t);}
    auto h=entry_hash(s,tag);
    for(const auto a:words)h=mix(h,r(s.memory,a));
    if(call_log) {
        if(call_log->size()>=256)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(h);
    }
    return std::mt19937_64(h);
}
// Results a child leaves: scratch store, caller-saved registers, flags,
// occasionally callee-saved registers (policy 2), XMM lanes (policy 4) and
// MXCSR sticky bits (policy 1).
void leave(WorldState &s,std::mt19937_64 &rng,std::uint32_t pc,std::uint32_t pops) {
    w(s.memory,s.observe,pc,scratch+std::uint32_t(rng()%32)*4u,4,std::uint32_t(rng()));
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(child.policy&2u && rng()%97==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng()%5);}
    if(child.policy&4u)for(unsigned i=0;i<8;++i)if(rng()%3==0) {
        std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng()),std::uint32_t(rng())};
        if(rng()%2)lanes[1]=lanes[2]=lanes[3]=0;
        std::memcpy(&s.xmm[i],lanes.data(),16);
    }
    if(child.policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    s.gp[4]+=4u+pops;
}
// 13A6B0: EDX the particle tag; EAX the interface.
void stub_13a6b0(WorldState &s) {
    auto rng=enter(s,0x13a6b0u,0x25048du,{});
    const auto interface=child.interface?child.interface:(rng()%2?0x486048u:0x48603cu);
    leave(s,rng,0x13a6b1u,0);
    s.gp[0]=rng()%97==0?0u:interface;
}
// 251650: EAX the system, EBX the particle, [ESP+4] the emitter; RET 4.
void stub_251650(WorldState &s) {
    auto rng=enter(s,0x251650u,0x2506f8u,{s.gp[3]+4u});
    leave(s,rng,0x251651u,4);
}
// 16E8A0: EDX the array, EAX the handle; RET.
void stub_16e8a0(WorldState &s) {
    auto rng=enter(s,0x16e8a0u,0x250705u,{s.gp[2]+0x44u});
    leave(s,rng,0x16e8a1u,0);
}
// 2C40D0: EAX the first particle, EDI the physics; three arguments; RET C.
void stub_2c40d0(WorldState &s) {
    auto rng=enter(s,0x2c40d0u,0x25075cu,{s.gp[4]+4u,s.gp[4]+8u,s.gp[4]+12u});
    leave(s,rng,0x2c40d1u,12);
}
// 2516C0: EDI the emitter; RET.
void stub_2516c0(WorldState &s) {
    auto rng=enter(s,0x2516c0u,0x251bb7u,{s.gp[7]+4u});
    leave(s,rng,0x2516c1u,0);
}
// The interface methods, through the unit's dispatch.
void stub_method(WorldState &s,std::uint32_t target,std::uint32_t site) {
    if(site==0x250630u && target==rest_method) {
        auto rng=enter(s,target,0x250633u,{s.gp[1]});
        leave(s,rng,0x250631u,0);
        s.gp[0]=(s.gp[0]&~0xffu)|(rng()%3?0u:std::uint32_t(rng()%256));
    } else if(site==0x251c3fu && target==select_method) {
        auto rng=enter(s,target,0x251c42u,{s.gp[4]+4u});
        leave(s,rng,0x251c40u,4);
    } else if(site==0x251c46u && target==radius_method) {
        auto rng=enter(s,target,0x251c49u,{});
        leave(s,rng,0x251c47u,0);
        if(!(child.policy&8u) || rng()%5) {
            const auto bits=rng()%9==0?special(rng):std::bit_cast<std::uint32_t>(float(int(rng()%201))/40.0f);
            s.fp.push((long double)std::bit_cast<float>(bits)/((rng()%4)?1.0L:3.0L));
        }
    } else throw std::runtime_error("Unbound particle frame dispatch target");
}
}
namespace original {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static void run_13a6b0(WorldState &s){stub_13a6b0(s);}
static void run_251650(WorldState &s){stub_251650(s);}
static void run_16e8a0(WorldState &s){stub_16e8a0(s);}
static void run_2c40d0(WorldState &s){stub_2c40d0(s);}
static void run_2516c0(WorldState &s){stub_2516c0(s);}
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool){stub_method(s,target,site);}
static void run_250450(WorldState &);
#include "particle.inc"
#include "particle_references.inc"
}
namespace {
std::uint32_t small_float(std::mt19937 &rng,int range=40,float scale=10.0f) {
    return std::bit_cast<std::uint32_t>(float(int(rng()%unsigned(2*range+1))-range)/scale);
}
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
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));
    }
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{};Child child{};};

Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 wide(rng());
    const unsigned shape=n%23;
    c.child.policy=pick(16);
    const auto value=[&](int range,float scale){return pick(13)==0?special(wide):small_float(rng,range,scale);};
    // Constants (46A0CC 0.5, 46A0D0 1, 46A0E8 0, 46A4B0 1/16), occasionally special.
    m.put(0x46a0ccu,pick(17)==0?special(wide):0x3f000000u);
    m.put(0x46a0d0u,pick(17)==0?special(wide):0x3f800000u);
    m.put(0x46a0e8u,pick(17)==0?special(wide):0u);
    m.put(0x46a4b0u,pick(17)==0?special(wide):0x3d800000u);
    // The frame stamp against the location's drawn frame.
    const std::int32_t stamp=std::int32_t(pick(1000))-(pick(9)==0?0x7fffffff:0);
    m.put(0x4c6854u,std::uint32_t(stamp));
    static constexpr int drawn_offsets[]{0,-1,-2,-3,1,-100};
    // Tag instances: group at +0, the definition at +8.
    m.put(0x4f0414u,tags);
    static constexpr std::uint32_t groups[]{0x5052544du,0x70727433u,0x5052544du,0x70727433u,0x65666665u,0x5052544du,0x70727433u,0u};
    for(std::uint32_t k=0;k<8;++k) {
        m.put(tags+k*16u,shape==5?groups[4]:groups[k]);m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,physics+k*0x40u);m.put(tags+k*16u+12u,std::uint32_t(rng()));
        for(std::uint32_t o=0;o<0x40u;o+=4)m.put(physics+k*0x40u+o,std::uint32_t(rng()));
    }
    // Negative tag indices (the sign-extended group read) land below the table.
    for(std::uint32_t k=0x8000u;k<0x8010u;++k)m.put(tags+std::uint32_t(std::int32_t(std::int16_t(k)))*16u,
        pick(3)?groups[pick(8)]:std::uint32_t(rng()));
    // The two interfaces and their vtables.
    for(const auto [interface,table]:{std::pair{0x486048u,vtables},std::pair{0x48603cu,vtables+0x100u}}) {
        m.put(interface,table);m.put(interface+4u,std::uint32_t(rng()));m.put(interface+8u,std::uint32_t(rng()));
        for(std::uint32_t o=0;o<0x60u;o+=4)m.put(table+o,std::uint32_t(rng()));
        m.put(table+0x34u,rest_method);m.put(table+0x4cu,radius_method);m.put(table+0x50u,select_method);
    }
    if(shape==5 && pick(2)==0)m.put(0u,vtables);            // non-faulting path when interface==0 (lu_other)
    if(shape==9)m.put(vtables+0x34u,0x00a00099u);          // an unbound method
    c.child.interface=pick(4)==0?(pick(2)?0x486048u:0x48603cu):0u;
    // Data arrays: emitters (76 bytes) and particles (64 bytes).
    m.put(0x52d718u,emitter_array);m.put(0x52d714u,particle_array);
    for(std::uint32_t o=0;o<0x50u;o+=4){m.put(emitter_array+o,std::uint32_t(rng()));m.put(particle_array+o,std::uint32_t(rng()));}
    m.put(emitter_array+0x44u,emitter_data);m.put(particle_array+0x44u,particle_data);
    // The system and its definition.
    for(std::uint32_t o=0;o<0x54u;o+=4)m.put(system_record+o,std::uint32_t(rng()));
    m.put(system_record+0xcu,pick(6)?(pick(256)|8u):pick(256),1);
    for(std::uint32_t o=0;o<0x38u;o+=4)m.put(definition+o,std::uint32_t(rng()));
    static constexpr std::uint32_t known_groups[]{0,1,2,3,5,6};
    const std::uint32_t tag_index=shape==6?0x8000u+pick(16):pick(41)==0?(pick(2)?4u:7u):known_groups[pick(6)];
    m.put(definition+4u,(pick(0x10000u)<<16)|tag_index);
    m.put(definition+0x16u,pick(256),1);m.put(definition+0x17u,pick(256),1);
    const unsigned emitters=shape==1?0:1+pick(4);
    m.put(definition+0x30u,shape==2?std::uint32_t(-1-int(pick(3))):std::uint32_t(int(emitters)-int(pick(3))+1));
    m.put(definition+0x34u,emitter_definitions);
    for(unsigned k=0;k<6;++k) {
        const auto at=emitter_definitions+k*0xb8u;
        for(std::uint32_t o=0;o<0xb8u;o+=4)m.put(at+o,std::uint32_t(rng()));
        m.put(at+4u,pick(3)==0?0xffffffffu:(pick(0x10000u)<<16)|pick(8));
        m.put(at+0x54u,functions+k*0x10u);
        m.put(functions+k*0x10u,std::uint32_t(rng()));
        m.put(functions+k*0x10u+1u,pick(3)==0?(pick(16)<<4):pick(16),1);
        m.put(functions+k*0x10u+4u,value(40,10.0f));m.put(functions+k*0x10u+8u,value(40,10.0f));
    }
    // The location.
    for(std::uint32_t o=0;o<0x34u;o+=4)m.put(location+o,std::uint32_t(rng()));
    for(std::uint32_t o=0x10u;o<0x1cu;o+=4)m.put(location+o,value(40,4.0f));
    m.put(location+0x1cu,pick(3)==0?0u:value(10,10.0f));
    m.put(location+0x30u,std::uint32_t(stamp)+std::uint32_t(drawn_offsets[pick(6)]));
    // Emitter chains with distinct particle chains.
    std::uint32_t next_particle=0;
    std::uint32_t previous=0xffffffffu;
    std::vector<std::uint32_t> emitter_indices;
    for(unsigned k=0;k<emitters;++k)emitter_indices.push_back(k*3u+pick(3));
    for(int k=int(emitters)-1;k>=0;--k) {
        const auto index=emitter_indices[std::size_t(k)];
        const auto at=emitter_data+index*0x4cu;
        for(std::uint32_t o=0;o<0x4cu;o+=4)m.put(at+o,std::uint32_t(rng()));
        m.put(at+8u,previous);
        // Its particles.
        const unsigned count=shape==3?0:pick(7);
        std::uint32_t chain=0xffffffffu;
        for(unsigned p=0;p<count;++p) {
            const auto particle=next_particle++;
            const auto pat=particle_data+particle*0x40u;
            for(std::uint32_t o=0;o<0x40u;o+=4)m.put(pat+o,std::uint32_t(rng()));
            static constexpr std::uint32_t flag_sets[]{0u,0x1u,0x8u,0x9u,0x10u,0x18u,0x11u,0x2u,0x12u,0x10u,0x10u,0xe000u};
            m.put(pat+2u,flag_sets[pick(12)]|(pick(4)==0?pick(0x10000u)&~0x19u:0u),2);
            m.put(pat+4u,chain);
            m.put(pat+8u,pick(5)==0?special(wide):std::bit_cast<std::uint32_t>(float(int(pick(130)))/100.0f));
            m.put(pat+0xcu,pick(9)==0?special(wide):std::bit_cast<std::uint32_t>(float(int(pick(50)))/10.0f));
            for(std::uint32_t o=0x1cu;o<0x28u;o+=4)m.put(pat+o,value(40,4.0f));
            // Velocities around the rest speed (1/4 per axis).
            for(std::uint32_t o=0x28u;o<0x34u;o+=4)m.put(pat+o,pick(3)==0?value(40,40.0f):small_float(rng,3,20.0f));
            m.put(pat+0x34u,value(40,10.0f));m.put(pat+0x38u,value(40,10.0f));
            chain=(pick(0x10000u)<<16)|particle;
        }
        m.put(at+4u,chain);m.put(at+2u,count,2);
        previous=(pick(0x10000u)<<16)|index;
    }
    m.put(location+4u,previous);
    for(std::uint32_t at=scratch;at<scratch+0x80u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x177954u);m.put(stack+4u,system_record);
    m.put(stack+8u,pick(9)==0?special(wide):std::bit_cast<std::uint32_t>(float(int(pick(40)))/240.0f));
    // Registers: EAX the definition, EBX the location.
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[0]=definition;c.abi.gp[3]=location;c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1f81u,0x1fbfu};
    c.mxcsr=modes[pick(9)];
    c.abi.x87_depth=pick(13)==0?6u+pick(2):pick(6);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(pick(2001))-1000)/7.0L;
    const unsigned top=(8u-c.abi.x87_depth+pick(8))&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(std::uint32_t(rng())&0x4700u)|(pick(5)==0?std::uint32_t(rng())&0x3fu:0u));
    const std::uint16_t control=std::uint16_t(0x3fu|0x40u|(pick(4)<<8)|(pick(4)<<10));
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,c.mxcsr};
    if(shape==22) {
        static constexpr std::uint32_t regions[]{location,system_record,definition,emitter_definitions,functions,emitter_data,
            particle_data,tags,0x486000u,vtables,0x46a000u,0x4c6000u,stack-0x100u};
        m.pages.erase(regions[pick(13)]>>12);
    }
    return c;
}
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{};
template<class V> bool native(V &s) {
    return pu::bridge_251a60(s,[](auto &v){original::native_effect_reference_251a60(v);},
        [](auto &v){original::run_13a6b0(v);},[](auto &v){original::run_251650(v);},[](auto &v){original::run_16e8a0(v);},
        [](auto &v){original::run_2c40d0(v);},[](auto &v){original::run_2516c0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){original::dispatch(v,target,site,false);});
}
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){
        if(result.stores.size()>=100000)throw std::runtime_error("Store budget exceeded");
        result.stores.push_back(e);
    };
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native particle update delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    const auto saved=_mm_getcsr();std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        s.load(c.abi);_mm_setcsr(c.mxcsr);
        child=c.child;call_log=&result.calls;
        if(is_native)result.admitted=native(s);
        else original::run_251a60(s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;result.mxcsr=_mm_getcsr();
    result.abi=s.save();
    asm volatile("fnclex\n\tfldcw %0"::"m"(control));_mm_setcsr(saved);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    if(a.floating_point && b.floating_point)same=same && a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr;
    return same;
}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(mode=="verify")setenv("HALO2_VERIFY_EFFECT_OWNERS","1",1);
    if(mode=="reference")setenv("HALO2_REFERENCE_EFFECT_OWNERS","20680,251a60",1);
    std::mt19937 rng(0xe1e1u+(census_mode?1u:0u)+(mode=="verify"?2u:0u)+(mode=="reference"?4u:0u));
    std::uint64_t stores{},compared{},faults{},children{},refused{};
    std::map<std::string,unsigned> fault_kinds;
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()) {
            // Unmasked MXCSR exceptions or the reference switch keep the translated route.
            if((c.mxcsr&0x1f80u)!=0x1f80u || mode=="reference"){++refused;continue;}
            std::fprintf(stderr,"case %u refused\n",n);return 1;
        }
        if(mode=="reference"){std::fprintf(stderr,"case %u admitted under the 251A60 reference switch\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();children+=reference.calls.size();
        if(!reference.fault.empty()){++faults;++fault_kinds[reference.fault.substr(0,40)];}
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u fault mismatch: native '%s' translated '%s'\n",n,native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u store mismatch at %zu of %zu/%zu",n,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            else if(at<reference.stores.size())std::fprintf(stderr," original pc=%x",reference.stores[at].instruction);
            else if(at<native_result.stores.size())std::fprintf(stderr," native pc=%x",native_result.stores[at].instruction);
            std::fprintf(stderr," (fault '%s')\n",reference.fault.c_str());return 1;
        }
        if(native_result.calls!=reference.calls) {
            std::size_t at=0;while(at<native_result.calls.size()&&at<reference.calls.size()&&native_result.calls[at]==reference.calls[at])++at;
            std::fprintf(stderr,"case %u child log mismatch at %zu of %zu/%zu\n",n,at,native_result.calls.size(),reference.calls.size());return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor){std::fprintf(stderr,"case %u cursor %x vs %x (fault '%s')\n",n,
            native_result.cursor,reference.cursor,reference.fault.c_str());return 1;}
        if(native_result.mxcsr!=reference.mxcsr){std::fprintf(stderr,"case %u MXCSR %x vs %x\n",n,native_result.mxcsr,reference.mxcsr);return 1;}
        if(reference.fault.empty() && !same_abi(native_result.abi,reference.abi)) {
            const auto &a=native_result.abi,&b=reference.abi;
            std::fprintf(stderr,"case %u register/flag/XMM/x87 mismatch: eflags %x/%x x87 %x/%x depth %u/%u\n",n,a.eflags,b.eflags,
                a.x87_status,b.x87_status,a.x87_depth,b.x87_depth);
            for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])std::fprintf(stderr,"  gp%u %x/%x\n",i,a.gp[i],b.gp[i]);
            for(unsigned i=0;i<8;++i)if(a.xmm[i]!=b.xmm[i])std::fprintf(stderr,"  xmm%u %x %x/%x %x\n",i,a.xmm[i][0],a.xmm[i][1],b.xmm[i][0],b.xmm[i][1]);
            return 1;
        }
        if(!left.same(right)){std::fprintf(stderr,"case %u bank mismatch\n",n);return 1;}
    }
    if(!census_mode && mode!="reference") {
        auto c=author(rng,1);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(a))throw std::runtime_error("MapPack11 admitted");
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
        const auto saved=_mm_getcsr();_mm_setcsr(0x1f00u);
        const bool unmasked=native(b);_mm_setcsr(saved);
        if(unmasked)throw std::runtime_error("Unmasked MXCSR admitted");
        WorldState e{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);e.begin_mmx();
        if(native(e))throw std::runtime_error("Active MMX admitted");
        services.instruction=[](auto){};
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
        if(native(d))throw std::runtime_error("Full instruction callbacks admitted");
        asm volatile("fnclex");
    }
    for(const auto &[kind,count]:fault_kinds)std::fprintf(stderr,"fault %5u %s\n",count,kind.c_str());
    std::printf("{\"cases\":%u,\"compared\":%llu,\"refused\":%llu,\"faults\":%llu,"
        "\"ordered_stores\":%llu,\"children\":%llu,\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",cases,
        (unsigned long long)compared,(unsigned long long)refused,
        (unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)pu::coverage().load(),unsigned(pu::branch_count),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
