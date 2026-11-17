#pragma once
// Native TU1.5 collision ray 16C620, bound at both translated owners (live
// support and AI frame; every other unit reaches the live-support body via
// its required route). 16C620 casts a segment through the structure BSP
// (1E68E0), the cluster bridges, instanced geometry (24DC90 -> MOPP 30AA70)
// and the cluster object lists (BA970/BA9E0, 120FA0, 16C370 -> 1F8200), then
// the optional clearance repair (16BB40, 14CDB0, 11E600). All of it runs as
// the existing native typed owners (native_tu15_query_bound_camera_scene):
// ordinary C++ control flow over the guest bank, SSE scalar operations in
// the original operand order and the original x87 sequences.
//
// ABI (stdcall, RET 0x18): [ESP+4] mask, +8 origin, +C displacement,
// +10/+14 ignored objects, +18 result packet. Returns AL.
// The binding publishes exactly what callers can consume:
//   - every non-stack store, including the two rewrites of the caller's mask
//     argument word (16C654, 16C886), in program order;
//   - EAX: AL the result, bits 8-31 the value the original leaves there
//     (the displacement pointer, or the 16BB40/14CDB0/11E600 value on the
//     clearance path);
//   - ESP+0x1C; EBX/EBP/ESI/EDI unchanged (callee-saved);
//   - x87 status (C0-C3, sticky flags, TOP) and MXCSR from the native run.
// ECX, EDX, EFLAGS and XMM0-7 keep their entry values. A capstone liveness
// pass over all 71 TU1.5 call sites (and every wrapper that returns EAX)
// finds no read of them, nor of EAX bits 8-31 beyond a register copy,
// before they are rewritten; see docs/native_replacement/16C620_collision_ray.md.
// 16C620's own frame and its children's frames (below the entry ESP) are
// private native DATA here: a host-backed 64 KiB scratch window at an
// unowned bank address holds the BSP output, contexts, iterators and the
// recursion frames, and is never visible in the bank.
//
// Every guest store is journaled and committed only after the native query
// completes, so a native refusal (an owner outside the authenticated domain)
// leaves the bank untouched and the call runs the translated body.
//
// Native is the default; HALO2_REFERENCE_COLLISION_RAY=1 restores the
// translated bodies.
// HALO2_VERIFY_COLLISION_RAY=1 runs, for every admitted call, the translated
// body on the live bank under a recording observer, undoes its stores,
// runs the native query and requires identical non-stack final bytes,
// identical ordered non-stack stores, EAX, ESP and x87/MXCSR words; then
// commits the native result. A summary is printed at exit.
// HALO2_COLLISION_RAY_STATS=1 prints call/refusal/time counts at exit.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/native_tu15_camera_query_binding.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <xmmintrin.h>

