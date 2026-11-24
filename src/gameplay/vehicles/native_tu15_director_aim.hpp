#pragma once
// Boundary adapter for the director aim, 245060
// (native_tu15_director_aim_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the unit, EAX the forward,
// the record on the stack (RET 4); 0xB8 bytes of frame with EBX, EBP, ESI
// and EDI saved below them, read from the ESP each child left; 11FA50 (ECX
// the forward, EDX the up), CD4A0, BC200 (0 pushed, RET 4), BCDF0 (2
// pushed, RET 4), BAD60 (EBX the marker; three pushed, RET C), BBE00,
// 144BF0 and 175380 through the unit's dispatch, every register each returns
// coming back (EDX and XMM6/XMM7 kept across 11FA50 as the original keeps
// them). XMM0 to XMM7 hold what the original's scalar moves and arithmetic
// leave in them at each call. Each access publishes its original
// instruction.
// HALO2_REFERENCE_DIRECTOR_AIM=1 restores the translated body.
// HALO2_VERIFY_DIRECTOR_AIM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/vehicles/native_tu15_director_aim_systems.hpp"

namespace halo2::native_director_aim_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_director_aim;
using Access=logic::director_aim::Access;
using Kept=logic::director_aim::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DIRECTOR_AIM");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DIRECTOR_AIM");return v;}
enum Branch : unsigned {unit, armed, marker, matrix, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Director aim"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
inline constexpr std::uint32_t sites[]{0x245063u,0x245071u,0x245078u,0x245080u,0x24508au,0x24508fu,0x245091u,0x245094u,0x245097u,0x24509au,
    0x24509du,0x2450a2u,0x2450a4u,0x2450b2u,0x2450b4u,0x2450b7u,0x2450bau,0x2450c0u,0x2450c3u,0x2450d6u,0x2450e6u,0x2450e9u,0x2450edu,
    0x245109u,0x24510du,0x245124u,0x24512eu,0x245130u,0x24513eu,0x245142u,0x245146u,0x24514du,0x245159u,0u,0u,0x2451e7u,
    0x2451fau,0x2451ffu,0x245204u,0x245208u,0x24520eu,0x245214u,0x24521au,0x24522cu,0x24523au,0x245240u,0x245244u,0x245256u,0x245264u,
    0x245277u,0x24528eu,0x245298u,0x24529du,0x2452a2u,0x2452a6u,0x2452acu,0x2452c1u,0x2452cfu,0x2452d3u,0x2452e0u,0x2452eeu,0x2452ffu,
    0x245305u,0x24530eu,0x245317u,0x24532cu,0x245331u,0x245336u,0x24533au,0x24533fu,0x245349u,0x245357u,0x245365u,0x24536bu,0x24536fu,
    0x245381u,0x2453a3u,0x2453a8u,0x2453adu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::state_set)+1u);
