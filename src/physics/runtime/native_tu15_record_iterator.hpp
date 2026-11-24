#pragma once
// Boundary adapter for the keyed record iterator, entered at 1E5250
// (native_tu15_record_iterator_systems.hpp). Its callers and the child
// 1E5810 keep the original ABI, which the adapter owns:
//   - EBX the owner, ESI the iterator; AL the result; PUSH ECX (the index
//     local), PUSH EBP, PUSH EDI; the local is addressed from the live ESP,
//     so a child that leaves the stack unbalanced moves it as it does for
//     the original;
//   - the stage jump table (1E5350) is read and mapped through the unit's
//     canonical branch map, as the original's JMP [1E5350+EAX*4];
//   - 1E5810 gets the pushed table, key and 0x14, with EAX/ECX/EDX as each
//     case leaves them and EDI cleared; every register it returns comes
//     back (EBP the stage, EDI the found block);
//   - the exit: AL 1 with the record copied, or AL 0; EDI/EBP/ECX restored;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_RECORD_ITERATOR=1 restores the translated body.
// HALO2_VERIFY_RECORD_ITERATOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_record_iterator_systems.hpp"

namespace halo2::native_record_iterator_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_iterator;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_ITERATOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_ITERATOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    exhausted, stage_open, past_stages, stage_both, stage_first, stage_second, stage_any, first_missing, first_found,
    stage_more, stage_last, record, next_stage, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record iterator verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

