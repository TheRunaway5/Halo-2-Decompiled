#pragma once
// Boundary adapter for the body list ray, 315190
// (native_tu15_body_list_ray_systems.hpp). Its callers and the virtual call
// keep the original ABI, which the adapter owns: ECX the list, the input and
// the collector on the stack (RET 8); an EBP frame, ESP aligned down to 16
// below 0x34 bytes, EBX, ESI and EDI saved below it and the frame read from
// the ESP the call left; each shape's cast (vtable +0x24: ECX the shape, the
// local input, the body and the collector pushed, RET C) through the unit's
// dispatch at its call instruction, every register it returns coming back.
// XMM0 to XMM7 hold what the original's moves, shuffles and arithmetic leave
// in them at the call. Each access publishes its original instruction.
// HALO2_REFERENCE_BODY_LIST_RAY=1 restores the translated body.
// HALO2_VERIFY_BODY_LIST_RAY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_body_list_ray_systems.hpp"

namespace halo2::native_body_list_ray_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_body_list_ray;
using Access=logic::body_list_ray::Access;
using Kept=logic::body_list_ray::Kept;
using Outcome=logic::body_list_ray::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BODY_LIST_RAY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BODY_LIST_RAY");return v;}
enum Branch : unsigned {empty, cast, filtered, skipped, called, branch_count};
static_assert(unsigned(Outcome::cast)==Branch::cast);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Body list ray"};return v;}
inline constexpr std::uint32_t sites[]{0x31519cu,0x31519fu,0x3151a2u,0x3151a6u,0x3151adu,0x3151b3u,0x3151b6u,0x3151c3u,0x3151c9u,0x3151cdu,
    0x3151d4u,0x3151e0u,0x3151e4u,0x3151e8u,0x3151eau,0x3151f4u,0u,0u,0u,0x3152c8u,0x3152cau,0x3152d3u,0x3152d6u,0x3152deu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::remaining_store)+1u);
// Each body's motion reads (the third column, the translation, the first and second columns, twice), ray ends and local stores.
inline constexpr std::uint32_t column_sites[]{0x3151f7u,0x3151fbu,0x3151ffu,0x315203u,0x315265u,0x315269u,0x31526du,0x315270u};
inline constexpr std::uint32_t point_sites[]{0x315207u,0x315256u},local_sites[]{0x315260u,0x3152c3u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers
    unsigned columns{},points{},locals{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t argument(unsigned n)const{return this->gp[ebp]+8u+4u*n;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case column:return column_sites[columns++];
        case point:return point_sites[points];
        case local_store:return local_sites[locals++];
        case entry:columns=points=locals=0u;return sites[unsigned(a)];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case input_argument:gp[edi]=v;break;
        case filter_info:case world_load:case world_filter:case count:case entry:case motion_load:case entry_again:case remaining_load:
            gp[eax]=v;break;
        case filter_enabled:set_low_byte(gp[eax],v);break;
        case bodies:gp[esi]=v;break;
        case collector_argument:gp[ebx]=v;break;
        case shape_load:gp[ecx]=v;break;
        case vtable:gp[edx]=v;break;
        default:break;
        }
    }
    void loaded_vector(Access a,std::uint32_t,__m128) {
        // The ray's from end is followed by SUBPS and ADD EAX,20 (the motion's rotation, read again from there).
        if(a==Access::point && points++==0u)this->gp[eax]=this->flags.plus(this->gp[eax],logic::motion::first,4);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,34
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x315190u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff0u,4),logic::local::size,4);
    }
    void saving() {                                                                                      // PUSH EBX; PUSH ESI; PUSH EDI
        auto &gp=this->gp;
        this->push(0x315199u,gp[ebx]);this->push(0x31519au,gp[esi]);this->push(0x31519bu,gp[edi]);depth=0u;
    }
    void filter_tested(std::uint32_t enabled) {                                                         // XOR EDX,EDX; TEST AL,AL
        this->gp[edx]=this->flags.logic(0u,4);this->test8(enabled);
    }
    void compared(std::uint32_t v){this->compare32(v,this->gp[edx]);}                                  // CMP EAX,EDX
    std::uint32_t filter_interface(std::uint32_t filter_object) {                                       // ADD EAX,10
        this->reached(Branch::filtered);
        return this->gp[eax]=this->flags.plus(filter_object,logic::filter::interface,4);
    }
    std::uint32_t decremented(std::uint32_t v){return this->gp[eax]=this->flags.increment(v,4,true);} // DEC EAX
    std::uint32_t incremented(std::uint32_t v){return this->gp[eax]=this->flags.increment(v,4,false);}// INC EAX
    void tested(std::uint32_t shape_address) {                                                          // TEST ECX,ECX
        this->test32(shape_address);
        if(!shape_address)this->reached(Branch::skipped);
    }
    void transformed(const logic::Transpose &t,const logic::Local &l) {                                // the shuffles and arithmetic's registers
        auto &x=this->s.xmm;
        x[0]=l.offset;x[1]=l.point;x[2]=l.z_term;x[3]=logic::sse::broadcast<0>(l.offset);x[4]=t.low;x[5]=l.y_term;
        x[6]=logic::sse::broadcast<2>(l.offset);x[7]=t.high;
    }
    void pushed(std::uint32_t collector,std::uint32_t body_address) {                                  // PUSH EBX; PUSH EAX; LEA EAX,[ESP+18]; PUSH EAX
        auto &gp=this->gp;
        put(0x3152ccu,collector);put(0x3152cdu,body_address);gp[eax]=local(logic::local::from);put(0x3152d2u,gp[eax]);
    }
    void cast(std::uint32_t target,Kept &kept) {                                                        // CALL [EDX+24]
        auto child=[&](State &v){dispatch(v,target,0x3152d3u);};
        this->call(0x3152d3u,0x3152d6u,target,child);depth-=12u;
        kept={this->gp[ebx],this->gp[esi],this->gp[edi]};
        this->reached(Branch::called);
    }
    std::uint32_t advanced(std::uint32_t cursor){return this->gp[esi]=this->flags.plus(cursor,4u,4);} // ADD ESI,4
    void restoring() {                                                                                   // POP EDI; POP ESI; POP EBX; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[edi]=this->pop(0x3152e8u);gp[esi]=this->pop(0x3152e9u);gp[ebx]=this->pop(0x3152eau);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x3152edu);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::body_list_ray::cast(memory,*this,gp[ecx],Kept{gp[ebx],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(0x3152eeu,8u);this->publish(0x3152eeu);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x315190u,0x3152eeu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_body_list_ray_detail