inline constexpr std::uint32_t marker_loads[]{0x24517fu,0x245186u,0x24518du,0x245196u,0x2451a0u,0x2451aau,0x2451b3u,0x2451bdu,0x2451c7u};
inline constexpr std::uint32_t marker_stores[]{0x245194u,0x24519du,0x2451a7u,0x2451b1u,0x2451bau,0x2451c4u,0x2451ceu,0x2451d1u,0x2451d4u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before them)
    unsigned markers_loaded{},markers_stored{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        kept.unit_handle=gp[ebx];kept.record_address=gp[ebp];kept.position=gp[esi];kept.forward=gp[edi];kept.up=gp[edx];
        kept.up_y=low(x[7]);kept.up_z=low(x[6]);
    }
    void child(std::uint32_t target,std::uint32_t pc,std::uint32_t next,Kept &kept) {
        auto route=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,next,target,route);
        refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        if(a==marker_load)return marker_loads[markers_loaded];
        if(a==marker_store)return marker_stores[markers_stored++];
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case zero_load:case zero_z:case forward_x_in:case forward_y_in:case headers_load:case unit_again:case definition_load:
        case seats_load:gp[edx]=v;break;
        case record_argument:gp[ebp]=v;break;
        case zero_x:case zero_y:gp[esi]=v;break;
        case reach_load:this->s.xmm[0]=lane0(v);break;                                                   // MOVSS XMM0
        case forward_z_in:case datums_load:case weapon_tag:case unit_reload:case seat_flags:gp[eax]=v;break;
        case unit_load:case instances_load:case weapon_load:gp[ecx]=v;break;
        case parent_load:gp[ebx]=v;break;
        case seat_load:gp[ecx]=sx16(v);break;
        case marker_load:{static constexpr unsigned regs[]{ecx,edx,eax};gp[regs[markers_loaded++%3]]=v;break;}
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // XORPS XMM0; SUB ESP,B8; PUSH EBX; PUSH EBP
        auto &gp=this->gp;
        this->s.xmm[0]=_mm_setzero_ps();
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
        this->push(0x24506fu,gp[ebx]);this->push(0x245070u,gp[ebp]);depth=0u-8u;
    }
    std::uint32_t cleared()const{return low(this->s.xmm[0]);}
    void unit_kept(Kept &kept){kept.unit_handle=this->gp[ebx]=this->gp[ecx];}                          // MOV EBX,ECX
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==1u){this->push(0x245089u,gp[esi]);depth=0u-4u;}else{this->push(0x2450acu,gp[edi]);depth=0u;}   // PUSH ESI / PUSH EDI
    }
    std::uint32_t offset_address(std::uint32_t record_address){return this->gp[ecx]=record_address+logic::record::offset;}   // LEA ECX,[EBP+10]
    std::uint32_t forward_input()const{return this->gp[eax];}
    std::uint32_t forward_address(std::uint32_t record_address) {                                       // LEA EDI,[EBP+2C]; MOV ECX,EDI
        auto &gp=this->gp;
        return gp[ecx]=gp[edi]=record_address+logic::record::forward;
    }
    void up_address(std::uint32_t record_address){this->gp[edx]=record_address+logic::record::up;}      // LEA EDX,[EBP+38]
    void completed(Kept &kept,unsigned n) {                                                              // (MOV ECX,EDI; LEA EDX,[EBP+38]); CALL 11FA50
        auto &gp=this->gp;
        if(n==1u){gp[ecx]=kept.forward;gp[edx]=kept.record_address+logic::record::up;}
        child(0x11fa50u,n?0x245293u:0x2450c8u,n?0x245298u:0x2450cdu,kept);
    }
    bool unit_compared(std::uint32_t handle) {                                                           // CMP EBX,-1
        this->compare32(handle,logic::none);
        if(handle!=logic::none)this->reached(Branch::unit);
        return handle!=logic::none;
    }
    std::uint32_t indexed(std::uint32_t handle) {                                                       // MOV EAX,EBX; AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle&logic::handle_mask,4);gp[ecx]=3u*gp[eax];
        return gp[eax];
    }
    void positioned(Kept &kept) {                                                                        // LEA ESI,[EBP+4]; MOV EAX,ESI; MOV ECX,EBX; CALL CD4A0
        auto &gp=this->gp;
        gp[esi]=gp[eax]=kept.record_address+logic::record::position;gp[ecx]=kept.unit_handle;
        child(0xcd4a0u,0x2450f8u,0x2450fdu,kept);
    }
    void lit(Kept &kept) {                                                                               // PUSH 0; LEA ECX,[EBP+44]; MOV EAX,EBX; CALL BC200
        auto &gp=this->gp;
        put(0x2450fdu,0u);gp[ecx]=kept.record_address+logic::record::lights;gp[eax]=kept.unit_handle;
        child(0xbc200u,0x245104u,0x245109u,kept);depth-=4u;
    }
    bool parent_compared(std::uint32_t parent){this->compare32(parent,logic::none);return parent!=logic::none;}   // CMP EBX,-1
    std::uint32_t armed(Kept &kept) {                                                                    // PUSH 2; MOV EAX,EBX; CALL BCDF0; TEST EAX,EAX
        auto &gp=this->gp;
        put(0x245119u,logic::weapon_slot);gp[eax]=kept.unit_handle;
        child(0xbcdf0u,0x24511du,0x245122u,kept);depth-=4u;
        this->test32(gp[eax]);
        if(gp[eax])this->reached(Branch::armed);
        return gp[eax];
    }
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        static_assert((1u<<4)==logic::tag_size);
        return this->gp[eax]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    std::uint32_t seat_offset(std::uint32_t seat){return this->gp[ecx]=this->flags.product(sx16(seat),logic::definition::seat_size);}   // IMUL ECX,ECX,B0
    bool marked(std::uint32_t flags_value) {                                                             // SHR EAX,7; TEST AL,1
        auto &gp=this->gp;
        gp[eax]=this->flags.right(flags_value,logic::definition::marked_bit,4,false);this->flags.logic(gp[eax]&1u,1);
        return gp[eax]&1u;
    }
    bool marker_found(Kept &kept) {                                                                      // PUSH 1, F0000DB, EBX; XOR CL,CL; LEA EBX,[ESP+64]; CALL BAD60; TEST AX,AX
        auto &gp=this->gp;
        put(0x245163u,1u);put(0x245165u,logic::marker_name);put(0x24516au,kept.unit_handle);
        set_low_byte(gp[ecx],this->flags.logic(0u,1));gp[ebx]=local(logic::local::marker);
        child(0xbad60u,0x245171u,0x245176u,kept);depth-=12u;
        this->flags.logic(gp[eax]&0xffffu,2);
        if(gp[eax]&0xffffu)this->reached(Branch::marker);
        return gp[eax]&0xffffu;
    }
    void parent_positioned(Kept &kept) {                                                                 // LEA ESI,[ESP+4C]; MOV EAX,EBX; CALL BBE00
        auto &gp=this->gp;
        gp[esi]=local(logic::local::position);gp[eax]=kept.unit_handle;
        child(0xbbe00u,0x2451e2u,0x2451e7u,kept);
    }
    void node_matrix(std::uint32_t weapon_address,Kept &kept) {                                         // LEA EAX,[ECX+7C]; ADD ECX,70; LEA EDX,[ESP+18]; CALL 144BF0
        auto &gp=this->gp;
        gp[eax]=weapon_address+logic::weapon::node_extra;gp[ecx]=this->flags.plus(weapon_address,logic::weapon::node,4);
        gp[edx]=local(logic::local::matrix);
        child(0x144bf0u,0x2451f5u,0x2451fau,kept);
        this->reached(Branch::matrix);
    }
    void rotated(std::uint32_t vx,std::uint32_t up_x_term,std::uint32_t up_y_term,std::uint32_t tz,std::uint32_t left_x_term,const logic::Rows &m) {
        auto &x=this->s.xmm;                                                                             // the MULSS/ADDSS registers before 11FA50
        x[0]=lane0(vx);x[1]=lane0(up_x_term);x[2]=lane0(up_y_term);x[3]=lane0(tz);x[4]=lane0(left_x_term);x[6]=lane0(m.up_z);x[7]=lane0(m.up_y);
    }
    void transposed(std::uint32_t left_z_uy,std::uint32_t forward_z_ux,std::uint32_t forward_y_ux,std::uint32_t ux,std::uint32_t uy,
                    std::uint32_t uz,std::uint32_t out_z,std::uint32_t out_y) {                         // the MULSS/ADDSS registers at the end
        auto &x=this->s.xmm;
        x[0]=lane0(left_z_uy);x[1]=lane0(forward_z_ux);x[2]=lane0(forward_y_ux);x[3]=lane0(ux);x[4]=lane0(uy);x[5]=lane0(uz);
        x[6]=_mm_move_ss(x[6],lane0(out_z));x[7]=_mm_move_ss(x[7],lane0(out_y));
    }
    void finished(Kept &kept) {                                                                          // MOV EAX,EBP; CALL 175380; POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,B8
        auto &gp=this->gp;
        gp[eax]=kept.record_address;
        child(0x175380u,0x2453b6u,0x2453bbu,kept);
        gp[edi]=this->pop(0x2453bbu);gp[esi]=this->pop(0x2453bcu);gp[ebp]=this->pop(0x2453bdu);gp[ebx]=this->pop(0x2453beu);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;auto &x=this->s.xmm;
        Memory<Adapter> memory{*this};
        logic::director_aim::set_up(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi],gp[edx],low(x[7]),low(x[6])});
        this->ret(0x2453c5u,4u);this->publish(0x2453c5u);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x245060u,0x2453c5u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_director_aim_detail
