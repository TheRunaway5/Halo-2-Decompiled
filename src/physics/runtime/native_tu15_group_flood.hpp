#pragma once
// Boundary adapter for the group flood, 1DE8E0
// (native_tu15_group_flood_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the object and twelve
// arguments on the stack (RET 30); 0x18 bytes of frame with EBX and ESI
// (and, past an empty group, EDI and EBP) saved below them, read from the
// ESP each child left. DDA90 and DDA10 (EAX the object's +8; two pushed,
// RET 8) and this routine itself (the flood of an other group; twelve
// pushed, RET 30) are direct; every register each returns comes back. Each
// exit leaves through the original's epilogue. Each access publishes its
// original instruction.
// HALO2_REFERENCE_GROUP_FLOOD=1 restores the translated body.
// HALO2_VERIFY_GROUP_FLOOD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_group_flood_systems.hpp"

namespace halo2::native_group_flood_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_group_flood;
using Access=logic::group_flood::Access;
using Exit=logic::group_flood::Exit;
using Kept=logic::group_flood::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_GROUP_FLOOD");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_GROUP_FLOOD");return v;}
enum Branch : unsigned {empty, root, indexed_member, listed_index, visited_mark, touching, kind_six, visited_skip, pair_failed,
    closed, flooded_other, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Group flood"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline constexpr std::uint32_t sites[]{
    0x1de8e3u,0x1de8eau,0x1de8efu,0x1de8fau,0x1de8feu,0x1de904u,0x1de90cu,0x1de916u,0x1de92au,0x1de938u,0x1de948u,0x1de956u,
    0x1de95du,0x1de961u,0x1de964u,0x1de973u,0x1de977u,0x1de97eu,0x1de98bu,0x1de99fu,0x1de9afu,0x1de9b3u,0x1de9b7u,0x1de9b9u,
    0x1de9bdu,0x1de9c0u,0x1de9c5u,0x1de9c7u,0x1de9cdu,0x1de9cdu,0x1de9d8u,0x1de9d8u,0x1de9dfu,0x1de9e3u,0x1de9f2u,0x1dea0au,
    0x1dea0au,0x1dea0cu,0x1dea10u,0x1dea13u,0x1dea1au,0x1dea24u,0x1dea30u,0x1dea33u,0x1dea43u,0x1dea4au,0x1dea53u,0x1dea5eu,
    0x1dea6eu,0x1dea78u,0x1dea87u,0x1dea95u,0x1dea9cu,0x1deaa2u,0x1deaa6u,0x1deab0u,0x1deac5u,0x1dead4u,0x1deae0u,0x1deae3u,
    0x1deae7u,0x1deaecu,0x1deaefu,0x1deb00u,0x1deb04u,0x1deb07u,0x1deb0bu,0x1deb1du,0x1deb28u,0x1deb2cu,0x1deb30u,0x1deb34u,
    0x1deb38u,0x1deb3cu,0x1deb3fu,0x1deb48u,0x1deb4cu,0x1deb60u,0x1deb75u,0x1deb80u,0x1deb8cu,0x1deb99u,0x1deb9du,0x1deba1u,
    0x1deba7u,0x1debb0u,0x1debd0u,0x1debd3u,0x1debd7u,0x1debdcu,0x1debdfu,0x1debf0u,0x1debf4u,0x1debf7u,0x1debfbu,0x1dec09u,
    0x1dec19u,0x1dec1du,0x1dec25u,0x1dec29u,0x1dec2du,0x1dec31u,0x1dec3cu,0x1dec40u,0x1dec45u,0x1dec4au,0x1dec4fu,0x1dec54u,
    0x1dec59u,0x1dec5eu,0x1dec64u,0x1dec6bu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::object_argument)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // DDA90, DDA10 and this routine
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        auto &gp=this->gp;
        kept.groups_address=gp[ebx];kept.base=gp[ebp];kept.group=gp[esi];kept.counter=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case flags_load:case root_count:case group_count:case root_count_again:case group_count_again:case model_load:
        case list_count_pointer:case object_load:case model_again:case data_again:case pair_count:case object_inner:
        case object_pair_again:case object_inner_again:case pair_count_again:case model_next:case data_next:case group_size_other:
        case pair_count_other:case pair_count_b:case object_pair_b_again:case object_inner_b:case object_inner_b_again:
        case failed_pointer:case argument11:case argument9:case argument3:gp[eax]=v;break;
        case group_first:gp[eax]=sx8(v);break;                                                             // MOVSX EAX,byte
        case root_member:case group_member:case detached_load:case root_first:case group_first_member:set_low_byte(gp[eax],v);break;   // MOV AL
        case node_index:gp[eax]=low_word(gp[eax],v);break;                                                 // MOV AX,word
        case groups_load:case nodes_load:case groups_reload:case groups_reload_pairs:case groups_reload_b:gp[ebx]=v;break;
        case node_load:case list_count:case list_count_again:case node_reload:case pairs_load:case pairs_load_b:
        case node_reload_pairs:case node_reload_b:gp[esi]=v;break;
        case groups_again:case edge_count:case enabled_load:case group_size:case kinds_load:case edge_count_next:case argument12:
        case argument10:case argument8:case argument2:case object_argument:gp[ecx]=v;break;
        case pair_y:case pair_y_again:case pair_y_b:case pair_y_b_again:gp[ecx]=sx16(v);break;             // MOVSX ECX,word
        case other_first:set_low_byte(gp[ecx],v);break;                                                    // MOV CL
        case edge_a:case edge_b:gp[ecx]=low_word(gp[ecx],v);break;                                         // MOV CX,word
        case data_load:case node_count:case visited_load:case edge_index_load:case object_pair:case object_pair_b:case other_reload:
        case argument5:case argument1:gp[edx]=v;break;
        case edge_b_test:case pair_x:case pair_x_again:case pair_x_b:case pair_x_b_again:gp[edx]=sx16(v);break;   // MOVSX EDX,word
        case single_load:set_low_byte(gp[edx],v);break;                                                    // MOV DL
        case list_load:case visited_again:gp[ebp]=v;break;
        case edges_load:case edge_offset_load:case kinds_again:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the group.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,18
    std::uint32_t object()const{return this->gp[ecx];}
    std::uint32_t attachment(std::uint32_t flags_value){return this->gp[eax]=this->flags.right(flags_value,logic::object_layout::attached_bit,4,false);}   // SHR EAX,D
    void saving(unsigned n) {                                                                            // PUSH EBX / ESI / EDI / EBP
        static constexpr std::uint32_t pcs[]{0x1de8e9u,0x1de8eeu,0x1de921u,0x1de924u};
        static constexpr unsigned regs[]{ebx,esi,edi,ebp};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t detached(std::uint32_t) {                                                              // NOT AL; AND AL,1
        auto &gp=this->gp;
        set_low_byte(gp[eax],~gp[eax]);set_low_byte(gp[eax],this->flags.logic(gp[eax]&1u,1));
        return gp[eax]&0xffu;
    }
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                       // CMP
    std::uint32_t group_offset(std::uint32_t group){return this->gp[eax]=this->flags.product(group+1u,logic::group_size);}   // LEA EAX,[ESI+1]; IMUL EAX,EAX,44
    std::uint32_t group_offset_edx(std::uint32_t group){return this->gp[edx]=this->flags.product(group+1u,logic::group_size);}   // LEA EDX,[ESI+1]; IMUL EDX,EDX,44
    bool positive(std::uint32_t v){this->test32(v);return std::int32_t(v)>0;}                                // TEST; JLE
    bool nonzero(std::uint32_t v){this->test32(v);return v!=0u;}                                             // TEST; JNZ
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}  // CMP; JGE / JL
    // The members.
    std::uint32_t first_member(){return this->gp[edi]=this->flags.logic(0u,4);}                              // XOR EDI,EDI
    std::uint32_t next_member(std::uint32_t counter){return this->gp[edi]=this->flags.increment(counter,4,false);}   // INC EDI
    std::uint32_t member_slot(std::uint32_t group,std::uint32_t counter) {                              // IMUL ESI,ESI,44; ADD ESI,EDI
        auto &fl=this->flags;
        return this->gp[esi]=fl.plus(fl.product(group,logic::group_size),counter,4);
    }
    std::uint32_t member(std::uint32_t v){if(this->gp[esi]==logic::none)this->reached(Branch::root);return this->gp[ecx]=sx8(v);}   // MOVSX ECX,AL
    // The node: index*90 as LEA (x9) and SHL 4.
    static_assert(logic::node_size==9u<<4);
    std::uint32_t node_address(std::uint32_t member_index,std::uint32_t nodes) {                        // LEA EAX,[ECX+ECX*8]; SHL EAX,4; ADD EAX,EBX; MOV EBX,ECX; XOR CL,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.plus(fl.shift(9u*member_index,4u,4),nodes,4);gp[ebx]=member_index;
        set_low_byte(gp[ecx],fl.logic(0u,1));
        return gp[eax];
    }
    bool indexed(std::uint32_t v){const bool set_bit=this->flags.logic(v&logic::node::indexed_bit,1)!=0u;if(set_bit)this->reached(Branch::indexed_member);return set_bit;}   // TEST byte,20
    bool non_negative(std::uint32_t){this->flags.logic(this->gp[eax]&0xffffu,2);return std::int16_t(this->gp[eax])>=0;}   // TEST AX,AX; JGE
    std::uint32_t clamped(std::uint32_t,std::uint32_t count) {                                          // MOVSX ESI,AX; DEC EDX; CMP ESI,EDX; JG; MOV EDX,ESI
        auto &gp=this->gp;
        gp[esi]=sx16(gp[eax]);gp[edx]=this->flags.increment(count,4,true);this->compare32(gp[esi],gp[edx]);
        if(!(std::int32_t(gp[esi])>std::int32_t(gp[edx])))gp[edx]=gp[esi];
        return gp[edx];
    }
    std::uint32_t zero(){return this->gp[edx]=this->flags.logic(0u,4);}                                      // XOR EDX,EDX
    bool index_kept(std::uint32_t clamped_index,std::uint32_t index) {                                  // MOVSX ESI,AX; CMP EDX,ESI
        auto &gp=this->gp;
        gp[esi]=sx16(gp[eax]);this->compare32(clamped_index,gp[esi]);
        return clamped_index==index;
    }
    bool single_set(){set_low_byte(this->gp[edx],1u);return true;}                                         // MOV DL,1
    bool set(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                         // TEST AL,AL
    void listed_as_index(std::uint32_t){auto &gp=this->gp;set_low_byte(gp[ecx],gp[edx]);gp[ebx]=gp[esi];this->reached(Branch::listed_index);}   // MOV CL,DL; MOV EBX,ESI
    bool single_from(std::uint32_t v){return (v&0xffu)!=0u;}                                                // (MOV DL loaded)
    std::uint32_t listed()const{return this->gp[ebx]&0xffu;}                                                // BL
    std::uint32_t counted(std::uint32_t v,bool) {                                                        // INC ESI; TEST CL,CL
        auto &gp=this->gp;
        gp[esi]=this->flags.increment(v,4,false);this->test8(gp[ecx]);
        return gp[esi];
    }
    std::uint32_t marked(std::uint32_t v,std::uint32_t mask){return this->flags.logic(v|mask,4);}            // OR dword,mask
    bool single_tested(bool single){this->test8(this->gp[edx]);return single;}                              // TEST DL,DL
    // The visited set.
    std::uint32_t group_word(std::uint32_t group) {                                                      // MOV ECX,ESI; SAR ECX,5; LEA EAX,[EDX+ECX*4]
        auto &gp=this->gp;
        gp[ecx]=this->flags.right(group,5u,4,true);gp[eax]=gp[edx]+4u*gp[ecx];
        this->reached(Branch::visited_mark);
        return gp[ecx];
    }
    std::uint32_t group_bit(std::uint32_t group) {                                                       // MOV ECX,ESI; AND ECX,1F; MOV EDX,1; SHL EDX,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(group&31u,4);gp[edx]=fl.shift(1u,gp[ecx],4);
        return gp[edx];
    }
    // The edges.
    std::uint32_t first_edge(){return this->gp[edx]=this->flags.logic(0u,4);}                                // XOR EDX,EDX
    bool edges_present(std::uint32_t count){this->compare32(count,this->gp[edx]);return std::int32_t(count)>0;}   // CMP ECX,EDX; JLE
    std::uint32_t edge_address(std::uint32_t edges,std::uint32_t offset){return this->gp[edi]=this->flags.plus(edges,offset,4);}   // ADD EDI,[ESP+18]
    std::uint32_t edge_bit(std::uint32_t index) {                                                        // MOV ECX,EDX; AND ECX,1F; MOV EAX,1; SHL EAX,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(index&31u,4);gp[eax]=fl.shift(1u,gp[ecx],4);
        return gp[eax];
    }
    std::uint32_t edge_word(std::uint32_t index){return this->gp[edx]=this->flags.right(index,5u,4,true);}   // SAR EDX,5
    bool active(std::uint32_t word,std::uint32_t bit_value){return this->flags.logic(word&bit_value,4)!=0u;}   // TEST dword,reg
    bool first_touches(std::uint32_t a,std::uint32_t group) {                                            // MOVSX EAX,CX; CMP EAX,ESI
        auto &gp=this->gp;
        gp[eax]=sx16(gp[ecx]);this->compare32(gp[eax],group);
        return sx16(a)==group;
    }
    bool second_touches(std::uint32_t b,std::uint32_t group){this->compare32(b,group);return b==group;}       // CMP EDX,ESI
    std::uint32_t other_is_first(std::uint32_t) {                                                        // CMP EAX,ESI; MOVSX EDX,CX
        auto &gp=this->gp;
        this->compare32(gp[eax],gp[esi]);this->reached(Branch::touching);
        return gp[edx]=sx16(gp[ecx]);
    }
    std::uint32_t other_is_second(std::uint32_t){this->reached(Branch::touching);return this->gp[edx]=sx16(this->gp[ecx]);}   // MOVSX EDX,CX
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    std::uint32_t group_entry(std::uint32_t other,std::uint32_t groups) {                               // IMUL EAX,EAX,44; ADD EAX,EBX
        auto &fl=this->flags;
        return this->gp[eax]=fl.plus(fl.product(other,logic::group_size),groups,4);
    }
    bool kind_is(std::uint32_t kind,std::uint32_t k) {                                                   // CMP byte,k
        this->flags.minus(kind&0xffu,k,1);
        const bool is=(kind&0xffu)==k;
        if(is && k==logic::no_kind)this->reached(Branch::kind_six);
        if(is && k==logic::closed_kind)this->reached(Branch::closed);
        return is;
    }
    std::uint32_t other_bit(std::uint32_t other) {                                                       // MOV ECX,EDX; AND ECX,1F; MOV EAX,1; SHL EAX,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(other&31u,4);gp[eax]=fl.shift(1u,gp[ecx],4);
        return gp[eax];
    }
    std::uint32_t other_word(std::uint32_t other){this->reached(Branch::visited_skip);return this->gp[ecx]=this->flags.right(other,5u,4,true);}   // MOV ECX,EDX; SAR ECX,5
    // The pairs.
    std::uint32_t first_pair(){return this->gp[ebp]=this->flags.logic(0u,4);}                                // XOR EBP,EBP
    std::uint32_t pair_offset_start(){return this->gp[ebx]=this->flags.logic(0u,4);}                         // XOR EBX,EBX
    std::uint32_t pair_address(std::uint32_t pairs,std::uint32_t offset){return this->gp[esi]=this->flags.plus(pairs,offset,4);}   // ADD ESI,EBX
    bool pair_accepted(Kept &kept,bool second_loop,bool again,std::uint32_t py,std::uint32_t px) {      // PUSH ECX; PUSH EDX; CALL DDA90 / DDA10; TEST AL,AL
        static constexpr std::uint32_t pcs[2][2]{{0x1deaf5u,0x1deb0eu},{0x1debe5u,0x1debfeu}};
        const auto pc=pcs[second_loop?1:0][again?1:0];
        put(pc,py);put(pc+1u,px);
        direct(pc+2u,again?0xdda10u:0xdda90u,8u,kept);
        this->test8(this->gp[eax]);
        const bool accepted=low_byte(this->gp[eax])!=0u;
        if(again && !accepted)this->reached(Branch::pair_failed);
        return accepted;
    }
    std::uint32_t next_pair(std::uint32_t counter){return this->gp[ebp]=this->flags.increment(counter,4,false);}   // INC EBP
    std::uint32_t pair_offset_next(std::uint32_t offset){return this->gp[ebx]=this->flags.plus(offset,logic::pair_size,4);}   // ADD EBX,0C
    std::uint32_t group_entry_again(std::uint32_t other,std::uint32_t groups) {                         // MOV EAX,EDX; IMUL EAX,EAX,44; LEA ECX,[EAX+EBX]
        auto &gp=this->gp;
        gp[eax]=this->flags.product(other,logic::group_size);
        return gp[ecx]=gp[eax]+groups;
    }
    std::uint32_t member_slot_eax(std::uint32_t group){return this->gp[eax]=this->flags.product(group,logic::group_size);}   // MOV EAX,ESI; IMUL EAX,EAX,44
    std::uint32_t widened_own(std::uint32_t){return this->gp[eax]=sx8(this->gp[eax]);}                       // MOVSX EAX,AL
    std::uint32_t widened_other(std::uint32_t){return this->gp[ecx]=sx8(this->gp[ecx]);}                     // MOVSX ECX,CL
    // The flood of the other group.
    void argument_pushed(unsigned k) {
        static constexpr std::uint32_t pcs[]{0x1dec44u,0x1dec49u,0x1dec4eu,0x1dec53u,0x1dec58u,0x1dec5du,0x1dec62u,0x1dec63u,0x1dec68u,
            0x1dec69u,0x1dec6au};
        static constexpr unsigned regs[]{ecx,eax,ecx,eax,ecx,edx,esi,edx,ebx,eax,ecx};
        put(pcs[k],this->gp[regs[k]]);
    }
    void flood_other(Kept &kept){put(0x1dec6fu,this->gp[edx]);direct(0x1dec70u,0x1de8e0u,0x30u,kept);this->reached(Branch::flooded_other);}   // PUSH EDX; CALL 1DE8E0
    std::uint32_t next_edge(std::uint32_t index){return this->gp[edx]=this->flags.increment(index,4,false);}   // INC EDX
    std::uint32_t edge_offset_next(std::uint32_t offset){return this->gp[edi]=this->flags.plus(offset,logic::edge_size,4);}   // ADD EDI,18
    void finish(Exit exit) {                                                                             // (POP EBP; POP EDI); POP ESI; POP EBX; ADD ESP,18
        auto &gp=this->gp;
        if(exit==Exit::flooded){gp[ebp]=this->pop(0x1deb56u);gp[edi]=this->pop(0x1deb57u);}
        else this->reached(Branch::empty);
        gp[esi]=this->pop(0x1deb58u);gp[ebx]=this->pop(0x1deb59u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
        this->ret(0x1deb5du,0x30u);this->publish(0x1deb5du);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::group_flood::flood(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x1de8e0u,0x1deb5du,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_group_flood_detail
