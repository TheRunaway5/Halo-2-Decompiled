// Native TU1.5 1F6D10 structure AABB query (the structure collision shape's
// queryAabb(aabb, hits), with 2E4550 inline) against the unmodified
// translated canonical body run_1f2420, extracted by
// tools/validate_native_structure_aabb_query_tu15.py into query_bodies.inc
// (with the translated 2E4550 the reference runs) with the unit's selected()
// operand cases (query_selected.inc) and the binder's verification reference
// (query_reference.inc).
//
// The translated children (2E9670, 1F6FA0, 1F76A0) and the definition's
// queryObb (a dispatch target) are deterministic stand-ins, entered by both
// routes with the same state. Each appends its complete entry state (target,
// canonical site, general registers, flags, x87 depth/status/TOP, an XMM
// digest) to a log in the fixture and reads its arguments, so a wrong
// register, flag or argument at a call reaches the store stream. They keep
// EBX, ESI, EDI and EBP, and change the scratch registers, flags and XMM
// registers. 2E9670 appends keys of every kind to the hits; 1F6FA0 sometimes
// drops the last key; 1F76A0 answers from the key; the queryObb reads the
// transform and the extents and appends surface keys (some with bits 13-15
// set), or none.
//
// Seeded fixtures: hits empty or not before the query, keys of every kind
// (instance keys with indices in and out of the table), definitions whose
// surface count is around 0x2000 (and negative), instance matrices of random
// and special floats (NaN payloads, SNaN, infinities, denormals, signed
// zeros; NaN payloads in both operands of the box's arithmetic), AABBs at
// varied alignments, the box constants special or not, a hits header inside
// its own key data (a removal's shift then overwrites the size it re-reads),
// random register files, x87 depths, an active MMX state, every MXCSR mode and
// fault injection with one fixture page removed. Each case compares the
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

