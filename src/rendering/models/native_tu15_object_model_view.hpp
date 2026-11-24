#pragma once
// Boundary adapter for the object model view, entered at 212C70
// (native_tu15_object_model_view_systems.hpp). Its callers and BC190 keep
// the original ABI, which the adapter owns: EAX the object handle, ESI the
// view; an 8-byte frame (the definition, the render model's address); EBX,
// EDI and (once the model is named) EBP saved, holding the node block, the
// tag instances then the marker block, and the model; BC190 gets the view +
// 8 pushed (and pops it), and every register it returns comes back; AL 1
// or 0 (MOV AL,BL) above the rest of EAX as the original leaves it; ECX and
// EDX as the original leaves them; the flags of the final ADD ESP; RET.
// Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_MODEL_VIEW=1 restores the translated body.
// HALO2_VERIFY_OBJECT_MODEL_VIEW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/models/native_tu15_object_model_view_systems.hpp"

namespace halo2::native_object_model_view_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_model_view;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_MODEL_VIEW");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_MODEL_VIEW");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_model, no_render, filled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object model view verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr std::uint32_t frame_bytes=8u;
template<class State,class Route> struct ModelView : Port<State> {
    using Access=logic::object_model_view::Access;
    using Local=logic::object_model_view::Local;
    using Kept=logic::object_model_view::Kept;
    Route &route;std::uint32_t depth{};
    ModelView(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t local(Local l)const{return this->gp[esp]+depth+4u*std::uint32_t(l);}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    void restore(std::uint32_t pc,unsigned r){this->gp[r]=this->pop(pc);depth-=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x212c73u;
        case header_datums:return 0x212c79u;
        case object_address:return 0x212c87u;
        case object_tag:return 0x212c8bu;
        case tag_instances:return 0x212c98u;
        case definition_address:return 0x212c9eu;
        case definition_store:return 0x212ca2u;
        case model:return 0x212ca6u;
        case model_address:return 0x212cbau;
        case render:return 0x212cbeu;
        case nodes:return 0x212cc6u;
        case render_address:return 0x212cd6u;
        case markers:return 0x212cdau;
        case render_store:return 0x212ce1u;
        case handle_store:return 0x212ce9u;
        case extra:return 0x212cebu;
        case extra_store:return 0x212cf5u;
        case definition_reload:return 0x212cfdu;
        case model_reload:return 0x212d01u;
        case render_reload:return 0x212d04u;
        case model_store:return 0x212d08u;
        case render_handle_reload:return 0x212d0bu;
        case model_address_store:return 0x212d0eu;
        case markers_store:return 0x212d12u;
        case render_address_store:return 0x212d16u;
        case nodes_store:return 0x212d19u;
        case render_handle_store:return 0x212d1cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case definition_address:case render_address:case render_handle_reload:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
        case header_datums:gp[edx]=v;gp[ecx]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case object_address:case extra:case model_reload:gp[ecx]=v;break;
        // MOV EDX,[ECX]; AND EDX,FFFF; PUSH EBX; SHL EDX,4; PUSH EDI.
        case object_tag:
            gp[edx]=fl.logic(v&0xffffu,4);save(0x212c93u,gp[ebx]);gp[edx]=fl.shift(gp[edx],logic::tag_instance_shift,4);
            save(0x212c97u,gp[edi]);break;
        case tag_instances:gp[edi]=v;break;
        // MOV EDX,[EDX+38]; XOR BL,BL; CMP EDX,-1.
        case model:gp[edx]=v;set_low_byte(gp[ebx],fl.logic(0u,1));fl.minus(v,logic::none,4);break;
        case model_address:gp[ebp]=v;break;
        case render:gp[edx]=v;fl.minus(v,logic::none,4);break;                                       // CMP EDX,-1
        // MOVSX EBX,[ECX+116] (blocks()); AND EDX,FFFF; SHL EDX,4.
        case nodes:gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),logic::tag_instance_shift,4);break;
        case definition_reload:case render_reload:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOVSX EBX,[ECX+116]; MOVSX EDI,[ECX+11A]; (the render model's slot); ADD EDI,ECX; ADD EBX,ECX.
    void blocks(const Kept &kept,std::uint32_t object) {
        auto &gp=this->gp;
        gp[edi]=this->flags.plus(kept.markers-object,object,4);gp[ebx]=this->flags.plus(kept.nodes-object,object,4);
    }
    // AND EDX,FFFF; SHL EDX,4; PUSH EBP.
    void saved_model() {
        auto &gp=this->gp;
        gp[edx]=this->flags.shift(this->flags.logic(gp[edx]&0xffffu,4),logic::tag_instance_shift,4);save(0x212cb9u,gp[ebp]);
    }
    void pushed_target(std::uint32_t target){this->gp[edx]=target;save(0x212cf4u,target);}         // LEA EDX,[ESI+8]; PUSH EDX
    void fill_from(Kept &kept) {
        auto &gp=this->gp;
        this->call_routed(route,0x212cf8u,0x212cfdu,0xbc190u);depth-=4u;
        kept={gp[esi],gp[ebp],gp[edi],gp[ebx]};
    }
    void restored_model(){restore(0x212d11u,ebp);}
    void restored_markers(){restore(0x212d15u,edi);}
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],frame_bytes,4);                                          // SUB ESP,8
        Memory<ModelView> memory{*this};
        using logic::object_model_view::Outcome;
        const auto outcome=logic::object_model_view::fill(memory,*this,gp[eax],gp[esi]);
        std::uint32_t exit=0;
        switch(outcome) {
        case Outcome::filled:
            this->reached(Branch::filled);set_low_byte(gp[eax],1u);restore(0x212d21u,ebx);exit=0x212d25u;break;
        case Outcome::no_render:                                                                  // POP EBP; POP EDI; MOV AL,BL; POP EBX
            this->reached(Branch::no_render);restore(0x212d26u,ebp);restore(0x212d27u,edi);set_low_byte(gp[eax],gp[ebx]);
            restore(0x212d2au,ebx);exit=0x212d2eu;break;
        case Outcome::no_model:                                                                   // POP EDI; MOV AL,BL; POP EBX
            this->reached(Branch::no_model);restore(0x212d2fu,edi);set_low_byte(gp[eax],gp[ebx]);
            restore(0x212d32u,ebx);exit=0x212d36u;break;
        }
        gp[esp]=this->flags.plus(gp[esp],frame_bytes,4);                                           // ADD ESP,8
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x212c70u,0x212d36u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ModelView<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_object_model_view_detail
