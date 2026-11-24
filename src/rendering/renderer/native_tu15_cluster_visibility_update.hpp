#pragma once
// Boundary adapter for the cluster visibility update, 13B6F0
// (native_tu15_cluster_visibility_update_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: no arguments
// (RET); 0xC0 bytes of frame with EBX, EBP, ESI and EDI saved below them,
// read from the ESP each child left; 13B610 (ESI the set, 2 pushed, RET 8),
// 1D1E00 (EBX the buffer, the count pushed, RET 4), 140EC0 (EAX the row,
// EDX the set, the target pushed, RET 4) and 13B590 (AX the cluster)
// direct, each listener's callback (3 pushed, RET C) through the unit's
// dispatch, every register each returns coming back. The copies are REP
// MOVSD/MOVSB and the comparisons REPE CMPSD in either direction. Each
// access publishes its original instruction.
// HALO2_REFERENCE_CLUSTER_VISIBILITY_UPDATE=1 restores the translated body.
// HALO2_VERIFY_CLUSTER_VISIBILITY_UPDATE=1 records the native run and
// replays the unmodified translated body against it, as in
// native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_cluster_visibility_update_systems.hpp"

namespace halo2::native_cluster_visibility_update_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_cluster_visibility_update;
using Access=logic::cluster_visibility_update::Access;
using Kept=logic::cluster_visibility_update::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CLUSTER_VISIBILITY_UPDATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CLUSTER_VISIBILITY_UPDATE");return v;}
enum Branch : unsigned {found, copied, cluster, seen_changed, reachable_changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Cluster visibility update"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x13b6f0u,0x13b6feu,0x13b717u,0x13b717u,0x13b72au,0x13b72au,0x13b735u,0x13b74cu,0x13b762u,0x13b76au,
    0x13b78bu,0x13b7adu,0x13b7adu,0x13b7b4u,0x13b7b4u,0x13b7b6u,0x13b7c7u,0x13b7d9u,0x13b7f8u,0x13b7f8u,0u,0u,0u,0x13b830u,0x13b847u,
    0x13b847u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::reachable_before)+1u);
