// Native TU1.5 2F36F0 child agent update (a collection agent's child agents,
// one per child shape near body A; 2E4550 and 30BAD0 inline) against the
// unmodified translated canonical body run_2efc30, extracted by
// tools/validate_native_child_agents_tu15.py into agents_bodies.inc (with the
// translated 2E4550 and 30BAD0 the reference runs) with the unit's selected()
// operand cases (agents_selected.inc) and the binder's verification reference
// (agents_reference.inc).
//
// The translated children (2E31E0, 2F3430, 2F34D0, 2F33B0) and the virtual
// targets (getAabb, queryAabb, the child agents' clean-up, getChildShape,
// isCollisionEnabled, the creation function, the thread memory's allocate and
// deallocate) are deterministic stand-ins, entered by both routes with the
// same state. Each appends its complete entry state (target, canonical site,
// general registers, flags, x87 depth/status/TOP, an XMM digest) to a log in
// the fixture and reads its arguments, so a wrong register, flag or argument
// at a call reaches the store stream. They keep EBX, ESI, EDI and EBP, and
// change the scratch registers, flags and XMM registers. getAabb writes the
// agent's cached AABB, a moved one or a random one; queryAabb appends the
// planned keys, growing the keys' storage past its inline buffer when there
// are more than 128; the reserve moves the storage; the creation function
// makes a new agent; the sort sorts unsigned.
//
// Seeded fixtures: the cache enabled or not and its AABB kept or moved (each
// corner), either matching, entries 0-12 with keys ascending (or not) and
// agents real, null or the null agent, keys returned in the entries' order
// with some removed and some new (or shuffled), the filter's verdict, the
// entries' storage full, owned or not, more than 128 keys, special
// tolerances and half factors (NaN payloads), an unbound virtual target,
// entries straddling a missing page, random register files, x87 depths, an
// active MMX state, every MXCSR mode and fault injection with one fixture
// page removed. Each case compares the ordered store events (pc, address,
// preimage, bytes), the final bank, registers, flags, XMM, x87, MXCSR and the
// cursor; fault cases compare the exception, the cursor and the bank.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include <algorithm>
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

