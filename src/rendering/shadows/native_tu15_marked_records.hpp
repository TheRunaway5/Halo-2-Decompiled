#pragma once
// Boundary adapter for the marked record pass, entered at 179A40
// (native_tu15_marked_records_systems.hpp). The callers and children keep
// the original ABI, which the adapter owns: no arguments; RET; EBX/EBP
// saved first, ESI/EDI once there is a record; 16EA60 (the unit's route)
// gets ECX the array and EDX zero; 17C710 (the unit's route) the handle and
// address pushed, with the registers the flag tests leave; 17CD10 the
// address pushed; every register a child returns comes back; the walk keeps
// EBP the handle (the scan's bit while scanning), EDI the index (the bound),
// ESI the address (the found index); the flags of the last CMP; each access
// publishes its original instruction.
// HALO2_REFERENCE_MARKED_RECORDS=1 restores the translated body.
// HALO2_VERIFY_MARKED_RECORDS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_marked_records_systems.hpp"

namespace halo2::native_marked_records_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_marked_records;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_MARKED_RECORDS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_MARKED_RECORDS");return p && *p;}();
    return value;
}
enum Branch : unsigned {none_in_use, in_use, disabled, enabled, held, visited, scan_found, scan_free, past_bound, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Marked records verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge marked_edges[]{
    {0x179a93u,0x179ae2u,disabled},{0x179a93u,0x179a9du,enabled},
    {0x179a9du,0x179ae2u,held},{0x179a9du,0x179abcu,visited},
    {0x179b01u,0x179b17u,scan_found},{0x179b01u,0x179b01u,scan_free},
    {0x179ae2u,0x179b36u,past_bound},{0x179ae2u,0x179b34u,past_bound},
};
template<class State,class Route,class Finish> struct Marked : Port<State> {
    using Access=logic::marked_records::Access;
    using Kept=logic::marked_records::Kept;
    Route &route;Finish &finish_child;bool started{};
    Marked(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Finish &f):Port<State>(s,c,t,marked_edges),route(r),finish_child(f){}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[edi],this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:this->push(0x179a40u,gp[ebx]);return 0x179a41u;
        case first_size:return 0x179a59u;
        case first_data:return 0x179a5cu;
        case first_salt:gp[ecx]=this->flags.product(gp[ecx],gp[eax]);return 0x179a62u;           // IMUL ECX,EAX
        case data:return 0x179a80u;
        case byte_flags:return 0x179a93u;
        case word_flags:return 0x179a9du;
        case array_reload:return 0x179ac9u;
        case bound:return 0x179ae2u;
        case bits:return 0x179ae9u;
        // MOV ECX,EAX; AND ECX,1F; MOV EBP,1; SHL EBP,CL; MOV ECX,EAX; SAR ECX,5.
        case bit_word:
            gp[ecx]=this->flags.logic(gp[eax]&0x1fu,4);gp[ebp]=this->flags.shift(1u,gp[ecx]&0xffu,4);
            gp[ecx]=this->flags.right(gp[eax],5u,4,true);return 0x179b01u;
        case next_size:return 0x179b17u;
        case next_data:return 0x179b1au;
        case next_salt:gp[edx]=this->flags.product(gp[edx],gp[esi]);return 0x179b20u;            // IMUL EDX,ESI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:case array_reload:gp[ebx]=v;if(a==array)this->push(0x179a47u,gp[ebp]);break;
        case first_size:gp[ecx]=v;break;
        case first_data:case bits:case next_size:gp[edx]=v;break;
        // MOVSX EBP,[ECX+EDX]; SHL EBP,10; OR EBP,EAX.
        case first_salt:gp[ebp]=this->flags.logic(this->flags.shift(logic::widen16(v),16u,4)|gp[eax],4);break;
        // MOV ECX,[EBX+44]; MOV EDI,EBP; AND EDI,FFFF; MOV ESI,EDI; IMUL ESI,190.
        case data:
            gp[ecx]=v;gp[edi]=this->flags.logic(gp[ebp]&0xffffu,4);gp[esi]=this->flags.product(gp[edi],logic::record_size);
            break;
        // MOV AL,[ESI+ECX+2]; ADD ESI,ECX; TEST AL,1.
        case byte_flags:gp[eax]=(gp[eax]&0xffffff00u)|(v&0xffu);gp[esi]=this->flags.plus(gp[esi],gp[ecx],4);this->test8(v&1u);break;
        // MOV AX,[ESI+2]; MOV CL,AL; SHR CL,2; TEST CL,1; MOV DL,AL; SHR DL,3;
        // TEST DL,1; SHR AL,6; TEST AL,1.
        case word_flags: {
            gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);
            const auto low=v&0xffu;
            const auto c=this->flags.right(low,2u,1,false);gp[ecx]=(gp[ecx]&0xffffff00u)|c;this->test8(c&1u);
            if(c&1u)break;
            const auto d=this->flags.right(low,3u,1,false);gp[edx]=(gp[edx]&0xffffff00u)|d;this->test8(d&1u);
            if(d&1u)break;
            const auto e=this->flags.right(low,6u,1,false);gp[eax]=(gp[eax]&0xffffff00u)|e;this->test8(e&1u);
            break;
        }
        case bound:gp[edi]=v;this->compare32(gp[eax],v);break;
        // TEST [EDX+ECX*4],EBP; (free) INC EAX; CMP EAX,EDI.
        case bit_word:
            this->test32(v&gp[ebp]);
            if(!(v&gp[ebp])){gp[eax]=this->flags.increment(gp[eax],4,false);this->compare32(gp[eax],gp[edi]);}
            break;
        case next_data:gp[eax]=v;break;
        case next_salt:gp[eax]=this->flags.logic(this->flags.shift(logic::widen16(v),16u,4)|gp[esi],4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // XOR EDX,EDX; MOV ECX,EBX; CALL 16EA60; OR EBP,-1; CMP EAX,-1.
    std::uint32_t first(Kept &kept) {
        auto &gp=this->gp;
        gp[edx]=this->flags.logic(0,4);gp[ecx]=gp[ebx];
        this->call_routed(route,0x179a4cu,0x179a51u,0x16ea60u);
        gp[ebp]=this->flags.logic(gp[ebp]|0xffffffffu,4);this->compare32(gp[eax],logic::none);
        refresh(kept);
        return gp[eax];
    }
    // The first: CMP EBP,-1 (PUSH ESI, EDI when there is one). The next: CMP
    // EAX,-1; MOV EBP,EAX.
    void handled(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        if(!started) {
            started=true;
            this->compare32(gp[ebp],logic::none);
            this->reached(gp[ebp]==logic::none?none_in_use:in_use);
            if(gp[ebp]!=logic::none){this->push(0x179a74u,gp[esi]);this->push(0x179a75u,gp[edi]);}
        } else {this->compare32(gp[eax],logic::none);gp[ebp]=gp[eax];}
        kept.handle=gp[ebp];
    }
    // PUSH EBP; PUSH ESI; CALL 17C710; PUSH ESI; CALL 17CD10.
    void visit(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x179abcu,gp[ebp]);this->push(0x179abdu,gp[esi]);
        this->call_routed(route,0x179abeu,0x179ac3u,0x17c710u);
        this->push(0x179ac3u,gp[esi]);
        this->call(0x179ac4u,0x179ac9u,0x17cd10u,finish_child);
        refresh(kept);
    }
    // CMP EBP,-1; XOR EAX,EAX or LEA EAX,[EDI+1]; OR ESI,-1; TEST EAX,EAX.
    void next(Kept &,std::uint32_t) {
        auto &gp=this->gp;
        this->compare32(gp[ebp],logic::none);
        gp[eax]=gp[ebp]==logic::none?this->flags.logic(0,4):gp[edi]+1u;
        gp[esi]=this->flags.logic(gp[esi]|0xffffffffu,4);this->test32(gp[eax]);
    }
    // (MOV ESI,EAX); OR EAX,-1; CMP ESI,-1.
    void scanned(std::uint32_t found) {
        auto &gp=this->gp;
        if(found!=logic::none)gp[esi]=found;
        gp[eax]=this->flags.logic(gp[eax]|0xffffffffu,4);this->compare32(gp[esi],logic::none);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Marked> memory{*this};
        if(logic::marked_records::each(memory,*this)==logic::marked_records::Exit::walked) {
            gp[edi]=this->pop(0x179b34u);gp[esi]=this->pop(0x179b35u);
        }
        gp[ebp]=this->pop(0x179b36u);gp[ebx]=this->pop(0x179b37u);
        this->ret(0x179b38u,0);this->publish(0x179b38u);
    }
};
template<class State,class Reference,class Route,class Finish>
bool bridge(State &s,Reference reference,Route route,Finish finish) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x179a40u,0x179b38u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Marked<State,Route,Finish>(v,c,t,route,finish);});
    return true;
}
} // namespace halo2::native_marked_records_detail
