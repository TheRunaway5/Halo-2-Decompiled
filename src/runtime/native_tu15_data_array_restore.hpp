#pragma once
// Boundary adapter for the data array restore, 16E510
// (native_tu15_data_array_restore_systems.hpp). Its callers and strncpy keep
// the original ABI, which the adapter owns: ESI the array, EAX the count,
// ECX the data (RET); EBP and EDI saved (EBX past the count's test);
// strncpy (32A4E0, cdecl: the signature, the array and 2 pushed, the caller
// cleaning) through the unit's dispatch, every register it returns coming
// back; unused elements filled as REP STOSD and REP STOSB fill them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_DATA_ARRAY_RESTORE=1 restores the translated body.
// HALO2_VERIFY_DATA_ARRAY_RESTORE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "runtime/native_tu15_data_array_restore_systems.hpp"

namespace halo2::native_data_array_restore_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_data_array_restore;
using Access=logic::data_array_restore::Access;
using Kept=logic::data_array_restore::Kept;
using Outcome=logic::data_array_restore::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DATA_ARRAY_RESTORE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DATA_ARRAY_RESTORE");return v;}
enum Branch : unsigned {empty, restored, used, filled, lowered, branch_count};
static_assert(unsigned(Outcome::restored)==Branch::restored);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Data array restore"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x16e510u,0x16e510u,0x16e51fu,0x16e522u,0x16e525u,0x16e529u,0x16e52cu,0x16e52fu,0x16e537u,0x16e53au,
    0x16e53au,0x16e547u,0x16e54au,0x16e554u,0x16e558u,0x16e575u,0x16e57cu,0x16e57eu,0u,0u,0u,0x16e5a2u,0x16e5a7u,0x16e5aau,0x16e5deu,
    0x16e5e1u,0x16e5e1u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::used_count)+1u);
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    bool setting{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        if(a==bitmap)return setting?0x16e5bau:0x16e583u;
        if(a==bit_load)return setting?0x16e5d1u:0x16e59au;
        if(a==bit_store)return setting?0x16e5d7u:0x16e5a0u;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case count_load:case count_again:case data:gp[eax]=v;break;
        case size:gp[ecx]=v;break;
        case mode:set_low_byte(gp[eax],v);break;
        case bitmap:gp[setting?edx:ecx]=v;break;
        case bit_load:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::fill_word){gp[edi]+=4u*step();--gp[ecx];}                                        // each STOSD step
        if(a==Access::fill_byte){gp[edi]+=step();--gp[ecx];}                                            // each STOSB step
    }
    std::uint32_t anded(std::uint32_t v,std::uint32_t mask){return this->flags.logic(v&mask&0xffu,1);}  // AND byte [ESI+2A],FD
    void saving(unsigned n,const Kept &kept) {
        auto &gp=this->gp;
        if(n==0u) {                                                                                       // PUSH EBP, EDI, 2; XOR EBP,EBP; LEA EDI; PUSH ESI, EDI
            put(0x16e514u,gp[ebp]);put(0x16e515u,gp[edi]);put(0x16e516u,logic::signature_bytes);
            gp[ebp]=this->flags.logic(kept.index,4);gp[edi]=kept.signature;
            put(0x16e51du,kept.array);put(0x16e51eu,kept.signature);
        } else put(0x16e546u,gp[ebx]);                                                                    // PUSH EBX
    }
    void signed_name(Kept &kept) {                                                                       // CALL strncpy
        auto child=[&](State &v){dispatch(v,0x32a4e0u,0x16e532u);};
        this->call(0x16e532u,0x16e537u,0x32a4e0u,child);
        refresh(kept);
    }
    std::uint32_t ored(std::uint32_t v,std::uint32_t bit){return this->flags.logic((v|bit)&0xffffu,2);} // OR word [EDI],8000
    void counted(std::uint32_t count,std::uint32_t index) {                                              // ADD ESP,C; CMP EAX,EBP
        auto &gp=this->gp;
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);depth-=0xcu;this->compare32(count,index);
    }
    std::uint32_t element(std::uint32_t size,std::uint32_t index,std::uint32_t data) {                  // MOV EDX,ECX; IMUL EDX,EBP; ADD EDX,EAX
        auto &fl=this->flags;
        return this->gp[edx]=fl.plus(fl.product(size,index),data,4);
    }
    bool tested(std::uint32_t first_word,std::uint32_t mode_value) {                                     // CMP word [EDX],0; SETNZ BL; SHR AL,3; TEST AL,1; TEST BL,BL
        auto &gp=this->gp;auto &fl=this->flags;
        fl.minus(first_word&0xffffu,0u,2);
        const bool in_use=(first_word&0xffffu)!=0u;
        set_low_byte(gp[ebx],in_use?1u:0u);
        set_low_byte(gp[eax],fl.right(mode_value&0xffu,logic::array::fill_bit,1,false));fl.logic(gp[eax]&1u,1);
        this->test8(gp[ebx]);
        setting=in_use;
        if(in_use)this->reached(Branch::used);
        return in_use;
    }
    std::uint32_t bit_word(std::uint32_t bitmap_value,std::uint32_t index,bool used_bit) {              // SAR 5; LEA; AND 1F; SHL
        auto &gp=this->gp;auto &fl=this->flags;
        const auto word=bitmap_value+4u*fl.right(index,5u,4,true);
        if(used_bit){gp[ecx]=index>>5;gp[eax]=word;}else gp[eax]=word;
        gp[ecx]=fl.logic(index&0x1fu,4);gp[edx]=fl.shift(1u,index&0x1fu,4);
        return word;
    }
    std::uint32_t bit_set(std::uint32_t bits,std::uint32_t,bool) {                                      // OR ECX,EDX; TEST BL,BL; LEA EAX,[EBP+1]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(bits|gp[edx],4);this->test8(gp[ebx]);
        return gp[ecx];
    }
    std::uint32_t incremented(std::uint32_t v){this->gp[eax]=this->gp[ebp]+1u;return this->flags.increment(v,4,false);}   // INC [ESI+3C]
    std::uint32_t filling(std::uint32_t size,std::uint32_t element) {                                    // MOV EBX,ECX; SHR ECX,2; MOV EAX,BABABABA; MOV EDI,EDX
        auto &gp=this->gp;
        gp[ebx]=size;gp[ecx]=this->flags.right(size,2u,4,false);gp[eax]=logic::fill;gp[edi]=element;
        this->reached(Branch::filled);
        return element;
    }
    void bytes(std::uint32_t size){this->gp[ecx]=this->flags.logic(size&3u,4);}                         // MOV ECX,EBX; AND ECX,3
    std::uint32_t bit_cleared(std::uint32_t bits,std::uint32_t) {                                       // NOT EDX; AND ECX,EDX
        auto &gp=this->gp;
        gp[edx]=~gp[edx];gp[ecx]=this->flags.logic(bits&gp[edx],4);
        return gp[ecx];
    }
    void lowered(std::uint32_t index,std::uint32_t first) {                                             // CMP EBP,[ESI+34]
        this->compare32(index,first);
        if(std::int32_t(index)<std::int32_t(first))this->reached(Branch::lowered);
    }
    void advanced(std::uint32_t index,std::uint32_t limit) {                                            // INC EBP; CMP EBP,EAX
        this->gp[ebp]=this->flags.increment(index-1u,4,false);this->compare32(index,limit);
    }
    void restoring(unsigned n) {
        auto &gp=this->gp;
        if(n==1u)gp[ebx]=take(0x16e5b2u);                                                                // POP EBX
        gp[edi]=take(0x16e5b3u);gp[ebp]=take(0x16e5b4u);                                                 // POP EDI; POP EBP
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::data_array_restore::restore(memory,*this,Kept{gp[ebp],gp[esi],gp[edi]},gp[eax],gp[ecx]);
        this->reached(unsigned(outcome));
        this->ret(0x16e5b5u,0u);this->publish(0x16e5b5u);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x16e510u,0x16e5e4u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_data_array_restore_detail