namespace halo2::native_collision_ray_detail {
inline bool flag(const char *name){const auto *p=std::getenv(name);return p && *p && *p!='0';}
// Native is the default (multiplayer verify clean, 2026-11-16);
// HALO2_REFERENCE_COLLISION_RAY=1 restores the translated bodies.
inline bool reference_mode(){static const bool value=flag("HALO2_REFERENCE_COLLISION_RAY");return value;}
inline bool verify_mode(){static const bool value=flag("HALO2_VERIFY_COLLISION_RAY");return value;}
inline bool stats_mode(){static const bool value=flag("HALO2_COLLISION_RAY_STATS");return value;}
inline bool census_admission(){static const bool value=flag("HALO2_CENSUS_NATIVE_BRIDGES");return value;}
inline bool &bypass(){thread_local bool value=false;return value;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool backend=true;
#else
inline constexpr bool backend=false;
#endif

constexpr std::uint32_t scratch_bytes=0x10000u;
constexpr std::uint32_t final_pc=0x16ce94u;

struct Summary {
    std::atomic<std::uint64_t> calls{},native{},declined{},refused{},verified{},verified_stores{},
        order_differences{},ns{},reference_calls{},reference_ns{};
    std::mutex lock;std::map<std::string,std::uint64_t> reasons;
    void reason(const std::string &why){std::lock_guard guard(lock);++reasons[why];}
    ~Summary() {
        if(calls && (stats_mode() || verify_mode())) {
            std::fprintf(stderr,"Collision ray 16C620: %llu calls, %llu native, %llu declined, %llu refused, %.3f ms native\n",
                (unsigned long long)calls.load(),(unsigned long long)native.load(),(unsigned long long)declined.load(),
                (unsigned long long)refused.load(),double(ns.load())/1e6);
            if(reference_calls)std::fprintf(stderr,"Collision ray 16C620 reference: %llu calls, %.3f ms translated\n",
                (unsigned long long)reference_calls.load(),double(reference_ns.load())/1e6);
            if(verified)std::fprintf(stderr,"Collision ray 16C620 verify: %llu calls, %llu non-stack stores identical, %llu store-order differences\n",
                (unsigned long long)verified.load(),(unsigned long long)verified_stores.load(),(unsigned long long)order_differences.load());
            for(const auto &[why,count]:reasons)std::fprintf(stderr,"  %llu x %s\n",(unsigned long long)count,why.c_str());
        }
    }
};
inline Summary &summary(){static Summary value;return value;}

// The query's view of the bank: a host-backed private window for the
// native DATA, and a write journal over everything else. Reads see the
// journal; nothing reaches the bank until commit().
class QueryMemory final : public SourcePhysicsContactMemory, public NativeTu15RetainedStoreObservationDomain {
    struct Entry {std::uint32_t address;std::uint8_t bytes;std::array<std::byte,16> data;};
    SourcePhysicsContactMemory &base_;
    std::uint32_t window_;
    std::byte *host_;
    std::vector<Entry> journal_;
    std::array<std::uint64_t,64> lines_{};   // 4096 hashed 16-byte lines
    static unsigned line(std::uint32_t a){return (a>>4)&4095u;}
    bool private_extent(std::uint32_t a,std::size_t n)const {
        const auto offset=std::uint64_t(a)-window_;
        if(offset<scratch_bytes) {
            if(offset+n>scratch_bytes)throw std::runtime_error("Native 16C620 private DATA access leaves its window");
            return true;
        }
        if(std::uint64_t(a)+n>window_ && a<window_)throw std::runtime_error("Native 16C620 access overlaps its private window");
        return false;
    }
    bool journaled(std::uint32_t a,std::size_t n)const {
        if(journal_.empty())return false;
        for(std::uint32_t l=a>>4,last=std::uint32_t((std::uint64_t(a)+n-1)>>4);;++l) {
            const auto k=l&4095u;
            if(lines_[k>>6]&(std::uint64_t(1)<<(k&63u)))return true;
            if(l==last)return false;
        }
    }
    void patch(std::uint32_t a,std::span<std::byte> out)const {
        for(const auto &e:journal_) {
            const auto lo=std::max<std::uint64_t>(a,e.address),hi=std::min<std::uint64_t>(std::uint64_t(a)+out.size(),std::uint64_t(e.address)+e.bytes);
            for(auto p=lo;p<hi;++p)out[std::size_t(p-a)]=e.data[std::size_t(p-e.address)];
        }
    }
public:
    QueryMemory(SourcePhysicsContactMemory &base,std::uint32_t window,std::byte *host):base_(base),window_(window),host_(host){journal_.reserve(256);}
    void read(std::uint32_t a,std::span<std::byte> out)const override {
        if(out.empty())return;
        if(private_extent(a,out.size())){std::memcpy(out.data(),host_+(a-window_),out.size());return;}
        base_.read(a,out);
        if(journaled(a,out.size()))patch(a,out);
    }
    void read_vector(std::uint32_t a,std::span<std::byte,16> out)const override {
        if(private_extent(a,16)){std::memcpy(out.data(),host_+(a-window_),16);return;}
        base_.read_vector(a,out);
        if(journaled(a,16))patch(a,out);
    }
    void write(std::uint32_t a,std::span<const std::byte> in)override {
        if(in.empty())return;
        if(private_extent(a,in.size())){std::memcpy(host_+(a-window_),in.data(),in.size());return;}
        for(std::size_t done=0;done<in.size();) {
            const auto n=std::min<std::size_t>(16,in.size()-done);
            Entry e{a+std::uint32_t(done),std::uint8_t(n),{}};std::memcpy(e.data.data(),in.data()+done,n);
            journal_.push_back(e);
            for(std::uint32_t l=e.address>>4,last=std::uint32_t((std::uint64_t(e.address)+n-1)>>4);;++l) {
                const auto k=l&4095u;lines_[k>>6]|=std::uint64_t(1)<<(k&63u);
                if(l==last)break;
            }
            done+=n;
        }
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t a,std::size_t n)const noexcept override {
        const auto offset=std::uint64_t(a)-window_;
        if(offset<scratch_bytes || (std::uint64_t(a)+n>window_ && a<window_))return std::nullopt;
        if(journaled(a,n))return std::nullopt;
        return base_.try_read_view(a,n);
    }
    // Private DATA stores are not original observable stores.
    bool observes_retained_store(std::uint32_t,std::uint32_t a,std::size_t n)const override {
        const auto offset=std::uint64_t(a)-window_;return !(offset<scratch_bytes && offset+n<=scratch_bytes);
    }
    bool withhold_defined_data(std::uint32_t a,std::size_t n)override {
        const auto offset=std::uint64_t(a)-window_;return offset<scratch_bytes && offset+n<=scratch_bytes;
    }
    std::size_t stores()const{return journal_.size();}
    // Publish the journal to the bank in program order. Observed events were
    // buffered at native store time with their journal position.
    void commit(const SourcePhysicsContactRecordObserver &observe,
        const std::vector<std::pair<std::size_t,SourcePhysicsContactRecordStore>> &events) {
        std::size_t next=0;
        for(std::size_t i=0;i<journal_.size();++i) {
            const auto &e=journal_[i];
            base_.write(e.address,std::span(e.data).first(e.bytes));
            while(next<events.size() && events[next].first==i+1)observe(events[next++].second);
        }
        while(next<events.size())observe(events[next++].second);
    }
};

struct Arguments {std::uint32_t entry_esp,mask,origin,displacement,ignore_first,ignore_second,result;};
template<class State> Arguments arguments(State &s) {
    const auto sp=s.gp[4];
    const auto word=[&](std::uint32_t at){return physics_step_detail::r(s.memory,at,4);};
    return {sp,word(sp+4u),word(sp+8u),word(sp+12u),word(sp+16u),word(sp+20u),word(sp+24u)};
}

inline std::uint16_t control_word() {
    std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));return cw;
}

