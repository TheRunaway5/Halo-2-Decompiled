#pragma once
// Boundary adapter for the geometry cache entry check, entered at 130D20
// (native_tu15_cache_entry_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: the handle at [ESP+4], RET 4; AL
// the result (22A790's when asked); ESI and EDI saved (the record and the
// entry), EBX once the entry's residency is read; 22A790 gets the block
// pushed and ESI the owner, and every register it returns comes back; the
// flags of the last instruction that sets them; each access publishes its
// original instruction.
// HALO2_REFERENCE_CACHE_ENTRY=1 restores the translated body.
// HALO2_VERIFY_CACHE_ENTRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/native_tu15_cache_entry_systems.hpp"

namespace halo2::native_cache_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_cache_entry;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CACHE_ENTRY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CACHE_ENTRY");return p && *p;}();
    return value;
}
enum Branch : unsigned {current, not_resident, stale, not_ready, asked, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Cache entry verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Ask> struct Entry : Port<State> {
    using Access=logic::cache_entry::Access;
    using Local=logic::cache_entry::Local;
    Ask &ask_child;
    std::uint32_t depth{};
    Entry(State &s,std::uint32_t *c,Trace<State> *t,Ask &a):Port<State>(s,c,t,{}),ask_child(a){}
    std::uint32_t local(Local)const{return this->gp[esp]+depth+4u;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case handle_argument:return 0x130d20u;
        case entries:return 0x130d24u;
        case entry_data:return 0x130d2au;
        case cache:return 0x130d3au;
        case records:return 0x130d40u;
        case record_data:return 0x130d43u;
        case stamp:return 0x130d4cu;
        case cache_frame:return 0x130d4fu;
        case resident:save(0x130d5fu,this->gp[ebx]);return 0x130d60u;                             // PUSH EBX
        case owner:return 0x130d74u;
        case ready:return 0x130d77u;
        case block:return 0x130d7eu;
        case shift:return 0x130d81u;
        case blocks:return 0x130d84u;
        case owner_again:return 0x130d8au;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case handle_argument:gp[eax]=v;break;
        case entries:case cache:case blocks:gp[edx]=v;break;
        // AND EAX,FFFF; LEA ECX,[EAX+EAX*2]; PUSH ESI; PUSH EDI; LEA EDI,[EDX+ECX*4].
        case entry_data:
            gp[edx]=v;gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);gp[ecx]=gp[eax]*3u;save(0x130d35u,gp[esi]);
            save(0x130d36u,gp[edi]);gp[edi]=v+gp[ecx]*4u;break;
        case records:gp[ecx]=v;break;
        // LEA EAX,[EAX+EAX*2]; LEA ESI,[ECX+EAX*8].
        case record_data:gp[ecx]=v;gp[eax]*=3u;gp[esi]=v+gp[eax]*8u;break;
        case stamp:gp[eax]=v;break;
        // SUB ECX,EAX; MOV AL,0.
        case cache_frame:gp[ecx]=this->flags.minus(v,gp[eax],4);set_low_byte(gp[eax],0u);break;
        // MOV BL; TEST BL,BL; (CMP ECX,5 for a resident entry).
        case resident:set_low_byte(gp[ebx],v);this->test8(v);if(v&0xffu)this->compare32(gp[ecx],logic::stale_frames);break;
        case ready:set_low_byte(gp[ebx],v);this->test8(v);break;
        case owner:gp[ecx]=v;break;
        case block:gp[eax]=v;break;
        case shift:gp[ecx]=v;break;
        case owner_again:gp[esi]=v;break;                                         // MOV ESI,[EDI+8]
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // SHL EAX,CL; ADD EAX,EDX (the flags; the sum is the logic's block address); PUSH EAX; CALL 22A790.
    void ask(std::uint32_t block_address) {
        auto &gp=this->gp;
        (void)this->flags.plus(this->flags.shift(gp[eax],gp[ecx]&0xffu,4),gp[edx],4);
        gp[eax]=block_address;save(0x130d91u,block_address);
        this->call(0x130d92u,0x130d97u,0x22a790u,ask_child);depth-=4u;
    }
    void run() {
        auto &gp=this->gp;
        using logic::cache_entry::Exit;
        Memory<Entry> memory{*this};
        const auto exit=logic::cache_entry::usable(memory,*this);
        switch(exit) {
        case Exit::current:
            this->reached(Branch::current);
            gp[edi]=this->pop(0x130d58u);set_low_byte(gp[eax],1u);gp[esi]=this->pop(0x130d5bu);
            this->ret(0x130d5cu,4u);this->publish(0x130d5cu);return;
        case Exit::not_resident:
            this->reached(Branch::not_resident);
            gp[ebx]=this->pop(0x130d67u);gp[edi]=this->pop(0x130d68u);set_low_byte(gp[eax],1u);gp[esi]=this->pop(0x130d6bu);
            this->ret(0x130d6cu,4u);this->publish(0x130d6cu);return;
        default:
            this->reached(exit==Exit::stale?Branch::stale:exit==Exit::not_ready?Branch::not_ready:Branch::asked);
            gp[ebx]=this->pop(0x130d97u);gp[edi]=this->pop(0x130d98u);gp[esi]=this->pop(0x130d99u);
            this->ret(0x130d9au,4u);this->publish(0x130d9au);return;
        }
    }
};
template<class State,class Reference,class Ask> bool bridge(State &s,Reference reference,Ask ask) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x130d20u,0x130d9au,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Entry<State,Ask>(v,c,t,ask);});
    return true;
}
} // namespace halo2::native_cache_entry_detail
