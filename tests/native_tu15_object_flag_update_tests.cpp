// Native TU1.5 E8D80 object flag update (an entry of the vehicle update unit:
// an object's 60 flags, kept or cleared by their update functions) against
// the unmodified translated canonical body run_e7e60, extracted by
// tools/validate_native_object_flag_update_tu15.py into flags_bodies.inc with
// the unit's selected() operand cases (flags_selected.inc) and the binder's
// verification reference (flags_reference.inc). The unit publishes canonical
// instructions, so both routes report E7E60..E7EE6.
//
// The update functions are one deterministic stand-in (a dispatch target),
// entered by both routes with the same state: it appends its complete entry
// state (target, canonical site, general registers, flags, x87, an XMM
// digest) to a log in the fixture, reads its two stack arguments, sometimes
// sets or clears another flag of the object (the original re-reads the word
// before a clear), keeps EBX, ESI, EDI and EBP, and answers in AL from its
// inputs.
//
// Seeded fixtures: handles with upper garbage (and bit 15 of the index), objects
// whose flag block lies
// before or after them (signed offsets), flag words of every density, flags'
// definitions with an update function, none, or an unbound target, a block
// that straddles a missing page, random register files, x87 depths, an
// active MMX state, every MXCSR mode and fault injection with one fixture
// page removed. Each case compares the ordered store events (pc, address,
// preimage, bytes), the final bank, registers, flags, XMM, x87, MXCSR and the
// cursor; fault cases compare the exception, the cursor and the bank.
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

