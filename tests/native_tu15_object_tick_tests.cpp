// Native TU1.5 per-tick object update owners (B8F40/B9090/B9180/BE4A0/10DD10)
// against the unmodified translated bodies, extracted by
// tools/validate_native_object_tick_closures_tu15.py into tick_<owner>.inc, plus the
// verification references (tick_<owner>_reference.inc) the production binder
// generates. Every child is a genuine stand-in whose effects (stores into the
// object table and data, registers including callee-saved clobbers, stack
// balance, flags, XMM) are a hash of its complete entry ABI and the bytes it
// can see, so any entry difference diverges. Seeded fixtures author complete
// object tables (headers, bit vectors, data blocks, child chains, type
// callback chains, player roots) with special flag patterns, invalid handles
// and fault injection (one fixture page removed). Each case compares the
// ordered store events (pc, address, preimage, bytes), the child call log,
// final bank, registers, flags, XMM, x87 status and instruction cursor; fault
// cases compare the exception, the cursor and the store prefix.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_tick.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace tick=halo2::native_object_tick_detail;
namespace {
std::uint32_t sign_extend_value(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}

constexpr std::uint32_t players=0x10000000u,global=0x10100000u,header=0x10200000u,elements=0x10300000u,
    bits=0x10400000u,data=0x10500000u,game=0x10600000u,clusters=0x10700000u,types=0x10900000u,
    components=0x10a00000u,stack=0x20010000u,scratch=0x10b00000u;
// Per-case stand-in policy: bit0 callee-saved clobbers, bit1 stack
// imbalance, bit2 XMM changes, bit3 flag-byte churn.
std::uint32_t policy{};std::uint32_t objects{};
std::vector<std::uint64_t> *call_log{};
struct Expect {std::uint32_t target;unsigned bytes;std::set<std::uint32_t> returns;};
const std::map<std::uint32_t,Expect> &expectations() {
    static const std::map<std::uint32_t,Expect> value{
        {0xbe4a0u,{0xbe4a0u,4,{0xb8f9eu,0xb9066u,0xb9270u}}},{0x10e630u,{0x10e630u,4,{0xb8fafu}}},
        {0xbe850u,{0xbe850u,4,{0xb90deu,0xb9172u,0xb9280u}}},{0x10e710u,{0x10e710u,0,{0xb90e7u}}},
        {0x102910u,{0x102910u,0,{0xb9126u,0xb91d0u}}},{0xba3e0u,{0xba3e0u,8,{0xb92b2u}}},
        {0xba490u,{0xba490u,8,{0xb92bau}}},{0xc13b0u,{0xc13b0u,0,{}}},
        {0x1d5790u,{0x1d5790u,0,{0xbe54cu}}},{0x10dd10u,{0x10dd10u,4,{0xbe554u}}},
        {0xc0910u,{0xc0910u,4,{0xbe564u}}},{0x1d6fb0u,{0x1d6fb0u,0,{0xbe5cau}}},
        {0xbda50u,{0xbda50u,0,{0xbe4fau}}},{0xb37a0u,{0xb37a0u,0,{0xbe51du}}},{0xd8330u,{0xd8330u,4,{0xbe55cu}}},
    };
    return value;
}
// A genuine child stand-in: every effect is a hash of the entry ABI, the
// return address/arguments and the object flag bytes it can observe.
void stub(WorldState &s,std::uint32_t target) {
    auto found=expectations().find(target);
    unsigned bytes=4;std::uint32_t expected_return=0;
    if(found!=expectations().end()) {
        bytes=found->second.bytes;
        const auto back=r(s.memory,s.gp[4]);
        if(!found->second.returns.empty() && !found->second.returns.count(back)) {
            char text[96];std::snprintf(text,sizeof text,"Child %x entered with return address %x",target,back);
            throw std::runtime_error(text);
        }
        expected_return=back;
    } else if(target<0x300000u || target>=0x300100u)throw std::runtime_error("Unbound child target");
    std::uint64_t h=mix(0x0b1ec7u,target);
    for(const auto v:s.gp)h=mix(h,v);
    for(unsigned i=0;i<3;++i)h=mix(h,r(s.memory,s.gp[4]+i*4u));
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,s.fp.status);h=mix(h,expected_return);
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    for(std::uint32_t i=0;i<objects && i<8;++i)h=mix(h,r(s.memory,elements+i*12u+2u,1));
    std::mt19937_64 rng(h);
    if(call_log) {
        // A deterministic budget ends clobber-induced rescans identically.
        if(call_log->size()>=4096)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(mix(h,target));
    }
    // One store into this stand-in's own scratch (the store stream sees it).
    w(s.memory,s.observe,target,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(objects && (policy&8u || rng()%3==0)) {
        const auto index=std::uint32_t(rng()%objects);
        w(s.memory,s.observe,target+1u,elements+index*12u+2u,1,std::uint32_t(rng()));
        if(rng()%2) {
            const auto block=data+index*0x200u;
            static constexpr std::uint32_t offsets[]{0xc0u,0xb3u,0xd8u,0xb4u};
            const auto at=offsets[rng()%4];
            w(s.memory,s.observe,target+2u,block+at,at==0xb4u?4:1,rng()%5==0?0xffffffffu:std::uint32_t(rng()%objects));
        }
    }
    if(rng()%7==0)w(s.memory,s.observe,target+3u,global+3u,1,std::uint32_t(rng()));
    const auto sp=s.gp[4];
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(rng()%3)s.gp[0]&=0xffffff00u; // most children report no change in AL
    if(policy&1u && rng()%5==0) {
        static constexpr std::uint32_t values[]{0xfffffffeu,0x7fffffffu,0xffffffffu,0u,1u,0x80000000u};
        static constexpr unsigned saved[]{3,5,6,7}; // EBX/EBP/ESI/EDI
        s.gp[saved[rng()%4]]=values[rng()%6];
    }
    if(policy&4u)for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;for(auto &v:b)v=std::uint32_t(rng());std::memcpy(&s.xmm[i],b.data(),16);}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t next=sp+4u+bytes;
    if(policy&2u && rng()%9==0)next+=rng()%2?4u:std::uint32_t(-4);
    s.gp[4]=next;
}
} // namespace

