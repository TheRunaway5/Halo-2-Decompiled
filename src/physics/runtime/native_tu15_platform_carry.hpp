#pragma once
// Boundary adapter for the platform carry, 1F8710
// (native_tu15_platform_carry_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the ground, the body and
// the character on the stack (RET C); 0x58 bytes of frame with EBX, EBP,
// ESI and EDI saved below them, read from the ESP each child left; 1FA110
// and 1FA1A0 (three pushed, RET C) and 1F8B30 (two pushed, RET 8) are
// direct, every register each returns coming back. The rotation copies are
// the original's REP MOVSD, nine dwords, backwards under the direction
// flag. XMM0 to XMM7 hold what the original's scalar moves and arithmetic
// leave in them at each call and at the end. Each access publishes its
// original instruction.
// HALO2_REFERENCE_PLATFORM_CARRY=1 restores the translated body.
// HALO2_VERIFY_PLATFORM_CARRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_platform_carry_systems.hpp"

namespace halo2::native_platform_carry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_platform_carry;
using Access=logic::platform_carry::Access;
using Kept=logic::platform_carry::Kept;
using Columns=logic::platform_carry::Columns;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLATFORM_CARRY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLATFORM_CARRY");return v;}
enum Branch : unsigned {moving, found, carried, ticked, moved_after, follows, capped, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Platform carry"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1f8713u,0x1f871du,0x1f871fu,0x1f872eu,0x1f8732u,0x1f8735u,0x1f8738u,0x1f873eu,0x1f8744u,
    0x1f8748u,0x1f874cu,0x1f8752u,0x1f8755u,0x1f8759u,0x1f875cu,0x1f875fu,0x1f8762u,0x1f8779u,0x1f8779u,0x1f877bu,0x1f877du,
    0x1f8781u,0x1f8784u,0x1f8787u,0x1f878bu,0x1f878eu,0x1f8792u,0x1f87a6u,0x1f87afu,0x1f87bbu,0x1f87c1u,0x1f87d0u,0x1f87deu,
    0x1f87e4u,0x1f87edu,0x1f87f4u,0x1f87f8u,0x1f87fcu,0x1f8800u,0x1f8804u,0x1f8808u,0x1f8808u,0x1f8818u,0x1f881cu,0x1f881cu,
    0x1f881eu,0x1f8826u,0x1f8829u,0x1f882eu,0x1f8833u,0x1f8839u,0x1f883eu,0x1f8844u,0x1f8847u,0x1f884cu,0x1f8851u,0x1f8857u,
    0x1f8861u,0x1f8867u,0x1f886du,0x1f891au,0x1f8877u,0x1f8889u,0x1f888du,0x1f8893u,0x1f8899u,0x1f889fu,0x1f88a5u,0x1f88abu,
    0x1f88b4u,0x1f88b6u,0x1f88bau,0x1f88bdu,0x1f88c1u,0x1f88d0u,0x1f88d6u,0x1f88dcu,0x1f88e2u,0x1f88e2u,0x1f88e6u,0x1f88eau,
    0x1f88f0u,0x1f88fau,0x1f8904u,0x1f890au,0x1f8910u,0x1f8920u,0x1f8924u,0x1f8935u,0x1f893du,0x1f8942u,0x1f8947u,0x1f894cu,
    0x1f8951u,0x1f8956u,0x1f895bu,0x1f8960u,0x1f8965u,0x1f896fu,0x1f8977u,0x1f8983u,0x1f8988u,0x1f898eu,0x1f8993u,0x1f8998u,
    0x1f89a1u,0x1f89a6u,0x1f89abu,0x1f89b5u,0x1f89bcu,0x1f89beu,0x1f89c2u,0x1f89c5u,0x1f89c8u,0x1f89ccu,0x1f89d4u,0x1f89d9u,
    0x1f89deu,0x1f89e2u,0x1f89e8u,0x1f89fau,0x1f8a08u,0x1f8a0cu,0x1f8a10u,0x1f8a15u,0x1f8a1bu,0x1f8a29u,0x1f8a2eu,0x1f8a38u,
    0x1f8a3du,0x1f8a41u,0x1f8a46u,0x1f8a4cu,0x1f8a52u,0x1f8a5fu,0x1f8a6du,0x1f8a7bu,0x1f8a7du,0x1f8a81u,0x1f8a84u,0x1f8a87u,
    0x1f8a8bu,0x1f8a93u,0x1f8a97u,0x1f8aa0u,0x1f8ab1u,0x1f8ab5u,0x1f8abau,0x1f8ac0u,0x1f8ac5u,0x1f8ac9u,0x1f8ad7u,0x1f8adcu,
    0x1f8ae1u,0x1f8ae6u,0x1f8aecu,0x1f8afcu,0x1f8b01u,0x1f8b05u,0x1f8b1au};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::ticks_store)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // the direct children, by entry address
    std::uint32_t depth{};   // bytes pushed below the saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        kept.surface_entry=this->gp[ebx];kept.body_address=this->gp[ebp];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case character_load:case surface_index:case zero_load:case zero_z:case search_character:case transform_ground:
        case transform_character:case ground_again:case forward_z:case up_z:case ticks_load:gp[eax]=v;break;
        case previous_z:case surface_data:case zero_x:case zero_y:case surface_object_load:case attached_x:case attached_z:
        case ground_final:gp[ecx]=v;break;
        case previous_x:case previous_y:case surfaces_load:case ground_load:case attached_y:case surface_x:case offset_x_load:
        case offset_y_load:case offset_z_load:case update_ground:case forward_x:case forward_y:case up_x:case up_y:gp[edx]=v;break;
        case identity_load:case update_character:gp[esi]=v;break;
        case body_load:gp[ebp]=v;break;
        case moving_surface:gp[ebx]=v;break;
        case moving_flag:case found_reload:case moving_final:case carried_final:case found_final:set_low_byte(gp[eax],v);break;   // MOV AL
        case moving_reload:case carried_load:set_low_byte(gp[ecx],v);break;                                 // MOV CL
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame, the displacement and the surface.
    void framing() {                                                                                     // SUB ESP,58
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;
    }
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x1f8727u,0x1f873cu,0x1f873du,0x1f8767u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t previous_address(std::uint32_t character){return this->gp[ecx]=character+logic::character::previous;}   // LEA ECX,[EAX+124]
    std::uint32_t surface_offset(std::uint32_t index) {                                                  // AND EAX,FFFF; LEA EBX,[EAX+EAX*4]; OR EAX,-1
        auto &gp=this->gp;
        gp[ebx]=5u*this->flags.logic(index&logic::handle_mask,4);gp[eax]=this->flags.logic(logic::none,4);
        return gp[ebx];
    }
    std::uint32_t surface_entry(std::uint32_t scaled,std::uint32_t data) {                              // SHL EBX,5; ADD EBX,ECX; ADD ESI,4; MOV ECX,9; LEA EDI,[ESP+44]
        auto &gp=this->gp;auto &fl=this->flags;
        static_assert(5u*(1u<<5)==logic::surface_size);
        gp[ebx]=fl.plus(fl.shift(scaled,5u,4),data,4);
        gp[esi]=fl.plus(gp[esi],logic::identity_rotation,4);gp[ecx]=logic::rotation_words;gp[edi]=local(logic::local::rotation);
        return gp[ebx];
    }
    std::uint32_t copy_offset(std::uint32_t k)const{return step()*k;}
    void copied(std::uint32_t source,std::uint32_t target) {                                            // (REP MOVSD done)
        auto &gp=this->gp;
        gp[esi]=source+logic::rotation_words*step();gp[edi]=target+logic::rotation_words*step();gp[ecx]=0u;
    }
    bool moving(std::uint32_t v) {                                                                       // TEST AL,AL
        this->test8(v);
        if(v&0xffu)this->reached(Branch::moving);
        return (v&0xffu)!=0u;
    }
    // The attached object and the carry.
    void search_arguments() {                                                                            // LEA EAX/ECX; PUSH x3 (the rotation, the attached offset and position)
        auto &gp=this->gp;
        gp[eax]=local(logic::local::rotation);put(0x1f879bu,gp[eax]);
        gp[ecx]=local(logic::local::attached_offset);put(0x1f87a0u,gp[ecx]);
        gp[eax]=local(logic::local::attached);put(0x1f87a5u,gp[eax]);
    }
    std::uint32_t attached(Kept &kept) {                                                                 // MOV ECX,EDX; CALL 1FA110; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=gp[edx];
        direct(0x1f87b4u,0x1fa110u,12u,kept);
        this->test8(gp[eax]);
        if(low_byte(gp[eax]))this->reached(Branch::found);
        return low_byte(gp[eax]);
    }
    void transform_arguments() {                                                                         // LEA ECX/EDX; PUSH ECX; PUSH EDX; PUSH EAX
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::rotation);put(0x1f87c9u,gp[ecx]);
        gp[edx]=local(logic::local::offset);put(0x1f87ceu,gp[edx]);
        put(0x1f87cfu,gp[eax]);
    }
    bool carried(Kept &kept) {                                                                           // CALL 1FA1A0; TEST AL,AL
        direct(0x1f87d7u,0x1fa1a0u,12u,kept);
        this->test8(this->gp[eax]);
        if(low_byte(this->gp[eax]))this->reached(Branch::carried);
        return low_byte(this->gp[eax])!=0u;
    }
    void answered_found(){this->test8(this->gp[eax]);}                                                    // TEST AL,AL
    std::uint32_t flagged(std::uint32_t flags_value){return this->flags.logic(flags_value|logic::body::attached_flag,4);}   // OR dword,8
    void copying_rotation(std::uint32_t body_address) {                                                 // LEA EDI,[EBP+30]; MOV ECX,9; LEA ESI,[ESP+44]
        auto &gp=this->gp;
        gp[edi]=body_address+logic::body::rotation;gp[ecx]=logic::rotation_words;gp[esi]=local(logic::local::rotation);
    }
    // The motion and the displacement.
    bool moving_again(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                // TEST CL,CL / TEST AL,AL
    void surface_position(std::uint32_t){this->gp[ebx]=this->flags.plus(this->gp[ebx],logic::surface_object::position,4);}   // ADD EBX,30
    void moved(std::uint32_t dz,std::uint32_t motion_y){auto &x=this->s.xmm;x[0]=lane0(dz);x[1]=lane0(motion_y);}
    bool carried_again(std::uint32_t carried_value,std::uint32_t found_value) {                         // TEST CL,CL; JNZ; TEST AL,AL
        this->test8(carried_value);
        if(carried_value&0xffu)return true;
        this->test8(this->gp[eax]);
        return (found_value&0xffu)!=0u;
    }
    void found_tested(std::uint32_t){this->test8(this->gp[eax]);}                                          // TEST AL,AL
    void motion_address(std::uint32_t body_address){this->gp[ecx]=body_address+logic::body::motion;}      // LEA ECX,[EBP+64]
    void displaced(std::uint32_t dx,std::uint32_t dy,std::uint32_t dz){auto &x=this->s.xmm;x[0]=lane0(dx);x[1]=lane0(dy);x[2]=lane0(dz);}
    bool ticked(std::uint32_t ticks) {                                                                   // CMP byte,1; JBE
        this->flags.minus(ticks,1u,1);
        if(ticks>1u)this->reached(Branch::ticked);
        return ticks>1u;
    }
    void attached_displaced(std::uint32_t ax,std::uint32_t az){auto &x=this->s.xmm;x[3]=lane0(ax);x[0]=lane0(az);}
    void update(Kept &kept,std::uint32_t,std::uint32_t) {                                                // LEA ECX,[ESP+14]; PUSH ECX; PUSH EDX; MOV EDI,EBP; CALL 1F8B30
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::displacement);put(0x1f892cu,gp[ecx]);put(0x1f892du,gp[edx]);gp[edi]=gp[ebp];
        direct(0x1f8930u,0x1f8b30u,8u,kept);
    }
    // The body follows.
    void positioned(std::uint32_t sum){this->s.xmm[0]=lane0(sum);this->reached(Branch::moved_after);}
    bool carried_final(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                               // TEST AL,AL
    bool found_final(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                 // TEST AL,AL
    void forward_address(std::uint32_t body_address){this->gp[ecx]=body_address+logic::body::forward;}    // LEA ECX,[EBP+18]
    void same_vector(){auto &gp=this->gp;gp[eax]=gp[ecx];this->compare32(gp[eax],gp[ecx]);}               // MOV EAX,ECX; CMP EAX,ECX
    void turning(){this->gp[eax]=local(logic::local::turned);}                                            // LEA EAX,[ESP+38]
    void turned_forward(const Columns &c,std::uint32_t out_z,std::uint32_t zz_product) {
        auto &x=this->s.xmm;
        x[0]=lane0(c.xx);x[1]=lane0(c.xy);x[2]=lane0(c.xz);x[3]=lane0(c.yx);x[4]=lane0(c.zx);x[5]=lane0(c.zz);x[6]=lane0(out_z);
        x[7]=lane0(zz_product);
        this->reached(Branch::follows);
    }
    void up_address(std::uint32_t body_address){this->gp[ecx]=body_address+logic::body::up;same_vector();}   // LEA ECX,[EBP+24]; MOV EAX,ECX; CMP EAX,ECX
    void turned_up(std::uint32_t out_x,std::uint32_t z2,std::uint32_t x3,std::uint32_t z3,std::uint32_t out_z) {
        auto &x=this->s.xmm;
        x[0]=lane0(out_z);x[1]=lane0(z3);x[2]=lane0(x3);x[3]=lane0(z2);x[6]=lane0(out_x);
    }
    // The ticks.
    std::uint32_t counted(std::uint32_t) {                                                               // POP EDI; POP ESI; INC EAX; CMP EAX,FE; POP EBP; POP EBX
        auto &gp=this->gp;
        gp[edi]=this->pop(0x1f8b09u);gp[esi]=this->pop(0x1f8b0au);
        gp[eax]=this->flags.increment(gp[eax],4,false);this->compare32(gp[eax],logic::tick_limit);
        gp[ebp]=this->pop(0x1f8b11u);gp[ebx]=this->pop(0x1f8b12u);
        if(std::int32_t(gp[eax])>std::int32_t(logic::tick_limit)){gp[eax]=logic::tick_limit;this->reached(Branch::capped);}
        return gp[eax]&0xffu;
    }
    void finished(){auto &gp=this->gp;gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);}                // ADD ESP,58
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::platform_carry::carry(memory,*this,Kept{gp[ebx],gp[ebp]});
        this->ret(0x1f8b20u,12u);this->publish(0x1f8b20u);
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x1f8710u,0x1f8b20u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_platform_carry_detail
