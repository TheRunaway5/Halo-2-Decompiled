#pragma once
// Boundary adapter for physics groups, 1DDBF0
// (native_tu15_physics_groups_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: four arguments on the stack
// (RET 10), EAX the maximum build status; MOV EAX,4E84 and the stack probe
// 32A660 make the frame, with EBX, EBP, ESI and EDI saved below it, read
// from the ESP each child left. The stack probe 32A660 and 2E31E0 (three
// pushed, caller ADD ESP,C) go through the dispatch; 212C70 (ESI view, EAX
// handle), 184920 (ESI matrices, two pushed, RET 8), 1DDEA0 (EAX motion
// bytes, eight pushed, RET 20), 1D8330 (EAX head, four pushed, RET 10), and
// 1DED80 (EDI view, ECX state) are direct; every register each returns comes
// back. Each access publishes its original instruction.
// HALO2_REFERENCE_PHYSICS_GROUPS=1 restores the translated body.
// HALO2_VERIFY_PHYSICS_GROUPS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_physics_groups_systems.hpp"

namespace halo2::native_physics_groups_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_physics_groups;
using Access=logic::physics_groups::Access;
using Kept=logic::physics_groups::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PHYSICS_GROUPS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PHYSICS_GROUPS");return v;}
enum Branch : unsigned {
    has_header_flag, force_keyframed, keep_keyframed, no_view, no_bodies,
    kind_2, kind_3, kind_4, kind_5, kind_6, kind_7,
    keyframed_override, fixed_none_node, fixed_node_clear, fixed_node_set,
    root_created, root_empty, root_nonneg, root_neg,
    no_nodes, node_visited, node_created, node_empty, node_max_updated, node_max_kept,
    no_groups, reserved, already_capacity, instantiated, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Physics groups"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{
    0x1ddbfau,0x1ddc00u,0x1ddc04u,0x1ddc0bu,0x1ddc1du,0x1ddc23u,0x1ddc2du,0x1ddc3bu,
    0x1ddc54u,0x1ddc66u,
    0x1ddc6fu,0x1ddc73u,0x1ddc77u,0x1ddc7bu,0x1ddc7fu,0x1ddc83u,0x1ddc87u,0x1ddc8bu,
    0x1ddc8fu,0x1ddca0u,0x1ddca3u,0x1ddcaau,0x1ddcd9u,0x1ddce9u,0x1ddcf9u,0x1ddd02u,0x1ddd0eu,
    0x1ddd1au,0x1ddd21u,
    0x1ddd43u,0x1ddd66u,0x1ddd7au,0x1ddd82u,
    0x1ddd86u,0x1ddd8du,0x1dddb6u,0x1dddc5u,0x1ddde5u,0x1dddfau,0x1dddfeu,0x1dde04u,
    0x1dde08u,0x1dde0fu,
    0x1dde1eu,0x1dde45u,0x1dde49u,0x1dde51u,
    0x1dde6cu,0x1dde6cu,0x1dde73u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::result_load)+1u);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 212C70, 184920, 1DDEA0, 1D8330, 1DED80
    Dispatch &dispatch;      // stack probe 32A660 and 2E31E0
    std::uint32_t depth{};   // bytes pushed beyond EBX, EBP, ESI and EDI (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.state_address=gp[ebx];kept.cursor=gp[ebp];kept.index=gp[esi];kept.count=gp[edi];}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto route=[&](State &v,std::uint32_t t,std::uint32_t site){dispatch(v,t,site);};
        this->call_routed(route,pc,pc+5u,target);depth-=popped;refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects_load:case objects_data:case nodes_load:gp[edx]=v;break;
        case state_argument:gp[ebx]=v;break;
        case object_handle:case object_handle_matrices:case body_count_first:case body_count_next:
        case root_step_result:case model_before_nodes:case node_step_result:case model_next_node:
        case group_count_load:case group_head_load:case result_load:gp[eax]=v;break;
        case header_kind:set_low_byte(gp[ecx],v);break;
        case model_load:gp[esi]=v;break;
        case bodies_load:case allow_root:case allow_node:case node_count_next:case array_capacity:gp[ecx]=v;break;
        case body_motion_type:gp[edx]=sx16(v);break;
        case keyframed_load:case fixed_load:set_low_byte(gp[edx],v);break;
        case body_node:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);break;
        case group_kind_load:set_low_byte(gp[edx],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // Frame setup: MOV EAX,4E84; CALL 32A660.
    void probed() {
        this->gp[eax]=logic::frame_size;
        auto route=[&](State &v,std::uint32_t t,std::uint32_t site){dispatch(v,t,site);};
        this->call_routed(route,0x1ddbf5u,0x1ddbfau,0x32a660u);
        depth=0u-16u;
    }
    void saving(unsigned n) {
        static constexpr std::uint32_t pcs[]{0x1ddc03u,0x1ddc19u,0x1ddc21u,0x1ddc22u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; PUSH EBP; LEA ECX,[EDX+ECX*4]
    std::uint32_t header_address(std::uint32_t data,std::uint32_t handle) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);
        gp[ecx]=3u*gp[ecx];
        saving(2u);
        return gp[ecx]=data+4u*gp[ecx];
    }
    // TEST byte ptr [ECX+2], 1
    bool header_flag(std::uint32_t v) {
        this->flags.logic((v&0xffu)&1u,1);
        if(!this->flags.zf){this->reached(Branch::has_header_flag);return true;}
        return false;
    }
    // MOV EDX,1; SHL EDX,CL; TEST DL,DL; JS
    bool kind_forces_keyframed(std::uint32_t kind_byte) {
        auto &gp=this->gp;
        gp[edx]=this->flags.shift(1u,kind_byte&0xffu,4);
        this->test8(gp[edx]);
        if(this->flags.sf){this->reached(Branch::keep_keyframed);return false;}
        this->reached(Branch::force_keyframed);
        return true;
    }
    // LEA ESI,[ESP+38]; CALL 212C70; TEST AL,AL
    bool load_model_view(Kept &kept) {
        auto &gp=this->gp;
        gp[esi]=local(logic::local::model_view);
        direct(0x1ddc47u,0x212c70u,0u,kept);
        this->test8(gp[eax]);
        if(this->flags.zf){this->reached(Branch::no_view);return false;}
        return true;
    }
    // PUSH 0; PUSH EAX; LEA ESI,[ESP+A54]; CALL 184920 (RET 8)
    void resolve_matrices(Kept &kept,std::uint32_t handle) {
        auto &gp=this->gp;
        put(0x1ddc57u,0u);
        put(0x1ddc59u,handle);
        gp[esi]=local(logic::local::matrices);
        direct(0x1ddc61u,0x184920u,8u,kept);
    }
    std::uint32_t zero_value(){return this->gp[eax]=this->flags.logic(0u,4);}                      // XOR EAX,EAX
    std::uint32_t zero_body_index(){return this->gp[edi]=this->flags.logic(0u,4);}                 // XOR EDI,EDI
    bool positive(std::uint32_t v) {                                                               // TEST r,r; JLE
        this->test32(v);
        return std::int32_t(v)>0;
    }
    bool has_bodies(std::uint32_t v) {
        const bool ok=positive(v);
        if(!ok)this->reached(Branch::no_bodies);
        return ok;
    }
    bool has_groups(std::uint32_t v) {
        const bool ok=positive(v);
        if(!ok)this->reached(Branch::no_groups);
        return ok;
    }
    std::uint32_t zero_body_offset(){return this->gp[ebp]=this->flags.logic(0u,4);}                // XOR EBP,EBP
    std::uint32_t advance_body(std::uint32_t base,std::uint32_t offset) {                          // ADD ECX,EBP
        return this->gp[ecx]=this->flags.plus(base,offset,4);
    }
    // JMP dword ptr [EDX*4 + 1DDE84]
    std::uint32_t mapped_mode(std::uint32_t target) {
        auto &gp=this->gp;
        switch(target) {
        case 0x1ddcb1u:this->reached(Branch::kind_2);return gp[eax]=2u;
        case 0x1ddcb8u:this->reached(Branch::kind_3);return gp[eax]=3u;
        case 0x1ddcbfu:this->reached(Branch::kind_4);return gp[eax]=4u;
        case 0x1ddcc6u:this->reached(Branch::kind_5);return gp[eax]=5u;
        case 0x1ddccdu:this->reached(Branch::kind_6);return gp[eax]=6u;
        case 0x1ddcd4u:this->reached(Branch::kind_7);return gp[eax]=7u;
        default:throw std::runtime_error("Required original solver dispatch target unavailable");
        }
    }
    bool flag_nonzero(std::uint32_t v){this->test8(v);return !this->flags.zf;}                     // TEST DL,DL
    std::uint32_t override_mode(std::uint32_t mode) {
        if(mode==7u)this->reached(Branch::keyframed_override);
        return this->gp[eax]=mode;
    }
    bool equals_seven(std::uint32_t mode){this->compare32(mode,7u);return this->flags.zf;}         // CMP EAX,7
    bool node_is_none(std::uint32_t word) {                                                        // CMP CX,-1
        this->flags.minus(word&0xffffu,logic::no_node,2);
        if(this->flags.zf){this->reached(Branch::fixed_none_node);return true;}
        return false;
    }
    // MOVSX ECX,CX; LEA ECX,[ECX+ECX*2] -> address EDX + ECX*4
    std::uint32_t node_address(std::uint32_t nodes,std::uint32_t word) {
        auto &gp=this->gp;
        gp[ecx]=sx16(word);
        gp[ecx]=3u*gp[ecx];
        return nodes+4u*gp[ecx];
    }
    // TEST byte ptr [EDX+ECX*4+4], 1
    bool node_fixed(std::uint32_t v) {
        this->flags.logic((v&0xffu)&1u,1);
        const bool set=!this->flags.zf;
        this->reached(set?Branch::fixed_node_set:Branch::fixed_node_clear);
        return set;
    }
    std::uint32_t next_body_index(std::uint32_t i){return this->gp[edi]=this->flags.increment(i,4,false);}
    std::uint32_t next_body_offset(std::uint32_t o){return this->gp[ebp]=this->flags.plus(o,logic::body_size,4);}
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}
    // 1DDD33: LEA EAX,[ESP+CC]; PUSH EAX; LEA ECX,[ESP+A50]; PUSH ECX
    void push_root_head() {
        auto &gp=this->gp;
        gp[eax]=local(logic::local::groups);put(0x1ddd3au,gp[eax]);
        gp[ecx]=local(logic::local::matrices);put(0x1ddd42u,gp[ecx]);
    }
    // 1DDD4A..1DDD66: push visited, step_result, -1, allow, model_view, state; XOR ESI,ESI; LEA EAX,[ESP+AC]
    std::uint32_t push_root_tail(std::uint32_t state_addr,std::uint32_t allow_val) {
        auto &gp=this->gp;
        gp[edx]=local(logic::local::visited);put(0x1ddd4eu,gp[edx]);
        gp[eax]=local(logic::local::step_result);put(0x1ddd53u,gp[eax]);
        put(0x1ddd54u,logic::none);
        put(0x1ddd56u,allow_val);
        gp[edx]=local(logic::local::model_view);put(0x1ddd5bu,gp[edx]);
        gp[esi]=this->flags.logic(0u,4);
        put(0x1ddd5eu,state_addr);
        gp[eax]=local(logic::local::motion_bytes);
        return gp[esi];
    }
    std::uint32_t zero_group_count(){return this->gp[edi]=this->flags.logic(0u,4);}                // XOR EDI,EDI
    bool build_group(Kept &kept,bool is_root) {                                                    // CALL 1DDEA0; TEST AL,AL
        direct(is_root?0x1ddd6cu:0x1dddedu,0x1ddea0u,0x20u,kept);
        this->test8(this->gp[eax]);
        const bool ok=!this->flags.zf;
        if(is_root)this->reached(ok?Branch::root_created:Branch::root_empty);
        else this->reached(ok?Branch::node_created:Branch::node_empty);
        return ok;
    }
    std::uint32_t one_group(){return this->gp[edi]=1u;}
    bool at_least(std::uint32_t a,std::uint32_t b) {                                               // CMP EAX,ESI; JL
        this->compare32(a,b);
        const bool ge=std::int32_t(a)>=std::int32_t(b);
        this->reached(ge?Branch::root_nonneg:Branch::root_neg);
        return ge;
    }
    bool has_nodes(std::uint32_t a,std::uint32_t b) {                                              // CMP [EAX+C8],ESI; JLE
        this->compare32(a,b);
        const bool ok=std::int32_t(a)>std::int32_t(b);
        if(!ok)this->reached(Branch::no_nodes);
        return ok;
    }
    bool max_exceeds(std::uint32_t a,std::uint32_t b) {                                            // CMP [ESP+14],EAX; JG
        this->compare32(a,b);
        const bool gt=std::int32_t(a)>std::int32_t(b);
        this->reached(gt?Branch::node_max_kept:Branch::node_max_updated);
        return gt;
    }
    // MOV ECX,EDI; IMUL ECX,ECX,4C; LEA EBP,[ESP+ECX+CC]
    std::uint32_t group_slot(std::uint32_t count) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.product(count,logic::group_size);
        return gp[ebp]=local(logic::local::groups)+gp[ecx];
    }
    // MOV ECX,ESI; AND ECX,1F; MOV EDX,1; SHL EDX,CL; MOV EAX,ESI; SAR EAX,5
    std::uint32_t visited_bit_mask(std::uint32_t idx) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(idx&0x1fu,4);
        gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);
        return gp[edx];
    }
    std::uint32_t visited_word_index(std::uint32_t idx) {
        return this->gp[eax]=this->flags.right(idx,5u,4,true);
    }
    bool bit_set(std::uint32_t word,std::uint32_t mask) {
        this->flags.logic(word&mask,4);
        if(!this->flags.zf){this->reached(Branch::node_visited);return true;}
        return false;
    }
    void push_node_head(std::uint32_t slot) {
        auto &gp=this->gp;
        put(0x1dddbcu,slot);
        gp[ecx]=local(logic::local::matrices);put(0x1dddc4u,gp[ecx]);
    }
    void push_node_tail(std::uint32_t state_addr,std::uint32_t allow_val,std::uint32_t node_idx) {
        auto &gp=this->gp;
        gp[edx]=local(logic::local::visited);put(0x1dddd0u,gp[edx]);
        gp[eax]=local(logic::local::step_result);put(0x1dddd5u,gp[eax]);
        put(0x1dddd6u,node_idx);
        put(0x1dddd7u,allow_val);
        gp[edx]=local(logic::local::model_view);put(0x1ddddcu,gp[edx]);
        put(0x1dddddu,state_addr);
        gp[eax]=local(logic::local::motion_bytes);
    }
    std::uint32_t next_group_count(std::uint32_t c){return this->gp[edi]=this->flags.increment(c,4,false);}
    std::uint32_t next_group_slot(std::uint32_t slot){return this->gp[ebp]=this->flags.plus(slot,logic::group_size,4);}
    std::uint32_t next_node(std::uint32_t idx){return this->gp[esi]=this->flags.increment(idx,4,false);}
    // LEA EAX,[EBX+70]; AND ECX,7FFFFFFF; CMP ECX,EDI; JGE
    bool needs_reserve(std::uint32_t state_addr,std::uint32_t capacity_raw,std::uint32_t count) {
        auto &gp=this->gp;
        gp[eax]=logic::at(state_addr,logic::state_field::groups_array);
        gp[ecx]=this->flags.logic(capacity_raw&logic::capacity_mask,4);
        this->compare32(gp[ecx],count);
        const bool need=std::int32_t(gp[ecx])<std::int32_t(count);
        this->reached(need?Branch::reserved:Branch::already_capacity);
        return need;
    }
    // PUSH 60; PUSH EDI; PUSH EAX; CALL 2E31E0; ADD ESP,C
    void reserve_groups(Kept &kept,std::uint32_t count) {
        auto &gp=this->gp;
        put(0x1dde2eu,logic::array_element_size);
        put(0x1dde30u,count);
        put(0x1dde31u,gp[eax]);
        routed(0x1dde32u,0x2e31e0u,0u,kept);
        gp[esp]=this->flags.plus(gp[esp],0xcu,4);depth-=0xcu;
    }
    // LEA ESI,[ESP+110]
    std::uint32_t group_tail_start(){return this->gp[esi]=local(logic::local::groups+logic::group_entry::count);}
    void clear_group_kind(){this->gp[edx]=this->flags.logic(0u,4);}                                // XOR EDX,EDX
    // LEA ECX,[ESI-40]; PUSH EDX; PUSH EAX
    std::uint32_t push_group_tail(std::uint32_t tail_ptr,std::uint32_t count_word,std::uint32_t kind_byte) {
        auto &gp=this->gp;
        gp[ecx]=tail_ptr-0x40u;
        put(0x1dde4fu,kind_byte&0xffu);
        put(0x1dde50u,count_word);
        return gp[ecx];
    }
    // PUSH ECX; PUSH EBX; CALL 1D8330 (RET 10)
    void instantiate_group(Kept &kept,std::uint32_t payload,std::uint32_t head_word) {
        this->gp[eax]=head_word;
        put(0x1dde54u,payload);
        put(0x1dde55u,kept.state_address);
        direct(0x1dde56u,0x1d8330u,0x10u,kept);
        this->reached(Branch::instantiated);
    }
    std::uint32_t next_group_tail(std::uint32_t tail_ptr){return this->gp[esi]=this->flags.plus(tail_ptr,logic::group_size,4);}
    std::uint32_t decrement_groups(std::uint32_t c){return this->gp[edi]=this->flags.increment(c,4,true);}
    // LEA EDI,[ESP+38]; MOV ECX,EBX; CALL 1DED80
    void finalize_view(Kept &kept) {
        auto &gp=this->gp;
        gp[edi]=local(logic::local::model_view);
        gp[ecx]=kept.state_address;
        direct(0x1dde67u,0x1ded80u,0u,kept);
    }
    std::uint32_t set_built_flag(std::uint32_t flags_val){return this->flags.logic(flags_val|logic::built_flag,4);}
    void finish() {
        auto &gp=this->gp;
        gp[edi]=this->pop(0x1dde77u);
        gp[esi]=this->pop(0x1dde78u);
        gp[ebp]=this->pop(0x1dde79u);
        gp[ebx]=this->pop(0x1dde7au);
        gp[esp]=this->flags.plus(gp[esp],logic::frame_size,4);
        this->ret(0x1dde81u,16u);
        this->publish(0x1dde81u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::physics_groups::build(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        finish();
    }
};
template<class State,class Reference,class Children,class Dispatch>
bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x1ddbf0u,0x1dde81u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_physics_groups_detail