inline constexpr Edge iterator_edges[]{
    {0x1e5262u,0x1e5347u,exhausted},{0x1e5262u,0x1e5278u,stage_open},
    {0x1e5278u,0x1e52ffu,past_stages},
    {0x1e5284u,0x1e528bu,stage_both},{0x1e5284u,0x1e52a8u,stage_first},{0x1e5284u,0x1e52b4u,stage_second},
    {0x1e5284u,0x1e52d2u,stage_any},
    {0x1e5297u,0x1e52ffu,first_missing},{0x1e52c1u,0x1e52ffu,first_missing},{0x1e52dfu,0x1e52ffu,first_missing},
    {0x1e5297u,0x1e52a0u,first_found},{0x1e52c1u,0x1e52cau,first_found},{0x1e52dfu,0x1e52e8u,first_found},
    {0x1e530bu,0x1e5278u,stage_more},{0x1e530bu,0x1e5347u,stage_last},
    {0x1e531eu,0x1e532cu,record},{0x1e531eu,0x1e5323u,next_stage},
};
template<class State,class Branch_,class Search> struct Iteration : Port<State> {
    using Access=logic::record_iterator::Access;
    using Local=logic::record_iterator::Local;
    using Kept=logic::record_iterator::Kept;
    Branch_ &branch;Search &search_child;
    // E: where the return address sits; the walk's ESP is E-C.
    std::uint32_t frame{};
    unsigned current_case{};
    Iteration(State &s,std::uint32_t *c,Trace<State> *t,Branch_ &b,Search &x)
        :Port<State>(s,c,t,iterator_edges),branch(b),search_child(x){}
    std::uint32_t local(Local)const{return frame-4u;}
    Kept entry()const{return {this->gp[ebx],this->gp[esi],this->gp[ebp],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_read:this->push(0x1e5250u,gp[ecx]);return 0x1e5251u;
        case index_store:this->push(0x1e5256u,gp[ebp]);return 0x1e5257u;
        case stage_check:gp[edi]=this->flags.logic(0,4);return 0x1e5262u;                      // XOR EDI,EDI
        case stage_read:return 0x1e5278u;
        case stage_jump:return 0x1e5284u;
        case first_key:return current_case==0?0x1e528bu:0x1e52a8u;
        case second_key:return current_case==0?0x1e52a0u:0x1e52cau;
        case stage_store:gp[ebp]=this->flags.increment(gp[ebp],4,false);return 0x1e52ffu;          // INC EBP
        case index_clear:return 0x1e5303u;
        case stage_bound:return 0x1e530bu;
        case index_reload:return 0x1e531au;
        case count:return 0x1e531eu;
        case records:return 0x1e532cu;
        case record_first:return 0x1e532fu;
        case result_first:return 0x1e5335u;
        case record_second:gp[eax]=gp[eax]+gp[ecx]*8u;return 0x1e5337u;                         // LEA EAX,[EAX+ECX*8]
        case result_second:return 0x1e533bu;
        case result_index:return 0x1e533eu;
        case stage_advance_read:case stage_advance:return 0x1e5323u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_read:gp[eax]=this->flags.increment(logic::widen16(v),4,false);break;          // MOVSX; INC
        case stage_check:case stage_bound:this->flags.minus(v,4u,2);break;                       // CMP WORD,4
        // MOV BP,[ESI+A]; MOVSX EAX,BP; CMP EAX,3.
        case stage_read:
            gp[ebp]=(gp[ebp]&0xffff0000u)|(v&0xffffu);gp[eax]=logic::widen16(v);this->compare32(gp[eax],3u);break;
        case first_key:gp[eax]=v;break;
        case second_key:(current_case==0?gp[edx]:gp[ecx])=v;break;
        case index_reload:gp[ecx]=v;break;
        case count:this->compare32(gp[ecx],v);break;
        case records:gp[eax]=v;break;
        case record_first:gp[edx]=v;break;
        case record_second:gp[eax]=v;gp[edi]=this->pop(0x1e533au);break;                       // POP EDI
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::index_store)this->push(0x1e525bu,this->gp[edi]);                         // PUSH EDI
    }
    // JMP [1E5350+EAX*4] through the canonical branch map.
    unsigned stage_case(std::uint32_t target) {
        switch(branch(target,this->s.image)) {
        case 0x1e528bu:current_case=0;break;
        case 0x1e52a8u:current_case=1;break;
        case 0x1e52b4u:current_case=2;break;
        case 0x1e52d2u:current_case=3;break;
        default:throw std::runtime_error("Required original solver dispatch target unavailable");
        }
        return current_case;
    }
    // The pushes, the call and TEST EAX,EAX (the first level) or MOV EDI,EAX
    // (the second).
    std::uint32_t search(Kept &kept,unsigned stage,unsigned level,std::uint32_t key) {
        auto &gp=this->gp;
        static constexpr std::uint32_t first_push[]{0x1e528eu,0x1e52abu,0x1e52b4u,0x1e52d2u};
        static constexpr std::uint32_t first_call[]{0x1e5297u,0x1e52dfu,0x1e52c1u,0x1e52dfu};
        static constexpr std::uint32_t second_push[]{0x1e52a3u,0x1e52e8u,0x1e52cdu,0x1e52e8u};
        if(level==0) {
            const auto p=first_push[stage];
            this->push(p,0x14u);this->push(stage<2?p+2u:p+2u,key);
            const auto table=gp[ebx]+logic::owner_table;
            (stage==0||stage==1?gp[ecx]:gp[edx])=table;
            this->push(stage==0?0x1e5294u:stage==1?0x1e52b1u:stage==2?0x1e52beu:0x1e52dcu,table);
            gp[edi]=this->flags.logic(0,4);                                                    // XOR EDI,EDI
            const auto call=first_call[stage];
            this->call(call,call+5u,0x1e5810u,search_child);
            this->test32(gp[eax]);
        } else {
            const auto p=second_push[stage];
            this->push(p,0x14u);this->push(p+2u,key);
            gp[eax]=this->flags.plus(gp[eax],logic::block_table,4);                                // ADD EAX,4
            this->push(0x1e52f2u,gp[eax]);
            this->call(0x1e52f3u,0x1e52f8u,0x1e5810u,search_child);
            gp[edi]=gp[eax];
        }
        frame=gp[esp]+0xcu;
        kept={gp[ebx],gp[esi],gp[ebp],gp[edi]};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Iteration> memory{*this};
        if(logic::record_iterator::next(memory,*this)) {
            gp[eax]=(gp[eax]&0xffffff00u)|1u;
            gp[ebp]=this->pop(0x1e5344u);gp[ecx]=this->pop(0x1e5345u);
            this->ret(0x1e5346u,0);this->publish(0x1e5346u);
        } else {
            gp[edi]=this->pop(0x1e5347u);
            gp[eax]=(gp[eax]&0xffffff00u)|this->flags.logic(0,1);                                 // XOR AL,AL
            gp[ebp]=this->pop(0x1e534au);gp[ecx]=this->pop(0x1e534bu);
            this->ret(0x1e534cu,0);this->publish(0x1e534cu);
        }
    }
};

template<class State,class Reference,class Branch_,class Search>
bool bridge(State &s,Reference reference,Branch_ branch,Search search) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5250u,0x1e534cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Iteration<State,Branch_,Search>(v,c,t,branch,search);});
    return true;
}
} // namespace halo2::native_record_iterator_detail
