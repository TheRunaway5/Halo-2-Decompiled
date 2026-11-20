// Native TU1.5 303C70/303D00 (construct and destroy an hkLocalArray of 256-byte
// shape buffers on the thread memory's stack) against the unmodified
// translated canonical bodies run_3001d0 and run_300260, extracted by
// tools/validate_native_local_buffers_tu15.py into buffers_*.inc with the
// unit's selected() operand cases (buffers_selected.inc) and the binder's
// verification references (buffers_reference.inc: the bodies with their
// dispatch calls routed to the recorded boundary).
//
// The thread memory's three virtual functions are deterministic stand-ins
// entered through dispatch() (both routes enter the same one with the same
// state). Each appends its complete entry state (target, canonical site,
// general registers, flags, x87 depth/status/TOP and an XMM digest) to a log
// in the fixture, reads its stack arguments, writes the thread memory, and
// leaves scratch registers and flags changed, as a real callee may. A wrong
// register, flag or argument at a call therefore reaches the store stream.
//
// Seeded fixtures: capacities that fit the stack, do not fit, or overflow the
// 32-bit size; stack marks at the block, zero or elsewhere; empty blocks;
// arrays that own or borrow their storage; random register files, flags,
// special XMM values, every MXCSR mode; fault injection with one fixture page
// removed. Each case compares the ordered store events (pc, address,
// preimage, bytes), the final bank, registers, flags, XMM, x87, MXCSR and the
// cursor; fault cases compare the exception, the cursor and the bank.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "native_tu15_world_leaves_remap.hpp"
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

