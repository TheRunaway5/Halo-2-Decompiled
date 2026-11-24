#pragma once
// Boundary adapter for the record walk, entered at 180520
// (native_tu15_record_walk_systems.hpp). The callers and the children keep
// the original ABI, which the adapter owns: no arguments; RET; EDI saved
// first, ESI before 43980, EBX/EBP after it; the scan keeps EAX the index,
// ECX the word index, EBX the bit, EDX the in-use bits, ESI the bound; 17FAF0
// gets ECX the record, EDX the handle, ESI and EAX the index, EBP the data;
// every register a child returns comes back; the flags of the last test;
// each access publishes its original instruction.
// HALO2_REFERENCE_RECORD_WALK=1 restores the translated body.
// HALO2_VERIFY_RECORD_WALK=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_record_walk_systems.hpp"

namespace halo2::native_record_walk_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_walk;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_WALK");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_WALK");return p && *p;}();
    return value;
}
enum Branch : unsigned {invalid, valid, past_bound, scanning, in_use, free_slot, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record walk verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge walk_edges[]{
    {0x180527u,0x18056fu,invalid},{0x180527u,0x18052eu,valid},
    {0x180547u,0x18056cu,past_bound},{0x180547u,0x18054eu,scanning},
    {0x180562u,0x180576u,in_use},{0x180562u,0x180562u,free_slot},{0x180562u,0x18056cu,free_slot},
};
template<class State,class Prepare,class Visit> struct Walk : Port<State> {
    using Access=logic::record_walk::Access;
    using Kept=logic::record_walk::Kept;
    Prepare &prepare_child;Visit &visit_child;
    Walk(State &s,std::uint32_t *c,Trace<State> *t,Prepare &p,Visit &v)
        :Port<State>(s,c,t,walk_edges),prepare_child(p),visit_child(v){}
    // LEA EAX,[ESI+1]; TEST EAX,EAX.
    void next_index(){this->gp[eax]=this->gp[esi]+1u;this->test32(this->gp[eax]);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:this->push(0x180520u,gp[edi]);return 0x180521u;
        case valid:return 0x180527u;
        case bound:return 0x180547u;
        case bits:return 0x18054eu;
        // MOV ECX,EAX; AND ECX,1F; MOV EBX,1; SHL EBX,CL; MOV ECX,EAX; SAR ECX,5.
        case bit_word:
            gp[ecx]=this->flags.logic(gp[eax]&0x1fu,4);gp[ebx]=this->flags.shift(1u,gp[ecx]&0xffu,4);
            gp[ecx]=this->flags.right(gp[eax],5u,4,true);
            return 0x180562u;
        case datum_size:return 0x180576u;
        case datum_data:return 0x180579u;
        case identifier:gp[ecx]=this->flags.product(gp[ecx],gp[eax]);return 0x18057fu;              // IMUL ECX,EAX
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:gp[edi]=v;break;
        case valid:gp[eax]=(gp[eax]&0xffffff00u)|(v&0xffu);this->test8(v);break;
        case bound:gp[esi]=v;this->compare32(gp[eax],v);break;
        case bits:gp[edx]=v;break;
        // TEST [EDX+ECX*4],EBX; then (free) INC EAX; CMP EAX,ESI.
        case bit_word:
            this->test32(v&gp[ebx]);
            if(!(v&gp[ebx])){gp[eax]=this->flags.increment(gp[eax],4,false);this->compare32(gp[eax],gp[esi]);}
            else this->compare32(gp[eax],logic::none);                                            // CMP EAX,-1
            break;
        case datum_size:gp[ecx]=v;break;
        case datum_data:gp[ebp]=v;break;
        // MOVSX EDX,[ECX+EBP]; ADD ECX,EBP; SHL EDX,10; OR EDX,EAX; MOV ESI,EAX.
        case identifier:
            gp[edx]=logic::widen16(v);gp[ecx]=this->flags.plus(gp[ecx],gp[ebp],4);
            gp[edx]=this->flags.logic(this->flags.shift(gp[edx],16u,4)|gp[eax],4);gp[esi]=gp[eax];break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI; CALL 43980; PUSH EBX; OR ESI,-1; PUSH EBP; LEA EAX,[ESI+1].
    void prepare(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x18052eu,gp[esi]);
        this->call(0x18052fu,0x180534u,0x43980u,prepare_child);
        this->push(0x180534u,gp[ebx]);gp[esi]=this->flags.logic(gp[esi]|0xffffffffu,4);this->push(0x180538u,gp[ebp]);
        kept={gp[edi],gp[esi]};
        next_index();
    }
    // CALL 17FAF0; then LEA EAX,[ESI+1]; TEST EAX,EAX.
    void visit(Kept &kept,std::uint32_t,std::uint32_t) {
        this->call(0x18058cu,0x180591u,0x17faf0u,visit_child);
        kept={this->gp[edi],this->gp[esi]};
        next_index();
    }
    void run() {
        auto &gp=this->gp;
        Memory<Walk> memory{*this};
        if(logic::record_walk::each(memory,*this)==logic::record_walk::Exit::walked) {
            gp[ebp]=this->pop(0x18056cu);gp[ebx]=this->pop(0x18056du);gp[esi]=this->pop(0x18056eu);
        }
        gp[edi]=this->pop(0x18056fu);
        this->ret(0x180570u,0);this->publish(0x180570u);
    }
};
template<class State,class Reference,class Prepare,class Visit>
bool bridge(State &s,Reference reference,Prepare prepare,Visit visit) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x180520u,0x180570u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Walk<State,Prepare,Visit>(v,c,t,prepare,visit);});
    return true;
}
} // namespace halo2::native_record_walk_detail
