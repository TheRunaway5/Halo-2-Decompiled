#pragma once
// Boundary adapter for the unit camera position, CD4A0
// (native_tu15_unit_camera_position_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: ECX the unit's
// handle, EAX the output (RET); 0xA4 bytes of frame with EBX, EBP, ESI and
// EDI saved below them, read from the ESP each child left. Every child goes
// through the unit's dispatch, which binds it in each unit: BBE00 and E3310
// (EAX the unit), 11FA50 (ECX the aim, EDX the up), E18D0 (three pushed,
// RET C), 30BD0 (EAX the move; XMM0), 16C620 (six pushed, RET 18), BAD60
// (EBX the marker, CL 0; three pushed, RET C) and BBF20 (ECX the output);
// every register each returns comes back. XMM0 to XMM7 hold what the
// original's scalar moves and arithmetic leave in them at each call and at
// each exit, which leaves through the original's own epilogue. Each access
// publishes its original instruction.
// HALO2_REFERENCE_UNIT_CAMERA_POSITION=1 restores the translated body.
// HALO2_VERIFY_UNIT_CAMERA_POSITION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_camera_position_systems.hpp"

namespace halo2::native_unit_camera_position_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_camera_position;
using Access=logic::unit_camera_position::Access;
using Exit=logic::unit_camera_position::Exit;
using Marker=logic::unit_camera_position::Marker;
using Kept=logic::unit_camera_position::Kept;
using Plane=logic::unit_camera_position::Plane;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_CAMERA_POSITION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_CAMERA_POSITION");return v;}
enum Branch : unsigned {seated, seat_camera, seat_marker, head, biped, crouched, short_move, first_hit, first_miss, second_miss, pulled,
    beside, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit camera position"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
inline constexpr std::uint32_t sites[]{0xcd4abu,0xcd4b0u,0xcd4c0u,0xcd4c4u,0xcd4c6u,0xcd4d5u,0xcd4d9u,0xcd4ddu,0xcd4e9u,0xcd4fbu,
    0xcd509u,0xcd513u,0xcd52cu,0xcd534u,0xcd53cu,0xcd548u,0xcd558u,0xcd560u,0xcd565u,0xcd56cu,0xcd56eu,0xcd571u,0xcd57au,0xcd57eu,
    0xcd580u,0xcd584u,0xcd587u,0xcd58bu,0xcd58eu,0xcd592u,0xcd59eu,0xcd5a5u,0xcd5beu,0xcd5c4u,0xcd5c8u,0xcd5ceu,0xcd5d4u,0xcd5d9u,
    0xcd5dfu,0xcd5e5u,0xcd5eeu,0xcd5f9u,0xcd606u,0xcd60eu,0xcd61fu,0xcd625u,0xcd636u,0xcd63cu,0xcd64cu,0xcd65bu,0xcd661u,0xcd669u,
    0xcd673u,0xcd677u,0xcd67du,0xcd687u,0xcd68cu,0xcd692u,0xcd69cu,0xcd6a1u,0xcd6a9u,0xcd6adu,0xcd6b1u,0xcd6b5u,0xcd6b9u,0xcd6bdu,
    0xcd6e6u,0xcd6ecu,0xcd6f2u,0xcd6f8u,0xcd6feu,0xcd704u,0xcd70au,0xcd710u,0xcd71eu,0xcd72cu,0xcd73au,0xcd74cu,0xcd75eu,0xcd77fu,
    0xcd78du,0xcd791u,0xcd79fu,0xcd7a4u,0xcd7b3u,0xcd7c2u,0xcd7c6u,0xcd7cau,0xcd7ceu,0xcd7d1u,0xcd7d4u,0xcd7e6u,0xcd7f9u,0xcd7fdu,
    0xcd80du,0xcd817u,0xcd81du,0xcd830u,0xcd837u,0xcd83eu,0xcd845u,0xcd848u,0xcd84bu,0xcd868u,0xcd879u,0xcd87cu,0xcd882u,0xcd885u,
    0xcd895u,0xcd899u,0xcd8a3u,0xcd8a7u,0xcd8b0u,0xcd8c3u,0xcd8cau,0xcd8dcu,0xcd8e3u,0xcd8eau,0xcd8f1u,0xcd8f3u,0xcd8f6u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::out_camera_z)+1u);
