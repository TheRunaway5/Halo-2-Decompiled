#pragma once
// Boundary adapter for the reference pose, entered at 1CF070
// (native_tu15_reference_pose_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the reference, ESI the
// vector, EBX the value, the argument on the stack (RET 4); 12 bytes of
// stack for the record (SUB ESP,C ... ADD ESP,C); EDI saved; 1E5DF0 gets
// EDI the record and the definition and the reference's +4 pushed, 212D40
// EAX the value, ECX the vector, EDX the record and the argument pushed;
// each pops its arguments and every register each returns comes back; XMM0
// zero; ECX and EDX as the original leaves them; the flags of the last
// instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_REFERENCE_POSE=1 restores the translated body.
// HALO2_VERIFY_REFERENCE_POSE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_reference_pose_systems.hpp"

namespace halo2::native_reference_pose_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_reference_pose;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_REFERENCE_POSE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_REFERENCE_POSE");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_reference, posed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Reference pose verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr std::uint32_t frame_bytes=0xcu;
template<class State,class Record,class Pose> struct ReferencePose : Port<State> {
    using Access=logic::reference_pose::Access;
    using Kept=logic::reference_pose::Kept;
    Record &record_child;
    Pose &pose_child;
    std::uint32_t component{},depth{};   // bytes pushed over the record
    ReferencePose(State &s,std::uint32_t *c,Trace<State> *t,Record &r,Pose &p):Port<State>(s,c,t,{}),record_child(r),pose_child(p){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    // [ESP+14] past the record, EDI and the 212D40 argument slot to come.
    std::uint32_t argument_slot()const{return this->gp[esp]+depth+frame_bytes+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t clears[]{0x1cf073u,0x1cf077u,0x1cf07cu};
        switch(a) {
        case vector_clear:if(component==0)this->s.xmm[0]=_mm_setzero_ps();return clears[component++];   // XORPS XMM0,XMM0
        case value_clear:return 0x1cf084u;
        case high:return 0x1cf088u;
        case local_value:this->gp[ecx]=this->flags.logic(0u,4);return 0x1cf091u;                    // XOR ECX,ECX
        case local_data:return 0x1cf094u;
        case local_group:return 0x1cf098u;
        case local_kind:return 0x1cf09cu;
        case second:return 0x1cf0a0u;
        case tag:return 0x1cf0a3u;
        case tags:return 0x1cf0a7u;
        case definition:return 0x1cf0b5u;
        case argument:return 0x1cf0c3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case high:fl.minus(v&0xffffu,0xffffu,2);break;                                              // CMP word [EAX+6],-1
        case second:gp[ecx]=v;break;
        // MOV EAX,[EAX]; PUSH EDI; PUSH ECX.
        case tag:gp[eax]=v;this->push(0x1cf0a5u,gp[edi]);this->push(0x1cf0a6u,gp[ecx]);depth=8u;break;
        case tags:gp[ecx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;              // AND EAX,FFFF; SHL EAX,4
        case definition:gp[edx]=v;break;
        case argument:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void reserve(){this->gp[esp]=this->flags.minus(this->gp[esp],frame_bytes,4);}                  // SUB ESP,C
    // PUSH EDX; LEA EDI,[ESP+C]; CALL 1E5DF0.
    void record(Kept &kept,std::uint32_t definition_address,std::uint32_t) {
        auto &gp=this->gp;
        gp[edx]=definition_address;this->push(0x1cf0b9u,definition_address);gp[edi]=gp[esp]+frame_bytes;
        this->call(0x1cf0beu,0x1cf0c3u,0x1e5df0u,record_child);
        depth=4u;kept={gp[esi],gp[ebx]};
    }
    // PUSH EAX; MOV EAX,EBX; MOV ECX,ESI; LEA EDX,[ESP+8]; CALL 212D40; POP EDI.
    void pose(Kept &kept,std::uint32_t argument_value) {
        auto &gp=this->gp;
        this->push(0x1cf0c7u,argument_value);gp[eax]=kept.value;gp[ecx]=kept.vector;gp[edx]=gp[esp]+8u;
        this->call(0x1cf0d0u,0x1cf0d5u,0x212d40u,pose_child);
        gp[edi]=this->pop(0x1cf0d5u);depth=0u;
    }
    void run() {
        auto &gp=this->gp;
        Memory<ReferencePose> memory{*this};
        this->reached(logic::reference_pose::resolve(memory,*this,gp[eax],gp[esi],gp[ebx])?Branch::posed:Branch::no_reference);
        gp[esp]=this->flags.plus(gp[esp],frame_bytes,4);this->ret(0x1cf0d9u,4u);this->publish(0x1cf0d9u);   // ADD ESP,C
    }
};
template<class State,class Reference,class Record,class Pose> bool bridge(State &s,Reference reference,Record record,Pose pose) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1cf070u,0x1cf0d9u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ReferencePose<State,Record,Pose>(v,c,t,record,pose);});
    return true;
}
} // namespace halo2::native_reference_pose_detail