namespace t_query {
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
#include "query_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
// Fixture addresses (the globals at their TU1.5 literals).
inline constexpr std::uint32_t globals_page=0x4ec000u,constants=0x46a000u,structure=0x10100000u,instances=0x10110000u,
    definitions=0x10120000u,collision=0x10130000u,vtables=0x10140000u,hits_base=0x10150000u,data=0x10160000u,aabb_base=0x10170000u,
    log_base=0x10900000u,fs=0x10700000u;
inline constexpr std::uint32_t query_target=0x7d0000u,data_capacity=0x400u;
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
// A key of any kind from a hash: mostly instance keys (kind 2) with indices
// in the instance table, sometimes outside it.
inline std::uint32_t planned_key(std::uint32_t h) {
    static constexpr std::uint32_t kinds[]{2u,2u,2u,1u,3u,4u,5u,0u};
    const auto index=(h>>8)%40u?(h>>12)%8u:(h>>12)&0xffffu;
    return (kinds[h%8u]<<29)|(((h>>16)&0x1fffu)<<16)|index;
}
// Append n keys to the hits (the data never grows past its capacity).
inline void append(ObservedWorldState &s,std::uint32_t hits,std::uint32_t n,std::uint32_t seed,bool surface) {
    auto size=r(s.memory,hits+4u);
    for(std::uint32_t i=0;i<n && size<data_capacity;++i,++size) {
        const auto h=(seed+i*0x9e3779b9u)*0x85ebca6bu;
        const auto key=surface?((h>>4)&0x1fffu)|((h>>20)%7u==0u?h&0xe000u:0u):planned_key(h);
        w(s.memory,s.observe,0x7b0000u,r(s.memory,hits)+4u*size,4,key);
    }
    w(s.memory,s.observe,0x7b0004u,hits+4u,4,size);
}
// 2E9670 (canonical 2E5BC0; thiscall on the shape; the AABB, the hits; RET 8).
static void run_2e5bc0(ObservedWorldState &s) {
    const auto shape=s.gp[1],aabb=argument(s,1),hits=argument(s,2);
    const auto seed=fold(s,shape^hits*31u,aabb,8);
    log_entry(s,0x2e9670u,0,seed);
    append(s,hits,(seed>>3)%7u,seed,false);
    scratch(s,seed);s.gp[4]+=0xcu;
}
// 1F6FA0 (canonical 1F26B0; ESI = the hits; the shape; RET 4): sometimes
// drops the last key.
static void run_1f26b0(ObservedWorldState &s) {
    const auto hits=s.gp[6],shape=argument(s,1);
    const auto seed=fold(s,shape*7u^hits,hits,3)^r(s.memory,globals_page+0xf00u);
    log_entry(s,0x1f6fa0u,0,seed);
    const auto size=r(s.memory,hits+4u);
    if(seed%3u==0u && std::int32_t(size)>0)w(s.memory,s.observe,0x1f6fb0u,hits+4u,4,size-1u);
    scratch(s,seed);s.gp[4]+=8u;
}
// 1F76A0 (EDX = the key; AL back).
static void run_1f2db0(ObservedWorldState &s) {
    const auto key=s.gp[2],seed=key*0x9e3779b1u^r(s.memory,globals_page+0xf00u);
    log_entry(s,0x1f76a0u,0,seed);
    const auto keep_ebx=s.gp[3];
    // Rarely the hits' size goes negative (the original re-reads it and
    // compares it signed).
    if(seed%37u==0u)w(s.memory,s.observe,0x1f76b0u,s.gp[6]+4u,4,0x80000000u|(seed>>8&3u?seed>>16:0x7fffffffu));
    scratch(s,seed);s.gp[0]=(seed&0xffffff00u)|((seed>>11)%4u!=0u?1u:0u);s.gp[3]=keep_ebx;s.gp[4]+=4u;
}
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    if(target==query_target) {
        // shape->queryObb(transform, extents, tolerance, hits): thiscall, RET 10.
        const auto shape=s.gp[1],transform=argument(s,1),extents=argument(s,2),tolerance=argument(s,3),hits=argument(s,4);
        auto seed=fold(s,shape^tolerance,transform,16);seed=fold(s,seed,extents,4);
        log_entry(s,target,site,seed);
        append(s,hits,(seed>>5)%5u,seed,true);
        scratch(s,seed);s.gp[4]+=0x14u;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "query_bodies.inc"
#include "query_reference.inc"
} // namespace t_query
using t_query::ObservedWorldState;

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
    c.abi.gp[4]=entry;c.abi.fs_base=t_query::fs;
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
Case author_query(std::mt19937 &rng,unsigned n) {
    using namespace t_query;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    m.pages_at(globals_page,1);m.pages_at(constants,1);m.pages_at(structure,1);m.pages_at(instances,1);m.pages_at(definitions,1);
    m.pages_at(collision,1);m.pages_at(vtables,1);m.pages_at(hits_base,1);m.pages_at(data,1);m.pages_at(aabb_base,1);
    m.pages_at(log_base,5);m.pages_at(0,1);
    m.put(globals_page+0xf00u,std::uint32_t(rng()));  // the stand-ins' seed
    m.put(0x4ecc18u,structure);
    // Kind 12: NaN payloads in the matrices, the half factor and the AABB, so
    // that both operands of the box's MULPS and ADDPS are NaNs.
    const auto payload=[&]{return 0x7f800001u|(std::uint32_t(rng())&0x803fffffu);};
    m.put(0x46a0ccu,kind==12?payload():pick(6)?0x3f000000u:special(rng));m.put(0x46a0d0u,pick(6)?0x3f800000u:special(rng));
    for(std::uint32_t at=0;at<0x200u;at+=4)m.put(structure+at,std::uint32_t(rng()));
    // Kind 14: instance 0 starts 8 bytes below a page whose predecessor is
    // missing (its forward axis faults after the left axis is read).
    const auto table=kind==14?instances+0x1000u-8u:instances;
    if(kind==14)m.pages_at(instances+0x1000u,1);
    m.put(structure+0x144u,table);m.put(structure+0x13cu,definitions);
    // Eight instances (special and small floats; signed definition indices
    // into four definitions, -1 wraps below the table).
    const bool nan_payloads=kind==12;
    for(std::uint32_t at=0;at<0x58u*8u;at+=4)
        m.put(table+at,nan_payloads?payload():pick(3)?std::bit_cast<std::uint32_t>(small(rng,50.0f)):special(rng));
    for(unsigned i=0;i<8;++i)m.put(table+i*0x58u+0x34u,(std::uint32_t(rng())&0xffff0000u)|(pick(30)?pick(4):0xffffu));
    static constexpr std::uint32_t counts[]{0u,1u,0x1fffu,0x2000u,0x2001u,0x7fffffffu,0x80000000u,0x40u,0x40u,0x40u};
    for(unsigned d=0;d<4;++d) {
        for(std::uint32_t at=0;at<0xc8u;at+=4)m.put(definitions+d*0xc8u+at,std::uint32_t(rng()));
        m.put(definitions+d*0xc8u+0x98u,kind==11 || kind==14?pick(0x2001):counts[pick(std::size(counts))]);
        // The collision data is sometimes near the top of the address space
        // (its MOPP shape at +50 wraps, and ADD ECX,50 carries).
        m.put(definitions+d*0xc8u+0xb4u,d==3 && pick(3)==0?0xffffffc0u+0x10u*pick(3):collision+d*0x100u);
        for(std::uint32_t at=0;at<0x100u;at+=4)m.put(collision+d*0x100u+at,std::uint32_t(rng()));
        m.put(collision+d*0x100u+0x50u,vtables+d*0x40u);
        for(std::uint32_t at=0;at<0x40u;at+=4)m.put(vtables+d*0x40u+at,std::uint32_t(rng()));
        m.put(vtables+d*0x40u+0x2cu,query_target);
    }
    for(std::uint32_t at=0;at<0x40u;at+=4)m.put(at,vtables+3u*0x40u);  // the wrapped shapes' vtable
    // The hits: 0-6 keys already present (none for kind 10).
    // Kind 13: the hits' header lies inside its own key data. Either the whole
    // header is among the keys (a key moved down overwrites the data
    // pointer), or the header starts one word below them: the size is then
    // key 0, an instance key, and the capacity key 1. Removing key 0 moves the
    // capacity (negative, or a small count) into the size, which the removal
    // re-reads as its bound.
    const bool header_below=kind==13 && pick(2);
    const auto hits=header_below?data-4u:kind==13?data+4u*pick(6):hits_base+0x10u*pick(16);
    for(std::uint32_t at=0;at<0x1000u;at+=4)m.put(data+at,std::uint32_t(rng()));
    const auto present=kind==10?0u:kind==14?3u:pick(7);
    for(std::uint32_t i=0;i<present;++i)m.put(data+4u*i,kind==14 && i==0?0x40000000u:planned_key(std::uint32_t(rng())));
    m.put(hits,data);
    m.put(hits+4u,header_below?0x40000000u|pick(8):kind==10 && pick(2)?0xfffffffeu:present);
    m.put(hits+8u,header_below && pick(2)?2u+pick(4):0x80000000u|data_capacity);
    // The AABB at varied alignments.
    const auto aabb=aabb_base+0x10u*pick(32)+(pick(8)?0u:4u*pick(4));
    for(std::uint32_t at=0;at<0x20u;at+=4)
        m.put(aabb+at,nan_payloads && pick(2)?payload():pick(4)?std::bit_cast<std::uint32_t>(small(rng,100.0f)):special(rng));
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x2f3de0u);m.put(entry+4u,aabb);m.put(entry+8u,hits);
    m.put(t_query::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    if(kind==15) {
        static constexpr std::uint32_t regions[]{globals_page,constants,structure,instances,definitions,collision,vtables,hits_base,data,
            aabb_base,log_base,stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    if(kind==14)m.pages.erase(instances>>12);
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
        services.instruction=[](auto){throw std::runtime_error("Native structure AABB query delivered a translated instruction");};
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
    Totals query;
    if(compare("1F6D10",cases,0x1f6d10u+variant,query,author_query,
        [](ObservedWorldState &s){return leaves::bridge_1f6d10(s,[](auto &v){t_query::native_world_leaves_reference_1f2420(v);},
            [](auto &v){t_query::run_2e5bc0(v);},[](auto &v){t_query::run_1f26b0(v);},[](auto &v){t_query::run_1f2db0(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_query::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_query::run_1f2420(s);}))return 1;
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)query.compared,(unsigned long long)query.stores,(unsigned long long)query.faults,
        (unsigned long long)query.children,(unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),
        mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