#define TICK_CHILD(name) static void run_##name(WorldState &s){stub(s,0x##name##u);}
#define TICK_DISPATCH static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target);}
namespace t_b8f40 {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
TICK_CHILD(be4a0) TICK_CHILD(10e630)
#include "tick_b8f40.inc"
#include "tick_b8f40_reference.inc"
}
namespace t_b9090 {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
TICK_CHILD(be850) TICK_CHILD(10e710) TICK_CHILD(102910)
#include "tick_b9090.inc"
#include "tick_b9090_reference.inc"
}
namespace t_b9180 {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
TICK_CHILD(be4a0) TICK_CHILD(be850) TICK_CHILD(102910) TICK_CHILD(ba3e0) TICK_CHILD(ba490) TICK_CHILD(c13b0)
#include "tick_b9180.inc"
#include "tick_b9180_reference.inc"
}
namespace t_be4a0 {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
TICK_DISPATCH
TICK_CHILD(1d5790) TICK_CHILD(10dd10) TICK_CHILD(c0910) TICK_CHILD(1d6fb0)
static void run_be4a0(WorldState &);
#include "tick_be4a0.inc"
#include "tick_be4a0_reference.inc"
}
namespace t_10dd10 {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
TICK_DISPATCH
#include "tick_10dd10.inc"
#include "tick_10dd10_reference.inc"
}

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

enum Owner : unsigned {b8f40,b9090,b9180,be4a0,dd10,owners};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};Owner owner{};std::uint32_t policy{},objects{};};

std::uint32_t handle(std::mt19937 &rng,std::uint32_t index){return (std::uint32_t(rng()%0x10000u)<<16)|index;}