namespace t_buffers {
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
#include "buffers_selected.inc"
    default:throw std::runtime_error("Unmapped selected() operand");
    }
}
inline constexpr std::uint32_t log_base=0x10900000u;
inline constexpr std::uint32_t allocate_stack_target=0x7a0024u,deallocate_stack_target=0x7a0028u,deallocate_target=0x7a0014u;
// One child entry: the complete register state, appended to the log.
inline void log_entry(ObservedWorldState &s,std::uint32_t target,std::uint32_t site) {
    const auto count=r(s.memory,log_base);
    const auto at=log_base+0x10u+(count%60u)*0x40u;
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
// The thread memory's virtual functions (thiscall; RET 4, 8, C).
static void dispatch(ObservedWorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    log_entry(s,target,site);
    const auto esp=s.gp[4],thread=s.gp[1];
    const auto first=r(s.memory,esp+4u);
    if(target==allocate_stack_target) {
        // allocate_stack(bytes): a block past the stack, marked.
        const auto block=0x10a00000u+((first*3u+s.gp[0])&0xff0u);
        w(s.memory,s.observe,0x7a0010u,thread+0x14u,4,block);
        s.gp[0]=block;s.gp[1]^=first;s.gp[2]=first+0x55u;s.flags.load((first^0x8d5u)&0x8d5u);
        s.gp[4]+=8u;return;
    }
    if(target==deallocate_stack_target) {
        // deallocate_stack(block, bytes): clears the mark.
        const auto bytes=r(s.memory,esp+8u);
        w(s.memory,s.observe,0x7a0020u,thread+0x14u,4,0u);
        w(s.memory,s.observe,0x7a0024u,thread+0x10u,4,first^bytes);
        s.gp[0]=bytes^0x1234u;s.gp[1]=first;s.gp[2]^=bytes;s.flags.load((bytes^0x45u)&0x8d5u);
        s.gp[4]+=12u;return;
    }
    if(target==deallocate_target) {
        // deallocate(data, bytes, class)
        const auto bytes=r(s.memory,esp+8u),storage_class=r(s.memory,esp+12u);
        w(s.memory,s.observe,0x7a0030u,thread+0x10u,4,first+bytes*storage_class);
        s.gp[0]=first^bytes;s.gp[1]=storage_class;s.gp[2]=bytes;s.flags.load((first^0x80u)&0x8d5u);
        s.gp[4]+=16u;return;
    }
    char text[96];std::snprintf(text,sizeof text,"Unbound test dispatch target %x at %x",target,site);
    throw std::runtime_error(text);
}
#include "buffers_3001d0.inc"
#include "buffers_300260.inc"
#include "buffers_reference.inc"
} // namespace t_buffers
using t_buffers::ObservedWorldState;

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
constexpr std::uint32_t array=0x10600000u,thread=0x10500000u,vtable=0x10510000u,stack=0x20010000u,fs=0x10700000u;
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};unsigned kind{};bool destroy{};};
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned kind=n%16;c.kind=kind;c.destroy=(n/16)%2;
    // The thread memory singleton, its vtable and its stack.
    m.put(0x48c938u,thread);
    m.put(thread,vtable);m.put(thread+4u,std::uint32_t(rng()));
    m.put(thread+8u,0x10b00000u+pick(0x100)*16u);
    m.put(thread+0xcu,kind==3?0xfffffff0u:pick(3)?0x400u+pick(0x4000):pick(0x200));
    m.put(thread+0x10u,std::uint32_t(rng()));
    m.put(vtable+0x14u,t_buffers::deallocate_target);m.put(vtable+0x24u,t_buffers::allocate_stack_target);
    m.put(vtable+0x28u,t_buffers::deallocate_stack_target);
    if(kind==13)m.put(vtable+0x24u,0x7a0099u),m.put(vtable+0x28u,0x7a0099u),m.put(vtable+0x14u,0x7a0099u);  // unbound targets
    for(std::uint32_t at=t_buffers::log_base;at<t_buffers::log_base+0x1000u;at+=4)m.put(at,0u);
    // The array (construct: garbage; destroy: a constructed array).
    const std::uint32_t capacity=kind==5?0x01000000u+pick(4):kind==6?0u:kind==7?0xffffffffu-pick(3):pick(24);
    const auto block=kind==8?0u:0x10b00000u+pick(0x80)*16u;
    for(std::uint32_t at=array-0x40u;at<array+0x40u;at+=4)m.put(at,special(rng));
    if(c.destroy) {
        m.put(array,pick(4)?block:0x10c00000u);m.put(array+4u,pick(8));
        m.put(array+8u,(pick(2)?0x80000000u:0u)|pick(32));m.put(array+0xcu,block);m.put(array+0x10u,capacity);
        // The stack mark: at the block, zero, or elsewhere.
        const unsigned mark=pick(3);
        m.put(thread+0x14u,mark==0?block:mark==1?0u:0x10d00000u);
    } else m.put(thread+0x14u,pick(2)?0u:0x10d00000u);
    // The stack: the return address and the capacity argument, random below.
    const auto entry=stack-4u*pick(4);
    for(std::uint32_t at=stack-0x200u;at<stack+0x40u;at+=4)m.put(at,special(rng));
    m.put(entry,0x304500u+pick(0x100));m.put(entry+4u,capacity);
    // fs:[0] holds the previous exception frame.
    m.put(fs,0x2001ff00u+pick(0x40)*4u);
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[1]=array;c.abi.gp[4]=entry;c.abi.fs_base=fs;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=special(rng);
    c.abi.x87_depth=pick(3);for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=static_cast<long double>(i)+0.5L;
    c.abi.x87_status=std::uint16_t((((8u-c.abi.x87_depth)&7u)<<11)|(pick(2)?0x4100u:0u));
    c.mxcsr=0x1f80u|(pick(4)<<13)|(pick(2)?0x40u:0u)|(pick(2)?0x8000u:0u)|(pick(3)?0u:pick(64));
    if(kind==15) {
        static constexpr std::uint32_t regions[]{array,thread,vtable,fs,0x48c938u,t_buffers::log_base};
        m.pages.erase(regions[pick(6)]>>12);
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
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native local buffers delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native local buffers delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    SourcePhysicsWorldRuntimeServices outer;leaves_remap::wrap(services,outer);
    ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    s.selected_observation_pcs=!leaves_remap::enabled;
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    try {
        const auto route=[](auto &v,std::uint32_t target,std::uint32_t site){t_buffers::dispatch(v,target,site,false);};
        if(is_native && c.destroy)result.admitted=leaves::bridge_303d00(s,[](auto &v){t_buffers::native_world_leaves_reference_300260(v);},route);
        else if(is_native)result.admitted=leaves::bridge_303c70(s,[](auto &v){t_buffers::native_world_leaves_reference_3001d0(v);},route);
        else if(c.destroy)t_buffers::run_300260(s);
        else t_buffers::run_3001d0(s);
    } catch(const std::exception &e){result.fault=e.what();}
    result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.fs_base==b.fs_base;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    const auto base=leaves_remap::parse(mode);census_mode=base=="census";verify=base=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_PHYSICS_WORLD_LEAVES","1",1);
    std::mt19937 rng(0x303c70u+(census_mode?1u:0u)+(verify?2u:0u)+(leaves_remap::enabled?4u:0u));
    std::uint64_t stores{},compared{},faults{},children{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();
        for(const auto &e:reference.stores)children+=e.instruction==0x7a0004u;
        if(!reference.fault.empty())++faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u fault mismatch: native '%s' translated '%s'\n",n,native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u (kind %u %s) store mismatch at %zu of %zu/%zu",n,c.kind,c.destroy?"destroy":"construct",at,
                native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u cursor %x vs %x\n",n,native_result.cursor,reference.cursor);return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || native_result.mxcsr!=reference.mxcsr || !left.same(right))) {
            std::fprintf(stderr,"case %u (kind %u %s) mismatch:%s%s%s",n,c.kind,c.destroy?"destroy":"construct",
                native_result.mxcsr!=reference.mxcsr?" mxcsr":"",left.same(right)?"":" bank",native_result.abi.eflags!=reference.abi.eflags?" eflags":"");
            for(unsigned i=0;i<8;++i)if(native_result.abi.gp[i]!=reference.abi.gp[i])
                std::fprintf(stderr," gp%u %x/%x",i,native_result.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native_result.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr," xmm%u",i);
            if(native_result.abi.x87_status!=reference.abi.x87_status)std::fprintf(stderr," x87 %x/%x",native_result.abi.x87_status,reference.abi.x87_status);
            std::fprintf(stderr,"\n");return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"child_calls\":%llu,"
        "\"coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)leaves::coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
