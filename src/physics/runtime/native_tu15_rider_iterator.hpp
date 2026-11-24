#pragma once
// Boundary adapter for the rider iterator, D2BD0
// (native_tu15_rider_iterator_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EDX the iterator, AL the answer (RET 0); ECX
// pushed for the answer byte, then EBX, EBP, ESI and EDI. The routine calls
// no child. Each exit leaves through the original's epilogue (the found
// unit's among its last stores). Each access publishes its original
// instruction.
// HALO2_REFERENCE_RIDER_ITERATOR=1 restores the translated body.
// HALO2_VERIFY_RIDER_ITERATOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_rider_iterator_systems.hpp"

namespace halo2::native_rider_iterator_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_rider_iterator;
using Access=logic::rider_iterator::Access;
using Exit=logic::rider_iterator::Exit;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RIDER_ITERATOR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RIDER_ITERATOR");return v;}
// (The answer byte is set only from the root, whose walk ends where it starts: its early test never answers.)
enum Branch : unsigned {started, riding, walked_none, found, ended, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Rider iterator"};return v;}
inline constexpr std::uint32_t sites[]{
    0xd2bd1u,0xd2bd3u,0xd2bd7u,0xd2be0u,0xd2bebu,0xd2bf1u,0xd2bffu,0xd2c02u,0xd2c06u,0xd2c09u,0xd2c0cu,0xd2c21u,0xd2c27u,0xd2c29u,
    0xd2c35u,0xd2c39u,0xd2c4cu,0xd2c50u,0xd2c61u,0xd2c6cu,0xd2c77u,0xd2c7cu,0xd2c88u,0xd2c8cu,0xd2c8fu,0xd2c92u,0xd2c95u,0xd2ca8u,
    0xd2cb0u,0xd2cbeu,0xd2cc1u,0xd2cc5u,0xd2cc8u,0xd2ccbu,0xd2ce0u,0xd2ceau,0xd2cf2u,0xd2cf5u,0xd2d01u,0xd2d05u,0xd2d1bu,0xd2d1fu,
    0xd2d30u,0xd2d3bu,0xd2d48u,0xd2d4du,0xd2d59u,0xd2d5du,0xd2d60u,0xd2d63u,0xd2d6cu,0xd2d76u,0xd2d79u,0xd2d80u,0xd2d84u,0xd2d87u,
    0xd2d8cu,0xd2d94u,0xd2d97u,0xd2d9fu,0xd2da3u,0xd2daau};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::found_object_store_next)+1u);
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};   // bytes pushed beyond ECX, EBX, EBP, ESI and EDI (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case root_load:case sibling_load:case root_object:case current_object:case current_load:case next_sibling:
        case next_pending_again:case sibling_current:case next_current_object:case found_object_load:gp[eax]=v;break;
        case found_test:case answer_load:set_low_byte(gp[eax],v);break;                                    // MOV AL
        case data_load:case root_reload:case child_flags:case data_current:case current_child:case next_data:case current_reload:
        case sibling_flags:case next_data_current:case next_current_child:case found_object_next:gp[ecx]=v;break;
        case type_load:case child_type:case next_type:case sibling_type:set_low_byte(gp[ecx],v);break;     // MOV CL
        case word_load:case word_load_next:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);break;                 // MOV CX,word
        case header_load:gp[ebx]=v;break;
        case data_root:case sibling_data:gp[ebp]=v;break;
        case object_load:case first_child:case child_sibling:case next_object:case current_sibling:case sibling_next:gp[esi]=v;break;
        case pending_load:case child_object:case next_pending:case sibling_object:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void reserving(){this->push(0xd2bd0u,this->gp[ecx]);depth=0u-16u;}                                    // PUSH ECX
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0xd2bd6u,0xd2bddu,0xd2bdeu,0xd2bdfu};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    bool same(std::uint32_t a,std::uint32_t b){this->compare32(a,b);if(a==b)this->reached(Branch::started);return a==b;}   // CMP
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg/[mem],-1 (or EDI)
    std::uint32_t ending_none(std::uint32_t v){this->gp[edi]=this->flags.logic(logic::none,4);return v;}      // OR EDI,-1
    // The pending children.
    std::uint32_t pending_index(std::uint32_t handle) {                                                  // MOV ECX,EDI; AND ECX,FFFF; LEA EAX,[ECX+ECX*2]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);gp[eax]=3u*gp[ecx];
        return gp[ecx];
    }
    bool unit(std::uint32_t) {                                                                           // MOV EAX,1; SHL EAX,CL; TEST AL,3
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.shift(1u,gp[ecx]&0xffu,4);
        return fl.logic(gp[eax]&logic::object::unit_types,1)!=0u;
    }
    bool pending_left(std::uint32_t v,bool first) {                                                      // CMP [EDX+8],-1 / OR EDI,-1; CMP EAX,EDI
        if(!first)this->gp[edi]=this->flags.logic(logic::none,4);
        return named(v);
    }
    // The chains.
    std::uint32_t root_index(std::uint32_t handle) {                                                     // AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[ecx]=3u*index;
        return index;
    }
    std::uint32_t child_index(std::uint32_t child) {                                                     // MOV ECX,ESI; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        const auto index=this->flags.logic(child&logic::handle_mask,4);
        this->gp[ecx]=3u*index;
        return index;
    }
    bool riding(std::uint32_t flags_value) {                                                             // SHR ECX,1A; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.right(flags_value,logic::object::riding_bit,4,false);
        const bool is=fl.logic(gp[ecx]&1u,1)!=0u;
        if(is)this->reached(Branch::riding);
        return is;
    }
    bool chain_end(std::uint32_t child,bool siblings) {                                                  // (OR EDI,-1); CMP ESI,EDI / CMP ESI,-1
        if(siblings)this->gp[edi]=this->flags.logic(logic::none,4);
        const bool any=named(child);
        if(!any)this->reached(Branch::walked_none);
        return any;
    }
    std::uint32_t current_index(std::uint32_t child) {                                                   // AND ESI,FFFF; LEA EAX,[ESI+ESI*2]
        auto &gp=this->gp;
        gp[esi]=this->flags.logic(child&logic::handle_mask,4);gp[eax]=3u*gp[esi];
        return gp[esi];
    }
    bool answered(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                    // TEST AL,AL
    void leaving_found(unsigned step) {                                                                  // POP EDI / POP ESI; POP EBP; MOV AL,1
        auto &gp=this->gp;
        if(step==1u){gp[edi]=this->pop(0xd2d9eu);return;}
        gp[esi]=this->pop(0xd2da6u);gp[ebp]=this->pop(0xd2da7u);set_low_byte(gp[eax],1u);
    }
    void finish(Exit exit) {
        auto &gp=this->gp;
        if(exit==Exit::found) {                                                                          // POP EBX; POP ECX; RET
            this->reached(Branch::found);
            gp[ebx]=this->pop(0xd2dadu);gp[ecx]=this->pop(0xd2daeu);
            this->ret(0xd2dafu,0u);this->publish(0xd2dafu);
            return;
        }
        this->reached(Branch::ended);
        gp[edi]=this->pop(0xd2d70u);gp[esi]=this->pop(0xd2d71u);gp[ebp]=this->pop(0xd2d72u);gp[ebx]=this->pop(0xd2d73u);   // POP EDI..ECX; RET
        gp[ecx]=this->pop(0xd2d74u);
        this->ret(0xd2d75u,0u);this->publish(0xd2d75u);
    }
    void run() {
        Memory<Adapter> memory{*this};
        finish(logic::rider_iterator::next(memory,*this,this->gp[edx]));
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0xd2bd0u,0xd2dafu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_rider_iterator_detail
