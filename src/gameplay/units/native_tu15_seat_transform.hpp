#pragma once
// Boundary adapter for the seat transform, D35C0
// (native_tu15_seat_transform_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the unit, the transform and
// the settings pushed (RET 8), the answer in AL; a 0x78-byte frame (the
// marker frame, the unit's address, the answer byte) below the saved EBX,
// EBP, ESI and EDI; BAD60 (the parent, the marker name and 1 pushed, RET C;
// CL 0, EBX the marker frame) and BBE00 (EAX the object, ESI the transform's
// +0x28) direct, every register each returns coming back; the matrix copied
// as REP MOVSD copies it, in the direction flag's direction. Each access
// publishes its original instruction.
// HALO2_REFERENCE_SEAT_TRANSFORM=1 restores the translated body.
// HALO2_VERIFY_SEAT_TRANSFORM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_seat_transform_systems.hpp"

namespace halo2::native_seat_transform_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seat_transform;
using Access=logic::seat_transform::Access;
using Kept=logic::seat_transform::Kept;
using Outcome=logic::seat_transform::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEAT_TRANSFORM");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEAT_TRANSFORM");return v;}
enum Branch : unsigned {no_marker, seated, unseated, backward, branch_count};
static_assert(unsigned(Outcome::unseated)==Branch::unseated);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seat transform"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0xd35c3u,0xd35c9u,0xd35dcu,0xd35e2u,0xd35e7u,0xd35eeu,0xd35f2u,0xd35f6u,0xd35fau,0xd3605u,
    0xd3616u,0xd3619u,0xd3626u,0xd362au,0xd362cu,0xd3643u,0xd3647u,0xd364du,0xd373eu,0xd366bu,0xd3678u,0xd3678u,0xd367au,0xd3681u,
    0u,0u,0u,0xd36bfu,0xd36c1u,0xd36d0u,0u,0u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::vector_write)+1u);
// The copies' instructions: the settings (seated, unseated) and the vectors (forward, up).
inline constexpr std::uint32_t settings_reads[2][5]{{0xd3696u,0xd369bu,0xd36a1u,0xd36a7u,0xd36aeu},{0xd3715u,0xd371au,0xd3720u,0xd3726u,0xd372du}},
    settings_writes[2][5]{{0xd3699u,0xd369eu,0xd36a4u,0xd36abu,0xd36b3u},{0xd3718u,0xd371du,0xd3723u,0xd372au,0xd3732u}},
    vector_reads[6]{0xd36dfu,0xd36e6u,0xd36ecu,0xd36f5u,0xd36fcu,0xd3709u},vector_writes[6]{0xd36e4u,0xd36e9u,0xd36efu,0xd36fau,0xd3706u,0xd370cu};
