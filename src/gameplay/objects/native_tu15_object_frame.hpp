#pragma once
// Boundary adapter for the object frame, entered at BC190
// (native_tu15_object_frame_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the handle, the frame on the
// stack (RET 4); EBX, EBP, ESI and EDI saved; 144BF0 gets EAX, ECX and ESI
// the object's up, forward and position and EDX the frame, 145560 ECX the
// parent's node and EDX the frame, pushed too (RET 4), every register each
// returns coming back; EAX the frame, ECX and EDX as the original leaves
// them; the flags of the parent's CMP (or the second child's). Each access
// publishes its original instruction.
// HALO2_REFERENCE_OBJECT_FRAME=1 restores the translated body.
// HALO2_VERIFY_OBJECT_FRAME=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_frame_systems.hpp"

namespace halo2::native_object_frame_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_frame;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_FRAME");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_FRAME");return p && *p;}();
    return value;
}
enum Branch : unsigned {root, child, branch_count};
static_assert(unsigned(logic::object_frame::Outcome::child)==Branch::child);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object frame verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Own,class Parented> struct ObjectFrame : Port<State> {
    using Access=logic::object_frame::Access;
    using Kept=logic::object_frame::Kept;
    Own &own_child;
    Parented &parented_child;
    ObjectFrame(State &s,std::uint32_t *c,Trace<State> *t,Own &o,Parented &p):Port<State>(s,c,t,{}),own_child(o),parented_child(p){}
    std::uint32_t argument()const{return this->gp[esp]+8u;}                                      // past PUSH EBX
    void refresh(Kept &kept){kept={this->gp[ebp],this->gp[edi],this->gp[ebx]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case frame_argument:return 0xbc191u;
        case objects:this->push(0xbc195u,gp[ebp]);return 0xbc196u;                                // PUSH EBP
        case object_data:return 0xbc19cu;
        // AND EAX,FFFF; PUSH ESI; LEA EAX,[EAX+EAX*2]; PUSH EDI.
        case object:
            gp[eax]=fl.logic(gp[eax]&0xffffu,4);this->push(0xbc1a4u,gp[esi]);gp[eax]*=3u;this->push(0xbc1a8u,gp[edi]);
            return 0xbc1a9u;
        case parent:return 0xbc1bdu;
        // AND EAX,FFFF; LEA EDX,[EAX+EAX*2].
        case object_data_again:gp[eax]=fl.logic(gp[eax]&0xffffu,4);gp[edx]=3u*gp[eax];return 0xbc1cdu;
        case parent_object:return 0xbc1d0u;
        case parent_node:return 0xbc1d4u;
        case nodes:return 0xbc1d8u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case frame_argument:gp[ebx]=v;break;
        case objects:gp[ebp]=v;break;
        case object_data:gp[ecx]=v;break;
        case object:gp[edi]=v;break;
        case parent:gp[eax]=v;this->compare32(v,logic::none);break;                               // CMP EAX,-1
        case object_data_again:case parent_object:gp[eax]=v;break;
        case parent_node:gp[edx]=logic::widen8(v&0xffu);break;
        case nodes:gp[ecx]=logic::widen16(v&0xffffu);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // LEA EAX,[EDI+7C]; LEA ECX,[EDI+70]; LEA ESI,[EDI+64]; MOV EDX,EBX; CALL 144BF0.
    void own(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=kept.object+logic::object::up;gp[ecx]=kept.object+logic::object::forward;
        gp[esi]=kept.object+logic::object::position;gp[edx]=kept.frame;
        this->call(0xbc1b8u,0xbc1bdu,0x144bf0u,own_child);
        refresh(kept);
    }
    // IMUL EDX,EDX,34; ADD EDX,EAX; ADD ECX,EDX; PUSH EBX; MOV EDX,EBX; CALL 145560.
    void parented(Kept &kept,std::uint32_t node) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.product(gp[edx],logic::node_size);gp[edx]=fl.plus(gp[edx],gp[eax],4);
        fl.plus(node-gp[edx],gp[edx],4);gp[ecx]=node;
        this->push(0xbc1e6u,kept.frame);gp[edx]=kept.frame;
        this->call(0xbc1e9u,0xbc1eeu,0x145560u,parented_child);
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xbc190u,gp[ebx]);                                                            // PUSH EBX
        Memory<ObjectFrame> memory{*this};
        std::uint32_t result{};
        this->reached(unsigned(logic::object_frame::of(memory,*this,gp[eax],result)));
        gp[edi]=this->pop(0xbc1eeu);gp[esi]=this->pop(0xbc1efu);gp[ebp]=this->pop(0xbc1f0u);
        gp[eax]=result;gp[ebx]=this->pop(0xbc1f3u);
        this->ret(0xbc1f4u,4u);this->publish(0xbc1f4u);
    }
};
template<class State,class Reference,class Own,class Parented> bool bridge(State &s,Reference reference,Own own,Parented parented) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbc190u,0xbc1f4u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ObjectFrame<State,Own,Parented>(v,c,t,own,parented);});
    return true;
}
} // namespace halo2::native_object_frame_detail
