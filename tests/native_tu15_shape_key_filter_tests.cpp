// Native TU1.5 1F76A0 shape key filter (whether the child shape a shape key
// names may still collide) against the unmodified translated canonical body
// run_1f2db0, extracted by tools/validate_native_shape_key_filter_tu15.py into
// filter_bodies.inc with the unit's selected() operand cases
// (filter_selected.inc), its canonical_branch() (filter_branch.inc) and the
// binder's verification reference (filter_reference.inc).
//
// 185AA0 (the breakable surface's state) is a deterministic stand-in entered
// by both routes through the dispatch; it logs its complete entry state
// (registers, flags, x87, an XMM digest) into the fixture, answers from its
// index and changes the scratch registers, flags and XMM.
//
// Seeded fixtures: keys of every kind 0-7 (kinds 0 and 5-7 read the jump
// table's neighbours: zero, another table's target or an unknown value),
// instanced surfaces (breakable or not; signed definition indices), structure
// surfaces with the breakable state bit set or clear (negative sets too),
// object regions (missing handles, excluded headers, matching and other
// permutations, signed region offsets and bytes), random register files, x87
// depths, an active MMX state, every MXCSR mode and one fixture page removed.
// Each case compares the ordered store events, the bank, the registers,
// flags, XMM, x87, MXCSR and the cursor; faults by exception, cursor and bank.
// Modes: <cases> [census|verify].
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

