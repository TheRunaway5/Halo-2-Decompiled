#pragma once
// Boundary adapter for the point blocked query, 16BDA0
// (native_tu15_point_blocked_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: four arguments pushed (RET 0x10),
// the answer in AL (and in the point argument's slot); an 8-byte frame (the
// iterator's cursor and state) below the saved EBX, EBP and EDI (ESI past the
// cluster); 14CDB0 (ECX, EDX the point, EAX 0), 16BBC0 (EAX the surface, EBX
// the point; the flags pushed, RET 4), 16E360, BA970 (EBX the state; the leaf
// and the cursor pushed, RET 8), BA9E0 (EBX the state; the cursor pushed,
// RET 4) and 16BC80 (six pushed, RET 0x18) direct, every register each
// returns coming back; XMM0 to XMM4 as the tests leave them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_POINT_BLOCKED=1 restores the translated body.
// HALO2_VERIFY_POINT_BLOCKED=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_point_blocked_systems.hpp"
#include <utility>

namespace halo2::native_point_blocked_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_point_blocked;
using Access=logic::point_blocked::Access;
using Kept=logic::point_blocked::Kept;
using Outcome=logic::point_blocked::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POINT_BLOCKED");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POINT_BLOCKED");return v;}
enum Branch : unsigned {outside, unasked, answered, blocked, plane, shape, surface, branch_count};
static_assert(unsigned(Outcome::blocked)==Branch::blocked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Point blocked"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x16bda3u,0x16bdabu,0x16bdb6u,0x16bdd1u,0x16bde8u,0x16bdeeu,0x16bdf1u,0x16bdf5u,0x16bdffu,0x16be0fu,
    0x16be1bu,0x16be24u,0x16be2eu,0u,0x16be38u,0u,0x16be46u,0u,0x16be54u,0x16be63u,0x16be69u,0x16be75u,0x16be7cu,0x16be86u,0x16be8eu,
    0x16be9cu,0x16beb0u,0x16beb6u,0x16bec4u,0x16becau,0x16bed7u,0x16bedcu,0x16bee3u,0x16bef5u,0x16bf30u,0x16bf34u,0x16bf43u,0x16bf4eu,
    0x16bf58u,0x16bf62u,0x16bf6cu,0x16bf9au,0x16bfa2u,0x16bfacu,0x16bfd5u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_final)+1u);
