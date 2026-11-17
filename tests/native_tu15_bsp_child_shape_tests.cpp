// Native TU1.5 1F7C10 BSP child shape (the structure collision shape's
// getChildShape(key, buffer)) against the unmodified translated canonical body
// run_1f3320, extracted by tools/validate_native_bsp_child_shape_tu15.py into
// child_bodies.inc with the unit's selected() operand cases
// (child_selected.inc) and the binder's verification reference
// (child_reference.inc).
//
// The translated children (1F76A0, 1F5FE0, 283450) and the dispatch targets
// (2E88B0, 184E20, the object's destructor, 1F67C0, 1F7330) are deterministic
// stand-ins, entered by both routes with the same state. Each appends its
// complete entry state (target, canonical site, general registers, flags, x87
// depth/status/TOP, an XMM digest) to a log in the fixture and reads its stack
// arguments, so a wrong register, flag or argument at a call reaches the store
// stream. They keep EBX, ESI, EDI and EBP, and change the scratch registers,
// flags and XMM registers. 1F76A0 answers from the key; 2E88B0 builds into the
// buffer and takes a reference on the object; 283450 answers from the object;
// the destructor marks the object; the others write the buffer.
//
// Seeded fixtures: keys of every kind 0-7 (indices in and out of the
// instance table), the filter's answer, buffers present or not, instances
// whose definition index is signed, collision objects whose reference count
// reaches zero (static or destroyed) or not (and wraps from zero), random
// register files, x87 depths, an active MMX state, every MXCSR mode and fault
// injection with one fixture page removed. Each case compares the ordered
// store events (pc, address, preimage, bytes), the final bank, registers,
// flags, XMM, x87, MXCSR and the cursor; fault cases compare the exception,
// the cursor and the bank.
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

