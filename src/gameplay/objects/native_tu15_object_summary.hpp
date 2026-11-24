#pragma once
// Boundary adapter for the object summary, entered at BA910
// (native_tu15_object_summary_systems.hpp). Its callers and 16E290 keep the
// original ABI, which the adapter owns: EDX the object handle, EDI the
// summary; ESI saved; 16E290 gets EAX the handle, and every register it
// returns comes back; EAX, ECX and EDX as the original leaves them (the
// last part of the vector, the word, the type); the flags of the last
// instruction that sets them; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_OBJECT_SUMMARY=1 restores the translated body.
// HALO2_VERIFY_OBJECT_SUMMARY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_summary_systems.hpp"

namespace halo2::native_object_summary_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_summary;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_SUMMARY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_SUMMARY");return p && *p;}();
    return value;
}
enum Branch : unsigned {plain, with_bits, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object summary verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Bits> struct ObjectSummary : Port<State> {
    using Access=logic::object_summary::Access;
    using Kept=logic::object_summary::Kept;
    Bits &bits_child;
    std::uint32_t part{};
    bool asked{};
    ObjectSummary(State &s,std::uint32_t *c,Trace<State> *t,Bits &b):Port<State>(s,c,t,{}),bits_child(b){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t vector_pcs[]{0xba94fu,0xba956u,0xba95cu},vector_store_pcs[]{0xba954u,0xba959u,0xba95fu};
        switch(a) {
        case headers:return 0xba91bu;
        case header_datums:return 0xba921u;
        case object_address:this->push(0xba924u,this->gp[esi]);return 0xba925u;                    // PUSH ESI
        case flags:return 0xba929u;
        case type:return 0xba93du;
        case bits_store:return 0xba945u;
        case type_store:return 0xba949u;
        case vector:if(part==0)this->gp[eax]=this->gp[esi]+logic::object::vector;return vector_pcs[part];   // LEA EAX,[ESI+40]
        case vector_store:return vector_store_pcs[part];
        case word:return 0xba962u;
        case word_store:return 0xba965u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case header_datums:gp[ecx]=v;break;
        case object_address:gp[esi]=v;break;
        // MOV ECX,[ESI+4]; SHR ECX,9; XOR EAX,EAX; TEST CL,1.
        case flags:gp[ecx]=fl.right(v,9u,4,false);gp[eax]=fl.logic(0u,4);this->test8(gp[ecx]&1u);break;
        case type:gp[edx]=(gp[edx]&0xffff0000u)|(logic::widen8(v)&0xffffu);break;                // MOVSX DX,byte
        // MOV EDX,[EAX]; LEA ECX,[EDI+4] / MOV EDX,[EAX+4] / MOV EAX,[EAX+8].
        case vector:if(part==2)gp[eax]=v;else gp[edx]=v;if(part==0)gp[ecx]=gp[edi]+logic::summary::vector;break;
        case word:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void component(std::uint32_t k){part=k;}
    // MOV EAX,EDX; CALL 16E290.
    std::uint32_t state_bits(Kept &kept,std::uint32_t handle) {
        auto &gp=this->gp;
        gp[eax]=handle;this->call(0xba938u,0xba93du,0x16e290u,bits_child);
        kept={gp[esi],gp[edi]};asked=true;
        return gp[eax]&0xffffu;
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        // MOV ECX,EDX; AND ECX,FFFF; LEA EAX,[ECX+ECX*2].
        gp[ecx]=fl.logic(gp[edx]&0xffffu,4);gp[eax]=gp[ecx]*3u;
        Memory<ObjectSummary> memory{*this};
        logic::object_summary::write(memory,*this,gp[edx],gp[edi]);
        this->reached(asked?Branch::with_bits:Branch::plain);
        gp[esi]=this->pop(0xba968u);this->ret(0xba969u,0);this->publish(0xba969u);
    }
};
template<class State,class Reference,class Bits> bool bridge(State &s,Reference reference,Bits bits) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xba910u,0xba969u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ObjectSummary<State,Bits>(v,c,t,bits);});
    return true;
}
} // namespace halo2::native_object_summary_detail
