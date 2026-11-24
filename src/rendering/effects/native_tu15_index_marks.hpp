#pragma once
// Boundary adapter for the index marks, entered at 3F3E0
// (native_tu15_index_marks_systems.hpp). The callers keep the original ABI,
// which the adapter owns: no arguments; RET; EDI saved first, EBX/ESI once
// there are entries; EDX the position, ESI the entry, EBX the bit, EAX the
// word's address and ECX the count at the exit, the flags of the last CMP;
// each access publishes its original instruction.
// HALO2_REFERENCE_INDEX_MARKS=1 restores the translated body.
// HALO2_VERIFY_INDEX_MARKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/effects/native_tu15_index_marks_systems.hpp"

namespace halo2::native_index_marks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_index_marks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_INDEX_MARKS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_INDEX_MARKS");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty, entries, more, last, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Index marks verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge marks_edges[]{
    {0x3f3e9u,0x3f435u,empty},{0x3f3e9u,0x3f3f2u,entries},{0x3f427u,0x3f400u,more},{0x3f427u,0x3f433u,last},
};
template<class State> struct Marks : Port<State> {
    using Access=logic::index_marks::Access;
    bool saved{};
    Marks(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,marks_edges){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case state:this->push(0x3f3e0u,gp[edi]);return 0x3f3e1u;
        case count_check:gp[edx]=this->flags.logic(0,4);return 0x3f3e9u;                        // XOR EDX,EDX
        // PUSH EBX; PUSH ESI; LEA ESI,[EDI+A6E] (first entry).
        case entry_index:
            if(!saved) {
                saved=true;this->push(0x3f3f2u,gp[ebx]);this->push(0x3f3f3u,gp[esi]);
                gp[esi]=gp[edi]+logic::list_entries;
            }
            return 0x3f400u;
        case mark_word:return 0x3f412u;
        case mark_store:return 0x3f425u;
        case count_reload:return 0x3f427u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case state:gp[edi]=v;break;
        case count_check:this->flags.minus(v,gp[edx],2);break;                                   // CMP WORD,DX
        // MOVSX ECX; MOV EAX,ECX; AND ECX,1F; SAR EAX,5; MOV EBX,1; SHL EBX,CL.
        case entry_index:
            gp[ecx]=logic::widen16(v);gp[eax]=gp[ecx];gp[ecx]=this->flags.logic(gp[ecx]&0x1fu,4);
            gp[eax]=this->flags.right(gp[eax],5u,4,true);gp[ebx]=this->flags.shift(1u,gp[ecx]&0xffu,4);
            break;
        // MOV ECX,[4D1EE0+EAX*4]; LEA EAX,[...]; ADD ESI,1A; OR ECX,EBX.
        case mark_word:
            gp[ecx]=v;gp[eax]=logic::globals::marks+gp[eax]*4u;gp[esi]=this->flags.plus(gp[esi],logic::entry_size,4);
            gp[ecx]=this->flags.logic(gp[ecx]|gp[ebx],4);break;
        // MOVSX ECX,[EDI+A6C]; INC EDX; CMP EDX,ECX.
        case count_reload:
            gp[ecx]=logic::widen16(v);gp[edx]=this->flags.increment(gp[edx],4,false);this->compare32(gp[edx],gp[ecx]);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<Marks> memory{*this};
        logic::index_marks::mark(memory);
        if(saved){gp[esi]=this->pop(0x3f433u);gp[ebx]=this->pop(0x3f434u);}
        gp[edi]=this->pop(0x3f435u);
        this->ret(0x3f436u,0);this->publish(0x3f436u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x3f3e0u,0x3f436u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Marks<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_index_marks_detail