// One authored world. `shape` selects structural edge cases.
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    c.owner=Owner(n%owners);
    const unsigned shape=(n/owners)%16;
    c.policy=pick(16);
    std::uint32_t count=1+pick(48);
    if(shape==3)count=1+pick(4);
    if(shape==5)count=32*(1+pick(3));
    if(shape==7)count=0;
    c.objects=count;
    m.put(0x51daf0u,players);m.put(0x4eabc4u,global);m.put(0x4ecbd0u,header);m.put(0x4f3218u,game);m.put(0x52be5cu,clusters);
    m.put(global,std::uint32_t(rng()));m.put(global+4u,std::uint32_t(rng()));
    m.put(header+0x24u,shape==9?16u:12u);m.put(header+0x38u,shape==11?std::uint32_t(-1-int(pick(3))):count);
    m.put(header+0x44u,elements);m.put(header+0x48u,bits);
    const auto stride=shape==9?16u:12u;
    const auto words=(count+31)/32+1;
    const auto density=1+pick(10);
    for(std::uint32_t w=0;w<words+1;++w) {
        std::uint32_t v=0;for(unsigned b=0;b<32;++b)if(pick(10)<density)v|=1u<<b;
        if(shape==13)v=0;
        m.put(bits+w*4u,v);
    }
    for(std::uint32_t i=0;i<std::max(count,1u)*(stride/12+1)+2;++i) {
        const auto e=elements+i*12u;
        m.put(e,pick(0x10000),2);
        std::uint32_t flags=std::uint32_t(rng())&0xffu;
        if(pick(3)==0)flags=(flags|3u)&~0x18u;   // active, awake, not deleted
        if(pick(4)==0)flags|=8u;
        m.put(e+2u,flags,1);m.put(e+3u,pick(256),1);
        m.put(e+4u,std::uint32_t(rng()));m.put(e+8u,data+i*0x200u);
    }
    // Object data: acyclic child chains (children have higher indices).
    for(std::uint32_t i=0;i<std::max(count,1u)+2;++i) {
        const auto d=data+i*0x200u;
        for(std::uint32_t at=0;at<0x200u;at+=4)m.put(d+at,std::uint32_t(rng()));
        const auto later=[&]{return i+1<count?handle(rng,i+1+pick(count-i-1)):0xffffffffu;};
        m.put(d+0x10u,pick(3)?0xffffffffu:later());
        m.put(d+0xcu,pick(2)?0xffffffffu:later());
        m.put(d+0xaau,pick(9)==0?std::uint32_t(-1-int(pick(3)))&0xffu:pick(12),1);
        m.put(d+0xb3u,pick(3)?0u:pick(256),1);
        m.put(d+0xb4u,pick(3)?0xffffffffu:handle(rng,pick(64)));
        m.put(d+0x12au,pick(3)?0xffffu:pick(24)*4u,2);
        if(pick(4)==0)for(std::uint32_t at=0x68u;at<0xd0u;at+=4)m.put(d+at,0xffffffffu);
    }
    // Strictly descending sibling chains keep BE4A0's recursion finite.
    for(std::uint32_t i=0;i<std::max(count,1u)+2;++i) {
        const auto d=data+i*0x200u;
        const auto child=r(m,d+0x10u),next=r(m,d+0xcu);
        if(child!=0xffffffffu && (child&0xffffu)<=i)m.put(d+0x10u,0xffffffffu);
        if(next!=0xffffffffu && (next&0xffffu)<=i)m.put(d+0xcu,0xffffffffu);
    }
    m.put(game+0xcu,pick(2)==0?3u+pick(4):pick(256),1);
    m.put(clusters+0x44u,0x10800000u);
    // Type table (movsx byte index, negative types read below the table).
    for(std::uint32_t at=0x474b90u-0x200u;at<0x474b90u+0x200u;at+=4)m.put(at,types+pick(16)*0x100u);
    for(std::uint32_t t=0;t<16;++t) {
        const auto def=types+t*0x100u;
        const auto entries=t%5==0?0u:1+pick(4);
        for(std::uint32_t k=0;k<entries;++k) {
            const auto component=components+(t*8u+k)*0x80u;
            m.put(def+0x84u+k*4u,component);
            m.put(component+0x40u,pick(4)==0?0u:0x300000u+pick(0x100));
        }
        m.put(def+0x84u+entries*4u,0);
    }
    // Player roots.
    std::uint32_t roots=pick(6);
    if(shape==2)roots=0;
    if(shape==4)roots=std::uint32_t(-int(pick(3)));
    m.put(players+0x1d80u,roots);
    for(std::uint32_t k=0;k<8;++k)m.put(players+0x1d00u+k*4u,count?handle(rng,pick(count)):0);
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x800u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x13ab00u+pick(0x100));
    m.put(stack+4u,count?(pick(11)==0?handle(rng,count+pick(3)):handle(rng,pick(count))):0);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    // Fault injection: remove one authored page (not the stack).
    if(shape==15) {
        static constexpr std::uint32_t regions[]{elements,bits,data,types,components,0x474000u,game};
        const auto region=regions[pick(7)];
        auto at=m.pages.lower_bound(region>>12);
        if(at!=m.pages.end()){std::advance(at,std::min<std::size_t>(pick(3),std::distance(at,m.pages.end())-1));m.pages.erase(at);}
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
void native_be4a0(WorldState &s);
template<class V> bool native(Owner owner,V &s) {
    switch(owner) {
    case b8f40:return tick::bridge_b8f40(s,[](auto &v){t_b8f40::native_object_tick_reference_b8f40(v);},
        [](auto &v){t_b8f40::run_be4a0(v);},[](auto &v){t_b8f40::run_10e630(v);});
    case b9090:return tick::bridge_b9090(s,[](auto &v){t_b9090::native_object_tick_reference_b9090(v);},
        [](auto &v){t_b9090::run_be850(v);},[](auto &v){t_b9090::run_10e710(v);},[](auto &v){t_b9090::run_102910(v);});
    case b9180:return tick::bridge_b9180(s,[](auto &v){t_b9180::native_object_tick_reference_b9180(v);},
        [](auto &v){t_b9180::run_be4a0(v);},[](auto &v){t_b9180::run_be850(v);},[](auto &v){t_b9180::run_102910(v);},
        [](auto &v){t_b9180::run_ba3e0(v);},[](auto &v){t_b9180::run_ba490(v);},[](auto &v){t_b9180::run_c13b0(v);});
    case be4a0:return tick::bridge_be4a0(s,[](auto &v){t_be4a0::native_object_tick_reference_be4a0(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){t_be4a0::dispatch(v,target,site,false);},
        [](auto &v){t_be4a0::run_1d5790(v);},[](auto &v){t_be4a0::run_10dd10(v);},[](auto &v){t_be4a0::run_c0910(v);},
        [](auto &v){native_be4a0(v);},[](auto &v){t_be4a0::run_1d6fb0(v);});
    case dd10:return tick::bridge_10dd10(s,[](auto &v){t_10dd10::native_object_tick_reference_10dd10(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){t_10dd10::dispatch(v,target,site,false);});
    default:return false;
    }
}
// Recursive BE4A0 children run natively at every level.
void native_be4a0(WorldState &s){if(!native(be4a0,s))throw std::runtime_error("Recursive BE4A0 refused");}
void translated(Owner owner,WorldState &s) {
    switch(owner) {
    case b8f40:t_b8f40::run_b8f40(s);return;
    case b9090:t_b9090::run_b9090(s);return;
    case b9180:t_b9180::run_b9180(s);return;
    case be4a0:t_be4a0::run_be4a0(s);return;
    case dd10:t_10dd10::run_10dd10(s);return;
    default:return;
    }
}
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native object tick delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native object tick delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    policy=c.policy;objects=c.objects;call_log=&result.calls;
    try {
        if(is_native)result.admitted=native(c.owner,s);
        else translated(c.owner,s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_OBJECT_TICK","1",1);
    std::mt19937 rng(0xb8f40u+(census_mode?1u:0u)+(verify?2u:0u));
    std::uint64_t stores{},compared{},faults{},children{};
    std::array<std::uint64_t,owners> per{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;++per[c.owner];stores+=reference.stores.size();children+=reference.calls.size();
        if(!reference.fault.empty())++faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u (owner %u) fault mismatch: native '%s' translated '%s'\n",n,c.owner,
                native_result.fault.c_str(),reference.fault.c_str());return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u (owner %u) store mismatch at %zu of %zu/%zu",n,c.owner,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(native_result.calls!=reference.calls){std::fprintf(stderr,"case %u (owner %u) child call log mismatch\n",n,c.owner);return 1;}
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u (owner %u) cursor %x vs %x\n",n,c.owner,native_result.cursor,reference.cursor);return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || !left.same(right))) {
            std::fprintf(stderr,"case %u (owner %u) register/flag/bank mismatch\n",n,c.owner);return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    // Admission refusals: other image, full callbacks without a cursor,
    // traces, arbitrary observers and the reference switch.
    if(!census_mode) {
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(b8f40,a))throw std::runtime_error("MapPack11 admitted");
        services.instruction=[](auto){};
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
        if(native(b9090,b))throw std::runtime_error("Full instruction callbacks admitted");
        bool trace=true;std::uint32_t cursor{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[](auto){};
        services.instruction_cursor.position=&cursor;services.instruction_cursor.service_sites=sites;
        services.instruction_cursor.callback_type=&services.instruction.target_type();
        services.instruction_cursor.executor=std::this_thread::get_id();services.instruction_cursor.literal_service_sites=true;
        services.instruction_cursor.first_trace=&trace;services.instruction_cursor.observation_only_stores=true;
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
        if(native(b9180,d))throw std::runtime_error("Active trace admitted");
        trace=false;services.instruction_cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver arbitrary=[](const auto&){};
        WorldState e{c.memory,arbitrary,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);
        if(native(dd10,e))throw std::runtime_error("Arbitrary observer admitted");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"per_owner\":[%llu,%llu,%llu,%llu,%llu],\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",cases,
        (unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)per[0],(unsigned long long)per[1],(unsigned long long)per[2],(unsigned long long)per[3],(unsigned long long)per[4],
        (unsigned long long)tick::coverage().load(),unsigned(tick::branch_count),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
