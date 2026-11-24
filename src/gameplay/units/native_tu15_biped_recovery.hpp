#pragma once
// Boundary adapter for the biped recovery, E52D0
// (native_tu15_biped_recovery_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the biped on the stack (RET 4),
// the answer in AL; EBX, EBP, ESI and EDI saved; E7E80 (EAX the biped),
// 111B10 (EAX 2, seven arguments pushed, RET 1C) and C1A70 (EAX the biped, 0
// pushed, RET 4) direct, 2031F0 (EAX the block) through the dispatch, every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_BIPED_RECOVERY=1 restores the translated body.
// HALO2_VERIFY_BIPED_RECOVERY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_biped_recovery_systems.hpp"

namespace halo2::native_biped_recovery_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_biped_recovery;
using Access=logic::biped_recovery::Access;
using Kept=logic::biped_recovery::Kept;
using Outcome=logic::biped_recovery::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BIPED_RECOVERY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BIPED_RECOVERY");return v;}
enum Branch : unsigned {idle, counting, got_up, left_seat, cleared_seat, landed, branch_count};
static_assert(unsigned(Outcome::counting)==Branch::counting);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Biped recovery"};return v;}
inline constexpr std::uint32_t sites[]{0xe52d2u,0xe52e1u,0xe52e6u,0xe52edu,0xe52f1u,0xe5301u,0xe530eu,0xe5310u,0xe531fu,0xe5323u,
    0xe5335u,0xe5376u,0xe537bu,0xe537eu,0xe5382u,0xe538cu,0xe53a0u,0xe53a0u,0xe53a7u,0xe53bbu,0xe53c7u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::countdown_store)+1u);