namespace t_child {
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
#include "child_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses (the globals at their TU1.5 literals).
inline constexpr std::uint32_t globals_page=0x4ec000u,structure=0x10100000u,instances=0x10110000u,definitions=0x10120000u,
    collision=0x10130000u,objects=0x10140000u,vtables=0x10150000u,bsp=0x10160000u,buffer_base=0x10170000u,
    log_base=0x10900000u,fs=0x10700000u;
inline constexpr std::uint32_t build_target=0x2e88b0u,place_target=0x184e20u,surface_target=0x1f67c0u,other_target=0x1f7330u,
    destructor=0x7d0000u;
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
inline void scratch(ObservedWorldState &s,std::uint32_t seed) {
    s.gp[1]^=seed*0x45d9f3bu;s.gp[2]=seed^0x5a5a5a5au;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    for(unsigned i=1;i<8;i+=1+(seed>>i&1u))
        s.xmm[i]=_mm_castsi128_ps(_mm_set_epi32(int(seed*i),int(seed*3u+i),int(seed^0x80000000u),int(seed+7u*i)));
}
// 1F76A0 (EDX = the key; AL back).
static void run_1f2db0(ObservedWorldState &s) {
    const auto key=s.gp[2],seed=key*0x9e3779b1u^r(s.memory,globals_page+0xf00u);
    log_entry(s,0x1f76a0u,0,seed);
    const auto keep_ebx=s.gp[3];
    s.gp[0]=(seed&0xffffff00u)|((seed>>11)%5u!=0u?1u:0u);scratch(s,seed);s.gp[3]=keep_ebx;s.gp[4]+=4u;
}
// 1F5FE0 (ECX = the shape, EDI = the matrix; BSP, kind, owner, index, key;
// RET 14): EAX = the shape.
static void run_1f16f0(ObservedWorldState &s) {
    const auto shape=s.gp[1];
    std::uint32_t seed=shape^s.gp[7]*31u;
    for(unsigned i=1;i<=5;++i)seed=seed*0x9e3779b1u^argument(s,i);
    log_entry(s,0x1f5fe0u,0,seed);
    w(s.memory,s.observe,0x1f6000u,shape,4,0x4620d0u);w(s.memory,s.observe,0x1f6004u,shape+0x14u,4,seed);
    scratch(s,seed);s.gp[0]=shape;s.gp[4]+=0x18u;
}
// 283450 (cdecl; the object): AL back.
static void run_27f9f0(ObservedWorldState &s) {
    const auto object=argument(s,1),seed=object*0x2545f491u^r(s.memory,object+8u);
    log_entry(s,0x283450u,0,seed);
    s.gp[0]=(seed&0xffffff00u)|((seed>>7)&1u);scratch(s,seed);s.gp[4]+=4u;
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    if(target==build_target) {
        // buffer->build(object): thiscall, RET 4; the shape takes a reference.
        const auto shape=s.gp[1],object=argument(s,1),seed=shape^object*7u^r(s.memory,object);
        log_entry(s,target,site,seed);
        w(s.memory,s.observe,0x2e88c0u,shape,4,0x4a0000u+(seed&0xffu));w(s.memory,s.observe,0x2e88c4u,shape+0x10u,4,object);
        if(seed%3u)w(s.memory,s.observe,0x2e88c8u,object+6u,2,(r(s.memory,object+6u,2)+1u)&0xffffu);
        scratch(s,seed);s.gp[0]=seed%11u==0u?0u:shape;s.gp[4]+=8u;return;
    }
    if(target==place_target) {
        // place(EAX = the transform, ECX = the instance): RET.
        const auto transform=s.gp[0],instance=s.gp[1],seed=transform^r(s.memory,instance+0x10u)*13u;
        log_entry(s,target,site,seed);
        if(transform>=0x10000u)w(s.memory,s.observe,0x184e30u,transform,4,r(s.memory,instance+0x14u));
        scratch(s,seed);s.gp[0]=seed;s.gp[4]+=4u;return;
    }
    if(target==destructor) {
        // object->~(1): thiscall, RET 4.
        const auto object=s.gp[1],seed=object^argument(s,1)*0x10001u;
        log_entry(s,target,site,seed);
        w(s.memory,s.observe,0x7d0010u,object,4,0xdeadu);
        scratch(s,seed);s.gp[0]=seed;s.gp[4]+=8u;return;
    }
    if(target==surface_target) {
        // (ECX = the buffer, EDX = the key, EAX = the collision data, EDI = the
        // instance; the secondary index): RET 4.
        const auto shape=s.gp[1],seed=shape^s.gp[2]*3u^s.gp[0]*5u^s.gp[7]*9u^argument(s,1);
        log_entry(s,target,site,seed);
        if(shape)w(s.memory,s.observe,0x1f67d0u,shape,4,seed);
        scratch(s,seed);s.gp[0]=shape;s.gp[4]+=8u;return;
    }
    if(target==other_target) {
        // (the key, the buffer): RET 8.
        const auto key=argument(s,1),shape=argument(s,2),seed=key*0x9e3779b1u^shape;
        log_entry(s,target,site,seed);
        if(shape)w(s.memory,s.observe,0x1f7340u,shape+4u,4,seed);
        scratch(s,seed);s.gp[0]=shape^key;s.gp[4]+=0xcu;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "child_bodies.inc"
#include "child_reference.inc"
} // namespace t_child
using t_child::ObservedWorldState;

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
    c.abi.gp[4]=entry;c.abi.fs_base=t_child::fs;
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
Case author_child(std::mt19937 &rng,unsigned n) {
    using namespace t_child;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    m.pages_at(globals_page,1);m.pages_at(structure,1);m.pages_at(instances,1);m.pages_at(definitions,1);m.pages_at(collision,1);
    m.pages_at(objects,1);m.pages_at(vtables,1);m.pages_at(bsp,1);m.pages_at(buffer_base,1);m.pages_at(log_base,5);
    m.put(globals_page+0xf00u,std::uint32_t(rng()));  // the filter stand-in's seed
    m.put(0x4ecc10u,bsp);m.put(0x4ecc18u,structure);
    for(std::uint32_t at=0;at<0x200u;at+=4)m.put(structure+at,std::uint32_t(rng()));
    m.put(structure+0x144u,instances);m.put(structure+0x13cu,definitions);
    // Eight instances with signed definition indices; four definitions, each
    // with collision data whose object at +40 is referenced 0-3 times.
    for(std::uint32_t at=0;at<0x58u*8u;at+=4)m.put(instances+at,std::uint32_t(rng()));
    for(unsigned i=0;i<8;++i)m.put(instances+i*0x58u+0x34u,(std::uint32_t(rng())&0xffff0000u)|(pick(8)?pick(4):0xffffu));
    for(std::uint32_t at=0;at<0xc8u*4u;at+=4)m.put(definitions+at,std::uint32_t(rng()));
    for(unsigned d=0;d<4;++d) {
        const auto data=collision+d*0x100u;
        m.put(definitions+d*0xc8u+0xb4u,data);
        for(std::uint32_t at=0;at<0x60u;at+=4)m.put(data+at,std::uint32_t(rng()));
        m.put(data+0x40u,vtables+d*0x10u);
        const std::uint32_t counts[]{1u,1u,2u,0u,3u};
        m.put(data+0x44u,(counts[pick(5)]<<16)|pick(0x10000));
        m.put(vtables+d*0x10u,destructor);
    }
    // The definition at index -1 wraps below the table (it faults there).
    const std::uint32_t kinds=kind<8?kind:pick(8);
    const auto index=pick(10)?pick(8):pick(0x10000);
    const std::uint32_t key=(kinds<<29)|(pick(0x2000)<<16)|index;
    const std::uint32_t buffer=pick(5)?buffer_base+0x40u*pick(16):0u;
    for(std::uint32_t at=0;at<0x400u;at+=4)m.put(buffer_base+at,std::uint32_t(rng()));
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x1f6f4cu);m.put(entry+4u,key);m.put(entry+8u,buffer);
    m.put(t_child::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    if(kind==15) {
        static constexpr std::uint32_t regions[]{globals_page,structure,instances,definitions,collision,vtables,buffer_base,
            log_base,stack-0x1000u};
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
        services.instruction=[](auto){throw std::runtime_error("Native BSP child shape delivered a translated instruction");};
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
    Totals child;
    if(compare("1F7C10",cases,0x1f7c10u+variant,child,author_child,
        [](ObservedWorldState &s){return leaves::bridge_1f7c10(s,[](auto &v){t_child::native_world_leaves_reference_1f3320(v);},
            [](auto &v){t_child::run_1f2db0(v);},[](auto &v){t_child::run_1f16f0(v);},[](auto &v){t_child::run_27f9f0(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_child::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_child::run_1f3320(s);}))return 1;
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)child.compared,(unsigned long long)child.stores,(unsigned long long)child.faults,
        (unsigned long long)child.children,(unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),
        mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
