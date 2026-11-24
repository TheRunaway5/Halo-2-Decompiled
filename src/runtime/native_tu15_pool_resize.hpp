#pragma once
// Boundary adapter for the pool resize, 191130
// (native_tu15_pool_resize_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the size, EBX the handle, the
// pool, label and line pushed (RET 0xC), the answer in AL; EBP, ESI and EDI
// saved; 33B590 (the time in EAX), 1910A0 (ECX the pool, EAX the line; the
// label's slot, the size and the label pushed, RET 0xC) and 191220 (EAX the
// handle, EDX the pool) direct, every register each returns coming back; the
// payload copied as REP MOVSD and REP MOVSB copy it, in the string direction
// flag's direction. Each access publishes its original instruction.
// HALO2_REFERENCE_POOL_RESIZE=1 restores the translated body.
// HALO2_VERIFY_POOL_RESIZE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "runtime/native_tu15_pool_resize_systems.hpp"

namespace halo2::native_pool_resize_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_pool_resize;
using Access=logic::pool_resize::Access;
using Kept=logic::pool_resize::Kept;
using Outcome=logic::pool_resize::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POOL_RESIZE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POOL_RESIZE");return v;}
enum Branch : unsigned {in_place, moved, failed, guarded, rounded, backward, branch_count};
static_assert(unsigned(Outcome::failed)==Branch::failed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Pool resize"};return v;}
inline constexpr std::uint32_t sites[]{0x191131u,0x191136u,0x19113bu,0x19115au,0x191161u,0x191164u,0x19116eu,0x191170u,0x191177u,0x19117au,
    0x19117cu,0x191188u,0x19118cu,0x191190u,0x191193u,0x19119bu,0x1911a6u,0x1911b1u,0x1911c0u,0x1911d1u,0x1911d3u,0x1911d5u,0u,0u,
    0x1911f2u,0x1911f9u,0x191206u,0x191209u,0x19120du};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::handle_store)+1u);
