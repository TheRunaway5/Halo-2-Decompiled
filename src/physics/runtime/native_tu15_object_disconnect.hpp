#pragma once
// Boundary adapter for the object disconnect, entered at BA7E0
// (native_tu15_object_disconnect_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the handle on the stack
// (RET 4); EBX, EBP, ESI and EDI saved; C0F60 gets the handle, 1, 0, 0, 0
// pushed (and pops them); 1D30F0 gets EDI the partition and the handle and
// the references pushed (and pops them); every register each returns comes
// back; EAX (the header flags byte cleared), ECX and EDX as the original
// leaves them; the flags of the last instruction that sets them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_DISCONNECT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_DISCONNECT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_disconnect_systems.hpp"

namespace halo2::native_object_disconnect_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_disconnect;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_DISCONNECT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_DISCONNECT");return p && *p;}();
    return value;
}
enum Branch : unsigned {unmarked, marked, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object disconnect verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Prepare,class Remove> struct ObjectDisconnect : Port<State> {
    using Access=logic::object_disconnect::Access;
    using Kept=logic::object_disconnect::Kept;
    Prepare &prepare_child;
    Remove &remove_child;
    ObjectDisconnect(State &s,std::uint32_t *c,Trace<State> *t,Prepare &p,Remove &r)
        :Port<State>(s,c,t,{}),prepare_child(p),remove_child(r){}
    // [ESP+C] after PUSH EBX; PUSH EBP.
    std::uint32_t argument()const{return this->gp[esp]+0xcu;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xba7e0u;
        case header_datums:return 0xba7e6u;
        case handle:return 0xba7ebu;
        case object_address:return 0xba801u;
        case attachment:return 0xba810u;
        case attachment_clear:return 0xba82eu;
        case header_flags:return 0xba835u;
        case header_store:return 0xba83du;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;this->push(0xba7e9u,gp[ebx]);this->push(0xba7eau,gp[ebp]);break;   // PUSH EBX; PUSH EBP
        // MOV EBP,[ESP+C]; PUSH ESI; PUSH EDI; PUSH 0; PUSH 0; MOV EAX,EBP; AND EAX,FFFF; PUSH 0; LEA EAX,[EAX+EAX*2].
        case handle:gp[ebp]=v;this->push(0xba7efu,gp[esi]);this->push(0xba7f0u,gp[edi]);this->push(0xba7f1u,0u);
            this->push(0xba7f3u,0u);gp[eax]=fl.logic(v&0xffffu,4);this->push(0xba7fcu,0u);gp[eax]*=3u;break;
        // MOV ESI,[EDX+EAX*4+8]; PUSH 1; LEA EBX,[EDX+EAX*4]; PUSH EBP.
        case object_address:gp[esi]=v;this->push(0xba805u,1u);gp[ebx]=gp[edx]+4u*gp[eax];this->push(0xba80au,gp[ebp]);break;
        // MOV EAX,[ESI+4]; SHR EAX,9; TEST AL,1.
        case attachment:gp[eax]=fl.right(v,logic::partition_bit,4,false);this->test8(gp[eax]&1u);break;
        // MOV AL,[EBX+2]; POP EDI; POP ESI; AND AL,BF; POP EBP.
        case header_flags:gp[edi]=this->pop(0xba838u);gp[esi]=this->pop(0xba839u);
            set_low_byte(gp[eax],fl.logic(v&~logic::header_connected&0xffu,1));gp[ebp]=this->pop(0xba83cu);break;
        default:break;
        }
    }
    // AND [ESI+4],FFFFFEFF: the flags of the result.
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::attachment_clear)(void)this->flags.logic(v,4);}
    // CALL C0F60.
    void prepare(Kept &kept) {
        auto &gp=this->gp;
        this->call(0xba80bu,0xba810u,0xc0f60u,prepare_child);
        kept={gp[esi],gp[ebx],gp[ebp]};
    }
    // MOV EDI,partition; LEA ECX,[ESI+60]; PUSH ECX; PUSH EBP; CALL 1D30F0.
    void remove(Kept &kept,std::uint32_t partition,std::uint32_t references) {
        auto &gp=this->gp;
        gp[edi]=partition;gp[ecx]=references;this->push(0xba827u,references);this->push(0xba828u,kept.handle);
        this->call(0xba829u,0xba82eu,0x1d30f0u,remove_child);
        kept={gp[esi],gp[ebx],gp[ebp]};
    }
    void run() {
        auto &gp=this->gp;
        Memory<ObjectDisconnect> memory{*this};
        this->reached(logic::object_disconnect::disconnect(memory,*this)?Branch::marked:Branch::unmarked);
        gp[ebx]=this->pop(0xba840u);this->ret(0xba841u,4u);this->publish(0xba841u);
    }
};
template<class State,class Reference,class Prepare,class Remove> bool bridge(State &s,Reference reference,Prepare prepare,Remove remove) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xba7e0u,0xba841u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ObjectDisconnect<State,Prepare,Remove>(v,c,t,prepare,remove);});
    return true;
}
} // namespace halo2::native_object_disconnect_detail
