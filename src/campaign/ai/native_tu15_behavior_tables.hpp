#pragma once
// Boundary adapter for the behavior tables, 1AF970
// (native_tu15_behavior_tables_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: no arguments (RET); EBP saved and made the
// frame pointer, ESP eight-byte aligned under a 0x2F0-byte frame, EBX, EBP,
// ESI and EDI saved below it; 1AFA90 (the slots, the entry, the defaults and
// their count pushed, RET 0x10) direct, every register it returns coming
// back; the entries cleared and the slots copied as REP STOSD, MOVSD and
// MOVSB do, in the string direction flag's direction. Each access publishes
// its original instruction.
// HALO2_REFERENCE_BEHAVIOR_TABLES=1 restores the translated body.
// HALO2_VERIFY_BEHAVIOR_TABLES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "campaign/ai/native_tu15_behavior_tables_systems.hpp"

namespace halo2::native_behavior_tables_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_behavior_tables;
using Access=logic::behavior_tables::Access;
using Kept=logic::behavior_tables::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BEHAVIOR_TABLES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BEHAVIOR_TABLES");return v;}
// Backward string operations (the direction flag set) clear the frame's own cursors before any slot is copied,
// so the copy only ever runs forward.
enum Branch : unsigned {built, filled, unfilled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Behavior tables"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1af98au,0x1af992u,0x1af996u,0x1af99au,0x1af9b0u,0x1af9b2u,0x1af9bbu,0x1af9c0u,0x1af9c3u,0x1af9d2u,
    0x1af9e0u,0x1af9eau,0x1af9f1u,0x1af9f6u,0x1af9fau,0x1afa11u,0x1afa16u,0x1afa1au,0u,0u,0x1afa40u,0x1afa40u,0x1afa46u,0x1afa4cu,0x1afa4fu,
    0x1afa5au,0x1afa60u,0x1afa64u,0x1afa68u,0x1afa73u,0x1afa77u,0x1afa7bu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::categories_again)+1u);
