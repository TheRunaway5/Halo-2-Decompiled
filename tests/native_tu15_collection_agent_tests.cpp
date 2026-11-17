// Native TU1.5 304350 (the welded collision agent of a shape collection)
// against the unmodified translated canonical body run_300870, extracted by
// tools/validate_native_collection_agent_tu15.py into agent_bodies.inc with the
// bodies of the children the native runs inline (303C70, 303D00, 303EE0,
// 303BA0, 2E47B0, 30BAD0), the unit's selected() operand cases
// (agent_selected.inc) and the binders' verification references
// (agent_reference.inc).
//
// The translated children (2F36F0, 304130, 30CD00, 30CF40, 2E3250) and every
// virtual or function-pointer target (getChildShape, getType, the weld
// predicate, the child agents' closest contact and processCollision, the
// thread memory) are deterministic stand-ins, entered by both routes with the
// same state. Each appends its complete entry state (target, canonical site,
// general registers, flags, x87 depth/status/TOP, an XMM digest) to a log in
// the fixture and reads its stack arguments, so a wrong register, flag, x87
// state or argument at any call reaches the store stream. They keep EBX, ESI,
// EDI and EBP, as the ABI requires, and change the scratch registers and
// flags. The weld changes the record count it returns and sometimes the
// records' size; 2E3250 moves the records to an owned block.
//
// Seeded fixtures: 0-12 children with null and real child agents, predicates
// that gather or process, contacts found or not, distances with ties, NaNs,
// signed zeros, denormals and infinities (the selection sort), child agents
// with 0-4 points, the profiler stream with and without room, buffers that
// fit the thread memory's stack or not, random register files, x87 depths 0-7
// (the deepest overflow the x87 stack, as in the original), every MXCSR mode,
// and fault injection with one fixture page removed. Each case compares the
// ordered store events (pc, address, preimage, bytes), the final bank,
// registers, flags, XMM, x87, MXCSR and the cursor; fault cases compare the
// exception, the cursor and the bank.
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