template<class State,class Locate,class Surface,class Ignored,class First,class Blocks,class Next> struct Adapter : Port<State> {
    Locate &locate_child;Surface &surface_child;Ignored &ignored_child;First &first_child;Blocks &blocks_child;Next &next_child;
    std::uint32_t depth{};      // bytes below the return address
    bool objects{};
    unsigned exit_kind{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Locate &l,Surface &u,Ignored &i,First &f,Blocks &b,Next &n)
        :Port<State>(s,c,t,{}),locate_child(l),surface_child(u),ignored_child(i),first_child(f),blocks_child(b),next_child(n){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        if(a==point_x)return objects?0x16bf53u:0x16be4bu;
        if(a==point_y)return objects?0x16bf5du:0x16be3du;
        if(a==point_z)return objects?0x16bf67u:0x16be33u;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case locator:case flags_argument:case flags_again:case skip_again:gp[ecx]=v;break;
        case point_argument:gp[ebp]=v;break;
        case bsp:case surfaces:case cursor_load:case skip:case flags_push:gp[edx]=v;break;
        case clusters:case leaf_load:gp[ebx]=v;break;
        case cluster_leaf:gp[ebx]=logic::widen16(v);break;
        case leaves:case planes:case shapes_global:case shape_list:gp[esi]=v;break;
        case kind:gp[eax]=logic::widen8(v);break;
        case answer_load:case answer_check:case answer_final:set_low_byte(gp[eax],v);break;
        case leaves_again:gp[edi]=v;break;
        case surface_count:case count_again:gp[eax]=v;break;
        case surface:gp[eax]=logic::widen16(v);break;
        case object_type:set_low_byte(gp[ecx],v);break;
        case object_flags:gp[ecx]=this->flags.logic(0u,4);set_low_word(gp[ecx],v);break;               // XOR ECX,ECX; MOV CX
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){this->gp[esp]=this->flags.minus(this->gp[esp],8u,4);depth=8u;}                    // SUB ESP,8
    void saving(unsigned n) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(n==0u){put(0x16bda9u,gp[ebx]);put(0x16bdaau,gp[ebp]);}                                        // PUSH EBX; PUSH EBP
        else if(n==1u) {                                                                                  // XOR BL,BL; PUSH EDI; MOV EDX,EBP; XOR EAX,EAX
            set_low_byte(gp[ebx],fl.logic(0u,1));put(0x16bdb1u,gp[edi]);gp[edx]=gp[ebp];gp[eax]=fl.logic(0u,4);
        } else{put(0x16bdd5u,gp[esi]);gp[esi]=fl.logic(gp[ecx]&logic::flag::leaf,4);}                    // PUSH ESI; MOV ESI,ECX; AND ESI,2
    }
    std::uint32_t locate(std::uint32_t,Kept &kept) {                                                    // CALL 14CDB0; OR EDI,-1; CMP EAX,EDI
        auto &gp=this->gp;
        this->call(0x16bdbau,0x16bdbfu,0x14cdb0u,locate_child);
        refresh(kept);
        gp[edi]=this->flags.logic(logic::none,4);this->compare32(gp[eax],gp[edi]);
        return gp[eax];
    }
    void tested8(std::uint32_t v){this->test8(v);}
    void leaf_tested(std::uint32_t bit){this->flags.logic(bit,4);}                                    // TEST ESI,ESI
    void leaf_found(std::uint32_t index){this->gp[ebx]=index;}
    std::uint32_t leaf(std::uint32_t index,std::uint32_t leaves_address) {                             // MOV EAX,EBX; IMUL EAX,B0; ADD EAX,ESI
        auto &fl=this->flags;
        return this->gp[eax]=fl.plus(fl.product(index,logic::leaf_size),leaves_address,4);
    }
    void kind_tested(std::uint32_t kind) {                                                             // CMP AL,FF; TEST AL,AL
        auto &fl=this->flags;
        fl.minus(kind&0xffu,0xffu,1);
        if((kind&0xffu)!=0xffu)fl.logic(kind&0xffu,1);
    }
    std::uint32_t plane(std::uint32_t kind,std::uint32_t entry) {                                      // AND EAX,7F; LEA EAX*3; (CMP); LEA EAX,[ESI+EAX*8]
        this->reached(Branch::plane);
        this->flags.logic(kind&logic::kind_mask,4);
        return this->gp[eax]=entry;
    }
    void used_tested(std::uint32_t used){this->flags.minus(used&0xffffu,0xffffu,2);}                  // CMP word,DI
    void plane_distance(std::uint32_t distance,std::uint32_t) {                                        // XORPS XMM1; COMISS XMM1,XMM0
        auto &x=this->s.xmm;
        x[0]=lane0(distance);x[1]=_mm_setzero_ps();
        this->flags.scalar_compare(x[1],x[0],false);
    }
    std::uint32_t shape(std::uint32_t kind,std::uint32_t offset) {                                     // AND EAX,7F; SHL EAX,4
        auto &fl=this->flags;
        this->reached(Branch::shape);
        fl.shift(fl.logic(kind&logic::kind_mask,4),4u,4);
        return this->gp[eax]=offset;
    }
    std::uint32_t surface_leaf(std::uint32_t index){return this->gp[esi]=this->flags.product(index,logic::leaf_size);}   // MOV ESI,EBX; IMUL ESI,B0
    std::uint32_t surfaces_counted(std::uint32_t scaled,std::uint32_t leaves_address,std::uint32_t count) {   // ADD ESI,EDI; XOR EDI,EDI; TEST EAX,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=fl.plus(scaled,leaves_address,4);gp[edi]=fl.logic(0u,4);this->test32(count);
        return gp[esi];
    }
    std::uint32_t surface_blocks(std::uint32_t surface_index,std::uint32_t flags_value,Kept &kept) {  // PUSH ECX; MOV EBX,EBP; CALL 16BBC0; TEST AL,AL
        auto &gp=this->gp;
        put(0x16bebau,flags_value);gp[ebx]=kept.point;gp[eax]=surface_index;
        this->call(0x16bebdu,0x16bec2u,0x16bbc0u,surface_child);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::surface);
        return gp[eax];
    }
    void surface_counted(std::uint32_t k,std::uint32_t count) {                                        // INC EDI; CMP EDI,EAX
        this->gp[edi]=this->flags.increment(k-1u,4,false);this->compare32(k,count);
    }
    std::pair<std::uint32_t,std::uint32_t> ignored_flags(Kept &kept) {                                 // CALL 16E360
        auto &gp=this->gp;
        this->call(0x16befdu,0x16bf02u,0x16e360u,ignored_child);
        refresh(kept);
        return {gp[eax],gp[ecx]};
    }
    void types(const Kept &kept) {                                                                     // MOV ESI,ECX; SHR ESI,4; AND ESI,1FFF; MOV EDI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=fl.logic(fl.right(gp[ecx],logic::flag::type_shift,4,false)&logic::flag::type_mask,4);
        gp[edi]=kept.mask;
    }
    std::uint32_t first_object(std::uint32_t leaf_index,Kept &kept) {                                  // LEA EAX; PUSH EAX; PUSH EBX; LEA EBX; CALL BA970; CMP EAX,-1
        auto &gp=this->gp;
        objects=true;
        gp[eax]=local(logic::local::cursor);put(0x16bf13u,gp[eax]);put(0x16bf14u,leaf_index);
        gp[ebx]=local(logic::local::iterator);
        this->call(0x16bf19u,0x16bf1eu,0xba970u,first_child);depth-=8u;
        refresh(kept);
        this->compare32(gp[eax],logic::none);
        return gp[eax];
    }
    void types_widened(const Kept &kept){this->gp[esi]=kept.source;}                                    // MOVZX ESI,SI
    bool type_tested(std::uint32_t types,std::uint32_t type) {                                         // MOV EBX,1; SHL EBX,CL; TEST ESI,EBX
        auto &gp=this->gp;
        gp[ebx]=this->flags.shift(1u,type&0xffu,4);
        return this->flags.logic(types&gp[ebx],4)!=0u;
    }
    bool flags_tested(std::uint32_t object_flags_value,std::uint32_t mask) {                           // AND ECX,EDI; TEST CX,CX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(object_flags_value&mask,4);
        return fl.logic(gp[ecx]&0xffffu,2)==0u;
    }
    void sphere(std::uint32_t reach,std::uint32_t dy,std::uint32_t dz,std::uint32_t radius,std::uint32_t distance) {   // COMISS XMM0,XMM4
        auto &x=this->s.xmm;
        x[0]=lane0(reach);x[1]=lane0(dy);x[2]=lane0(dz);x[3]=lane0(radius);x[4]=lane0(distance);
        this->flags.scalar_compare(x[0],x[4],false);
    }
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void pushing_skips(std::uint32_t skip_again_value,std::uint32_t skip_value){put(0x16bfaau,skip_again_value);put(0x16bfabu,skip_value);}
    std::uint32_t object_blocks(std::uint32_t found,std::uint32_t flags_value,Kept &kept) {           // PUSH EBP, EDX, 1, EAX; CALL 16BC80; TEST AL,AL
        auto &gp=this->gp;
        put(0x16bfb0u,kept.point);put(0x16bfb1u,flags_value);put(0x16bfb2u,1u);put(0x16bfb4u,found);
        this->call(0x16bfb5u,0x16bfbau,0x16bc80u,blocks_child);depth-=24u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    std::uint32_t next_object(Kept &kept) {                                                            // LEA EAX; PUSH EAX; LEA EBX; CALL BA9E0; CMP EAX,-1
        auto &gp=this->gp;
        gp[eax]=local(logic::local::cursor);put(0x16bfc2u,gp[eax]);gp[ebx]=local(logic::local::iterator);
        this->call(0x16bfc7u,0x16bfccu,0xba9e0u,next_child);depth-=4u;
        refresh(kept);
        this->compare32(gp[eax],logic::none);
        return gp[eax];
    }
    void restoring(unsigned k){exit_kind=k;}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::point_blocked::query(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]},outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at;
        switch(exit_kind) {
        case 0u:gp[edi]=take(0x16bdc6u);gp[ebp]=take(0x16bdc7u);set_low_byte(gp[eax],answer);gp[ebx]=take(0x16bdcau);at=0x16bdcbu;break;
        case 1u:set_low_byte(gp[eax],answer);gp[esi]=take(0x16bff1u);gp[edi]=take(0x16bff2u);gp[ebp]=take(0x16bff3u);gp[ebx]=take(0x16bff4u);
            at=0x16bff5u;break;
        case 2u:set_low_byte(gp[eax],answer);gp[esi]=take(0x16bfd9u);gp[edi]=take(0x16bfdau);gp[ebp]=take(0x16bfdbu);gp[ebx]=take(0x16bfdcu);
            at=0x16bfddu;break;
        default:gp[esi]=take(0x16bfe3u);gp[edi]=take(0x16bfe4u);gp[ebp]=take(0x16bfe5u);set_low_byte(gp[eax],answer);gp[ebx]=take(0x16bfe8u);
            at=0x16bfe9u;break;
        }
        gp[esp]=fl.plus(gp[esp],8u,4);                                                                   // ADD ESP,8
        this->ret(at+3u,0x10u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Locate,class Surface,class Ignored,class First,class Blocks,class Next>
bool bridge(State &s,Reference reference,Locate locate,Surface surface,Ignored ignored,First first,Blocks blocks,Next next) {
    return run_owner(s,0x16bda0u,0x16bff8u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,Locate,Surface,Ignored,First,Blocks,Next>(v,c,t,locate,surface,ignored,first,blocks,next);});
}
} // namespace halo2::native_point_blocked_detail