template<class State,class Fill> struct Adapter : Port<State> {
    Fill &fill_child;
    std::uint32_t depth{};      // bytes pushed below the frame's base
    bool copying_bytes{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Fill &f):Port<State>(s,c,t,{}),fill_child(f){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::copy_read || a==Access::copy_write)return copying_bytes?0x1afa3eu:0x1afa37u;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case category_count:set_low_word(gp[ecx],v);break;
        case category_entries:gp[eax]=v;gp[ecx]&=0xffffu;break;                                         // MOV EAX,[EDX]; MOVZX ECX,CX
        case descriptor:case entry_load:case behaviors_load:case categories_load:gp[eax]=v;break;
        case defaults:case category_load:gp[edx]=v;break;
        case default_count:gp[ecx]=this->flags.logic(0u,4);gp[ecx]=v&0xffffu;break;                    // XOR ECX,ECX; MOV CX
        case record_load:gp[ebx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::entries_clear){gp[edi]+=4u*step();--gp[ecx];}                                    // each STOSD step
        if(a==Access::copy_write) {                                                                      // each MOVSD (MOVSB) step
            const auto delta=copying_bytes?step():4u*step();
            gp[esi]+=delta;gp[edi]+=delta;--gp[ecx];
        }
    }
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-8; SUB ESP,2F0; PUSH EBX, EBP, ESI; MOV EBX; MOV EDX; PUSH EDI
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x1af970u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff8u,4),0x2f0u,4);
        this->push(0x1af97cu,gp[ebx]);this->push(0x1af97du,gp[ebp]);this->push(0x1af97eu,gp[esi]);
        gp[ebx]=logic::globals::records;gp[edx]=logic::globals::categories;
        this->push(0x1af989u,gp[edi]);
    }
    std::uint32_t clearing(std::uint32_t entries) {                                                    // XOR EAX,EAX; MOV ECX,83; LEA EDI; REP STOSD
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::behavior_count;gp[edi]=entries;
        return entries;
    }
    void counted(std::uint32_t count){this->flags.logic(count&0xffffu,2);}                             // TEST CX,CX
    void indexed(std::uint32_t index){this->gp[edx]=index;}                                            // MOVSX EDX,word [EAX]
    void listed(std::uint32_t entry,std::uint32_t left) {                                              // ADD EAX,C; DEC ECX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.plus(entry-logic::entry_size,logic::entry_size,4);gp[ecx]=fl.increment(left+1u,4,true);
    }
    void behavior(const Kept &){this->gp[ebp]=this->flags.logic(0u,4);}                                // XOR EBP,EBP
    void described(std::uint32_t descriptor_address){this->test32(descriptor_address);}                // TEST EAX,EAX
    void kinded(std::uint32_t kind){this->flags.minus(kind&0xffffu,logic::fill_kind,2);}               // CMP word [EAX+2],1
    std::uint32_t fill(std::uint32_t count,std::uint32_t defaults_address,std::uint32_t entry,Kept &kept) {   // PUSH ECX, EDX, EAX; LEA ECX; PUSH ECX; CALL 1AFA90
        auto &gp=this->gp;
        put(0x1afa01u,count);put(0x1afa02u,defaults_address);put(0x1afa03u,entry);
        gp[ecx]=local(logic::local::slots);put(0x1afa08u,gp[ecx]);
        this->call(0x1afa09u,0x1afa0eu,0x1afa90u,fill_child);depth-=16u;
        refresh(kept);
        this->flags.logic(gp[eax]&0xffffu,2);                                                            // TEST AX,AX
        this->reached(std::int16_t(gp[eax]&0xffffu)>0?Branch::filled:Branch::unfilled);
        return gp[eax];
    }
    void started(std::uint32_t start){set_low_word(this->gp[edx],start);}                             // MOV DX,[ESP+14]
    std::uint32_t copying(std::uint32_t count,std::uint32_t start,std::uint32_t from) {                // MOVSX ECX,AX; SHL ECX,2; MOVSX EDI,DX; MOV EDX,ECX; SHR ECX,2; LEA EDI; LEA ESI
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.shift(count,2u,4);gp[ecx]=fl.right(gp[edx],2u,4,false);
        gp[edi]=logic::globals::slot_pool+4u*start;gp[esi]=from;copying_bytes=false;
        return gp[edx];
    }
    void bytes(std::uint32_t total){this->gp[ecx]=this->flags.logic(total&3u,4);copying_bytes=true;} // MOV ECX,EDX; AND ECX,3
    std::uint32_t added(std::uint32_t index,std::uint32_t filled_count){return this->flags.plus(index,filled_count,4);}   // ADD [ESP+14],EAX
    void advanced(const Kept &kept,std::uint32_t left) {                                               // ADD EBP,4; ADD EBX,50; DEC EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebp]=fl.plus(kept.offset-4u,4u,4);gp[ebx]=fl.plus(kept.record-logic::record_size,logic::record_size,4);
        gp[eax]=fl.increment(left+1u,4,true);
    }
    void next_category(std::uint32_t category_address,std::uint32_t record_address,std::uint32_t left) {   // ADD EDX,8; ADD EBX,4; DEC EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.plus(category_address-logic::category_size,logic::category_size,4);
        gp[ebx]=fl.plus(record_address-logic::record_step,logic::record_step,4);
        gp[eax]=fl.increment(left+1u,4,true);
    }
    void restoring() {                                                                                   // POP EDI, ESI, EBP, EBX; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[edi]=this->pop(0x1afa85u);gp[esi]=this->pop(0x1afa86u);gp[ebp]=this->pop(0x1afa87u);gp[ebx]=this->pop(0x1afa88u);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x1afa8bu);
    }
    void run() {
        Memory<Adapter> memory{*this};
        logic::behavior_tables::build(memory,*this);
        this->reached(Branch::built);
        this->ret(0x1afa8cu,0u);this->publish(0x1afa8cu);
    }
};
template<class State,class Reference,class Fill> bool bridge(State &s,Reference reference,Fill fill) {
    return run_owner(s,0x1af970u,0x1afa8cu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Fill>(v,c,t,fill);});
}
} // namespace halo2::native_behavior_tables_detail
