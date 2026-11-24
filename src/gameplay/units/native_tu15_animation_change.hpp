#pragma once
// Boundary adapter for the animation change, 111B10
// (native_tu15_animation_change_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the flags, seven
// arguments pushed (RET 0x1C), the answer in AL; ECX pushed (the block's
// field in its slot), EBX, EBP, ESI and EDI saved; 1D5320 (EAX the name, ECX
// the block; five pushed, RET 0x14), 115E20 and 1D4350 (EAX), 115E90 (EAX
// the name, ECX the current name, DL the switch; two pushed, RET 8; the
// blend in XMM0), BC380 (ESI the unit; the blend pushed, RET 4) and 115AF0
// (the unit pushed, RET 4) direct, every register each returns coming back.
// Each access publishes its original instruction.
// HALO2_REFERENCE_ANIMATION_CHANGE=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_CHANGE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_animation_change_systems.hpp"

namespace halo2::native_animation_change_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_animation_change;
using Access=logic::animation_change::Access;
using Kept=logic::animation_change::Kept;
using Outcome=logic::animation_change::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ANIMATION_CHANGE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ANIMATION_CHANGE");return v;}
enum Branch : unsigned {refused, unchanged, changed, partner, switched_on, blended, branch_count};
static_assert(unsigned(Outcome::changed)==Branch::changed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Animation change"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x111b11u,0x111b17u,0x111b1du,0x111b2au,0x111b2eu,0x111b33u,0x111b3au,0x111b3eu,0x111b43u,0x111b51u,
    0x111b65u,0x111b80u,0x111b8cu,0x111b95u,0x111ba4u,0x111bb7u,0x111bbfu,0x111bd8u,0x111bdcu,0x111be0u,0u,0x111bf3u,0x111bfau,0x111c0bu,
    0x111c19u,0x111c21u,0x111c37u,0x111c37u,0x111c31u,0x111c45u,0x111c4cu,0x111c59u,0x111c60u,0x111c6au,0x111c75u,0x111c79u,0x111c83u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::finish_unit)+1u);
