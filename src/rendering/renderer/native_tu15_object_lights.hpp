#pragma once
// Boundary adapter for the object lights, 1181F0
// (native_tu15_object_lights_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the object and the active
// byte on the stack (RET 4); a 0x58-byte frame (the object, the model state)
// below the saved EBP, and ESI, EDI and EBX as the walk needs them; 212C70
// (EAX the object, ESI the state), the owner's virtual method and 1D8ED0
// through the dispatch, 1D9080, 1D9100 and 1D9190 direct, every register
// each returns coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_LIGHTS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_LIGHTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_object_lights_systems.hpp"

namespace halo2::native_object_lights_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_lights;
using Access=logic::object_lights::Access;
using Kept=logic::object_lights::Kept;
using Outcome=logic::object_lights::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_LIGHTS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_LIGHTS");return v;}
enum Branch : unsigned {no_block, no_model, updated, attached, held_light, default_transform, branch_count};
static_assert(unsigned(Outcome::updated)==Branch::updated);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object lights"};return v;}
inline constexpr std::uint32_t sites[]{0x1181f3u,0x1181f9u,0x118208u,0x11820cu,0x118213u,0x11822fu,0x11823du,0x118242u,0x118248u,
    0x118260u,0x118263u,0x118267u,0x11826au,0x11826cu,0x118278u,0x11827eu,0x118289u,0x11828cu,0x118292u,0x118296u,0x118299u,
    0x1182a6u,0x1182b3u,0u,0u,0x118308u,0x11830cu,0x11831cu,0x118322u,0u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::object_flags_store)+1u);