// Each exit's remaining pops (ESI, EBP, EBX after EDI, which the unchanged
// exit pops first) and its RET.
struct Epilogue {std::uint32_t edi,esi,add,ret;};
inline constexpr Epilogue epilogues[]{{0u,0xcd7b8u,0xcd7bbu,0xcd7c1u},{0u,0xcd7d7u,0xcd7dau,0xcd7e0u},{0u,0xcd84eu,0xcd851u,0xcd857u},
    {0xcd8f9u,0xcd8fau,0xcd8fdu,0xcd903u}};
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
        kept.handle=gp[ebx];kept.base=gp[ebp];kept.output=gp[esi];kept.unit_address=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers_load:case definition_load:case parent_load:case biped_tag:case eye_y_copy:case aim_y:case hit_z_copy:
        case seat_definition:case parent_tag:case marker_x:case camera_y:case moved_z_final:gp[eax]=v;break;
        case datums_load:case biped_definition:case controlled_object:case parent_seats:gp[ebp]=v;break;
        case unit_load:case controlled_load:case parent_reload:gp[edi]=v;break;
        case unit_tag:case eye_x_copy:case aim_x:case hit_y_copy:case seat_marker:case marker_z:case camera_x:case moved_y_final:gp[edx]=v;break;
        case instances_load:case eye_z_copy:case aim_z:case hit_x_copy:case seats_load:case headers_again:case datums_again:
        case parent_object:case camera_test:case camera_marker:case marker_y:case camera_z:case moved_x_final:gp[ecx]=v;break;
        case instances_again:case parent_definition:gp[ebx]=v;break;
        case object_flags:case object_type:case parent_type:set_low_byte(gp[edx],v);break;                 // MOV DL
        case controlled_seat:gp[eax]=low_word(gp[eax],v);break;                                            // MOV AX,word
        case seat_word:gp[edx]=low_word(gp[edx],v);break;                                                  // MOV DX,word
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the unit.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;}   // SUB ESP,A4
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0xcd4a6u,0xcd4a7u,0xcd4a8u,0xcd4bfu};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t output(){return this->gp[esi]=this->gp[eax];}                                              // MOV ESI,EAX
    std::uint32_t handle(){return this->gp[ebx]=this->gp[ecx];}                                              // MOV EBX,ECX
    std::uint32_t indexed(std::uint32_t handle_value) {                                                  // MOV EAX,EBX; AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle_value&logic::handle_mask,4);gp[ecx]=3u*gp[eax];
        return gp[eax];
    }
    std::uint32_t tag_offset_edx(std::uint32_t tag){auto &fl=this->flags;return this->gp[edx]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);}
    std::uint32_t tag_offset_eax(std::uint32_t tag){auto &fl=this->flags;return this->gp[eax]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);}
    bool seated(std::uint32_t parent){this->compare32(parent,logic::none);if(parent!=logic::none)this->reached(Branch::seated);return parent!=logic::none;}
    // A seated unit.
    void seated_position(Kept &kept){this->gp[ecx]=kept.output;routed(0xcd863u,0xbbf20u,0u,kept);}           // MOV ECX,ESI; CALL BBF20
    bool in_seat(std::uint32_t seat_value){this->flags.minus(seat_value,0xffffu,2);return (seat_value&0xffffu)!=0xffffu;}   // CMP DX,-1
    std::uint32_t parent_index(std::uint32_t parent_handle) {                                           // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        return this->gp[eax]=3u*this->flags.logic(parent_handle&logic::handle_mask,4);
    }
    void seat_widened(std::uint32_t){this->gp[eax]=sx16(this->gp[edx]);}                                     // MOVSX EAX,DX
    std::uint32_t seat_entry(std::uint32_t,std::uint32_t seats) {                                       // IMUL EAX,EAX,B0; ADD EAX,EBP
        auto &gp=this->gp;auto &fl=this->flags;
        return gp[eax]=fl.plus(fl.product(gp[eax],logic::seat_size),seats,4);
    }
    bool vehicle(std::uint32_t type){this->flags.minus(type,logic::vehicle,1);return (type&0xffu)==logic::vehicle;}   // CMP DL,1
    bool named(std::uint32_t name){this->test32(name);return name!=0u;}                                       // TEST ECX,ECX
    void marker_query(Kept &kept,std::uint32_t name,std::uint32_t owner,Marker source) {                 // (PUSH 1; PUSH name; PUSH owner); XOR CL,CL; LEA EBX,[ESP+50]; CALL BAD60
        auto &gp=this->gp;
        std::uint32_t call_pc;
        if(source==Marker::camera){put(0xcd8cdu,1u);put(0xcd8cfu,name);put(0xcd8d0u,owner);call_pc=0xcd8d7u;this->reached(Branch::seat_camera);}
        else {
            if(source==Marker::seat){put(0xcd821u,1u);put(0xcd823u,name);this->reached(Branch::seat_marker);}
            else{put(0xcd858u,1u);put(0xcd85au,name);this->reached(Branch::head);}
            put(0xcd824u,owner);call_pc=0xcd82bu;
        }
        set_low_byte(gp[ecx],this->flags.logic(0u,1));gp[ebx]=local(logic::local::marker);
        routed(call_pc,0xbad60u,12u,kept);
    }
    void leaving_marker(){this->gp[edi]=this->pop(0xcd847u);depth-=4u;}                                      // POP EDI
    // A unit that is not a biped.
    bool controlling(std::uint32_t flags_value) {                                                        // SHR DL,2; TEST DL,1
        auto &gp=this->gp;
        set_low_byte(gp[edx],this->flags.right(flags_value&0xffu,logic::unit::controlled_bit,1,false));this->flags.logic(gp[edx]&1u,1);
        return gp[edx]&1u;
    }
    bool not_biped(std::uint32_t type){this->test8(type);return (type&0xffu)!=0u;}                           // TEST DL,DL
    void parent_tested(std::uint32_t parent){this->compare32(parent,logic::none);}                          // CMP EAX,-1
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                       // CMP EDI,EAX
    std::uint32_t controlled_index(std::uint32_t controlled) {                                          // AND EDI,FFFF; LEA ECX,[EDI+EDI*2]
        auto &gp=this->gp;
        gp[edi]=this->flags.logic(controlled&logic::handle_mask,4);
        return gp[ecx]=3u*gp[edi];
    }
    bool in_seat_word(std::uint32_t seat_value){this->flags.minus(seat_value,0xffffu,2);return (seat_value&0xffffu)!=0xffffu;}   // CMP AX,-1
    void seat_widened_edx(std::uint32_t){this->gp[edx]=sx16(this->gp[eax]);}                                 // MOVSX EDX,AX
    std::uint32_t seat_offset(std::uint32_t){return this->gp[edx]=this->flags.product(this->gp[edx],logic::seat_size);}   // IMUL EDX,EDX,B0
    // A biped.
    void biped_position(Kept &kept){this->gp[eax]=kept.handle;routed(0xcd519u,0xbbe00u,0u,kept);this->reached(Branch::biped);}   // MOV EAX,EBX; CALL BBE00
    bool uncrouched(Kept &kept) {                                                                        // MOV EAX,EBX; CALL E3310; TEST AL,AL; XORPS XMM0,XMM0
        auto &gp=this->gp;
        gp[eax]=kept.handle;routed(0xcd520u,0xe3310u,0u,kept);
        this->test8(gp[eax]);this->s.xmm[0]=_mm_setzero_ps();
        if(!low_byte(gp[eax]))this->reached(Branch::crouched);
        return low_byte(gp[eax])!=0u;
    }
    void eye_registers(std::uint32_t crouch,std::uint32_t height,std::uint32_t crouched_value) {
        auto &x=this->s.xmm;x[0]=lane0(crouch);x[1]=lane0(height);x[2]=lane0(crouched_value);
    }
    void eye_copying(Kept &kept){this->gp[ecx]=kept.output;}                                                 // MOV ECX,ESI
    std::uint32_t aim_address(Kept &kept){return this->gp[edi]=this->flags.plus(kept.unit_address,logic::unit::aim,4);}   // ADD EDI,168
    void aim_arguments(){auto &gp=this->gp;gp[edx]=local(logic::local::end);gp[ecx]=local(logic::local::ray);}   // LEA EDX,[ESP+28]; LEA ECX,[ESP+1C]
    void move_camera(Kept &kept) {                                                                       // CALL 11FA50; MOV EAX,ECX; PUSH EDX; PUSH EAX; LEA ECX,[ESP+18]; PUSH ECX; MOV EAX,EBX; CALL E18D0
        auto &gp=this->gp;
        routed(0xcd5a9u,0x11fa50u,0u,kept);
        gp[eax]=gp[ecx];put(0xcd5b0u,gp[edx]);put(0xcd5b1u,gp[eax]);
        gp[ecx]=local(logic::local::moved);put(0xcd5b6u,gp[ecx]);gp[eax]=kept.handle;
        routed(0xcd5b9u,0xe18d0u,12u,kept);
    }
    void move_address(){this->gp[eax]=local(logic::local::move);}                                             // LEA EAX,[ESP+34]
    std::uint32_t measured(Kept &kept,std::uint32_t move_z) {                                           // CALL 30BD0
        this->s.xmm[0]=lane0(move_z);
        routed(0xcd5f4u,0x30bd0u,0u,kept);
        return low(this->s.xmm[0]);
    }
    bool beyond(std::uint32_t,std::uint32_t epsilon) {                                                   // COMISS XMM0,[46A0E8]; JBE
        auto &fl=this->flags;
        fl.scalar_compare(this->s.xmm[0],lane0(epsilon),false);
        const bool far=!(fl.cf||fl.zf);
        if(!far)this->reached(Branch::short_move);
        return far;
    }
    // The rays.
    void ray_result_pushed(){auto &gp=this->gp;gp[edx]=local(logic::local::result);put(0xcd618u,gp[edx]);put(0xcd619u,logic::none);}   // LEA EDX; PUSH EDX; PUSH -1
    void ray_pushed(){auto &gp=this->gp;put(0xcd62bu,logic::none);gp[eax]=local(logic::local::ray);put(0xcd635u,gp[eax]);}        // PUSH -1; LEA EAX; PUSH EAX
    void origin_pushed(Kept &kept){put(0xcd642u,kept.output);put(0xcd647u,logic::ray_flags);}                 // PUSH ESI; PUSH 4808C2D
    bool cast(Kept &kept,bool second,std::uint32_t reach,std::uint32_t scaled_z) {                       // CALL 16C620; TEST AL,AL
        auto &gp=this->gp;
        if(!second){this->s.xmm[0]=lane0(reach);this->s.xmm[1]=lane0(scaled_z);}
        routed(second?0xcd6d9u:0xcd652u,0x16c620u,24u,kept);
        this->test8(gp[eax]);
        const bool hit=low_byte(gp[eax])!=0u;
        if(!second)this->reached(hit?Branch::first_hit:Branch::first_miss);
        else if(!hit)this->reached(Branch::second_miss);
        return hit;
    }
    void ended(std::uint32_t t,std::uint32_t end_z){auto &x=this->s.xmm;x[0]=lane0(t);x[1]=lane0(end_z);}
    void second_ray_pushed() {                                                                           // LEA ECX; PUSH ECX; PUSH -1 x2; LEA EDX; PUSH EDX; LEA EAX; PUSH EAX; PUSH 4808C2D
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::result);put(0xcd6c5u,gp[ecx]);put(0xcd6c6u,logic::none);put(0xcd6c8u,logic::none);
        gp[edx]=local(logic::local::move);put(0xcd6ceu,gp[edx]);
        gp[eax]=local(logic::local::end);put(0xcd6d3u,gp[eax]);put(0xcd6d4u,logic::ray_flags);
    }
    bool past(std::uint32_t camera_along,std::uint32_t hit_along,const Plane &p) {                       // COMISS XMM1,XMM0; JBE
        auto &x=this->s.xmm;auto &fl=this->flags;
        x[0]=lane0(hit_along);x[1]=lane0(camera_along);x[2]=lane0(p.margin);x[3]=lane0(p.dx);x[4]=lane0(p.px);x[5]=lane0(p.py);
        x[6]=lane0(p.dz);x[7]=lane0(p.dy);
        fl.scalar_compare(x[1],x[0],false);
        const bool beyond_plane=!(fl.cf||fl.zf);
        this->reached(beyond_plane?Branch::pulled:Branch::beside);
        return beyond_plane;
    }
    void leaving_pulled(){this->gp[edi]=this->pop(0xcd7aau);depth-=4u;}                                      // POP EDI
    void pulled(const Plane &,std::uint32_t out_z,std::uint32_t dx_margin,std::uint32_t dy_margin,std::uint32_t dz_margin) {
        auto &x=this->s.xmm;x[0]=lane0(out_z);x[3]=lane0(dx_margin);x[6]=lane0(dz_margin);x[7]=lane0(dy_margin);
    }
    void leaving_moved(){this->gp[edi]=this->pop(0xcd7d0u);depth-=4u;}                                       // POP EDI
    void finish(Exit exit) {                                                                             // (POP EDI); POP ESI; POP EBP; POP EBX; ADD ESP,A4; RET
        auto &gp=this->gp;
        const auto &e=epilogues[unsigned(exit)];
        if(e.edi)gp[edi]=this->pop(e.edi);
        gp[esi]=this->pop(e.esi);gp[ebp]=this->pop(e.esi+1u);gp[ebx]=this->pop(e.esi+2u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
        this->ret(e.ret,0u);this->publish(e.ret);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::unit_camera_position::position(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0xcd4a0u,0xcd903u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_unit_camera_position_detail
