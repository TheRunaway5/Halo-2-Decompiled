#pragma once
// Boundary adapter for the aim choice, 1ADF60
// (native_tu15_aim_choice_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the ray and five arguments
// on the stack (RET 14), AL the answer; MOV EAX,1808 and the stack probe
// 32A660 make the frame, with EBX, EBP and EDI (and, past the leaf, ESI)
// saved below it, read from the ESP each child left. The probe and 14CDB0
// (ECX the query, EDX the ray, EAX 0) go through the dispatch; 1AE090
// (seven pushed, RET 1C), qsort 32B560 (four pushed, the caller's ADD ESP,10)
// and 1AF3E0 (ECX, EBX and EAX) are direct; every register each returns
// comes back. The chosen candidate's copy keeps the original's REP MOVSD
// (ESI, EDI and ECX stepping with each word, in the direction flag's
// direction). Each exit leaves through the original's epilogue. Each access
// publishes its original instruction.
// HALO2_REFERENCE_AIM_CHOICE=1 restores the translated body.
// HALO2_VERIFY_AIM_CHOICE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_aim_choice_systems.hpp"

namespace halo2::native_aim_choice_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_aim_choice;
using Access=logic::aim_choice::Access;
using Exit=logic::aim_choice::Exit;
using Kept=logic::aim_choice::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_AIM_CHOICE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_AIM_CHOICE");return v;}
enum Branch : unsigned {no_leaf, no_cluster, unlisted, rejected, exhausted, chosen, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Aim choice"};return v;}
inline constexpr std::uint32_t sites[]{0x1adf6cu,0x1adf76u,0x1adf82u,0x1adf94u,0x1adf9au,0x1adf9du,0x1adfabu,0x1adfb8u,0x1adfc0u,
    0x1adfe8u,0x1ae000u,0x1ae006u,0x1ae010u,0x1ae01au,0x1ae026u,0x1ae030u,0x1ae036u,0x1ae047u,0x1ae05eu,0x1ae05eu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::copy_store)+1u);