template<class State,class Dispatch,class Place,class Aim,class Hold> struct Adapter : Port<State> {
    Dispatch &dispatch;
    Place &place_child;
    Aim &aim_child;
    Hold &hold_child;
    std::uint32_t depth{};      // bytes below the return address
    bool active_path{},no_node{},saved_esi{},final_active{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d,Place &p,Aim &a,Hold &h)
        :Port<State>(s,c,t,{}),dispatch(d),place_child(p),aim_child(a),hold_child(h){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        if(a==marker_node)return active_path?0x1182c1u:0x1182ebu;
        if(a==matrices_load)return active_path?0x1182d8u:0x1182fau;
        if(a==object_flags_store)return final_active?0x11833fu:0x11832fu;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case datums:gp[edx]=v;break;
        case object_address:case object_again:gp[ebp]=v;break;
        case light_block_again:gp[eax]=v;gp[esi]=5u*fl.logic(v&0xffffu,4);break;                    // AND EAX,FFFF; LEA ESI,[EAX+EAX*4]
        case light_blocks:case owner_vtable:case model_load:case count_again:gp[eax]=v;break;
        case blocks_data:gp[ecx]=v;gp[esi]=fl.shift(gp[esi],5u,4);break;                             // SHL ESI,5
        case light_count:gp[eax]=v;gp[esi]=fl.plus(gp[esi],gp[ecx],4);break;                         // ADD ESI,ECX
        case entries:case light_owner:case marker_count:case model_markers:case matrices_load:case scale_aim:gp[ecx]=v;break;
        case entry_light:case marker_list:gp[edx]=v;break;
        case entries_again:gp[ecx]=v;gp[eax]=gp[ebx]+v;break;                                        // LEA EAX,[EBX+ECX]
        case marker_byte:gp[eax]=logic::widen8(v);gp[ebp]=9u*gp[eax];break;                          // LEA EBP,[EAX+EAX*8]
        case active_argument:set_low_byte(gp[eax],v);break;
        case scale_place:gp[eax]=v;break;
        case marker_node:gp[eax]=logic::widen16(v);break;
        case active_flag:set_low_byte(gp[eax],v);this->test8(v&0xffu);break;                          // MOV AL; TEST AL,AL
        case object_flags:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void indexing(std::uint32_t object_handle) {                                                       // MOV ECX,EAX; AND ECX,FFFF; LEA; PUSH EBP
        auto &gp=this->gp;
        gp[ecx]=3u*this->flags.logic(object_handle&0xffffu,4);put(0x118207u,gp[ebp]);
    }
    void block_tested(std::uint32_t block){this->compare32(block,logic::none);}                       // CMP [EBP+B4],-1
    void saving(unsigned which) {
        auto &gp=this->gp;
        if(which==0u){put(0x11821du,gp[esi]);saved_esi=true;}
        else if(which==1u)put(0x11824eu,gp[edi]);
        else{put(0x118259u,gp[ebx]);gp[ebx]=this->flags.logic(0u,4);}                                // PUSH EBX; XOR EBX,EBX
    }
    void restoring(unsigned){this->gp[ebx]=take(0x11831bu);}
    std::uint32_t model_state(std::uint32_t object_handle,Kept &kept) {                                // LEA ESI,[ESP+C]; CALL 212C70; TEST AL,AL
        auto &gp=this->gp;
        gp[esi]=local(logic::local::state);gp[eax]=object_handle;
        auto child=[&](State &v){dispatch(v,0x212c70u,0x118222u);};
        this->call(0x118222u,0x118227u,0x212c70u,child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void count_tested(std::uint32_t count,const Kept &){this->gp[edi]=this->flags.logic(0u,4);this->test32(count);}   // XOR EDI,EDI; TEST EAX,EAX
    std::uint32_t type(std::uint32_t owner,std::uint32_t method,Kept &kept) {                         // CALL [EAX+18] (ECX the owner)
        auto &gp=this->gp;
        gp[ecx]=owner;
        auto child=[&](State &v){dispatch(v,method,0x11826cu);};
        this->call(0x11826cu,0x11826fu,method,child);
        refresh(kept);
        return gp[eax];
    }
    void type_tested(std::uint32_t type_value){this->compare32(type_value,logic::light_type);}         // CMP EAX,6
    void markers_tested(std::uint32_t,std::uint32_t markers){this->test32(markers);}                  // TEST ECX,ECX
    void marker_chosen(const Kept &kept,std::uint32_t active) {                                        // SHL EBP,4; ADD EBP,ECX; TEST AL,AL
        auto &gp=this->gp;auto &fl=this->flags;
        fl.plus(fl.shift(gp[ebp],4u,4),gp[ecx],4);gp[ebp]=kept.held;this->test8(active&0xffu);active_path=(active&0xffu)!=0u;
    }
    void node_tested(std::uint32_t node) {                                                             // CMP EAX,-1
        this->compare32(node,logic::none);no_node=node==logic::none;
        if(no_node)this->reached(Branch::default_transform);
    }
    void transform_registers(std::uint32_t transform) {                                                // LEA EAX,[ESP+1C] or IMUL EAX,34; ADD EAX,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        if(no_node)gp[eax]=transform;
        else{fl.product(gp[eax],logic::matrix_size);gp[eax]=fl.plus(transform-gp[ecx],gp[ecx],4);}
    }
    void place(std::uint32_t,Kept &kept) {                                                             // PUSH ESI; MOV ECX,EDI; CALL 1D9080
        put(0x1182abu,kept.block);this->gp[ecx]=kept.index;
        this->call(0x1182aeu,0x1182b3u,0x1d9080u,place_child);depth-=4u;
        refresh(kept);
    }
    void aim(std::uint32_t,Kept &kept) {                                                               // PUSH ESI; MOV EAX,EDI; CALL 1D9100
        put(0x1182b9u,kept.block);this->gp[eax]=kept.index;
        this->call(0x1182bcu,0x1182c1u,0x1d9100u,aim_child);depth-=4u;
        refresh(kept);
    }
    void attach(std::uint32_t transform,Kept &kept) {                                                  // PUSH ESI; MOV ECX,EDI; CALL 1D8ED0
        transform_registers(transform);
        const std::uint32_t at=no_node?0x1182ceu:0x1182e1u;
        put(at,kept.block);this->gp[ecx]=kept.index;
        auto child=[&](State &v){dispatch(v,0x1d8ed0u,at+3u);};
        this->call(at+3u,at+8u,0x1d8ed0u,child);depth-=4u;
        refresh(kept);
        this->reached(Branch::attached);
    }
    void hold(std::uint32_t transform,Kept &kept) {                                                    // CALL 1D9190 (EAX the transform)
        transform_registers(transform);
        this->call(0x118303u,0x118308u,0x1d9190u,hold_child);
        refresh(kept);
        this->reached(Branch::held_light);
    }
    void advanced(const Kept &kept,std::uint32_t count) {                                              // INC EDI; ADD EBX,60; CMP EDI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edi]=fl.increment(kept.index-1u,4,false);gp[ebx]=fl.plus(kept.offset-logic::entry_size,logic::entry_size,4);this->compare32(kept.index,count);
    }
    void finished(std::uint32_t active){this->gp[edi]=take(0x118328u);final_active=(active&0xffu)!=0u;}  // POP EDI
    void flagged(std::uint32_t now,std::uint32_t active) {                                             // OR EAX,40 (POP ESI) or AND EAX,FFFFFFBF
        this->gp[eax]=this->flags.logic(now,4);
        if(!(active&0xffu)){this->gp[esi]=take(0x11832eu);saved_esi=false;}
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x58u,4);depth=0x58u;                                                 // SUB ESP,58
        Memory<Adapter> memory{*this};
        const auto outcome=logic::object_lights::update(memory,*this,gp[eax],Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        if(outcome==Outcome::updated && !saved_esi){gp[ebp]=take(0x118335u);gp[esp]=fl.plus(gp[esp],0x58u,4);this->ret(0x118339u,4u);this->publish(0x118339u);return;}
        if(saved_esi)gp[esi]=take(0x118345u);
        gp[ebp]=take(0x118346u);
        gp[esp]=fl.plus(gp[esp],0x58u,4);                                                              // ADD ESP,58
        this->ret(0x11834au,4u);this->publish(0x11834au);
    }
};
template<class State,class Reference,class Dispatch,class Place,class Aim,class Hold>
bool bridge(State &s,Reference reference,Dispatch dispatch,Place place,Aim aim,Hold hold) {
    return run_owner(s,0x1181f0u,0x11834au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch,Place,Aim,Hold>(v,c,t,dispatch,place,aim,hold);});
}
} // namespace halo2::native_object_lights_detail