template<class State,class Gather,class Find,class Add,class Cluster,class Dispatch> struct Adapter : Port<State> {
    Gather &gather;Find &find;Add &add;Cluster &cluster_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers
    bool reachable_phase{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Gather &g,Find &f,Add &a,Cluster &cl,Dispatch &d)
        :Port<State>(s,c,t,{}),gather(g),find(f),add(a),cluster_child(cl),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case listener_load:return reachable_phase?0x13b850u:0x13b800u;
        case listener_count:return reachable_phase?0x13b85au:0x13b80au;
        case listener_game:return reachable_phase?0x13b860u:0x13b810u;
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case game_load:case listener_game:gp[edx]=v;break;
        case structure_load:gp[ebp]=v;break;
        case game_again:case game_found:case game_after:gp[esi]=v;break;
        case count_word:set_low_word(gp[eax],v);break;
        case count_load:case count_again:case listener_count:gp[ecx]=v;break;
        case game_copied:gp[ebx]=v;break;
        case bits_load:case listener_load:gp[eax]=v;break;
        case game_reachable:gp[edi]=v;break;
        case seen_before:case reachable_before:gp[esi]+=4u*step();gp[edi]+=4u*step();--gp[ecx];break;   // each CMPSD step
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        if(a==seen_copy_store||a==reachable_copy_store||a==copy_word_store){gp[esi]+=4u*step();gp[edi]+=4u*step();--gp[ecx];}   // MOVSD
        if(a==copy_byte_store){gp[esi]+=step();gp[edi]+=step();--gp[ecx];}                                 // MOVSB
    }
    void framing() {                                                                                     // SUB ESP,C0; PUSH EBX; PUSH EBP
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);this->push(0x13b6fcu,gp[ebx]);this->push(0x13b6fdu,gp[ebp]);
    }
    void saving(){auto &gp=this->gp;this->push(0x13b704u,gp[esi]);this->push(0x13b705u,gp[edi]);depth=0u;}   // PUSH ESI; PUSH EDI
    std::uint32_t seen_copying(std::uint32_t game_address) {                                            // LEA EAX,[EDX+1138]; MOV ESI,EAX; MOV ECX,10; LEA EDI,[ESP+50]
        auto &gp=this->gp;
        gp[eax]=gp[esi]=game_address+logic::game::seen;gp[ecx]=logic::set_words;gp[edi]=local(logic::local::seen_before);
        return gp[esi];
    }
    void reachable_copying(std::uint32_t from,std::uint32_t to) {                                       // LEA ESI; MOV ECX,10; LEA EDI,[ESP+10]; PUSH 0
        auto &gp=this->gp;
        gp[esi]=from;gp[ecx]=logic::set_words;gp[edi]=to;put(0x13b728u,0u);
    }
    void seen_gathered(std::uint32_t set,Kept &kept) {                                                  // PUSH 0; MOV ESI,EAX; CALL 13B610
        put(0x13b72cu,0u);this->gp[esi]=set;
        this->call(0x13b730u,0x13b735u,0x13b610u,gather);depth-=8u;
        refresh(kept);
    }
    void local_gathered(std::uint32_t game_now,Kept &kept) {                                            // PUSH 0; PUSH 1; ADD ESI,1178; CALL 13B610
        put(0x13b73bu,0u);put(0x13b73du,1u);this->gp[esi]=this->flags.plus(game_now,logic::game::local,4);
        this->call(0x13b745u,0x13b74au,0x13b610u,gather);depth-=8u;
        refresh(kept);
    }
    void count_cleared(){this->gp[eax]=this->flags.logic(0u,4);}                                        // XOR EAX,EAX
    bool clusters_found(std::uint32_t,Kept &kept) {                                                      // LEA EBX,[ESP+90]; PUSH EAX; CALL 1D1E00; TEST AL,AL
        auto &gp=this->gp;
        gp[ebx]=local(logic::local::clusters);put(0x13b75au,gp[eax]);
        this->call(0x13b75bu,0x13b760u,0x1d1e00u,find);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        this->reached((gp[eax]&0xffu)?Branch::found:Branch::copied);
        return gp[eax]&0xffu;
    }
    void clusters_added(std::uint32_t game_value,Kept &kept) {                                          // LEA EBX,[ESI+11B8]; LEA EDX,[ESI+1138]; PUSH EBX; LEA EAX,[ESP+94]; CALL 140EC0
        auto &gp=this->gp;
        gp[ebx]=game_value+logic::game::reachable;gp[edx]=game_value+logic::game::seen;put(0x13b77cu,gp[ebx]);
        gp[eax]=local(logic::local::clusters);
        this->call(0x13b784u,0x13b789u,0x140ec0u,add);depth-=4u;
        refresh(kept);
    }
    std::uint32_t words(std::uint32_t count,std::uint32_t game_value,Kept &kept) {                     // ADD ECX,1F; SAR ECX,5; SHL ECX,2; MOV EDX,ECX; LEA ESI; ADD EBX,11B8; SHR ECX,2; MOV EDI,EBX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.shift(fl.right(fl.plus(count,0x1fu,4),5u,4,true),2u,4);gp[edx]=gp[ecx];
        gp[esi]=game_value+logic::game::seen;kept.reachable=gp[ebx]=fl.plus(game_value,logic::game::reachable,4);
        gp[ecx]=fl.right(gp[ecx],2u,4,false);gp[edi]=gp[ebx];
        return gp[ecx];
    }
    std::uint32_t bytes(){auto &gp=this->gp;return gp[ecx]=this->flags.logic(gp[edx]&3u,4);}           // MOV ECX,EDX; AND ECX,3
    std::uint32_t cluster(Kept &kept) {                                                                  // CALL 13B590; CMP AX,FFFF
        auto &gp=this->gp;
        this->call(0x13b7bcu,0x13b7c1u,0x13b590u,cluster_child);
        refresh(kept);
        this->flags.minus(gp[eax]&0xffffu,logic::cluster_none,2);
        if((gp[eax]&0xffffu)!=logic::cluster_none)this->reached(Branch::cluster);
        return gp[eax];
    }
    std::uint32_t row(std::uint32_t count,std::uint32_t cluster_value) {                                // MOVSX EAX,AX; LEA EDX,[ECX+1F]; SAR EDX,5; IMUL EDX,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=sx16(cluster_value);
        return gp[edx]=fl.product(fl.right(count+0x1fu,5u,4,true),gp[eax]);
    }
    void row_added(std::uint32_t bits,std::uint32_t row_offset,Kept &kept) {                            // LEA EAX,[EAX+EDX*4]; PUSH EBX; MOV EDX,EBX; CALL 140EC0
        auto &gp=this->gp;
        gp[eax]=bits+4u*row_offset;put(0x13b7dfu,kept.reachable);gp[edx]=kept.reachable;
        this->call(0x13b7e2u,0x13b7e7u,0x140ec0u,add);depth-=4u;
        refresh(kept);
    }
    void seen_comparing(std::uint32_t set) {                                                             // LEA EDI,[ESI+1138]; MOV ECX,10; LEA ESI,[ESP+50]; XOR EDX,EDX
        auto &gp=this->gp;
        gp[edi]=set;gp[ecx]=logic::set_words;gp[esi]=local(logic::local::seen_before);gp[edx]=this->flags.logic(0u,4);
    }
    void reachable_comparing(std::uint32_t game_value) {                                                 // ADD EDI,11B8; MOV ECX,10; LEA ESI,[ESP+10]; XOR EDX,EDX
        auto &gp=this->gp;
        reachable_phase=true;
        gp[edi]=this->flags.plus(game_value,logic::game::reachable,4);gp[ecx]=logic::set_words;
        gp[esi]=local(logic::local::reachable_before);gp[edx]=this->flags.logic(0u,4);
    }
    bool words_compared(std::uint32_t before,std::uint32_t now) {                                       // CMPSD
        this->flags.minus(before,now,4);
        if(before!=now)this->reached(reachable_phase?Branch::reachable_changed:Branch::seen_changed);
        return before==now;
    }
    std::uint32_t listeners(){return this->gp[esi]=this->flags.logic(0u,4);}                            // XOR ESI,ESI
    void callback_tested(std::uint32_t callback){this->test32(callback);}                              // TEST EAX,EAX
    void announced(std::uint32_t callback,std::uint32_t count,std::uint32_t game_value,std::uint32_t set,std::uint32_t before,Kept &kept) {
        auto &gp=this->gp;                                                                               // PUSH ECX; ADD EDX,set; PUSH EDX; LEA ECX,[ESP+..]; PUSH ECX; CALL EAX
        const std::uint32_t base=reachable_phase?0x13b866u:0x13b816u;
        put(base,count);gp[edx]=this->flags.plus(game_value,set,4);put(base+7u,gp[edx]);
        gp[ecx]=local(before);put(base+0xcu,gp[ecx]);
        auto child=[&](State &v){dispatch(v,callback,base+0xdu);};
        this->call(base+0xdu,base+0xfu,callback,child);depth-=12u;
        refresh(kept);
    }
    bool next_listener(Kept &kept) {                                                                     // ADD ESI,24; CMP ESI,990; JB
        auto &gp=this->gp;
        kept.game_address=gp[esi]=this->flags.plus(kept.game_address,logic::listener::size,4);
        this->compare32(gp[esi],logic::listener::end);
        return gp[esi]<logic::listener::end;
    }
    void restoring() {                                                                                   // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,C0
        auto &gp=this->gp;
        gp[edi]=this->pop(0x13b880u);gp[esi]=this->pop(0x13b881u);gp[ebp]=this->pop(0x13b882u);gp[ebx]=this->pop(0x13b883u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::cluster_visibility_update::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi]});
        this->ret(0x13b88au,0u);this->publish(0x13b88au);
    }
};
template<class State,class Reference,class Gather,class Find,class Add,class Cluster,class Dispatch>
bool bridge(State &s,Reference reference,Gather gather,Find find,Add add,Cluster cluster,Dispatch dispatch) {
    return run_owner(s,0x13b6f0u,0x13b88au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Gather,Find,Add,Cluster,Dispatch>(v,c,t,gather,find,add,cluster,dispatch);});
}
} // namespace halo2::native_cluster_visibility_update_detail