template<class State,class Marker,class Complete> struct Adapter : Port<State> {
    Marker &marker_child;
    Complete &complete_child;
    std::uint32_t depth{};      // bytes below the return address
    bool seated_path{};
    unsigned settings_done{},vectors_done{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Marker &m,Complete &b):Port<State>(s,c,t,{}),marker_child(m),complete_child(b){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const unsigned path=seated_path?0u:1u;
        switch(a) {
        case settings_argument:return seated_path?0xd368cu:0xd36ffu;
        case settings_read:return settings_reads[path][settings_done];
        case settings_write:return settings_writes[path][settings_done-1u];
        case vector_read:return vector_reads[vectors_done];
        case vector_write:return vector_writes[vectors_done-1u];
        default:return sites[unsigned(a)];
        }
    }
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case datums_again:case parent_tag:case parent_definition:case unit_tag:gp[edx]=v;break;
        case one:this->s.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(v)));break;                       // MOVSS XMM0,[46A0D0]
        case datums:gp[esi]=v;gp[ecx]=3u*gp[ecx];break;                                                 // LEA ECX,[ECX+ECX*2]
        case object_address:case parent_again:gp[ebx]=v;break;
        case transform_argument:gp[edi]=v;break;
        case seat:set_low_word(gp[ecx],v);break;
        case parent_object:case object_load:case marker_name:case settings_argument:case parent_of_parent:gp[eax]=v;break;
        case seats:case transform_again:gp[esi]=v;break;
        case instances:                                                                                  // MOVSX EBP,CX; IMUL EBP,B0; AND EDX,FFFF; SHL EDX,4
            gp[eax]=v;gp[ebp]=fl.product(logic::widen16(gp[ecx]),logic::seat_size);
            gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;
        case unit_instances:gp[ecx]=v;gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;       // AND EDX,FFFF; SHL EDX,4
        case unit_definition:gp[ebp]=v;break;
        case answer_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
        if(a==parent_object)(void)0;
        if(a==datums_again)gp[eax]=3u*fl.logic(gp[ebx]&0xffffu,4);                                      // MOV EAX,EBX; AND; LEA
        if(a==marker_name)gp[ebp]=fl.plus(gp[ebp],gp[esi],4);                                           // ADD EBP,ESI
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::matrix_write){gp[esi]+=step();gp[edi]+=step();--gp[ecx];}                       // each MOVSD step
    }
    void saving(unsigned n,std::uint32_t unit_handle) {
        auto &gp=this->gp;
        if(n==0u) {                                                                                      // PUSH EBX, EBP; MOV ECX,EAX; AND; PUSH ESI
            put(0xd35d1u,gp[ebx]);put(0xd35d2u,gp[ebp]);gp[ecx]=this->flags.logic(unit_handle&0xffffu,4);put(0xd35dbu,gp[esi]);
        } else put(0xd35e6u,gp[edi]);                                                                    // PUSH EDI
    }
    void parent_tested(std::uint32_t parent_value){this->compare32(parent_value,logic::none);}         // CMP [EBX+14],-1
    void seat_tested(std::uint32_t seat_value){this->flags.minus(seat_value&0xffffu,0xffffu,2);}        // CMP CX,-1
    std::uint32_t marker(std::uint32_t name,Kept &kept) {                                               // PUSH 1, EAX, EBX; XOR CL,CL; LEA EBX; CALL BAD60
        auto &gp=this->gp;auto &fl=this->flags;
        seated_path=true;
        gp[ebp]=kept.record;gp[eax]=name;
        put(0xd3653u,logic::marker_mode);put(0xd3655u,name);put(0xd3656u,kept.held);
        set_low_byte(gp[ecx],fl.logic(0u,1));
        gp[ebx]=local(logic::local::marker);
        this->call(0xd365du,0xd3662u,0xbad60u,marker_child);depth-=12u;
        refresh(kept);
        fl.logic(gp[eax]&0xffffu,2);                                                                     // TEST AX,AX
        return gp[eax];
    }
    void copying(std::uint32_t from,std::uint32_t to) {                                                // MOV ECX,D; LEA ESI,[ESP+50] (then REP MOVSD)
        auto &gp=this->gp;
        gp[ecx]=logic::matrix_words;gp[esi]=from;gp[edi]=to;
        if(step()!=4u)this->reached(Branch::backward);
    }
    void complete(std::uint32_t handle,std::uint32_t base,std::uint32_t offset,Kept &kept) {           // ADD ESI,28 (LEA ESI,[EDI+28]); CALL BBE00
        auto &gp=this->gp;
        gp[eax]=handle;
        gp[esi]=seated_path?this->flags.plus(base,offset,4):base+offset;
        this->call(seated_path?0xd3687u:0xd36d7u,seated_path?0xd368cu:0xd36dcu,0xbbe00u,complete_child);
        refresh(kept);
    }
    void vector(unsigned which,unsigned word,std::uint32_t value,const Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        ++vectors_done;
        if(which==0u) {
            if(word==0u){gp[edx]=kept.held+logic::object::forward;gp[ecx]=value;gp[eax]=kept.out+logic::transform::forward;}   // LEA EDX; LEA EAX
            else if(word==1u)gp[ecx]=value;
            else gp[edx]=value;
        } else {
            if(word==0u){gp[ebx]=fl.plus(kept.held,logic::object::up,4);gp[eax]=value;gp[edi]=fl.plus(kept.out,logic::transform::up,4);}   // ADD EBX,7C; ADD EDI,1C
            else if(word==1u)gp[ecx]=value;
            else gp[edx]=value;
        }
    }
    void settings_start(std::uint32_t record,std::uint32_t offset){this->gp[ebp]=this->flags.plus(record,offset,4);}   // ADD EBP,24 (F0)
    void settings(std::uint32_t word,std::uint32_t value) {
        auto &gp=this->gp;
        ++settings_done;
        if(word&1u)gp[edx]=value;else gp[ecx]=value;
        const std::uint32_t at=seated_path?0xd36aau:0xd3729u;
        if(word==3u)gp[edi]=take(at);                                                                   // POP EDI
        if(word==4u){gp[esi]=take(at+7u);gp[ebp]=take(at+8u);}                                           // POP ESI; POP EBP
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x78u,4);depth=0x78u;                                                   // SUB ESP,78
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::seat_transform::fill(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at;
        if(outcome==Outcome::no_marker) {
            gp[edi]=take(0xd3742u);gp[esi]=take(0xd3743u);gp[ebp]=take(0xd3744u);gp[ebx]=take(0xd3745u);at=0xd3746u;
        } else {
            const std::uint32_t end=outcome==Outcome::seated?0xd36b6u:0xd3735u;
            set_low_byte(gp[eax],answer);gp[ebx]=take(end+2u);at=end+3u;                                  // MOV AL,1; POP EBX
        }
        if(outcome==Outcome::no_marker)set_low_byte(gp[eax],answer);
        gp[esp]=fl.plus(gp[esp],0x78u,4);                                                                // ADD ESP,78
        this->ret(at+3u,8u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Marker,class Complete> bool bridge(State &s,Reference reference,Marker marker,Complete complete) {
    return run_owner(s,0xd35c0u,0xd3749u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Marker,Complete>(v,c,t,marker,complete);});
}
} // namespace halo2::native_seat_transform_detail