template<class State,class Clock,class Allocate,class Release> struct Adapter : Port<State> {
    Clock &clock_child;
    Allocate &allocate_child;
    Release &release_child;
    std::uint32_t depth{};      // bytes below the return address
    bool copying_bytes{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Clock &k,Allocate &a,Release &r)
        :Port<State>(s,c,t,{}),clock_child(k),allocate_child(a),release_child(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::copy_read || a==Access::copy_write)return copying_bytes?0x1911ebu:0x1911e0u;
        return sites[unsigned(a)];
    }
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case pool_argument:gp[ebp]=v;break;
        case data:case old_data:gp[esi]=v;break;
        case guarded:case guarded_again:set_low_byte(gp[eax],v);break;
        case next:case pool_size:case block_size:case move_label:case new_data_final:gp[edx]=v;break;
        case pool_free:case new_data:gp[edi]=v;break;
        case label:case move_line:gp[eax]=v;break;
        case line:case old_size:case new_data_again:gp[ecx]=v;break;
        case guarded_moved:case unowned:set_low_byte(gp[ecx],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::copy_write) {                                                                     // each MOVSD (MOVSB) step
            const auto delta=copying_bytes?step():4u*step();
            gp[esi]+=delta;gp[edi]+=delta;--gp[ecx];
        }
    }
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u)put(0x191130u,gp[ebp]);                                                                // PUSH EBP
        else if(n==1u)put(0x191135u,gp[esi]);                                                           // PUSH ESI
        else{put(0x191138u,gp[edi]);gp[edi]=gp[eax];}                                                   // PUSH EDI; MOV EDI,EAX
    }
    std::uint32_t headed(std::uint32_t data_address,std::uint32_t is_guarded) {                        // SUB ESI,10; TEST AL,AL; MOV ECX,10 (20)
        auto &gp=this->gp;
        gp[esi]=this->flags.minus(data_address,logic::header_size,4);this->test8(is_guarded);
        gp[ecx]=logic::header_size+((is_guarded&0xffu)?logic::guard_size:0u);
        if(is_guarded&0xffu)this->reached(Branch::guarded);
        return gp[ecx];
    }
    std::uint32_t sized(std::uint32_t overhead,std::uint32_t size) {                                   // ADD ECX,EDI; TEST CL,F; (OR ECX,F; INC ECX)
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.plus(overhead,size,4);fl.logic(gp[ecx]&(logic::alignment-1u),1);
        if(gp[ecx]&(logic::alignment-1u)){gp[ecx]=fl.increment(fl.logic(gp[ecx]|(logic::alignment-1u),4),4,false);this->reached(Branch::rounded);}
        return gp[ecx];
    }
    void limit_tested(std::uint32_t limit){this->test32(limit);}                                       // TEST EDX,EDX
    void pool_end(std::uint32_t bytes,std::uint32_t base){this->gp[edx]=this->flags.plus(bytes,base,4);}   // ADD EDX,[EBP+28]
    void fitted(std::uint32_t needed,std::uint32_t header,std::uint32_t limit) {                       // LEA EAX,[ECX+ESI]; CMP EAX,EDX
        auto &gp=this->gp;
        gp[eax]=needed+header;this->compare32(gp[eax],limit);
    }
    std::uint32_t freed(std::uint32_t old,std::uint32_t needed,std::uint32_t free_bytes) {             // SUB EDX,ECX; ADD EDI,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.minus(old,needed,4);gp[edi]=fl.plus(free_bytes,gp[edx],4);
        return gp[edi];
    }
    void guarded_tested(std::uint32_t flag_value){this->test8(flag_value);}                           // TEST AL,AL
    std::uint32_t guard_tested(std::uint32_t header){return this->gp[esi]=this->flags.plus(header,0xfffffff0u,4);}   // ADD ESI,-10
    std::uint32_t clock(Kept &kept) {                                                                  // CALL 33B590
        this->call(0x191196u,0x19119bu,0x33b590u,clock_child);
        refresh(kept);
        return this->gp[eax];
    }
    void pushing(std::uint32_t size,std::uint32_t label_value) {                                       // PUSH EDX; PUSH EDI; LEA EAX; PUSH EAX
        auto &gp=this->gp;
        put(0x1911aau,label_value);put(0x1911abu,size);
        gp[eax]=argument(logic::argument::label);put(0x1911b0u,gp[eax]);
    }
    std::uint32_t allocate(std::uint32_t,Kept &kept) {                                                 // MOV ECX,EBP; CALL 1910A0; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.pool;
        this->call(0x1911b7u,0x1911bcu,0x1910a0u,allocate_child);depth-=12u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    std::uint32_t overhead(std::uint32_t is_guarded) {                                                 // TEST CL,CL; MOV EAX,10 (20)
        auto &gp=this->gp;
        this->test8(is_guarded);
        return gp[eax]=logic::header_size+((is_guarded&0xffu)?logic::guard_size:0u);
    }
    std::uint32_t moving(std::uint32_t old,std::uint32_t overhead_value,std::uint32_t,std::uint32_t) { // SUB ECX,EAX; MOV EDX,ECX; SHR ECX,2
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.minus(old,overhead_value,4);gp[ecx]=fl.right(gp[edx],2u,4,false);
        if(step()!=1u)this->reached(Branch::backward);
        return gp[edx];
    }
    void moved_word(std::uint32_t){}
    void bytes(std::uint32_t payload,const Kept &kept) {                                               // MOV ECX,EDX; AND ECX,3; MOV EAX,EBX; MOV EDX,EBP
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(payload&3u,4);gp[eax]=kept.handle;gp[edx]=kept.pool;copying_bytes=true;
    }
    void moved_byte(std::uint32_t){}
    void release(Kept &kept) {                                                                         // CALL 191220
        auto &gp=this->gp;
        gp[eax]=kept.handle;gp[edx]=kept.pool;
        this->call(0x1911edu,0x1911f2u,0x191220u,release_child);
        refresh(kept);
    }
    void owners_tested(std::uint32_t keeps_none){this->gp[eax]=this->flags.logic(0u,4);this->test8(keeps_none);}   // XOR EAX,EAX; TEST CL,CL
    std::uint32_t owned(std::uint32_t keeps_none,const Kept &kept) {                                   // SETNZ AL; POP EDI, ESI, EBP; DEC EAX; AND EAX,EBX
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[eax],(keeps_none&0xffu)?1u:0u);
        gp[edi]=take(0x191200u);gp[esi]=take(0x191201u);gp[ebp]=take(0x191202u);
        gp[eax]=fl.logic(fl.increment(gp[eax],4,true)&kept.handle,4);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::pool_resize::resize(memory,*this,gp[eax],Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        std::uint32_t at=0x191211u;
        if(outcome==Outcome::in_place) {                                                                // POP EDI; POP ESI; MOV AL,1; POP EBP
            gp[edi]=take(0x19119eu);gp[esi]=take(0x19119fu);set_low_byte(gp[eax],1u);gp[ebp]=take(0x1911a2u);at=0x1911a3u;
        } else if(outcome==Outcome::failed) {                                                           // POP EDI; POP ESI; XOR AL,AL; POP EBP
            gp[edi]=take(0x191214u);gp[esi]=take(0x191215u);set_low_byte(gp[eax],fl.logic(0u,1));gp[ebp]=take(0x191218u);at=0x191219u;
        } else set_low_byte(gp[eax],1u);                                                                 // MOV AL,1
        this->ret(at,0xcu);this->publish(at);
    }
};
template<class State,class Reference,class Clock,class Allocate,class Release>
bool bridge(State &s,Reference reference,Clock clock,Allocate allocate,Release release) {
    return run_owner(s,0x191130u,0x191219u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Clock,Allocate,Release>(v,c,t,clock,allocate,release);});
}
} // namespace halo2::native_pool_resize_detail
