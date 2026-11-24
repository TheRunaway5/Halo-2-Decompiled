#pragma once
// Boundary adapter for the object point surface, 14D410
// (native_tu15_object_point_surface_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: the record, the
// model's tag and the result on the stack (RET C), AL the answer; 0x58
// bytes of frame with EBX, EBP, ESI and EDI saved below them, read from the
// ESP each child left. 130980 (EAX 3, ECX the geometry), 145200 (ECX the
// matrix, ESI and EDX the point), 1F7E00 (ESI the buffer; one pushed, RET
// 4), 1F8040 (two pushed, RET 8), 243F20 (EAX the model's vertices, EDI the
// material, ECX the vertex; one pushed, RET 4) and 1211C0 (ESI the point,
// EAX, EDX and ECX the triangle; two pushed, RET 8) go through the dispatch
// so that every unit's copy binds; every register each returns comes back.
// Each exit leaves through the original's epilogue. Each access publishes
// its original instruction.
// HALO2_REFERENCE_OBJECT_POINT_SURFACE=1 restores the translated body.
// HALO2_VERIFY_OBJECT_POINT_SURFACE=1 records the native run and replays
// the unmodified translated body against it, as in
// native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_object_point_surface_systems.hpp"

namespace halo2::native_object_point_surface_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_point_surface;
using Access=logic::object_point_surface::Access;
using Exit=logic::object_point_surface::Exit;
using Kept=logic::object_point_surface::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_POINT_SURFACE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_POINT_SURFACE");return v;}
enum Branch : unsigned {no_permutation, no_bsp, rejected, nodeless, unbuffered, partless, unmatched, unkeyed, out_of_range, unsurfaced,
    off_plane, missed, hit, sectioned, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object point surface"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{
    0x14d413u,0x14d41eu,0x14d422u,0x14d425u,0x14d428u,0x14d42bu,0x14d42eu,0x14d431u,0x14d434u,0x14d437u,0x14d43au,0x14d446u,
    0x14d44cu,0x14d450u,0x14d45bu,0x14d460u,0x14d463u,0x14d467u,0x14d470u,0x14d474u,0x14d478u,0x14d47fu,0x14d483u,0x14d487u,
    0x14d48du,0x14d494u,0x14d499u,0x14d49eu,0x14d4a5u,0x14d4b3u,0x14d4b9u,0x14d4beu,0x14d4c2u,0x14d4dbu,0x14d4e6u,0x14d4ebu,
    0x14d4eeu,0x14d4f5u,0x14d4f7u,0x14d4fau,0x14d4feu,0x14d501u,0x14d505u,0x14d50cu,0x14d51eu,0x14d522u,0x14d53cu,0x14d540u,
    0x14d554u,0x14d569u,0x14d575u,0x14d578u,0x14d57eu,0x14d585u,0x14d589u,0x14d58cu,0x14d591u,0x14d59bu,0x14d5a8u,0x14d5b0u,
    0x14d5bdu,0x14d5d4u,0x14d5d8u,0x14d5dbu,0x14d5dfu,0x14d5e5u,0x14d5ecu,0x14d5f6u,0x14d5f9u,0x14d5ffu,0x14d602u,0x14d606u,
    0x14d60eu,0x14d61au,0x14d628u,0x14d631u,0x14d637u,0x14d63du,0x14d647u,0x14d64fu,0x14d652u,0x14d65bu,0x14d661u,0x14d665u,
    0x14d668u,0x14d66bu,0x14d671u,0x14d675u,0x14d681u,0x14d685u,0x14d689u,0x14d68eu,0x14d6c7u,0x14d6cbu,0x14d6cfu,0x14d6d3u,
    0x14d6d7u,0x14d6dbu,0x14d6e3u,0x14d6edu,0x14d6fbu,0x14d6ffu,0x14d706u,0x14d709u,0x14d70fu,0x14d712u,0x14d715u,0x14d719u,
    0x14d720u,0x14d723u,0x14d730u,0x14d737u,0x14d74bu,0x14d74eu,0x14d752u,0x14d755u,0x14d759u,0x14d75cu,0x14d760u,0x14d764u,
    0x14d767u,0x14d76bu,0x14d76eu,0x14d772u,0x14d775u,0x14d778u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer)+1u);