namespace t_filter {
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
#include "filter_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
#include "filter_branch.inc"
inline constexpr std::uint32_t fs=0x10700000u,log_base=0x10900000u,structure=0x10100000u,instances=0x10110000u,
    definitions=0x10120000u,surfaces=0x10130000u,collision=0x10140000u,collision_surfaces=0x10150000u,bits=0x10160000u,
    handles=0x10170000u,objects=0x10180000u,headers=0x10190000u,object_blocks=0x101a0000u;
inline constexpr std::uint32_t breakable_target=0x186e70u;
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
// 185AA0: the breakable surface's state, from EAX; returns AL.
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    log_entry(s,target,site);
    if(target!=breakable_target){char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
        throw std::runtime_error(text);}
    const auto seed=s.gp[0]*0x9e3779b1u^0x5bd1e995u;
    s.gp[0]=(seed&0xffffff00u)|((seed>>13)%3u==0u?0u:(seed>>20)&0xffu|1u);
    s.gp[1]^=seed;s.gp[2]=seed^0x12345678u;s.flags.load((seed*0x2c1b3c6du)&0x8d5u);
    s.xmm[2]=_mm_castsi128_ps(_mm_set_epi32(int(seed),int(seed*3u),int(seed^0x80000000u),int(seed+7u)));
    s.gp[4]+=4u;
}
#include "filter_bodies.inc"
#include "filter_reference.inc"
} // namespace t_filter
using t_filter::ObservedWorldState;

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
constexpr std::uint32_t stack=0x20010000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};};
// A random register file, x87 stack and MXCSR mode; the stack's arguments.
void registers(Case &c,std::mt19937 &rng,std::uint32_t entry,unsigned kind) {
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=entry;c.abi.fs_base=t_filter::fs;
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

// ---- 1F76A0
Case author_filter(std::mt19937 &rng,unsigned n) {
    using namespace t_filter;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    // The jump table and its neighbours.
    m.pages_at(0x1f7000u,1);
    const std::uint32_t targets[]{0x1f76c1u,0x1f7711u,0x1f775bu,0x1f77a7u};
    for(unsigned i=0;i<4;++i)m.put(0x1f77acu+4u*i,targets[i]);
    const std::uint32_t neighbours[]{0u,0x231000u+pick(0x100),std::uint32_t(rng()),0x1f2dd1u,0x23100bu};
    for(const auto at:{0x1f77a8u,0x1f77bcu,0x1f77c0u,0x1f77c4u})m.put(at,neighbours[pick(std::size(neighbours))]);
    // The globals.
    m.pages_at(0x4ec000u,1);m.pages_at(0x474000u,1);m.pages_at(0x4f9000u,1);m.pages_at(0x52b000u,1);
    m.put(0x4ecc18u,structure);m.put(0x4ecc10u,collision);m.put(0x474c24u,(pick(4)?pick(3):0xfffeu+pick(2))|(std::uint32_t(rng())<<16));
    m.put(0x4f9b50u,bits+pick(4)*0x40u);m.put(0x52be70u,handles);m.put(0x4ecbd0u,objects);
    for(const auto page:{structure,instances,definitions,surfaces,collision,collision_surfaces,bits,handles,objects,headers,
                         object_blocks})for(std::uint32_t at=0;at<0x1000u;at+=4)m.put(page+at,std::uint32_t(rng()));
    m.pages_at(bits+0x1000u,1);for(std::uint32_t at=0;at<0x1000u;at+=4)m.put(bits+0x1000u+at,std::uint32_t(rng()));
    m.put(structure+0x144u,instances);m.put(structure+0x13cu,definitions);
    for(std::uint32_t i=0;i<32;++i)m.put(instances+i*0x58u+0x34u,(pick(6)?pick(8):0xfffcu+pick(4))|(std::uint32_t(rng())<<16));
    for(std::uint32_t i=0;i<8;++i)m.put(definitions+i*0xc8u+0x9cu,surfaces+i*0x80u);
    m.pages_at(definitions-0x1000u,1);
    for(std::uint32_t i=1;i<=4;++i)m.put(definitions-i*0xc8u+0x9cu,surfaces+0x800u);
    m.put(collision+0x2cu,collision_surfaces);
    for(std::uint32_t i=0;i<0x200u;++i){
        m.put(surfaces+i*8u+4u,(pick(2)?0x08u:pick(256))|pick(256)<<8|std::uint32_t(rng())<<16);
        m.put(collision_surfaces+i*8u+4u,(pick(2)?0x08u:pick(256))|pick(256)<<8|std::uint32_t(rng())<<16);
    }
    m.put(objects+0x44u,headers);
    for(std::uint32_t i=0;i<64;++i) {
        m.put(handles+4u*i,pick(5)==0?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(16));
        m.put(headers+i*12u,(pick(4)==0?0x10u<<16:pick(2)?0u:std::uint32_t(rng())&0x00ef0000u)|pick(0x10000));
        m.put(headers+i*12u+8u,object_blocks+pick(8)*0x200u);
    }
    for(std::uint32_t i=0;i<8;++i)m.put(object_blocks+i*0x200u+0x118u,(pick(4)?0x40u+pick(0x40):0xffc0u+pick(0x40))<<16);
    // The key.
    const std::uint32_t key_kind=kind<12?1u+kind%4u:kind==12?0u:kind==13?5u+pick(3):1u+pick(4);
    const std::uint32_t index=pick(8)?pick(kind==3?64u:32u):pick(0x10000);
    const std::uint32_t secondary=key_kind==3?(pick(4)?(pick(32)|std::uint32_t(rng()&0xff)<<5):pick(0x2000)):pick(6)?pick(16):pick(0x2000);
    const std::uint32_t key=key_kind<<29|secondary<<16|index;
    // Kind 3: the region bytes often hold the key's permutation (half of
    // them 80..FF, which the signed byte never equals).
    for(std::uint32_t i=0;i<8;++i) {
        const auto object=object_blocks+i*0x200u;
        const auto offset=std::uint32_t(std::int32_t(std::int16_t(r(m,object+0x118u)>>16)));
        const auto at=object+offset+(secondary&31u);
        if(pick(2) && m.pages.count(at>>12))m.page(at,true)[at&4095u]=std::byte((secondary>>5)&0xffu);
    }
    m.pages_at(log_base,4);
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x1f6e00u+pick(0x100));
    m.put(t_filter::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    c.abi.gp[2]=key;
    if(kind==15) {
        static constexpr std::uint32_t regions[]{structure,instances,definitions,surfaces,collision,collision_surfaces,bits,
            handles,objects,headers,object_blocks,0x4ec000u,0x52b000u,log_base,stack-0x1000u};
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
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native shape key filter delivered a translated instruction");};
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
    Totals filter;
    if(compare("1F76A0",cases,0x1f76a0u+variant,filter,author_filter,
        [](ObservedWorldState &s){return leaves::bridge_1f76a0(s,[](auto &v){t_filter::native_world_leaves_reference_1f2db0(v);},
            [&s](std::uint32_t target){(void)t_filter::canonical_branch(target,s.image);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_filter::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_filter::run_1f2db0(s);}))return 1;
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,\"coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)filter.compared,(unsigned long long)filter.stores,(unsigned long long)filter.faults,
        (unsigned long long)filter.children,(unsigned long long)leaves::coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
