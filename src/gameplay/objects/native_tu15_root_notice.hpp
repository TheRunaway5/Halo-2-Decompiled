#pragma once
// Boundary adapter for the root notice, entered at 1FA110
// (native_tu15_root_notice_systems.hpp). Its callers and 184EC0 keep the
// original ABI, which the adapter owns: EAX the record, ECX the other, the
// three arguments on the stack (RET C); EBX and ESI and, past the object's
// test, EBP and EDI saved; 184EC0 (through the unit's dispatch) gets ECX
// the cluster and its six arguments pushed (and pops them), and every
// register it returns comes back; the result in AL (XOR AL,AL, or 184EC0's)
// above the rest of EAX; ECX and EDX as the original leaves them; the flags
// of the last instruction that sets them. Each access publishes its
// original instruction.
// HALO2_REFERENCE_ROOT_NOTICE=1 restores the translated body.
// HALO2_VERIFY_ROOT_NOTICE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_root_notice_systems.hpp"

namespace halo2::native_root_notice_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_root_notice;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ROOT_NOTICE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ROOT_NOTICE");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_object, no_roots, not_root, notified, branch_count};
static_assert(unsigned(logic::root_notice::Outcome::notified)==Branch::notified);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Root notice verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Notice> struct RootNotice : Port<State> {
    using Access=logic::root_notice::Access;
    Notice &notice_route;
    std::uint32_t depth{};   // bytes pushed past the four saved registers
    RootNotice(State &s,std::uint32_t *c,Trace<State> *t,Notice &n):Port<State>(s,c,t,{}),notice_route(n){}
    // The k-th argument past the return address and the four saves.
    std::uint32_t argument_slot(std::uint32_t k)const{return this->gp[esp]+depth+0x14u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case object:return 0x1fa116u;
        case roots:this->push(0x1fa120u,this->gp[ebp]);this->push(0x1fa121u,this->gp[edi]);return 0x1fa122u;   // PUSH EBP; PUSH EDI
        case count:return 0x1fa128u;
        case root:return 0x1fa140u;
        // AND ECX,FFFF; LEA EAX,[ECX+ECX*2] before MOV ECX,[4ECBD0].
        case headers:this->gp[ecx]=this->flags.logic(this->gp[ecx]&0xffffu,4);this->gp[eax]=this->gp[ecx]*3u;return 0x1fa15cu;
        case header_datums:return 0x1fa162u;
        case object_address:return 0x1fa165u;
        case third:return 0x1fa169u;
        case second_argument:return 0x1fa16du;
        case first_argument:return 0x1fa172u;
        case cluster:return 0x1fa180u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV ECX,[ESI+10]; XOR AL,AL; CMP ECX,-1.
        case object:gp[ecx]=v;set_low_byte(gp[eax],fl.logic(0u,1));fl.minus(v,logic::none,4);break;
        case roots:gp[edi]=v;break;
        // MOV EBP,[EDI+1D80]; XOR EDX,EDX; TEST EBP,EBP.
        case count:gp[ebp]=v;gp[edx]=fl.logic(0u,4);this->test32(v);break;
        case root:fl.minus(v,gp[ecx],4);break;                                                      // CMP [EDI],ECX
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;break;
        case object_address:gp[eax]=v;break;
        case third:gp[ecx]=v;break;
        case second_argument:gp[edx]=v;break;
        case first_argument:gp[ecx]=v;break;
        case cluster:gp[ecx]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // ADD EDI,1D00; LEA EBX,[EBX].
    void walking(){auto &gp=this->gp;gp[edi]=this->flags.plus(gp[edi],0x1d00u,4);}
    // INC EDX; ADD EDI,4; CMP EDX,EBP.
    void stepped() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.increment(gp[edx],4,false);gp[edi]=fl.plus(gp[edi],4u,4);fl.minus(gp[edx],gp[ebp],4);
    }
    void pushed(std::uint32_t third_value){this->push(0x1fa171u,third_value);depth=4u;}            // PUSH ECX
    // PUSH EDX; PUSH ECX; LEA EDX,[EBX+24]; PUSH EDX; LEA ECX,[ESI+58]; PUSH ECX.
    void passing(std::uint32_t second_value,std::uint32_t first_value,std::uint32_t other_part,std::uint32_t second_part) {
        auto &gp=this->gp;
        this->push(0x1fa176u,second_value);this->push(0x1fa177u,first_value);
        gp[edx]=other_part;this->push(0x1fa17bu,other_part);gp[ecx]=second_part;this->push(0x1fa17fu,second_part);
    }
    // ADD ESI,E8; PUSH ESI; CALL 184EC0.
    std::uint32_t notice(std::uint32_t first_part,std::uint32_t) {
        auto &gp=this->gp;
        gp[esi]=this->flags.plus(gp[esi],0xe8u,4);this->push(0x1fa18cu,first_part);
        this->call_routed(notice_route,0x1fa18du,0x1fa192u,0x184ec0u);
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::root_notice::Outcome;
        this->push(0x1fa110u,gp[ebx]);this->push(0x1fa111u,gp[esi]);gp[esi]=gp[eax];gp[ebx]=gp[ecx];   // PUSH EBX; PUSH ESI; MOV ESI,EAX; MOV EBX,ECX
        Memory<RootNotice> memory{*this};
        const auto result=logic::root_notice::notify(memory,*this,gp[esi],gp[ebx]);
        this->reached(unsigned(result.outcome));
        if(result.outcome==Outcome::not_root) {
            gp[edi]=this->pop(0x1fa14cu);gp[ebp]=this->pop(0x1fa14du);gp[esi]=this->pop(0x1fa14eu);gp[ebx]=this->pop(0x1fa14fu);
            this->ret(0x1fa150u,0xcu);this->publish(0x1fa150u);return;
        }
        set_low_byte(gp[eax],result.value);
        if(result.outcome!=Outcome::no_object){gp[edi]=this->pop(0x1fa192u);gp[ebp]=this->pop(0x1fa193u);}
        gp[esi]=this->pop(0x1fa194u);gp[ebx]=this->pop(0x1fa195u);this->ret(0x1fa196u,0xcu);this->publish(0x1fa196u);
    }
};
template<class State,class Reference,class Notice> bool bridge(State &s,Reference reference,Notice notice) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1fa110u,0x1fa196u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RootNotice<State,Notice>(v,c,t,notice);});
    return true;
}
} // namespace halo2::native_root_notice_detail