static_assert(logic::candidate_size==3u<<5 && logic::candidate_words*4u==logic::candidate_size);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 1AE090, 32B560 and 1AF3E0
    Dispatch &dispatch;      // the stack probe 32A660 and 14CDB0
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.answer=gp[ebx];kept.base=gp[ebp];kept.count=gp[esi];kept.ray=gp[edi];}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;refresh(kept);
    }
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case first_argument:gp[ebp]=v;break;
        case query_load:case structure_load:case third_argument:case third_again:case cursor_next_load:gp[ecx]=v;break;
        case leaves_load:case fourth_argument:case second_argument:gp[edx]=v;break;
        case leaf_cluster:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;                               // MOV AX,word
        case cursor_load:case candidate_key:gp[eax]=v;break;
        case answer_load:set_low_byte(gp[eax],v);break;                                                    // MOV AL
        case chosen_argument:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {                                                  // REP MOVSD: ESI, EDI and ECX step
        auto &gp=this->gp;
        if(a==Access::copy_store){const auto d=step();gp[esi]+=d;gp[edi]+=d;--gp[ecx];}
    }
    // The frame: MOV EAX,1808; CALL 32A660 (the stack probe).
    void probed() {
        auto &gp=this->gp;
        gp[eax]=logic::frame_size;
        auto route=[&](State &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site);};
        this->call_routed(route,0x1adf65u,0x1adf6au,0x32a660u);
        depth=0u-16u;
    }
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / EDI / ESI
        static constexpr std::uint32_t pcs[]{0x1adf6au,0x1adf6bu,0x1adf73u,0x1adfb2u};
        static constexpr unsigned regs[]{ebx,ebp,edi,esi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    // The leaf and the cluster.
    std::uint32_t ray(){return this->gp[edi]=this->gp[ecx];}                                                  // MOV EDI,ECX
    std::uint32_t unanswered(){set_low_byte(this->gp[ebx],this->flags.logic(0u,1));return this->gp[ebx];}    // XOR BL,BL
    void leaf_arguments(std::uint32_t ray_value){auto &gp=this->gp;gp[edx]=ray_value;gp[eax]=this->flags.logic(0u,4);}   // MOV EDX,EDI; XOR EAX,EAX
    std::uint32_t leaf(Kept &kept,std::uint32_t) {                                                       // CALL 14CDB0
        auto route=[&](State &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site);};
        this->call_routed(route,0x1adf86u,0x1adf8bu,0x14cdb0u);refresh(kept);
        return this->gp[eax];
    }
    bool named(std::uint32_t v) {                                                                        // CMP EAX,-1
        this->compare32(v,logic::none);
        if(v==logic::none)this->reached(Branch::no_leaf);
        return v!=logic::none;
    }
    bool cluster_named(std::uint32_t v) {                                                                // CMP AX,FFFF
        this->flags.minus(v&0xffffu,logic::no_cluster,2);
        if((v&0xffffu)==logic::no_cluster)this->reached(Branch::no_cluster);
        return (v&0xffffu)!=logic::no_cluster;
    }
    // The search and the sort.
    void list_pushed(){auto &gp=this->gp;gp[ecx]=local(logic::local::list);put(0x1adfb7u,gp[ecx]);}         // LEA ECX,[ESP+18]; PUSH ECX
    void fourth_pushed(std::uint32_t fourth){put(0x1adfbfu,fourth);}                                         // PUSH EDX
    std::uint32_t searched(Kept &kept,std::uint32_t third,std::uint32_t second) {                       // PUSH ECX, EDX, EDI, EAX, EBP; CALL 1AE090; MOV ESI,EAX
        auto &gp=this->gp;
        put(0x1adfc7u,third);put(0x1adfc8u,second);put(0x1adfc9u,kept.ray);put(0x1adfcau,gp[eax]);put(0x1adfcbu,kept.base);
        direct(0x1adfccu,0x1ae090u,0x1cu,kept);
        return gp[esi]=gp[eax];
    }
    bool positive(std::uint32_t count) {                                                                 // TEST ESI,ESI; JLE
        this->test32(count);
        if(std::int32_t(count)<=0)this->reached(Branch::unlisted);
        return std::int32_t(count)>0;
    }
    void sort_pushed(std::uint32_t count) {                                                              // PUSH 1AF2E0; PUSH 60; LEA EAX,[ESP+20]; PUSH ESI; PUSH EAX
        auto &gp=this->gp;
        put(0x1adfdbu,logic::comparator);put(0x1adfe0u,logic::candidate_size);
        gp[eax]=local(logic::local::list);put(0x1adfe6u,count);put(0x1adfe7u,gp[eax]);
    }
    void sorted(Kept &kept) {                                                                            // CALL 32B560; ADD ESP,10
        auto &gp=this->gp;
        direct(0x1adfeeu,0x32b560u,0u,kept);
        gp[esp]=this->flags.plus(gp[esp],0x10u,4);depth-=0x10u;
    }
    std::uint32_t first_candidate(){return this->gp[ebp]=this->flags.logic(0u,4);}                           // XOR EBP,EBP
    // The candidates.
    std::uint32_t cursor_start(){return this->gp[eax]=local(logic::local::list+logic::cursor_offset);}       // LEA EAX,[ESP+2C]
    std::uint32_t candidate_part(std::uint32_t address){return this->gp[ebx]=address;}                       // LEA EBX,[EAX-C]
    bool accepted(Kept &kept,std::uint32_t,std::uint32_t) {                                              // CALL 1AF3E0; TEST AL,AL
        direct(0x1ae01du,0x1af3e0u,0u,kept);
        this->test8(this->gp[eax]);
        const bool yes=low_byte(this->gp[eax])!=0u;
        this->reached(yes?Branch::chosen:Branch::rejected);
        return yes;
    }
    std::uint32_t copying(std::uint32_t index,std::uint32_t) {                                           // LEA ESI,[EBP+EBP*2]; SHL ESI,5; LEA ESI,[ESP+ESI+18]; MOV ECX,18
        auto &gp=this->gp;
        gp[esi]=this->flags.shift(3u*index,5u,4);gp[esi]=local(logic::local::list)+gp[esi];gp[ecx]=logic::candidate_words;
        return gp[esi];
    }
    std::uint32_t next_candidate(std::uint32_t index){return this->gp[ebp]=this->flags.increment(index,4,false);}   // INC EBP
    std::uint32_t next_cursor(std::uint32_t cursor){return this->gp[ecx]=this->flags.plus(cursor,logic::candidate_size,4);}   // ADD ECX,60
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP EBP,ESI; JL
    void finish(Exit exit) {
        auto &gp=this->gp;
        std::uint32_t pc;
        switch(exit) {
        case Exit::early:                                                                                // POP EDI; POP EBP; MOV AL,BL; POP EBX
            gp[edi]=this->pop(0x1ae07eu);gp[ebp]=this->pop(0x1ae07fu);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=this->pop(0x1ae082u);
            pc=0x1ae083u;break;
        case Exit::unlisted:                                                                             // POP ESI; POP EDI; POP EBP; MOV AL,BL; POP EBX
            gp[esi]=this->pop(0x1ae06fu);gp[edi]=this->pop(0x1ae070u);gp[ebp]=this->pop(0x1ae071u);set_low_byte(gp[eax],gp[ebx]);
            gp[ebx]=this->pop(0x1ae074u);pc=0x1ae075u;break;
        case Exit::exhausted:                                                                            // POP ESI; POP EDI; POP EBP; POP EBX
            this->reached(Branch::exhausted);
            gp[esi]=this->pop(0x1ae03au);gp[edi]=this->pop(0x1ae03bu);gp[ebp]=this->pop(0x1ae03cu);gp[ebx]=this->pop(0x1ae03du);
            pc=0x1ae03eu;break;
        default:                                                                                         // POP ESI; POP EDI; POP EBP; MOV AL,1; POP EBX
            gp[esi]=this->pop(0x1ae060u);gp[edi]=this->pop(0x1ae061u);gp[ebp]=this->pop(0x1ae062u);set_low_byte(gp[eax],1u);
            gp[ebx]=this->pop(0x1ae065u);pc=0x1ae066u;break;
        }
        gp[esp]=this->flags.plus(gp[esp],logic::frame_size,4);                                           // ADD ESP,1808; RET 14
        this->ret(pc+6u,0x14u);this->publish(pc+6u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::aim_choice::choose(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x1adf60u,0x1ae089u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_aim_choice_detail