// The shifts by 4 index 16-byte regions, permutations and parts.
static_assert(logic::model_region::size==1u<<4 && logic::model_permutation::size==1u<<4 && logic::part::size==1u<<4);
// Each exit's pops (EDI, ESI, EBP, EBX), ADD ESP,58 and RET C.
inline constexpr std::uint32_t epilogues[]{0x14d5c1u,0x14d6f1u,0x14d77cu};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;      // every child
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto route=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,route);depth-=popped;
        auto &gp=this->gp;
        kept.cursor=gp[ebx];kept.result=gp[ebp];kept.item=gp[esi];kept.record=gp[edi];
    }
    bool answered(){this->test8(this->gp[eax]);return low_byte(this->gp[eax])!=0u;}                       // TEST AL,AL
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag_argument:case object_handle:case objects_load:case bsp_load:case object_handle_again:case objects_again:
        case object_handle_pushed:case shape_load:case shape_reload:case surfaces:case vertices_load:case leaf_reload:case record_hit:
        case hit_data:case sections:case reference_load:gp[eax]=v;break;
        case permutations_offset:case record_node_again:case record_node_search:case record_region_final:gp[eax]=sx16(v);break;
        case region_permutation:gp[eax]=sx8(v);break;
        case reference_vertices:case vertex_index:case reference_second_word:gp[eax]=v;break;               // MOVZX EAX,word
        case unmatched_answer:case exhausted_answer:case answer:set_low_byte(gp[eax],v);break;              // MOV AL
        case definitions_load:case objects_data:case object_address:case region_permutations:case model_vertices:
        case objects_data_again:case point_y:case model_nodes:case result_bsp:case bsp_address_load:case bsp_data:case record_plane:
        case vertex_lists:case vertex_list_load:case part_reload:case hit_object:case tag_reload:case material_load:gp[ecx]=v;break;
        case record_region:case node_bytes:case record_node:gp[ecx]=sx16(v);break;
        case section_first:gp[ecx]=v;break;                                                                // MOVZX ECX,word
        case model_regions:case model_bsps:case point_x:case point_z:case record_shape:case model_shapes:case part_count:
        case shape_reference_load:case part_key:case record_leaf:case leaf_first:case bsp_address_again:case bsp_data_again:
        case bsp_data_reload:case reference_index_reload:case record_final:gp[edx]=v;break;
        case permutation_bsp:case nodes_offset:gp[edx]=sx16(v);break;
        case reference_first_word:gp[edx]=v;break;                                                         // MOVZX EDX,word
        case model_load:case parts:case leaf_first_next:gp[ebx]=v;break;
        case part_node:case section_size:gp[ebx]=v;break;                                                  // MOVZX EBX
        case result_argument:gp[ebp]=v;break;
        case record_region_again:case leaf_count:case leaf_count_next:gp[esi]=sx16(v);break;
        case object_again:case part_nodes:case shape_key:case part_leaves:case references:case reference_surface:case section_count:
            gp[esi]=v;break;
        case record_argument:case record_reload:case result_first_word:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame, the result and the model.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x58u,4);depth=0u-16u;}               // SUB ESP,58
    std::uint32_t none_value(){return this->gp[eax]=this->flags.logic(logic::none,4);}                        // OR EAX,-1
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x14d41cu,0x14d41du,0x14d44au,0x14d44bu};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t tag_index(std::uint32_t tag) {                                                         // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        const auto index=fl.logic(tag&logic::handle_mask,4);
        this->gp[eax]=fl.shift(index,4u,4);
        return index;
    }
    std::uint32_t object_index(std::uint32_t handle) {                                                   // AND EAX,FFFF; LEA EDX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle&logic::handle_mask,4);gp[edx]=3u*gp[eax];
        return gp[eax];
    }
    std::uint32_t permutations_address(std::uint32_t offset,std::uint32_t object){return this->gp[eax]=this->flags.plus(offset,object,4);}   // ADD EAX,ECX
    bool permutation_named(std::uint32_t v) {                                                            // CMP byte,FF
        this->flags.minus(v&0xffu,logic::no_permutation,1);
        const bool named_value=(v&0xffu)!=logic::no_permutation;
        if(!named_value)this->reached(Branch::no_permutation);
        return named_value;
    }
    std::uint32_t region_offset(std::uint32_t region){return this->gp[ecx]=this->flags.shift(region,4u,4);}     // SHL ECX,4
    std::uint32_t permutation_offset(std::uint32_t permutation){return this->gp[eax]=this->flags.shift(permutation,4u,4);}   // SHL EAX,4
    bool has_bsp(std::uint32_t v) {                                                                      // CMP EAX,FF
        this->compare32(v,logic::no_bsp);
        if(v==logic::no_bsp)this->reached(Branch::no_bsp);
        return v!=logic::no_bsp;
    }
    std::uint32_t bsp_offset(std::uint32_t index){return this->gp[eax]=this->flags.product(index,logic::bsp::size);}   // IMUL EAX,EAX,5C
    std::uint32_t bsp_address(std::uint32_t offset,std::uint32_t bsps){return this->gp[eax]=this->flags.plus(offset,bsps,4);}   // ADD EAX,EDX
    bool geometry_accepted(Kept &kept,std::uint32_t geometry) {                                          // LEA ECX,[EAX+38]; MOV EAX,3; CALL 130980; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=geometry;gp[eax]=logic::geometry_kind;
        routed(0x14d4ceu,0x130980u,0u,kept);
        const bool accepted=answered();
        if(!accepted)this->reached(Branch::rejected);
        return accepted;
    }
    // The point in the node's space.
    std::uint32_t point_address(std::uint32_t record){return this->gp[eax]=record+logic::record::point;}       // LEA EAX,[EDI+8]
    bool has_nodes(std::uint32_t bytes) {                                                                // MOV EAX,4EC4EC4F; MUL ECX; SHR EDX,4; TEST EDX,EDX; JLE
        auto &gp=this->gp;auto &fl=this->flags;
        const auto product=std::uint64_t(0x4ec4ec4fu)*bytes;
        gp[eax]=std::uint32_t(product);gp[edx]=std::uint32_t(product>>32u);fl.cf=fl.of=gp[edx]!=0u;
        gp[edx]=fl.right(gp[edx],4u,4,false);
        this->test32(gp[edx]);
        const bool any=std::int32_t(gp[edx])>0;
        if(!any)this->reached(Branch::nodeless);
        return any;
    }
    std::uint32_t node_matrix(std::uint32_t node,std::uint32_t nodes,std::uint32_t object) {            // IMUL ECX,ECX,34; ADD EDX,ESI; ADD ECX,EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.product(node,logic::node_matrix_size);gp[edx]=fl.plus(nodes,object,4);
        return gp[ecx]=fl.plus(gp[ecx],gp[edx],4);
    }
    std::uint32_t into_node(Kept &kept,std::uint32_t) {                                                  // LEA EDX,[ESP+38]; LEA ESI,[EDI+8]; CALL 145200
        auto &gp=this->gp;
        gp[edx]=local(logic::local::point);gp[esi]=kept.record+logic::record::point;
        routed(0x14d537u,0x145200u,0u,kept);
        return gp[edx];
    }
    std::uint32_t model_matrix(std::uint32_t node,std::uint32_t list) {                                  // LEA EAX,[EAX+EAX*2]; SHL EAX,5; LEA ECX,[EAX+ECX+28]
        auto &gp=this->gp;
        gp[eax]=this->flags.shift(3u*node,5u,4);
        return gp[ecx]=gp[eax]+list+logic::model_node::matrix;
    }
    void into_model(Kept &kept,std::uint32_t,std::uint32_t moved){this->gp[esi]=moved;routed(0x14d54fu,0x145200u,0u,kept);}   // MOV ESI,EDX; CALL 145200
    void object_pushed(std::uint32_t handle){put(0x14d557u,handle);}                                          // PUSH EAX
    bool buffered(Kept &kept) {                                                                          // LEA ESI,[ESP+48]; CALL 1F7E00; TEST AL,AL
        this->gp[esi]=local(logic::local::buffer);
        routed(0x14d55cu,0x1f7e00u,4u,kept);
        const bool done=answered();
        if(!done)this->reached(Branch::unbuffered);
        return done;
    }
    std::uint32_t shaped(Kept &kept,std::uint32_t key) {                                                 // PUSH EDX; MOV EAX,ESI; PUSH EAX; CALL 1F8040
        auto &gp=this->gp;
        put(0x14d56cu,key);gp[eax]=gp[esi];put(0x14d56fu,gp[eax]);
        routed(0x14d570u,0x1f8040u,8u,kept);
        return gp[eax];
    }
    // The part.
    std::uint32_t shape_reference(std::uint32_t shapes,std::uint32_t bsp){return this->gp[eax]=shapes+logic::shape_reference::size*bsp;}   // LEA EAX,[EDX+ECX*8]
    bool positive(std::uint32_t v) {                                                                     // TEST EDX,EDX; JLE
        this->test32(v);
        if(std::int32_t(v)<=0)this->reached(Branch::partless);
        return std::int32_t(v)>0;
    }
    std::uint32_t first_part(){return this->gp[ecx]=this->flags.logic(0u,4);}                                 // XOR ECX,ECX
    bool node_matches(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a==b;}                    // CMP EBX,EAX
    std::uint32_t next_part(std::uint32_t index) {                                                       // INC ECX
        this->reached(Branch::unmatched);
        return this->gp[ecx]=this->flags.increment(index,4,false);
    }
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP; JL / JGE
    bool above(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>std::int32_t(b);}   // CMP; JLE
    bool named(std::uint32_t v) {                                                                        // CMP reg,-1
        this->compare32(v,logic::none);
        if(v==logic::none)this->reached(Branch::unsurfaced);
        return v!=logic::none;
    }
    std::uint32_t part_offset(std::uint32_t index){return this->gp[ecx]=this->flags.shift(index,4u,4);}        // SHL ECX,4
    std::uint32_t part_address(std::uint32_t offset,std::uint32_t parts){return this->gp[ecx]=this->flags.plus(offset,parts,4);}   // ADD ECX,EBX
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);if(a!=b)this->reached(Branch::unkeyed);return a!=b;}   // CMP EDX,ESI
    // The leaf and its references.
    std::uint32_t leaf_address(std::uint32_t address){return this->gp[esi]=address;}                          // LEA ESI,[ESI+EDX*8]
    std::uint32_t leaf_end(std::uint32_t count,std::uint32_t first){return this->gp[esi]=this->flags.plus(count,first,4);}   // ADD ESI,reg
    bool non_negative(std::uint32_t index) {                                                             // TEST EDX,EDX; JL
        this->test32(index);
        if(std::int32_t(index)<0)this->reached(Branch::out_of_range);
        return std::int32_t(index)>=0;
    }
    std::uint32_t reference_address(std::uint32_t address){return this->gp[ebx]=address;}                    // LEA EBX,[ESI+EDX*8]
    std::uint32_t plane(std::uint32_t v){return this->gp[ecx]=this->flags.logic(v&logic::plane_mask,4);}        // AND ECX,7FFF
    bool on_plane(std::uint32_t word,std::uint32_t plane_value) {                                        // CMP word,CX
        this->flags.minus(word&0xffffu,plane_value&0xffffu,2);
        const bool on=(word&0xffffu)==(plane_value&0xffffu);
        if(!on)this->reached(Branch::off_plane);
        return on;
    }
    std::uint32_t vertex_list(std::uint32_t address){return this->gp[eax]=address;}                           // LEA EAX,[ECX+EAX*2]
    std::uint32_t first_vertex(){return this->gp[esi]=this->flags.logic(0u,4);}                               // XOR ESI,ESI
    std::uint32_t triangle_address(){return this->gp[ebx]=local(logic::local::triangle);}                     // LEA EBX,[ESP+44]
    void vertex_pushed(std::uint32_t index){put(0x14d68du,index);}                                            // PUSH EAX
    void vertex(Kept &kept,std::uint32_t) {                                                              // LEA EDI,[ESP+38]; MOV ECX,EBX; CALL 243F20
        auto &gp=this->gp;
        gp[edi]=local(logic::local::material);gp[ecx]=gp[ebx];
        routed(0x14d698u,0x243f20u,4u,kept);
    }
    std::uint32_t next_vertex(std::uint32_t index){return this->gp[esi]=this->flags.increment(index,4,false);}   // INC ESI
    std::uint32_t next_vertex_address(std::uint32_t address){return this->gp[ebx]=this->flags.plus(address,logic::vertex_size,4);}   // ADD EBX,0C
    bool vertices_left(std::uint32_t index){this->compare32(index,logic::triangle);return std::int32_t(index)<std::int32_t(logic::triangle);}   // CMP ESI,3; JL
    bool inside(Kept &kept) {                                                                            // LEA ECX/EDX,[EBP+34/30]; PUSH x2; LEA ECX,EDX,EAX,ESI; CALL 1211C0; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.result+logic::result::second;put(0x14d6a9u,gp[ecx]);
        gp[edx]=kept.result+logic::result::first;put(0x14d6adu,gp[edx]);
        gp[ecx]=local(logic::local::triangle+2u*logic::vertex_size);gp[edx]=local(logic::local::triangle+logic::vertex_size);
        gp[eax]=local(logic::local::triangle);gp[esi]=local(logic::local::point);
        routed(0x14d6beu,0x1211c0u,8u,kept);
        const bool in=answered();
        this->reached(in?Branch::hit:Branch::missed);
        return in;
    }
    // The section.
    std::uint32_t first_section(){return this->gp[edx]=this->flags.logic(0u,4);}                              // XOR EDX,EDX
    std::uint32_t section_cursor(std::uint32_t list){return this->gp[eax]=this->flags.plus(list,logic::section::first,4);}   // ADD EAX,6
    std::uint32_t section_end(std::uint32_t size,std::uint32_t start) {                                  // ADD EBX,ECX
        this->reached(Branch::sectioned);
        return this->gp[ebx]=this->flags.plus(size,start,4);
    }
    std::uint32_t next_section(std::uint32_t index){return this->gp[edx]=this->flags.increment(index,4,false);}   // INC EDX
    std::uint32_t next_section_cursor(std::uint32_t cursor){return this->gp[eax]=this->flags.plus(cursor,logic::section::size,4);}   // ADD EAX,48
    std::uint32_t next_reference(std::uint32_t index){return this->gp[edx]=this->flags.increment(index,4,false);}   // INC EDX
    void finish(Exit exit) {                                                                             // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,58; RET C
        auto &gp=this->gp;
        const auto pc=epilogues[unsigned(exit)];
        gp[edi]=this->pop(pc);gp[esi]=this->pop(pc+1u);gp[ebp]=this->pop(pc+2u);gp[ebx]=this->pop(pc+3u);
        gp[esp]=this->flags.plus(gp[esp],0x58u,4);
        this->ret(pc+7u,0xcu);this->publish(pc+7u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::object_point_surface::find(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x14d410u,0x14d783u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_object_point_surface_detail
