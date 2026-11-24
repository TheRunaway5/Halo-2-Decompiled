#pragma once
// Boundary adapter for the rigid body build, 1DDEA0
// (native_tu15_rigid_body_build_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the type table, eight
// arguments on the stack (RET 20), AL the answer; an EBP frame, ESP aligned
// down to 16 below 0xF4 bytes, EBX, ESI and EDI saved below it, the frame
// read from the ESP each child left; 1DE170 (15 arguments, RET 3C), 1DEC80
// (2, RET 8), 1DF380 (14, RET 38) and 1DF5B0 (2, RET 8) direct, 283450 (the
// shape pushed, the caller cleaning) and the shape's destructor (vtable +0,
// ECX the shape, 1 pushed, RET 4) through the unit's dispatch, every
// register each returns coming back. Each argument is pushed from the
// register the original pushes it from, at its instruction. XMM0 to XMM3
// hold what the matrix's moves leave in them; the shape info block is
// copied as REP MOVSD then REP MOVSB in either direction. Each access
// publishes its original instruction.
// HALO2_REFERENCE_RIGID_BODY_BUILD=1 restores the translated body.
// HALO2_VERIFY_RIGID_BODY_BUILD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_rigid_body_build_systems.hpp"

namespace halo2::native_rigid_body_build_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_rigid_body_build;
using Access=logic::rigid_body_build::Access;
using Kept=logic::rigid_body_build::Kept;
using Outcome=logic::rigid_body_build::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RIGID_BODY_BUILD");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RIGID_BODY_BUILD");return v;}
enum Branch : unsigned {failed, built, marked, own_matrix, flagged, released, destroyed, scaled, branch_count};
static_assert(unsigned(Outcome::marked)==Branch::marked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Rigid body build"};return v;}
inline constexpr unsigned none_register=8u;
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// Each pushed argument: its PUSH and the register it is pushed from.
struct Pass {std::uint32_t pc;unsigned reg;};
inline constexpr Pass passes[]{
    {0x1ddeb8u,eax},{0x1ddebdu,ecx},{0x1ddebeu,edx},{0x1ddec3u,eax},{0x1ddecbu,ecx},{0x1dded6u,edx},{0x1ddedeu,eax},{0x1ddee2u,ecx},
    {0x1ddee3u,edx},{0x1ddee4u,eax},{0x1ddeecu,ecx},{0x1ddef7u,edx},{0x1ddef8u,edi},{0x1ddef9u,eax},{0x1ddefcu,ecx},    // 1DE170
    {0x1ddf59u,edx},{0x1ddf5au,eax},                                                                                     // 1DEC80
    {0x1de085u,ecx},{0x1de089u,ecx},{0x1de096u,edx},{0x1de09bu,edx},{0x1de0a0u,ecx},{0x1de0a1u,edi},{0x1de0a2u,edx},{0x1de0aau,ecx},
    {0x1de0abu,none_register},{0x1de0adu,none_register},{0x1de0b9u,edx},{0x1de0bau,eax},{0x1de0cbu,ecx},{0x1de0ccu,esi}};      // 1DF380
inline constexpr std::uint32_t forwarded_sites[]{0x1ddeacu,0x1ddeccu,0x1dded7u,0x1ddedfu,0x1ddee5u,0x1ddeedu};
inline constexpr unsigned forwarded_registers[]{edx,ecx,edx,eax,eax,ecx};
inline constexpr std::uint32_t component_loads[]{0x1ddf84u,0x1ddf89u,0x1ddf8eu,0x1ddfaeu,0x1ddfb3u,0x1ddfb8u,0x1ddfd5u,0x1ddfdau,0x1ddfdfu,
    0x1ddffcu,0x1de007u,0x1de012u};
inline constexpr unsigned component_registers[]{0,1,2,1,2,3,1,2,3,1,1,1};
inline constexpr std::uint32_t component_stores[]{0x1ddf93u,0x1ddf9cu,0x1ddfa2u,0x1ddfa8u,0x1ddfbdu,0x1ddfc3u,0x1ddfc9u,0x1ddfcfu,0x1ddfe4u,
    0x1ddfeau,0x1ddff0u,0x1ddff6u,0x1de001u,0x1de00cu,0x1de017u,0x1de01du};
inline constexpr std::uint32_t sites[]{0u,0x1ddefdu,0x1ddf05u,0x1ddf09u,0x1ddf0eu,0x1ddf21u,0x1ddf29u,0x1ddf2cu,0x1ddf2fu,0x1ddf38u,0x1ddf3cu,
    0x1ddf4du,0x1ddf51u,0x1ddf54u,0x1ddf56u,0x1ddf5bu,0x1ddf5eu,0x1ddf67u,0x1ddf6du,0x1ddf73u,0x1ddf7bu,0x1ddf81u,0u,0u,0x1de023u,0x1de028u,
    0x1de02du,0x1de036u,0x1de039u,0x1de03cu,0x1de048u,0x1de04du,0x1de050u,0x1de054u,0x1de05fu,0x1de067u,0x1de06du,0x1de082u,0x1de086u,
    0x1de097u,0x1de09cu,0x1de0bbu,0x1de0bfu,0x1de0c3u,0x1de0d2u,0x1de0d2u,0x1de0d6u,0x1de0ecu,0x1de0f4u,0x1de0fau,0x1de0fcu,0x1de110u,
    0x1de113u,0x1de117u,0x1de12au,0x1de12au,0x1de131u,0x1de131u,0x1de133u,0x1de137u,0x1de13au,0x1de140u,0x1de147u,0x1de14au,0x1de14au};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::flags_store)+1u);
