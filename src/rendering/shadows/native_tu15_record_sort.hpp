#pragma once
// Boundary adapter for the record sort, 4B150
// (native_tu15_record_sort_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the first, the records, the mode and the last
// on the stack (RET C); a 0x2C-byte frame below the saved EBX, EBP, ESI and
// EDI; EAX the earlier record + 8 and EBP the index while comparing, ECX,
// ESI and EDI stepping with each REP MOVSD, EDX the last. Each access
// publishes its original instruction.
// HALO2_REFERENCE_RECORD_SORT=1 restores the translated body.
// HALO2_VERIFY_RECORD_SORT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/shadows/native_tu15_record_sort_systems.hpp"

namespace halo2::native_record_sort_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_record_sort;
using Access=logic::record_sort::Access;
using Outcome=logic::record_sort::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RECORD_SORT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RECORD_SORT");return v;}
enum Branch : unsigned {empty, sorted, float_mode0, integer_mode, float_other, swapped, branch_count};
static_assert(unsigned(Outcome::sorted)==Branch::sorted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Record sort"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    std::uint32_t mode_now{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t counter()const{return this->gp[esp]+depth-0x2cu;}
    std::uint32_t start()const{return this->gp[esp]+depth-0x28u;}
    std::uint32_t swap_record()const{return this->gp[esp]+depth-0x24u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case last:return 0x4b153u;
        case counter_init:return 0x4b159u;
        case start_store:return 0x4b166u;
        case start_load:return 0x4b170u;
        case records:return 0x4b178u;
        case mode:return 0x4b184u;
        case later_key:return mode_now==0u?0x4b1f7u:mode_now==1u?0x4b19fu:0x4b190u;
        case earlier_key:return mode_now==0u?0x4b1fcu:mode_now==1u?0x4b19du:0x4b195u;
        case save_read:case save_write:return 0x4b1b7u;
        case last_again:return 0x4b1beu;
        case down_read:case down_write:return 0x4b1c9u;
        case up_read:case up_write:return 0x4b1d6u;
        case counter_load:return 0x4b1e0u;
        case counter_store:return 0x4b1e7u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case last:gp[edx]=v;this->compare32(gp[eax],v);break;                                          // MOV EDX,[ESP+38]; CMP EAX,EDX
        case start_load:gp[ebp]=v;this->compare32(v,gp[edx]);break;                                 // MOV EBP,[ESP+14]; CMP EBP,EDX
        case records:gp[ecx]=v;gp[eax]=v+4u*(9u*gp[ebp])-0x1cu;break;                                  // LEA EAX,[EBP+EBP*8]; LEA EAX,[ECX+EAX*4-1C]
        case mode:                                                                                     // MOV ECX,[ESP+44]; SUB ECX,0; (DEC ECX)
            mode_now=v;gp[ecx]=fl.minus(v,0u,4);if(v)gp[ecx]=fl.increment(gp[ecx],4,true);break;
        case later_key:
            if(mode_now==1u)this->compare32(gp[ecx],v);                                                // CMP ECX,[EAX+24]
            else x[0]=lane0(v);                                                                        // MOVSS XMM0
            break;
        case earlier_key:
            if(mode_now==1u)gp[ecx]=v;                                                                 // MOV ECX,[EAX]
            else fl.scalar_compare(x[0],lane0(v),false);                                               // COMISS XMM0,[...]
            break;
        case last_again:gp[edx]=v;break;
        case counter_load:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        if(a==save_write||a==down_write||a==up_write){const auto d=step();gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
    }
    void saving() {                                                                                    // PUSH EBX; PUSH EBP; INC EAX; PUSH ESI
        auto &gp=this->gp;
        put(0x4b162u,gp[ebx]);put(0x4b163u,gp[ebp]);gp[eax]=this->flags.increment(gp[eax],4,false);put(0x4b165u,gp[esi]);
    }
    void saved(){put(0x4b16au,this->gp[edi]);}                                                         // PUSH EDI
    void compared(std::uint32_t mode_value,bool higher) {
        this->reached(mode_value==0u?Branch::float_mode0:mode_value==1u?Branch::integer_mode:Branch::float_other);
        if(mode_value==1u){set_low_byte(this->gp[ecx],higher?1u:0u);this->test8(higher?1u:0u);}       // SETL CL; TEST CL,CL
        if(higher)this->reached(Branch::swapped);
    }
    void copying(unsigned which,std::uint32_t from,std::uint32_t to) {
        auto &gp=this->gp;
        switch(which) {
        case 0:gp[edx]=from;gp[esi]=from;gp[ecx]=9u;gp[edi]=to;break;                                  // LEA EDX,[EAX-8]; MOV ESI,EDX; MOV ECX,9; LEA EDI,[ESP+18]
        case 1:gp[ebx]=from;gp[edi]=to;gp[ecx]=9u;gp[esi]=gp[ebx];break;                               // LEA EBX,[EAX+1C]; MOV EDI,EDX; MOV ECX,9; MOV ESI,EBX
        default:gp[ecx]=9u;gp[esi]=from;gp[edi]=to;break;                                              // MOV ECX,9; LEA ESI,[ESP+18]; MOV EDI,EBX
        }
    }
    void advanced(std::uint32_t j,std::uint32_t earlier,std::uint32_t last_value) {                    // INC EBP; ADD EAX,24; CMP EBP,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebp]=fl.increment(j-1u,4,false);gp[eax]=fl.plus(earlier+8u-logic::record_size,logic::record_size,4);this->compare32(gp[ebp],last_value);
    }
    void counted(std::uint32_t value,std::uint32_t last_value) {                             // INC EAX; CMP EAX,EDX
        this->gp[eax]=this->flags.increment(value-1u,4,false);this->compare32(this->gp[eax],last_value);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x2cu,4);depth=0x2cu;                                                 // SUB ESP,2C
        Memory<Adapter> memory{*this};
        const auto outcome=logic::record_sort::sort(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::sorted){gp[edi]=take(0x4b1edu);gp[esi]=take(0x4b1eeu);gp[ebp]=take(0x4b1efu);gp[ebx]=take(0x4b1f0u);}
        gp[esp]=fl.plus(gp[esp],0x2cu,4);                                                              // ADD ESP,2C
        this->ret(0x4b1f4u,0xcu);this->publish(0x4b1f4u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x4b150u,0x4b1f4u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_record_sort_detail