namespace t_agents {
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
#include "agents_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses (the globals at their TU1.5 literals).
inline constexpr std::uint32_t agent=0x10100000u,body_a=0x10101000u,body_b=0x10101040u,motion_a=0x10102000u,motion_b=0x10102100u,
    shape_a=0x10103000u,tree=0x10103100u,container=0x10103200u,vtables=0x10104000u,input=0x10105000u,filter=0x10105100u,
    plan=0x10106000u,entries=0x10110000u,pool=0x10120000u,heap=0x10130000u,log_base=0x10900000u,fs=0x10700000u,
    memory_object=0x10105200u,null_agent=0x48cdc4u;
inline constexpr std::uint32_t aabb_target=0x7d0000u,query_target=0x7d0010u,cleanup_target=0x7d0020u,child_shape_target=0x7d0030u,
    filter_target=0x7d0040u,create_target=0x7d0050u,allocate_target=0x7d0060u,deallocate_target=0x7d0070u;
// plan: +0 the AABB's plan, +4 the filter's mode, +8 the key count, +10.. the keys.
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
inline void scratch(ObservedWorldState &s,std::uint32_t seed) {
    s.gp[0]^=seed*0x2c1b3c6du;s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    for(unsigned i=1;i<8;i+=1+(seed>>i&1u))
        s.xmm[i]=_mm_castsi128_ps(_mm_set_epi32(int(seed*i),int(seed*3u+i),int(seed^0x80000000u),int(seed+7u*i)));
}
// A bump allocation from the fixture's heap (its next free byte at heap+0).
inline std::uint32_t allocate_block(ObservedWorldState &s,std::uint32_t bytes) {
    const auto next=r(s.memory,heap);
    w(s.memory,s.observe,0x7b0008u,heap,4,next+((bytes+15u)&~15u));
    return next;
}
inline void keep_callee(ObservedWorldState &s,std::uint32_t seed,std::uint32_t returned,std::uint32_t pop) {
    const auto ebx=s.gp[3];scratch(s,seed);s.gp[3]=ebx;s.gp[0]=returned;s.gp[4]+=pop;
}
// 2E31E0 (canonical 2DF710; cdecl: array, capacity, element size): the
// storage moves to a new owned block.
static void run_2df710(ObservedWorldState &s) {
    const auto array=argument(s,1),capacity=argument(s,2),size=argument(s,3);
    const auto seed=fold(s,array^capacity*31u^size,array,3);
    log_entry(s,0x2e31e0u,0,seed);
    const auto count=r(s.memory,array+4u),data=r(s.memory,array);
    const auto block=allocate_block(s,(capacity&0xffffu)*size);
    for(std::uint32_t i=0;i<(count&0xffffu)*size && i<0x400u;i+=4)w(s.memory,s.observe,0x7b0010u,block+i,4,r(s.memory,data+i));
    w(s.memory,s.observe,0x7b0014u,array,4,block);w(s.memory,s.observe,0x7b0018u,array+8u,4,capacity&0x7fffffffu);
    keep_callee(s,seed,block,4u);
}
// 2F3430 (canonical 2EF970; hkLocalArray<ChildEntry>(count): ECX, RET 4).
static void run_2ef970(ObservedWorldState &s) {
    const auto array=s.gp[1],count=argument(s,1);
    const auto seed=fold(s,array^count,array,5);
    log_entry(s,0x2f3430u,0,seed);
    const auto block=allocate_block(s,(count&0xffffu)*12u+(seed%3u==0u?0u:24u));
    const std::uint32_t words[]{block,0u,0x80000000u|count,block,count};
    for(unsigned i=0;i<5;++i)w(s.memory,s.observe,0x7b0020u,array+4u*i,4,words[i]);
    keep_callee(s,seed,array,8u);
}
// 2F34D0 (canonical 2EFA10; ~hkLocalArray: ECX).
static void run_2efa10(ObservedWorldState &s) {
    const auto array=s.gp[1];
    const auto seed=fold(s,array,array,5);
    log_entry(s,0x2f34d0u,0,seed);
    w(s.memory,s.observe,0x7b0030u,array+0xcu,4,seed);
    keep_callee(s,seed,seed>>3,4u);
}
// 2F33B0 (canonical 2EF8F0; cdecl: keys, low, high, comparator): unsigned
// ascending.
static void run_2ef8f0(ObservedWorldState &s) {
    const auto keys=argument(s,1),low=argument(s,2),high=argument(s,3),comparator=argument(s,4);
    const auto seed=fold(s,keys^low*7u^high*13u^comparator,keys,2);
    log_entry(s,0x2f33b0u,0,seed);
    if(std::int32_t(high)>=std::int32_t(low) && high-low<0x400u) {
        std::vector<std::uint32_t> values;
        for(auto i=low;i<=high;++i)values.push_back(r(s.memory,keys+4u*i));
        std::sort(values.begin(),values.end());
        for(std::uint32_t i=0;i<values.size();++i)w(s.memory,s.observe,0x7b0040u,keys+4u*(low+i),4,values[i]);
    }
    keep_callee(s,seed,seed,4u);
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    switch(target) {
    case aabb_target: {  // shape->getAabb(transform, tolerance, aabb): RET C
        const auto transform=argument(s,1),tolerance=argument(s,2),out=argument(s,3);
        auto seed=fold(s,s.gp[1]^tolerance,transform,16);
        log_entry(s,target,site,seed);
        const auto mode=r(s.memory,plan);
        for(unsigned i=0;i<8;++i) {
            auto value=r(s.memory,agent+0x20u+4u*i);  // the cached AABB
            if(mode==1u && i==4u)value^=0x00400000u;  // the maximum's x moves
            if(mode==2u && i==1u)value^=0x00400000u;  // the minimum's y moves
            if(mode==3u)value=std::bit_cast<std::uint32_t>(float(std::int32_t(seed>>8&0xfffu)-2048)/16.0f+float(i));
            if(mode==4u && i==2u)value+=1u;           // within any slack
            w(s.memory,s.observe,0x7c0000u,out+4u*i,4,value);
        }
        keep_callee(s,seed,out,0x10u);return;
    }
    case query_target: {  // tree->queryAabb(aabb, hits): RET 8
        const auto aabb=argument(s,1),hits=argument(s,2);
        const auto seed=fold(s,s.gp[1],aabb,8);
        log_entry(s,target,site,seed);
        const auto count=r(s.memory,plan+8u);
        for(std::uint32_t i=0;i<count;++i) {
            auto size=r(s.memory,hits+4u);const auto capacity=r(s.memory,hits+8u)&0x7fffffffu;
            if(size>=capacity) {  // grow: a new owned block of twice the capacity
                const auto block=allocate_block(s,capacity*8u);
                for(std::uint32_t k=0;k<size;++k)w(s.memory,s.observe,0x7c0010u,block+4u*k,4,r(s.memory,r(s.memory,hits)+4u*k));
                w(s.memory,s.observe,0x7c0014u,hits,4,block);w(s.memory,s.observe,0x7c0018u,hits+8u,4,capacity*2u);
            }
            w(s.memory,s.observe,0x7c001cu,r(s.memory,hits)+4u*size,4,r(s.memory,plan+0x10u+4u*(i%0x3f0u)));
            w(s.memory,s.observe,0x7c0020u,hits+4u,4,size+1u);
        }
        keep_callee(s,seed,hits,0xcu);return;
    }
    case cleanup_target: {  // child->cleanup(): RET 0
        const auto child=s.gp[1];
        const auto seed=fold(s,child,child,2);
        log_entry(s,target,site,seed);
        w(s.memory,s.observe,0x7c0030u,child+4u,4,r(s.memory,child+4u)+1u);
        keep_callee(s,seed,child,4u);return;
    }
    case child_shape_target: {  // container->getChildShape(key, buffer): RET 8
        const auto key=argument(s,1),buffer=argument(s,2);
        const auto seed=fold(s,s.gp[1]^key,buffer,2);
        log_entry(s,target,site,seed);
        w(s.memory,s.observe,0x7c0040u,buffer,4,key);w(s.memory,s.observe,0x7c0044u,buffer+4u,4,seed);
        keep_callee(s,seed,buffer+(seed&1u?0u:0x10u),0xcu);return;
    }
    case filter_target: {  // filter->isCollisionEnabled(result, input, a, b, container, key): RET 18
        const auto result=argument(s,1),key=argument(s,6),body=argument(s,4);
        auto seed=fold(s,s.gp[1]^key^argument(s,2)^argument(s,3)*3u^argument(s,5)*5u,body,4);
        log_entry(s,target,site,seed);
        const auto mode=r(s.memory,plan+4u);
        const bool enabled=mode==0u?(key*0x9e3779b1u>>29)%4u!=0u:mode==1u;
        w(s.memory,s.observe,0x7c0050u,result,1,enabled?1u+(seed&0x7eu):0u);
        keep_callee(s,seed,result,0x1cu);return;
    }
    case create_target: {  // create(a, child body, input, manager): cdecl
        const auto body=argument(s,2),manager=argument(s,4);
        const auto seed=fold(s,argument(s,1)^manager*3u^argument(s,3)*7u,body,4);
        log_entry(s,target,site,seed);
        const auto made=allocate_block(s,0x20u);
        w(s.memory,s.observe,0x7c0060u,made,4,vtables+0x400u);w(s.memory,s.observe,0x7c0064u,made+8u,4,r(s.memory,body+4u));
        keep_callee(s,seed,made,4u);return;
    }
    case allocate_target: {  // memory->allocate(bytes, class): RET 8
        const auto bytes=argument(s,1),type=argument(s,2);
        const auto seed=bytes*0x9e3779b1u^type;
        log_entry(s,target,site,seed);
        keep_callee(s,seed,allocate_block(s,bytes&0xffffu),0xcu);return;
    }
    case deallocate_target: {  // memory->deallocate(data, bytes, class): RET C
        const auto data=argument(s,1),bytes=argument(s,2),type=argument(s,3);
        const auto seed=data^bytes*0x9e3779b1u^type;
        log_entry(s,target,site,seed);
        w(s.memory,s.observe,0x7c0070u,heap+4u,4,r(s.memory,heap+4u)+bytes);
        keep_callee(s,seed,seed,0x10u);return;
    }
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "agents_bodies.inc"
#include "agents_reference.inc"
} // namespace t_agents
using t_agents::ObservedWorldState;

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
    c.abi.gp[4]=entry;c.abi.fs_base=t_agents::fs;
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

Case author_agents(std::mt19937 &rng,unsigned n) {
    using namespace t_agents;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    for(const auto page:{0x461000u,0x46a000u,0x46e000u,0x48c000u,agent,body_a,motion_a,shape_a,vtables,input,plan,entries,
        entries+0x1000u,pool,heap,heap+0x1000u,heap+0x2000u,heap+0x3000u})m.pages_at(page,1);
    m.pages_at(log_base,5);
    // The globals.
    for(unsigned i=0;i<4;++i)m.put(0x461950u+4u*i,pick(8)?0x7fffffffu:std::uint32_t(rng()));
    const auto payload=[&]{return 0x7f800001u|(std::uint32_t(rng())&0x803fffffu);};
    m.put(0x46a0ccu,kind==12?(pick(2)?payload():special(rng)):pick(6)?0x3f000000u:special(rng));
    m.put8(0x46e468u,kind==4 || kind==5?1u+pick(255):pick(2)?0u:1u);
    m.put8(0x48cbb8u,kind==6?1u:kind==7 || kind==11?pick(2):pick(2)?0u:1u+pick(255));
    m.put(0x48c938u,memory_object);m.put(null_agent,vtables+0x500u);
    m.put(heap,heap+0x100u);
    // The bodies, the shapes and their virtual tables.
    m.put(body_a,shape_a);m.put(body_a+4u,std::uint32_t(rng()));m.put(body_a+8u,motion_a);m.put(body_a+0xcu,0u);
    m.put(body_b,tree);m.put(body_b+4u,std::uint32_t(rng()));m.put(body_b+8u,motion_b);m.put(body_b+0xcu,0u);
    for(std::uint32_t at=0;at<0x60u;at+=4) {
        m.put(motion_a+at,pick(4)?std::bit_cast<std::uint32_t>(small(rng,2.0f)):special(rng));
        m.put(motion_b+at,pick(4)?std::bit_cast<std::uint32_t>(small(rng,2.0f)):special(rng));
    }
    m.put(shape_a,vtables);m.put(tree,vtables+0x100u);m.put(tree+0xcu,container);m.put(container,vtables+0x200u);
    m.put(filter,vtables+0x300u);m.put(memory_object,vtables+0x600u);
    for(std::uint32_t at=0;at<0x700u;at+=4)m.put(vtables+at,std::uint32_t(rng()));
    m.put(vtables+0x18u,aabb_target);m.put(vtables+0x130u,query_target);m.put(vtables+0x234u,child_shape_target);
    m.put(vtables+0x300u,filter_target);m.put(vtables+0x414u,cleanup_target);m.put(vtables+0x514u,cleanup_target);
    m.put(vtables+0x610u,allocate_target);m.put(vtables+0x614u,deallocate_target);
    if(kind==13) {  // one virtual target is not bound
        static constexpr std::uint32_t slots[]{0x18u,0x130u,0x234u,0x300u,0x414u,0x610u,0x614u};
        m.put(vtables+slots[pick(std::size(slots))],0x7e0000u+pick(16));
    }
    m.put(input,std::uint32_t(rng()));m.put(input+4u,filter);
    m.put(input+8u,kind==12?(pick(2)?payload():special(rng)):pick(5)?std::bit_cast<std::uint32_t>(0.05f+small(rng,0.04f)):special(rng));
    // The agent: its cached AABB and its entries.
    for(std::uint32_t at=0;at<0x40u;at+=4)m.put(agent+at,std::uint32_t(rng()));
    for(std::uint32_t at=0x20u;at<0x40u;at+=4)m.put(agent+at,pick(8)?std::bit_cast<std::uint32_t>(small(rng,20.0f)):special(rng));
    const std::uint32_t count=kind==10?0u:kind==7?pick(4):pick(9)+(pick(4)?0u:pick(5));
    std::vector<std::uint32_t> keys;
    for(std::uint32_t i=0,key=pick(64);i<count;++i,key+=1u+pick(pick(4)?6u:0x40000000u))keys.push_back(key);
    if(pick(8)==0)std::shuffle(keys.begin(),keys.end(),rng);
    const auto data=kind==14?entries+0x1000u-12u*pick(count+1u):entries+0x10u*pick(16);
    for(std::uint32_t at=0;at<0x2000u;at+=4)m.put(entries+at,std::uint32_t(rng()));
    for(std::uint32_t i=0;i<count;++i) {
        const auto child=pool+0x20u*i;
        m.put(child,vtables+0x400u);m.put(child+4u,0u);
        const auto which=pick(10);
        m.put(data+12u*i,keys[i]);m.put(data+12u*i+8u,which<7?child:which<9?null_agent:0u);
    }
    m.put(agent+0xcu,data);m.put(agent+0x10u,count);
    const auto capacity=kind==11?count:count+pick(4);
    m.put(agent+0x14u,capacity|(pick(3)?0u:0x80000000u));
    // The plan: the AABB, the filter and the keys the query returns.
    m.put(plan,kind==4?(pick(2)?0u:4u):kind==5?2u:kind==6 || kind==7 || kind==11?(pick(2)?1u:3u):pick(5));
    m.put(plan+4u,pick(3));
    std::vector<std::uint32_t> hits;
    if(kind==7)for(std::uint32_t i=0,n=129u+pick(12);i<n;++i)hits.push_back(std::uint32_t(rng())%0x200u);
    else {
        for(const auto key:keys)if(pick(4))hits.push_back(key);  // kept, in the entries' order
        for(std::uint32_t i=0,extra=pick(5);i<extra;++i)  // new keys, at random places
            hits.insert(hits.begin()+pick(std::uint32_t(hits.size())+1u),pick(3)?std::uint32_t(rng())%0x100u:std::uint32_t(rng()));
        if(pick(4)==0)std::shuffle(hits.begin(),hits.end(),rng);
        if(pick(10)==0 && !hits.empty())hits.push_back(hits[pick(std::uint32_t(hits.size()))]);  // a duplicate
    }
    m.put(plan+8u,std::uint32_t(hits.size()));
    for(std::uint32_t i=0;i<hits.size();++i)m.put(plan+0x10u+4u*i,hits[i]);
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x3043e0u);m.put(entry+4u,body_a);m.put(entry+8u,body_b);m.put(entry+12u,input);m.put(entry+16u,create_target);
    m.put(t_agents::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    c.abi.gp[1]=agent;
    if(kind==15) {
        static constexpr std::uint32_t regions[]{0x461000u,0x46a000u,0x46e000u,0x48c000u,agent,body_a,motion_a,shape_a,vtables,input,
            entries,pool,heap,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    if(kind==14)m.pages.erase((entries+0x1000u)>>12);
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
        services.instruction=[](auto){throw std::runtime_error("Native child agent update delivered a translated instruction");};
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
    Totals agents;
    if(compare("2F36F0",cases,0x2f36f0u+variant,agents,author_agents,
        [](ObservedWorldState &s){return leaves::bridge_2f36f0(s,[](auto &v){t_agents::native_world_leaves_reference_2efc30(v);},
            [](auto &v){t_agents::run_2df710(v);},[](auto &v){t_agents::run_2ef970(v);},[](auto &v){t_agents::run_2efa10(v);},
            [](auto &v){t_agents::run_2ef8f0(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_agents::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_agents::run_2efc30(s);}))return 1;
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)agents.compared,(unsigned long long)agents.stores,(unsigned long long)agents.faults,
        (unsigned long long)agents.children,(unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),
        mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