template<class State,class Decide,class Prepare,class SwitchOn,class Blend,class Take,class Finish> struct Adapter : Port<State> {
    Decide &decide_child;Prepare &prepare_child;SwitchOn &switch_child;Blend &blend_child;Take &take_child;Finish &finish_child;
    std::uint32_t depth{};      // bytes below the return address
    bool clearing_byte{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Decide &d,Prepare &p,SwitchOn &w,Blend &b,Take &k,Finish &f)
        :Port<State>(s,c,t,{}),decide_child(d),prepare_child(p),switch_child(w),blend_child(b),take_child(k),finish_child(f){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        if(a==second_flags || a==second_store)return clearing_byte?0x111c3cu:0x111c37u;
        if(a==keep_argument)return 0x111bedu;
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:case field:case partner_name:case b_argument:case a_argument:gp[ecx]=v;break;
        case datums:case name_argument:case name_again:case name_final:case blend_load:gp[edx]=v;break;
        case unit_argument:case object_address:case mode_argument:case mode_again:case c_argument:case finish_unit:gp[eax]=v;break;
        case block_offset:gp[esi]=logic::widen16(v);break;
        case current:gp[ebp]=v;break;
        case second_offset:gp[edi]=logic::widen16(v);break;
        case cutscene:case keep_argument:case keep_again:set_low_byte(gp[eax],v);break;
        case switch_load:case still:set_low_byte(gp[ecx],v);break;
        case switch_byte:set_low_byte(gp[edx],v);break;
        case blend_unit:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        switch(n) {
        case 0u:put(0x111b10u,gp[ecx]);break;                                                           // PUSH ECX
        case 1u:put(0x111b1au,gp[ebx]);gp[ebx]=gp[eax];break;                                           // PUSH EBX; MOV EBX,EAX
        case 2u:put(0x111b32u,gp[esi]);break;                                                           // PUSH ESI
        default:put(0x111b42u,gp[edi]);break;                                                           // PUSH EDI
        }
    }
    void indexing(std::uint32_t unit_handle) {                                                          // AND EAX,FFFF; PUSH EBP; LEA
        auto &gp=this->gp;
        const auto index=this->flags.logic(unit_handle&0xffffu,4);
        put(0x111b26u,gp[ebp]);gp[eax]=3u*index;
    }
    std::uint32_t placed(std::uint32_t offset,std::uint32_t unit_address,unsigned which) {             // ADD ESI,EAX; ADD EDI,EAX
        return this->gp[which?edi:esi]=this->flags.plus(offset,unit_address,4);
    }
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void tested8(std::uint32_t v){this->test8(v);}
    std::uint32_t flagged(std::uint32_t flags,std::uint32_t bit) {                                     // OR EBX,bit
        if(bit==logic::flags::partner)this->reached(Branch::partner);
        return this->gp[ebx]=this->flags.logic(flags|bit,4);
    }
    void clear_ecx(){this->gp[ecx]=this->flags.logic(0u,4);}                                           // XOR ECX,ECX
    void partnered(std::uint32_t){}
    void counted(std::uint32_t k){this->gp[ecx]=this->flags.increment(k-1u,4,false);this->compare32(k,logic::partner_count);}   // INC ECX; CMP ECX,4
    void no_partner(){this->gp[ecx]=this->flags.logic(logic::none,4);}                                 // OR ECX,-1
    std::uint32_t answered(std::uint32_t flags,std::uint32_t al){return this->gp[ebx]=(flags&0xffffff00u)|al;}   // MOV BL,1
    void pushing_request(std::uint32_t b,const Kept &kept) {                                            // PUSH 3F; PUSH EBX; PUSH ECX
        put(0x111bbbu,logic::request_mask);put(0x111bbdu,kept.flags);put(0x111bbeu,b);
    }
    std::uint32_t decide(std::uint32_t a,std::uint32_t mode,std::uint32_t name,Kept &kept) {           // PUSH ECX; PUSH EAX; MOV EAX,EDX; MOV ECX,ESI; CALL 1D5320; MOV BL,AL; TEST BL,BL
        auto &gp=this->gp;
        put(0x111bc3u,a);put(0x111bc4u,mode);gp[eax]=name;gp[ecx]=kept.block;
        this->call(0x111bc9u,0x111bceu,0x1d5320u,decide_child);depth-=20u;
        gp[ebx]=(gp[ebx]&0xffffff00u)|(gp[eax]&0xffu);
        refresh(kept);
        this->test8(gp[ebx]);
        if(!(gp[ebx]&0xffu))this->reached(Branch::refused);
        return gp[ebx];
    }
    void changed(bool change) {                                                                         // MOV AL,1 or XOR AL,AL
        if(change)set_low_byte(this->gp[eax],1u);else set_low_byte(this->gp[eax],this->flags.logic(0u,1));
    }
    void switching(std::uint32_t unit_handle,Kept &kept) {                                              // CALL 115E20
        this->gp[eax]=unit_handle;
        this->call(0x111c25u,0x111c2au,0x115e20u,prepare_child);
        refresh(kept);
    }
    std::uint32_t switch_on(Kept &kept) {                                                               // CALL 1D4350; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x111c2au,0x111c2fu,0x1d4350u,switch_child);
        refresh(kept);
        this->test8(gp[eax]);
        clearing_byte=!(gp[eax]&0xffu);
        if(!clearing_byte)this->reached(Branch::switched_on);
        return gp[eax];
    }
    std::uint32_t switched_byte(std::uint32_t byte,bool on) {                                           // OR byte [EDI],2 or AND byte [EDI],FD
        return on?this->flags.logic((byte|logic::flags::switched)&0xffu,1):this->flags.logic(byte&~logic::flags::switched&0xffu,1);
    }
    void pushing_blend(std::uint32_t c,std::uint32_t name){put(0x111c49u,c);this->gp[eax]=name;}      // PUSH EAX; MOV EAX,EDX
    std::uint32_t blend(std::uint32_t,Kept &kept) {                                                     // PUSH 0; MOV ECX,EBP; CALL 115E90
        auto &gp=this->gp;
        put(0x111c50u,0u);gp[ecx]=kept.current;
        this->call(0x111c54u,0x111c59u,0x115e90u,blend_child);depth-=8u;
        refresh(kept);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    void compared_float(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS XMM0,[46A0E8]
    bool still(std::uint32_t switches) {                                                                // XOR ECX,ECX; (MOV CL); SHR ECX,4; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        fl.logic(0u,4);
        gp[ecx]=fl.right(switches&0xffu,logic::block::still_bit,4,false);fl.logic(gp[ecx]&1u,1);
        return gp[ecx]&1u;
    }
    void take_blend(std::uint32_t unit_handle,std::uint32_t stored,Kept &kept) {                        // PUSH EDX; CALL BC380
        this->gp[esi]=unit_handle;put(0x111c7du,stored);
        this->call(0x111c7eu,0x111c83u,0xbc380u,take_child);depth-=4u;
        refresh(kept);
        this->reached(Branch::blended);
    }
    void finish(std::uint32_t unit_handle,Kept &kept) {                                                 // PUSH EAX; CALL 115AF0
        put(0x111c87u,unit_handle);
        this->call(0x111c88u,0x111c8du,0x115af0u,finish_child);depth-=4u;
        refresh(kept);
    }
    void restoring() {                                                                                    // POP EDI, ESI, EBP; MOV AL,BL; POP EBX; POP ECX
        auto &gp=this->gp;
        gp[edi]=take(0x111c8du);gp[esi]=take(0x111c8eu);gp[ebp]=take(0x111c8fu);
        set_low_byte(gp[eax],gp[ebx]);gp[ebx]=take(0x111c92u);gp[ecx]=take(0x111c93u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        logic::animation_change::request(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        this->ret(0x111c94u,0x1cu);this->publish(0x111c94u);
    }
};
template<class State,class Reference,class Decide,class Prepare,class SwitchOn,class Blend,class Take,class Finish>
bool bridge(State &s,Reference reference,Decide decide,Prepare prepare,SwitchOn switch_on,Blend blend,Take take,Finish finish) {
    return run_owner(s,0x111b10u,0x111c94u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,Decide,Prepare,SwitchOn,Blend,Take,Finish>(v,c,t,decide,prepare,switch_on,blend,take,finish);});
}
} // namespace halo2::native_animation_change_detail
