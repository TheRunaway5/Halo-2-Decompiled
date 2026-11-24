#pragma once
// Boundary adapter for the contrails update, 17EAD0
// (native_tu15_contrails_update_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: dt on the stack (RET 4);
// 0x10 bytes of frame with EBX and EBP (and, once a contrail is found, ESI
// and EDI) saved below them, read from the ESP each child left. 17F3D0
// (two pushed, RET 8) is direct; 16EA60, BCB70 (three pushed, RET C),
// 17EF70 (two pushed, RET 8; its last argument pushed before 17EED0),
// 17EED0 (one pushed, RET 4), 17EE10, 17F710 and 16E8A0 go through the
// dispatch; every register each returns comes back. XMM0 to XMM5 hold what
// the original's scalar moves and arithmetic leave in them, the arithmetic
// keeping each register's upper lanes. Each access publishes its original
// instruction.
// HALO2_REFERENCE_CONTRAILS_UPDATE=1 restores the translated body.
// HALO2_VERIFY_CONTRAILS_UPDATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/effects/native_tu15_contrails_update_systems.hpp"

namespace halo2::native_contrails_update_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_contrails_update;
using Access=logic::contrails_update::Access;
using Kept=logic::contrails_update::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CONTRAILS_UPDATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CONTRAILS_UPDATE");return v;}
enum Branch : unsigned {empty_array, attached, changed, active, inactive, moving, timed, scaled_rate, advanced, scaled_speed, removed,
    next_found, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Contrails update"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
inline constexpr std::uint32_t sites[]{0x17ead5u,0x17eaecu,0x17eaefu,0x17eaf5u,0x17eb03u,0x17eb11u,0x17eb20u,0x17eb23u,0x17eb37u,
    0x17eb3bu,0x17eb44u,0x17eb52u,0x17eb56u,0x17eb5cu,0x17eb60u,0x17eb66u,0x17eb6du,0x17eb7bu,0x17eb87u,0x17eb8bu,0x17eb91u,
    0x17eb9cu,0x17eba5u,0x17ebabu,0x17ebb7u,0x17ebbbu,0x17ebbfu,0x17ebbfu,0x17ec51u,0x17ec51u,0x17ebd8u,0x17ebdeu,0x17ebe2u,
    0x17ebefu,0x17ebf9u,0x17ec07u,0x17ec14u,0x17ec1au,0x17ec26u,0x17ec35u,0x17ec65u,0x17ec5au,0x17ec6eu,0x17ec73u,0x17ec77u,
    0x17ec7eu,0x17ec8au,0x17ec97u,0x17ec9cu,0x17eca0u,0x17eca7u,0x17ecb7u,0x17ecbcu,0x17ecc1u,0x17ecd3u,0x17ece9u,0x17ecf1u,
    0x17ed19u,0x17ed20u,0x17ed34u,0x17ed4au,0x17ed4du,0x17ed53u,0x17ed5fu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::next_store)+1u);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 17F3D0
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes pushed below the saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void scalar(unsigned n,std::uint32_t v){auto &x=this->s.xmm;x[n]=_mm_move_ss(x[n],lane0(v));}       // an SSE result in XMMn's lane 0
    void refresh(Kept &kept) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        kept.handle=gp[ebx];kept.base=gp[ebp];kept.contrail_address=gp[esi];kept.index=gp[edi];
        kept.step=low(x[1]);kept.rest=low(x[2]);kept.period=low(x[3]);kept.zero=low(x[4]);kept.elapsed=low(x[5]);
    }
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);depth-=popped;refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case contrails_load:case definition_load:case point_load:case contrails_again:gp[ebp]=v;break;
        case element_size:case data_load:case marker_load:case dt_points:gp[ecx]=v;break;
        case first_salt:gp[ecx]=sx16(v);break;                                                             // MOVSX ECX,word
        case first_data:case tag_load:case next_size:case bits_load:gp[edx]=v;break;
        case flags_load:set_low_byte(gp[edx],v);break;                                                     // MOV DL
        case handle_next:case handle_reload_again:gp[ebx]=v;break;
        case instances_load:case object_load:case handle_reload:case dt_again:case next_data:gp[eax]=v;break;
        case next_salt:gp[eax]=sx16(v);break;                                                              // MOVSX EAX,word
        case handle_again:case index_reload:case count_load:gp[edi]=v;break;
        case dt_load:case elapsed_reload:x[5]=lane0(v);break;                                              // MOVSS XMM5
        case scale_load:case scale_saved:case first_speed:case second_speed:case frame_rate:case frame_time_rest:case frame_time_all:
        case first_speed_again:case second_speed_again:x[0]=lane0(v);break;                                // MOVSS XMM0
        case rate_scale:case first_scale:case second_scale:case first_load:x[1]=lane0(v);break;            // MOVSS XMM1
        case one_load:x[3]=lane0(v);break;                                                                 // MOVSS XMM3
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the first contrail.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,10
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x17ead3u,0x17ead4u,0x17eb0du,0x17eb0eu};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t first_index(Kept &kept) {                                                              // XOR EDX,EDX; MOV ECX,EBP; CALL 16EA60
        auto &gp=this->gp;
        gp[edx]=this->flags.logic(0u,4);gp[ecx]=kept.base;
        routed(0x17eadfu,0x16ea60u,0u,kept);
        return gp[eax];
    }
    std::uint32_t no_handle(){return this->gp[ecx]=this->flags.logic(logic::none,4);}                        // OR ECX,-1
    bool found(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    std::uint32_t element_offset(std::uint32_t size,std::uint32_t index){return this->gp[ecx]=this->flags.product(size,index);}   // IMUL ECX,EAX
    std::uint32_t handle_of(std::uint32_t salt,std::uint32_t index,bool next) {                         // SHL reg,10; OR reg,index
        auto &fl=this->flags;
        return this->gp[next?eax:ecx]=fl.logic(fl.shift(salt,16u,4)|index,4);
    }
    std::uint32_t walking(std::uint32_t handle) {                                                        // CMP ECX,-1; MOV EBX,ECX
        auto &gp=this->gp;
        this->compare32(handle,logic::none);gp[ebx]=handle;
        if(handle==logic::none)this->reached(Branch::empty_array);
        return handle;
    }
    // A contrail.
    void zeroed(){this->s.xmm[4]=_mm_setzero_ps();}                                                         // XORPS XMM4,XMM4
    std::uint32_t contrail_index(std::uint32_t handle) {                                                 // MOV EDI,EBX; AND EDI,FFFF; LEA EAX,[EDI+EDI*8]
        auto &gp=this->gp;
        gp[edi]=this->flags.logic(handle&logic::handle_mask,4);gp[eax]=9u*gp[edi];
        return gp[edi];
    }
    void aged(std::uint32_t elapsed){scalar(5u,elapsed);}                                                   // SUBSS XMM5,[contrail+2C]
    std::uint32_t contrail_address(std::uint32_t address){return this->gp[esi]=address;}                     // LEA ESI,[ECX+EAX*8]
    std::uint32_t tag_offset(std::uint32_t tag){auto &fl=this->flags;return this->gp[edx]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);}   // AND EDX,FFFF; SHL EDX,4
    bool attached(std::uint32_t object) {                                                                // CMP EAX,-1
        this->compare32(object,logic::none);
        if(object!=logic::none)this->reached(Branch::attached);
        return object!=logic::none;
    }
    bool marker(Kept &kept,std::uint32_t name,std::uint32_t object) {                                    // LEA EDI,[ESI+14]; PUSH EDI; PUSH ECX; PUSH EAX; CALL BCB70
        auto &gp=this->gp;
        gp[edi]=kept.contrail_address+logic::contrail::scale;
        put(0x17eb73u,gp[edi]);put(0x17eb74u,name);put(0x17eb75u,object);
        routed(0x17eb76u,0xbcb70u,12u,kept);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t answered(bool){auto &gp=this->gp;set_low_byte(gp[ebx],gp[eax]);return gp[ebx];}           // MOV BL,AL
    bool changed(std::uint32_t flags_value) {                                                            // AND DL,1; CMP BL,DL
        auto &gp=this->gp;
        set_low_byte(gp[edx],this->flags.logic(flags_value&logic::contrail::active_bit,1));
        this->flags.minus(gp[ebx]&0xffu,gp[edx]&0xffu,1);
        const bool differs=(gp[ebx]&0xffu)!=(gp[edx]&0xffu);
        if(differs)this->reached(Branch::changed);
        return differs;
    }
    void forced_pushed(){put(0x17eb8fu,1u);}                                                                 // PUSH 1
    void count_pushed(){put(0x17eb9au,1u);}                                                                  // PUSH 1
    std::uint32_t cleared(){this->s.xmm[0]=_mm_setzero_ps();return 0u;}                                     // XORPS XMM0,XMM0
    void emit(Kept &kept,bool){routed(0x17eba0u,0x17ef70u,8u,kept);}                                       // CALL 17EF70
    bool active_now(std::uint32_t placed){this->test8(placed);this->reached((placed&0xffu)?Branch::active:Branch::inactive);return (placed&0xffu)!=0u;}   // TEST BL,BL
    std::uint32_t flag_set(std::uint32_t v){return this->flags.logic(v|logic::contrail::active_bit,1);}     // OR byte,1
    std::uint32_t flag_cleared(std::uint32_t v){return this->flags.logic(v&0xfeu,1);}                        // AND byte,FE
    void emit_for(Kept &kept,std::uint32_t dt) {                                                         // PUSH 1; PUSH EAX; MOV EAX,EDI; CALL 17EED0; PUSH EAX; MOV EAX,EDI; CALL 17EF70
        auto &gp=this->gp;
        put(0x17ebc3u,1u);put(0x17ebc5u,dt);gp[eax]=kept.index;
        routed(0x17ebc8u,0x17eed0u,4u,kept);
        put(0x17ebcdu,gp[eax]);gp[eax]=kept.index;
        routed(0x17ebd0u,0x17ef70u,8u,kept);
    }
    // Its frame time and positions.
    bool above(std::uint32_t a,std::uint32_t b) {                                                        // COMISS a,b; JA
        auto &fl=this->flags;
        fl.scalar_compare(lane0(a),lane0(b),false);
        return !(fl.cf||fl.zf);
    }
    std::uint32_t rest_from_elapsed(){auto &x=this->s.xmm;x[2]=x[5];this->reached(Branch::moving);return low(x[2]);}   // MOVAPS XMM2,XMM5
    bool flagged(std::uint32_t v,std::uint32_t mask){return this->flags.logic(v&mask,1)!=0u;}               // TEST byte,mask
    void rate_scaled(std::uint32_t rate){auto &x=this->s.xmm;scalar(1u,rate);x[0]=x[1];this->reached(Branch::scaled_rate);}   // MULSS XMM1,XMM0; MOVAPS XMM0,XMM1
    void period_set(std::uint32_t period){scalar(3u,period);this->reached(Branch::timed);}                  // DIVSS XMM3,XMM0
    void step_from_period(){auto &x=this->s.xmm;x[1]=x[3];}                                                  // MOVAPS XMM1,XMM3
    void step_set(std::uint32_t step){scalar(1u,step);}                                                     // SUBSS XMM1,[contrail+28]
    bool short_of(std::uint32_t rest,std::uint32_t step) {                                               // COMISS XMM2,XMM1; JB
        auto &fl=this->flags;
        fl.scalar_compare(lane0(rest),lane0(step),false);
        return fl.cf;
    }
    void frame_time_set(std::uint32_t frame_time){scalar(0u,frame_time);}                                   // ADDSS XMM0,XMM2 / XMM5
    void advance(Kept &kept){this->gp[eax]=kept.contrail_address;routed(0x17ec41u,0x17ee10u,0u,kept);this->reached(Branch::advanced);}   // MOV EAX,ESI; CALL 17EE10
    void rest_set(std::uint32_t rest){scalar(2u,rest);}                                                     // SUBSS XMM2,XMM1
    void speed_scaled(std::uint32_t speed){auto &x=this->s.xmm;scalar(1u,speed);x[0]=x[1];this->reached(Branch::scaled_speed);}                // MULSS XMM1,XMM0; MOVAPS XMM0,XMM1
    void first_moved(std::uint32_t drop,std::uint32_t first){scalar(0u,drop);scalar(1u,first);}              // MULSS XMM0,XMM5; SUBSS XMM1,XMM0
    void second_moved(std::uint32_t second){scalar(0u,second);}                                             // MULSS XMM0,XMM5; ADDSS XMM0,[contrail+20]
    // Its points and deletion.
    void points(Kept &kept,std::uint32_t dt){put(0x17ecc5u,dt);put(0x17ecc6u,kept.handle);auto child=[&](State &v){children(v,0x17f3d0u);};
        this->call(0x17ecc7u,0x17ecccu,0x17f3d0u,child);depth-=8u;refresh(kept);}                            // PUSH ECX; PUSH EBX; CALL 17F3D0
    std::uint32_t first_slot(){return this->gp[eax]=this->flags.logic(0u,4);}                                // XOR EAX,EAX
    std::uint32_t slot_widened(std::uint32_t){return this->gp[edx]=sx16(this->gp[eax]);}                     // MOVSX EDX,AX
    bool has_point(std::uint32_t point) {                                                                // OR ECX,-1; CMP EBP,ECX
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(logic::none,4);this->compare32(point,gp[ecx]);
        return point!=logic::none;
    }
    std::uint32_t next_slot(std::uint32_t slot){return this->gp[eax]=this->flags.increment(slot,4,false);}   // INC EAX
    bool more_slots(std::uint32_t slot){this->flags.minus(slot&0xffffu,logic::points,2);return std::int16_t(slot)<std::int16_t(logic::points);}   // CMP AX,4; JL
    bool all_slots(std::uint32_t slot){this->flags.minus(slot&0xffffu,logic::points,2);return (slot&0xffffu)==logic::points;}   // CMP AX,4
    bool object_kept(std::uint32_t object){this->compare32(object,this->gp[ecx]);return object!=this->gp[ecx];}   // CMP [ESI+8],ECX
    void remove(Kept &kept) {                                                                            // MOV EAX,EBX; CALL 17F710; MOV EAX,EBX; MOV EDX,EBP; CALL 16E8A0
        auto &gp=this->gp;
        gp[eax]=kept.handle;routed(0x17ecf8u,0x17f710u,0u,kept);
        gp[eax]=kept.handle;gp[edx]=kept.base;routed(0x17ed01u,0x16e8a0u,0u,kept);
        this->reached(Branch::removed);
    }
    // The next contrail.
    std::uint32_t next_start(std::uint32_t handle,std::uint32_t index) {                                // CMP EBX,-1; XOR EAX,EAX / LEA EAX,[EDI+1]
        auto &gp=this->gp;
        this->compare32(handle,logic::none);
        if(handle==logic::none)return gp[eax]=this->flags.logic(0u,4);
        return gp[eax]=index+1u;
    }
    std::uint32_t not_found(){return this->gp[esi]=this->flags.logic(logic::none,4);}                        // OR ESI,-1
    bool counted(std::uint32_t index){this->test32(index);return std::int32_t(index)>=0;}                    // TEST EAX,EAX; JL
    bool below(std::uint32_t index,std::uint32_t count){this->compare32(index,count);return std::int32_t(index)<std::int32_t(count);}   // CMP EAX,EDI; JGE
    std::uint32_t bit_of(std::uint32_t index) {                                                          // MOV ECX,EAX; AND ECX,1F; MOV EBX,1; SHL EBX,CL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(index&31u,4);gp[ebx]=fl.shift(1u,gp[ecx],4);
        return gp[ebx];
    }
    std::uint32_t word_of(std::uint32_t index){return this->gp[ecx]=this->flags.right(index,5u,4,true);}    // MOV ECX,EAX; SAR ECX,5
    bool active(std::uint32_t word,std::uint32_t bit){return this->flags.logic(word&bit,4)!=0u;}             // TEST [EDX+ECX*4],EBX
    std::uint32_t found_at(std::uint32_t index){this->reached(Branch::next_found);return this->gp[esi]=index;}   // MOV ESI,EAX
    std::uint32_t next_index(std::uint32_t index){return this->gp[eax]=this->flags.increment(index,4,false);}   // INC EAX
    std::uint32_t next_none(){return this->gp[eax]=this->flags.logic(logic::none,4);}                       // OR EAX,-1
    std::uint32_t element_offset_next(std::uint32_t size,std::uint32_t found_index){return this->gp[edx]=this->flags.product(size,found_index);}   // IMUL EDX,ESI
    void finished(bool full) {                                                                           // (POP EDI; POP ESI); POP EBP; POP EBX; ADD ESP,10
        auto &gp=this->gp;
        if(full){gp[edi]=this->pop(0x17ed69u);gp[esi]=this->pop(0x17ed6au);}
        gp[ebp]=this->pop(0x17ed6bu);gp[ebx]=this->pop(0x17ed6cu);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;auto &x=this->s.xmm;
        Memory<Adapter> memory{*this};
        logic::contrails_update::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi],low(x[1]),low(x[2]),low(x[3]),low(x[4]),low(x[5])});
        this->ret(0x17ed70u,4u);this->publish(0x17ed70u);
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x17ead0u,0x17ed70u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_contrails_update_detail
