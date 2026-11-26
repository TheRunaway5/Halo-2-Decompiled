#pragma once
// Shared comparison driver for the group F native closures
// (tools/validate_native_frame_misc_tu15.py). Each test supplies authored
// fixtures, the native bridge and the unmodified translated body; the driver
// runs both on copies of the same bank and compares ordered stores (pc,
// address, preimage, bytes), the child call log, the whole bank, GP, EFLAGS,
// XMM, x87 status/lanes/control, MXCSR and the cursor. Faults compare the
// exception text, cursor and store prefix. Modes: "" (literal cursor),
// "census" (full callbacks without a cursor under HALO2_CENSUS_NATIVE_BRIDGES)
// and "verify" (the owner's record/replay verifier).
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace frame_misc_harness {
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
// The generated units' sign extension (translated bodies in body.inc).
inline std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
inline std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}
inline std::vector<std::uint64_t> *call_log{};
inline std::uint32_t special_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0x7f800000u,0xff800000u,
        0x00000001u,0x807fffffu,0x80000000u,0x00000000u,0x3f800000u,0x447a0000u,0x00800000u};
    return values[rng()%12];
}
// Hash of a child's complete entry ABI plus selected memory words.
inline std::uint64_t entry_hash(const WorldState &s,std::uint32_t target) {
    std::uint64_t h=target;
    for(const auto v:s.gp)h=mix(h,v);
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,_mm_getcsr());
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    h=mix(h,s.fp.count);h=mix(h,s.fp.status);
    return h;
}
inline void log_call(std::uint64_t h) {
    if(!call_log)return;
    if(call_log->size()>=16384)throw std::runtime_error("Child call budget exceeded");
    call_log->push_back(h);
}

struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    std::optional<std::uint32_t> denied_write;
    unsigned writes_before_denial{};
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
        if(denied_write==at) {
            if(writes_before_denial==0)throw std::runtime_error("Authored write rejected");
            --writes_before_denial;
        }
        for(std::size_t n=0;n<in.size();++n)page(at+std::uint32_t(n),false)[(at+n)&4095u]=in[n];
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));
    }
    std::uint32_t get(std::uint32_t at,unsigned width=4) {
        std::uint32_t v=0;for(unsigned n=0;n<width;++n)v|=std::to_integer<std::uint32_t>(page(at+n,true)[(at+n)&4095u])<<(8*n);return v;
    }
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));
        copy.denied_write=denied_write;copy.writes_before_denial=writes_before_denial;
        return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
    // Fault injection: remove the page holding one authored address.
    void drop(std::uint32_t at){pages.erase(at>>12);}
};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{0x1f80u};std::uint32_t policy{};
    std::function<std::uint32_t(std::uint32_t)> reciprocal_sqrt{};
    bool full_observation{};
    std::uint32_t reject_instruction{};
    std::uint32_t activate_trace_at_store{};
    std::uint32_t reject_after_store_pc{};
    std::array<std::uint32_t,2> census_child_range{};
};
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    std::vector<std::uint32_t> instructions;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
inline void random_abi(std::mt19937 &rng,Case &c,std::uint32_t stack) {
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u)|(rng()%5==0?0x400u:0u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1f81u};
    c.mxcsr=modes[rng()%8];c.policy=std::uint32_t(rng()%4);
    // Live x87 stack, condition bits and TOP with masked exceptions.
    c.abi.x87_depth=std::uint32_t(rng()%5);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
    const unsigned top=(8u-c.abi.x87_depth+std::uint32_t(rng()%8))&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(std::uint32_t(rng())&0x4700u));
    const std::uint16_t control=std::uint16_t(0x3fu|0x40u|((rng()%4)<<8)|((rng()%4)<<10));
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,c.mxcsr};
}
inline bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
    if(a.active_mmx.has_value()!=b.active_mmx.has_value())return false;
    if(a.active_mmx && (a.active_mmx->known_mask!=b.active_mmx->known_mask || a.active_mmx->physical!=b.active_mmx->physical))
        return false;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    if(a.floating_point && b.floating_point)same=same && a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    return same;
}
inline bool census_mode{};
inline std::uint32_t policy{};
template<class Native,class Original>
Result execute(Memory &memory,const Case &c,bool is_native,Native native,Original original,
               const SourcePhysicsContactRuntimeServices::RequiredOwner &required={}) {
    Result result;
    bool trace_active=false;
    // A loop index read from a shifted (unbalanced child) frame slot can run
    // away; both routes then stop at the same store with the same fault.
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e) {
        if(result.stores.size()>=(1u<<18))throw std::runtime_error("Store budget exceeded");
        result.stores.push_back(e);
        if(c.activate_trace_at_store && e.instruction==c.activate_trace_at_store)trace_active=true;
        if(c.reject_after_store_pc && e.instruction==c.reject_after_store_pc)
            throw std::runtime_error("Observer rejected committed store");
    };
    SourcePhysicsWorldRuntimeServices services;
    if(required)services.required_owner=required;
    if(c.reciprocal_sqrt)services.reciprocal_sqrt=c.reciprocal_sqrt;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(c.full_observation) {
        services.instruction=[&](std::uint32_t pc) {
            result.cursor=pc;
            result.instructions.push_back(pc);
            if(pc==c.reject_instruction)throw std::runtime_error("Instruction rejected by observer");
        };
    } else if(is_native) {
        services.instruction=[&](std::uint32_t pc) {
            if(census_mode && c.census_child_range[0] && pc>=c.census_child_range[0] && pc<=c.census_child_range[1])
                return;
            if(!trace_active)throw std::runtime_error("Native closure delivered a translated instruction");
            result.cursor=pc;
            result.instructions.push_back(pc);
        };
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
            if(c.activate_trace_at_store)cursor.first_trace=&trace_active;
        }
    } else services.instruction=[&](std::uint32_t pc) {
        result.cursor=pc;
        if(trace_active)result.instructions.push_back(pc);
    };
    const auto saved=_mm_getcsr();std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    _mm_setcsr(c.mxcsr);
    policy=c.policy;call_log=&result.calls;
    try {
        if(is_native)result.admitted=native(s);
        else original(s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;result.mxcsr=_mm_getcsr();
    result.abi=s.save();
    asm volatile("fnclex\n\tfldcw %0"::"m"(control));_mm_setcsr(saved);
    return result;
}
struct Totals {std::uint64_t compared{},stores{},children{},faults{},refused{};};
// Compare one authored case; returns false (with a diagnostic) on any difference.
template<class Native,class Original>
bool compare(unsigned n,const Case &c,Native native,Original original,Totals &t,bool allow_refusal=false,
             const SourcePhysicsContactRuntimeServices::RequiredOwner &required={},bool compare_fault_abi=false) {
    auto left=c.memory.clone(),right=c.memory.clone();
    const auto mine=execute(right,c,true,native,original,required);
    if(!mine.admitted && mine.fault.empty()) {
        if(allow_refusal){++t.refused;return true;}
        std::fprintf(stderr,"case %u refused\n",n);return false;
    }
    const auto theirs=execute(left,c,false,native,original,required);
    ++t.compared;t.stores+=theirs.stores.size();t.children+=theirs.calls.size();
    if(!theirs.fault.empty())++t.faults;
    if(mine.fault!=theirs.fault) {
        std::fprintf(stderr,"case %u fault mismatch: native '%s' translated '%s'\n",n,mine.fault.c_str(),theirs.fault.c_str());
        return false;
    }
    if(mine.stores!=theirs.stores) {
        std::size_t at=0;while(at<mine.stores.size()&&at<theirs.stores.size()&&mine.stores[at]==theirs.stores[at])++at;
        std::fprintf(stderr,"case %u store mismatch at %zu of %zu/%zu",n,at,mine.stores.size(),theirs.stores.size());
        if(at<mine.stores.size()&&at<theirs.stores.size())
            std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",mine.stores[at].instruction,mine.stores[at].address,
                theirs.stores[at].instruction,theirs.stores[at].address);
        std::fprintf(stderr,"\n");return false;
    }
    if(mine.calls!=theirs.calls){std::fprintf(stderr,"case %u child log mismatch (%zu/%zu)\n",n,mine.calls.size(),theirs.calls.size());return false;}
    if(mine.instructions!=theirs.instructions){std::fprintf(stderr,"case %u instruction log mismatch\n",n);return false;}
    if(!census_mode && mine.cursor!=theirs.cursor){std::fprintf(stderr,"case %u cursor %x vs %x\n",n,mine.cursor,theirs.cursor);return false;}
    if(mine.mxcsr!=theirs.mxcsr){std::fprintf(stderr,"case %u MXCSR %x vs %x\n",n,mine.mxcsr,theirs.mxcsr);return false;}
    if((theirs.fault.empty() || compare_fault_abi) && !same_abi(mine.abi,theirs.abi)) {
        std::fprintf(stderr,"case %u register/flag/XMM/x87 mismatch:",n);
        for(unsigned i=0;i<8;++i)if(mine.abi.gp[i]!=theirs.abi.gp[i])std::fprintf(stderr," r%u %x/%x",i,mine.abi.gp[i],theirs.abi.gp[i]);
        if(mine.abi.eflags!=theirs.abi.eflags)std::fprintf(stderr," flags %x/%x",mine.abi.eflags,theirs.abi.eflags);
        std::fprintf(stderr,"\n");return false;
    }
    if(!left.same(right)){std::fprintf(stderr,"case %u bank mismatch\n",n);return false;}
    return true;
}
// The refusals every closure shares: MapPack11, full callbacks without a
// cursor outside the census diagnostic, and an active trace.
template<class Native> void check_refusals(Case c,Native native) {
    SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
    WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
    if(native(a))throw std::runtime_error("MapPack11 admitted");
    services.instruction=[](auto){};
    WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
    if(native(b))throw std::runtime_error("Full instruction callbacks without a cursor admitted");
    std::uint32_t position{};bool trace=true;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto &cursor=services.instruction_cursor;cursor.position=&position;cursor.service_sites=sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;cursor.first_trace=&trace;
    WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
    if(native(d))throw std::runtime_error("Active trace admitted");
    trace=false;cursor.observation_only_stores=false;
    SourcePhysicsContactRecordObserver arbitrary=[](const auto &){};
    WorldState e{c.memory,arbitrary,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);
    if(native(e))throw std::runtime_error("Arbitrary store observer admitted");
}
inline void set_mode(const std::string &mode,const char *verify_env) {
    census_mode=mode=="census";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(mode=="verify")setenv(verify_env,"1",1);
}
inline void report(unsigned cases,const Totals &t,std::uint64_t coverage,unsigned branches,const std::string &mode) {
    std::printf("{\"cases\":%u,\"compared\":%llu,\"refused\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",cases,(unsigned long long)t.compared,(unsigned long long)t.refused,
        (unsigned long long)t.faults,(unsigned long long)t.stores,(unsigned long long)t.children,
        (unsigned long long)coverage,branches,mode.c_str());
}
} // namespace frame_misc_harness