struct FpSnapshot {
    std::array<std::uint32_t,7> environment{};unsigned mxcsr{};
    void save(){asm volatile("fnstenv %0":"=m"(environment)::"memory");asm volatile("fldenv %0"::"m"(environment):"memory");mxcsr=_mm_getcsr();}
    void restore()const{asm volatile("fldenv %0"::"m"(environment):"memory");_mm_setcsr(mxcsr);}
};

// Records the 64-byte lines a native query reads or writes outside its
// private window (capture mode: the physical-original replay inputs).
class RecordingMemory final : public SourcePhysicsContactMemory {
    SourcePhysicsContactMemory &base_;
public:
    mutable std::set<std::uint32_t> lines;
    explicit RecordingMemory(SourcePhysicsContactMemory &base):base_(base){}
    void note(std::uint32_t a,std::size_t n)const{for(auto l=std::uint64_t(a)&~63ull;l<std::uint64_t(a)+n;l+=64)lines.insert(std::uint32_t(l));}
    void read(std::uint32_t a,std::span<std::byte> out)const override{note(a,out.size());base_.read(a,out);}
    void read_vector(std::uint32_t a,std::span<std::byte,16> out)const override{note(a,16);base_.read_vector(a,out);}
    void write(std::uint32_t a,std::span<const std::byte> in)override{note(a,in.size());}
};

// The native query over base through a QueryMemory window; false when it
// refused (nothing escaped). On success base holds the committed stores.
// State-free: the binding and the physical-original replay both use it.
inline bool run_query(SourcePhysicsContactMemory &base,std::uint32_t window,std::byte *host,const Arguments &a,
    const SourcePhysicsContactRecordObserver &observe,std::uint32_t &eax,NativeTu15PhysicsFrameFp &fp,std::string &refusal,
    bool commit=true) {
    QueryMemory memory(base,window,host);
    std::vector<std::pair<std::size_t,SourcePhysicsContactRecordStore>> events;
    SourcePhysicsContactRecordObserver buffered;
    // Wrapped providers (the MOPP setup guard) hide the window's domain from
    // the per-store observation filter; private DATA events end here.
    if(observe)buffered=[&](const SourcePhysicsContactRecordStore &e){
        if(std::uint64_t(e.address)-window<scratch_bytes)return;
        events.emplace_back(memory.stores(),e);
    };
    NativeTu15CameraQueryBindingActivity activity;
    bool hit{};
    try {
        hit=native_tu15_query_bound_camera_scene(memory,
            {a.mask,a.origin,a.displacement,a.ignore_first,a.ignore_second,a.result,a.entry_esp+4u},
            {window},fp,0,buffered,&activity);
    } catch(const std::exception &e){refusal=e.what();return false;}
    if(commit)memory.commit(observe,events);
    eax=activity.query.exit_eax;
    if((eax&255u)!=unsigned(hit))throw std::runtime_error("Native 16C620 exit EAX lost its result");
    return true;
}