template<class State,class GettingUp,class Request,class LeaveSeat,class Dispatch> struct Adapter : Port<State> {
    GettingUp &getting_up_child;
    Request &request_child;
    LeaveSeat &leave_seat_child;
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,GettingUp &g,Request &r,LeaveSeat &l,Dispatch &d)
        :Port<State>(s,c,t,{}),getting_up_child(g),request_child(r),leave_seat_child(l),dispatch(d){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx]&0xffu,this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case handle_argument:gp[ebp]=v;break;
        case headers:case headers_again:gp[eax]=v;break;
        case datums:case datums_again:gp[ecx]=v;break;
        case object_address:gp[edi]=v;break;
        case countdown:case countdown_again:set_low_byte(gp[eax],v);break;
        case physics:fl.minus(v&0xffu,logic::recovering_mode,1);break;                                // CMP byte [EDI+3DC],3
        case tag:gp[edx]=v;break;
        case instances:gp[ecx]=v;gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;           // AND EDX,FFFF; SHL EDX,4
        case definition:gp[edx]=v;break;
        case definition_flags:gp[ecx]=v;break;
        case flags_byte:set_low_byte(gp[edx],v);break;
        case object_again:gp[esi]=v;break;
        case seat:fl.minus(v&0xffffu,0xffffu,2);break;                                                 // CMP word [ESI+112],-1
        case seated:gp[edx]=v;break;
        case physics_again:fl.minus(v&0xffu,logic::landing_mode,1);break;                              // CMP byte [EDI+3DC],2
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::flags_store)this->reached(Branch::cleared_seat);}
    void indexing(const Kept &kept) {                                                                  // MOV EAX,EBP; AND EAX,FFFF; PUSH ESI; LEA ESI,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(kept.handle&0xffffu,4);put(0xe52ddu,gp[esi]);gp[esi]=3u*gp[eax];
    }
    void offset(const Kept &kept) {                                                                    // SHL ESI,2; PUSH EDI
        auto &gp=this->gp;
        this->flags.shift(gp[esi],2u,4);gp[esi]=kept.held;put(0xe52ecu,gp[edi]);
    }
    void counted(std::uint32_t count,const Kept &kept){set_low_byte(this->gp[ebx],this->flags.logic(kept.answer,1));this->test8(count&0xffu);}
    void count_tested(std::uint32_t count){this->test8(count&0xffu);}
    void definition_tested(std::uint32_t flags) {                                                      // SHR ECX,A; TEST CL,1
        auto &fl=this->flags;
        this->gp[ecx]=fl.right(flags,10u,4,false);fl.logic(this->gp[ecx]&1u,1);
    }
    void byte_tested(std::uint32_t byte) {                                                             // SHR DL,2; TEST DL,1
        auto &fl=this->flags;
        set_low_byte(this->gp[edx],fl.right(byte&0xffu,2u,1,false));fl.logic(this->gp[edx]&1u,1);
    }
    void last_tested(std::uint32_t count){this->flags.minus(count&0xffu,1u,1);}                        // CMP AL,1
    void getting_up(Kept &kept) {                                                                      // MOV EAX,EBP; CALL E7E80
        this->gp[eax]=kept.handle;
        this->call(0xe5349u,0xe534eu,0xe7e80u,getting_up_child);
        refresh(kept);
        this->reached(Branch::got_up);
    }
    void request(Kept &kept) {                                                                         // PUSH 0; PUSH 0; PUSH names; PUSH EBP; MOV EAX,2; CALL 111B10
        auto &gp=this->gp;
        put(0xe534eu,0u);put(0xe5350u,0u);
        put(0xe5352u,logic::recovery_names[3]);put(0xe5357u,logic::recovery_names[2]);
        put(0xe535cu,logic::recovery_names[1]);put(0xe5361u,logic::recovery_names[0]);
        put(0xe5366u,kept.handle);gp[eax]=logic::request_mode;
        this->call(0xe536cu,0xe5371u,0x111b10u,request_child);depth-=0x1cu;
        refresh(kept);
    }
    void handle_tested(std::uint32_t handle){this->compare32(handle,logic::none);}                     // CMP EBP,-1
    void seated_tested(std::uint32_t flags) {                                                          // SHR EDX,1D; TEST DL,1
        auto &fl=this->flags;
        this->gp[edx]=fl.right(flags,29u,4,false);fl.logic(this->gp[edx]&1u,1);
    }
    void leave_seat(Kept &kept) {                                                                      // PUSH 0; MOV EAX,EBP; CALL C1A70
        put(0xe5397u,0u);this->gp[eax]=kept.handle;
        this->call(0xe539bu,0xe53a0u,0xc1a70u,leave_seat_child);depth-=4u;
        refresh(kept);
        this->reached(Branch::left_seat);
    }
    void cleared(std::uint32_t value){this->flags.logic(value,4);}                                     // AND dword [ESI+4],DFFFFFFF
    void land(std::uint32_t block,Kept &kept) {                                                        // LEA EAX,[EDI+3EC]; CALL 2031F0
        this->gp[eax]=block;
        auto child=[&](State &v){dispatch(v,0x2031f0u,0xe53b6u);};
        this->call(0xe53b6u,0xe53bbu,0x2031f0u,child);
        refresh(kept);
        this->reached(Branch::landed);
    }
    void answered(const Kept &kept){set_low_byte(this->gp[eax],kept.answer);}                         // MOV AL,BL
    void counted_down(std::uint32_t value){set_low_byte(this->gp[eax],this->flags.increment((value+1u)&0xffu,1,true));}   // DEC AL
    void run() {
        auto &gp=this->gp;
        put(0xe52d0u,gp[ebx]);put(0xe52d1u,gp[ebp]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::biped_recovery::update(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::counting) {
            gp[edi]=take(0xe53cdu);gp[esi]=take(0xe53ceu);gp[ebp]=take(0xe53cfu);set_low_byte(gp[eax],1u);gp[ebx]=take(0xe53d2u);
            this->ret(0xe53d3u,4u);this->publish(0xe53d3u);return;
        }
        gp[edi]=take(0xe53d6u);gp[esi]=take(0xe53d7u);gp[ebp]=take(0xe53d8u);gp[ebx]=take(0xe53dbu);
        this->ret(0xe53dcu,4u);this->publish(0xe53dcu);
    }
};
template<class State,class Reference,class GettingUp,class Request,class LeaveSeat,class Dispatch>
bool bridge(State &s,Reference reference,GettingUp getting_up,Request request,LeaveSeat leave_seat,Dispatch dispatch) {
    return run_owner(s,0xe52d0u,0xe53dcu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,GettingUp,Request,LeaveSeat,Dispatch>(v,c,t,getting_up,request,leave_seat,dispatch);});
}
} // namespace halo2::native_biped_recovery_detail
