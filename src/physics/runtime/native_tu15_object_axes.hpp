#pragma once
// Boundary adapter for the object axes, entered at BBFF0
// (native_tu15_object_axes_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the object handle, ESI the forward output and
// EDI the up output (either zero); EBX saved around the node arithmetic;
// EAX, ECX and EDX as the original leaves them (the copies run through
// them; the node matrix, the object and its parent otherwise); XMM0 to XMM4
// as the last turn leaves them; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_AXES=1 restores the translated body.
// HALO2_VERIFY_OBJECT_AXES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_axes_systems.hpp"

namespace halo2::native_object_axes_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_axes;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_AXES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_AXES");return p && *p;}();
    return value;
}
enum Branch : unsigned {copied_forward, copied_up, turned_forward, turned_up, unwanted, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object axes verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// The turn's instructions for each axis, in Access order from axis_y.
inline constexpr std::uint32_t turn_pcs[2][15]{
    {0xbc068u,0xbc06du,0xbc072u,0xbc077u,0xbc07cu,0xbc08du,0xbc09au,0xbc09eu,0xbc0a3u,0xbc0b4u,0xbc0c1u,0xbc0c6u,0xbc0cfu,0xbc0d8u,0xbc0e9u},
    {0xbc0f6u,0xbc0feu,0xbc106u,0xbc10bu,0xbc110u,0xbc121u,0xbc12eu,0xbc132u,0xbc137u,0xbc148u,0xbc155u,0xbc15au,0xbc163u,0xbc16cu,0xbc17du},
};
// The copies' reads and stores, and the register each read lands in.
inline constexpr std::uint32_t copy_reads[2][3]{{0xbc014u,0xbc018u,0xbc01eu},{0xbc02fu,0xbc033u,0xbc039u}},
    copy_stores[2][3]{{0xbc016u,0xbc01bu,0xbc021u},{0xbc031u,0xbc036u,0xbc03cu}};
inline constexpr unsigned copy_registers[2][3]{{eax,eax,edx},{eax,edx,eax}};
template<class State> struct Axes : Port<State> {
    using Access=logic::object_axes::Access;
    using Axis=logic::object_axes::Axis;
    unsigned axis{},word{};bool parented{},up_copied{};
    Axes(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xbbff0u;
        case header_datums:return 0xbbff6u;
        case object_address:return 0xbc001u;
        case parent:return 0xbc005u;
        case axis_copy:return copy_reads[axis][word];
        case axis_store:return copy_stores[axis][word++];
        case parent_node:this->push(0xbc040u,this->gp[ebx]);return 0xbc041u;                        // PUSH EBX
        case parent_address:return 0xbc050u;
        case nodes:return 0xbc054u;
        default:return turn_pcs[axis][unsigned(a)-unsigned(axis_y)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers:case object_address:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case parent:gp[eax]=v;fl.minus(v,logic::none,4);break;                                     // CMP EAX,-1
        case axis_copy:gp[copy_registers[axis][word]]=v;break;
        // MOVSX EBX; IMUL EBX,EBX,34; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case parent_node:
            gp[ebx]=fl.product(std::uint32_t(std::int32_t(std::int8_t(v))),logic::node_size);
            gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case parent_address:gp[edx]=v;break;
        // MOVSX EAX; ADD EBX,EDX; ADD EAX,EBX.
        case nodes:gp[eax]=logic::widen16(v);gp[ebx]=fl.plus(gp[ebx],gp[edx],4);gp[eax]=fl.plus(gp[eax],gp[ebx],4);
            parented=true;break;
        case axis_y:x[1]=logic::sse::scalar(v);break;
        case axis_z:x[2]=logic::sse::scalar(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // TEST ESI,ESI (POP EBX after it on the parent path) / TEST EDI,EDI.
    bool tested(Axis which,std::uint32_t output) {
        this->test32(output);
        if(which==Axis::forward && parented)this->gp[ebx]=this->pop(0xbc061u);
        if(!output)this->reached(Branch::unwanted);
        return output!=0;
    }
    // LEA EDX,[ECX+70] / ADD ECX,7C.
    void copying(Axis which) {
        auto &gp=this->gp;
        axis=which==Axis::up?1u:0u;word=0;
        if(which==Axis::forward){gp[edx]=gp[ecx]+logic::object::forward;this->reached(Branch::copied_forward);}
        else{gp[ecx]=this->flags.plus(gp[ecx],logic::object::up,4);up_copied=true;this->reached(Branch::copied_up);}
    }
    void turning(Axis which){axis=which==Axis::up?1u:0u;this->reached(which==Axis::up?Branch::turned_up:Branch::turned_forward);}
    void turned(const logic::object_axes::Turned &t) {
        auto &x=this->s.xmm;
        x[0]=logic::sse::scalar(t.xmm0);x[1]=logic::sse::scalar(t.xmm1);x[2]=logic::sse::scalar(t.xmm2);
        x[3]=logic::sse::scalar(t.xmm3);x[4]=logic::sse::scalar(t.xmm4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Axes> memory{*this};
        (void)logic::object_axes::of(memory,*this,gp[eax],gp[esi],gp[edi]);
        const auto exit=up_copied?0xbc03fu:0xbc182u;
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbbff0u,0xbc182u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Axes<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_object_axes_detail