// HALO2_COLLISION_RAY_CAPTURE=<file> (with _EVERY=k, default 50, and
// _LIMIT=n, default 200): one JSON line per sampled call with the arguments,
// FP words and every 64-byte line the native query touches plus the stack
// lines the original frame can use, all before the call. Diagnostic only.
inline void capture(SourcePhysicsContactMemory &base,std::uint32_t window,const Arguments &a,const NativeTu15PhysicsFrameFp &fp) {
    static const char *path=std::getenv("HALO2_COLLISION_RAY_CAPTURE");
    if(!path || !*path)return;
    static const unsigned every=[]{const auto *p=std::getenv("HALO2_COLLISION_RAY_CAPTURE_EVERY");return p?unsigned(std::strtoul(p,nullptr,10)):50u;}();
    static const unsigned limit=[]{const auto *p=std::getenv("HALO2_COLLISION_RAY_CAPTURE_LIMIT");return p?unsigned(std::strtoul(p,nullptr,10)):200u;}();
    static std::atomic<unsigned> seen{0},written{0};
    if(seen.fetch_add(1)%std::max(every,1u) || written>=limit)return;
    RecordingMemory recording(base);
    std::vector<std::byte> host(scratch_bytes);
    auto local=fp;std::uint32_t eax{};std::string refusal;
    FpSnapshot entry;entry.save();
    const bool done=run_query(recording,window,host.data(),a,{},eax,local,refusal,false);
    entry.restore();
    if(!done)return;
    for(std::uint32_t at=(a.entry_esp-0x3000u)&~63u;at<a.entry_esp+0x40u;at+=64)recording.lines.insert(at);
    static std::mutex lock;std::lock_guard guard(lock);
    static std::FILE *out=std::fopen(path,"w");
    if(!out)return;
    std::fprintf(out,"{\"id\":%u,\"esp\":%u,\"args\":[%u,%u,%u,%u,%u,%u],\"fp\":[%u,%u,%u],\"lines\":[",
        written.load(),a.entry_esp,a.mask,a.origin,a.displacement,a.ignore_first,a.ignore_second,a.result,fp.control_word,fp.status,fp.mxcsr);
    bool first=true;std::array<std::byte,64> bytes{};
    for(const auto line:recording.lines) {
        try{base.read(line,bytes);}catch(const std::exception&){continue;}
        std::fprintf(out,"%s[%u,\"",first?"":",",line);first=false;
        for(const auto b:bytes)std::fprintf(out,"%02x",unsigned(b));
        std::fputc('"',out);std::fputc(']',out);
    }
    std::fprintf(out,"]}\n");std::fflush(out);written.fetch_add(1);
}

template<class State> bool run_native(State &s,const Arguments &a,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t &eax,NativeTu15PhysicsFrameFp &fp,std::string &refusal) {
    auto *bank=dynamic_cast<SourceRuntimeMemory*>(&s.memory);
    if(!bank){refusal="memory is not the TU1.5 bank";return false;}
    const auto window=bank->find_unowned_range(0x61000000u,0x62000000u,scratch_bytes,0x10000u);
    if(!window){refusal="no unowned private window";return false;}
    capture(*bank,*window,a,fp);
    thread_local std::vector<std::byte> host(scratch_bytes);
    return run_query(*bank,*window,host.data(),a,observe,eax,fp,refusal);
}

template<class State> void copy_registers(State &to,const State &from) {
    to.gp=from.gp;to.flags=from.flags;to.other_flags=from.other_flags;to.fs_base=from.fs_base;to.fp=from.fp;
    for(unsigned i=0;i<8;++i)to.xmm[i]=from.xmm[i];
}