namespace t_agent {
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
#include "agent_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses.
inline constexpr std::uint32_t agent=0x10100000u,children=0x10110000u,child_agents=0x10120000u,agent_vtable=0x10130000u,
    body_a=0x10140000u,shape_a=0x10141000u,body_b=0x10142000u,shape_b=0x10143000u,collection=0x10144000u,
    inner=0x10145000u,inner_vtable=0x10146000u,motion_b=0x10147000u,shape_vtable=0x10148000u,child_shapes=0x10150000u,
    input=0x10160000u,dispatcher=0x10161000u,output=0x10170000u,manager=0x10180000u,monitor=0x10190000u,
    thread=0x10500000u,thread_vtable=0x10510000u,grown=0x10800000u,log_base=0x10900000u,fs=0x10700000u;
// Stand-in targets.
inline constexpr std::uint32_t allocate_stack_target=0x7a0024u,deallocate_stack_target=0x7a0028u,deallocate_target=0x7a0014u,
    child_shape_target=0x7b0034u,type_target=0x7b0014u,predicate_target=0x7b0040u,closest_target=0x7b0020u,
    process_target=0x7b0010u;
inline void log_entry(ObservedWorldState &s,std::uint32_t target,std::uint32_t site) {
    const auto count=r(s.memory,log_base);
    const auto at=log_base+0x10u+(count%250u)*0x40u;
    std::uint32_t xmm_digest=0;
    for(unsigned i=0;i<8;++i) {
        std::uint32_t lanes[4];std::memcpy(lanes,&s.xmm[i],16);
        for(const auto lane:lanes)xmm_digest=(xmm_digest*0x9e3779b1u)^lane;
    }
    const std::uint32_t words[]{target,site,s.gp[0],s.gp[1],s.gp[2],s.gp[3],s.gp[4],s.gp[5],s.gp[6],s.gp[7],
        s.flags.bits(),std::uint32_t(s.fp.count),std::uint32_t(s.fp.status),std::uint32_t(s.fp.top),xmm_digest,count};
    for(unsigned i=0;i<16;++i)w(s.memory,s.observe,0x7a0000u,at+4u*i,4,words[i]);
    w(s.memory,s.observe,0x7a0004u,log_base,4,count+1u);
}
inline std::uint32_t argument(ObservedWorldState &s,unsigned n){return r(s.memory,s.gp[4]+4u*n);}
// Scratch registers and flags change, as a real callee may.
inline void scratch(ObservedWorldState &s,std::uint32_t seed) {
    s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    s.xmm[1]=_mm_castsi128_ps(_mm_set_epi32(int(seed),int(seed*3u),int(seed^0x80000000u),int(seed+7u)));
}
// 2F36F0: update the child agents (stdcall, RET 10). It reads its arguments
// and the wrapper body, and may turn one child's agent into the null agent or
// back.
static void run_2efc30(ObservedWorldState &s) {
    log_entry(s,0x2f36f0u,0);
    const auto body=argument(s,1),wrapper=argument(s,2),process_input=argument(s,3),filter=argument(s,4);
    const auto shape=r(s.memory,wrapper),key=r(s.memory,wrapper+4u),motion=r(s.memory,wrapper+8u),parent=r(s.memory,wrapper+12u);
    const auto seed=body^shape^key^motion^parent^process_input^filter;
    const auto count=r(s.memory,agent+0x10u);
    if(count && (seed&3u)==0u) {
        const auto entry=children+12u*(seed%count);
        w(s.memory,s.observe,0x2f3700u,entry+8u,4,r(s.memory,entry+8u)==0x48cdc4u?child_agents:0x48cdc4u);
    }
    s.gp[0]=seed;scratch(s,seed);s.gp[4]+=0x14u;
}
// 304130: the weld (thiscall; contact, records, first, n, parameter; RET 14).
// Returns the record count, sometimes less, and sometimes shrinks the array.
static void run_300650(ObservedWorldState &s) {
    log_entry(s,0x304130u,0);
    const auto contact=argument(s,1),records=argument(s,2),first=argument(s,3),n=argument(s,4),parameter=argument(s,5);
    std::uint32_t seed=s.gp[1]^parameter^first*7u^n*13u;
    for(unsigned i=0;i<8;++i)seed=seed*0x9e3779b1u^r(s.memory,contact+4u*i);
    const auto size=r(s.memory,records+4u),data=r(s.memory,records);
    auto result=n;
    if(std::int32_t(n)>std::int32_t(first) && (seed>>3)%4u==0u)result=n-1u;
    if(std::int32_t(first)<std::int32_t(size))w(s.memory,s.observe,0x304140u,data+first*0x80u+0x7cu,1,seed&1u);
    if(size && (seed>>5)%11u==0u)w(s.memory,s.observe,0x304144u,records+4u,4,size-1u);
    s.gp[0]=result;scratch(s,seed);s.gp[4]+=0x18u;
}
// 30CD00: confirm a contact (cdecl, 7 arguments).
static void run_309210(ObservedWorldState &s) {
    log_entry(s,0x30cd00u,0);
    std::uint32_t seed=0;for(unsigned i=1;i<=7;++i)seed=seed*31u+argument(s,i);
    const auto child=argument(s,2);
    seed^=r(s.memory,child)^r(s.memory,child+4u)^r(s.memory,child+8u)^r(s.memory,child+12u);
    w(s.memory,s.observe,0x30cd10u,argument(s,4)+0x24u,4,seed);
    s.gp[0]=seed;scratch(s,seed);s.gp[4]+=4u;
}
// 30CF40: report a record (cdecl, 8 arguments): appended to the output.
static void run_309450(ObservedWorldState &s) {
    log_entry(s,0x30cf40u,0);
    std::uint32_t seed=0;for(unsigned i=1;i<=8;++i)seed=seed*31u+argument(s,i);
    const auto child=argument(s,2);
    seed^=r(s.memory,child)^r(s.memory,child+4u)^r(s.memory,child+8u)^r(s.memory,child+12u);
    const auto count=r(s.memory,output);
    w(s.memory,s.observe,0x30cf50u,output+8u+4u*(count%64u),4,seed^argument(s,4));
    w(s.memory,s.observe,0x30cf54u,output,4,count+1u);
    s.gp[0]=seed;scratch(s,seed);s.gp[4]+=4u;
}
// 2E3250: hkArrayUtil::_reserveMore (cdecl; array, element size): the
// records move to an owned block with twice the capacity.
static void run_2df780(ObservedWorldState &s) {
    log_entry(s,0x2e3250u,0);
    const auto array=argument(s,1),size=argument(s,2);
    const auto data=r(s.memory,array),count=r(s.memory,array+4u),capacity=r(s.memory,array+8u)&0x7fffffffu;
    for(std::uint32_t at=0;at<count*size && at<0x1000u;at+=4)w(s.memory,s.observe,0x2e3260u,grown+at,4,r(s.memory,data+at));
    w(s.memory,s.observe,0x2e3264u,array,4,grown);
    w(s.memory,s.observe,0x2e3268u,array+8u,4,capacity*2u+1u);
    s.gp[0]=array;scratch(s,count);s.gp[4]+=4u;
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    log_entry(s,target,site);
    const auto self=s.gp[1];
    if(target==allocate_stack_target) {
        const auto bytes=argument(s,1);
        const auto block=0x10a00000u+((bytes*3u+s.gp[0])&0x3f0u);
        w(s.memory,s.observe,0x7a0010u,self+0x14u,4,block);
        s.gp[0]=block;scratch(s,bytes);s.gp[4]+=8u;return;
    }
    if(target==deallocate_stack_target) {
        const auto block=argument(s,1),bytes=argument(s,2);
        w(s.memory,s.observe,0x7a0020u,self+0x14u,4,0u);
        w(s.memory,s.observe,0x7a0024u,self+0x10u,4,block^bytes);
        s.gp[0]=bytes;scratch(s,block);s.gp[4]+=12u;return;
    }
    if(target==deallocate_target) {
        const auto data=argument(s,1),bytes=argument(s,2),storage_class=argument(s,3);
        w(s.memory,s.observe,0x7a0030u,self+0x10u,4,data+bytes*storage_class);
        s.gp[0]=data^bytes;scratch(s,storage_class);s.gp[4]+=16u;return;
    }
    if(target==child_shape_target) {
        // getChildShape(key, buffer): builds the child in the buffer.
        const auto key=argument(s,1),buffer=argument(s,2);
        w(s.memory,s.observe,0x7b0100u,buffer,4,key^self);w(s.memory,s.observe,0x7b0104u,buffer+0xfcu,4,key);
        s.gp[0]=child_shapes+(key%6u)*0x40u;scratch(s,key);s.gp[4]+=12u;return;
    }
    if(target==type_target) {
        s.gp[0]=r(s.memory,self+4u);scratch(s,self);s.gp[4]+=4u;return;
    }
    if(target==predicate_target) {
        // (&verdict, dispatcher, type A, type B): cdecl.
        const auto result=argument(s,1),from=argument(s,2),type_a=argument(s,3),type_b=argument(s,4);
        const auto verdict=r(s.memory,from+4u+4u*((type_a^type_b)&3u))&0xffu;
        w(s.memory,s.observe,0x7b0110u,result,1,verdict);
        s.gp[0]=result;scratch(s,type_a*5u+type_b);s.gp[4]+=4u;return;
    }
    if(target==closest_target) {
        // (&found, bodyA, &child body, input, &contact): the contact's
        // fields come from a table chosen by the child body.
        const auto found=argument(s,1),a=argument(s,2),child=argument(s,3),process_input=argument(s,4),contact=argument(s,5);
        const auto shape=r(s.memory,child),key=r(s.memory,child+4u),motion=r(s.memory,child+8u),parent=r(s.memory,child+12u);
        const auto seed=(shape^key*0x9e3779b1u^motion^parent^a^process_input^self)*0x2545f491u;
        static constexpr std::uint32_t distances[]{0x3f800000u,0x40000000u,0x3f000000u,0xbf800000u,0x3f800000u,0x7fc00000u,
            0x7f812345u,0u,0x80000000u,0x00000001u,0x7f800000u,0x40400000u,0x3f800000u,0xc0000000u};
        for(unsigned i=0;i<20;++i)w(s.memory,s.observe,0x7b0120u,contact+4u*i,4,seed*(i+1u)^0x3f000000u);
        w(s.memory,s.observe,0x7b0124u,contact+0x20u,4,distances[(seed>>7)%std::size(distances)]);
        w(s.memory,s.observe,0x7b0128u,found,1,(seed>>12)%5u==0u?0u:(seed>>16)&0xffu|1u);
        s.gp[0]=found;scratch(s,seed);s.gp[4]+=0x18u;return;
    }
    if(target==process_target) {
        const auto a=argument(s,1),child=argument(s,2),process_input=argument(s,3),out=argument(s,4);
        const auto seed=a^r(s.memory,child)^r(s.memory,child+4u)^r(s.memory,child+8u)^r(s.memory,child+12u)^process_input^self;
        const auto count=r(s.memory,out);
        w(s.memory,s.observe,0x7b0130u,out+8u+4u*(count%64u),4,seed);
        w(s.memory,s.observe,0x7b0134u,out,4,count+1u);
        s.gp[0]=seed;scratch(s,seed);s.gp[4]+=0x14u;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "agent_bodies.inc"
#include "agent_reference.inc"
} // namespace t_agent
using t_agent::ObservedWorldState;

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
constexpr std::uint32_t stack=0x20010000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};};
Case author(std::mt19937 &rng,unsigned n) {
    using namespace t_agent;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    // The agent and its children: null or real child agents.
    const std::uint32_t count=kind==1?0u:kind==2?13u+pick(3):pick(13);
    m.put(agent,pick(0x1000));m.put(agent+4u,1u);m.put(agent+8u,manager);
    m.put(agent+0xcu,children);m.put(agent+0x10u,count);m.put(agent+0x14u,count);
    m.pages_at(children,1);
    for(std::uint32_t i=0;i<count && i<300u;++i) {
        m.put(children+12u*i,pick(4)?pick(12):std::uint32_t(rng()));m.put(children+12u*i+4u,std::uint32_t(rng()));
        m.put(children+12u*i+8u,pick(4)==0?0x48cdc4u:child_agents+pick(6)*0x200u);
    }
    // Child agents: the kind byte and 0-4 points.
    m.pages_at(child_agents,1);
    for(std::uint32_t k=0;k<6;++k) {
        const auto base=child_agents+k*0x200u;
        for(std::uint32_t at=0;at<0x200u;at+=4)m.put(base+at,special(rng));
        m.put(base,agent_vtable);m.put(base+0x14u,(pick(4)<<8)|std::uint32_t(rng())<<16);
        m.put(base+0x18u,kind==3?0xffffffffu:pick(5));
    }
    m.put(agent_vtable+0x10u,process_target);m.put(agent_vtable+0x20u,closest_target);
    // Bodies, shapes and the collection.
    m.put(body_a,shape_a);m.put(body_a+4u,std::uint32_t(rng()));m.put(body_a+8u,std::uint32_t(rng()));m.put(body_a+12u,std::uint32_t(rng()));
    m.put(shape_a,shape_vtable);m.put(shape_a+4u,pick(8));
    m.put(body_b,shape_b);m.put(body_b+4u,std::uint32_t(rng()));m.put(body_b+8u,motion_b);m.put(body_b+12u,std::uint32_t(rng()));
    m.put(shape_b,std::uint32_t(rng()));m.put(shape_b+0xcu,collection);m.put(collection+0xcu,inner);m.put(inner,inner_vtable);
    m.put(inner_vtable+0x34u,child_shape_target);m.put(shape_vtable+0x14u,type_target);
    for(std::uint32_t at=motion_b;at<motion_b+0x60u;at+=4)m.put(at,special(rng));
    for(std::uint32_t k=0;k<6;++k){m.put(child_shapes+k*0x40u,shape_vtable);m.put(child_shapes+k*0x40u+4u,pick(8));}
    // The input (dispatcher, weld parameter), the dispatcher's verdict table,
    // the output.
    m.put(input,dispatcher);m.put(input+4u,std::uint32_t(rng()));m.put(input+8u,std::uint32_t(rng()));m.put(input+0xcu,special(rng));
    for(unsigned i=0;i<4;++i)m.put(dispatcher+4u+4u*i,kind==4?0u:kind==5?1u:pick(3)?pick(2):std::uint32_t(rng()));
    m.pages_at(output,1);m.pages_at(manager,1);
    // Globals: the profiler stream, the predicate, the bias, the null agent.
    m.pages_at(monitor,1);
    const bool room=kind==6 || pick(4)==0;
    m.put(0x48c974u,monitor+pick(8)*12u);m.put(0x48c978u,room?monitor+0x800u:monitor+pick(8)*12u);
    m.put(0x46e46cu,predicate_target);m.put(0x46a180u,special(rng));
    // The thread memory: its stack and virtual functions.
    m.put(0x48c938u,thread);m.put(thread,thread_vtable);m.put(thread+8u,0x10b00000u+pick(0x10)*16u);
    m.put(thread+0xcu,kind==7?0x80u:0x4000u+pick(0x1000));m.put(thread+0x10u,0u);m.put(thread+0x14u,pick(2)?0u:0x10d00000u);
    m.put(thread_vtable+0x14u,deallocate_target);m.put(thread_vtable+0x24u,allocate_stack_target);
    m.put(thread_vtable+0x28u,deallocate_stack_target);
    m.pages_at(0x10b00000u,2);m.pages_at(0x10a00000u,2);m.pages_at(grown,2);m.pages_at(log_base,4);
    // The stack: the return address and four arguments, random below.
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x2f4000u+pick(0x100));m.put(entry+4u,body_a);m.put(entry+8u,body_b);m.put(entry+12u,input);m.put(entry+16u,output);
    m.put(fs,0x2001ff00u+pick(0x40)*4u);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[1]=agent;c.abi.gp[4]=entry;c.abi.fs_base=fs;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=special(rng);
    c.abi.x87_depth=kind==8?6u+pick(2):pick(4);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=static_cast<long double>(i)+0.5L;
    c.abi.x87_status=std::uint16_t((((8u-c.abi.x87_depth)&7u)<<11)|(pick(2)?0x4100u:0u)|(pick(2)?0x200u:0u));
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
    if(kind==15) {
        static constexpr std::uint32_t regions[]{agent,children,child_agents,body_a,body_b,motion_b,input,output,monitor,
            thread,0x10b00000u,grown,log_base,fs,child_shapes,inner_vtable,0x48c000u,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    std::uint64_t counter=0x123456789abcull;
    services.timestamp_counter=[&counter]{counter+=0x10203u;return counter;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native collection agent delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native collection agent delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    s.selected_observation_pcs=true;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        if(is_native)result.admitted=leaves::bridge_304350(s,[](auto &v){t_agent::native_world_leaves_reference_300870(v);},
            [](auto &v){t_agent::run_2efc30(v);},[](auto &v){t_agent::run_300650(v);},[](auto &v){t_agent::run_309210(v);},
            [](auto &v){t_agent::run_309450(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_agent::dispatch(v,target,site,false);},
            [](auto &v){t_agent::run_2df780(v);});
        else t_agent::run_300870(s);
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.fs_base==b.fs_base;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    return same;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    std::mt19937 rng(0x304350u+(census_mode?1u:0u)+(verify?2u:0u));
    std::uint64_t stores{},compared{},faults{},children{},records{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();
        for(const auto &e:reference.stores){children+=e.instruction==0x7a0004u;records+=e.instruction==0x3044fdu;}
        if(!reference.fault.empty())++faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u (kind %u) fault mismatch: native '%s' translated '%s'\n",n,c.kind,native_result.fault.c_str(),
                reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u (kind %u) store mismatch at %zu of %zu/%zu",n,c.kind,at,native_result.stores.size(),
                reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size()) {
                const auto &a=native_result.stores[at],&b=reference.stores[at];
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",a.instruction,a.address,b.instruction,b.address);
                if(a.instruction==b.instruction && a.address==b.address && a.written!=b.written) {
                    std::fprintf(stderr," bytes");
                    for(std::size_t i=0;i<a.written.size();++i)std::fprintf(stderr," %02x/%02x",unsigned(a.written[i]),unsigned(b.written[i]));
                }
            }
            std::fprintf(stderr,"\n");return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u (kind %u) cursor %x vs %x (%s)\n",n,c.kind,native_result.cursor,reference.cursor,reference.fault.c_str());
            return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || native_result.mxcsr!=reference.mxcsr || !left.same(right))) {
            std::fprintf(stderr,"case %u (kind %u) mismatch:%s%s%s",n,c.kind,native_result.mxcsr!=reference.mxcsr?" mxcsr":"",
                left.same(right)?"":" bank",native_result.abi.eflags!=reference.abi.eflags?" eflags":"");
            for(unsigned i=0;i<8;++i)if(native_result.abi.gp[i]!=reference.abi.gp[i])
                std::fprintf(stderr," gp%u %x/%x",i,native_result.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native_result.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr," xmm%u",i);
            if(native_result.abi.x87_status!=reference.abi.x87_status || native_result.abi.x87_depth!=reference.abi.x87_depth)
                std::fprintf(stderr," x87 %x/%x depth %u/%u",native_result.abi.x87_status,reference.abi.x87_status,
                    native_result.abi.x87_depth,reference.abi.x87_depth);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"child_calls\":%llu,"
        "\"gathered_records\":%llu,\"coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)records,(unsigned long long)leaves::coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