template<class State,class Shape,class Value,class Create,class Scale,class Dispatch> struct Adapter : Port<State> {
    Shape &shape_child;Value &value_child;Create &create_child;Scale &scale_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers
    unsigned passed{},forwards{},components{},component_puts{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Shape &a,Value &b,Create &d,Scale &e,Dispatch &f)
        :Port<State>(s,c,t,{}),shape_child(a),value_child(b),create_child(d),scale_child(e),dispatch(f){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t argument(unsigned n)const{return this->gp[ebp]+8u+4u*n;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void refresh(Kept &kept) {
        auto &gp=this->gp;
        kept.entry=kept.object=gp[ebx];kept.shape=gp[esi];kept.types=kept.type=kept.body=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case forwarded:return forwarded_sites[forwards];
        case component:return component_loads[components];
        case component_store:return component_stores[component_puts++];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case forwarded:gp[forwarded_registers[forwards++]]=v;break;
        case node:gp[ecx]=sx8(v);break;
        case model_again:case entries_load:case fourth_value:case model_indexed:case saved_load:case value_load:case shape_value_load:
        case shape_vtable:case info_size_load:gp[edx]=v;break;
        case shapes_load:case fourth_again:case model_value:case object_value:case object_load:case handle_load:case headers_load:
        case datums_load:case shapes_again:case record_load:gp[eax]=v;break;
        case fixed_load:case scaled_load:set_low_byte(gp[eax],v);break;
        case entry_matrix:gp[ecx]=sx16(v);break;
        case model_own:case entry_second:case entry_first:case object_last:gp[ecx]=v;break;
        case component:this->s.xmm[component_registers[components++]]=lane0(v);break;                 // MOVSS
        case entry_flags:set_low_word(gp[edx],v);break;
        case type_load:gp[edi]=v;break;
        case object_type:case second_load:case first_load:set_low_byte(gp[ecx],v);break;
        case object_again:gp[ebx]=v;break;
        default:break;
        }
    }
    void loaded_vector(Access,std::uint32_t,__m128 v){this->s.xmm[0]=v;}                               // MOVAPS XMM0
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::info_word_store){gp[esi]+=4u*step();gp[edi]+=4u*step();--gp[ecx];}               // each MOVSD step
        if(a==Access::info_byte_store){gp[esi]+=step();gp[edi]+=step();--gp[ecx];}                     // each MOVSB step
    }
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,F4
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x1ddea0u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff0u,4),logic::local::size,4);
    }
    void saving(const Kept &kept) {                                                                     // PUSH EBX; PUSH ESI; PUSH EDI; MOV EDI,EAX
        auto &gp=this->gp;
        this->push(0x1ddeafu,gp[ebx]);this->push(0x1ddeb0u,gp[esi]);this->push(0x1ddeb1u,gp[edi]);depth=0u;
        gp[edi]=kept.types;
    }
    void pass(std::uint32_t v) {
        const auto &p=passes[passed++];
        if(p.reg!=none_register)this->gp[p.reg]=v;
        put(p.pc,v);
    }
    void cleared(){set_low_byte(this->gp[ebx],this->flags.logic(0u,1));}                               // XOR BL,BL
    std::uint32_t shape_built(Kept &kept) {                                                              // CALL 1DE170; MOV ESI,EAX; TEST ESI,ESI
        auto &gp=this->gp;
        this->call(0x1ddf12u,0x1ddf17u,0x1de170u,shape_child);depth-=0x3cu;
        refresh(kept);
        gp[esi]=gp[eax];this->test32(gp[esi]);
        return gp[esi];
    }
    void failed(){set_low_byte(this->gp[eax],this->gp[ebx]);}                                          // MOV AL,BL
    std::uint32_t node_offset(std::uint32_t node_index) {                                               // LEA EBX,[ECX+ECX*8]; SHL EBX,4
        static_assert((9u<<4)==logic::entry::size);
        return this->gp[ebx]=this->flags.shift(9u*sx8(node_index),4u,4);
    }
    std::uint32_t entry(std::uint32_t offset,std::uint32_t entries,std::uint32_t fixed) {              // ADD EBX,EDX; TEST AL,AL
        this->gp[ebx]=this->flags.plus(offset,entries,4);this->test8(fixed);
        return this->gp[ebx];
    }
    std::uint32_t node(std::uint32_t node_index)const{return sx8(node_index);}
    std::uint32_t typed(std::uint32_t type){return this->gp[edi]=sx8(type);}                           // MOV EDI,7 / MOVSX EDI,byte
    std::uint32_t valued(std::uint32_t,Kept &kept) {                                                    // CALL 1DEC80
        this->call(0x1ddf62u,0x1ddf67u,0x1dec80u,value_child);depth-=8u;
        refresh(kept);
        return this->gp[eax];
    }
    void matrix_compared(std::uint32_t index){this->compare32(sx16(index),0xffffffffu);}               // CMP ECX,-1
    std::uint32_t own_matrix(std::uint32_t model) {                                                     // ADD ECX,8
        this->reached(Branch::own_matrix);
        return this->gp[ecx]=this->flags.plus(model,logic::model::matrix,4);
    }
    std::uint32_t indexed_matrix(std::uint32_t index,std::uint32_t matrices) {                         // IMUL ECX,ECX,34; ADD ECX,[EDX+50]
        auto &fl=this->flags;
        return this->gp[ecx]=fl.plus(fl.product(sx16(index),logic::model::matrix_size),matrices,4);
    }
    void zeroed(){this->s.xmm[0]=_mm_setzero_ps();}                                                     // XORPS XMM0,XMM0
    std::uint32_t flag_tested(std::uint32_t word) {                                                     // TEST DL,1
        this->flags.logic(word&1u,1);
        if(word&1u)this->reached(Branch::flagged);
        return word;
    }
    std::uint32_t handle_index(std::uint32_t handle) {                                                  // AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle&logic::handle_mask,4);gp[ecx]=3u*gp[eax];
        return gp[eax];
    }
    std::uint32_t saved()const{return this->gp[edx];}
    std::uint32_t type_bit(std::uint32_t type){return this->gp[edx]=this->flags.shift(1u,type&0xffu,4);}  // MOV EDX,1; SHL EDX,CL
    void type_tested(std::uint32_t bit){this->flags.logic(bit&logic::marked_types&0xffu,1);}            // TEST DL,2
    void restored(std::uint32_t){}
    std::uint32_t fixed(std::uint32_t type) {                                                           // CMP EDI,7; CMP EDI,6; MOV AL,1 / XOR AL,AL
        auto &gp=this->gp;
        this->compare32(type,logic::fixed_type);
        if(type==logic::fixed_type){set_low_byte(gp[eax],1u);return gp[eax];}
        this->compare32(type,logic::static_type);
        set_low_byte(gp[eax],type==logic::static_type?1u:this->flags.logic(0u,1));
        return gp[eax];
    }
    std::uint32_t halved_flags() {                                                                       // SHR DL,1; LEA ECX,[ESP+48]; AND EDX,FFFFFF01
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.right(gp[edx]&0xffu,1u,1,false));
        gp[ecx]=local(logic::local::transform);
        return gp[edx]=fl.logic(gp[edx]&0xffffff01u,4);
    }
    void count_compared(std::uint32_t count){this->compare32(count,1u);}                               // CMP [EAX+38],1
    std::uint32_t single() {                                                                             // SETZ CL; MOV EAX,EBX
        auto &gp=this->gp;
        set_low_byte(gp[ecx],this->flags.zf?1u:0u);gp[eax]=gp[ebx];
        return gp[ecx];
    }
    std::uint32_t created(Kept &kept) {                                                                  // CALL 1DF380; MOV EDI,EAX
        auto &gp=this->gp;
        this->call(0x1de0cdu,0x1de0d2u,0x1df380u,create_child);depth-=0x38u;
        refresh(kept);
        return kept.body=gp[edi]=gp[eax];
    }
    std::uint32_t released(std::uint32_t references) {                                                  // DEC word [ESI+6]
        const auto left=this->flags.increment(references&0xffffu,2,true)&0xffffu;
        if(!left)this->reached(Branch::released);
        return left;
    }
    void references_tested(std::uint32_t references){this->flags.minus(references&0xffffu,0u,2);}     // CMP word [ESI+6],0
    std::uint32_t kept_static(Kept &kept) {                                                              // PUSH ESI; CALL 283450; ADD ESP,4; TEST AL,AL
        auto &gp=this->gp;
        put(0x1de0dfu,kept.shape);
        auto child=[&](State &v){dispatch(v,0x283450u,0x1de0e0u);};
        this->call(0x1de0e0u,0x1de0e5u,0x283450u,child);
        refresh(kept);
        gp[esp]=this->flags.plus(gp[esp],4u,4);depth-=4u;
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void destructor_argument(const Kept &kept){put(0x1de0f6u,1u);this->gp[ecx]=kept.shape;}           // PUSH 1; MOV ECX,ESI
    void destroyed(std::uint32_t target,Kept &kept) {                                                    // CALL [EDX]
        auto child=[&](State &v){dispatch(v,target,0x1de0fau);};
        this->call(0x1de0fau,0x1de0fcu,target,child);depth-=4u;
        refresh(kept);
        this->reached(Branch::destroyed);
    }
    void byte_tested(std::uint32_t v){this->test8(v);}
    std::uint32_t scaled(Kept &kept) {                                                                   // PUSH 3F59999A; PUSH EBX; CALL 1DF5B0; MOV EDI,EAX
        auto &gp=this->gp;
        put(0x1de103u,logic::scaling);put(0x1de108u,kept.object);
        this->call(0x1de109u,0x1de10eu,0x1df5b0u,scale_child);depth-=8u;
        refresh(kept);
        this->reached(Branch::scaled);
        return kept.body=gp[edi]=gp[eax];
    }
    std::uint32_t copying(std::uint32_t size,std::uint32_t from,std::uint32_t to) {                    // MOV ECX,EDX; MOV EBX,ECX; SHR ECX,2; LEA EDI; LEA ESI
        auto &gp=this->gp;
        gp[ebx]=size;gp[ecx]=this->flags.right(size,2u,4,false);gp[edi]=to;gp[esi]=from;
        return from;
    }
    void bytes(std::uint32_t size){this->gp[ecx]=this->flags.logic(size&3u,4);}                         // MOV ECX,EBX; AND ECX,3
    void succeeded(){set_low_byte(this->gp[eax],1u);}                                                   // MOV AL,1
    std::uint32_t ored(std::uint32_t v,std::uint32_t bit){return this->flags.logic(v|bit,4);}           // OR [ECX+4],4000
    bool exit_marked{};
    void restoring(bool marked_exit) {                                                                   // POP EDI; POP ESI; POP EBX; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        const std::uint32_t base=marked_exit?0x1de151u:0x1de15cu;
        gp[edi]=this->pop(base);gp[esi]=this->pop(base+1u);gp[ebx]=this->pop(base+2u);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(base+5u);
        exit_marked=marked_exit;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::rigid_body_build::build(memory,*this,Kept{gp[ebx],gp[ebx],gp[esi],gp[eax],gp[eax],gp[eax]});
        this->reached(unsigned(outcome));
        const std::uint32_t exit=exit_marked?0x1de157u:0x1de162u;
        this->ret(exit,0x20u);this->publish(exit);
    }
};
template<class State,class Reference,class Shape,class Value,class Create,class Scale,class Dispatch>
bool bridge(State &s,Reference reference,Shape shape,Value value,Create create,Scale scale,Dispatch dispatch) {
    return run_owner(s,0x1ddea0u,0x1de162u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,Shape,Value,Create,Scale,Dispatch>(v,c,t,shape,value,create,scale,dispatch);});
}
} // namespace halo2::native_rigid_body_build_detail