namespace t_flags {
struct VehicleState : halo2::physics_world_runtime_detail::WorldState {
    using halo2::physics_world_runtime_detail::WorldState::WorldState;
};
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image) {
    if(image==SourcePhysicsContactImage::MapPack11)return p;
    switch(p) {
#include "flags_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
inline constexpr std::uint32_t headers=0x10100000u,objects=0x10110000u,definitions_page=0x473000u,definition_records=0x10120000u,
    log_base=0x10900000u,fs=0x10700000u,update_target=0x7d0000u;
inline void log_entry(VehicleState &s,std::uint32_t target,std::uint32_t site,std::uint32_t extra=0) {
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
inline std::uint32_t argument(VehicleState &s,unsigned n){return r(s.memory,s.gp[4]+4u*n);}
static void dispatch(VehicleState &s,std::uint32_t target,std::uint32_t site,bool) {
    if(target==update_target) {
        // update(handle, flag): stdcall, RET 8; AL: keep the flag.
        const auto handle=argument(s,1),flag=argument(s,2);
        const auto seed=(handle*0x9e3779b1u)^(flag*0x85ebca6bu)^r(s.memory,headers+0xf00u)^s.gp[3]*7u^s.gp[6]*13u;
        log_entry(s,target,site,seed);
        // Sometimes another flag of the object changes (the block is EBX+4).
        if(seed%7u==0u) {
            const auto word=s.gp[3]+4u+((seed>>8)&4u);
            w(s.memory,s.observe,0x7b0000u,word,4,r(s.memory,word)^(1u<<((seed>>11)&31u)));
        }
        const auto ebx=s.gp[3];
        s.gp[0]=(seed&0xffffff00u)|((seed>>17)%3u?1u+((seed>>20)&0xfeu):0u);s.gp[1]^=seed;s.gp[2]=seed^0x5a5a5a5au;
        s.flags.load((seed*0x2c1b3c6du)&0x8d5u);s.gp[3]=ebx;
        for(unsigned i=1;i<8;i+=2)s.xmm[i]=_mm_castsi128_ps(_mm_set1_epi32(int(seed*i)));
        s.gp[4]+=0xcu;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "flags_bodies.inc"
#include "flags_reference.inc"
} // namespace t_flags
using ObservedWorldState=t_flags::VehicleState;

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
    c.abi.gp[4]=entry;c.abi.fs_base=t_flags::fs;
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

Case author_flags(std::mt19937 &rng,unsigned n) {
    using namespace t_flags;
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;
    m.pages_at(0x4ec000u,1);m.pages_at(headers,1);m.pages_at(objects,3);m.pages_at(definitions_page,1);m.pages_at(definition_records,1);
    m.pages_at(log_base,5);
    m.put(headers+0xf00u,std::uint32_t(rng()));  // the stand-in's seed
    m.put(0x4ecbd0u,headers);m.put(headers+0x44u,headers+0x100u);
    // Eight objects; a handle's upper half is garbage. An eighth of the
    // handles index the headers' second run at 8000 (bit 15 of the index).
    const auto index=pick(8)|(pick(8)?0u:0x8000u);
    for(std::uint32_t i=0;i<8;++i) {
        m.put(headers+0x100u+12u*i+8u,objects+0x1000u+0x100u*i);
        m.put(headers+0x100u+12u*(0x8000u+i)+8u,objects+0x1000u+0x100u*((i+3u)&7u));
    }
    for(std::uint32_t at=0;at<0x3000u;at+=4)m.put(objects+at,std::uint32_t(rng()));
    const auto object=objects+0x1000u+0x100u*(index&0x8000u?(index+3u)&7u:index);
    // The flag block: after the object, before it (negative offset), or (kind
    // 14) straddling a missing page.
    const std::int32_t offset=kind==14?std::int32_t(objects+0x2000u-6u)-std::int32_t(object):
        kind==12?-std::int32_t(0x20u*(1u+pick(8))):std::int32_t(0x350u+4u*pick(16));
    m.put(object+0x344u,(std::uint32_t(offset)<<16)|(rng()&0xffffu));
    const auto block=object+std::uint32_t(offset);
    const auto density=pick(4);
    for(unsigned w=0;w<2;++w) {
        std::uint32_t bits=std::uint32_t(rng());
        if(density==0)bits&=std::uint32_t(rng())&std::uint32_t(rng());else if(density==3)bits|=std::uint32_t(rng());
        if(kind!=14)m.put(block+4u+4u*w,bits);
    }
    // The flags' definitions: an update function, none, or (kind 13) an
    // unbound target.
    for(std::uint32_t f=0;f<0x3cu;++f) {
        const auto record=definition_records+0x10u*f;
        m.put(0x473d28u+4u*f,record);m.put(record,std::uint32_t(rng()));
        m.put(record+4u,pick(5)?update_target:kind==13 && pick(3)==0?0x7e0000u+pick(16):0u);
    }
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x1000u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x10c5d0u);m.put(entry+4u,(std::uint32_t(rng())&0xffff0000u)|index);
    m.put(t_flags::fs,0x2001ff00u);
    registers(c,rng,entry,kind);
    if(kind==15) {
        static constexpr std::uint32_t regions[]{0x4ec000u,headers,objects,objects+0x1000u,definitions_page,definition_records,log_base,
            stack-0x1000u};
        m.pages.erase(regions[pick(std::size(regions))]>>12);
    }
    if(kind==14)m.pages.erase((objects+0x2000u)>>12);
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
        services.instruction=[](auto){throw std::runtime_error("Native object flag update delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try{s.load(c.abi);}catch(const std::exception &e){result.fault=e.what();return result;}
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
    Totals flags;
    if(compare("E8D80",cases,0xe8d80u+variant,flags,author_flags,
        [](ObservedWorldState &s){return leaves::bridge_e8d80(s,[](auto &v){t_flags::native_world_leaves_reference_e7e60(v);},
            [](auto &v,std::uint32_t target,std::uint32_t site){t_flags::dispatch(v,target,site,false);});},
        [](ObservedWorldState &s){t_flags::run_e7e60(s);}))return 1;
    std::printf("{\"cases\":%u,\"compared\":%llu,\"ordered_stores\":%llu,\"faults\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"coverage_high\":%llu,\"coverage_top\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)flags.compared,(unsigned long long)flags.stores,(unsigned long long)flags.faults,
        (unsigned long long)flags.children,(unsigned long long)leaves::coverage(0).load(),(unsigned long long)leaves::coverage(1).load(),
        (unsigned long long)leaves::coverage(2).load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
