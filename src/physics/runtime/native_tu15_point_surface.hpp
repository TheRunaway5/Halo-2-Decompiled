#pragma once
// Boundary adapter for the point surface, 14D790
// (native_tu15_point_surface_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the record and the result
// on the stack, AL the answer (RET 8); 0x58 bytes of frame with EBX, EBP,
// ESI and EDI saved below them, read from the ESP each child left. Every
// child goes through the unit's dispatch, which binds it in each unit:
// 130980 (EAX 3, ECX the geometry), 145200 (EDX and ESI the point), 171010
// (EAX the projection's second word; its first and third pushed, RET 8) and 1211C0
// (EAX, EDX, ECX the vertices, ESI the point; two pushed, RET 8); every
// register each returns comes back. Each exit leaves through the original's
// own epilogue. Each access publishes its original instruction.
// HALO2_REFERENCE_POINT_SURFACE=1 restores the translated body.
// HALO2_VERIFY_POINT_SURFACE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_point_surface_systems.hpp"

namespace halo2::native_point_surface_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_point_surface;
using Access=logic::point_surface::Access;
using Exit=logic::point_surface::Exit;
using Kept=logic::point_surface::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POINT_SURFACE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POINT_SURFACE");return v;}
enum Branch : unsigned {structure_leaf, instance, leafless, accepted, plane_match, first_vertex, missing_vertex, triangle, found,
    searched, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Point surface"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{0x14d793u,0x14d79au,0x14d79cu,0x14d7a0u,0x14d7a3u,0x14d7a7u,0x14d7adu,0x14d7b1u,0x14d7bcu,
    0x14d7c0u,0x14d7c3u,0x14d7c7u,0x14d7cau,0x14d7d2u,0x14d7d7u,0x14d7dbu,0x14d7dfu,0x14d7e5u,0x14d7e8u,0x14d7f5u,0x14d803u,
    0x14d810u,0x14d813u,0x14d81eu,0x14d827u,0x14d82fu,0x14d839u,0x14d852u,0x14d85bu,0x14d87bu,0x14d87eu,0x14d882u,0x14d886u,
    0x14d88cu,0x14d890u,0x14d89du,0x14d8a0u,0x14d8a8u,0x14d8b2u,0x14d8bdu,0x14d8c2u,0x14d8cbu,0x14d8d6u,0x14d8dcu,0x14d8dfu,
    0x14d8e4u,0x14d8eau,0x14d8edu,0x14d8f7u,0x14d8fbu,0x14d8ffu,0x14d918u,0x14d91du,0x14d920u,0x14d923u,0x14d92cu,0x14d93au,
    0x14d93cu,0x14d940u,0x14d943u,0x14d946u,0x14d94au,0x14d94eu,0x14d955u,0x14d961u,0x14d964u,0x14d967u,0x14d971u,0x14d97fu,
    0x14d981u,0x14d985u,0x14d988u,0x14d98bu,0x14d98fu,0x14d993u,0x14d998u,0x14d99cu,0x14d99fu,0x14d9a8u,0x14d9b6u,0x14d9b8u,
    0x14d9bbu,0x14d9beu,0x14d9c2u,0x14d9c6u,0x14d9cau,0x14d9cfu,0x14d9f4u,0x14d9f8u,0x14d9fcu,0x14da00u,0x14da04u,0x14da0cu,
    0x14da16u,0x14da24u,0x14da28u,0x14da2cu,0x14da2fu,0x14da33u,0x14da37u,0x14da3au,0x14da3eu,0x14da41u,0x14da45u,0x14da48u,
    0x14da4bu,0x14da4du,0x14da50u,0x14da53u,0x14da58u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_load)+1u);
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes pushed below the saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        auto &gp=this->gp;
        kept.structure=gp[ebx];kept.leaf_address=gp[ebp];kept.node=gp[esi];kept.definition=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case record_load:case kind_load:case leaves_load:case leaf_count:case definition_leaves:case definition_surfaces:
        case surfaces_check:case instance_test:case structure_references:case definition_references:case reference_surface:
        case vertices_load:case third_x:case projection_x_load:case projection_z_load:case record_again:case leaf_again:gp[eax]=v;break;
        case surface_plane:gp[eax]=sx16(v);break;
        case third_count:gp[eax]=v&0xffffu;break;                                                          // MOVZX EAX,word
        case point_x:case point_y:case leaf_index_load:case structure_surfaces:case structure_surface_array:
        case definition_surface_array:case vertex_list_load:case first_x:case first_y:case second_offset:case third_offset:
        case second_z:case third_z:case projection_y_load:gp[edx]=v;break;
        case first_index:gp[edx]=v&0xffffu;break;                                                          // MOVZX EDX,word
        case node_word:case count_again:case reference_count:gp[edx]=sx16(v);break;                        // MOVSX EDX,word
        case point_z:case plane_load:case instance_load:case node_surfaces:case result_load:case first_reference:case first_offset:
        case first_z:case second_x:case second_y:case third_y:case index_reload:case leaf_last:case record_instance:gp[ecx]=v;break;
        case reference_vertices:case second_index:case third_index:case reference_extra:gp[ecx]=v&0xffffu;break;   // MOVZX ECX,word
        case structure_load:case surfaces_again:case result_again:case structure_reload:gp[ebx]=v;break;
        case first_count:case second_count:gp[ebx]=v&0xffffu;break;                                        // MOVZX EBX,word
        case node_index:gp[esi]=sx16(v);break;                                                             // MOVSX ESI,word
        case definitions_load:case first_again:gp[esi]=v;break;
        case instances_load:gp[edi]=v;break;
        case definition_index:gp[edi]=sx16(v);break;                                                       // MOVSX EDI,word
        case leaf_reload:gp[ebp]=v;break;
        case answer_final:case answer_load:set_low_byte(gp[eax],v);break;                                  // MOV AL
        case taken_test:set_low_byte(gp[ecx],v);break;                                                     // MOV CL
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame, the point and the leaf.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,58
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x14d7a6u,0x14d7bau,0x14d7bbu,0x14d7ccu};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t point_address(std::uint32_t record_address){return this->gp[ecx]=record_address+logic::record::point;}   // LEA ECX,[EAX+8]
    std::uint32_t plane(std::uint32_t v){return this->gp[ecx]=this->flags.logic(v&logic::plane_mask,4);}       // AND ECX,7FFF
    std::uint32_t no_definition(){return this->gp[edi]=this->flags.logic(0u,4);}                               // XOR EDI,EDI
    bool kind_is(std::uint32_t record_kind,std::uint32_t k){this->compare32(record_kind,k);return record_kind==k;}   // CMP EAX,k
    std::uint32_t leaf_address(std::uint32_t address){this->reached(Branch::structure_leaf);return this->gp[ebp]=address;}   // LEA EBP,[EAX+EDX*8]
    std::uint32_t node_address(std::uint32_t number,std::uint32_t nodes) {                              // IMUL ESI,ESI,B0; ADD ESI,[EBX+A0]
        auto &fl=this->flags;
        return this->gp[esi]=fl.plus(fl.product(number,logic::node_size),nodes,4);
    }
    void test_target(std::uint32_t target){auto &gp=this->gp;gp[ecx]=target;gp[eax]=logic::geometry_kind;}   // LEA ECX,[..+28]; MOV EAX,3
    bool accepted(Kept &kept,bool in_instance) {                                                         // CALL 130980; TEST AL,AL
        routed(in_instance?0x14d872u:0x14d807u,0x130980u,0u,kept);
        this->test8(this->gp[eax]);
        if(low_byte(this->gp[eax]))this->reached(Branch::accepted);
        return low_byte(this->gp[eax])!=0u;
    }
    std::uint32_t instance_offset(std::uint32_t index) {                                                 // IMUL ECX,ECX,58; ADD ECX,EDI
        auto &gp=this->gp;auto &fl=this->flags;
        const auto offset=fl.product(index,logic::instance_size);
        gp[ecx]=fl.plus(offset,gp[edi],4);
        this->reached(Branch::instance);
        return offset;
    }
    std::uint32_t definition_offset(std::uint32_t number){return this->gp[edi]=this->flags.product(number,logic::definition_size);}   // IMUL EDI,EDI,C8
    std::uint32_t definition_address(std::uint32_t offset,std::uint32_t definitions){return this->gp[edi]=this->flags.plus(offset,definitions,4);}   // ADD EDI,ESI
    bool has_leaves(std::uint32_t count){this->test32(count);if(!count)this->reached(Branch::leafless);return count!=0u;}   // TEST EAX,EAX
    std::uint32_t instance_leaf(std::uint32_t leaves,std::uint32_t leaf_index){return this->gp[edx]=leaves+logic::leaf_size*leaf_index;}   // LEA EDX,[EAX+EDX*8]
    void into_instance(Kept &kept) {                                                                     // LEA EDX,[ESP+38]; MOV ESI,EDX; CALL 145200
        auto &gp=this->gp;
        gp[esi]=gp[edx]=local(logic::local::point);
        routed(0x14d865u,0x145200u,0u,kept);
    }
    // The references.
    bool nonzero(std::uint32_t v){this->test32(v);return v!=0u;}                                             // TEST EAX,EAX
    std::uint32_t reference_end(std::uint32_t count,std::uint32_t index){return this->gp[edx]=this->flags.plus(count,index,4);}   // ADD EDX,ECX
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}  // CMP; JGE
    std::uint32_t no_index(){return this->gp[edx]=this->flags.logic(logic::none,4);}                          // OR EDX,-1
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                       // CMP
    std::uint32_t reference_address(std::uint32_t references,std::uint32_t index){return this->gp[ebp]=references+logic::reference_size*index;}   // LEA EBP,[EAX+ECX*8]
    std::uint32_t projecting(Kept &kept,std::uint32_t list) {                                            // LEA EAX,[ESP+2C]; LEA ESI; PUSH EAX; LEA EDX,[ESP+38]; PUSH EDX; LEA EAX,[ESP+38]; CALL 171010
        auto &gp=this->gp;
        this->reached(Branch::plane_match);
        gp[eax]=local(logic::local::projection);gp[esi]=list;put(0x14d909u,gp[eax]);
        gp[edx]=local(logic::local::projection+8u);put(0x14d90eu,gp[edx]);gp[eax]=local(logic::local::projection+4u);
        routed(0x14d913u,0x171010u,8u,kept);
        return gp[esi];
    }
    std::uint32_t surfaces_copy(std::uint32_t surfaces_address){return this->gp[eax]=surfaces_address;}      // MOV EAX,EBX
    std::uint32_t vertex_base(unsigned k,std::uint32_t base,std::uint32_t extra) {                       // ADD ECX/EDX,[EAX+8]
        return this->gp[k?edx:ecx]=this->flags.plus(base,extra,4);
    }
    bool in_range(std::uint32_t index){this->test32(index);return std::int32_t(index)>=0;}                   // TEST reg,reg; JL
    std::uint32_t vertex_offset(std::uint32_t index,unsigned k) {                                        // LEA i,[i+i*2]; LEA base,[base+i*4]
        auto &gp=this->gp;
        if(k==0u){gp[edx]=3u*index;gp[ecx]+=4u*gp[edx];this->reached(Branch::first_vertex);}
        else{gp[ecx]=3u*index;gp[edx]+=4u*gp[ecx];}
        return index;
    }
    bool set(std::uint32_t v){this->test8(v);if(!(v&0xffu))this->reached(Branch::missing_vertex);return (v&0xffu)!=0u;}   // TEST CL,CL
    bool inside(Kept &kept,std::uint32_t result_address) {                                              // LEA/PUSH x2; LEA ECX/EDX/EAX/ESI; CALL 1211C0; TEST AL,AL
        auto &gp=this->gp;
        this->reached(Branch::triangle);
        gp[eax]=result_address+logic::result::second;put(0x14d9d6u,gp[eax]);
        gp[ecx]=result_address+logic::result::first;put(0x14d9dau,gp[ecx]);
        gp[ecx]=local(logic::local::third);gp[edx]=local(logic::local::second);gp[eax]=local(logic::local::first);gp[esi]=local(logic::local::point);
        routed(0x14d9ebu,0x1211c0u,8u,kept);
        this->test8(gp[eax]);
        if(low_byte(gp[eax]))this->reached(Branch::found);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t next_index(std::uint32_t index){return this->gp[ecx]=this->flags.increment(index,4,false);}   // INC ECX
    std::uint32_t reference_end_again(std::uint32_t count,std::uint32_t first){return this->gp[edx]=this->flags.plus(count,first,4);}   // ADD EDX,ESI
    void finish(Exit exit) {
        auto &gp=this->gp;
        static constexpr std::uint32_t pops[3][5]{{0x14d846u,0x14d847u,0x14d848u,0x14d84bu,0x14d84fu},
            {0x14da5cu,0x14da5du,0x14da5eu,0x14da5fu,0x14da63u},{0x14da1au,0x14da1bu,0x14da1cu,0x14da1du,0x14da21u}};
        const auto &p=pops[unsigned(exit)];
        gp[edi]=this->pop(p[0]);gp[esi]=this->pop(p[1]);gp[ebp]=this->pop(p[2]);
        if(exit==Exit::leafless)set_low_byte(gp[eax],this->flags.logic(0u,1));                            // XOR AL,AL
        if(exit==Exit::searched)this->reached(Branch::searched);
        gp[ebx]=this->pop(p[3]);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
        this->ret(p[4],8u);this->publish(p[4]);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::point_surface::find(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x14d790u,0x14da63u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_point_surface_detail