// The translated body on the live bank under a recording observer, then its
// stores undone; the native query then runs and must match it.
template<class State,class Reference> void verify(State &s,Reference reference,const Arguments &a) {
    struct Store {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Store> original;
    const SourcePhysicsContactRecordObserver record=[&](const SourcePhysicsContactRecordStore &e){original.push_back({e.address,e.before,e.written});};
    FpSnapshot entry;entry.save();
    State translated{s.memory,record,s.services,s.image};copy_registers(translated,s);
    std::string fault;
    try {struct Hold{Hold(){bypass()=true;}~Hold(){bypass()=false;}} hold;reference(translated);}
    catch(const std::exception &e){fault=e.what();}
    const auto translated_status=translated.fp.word();const auto translated_mxcsr=_mm_getcsr();
    for(auto it=original.rbegin();it!=original.rend();++it)s.memory.write(it->address,it->before);
    entry.restore();
    if(!fault.empty()){summary().reason("verify: translated faulted: "+fault);return;}
    const auto stack=[&](std::uint32_t at){return at<a.entry_esp && at>=a.entry_esp-0x10000u;};
    std::vector<Store> native;
    const SourcePhysicsContactRecordObserver mine=[&](const SourcePhysicsContactRecordStore &e){native.push_back({e.address,e.before,e.written});};
    std::uint16_t cw=control_word();
    NativeTu15PhysicsFrameFp fp{cw,s.fp.word(),_mm_getcsr()};
    std::uint32_t eax{};std::string refusal;
    if(!run_native(s,a,mine,eax,fp,refusal)){summary().reason("verify: native refused: "+refusal);return;}
    // Native stores are now in the bank; compare, then keep them.
    std::map<std::uint32_t,std::pair<std::byte,std::byte>> want,have;   // address -> (before, final)
    std::vector<std::pair<std::uint32_t,std::byte>> order_want,order_have;
    for(const auto &e:original)for(std::size_t i=0;i<e.written.size();++i) {
        const auto at=e.address+std::uint32_t(i);if(stack(at))continue;
        want.try_emplace(at,e.before[i],e.written[i]).first->second.second=e.written[i];order_want.emplace_back(at,e.written[i]);
    }
    for(const auto &e:native)for(std::size_t i=0;i<e.written.size();++i) {
        const auto at=e.address+std::uint32_t(i);if(stack(at))continue;
        have.try_emplace(at,e.before[i],e.written[i]).first->second.second=e.written[i];order_have.emplace_back(at,e.written[i]);
    }
    std::string problem;
    for(const auto &[at,v]:want) {
        const auto found=have.find(at);
        const auto final_native=found==have.end()?v.first:found->second.second;
        if(final_native!=v.second){char t[160];std::snprintf(t,sizeof t,"byte %08x translated %02x native %02x (before %02x)",at,
            unsigned(v.second),unsigned(final_native),unsigned(v.first));problem=t;break;}
    }
    if(problem.empty())for(const auto &[at,v]:have)if(!want.count(at) && v.second!=v.first) {
        char t[160];std::snprintf(t,sizeof t,"byte %08x only native wrote %02x (before %02x)",at,unsigned(v.second),unsigned(v.first));problem=t;break;
    }
    if(problem.empty() && eax!=translated.gp[0]) {
        char t[96];std::snprintf(t,sizeof t,"EAX translated %08x native %08x",translated.gp[0],eax);problem=t;
    }
    if(problem.empty() && translated.gp[4]!=a.entry_esp+0x1cu)problem="translated ESP differs";
    if(problem.empty() && ((fp.status^translated_status)&~0x3800u)) {
        char t[96];std::snprintf(t,sizeof t,"x87 SW translated %04x native %04x",translated_status,fp.status);problem=t;
    }
    if(problem.empty() && fp.mxcsr!=translated_mxcsr) {
        char t[96];std::snprintf(t,sizeof t,"MXCSR translated %08x native %08x",translated_mxcsr,fp.mxcsr);problem=t;
    }
    for(unsigned n:{3u,5u,6u,7u})if(problem.empty() && translated.gp[n]!=s.gp[n])problem="translated callee-saved register changed";
    if(!problem.empty()) {
        char head[160];std::snprintf(head,sizeof head,"Native 16C620 differs (mask %08x origin %08x result %08x): ",a.mask,a.origin,a.result);
        throw std::runtime_error(head+problem);
    }
    if(order_want!=order_have) {
        // The final bytes match; report where the first few orders diverge.
        if(summary().order_differences.fetch_add(1)<4) {
            std::size_t at=0;while(at<order_want.size() && at<order_have.size() && order_want[at]==order_have[at])++at;
            std::fprintf(stderr,"Collision ray 16C620 verify: store order differs at byte %zu of %zu/%zu: translated %08x native %08x (mask %08x)\n",
                at,order_want.size(),order_have.size(),at<order_want.size()?order_want[at].first:0u,at<order_have.size()?order_have[at].first:0u,a.mask);
        }
    }
    summary().verified.fetch_add(1);summary().verified_stores.fetch_add(order_want.size());
    // Publish the native state as the bridge does.
    s.gp[0]=eax;s.gp[4]=a.entry_esp+0x1cu;
    s.fp.status=fp.status;physics_contact_runtime_detail::source_load_x87_exception_flags(fp.status);_mm_setcsr(fp.mxcsr);
}

inline bool in_closure(std::uint32_t site) {
    // The instruction-service sites (38846E, 21F590, 33B595) lie outside the
    // 16C620 tree; any policy that adds one inside it keeps the translation.
    return site>=0x16b800u && site<0x16cea0u;
}

template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(!backend){(void)s;(void)reference;return false;}
    else {
    if(bypass() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if(reference_mode()) {
        if(!stats_mode())return false;
        // Stats only: time the translated body as the reference route.
        const auto started=std::chrono::steady_clock::now();
        {struct Hold{Hold(){bypass()=true;}~Hold(){bypass()=false;}} hold;reference(s);}
        auto &total=summary();total.reference_calls.fetch_add(1,std::memory_order_relaxed);total.calls.fetch_add(1,std::memory_order_relaxed);
        total.reference_ns.fetch_add(std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count()),std::memory_order_relaxed);
        return true;
    }
    auto &total=summary();total.calls.fetch_add(1,std::memory_order_relaxed);
    const auto decline=[&](const char *why){total.declined.fetch_add(1,std::memory_order_relaxed);if(stats_mode())total.reason(std::string("declined: ")+why);return false;};
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return decline("active MMX");
    const auto mxcsr=_mm_getcsr();
    if((mxcsr&0x1f80u)!=0x1f80u || (mxcsr&0xffff0000u))return decline("unmasked SSE exceptions");
    const auto cw=control_word();
    if((cw&0x3fu)!=0x3fu)return decline("unmasked x87 exceptions");
    if(s.fp.count)return decline("live x87 stack");
    if(s.fp.status&0x8080u)return decline("x87 error summary");
    std::uint32_t *cursor=nullptr;
    if(s.services.instruction && census_admission() && !s.instruction_cursor) {
        // Diagnostic census only: full callbacks without a literal cursor.
    } else if(s.services.instruction) {
        if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return decline("instruction callbacks without a literal cursor");
        const auto *policy=s.instruction_cursor;
        if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace))return decline("active trace");
        if(s.observe && !policy->observation_only_stores)return decline("store observer");
        for(const auto site:policy->service_sites)if(in_closure(site))return decline("service site inside 16C620");
        cursor=policy->position;
    }
    const auto a=arguments(s);
    if(verify_mode()){verify(s,reference,a);if(cursor)*cursor=final_pc;total.native.fetch_add(1,std::memory_order_relaxed);return true;}
    const auto started=stats_mode()?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    NativeTu15PhysicsFrameFp fp{cw,s.fp.word(),mxcsr};
    std::uint32_t eax{};std::string refusal;
    if(!run_native(s,a,s.observe,eax,fp,refusal)) {
        total.refused.fetch_add(1,std::memory_order_relaxed);total.reason("refused: "+refusal);
        return false;
    }
    if(((fp.status>>11u)&7u)!=s.fp.top)throw std::runtime_error("Native 16C620 unbalanced the x87 stack");
    s.gp[0]=eax;s.gp[4]=a.entry_esp+0x1cu;
    s.fp.status=fp.status;physics_contact_runtime_detail::source_load_x87_exception_flags(fp.status);_mm_setcsr(fp.mxcsr);
    if(cursor)*cursor=final_pc;
    total.native.fetch_add(1,std::memory_order_relaxed);
    if(stats_mode())total.ns.fetch_add(std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count()),std::memory_order_relaxed);
    return true;
    }
}
} // namespace halo2::native_collision_ray_detail
