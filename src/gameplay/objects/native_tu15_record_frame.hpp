#pragma once
// Boundary adapter for the record frame, entered at 1DC610
// (native_tu15_record_frame_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EDI the record, the argument on
// the stack (RET 4); a 0x34-byte frame; 1D8B90 gets EAX the marker, ECX the
// record and ESI the frame (ESI saved around it), BC190 EAX the object and
// the frame pushed (RET 4), 1DC680 EAX the record, ECX the frame and the
// argument pushed (RET 4), every register each returns coming back; the
// flags of the closing ADD ESP,34. Each access publishes its original
// instruction.
// HALO2_REFERENCE_RECORD_FRAME=1 restores the translated body.
// HALO2_VERIFY_RECORD_FRAME=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_record_frame_systems.hpp"

namespace halo2::native_record_frame_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_frame;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_FRAME");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_FRAME");return p && *p;}();
    return value;
}
enum Branch : unsigned {marked, unmarked, past_markers, branch_count};
static_assert(unsigned(logic::record_frame::Outcome::past_markers)==Branch::past_markers);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record frame verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route,class Take> struct RecordFrame : Port<State> {
    using Access=logic::record_frame::Access;
    Route &route;
    Take &take_child;
    bool marked{};
    RecordFrame(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Take &k):Port<State>(s,c,t,{}),route(r),take_child(k){}
    // [ESP+3C] with ESI saved, [ESP+38] without.
    std::uint32_t argument()const{return this->gp[esp]+(marked?0x3cu:0x38u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case marker:return 0x1dc610u;
        case markers:return 0x1dc61au;
        case object:return 0x1dc654u;
        case argument:return marked?0x1dc63du:0x1dc660u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV AL,[EDI+18]; SUB ESP,34; TEST AL,AL.
        case marker:set_low_byte(gp[eax],v);gp[esp]=fl.minus(gp[esp],0x34u,4);this->test8(v);break;
        // MOV EDX,[EDI+74]; MOVSX ECX,AL; CMP ECX,EDX.
        case markers:gp[edx]=v;gp[ecx]=logic::widen8(gp[eax]&0xffu);this->compare32(gp[ecx],v);break;
        case object:gp[eax]=v;break;
        case argument:gp[eax]=v;if(marked)gp[esi]=this->pop(0x1dc641u);break;                    // (POP ESI)
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOVSX AX,AL; CMP AX,-1; PUSH ESI; MOVSX EAX,AX; LEA ESI,[ESP+4]; MOV ECX,EDI; CALL 1D8B90.
    std::uint32_t marker_frame(std::uint32_t record_address,std::uint32_t marker_value) {
        auto &gp=this->gp;
        marked=true;
        this->flags.minus(marker_value&0xffffu,0xffffu,2);
        this->push(0x1dc62eu,gp[esi]);
        gp[eax]=marker_value;gp[esi]=gp[esp]+4u;gp[ecx]=record_address;
        this->call_routed(route,0x1dc638u,0x1dc63du,0x1d8b90u);
        return gp[edi];
    }
    // LEA EDX,[ESP]; PUSH EDX; CALL BC190.
    std::uint32_t object_frame(std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        gp[edx]=gp[esp];this->push(0x1dc65au,gp[edx]);
        this->call_routed(route,0x1dc65bu,0x1dc660u,0xbc190u);
        return gp[edi];
    }
    // PUSH EAX; LEA ECX,[ESP+4]; MOV EAX,EDI; CALL 1DC680.
    void take(std::uint32_t record_address,std::uint32_t argument_value) {
        auto &gp=this->gp;
        const auto site=marked?0x1dc642u:0x1dc664u;
        this->push(site,argument_value);gp[ecx]=gp[esp]+4u;gp[eax]=record_address;
        this->call(site+7u,site+12u,0x1dc680u,take_child);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<RecordFrame> memory{*this};
        this->reached(unsigned(logic::record_frame::take(memory,*this,gp[edi])));
        gp[esp]=fl.plus(gp[esp],0x34u,4);                                                         // ADD ESP,34
        const auto exit=marked?0x1dc651u:0x1dc673u;
        this->ret(exit,4u);this->publish(exit);
    }
};
template<class State,class Reference,class Route,class Take> bool bridge(State &s,Reference reference,Route route,Take take) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1dc610u,0x1dc673u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RecordFrame<State,Route,Take>(v,c,t,route,take);});
    return true;
}
} // namespace halo2::native_record_frame_detail
