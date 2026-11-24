#pragma once
// Boundary adapter for the instance blocks query, 16BBC0
// (native_tu15_instance_blocks_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the instance, EBX the
// point, the flags pushed (RET 4), the answer in AL; a 12-byte frame (the
// point in the instance's space) below the saved EBP, ESI and EDI; 16B810
// (EDX the definition; the flags pushed, RET 4), 145200 (ECX the instance,
// ESI the point, EDX the frame) and 14CDB0 (ECX the definition's BSP, EAX 0)
// direct, every register each returns coming back; XMM0 to XMM4 as the
// sphere test leaves them. Each access publishes its original instruction.
// HALO2_REFERENCE_INSTANCE_BLOCKS=1 restores the translated body.
// HALO2_VERIFY_INSTANCE_BLOCKS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_instance_blocks_systems.hpp"

namespace halo2::native_instance_blocks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_instance_blocks;
using Access=logic::instance_blocks::Access;
using Kept=logic::instance_blocks::Kept;
using Outcome=logic::instance_blocks::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_INSTANCE_BLOCKS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_INSTANCE_BLOCKS");return v;}
enum Branch : unsigned {refused, far, inside, holds, branch_count};
static_assert(unsigned(Outcome::holds)==Branch::holds);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Instance blocks"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x16bbc8u,0x16bbcdu,0x16bbd3u,0x16bbd9u,0x16bbe2u,0x16bbfdu,0x16bc02u,0x16bc06u,0x16bc0bu,0x16bc10u,
    0x16bc15u,0x16bc1au};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::radius)+1u);
template<class State,class Accept,class Transform,class Locate> struct Adapter : Port<State> {
    Accept &accept_child;Transform &transform_child;Locate &locate_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Accept &a,Transform &x,Locate &l)
        :Port<State>(s,c,t,{}),accept_child(a),transform_child(x),locate_child(l){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case bsp:case flags_argument:gp[eax]=v;break;
        case instances:gp[edx]=v;break;
        case definitions:gp[ecx]=v;break;
        case definition:gp[ebp]=logic::widen16(v);break;                                                // MOVSX EBP,word [EDI+EDX+34]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving() {                                                                                      // SUB ESP,C; PUSH EBP, ESI, EDI; MOV ESI,EAX
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0xcu,4);depth=0xcu;
        put(0x16bbc3u,gp[ebp]);put(0x16bbc4u,gp[esi]);put(0x16bbc5u,gp[edi]);gp[esi]=gp[eax];
    }
    std::uint32_t scaled(std::uint32_t index){return this->gp[edi]=this->flags.product(index,logic::instance_size);}   // MOV EDI,ESI; IMUL EDI,58
    std::uint32_t instance(std::uint32_t scaled_value,std::uint32_t instances_address) {                // ADD EDI,EDX
        return this->gp[edi]=this->flags.plus(scaled_value,instances_address,4);
    }
    std::uint32_t definition(std::uint32_t index,std::uint32_t definitions_address) {                   // IMUL EBP,C8; ADD EBP,ECX
        auto &fl=this->flags;
        return this->gp[ebp]=fl.plus(fl.product(index,logic::definition_size),definitions_address,4);
    }
    std::uint32_t accepted(std::uint32_t flags,Kept &kept) {                                           // PUSH EAX; MOV EDX,EBP; CALL 16B810; TEST AL,AL
        auto &gp=this->gp;
        put(0x16bbf1u,flags);gp[edx]=kept.definition;
        this->call(0x16bbf4u,0x16bbf9u,0x16b810u,accept_child);depth-=4u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    void sphere(std::uint32_t reach,std::uint32_t dy,std::uint32_t dz,std::uint32_t radius,std::uint32_t distance) {   // COMISS XMM0,XMM4
        auto &x=this->s.xmm;
        x[0]=lane0(reach);x[1]=lane0(dy);x[2]=lane0(dz);x[3]=lane0(radius);x[4]=lane0(distance);
        this->flags.scalar_compare(x[0],x[4],false);
    }
    void transformed(Kept &kept) {                                                                       // LEA EDX; MOV ESI,EBX; MOV ECX,EDI; CALL 145200
        auto &gp=this->gp;
        gp[edx]=local(logic::local::point);gp[esi]=kept.point;gp[ecx]=kept.instance;
        this->call(0x16bc50u,0x16bc55u,0x145200u,transform_child);
        refresh(kept);
    }
    std::uint32_t located(Kept &kept) {                                                                  // LEA ECX,[EBP+70]; XOR EAX,EAX; CALL 14CDB0; CMP EAX,-1
        auto &gp=this->gp;
        gp[ecx]=kept.definition+logic::definition::bsp;gp[eax]=this->flags.logic(0u,4);
        this->call(0x16bc5au,0x16bc5fu,0x14cdb0u,locate_child);
        refresh(kept);
        this->compare32(gp[eax],logic::none);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::instance_blocks::query(memory,*this,gp[eax],Kept{gp[ebx],gp[ebp],gp[edi]});
        this->reached(unsigned(outcome));
        std::uint32_t at;
        if(outcome==Outcome::holds) {                                                                    // POP EDI; POP ESI; MOV AL,1; POP EBP
            gp[edi]=take(0x16bc64u);gp[esi]=take(0x16bc65u);set_low_byte(gp[eax],1u);gp[ebp]=take(0x16bc68u);at=0x16bc69u;
        } else {                                                                                         // POP EDI; POP ESI; XOR AL,AL; POP EBP
            gp[edi]=take(0x16bc6fu);gp[esi]=take(0x16bc70u);set_low_byte(gp[eax],fl.logic(0u,1));gp[ebp]=take(0x16bc73u);at=0x16bc74u;
        }
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                                 // ADD ESP,C
        this->ret(at+3u,4u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Accept,class Transform,class Locate>
bool bridge(State &s,Reference reference,Accept accept,Transform transform,Locate locate) {
    return run_owner(s,0x16bbc0u,0x16bc77u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Accept,Transform,Locate>(v,c,t,accept,transform,locate);});
}
} // namespace halo2::native_instance_blocks_detail
