#pragma once
// Boundary adapter for the light record, C3820
// (native_tu15_light_record_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the record, EDX the light
// handle and the split argument on the stack (RET 4), the answer in AL; EBX,
// ESI and EDI saved (EBP around the session's test); C3A20 (EAX the handle,
// EDI the record) and C3AB0 (ECX, EDX and the definition pushed, RET 4) go
// direct in one unit and through the dispatch in the other, so the bridge
// takes the unit's steps; every register each returns comes back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_LIGHT_RECORD=1 restores the translated body.
// HALO2_VERIFY_LIGHT_RECORD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "pstencil/native_tu15_light_record_systems.hpp"

namespace halo2::native_light_record_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_light_record;
using Access=logic::light_record::Access;
using Kept=logic::light_record::Kept;
using Outcome=logic::light_record::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIGHT_RECORD");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIGHT_RECORD");return v;}
enum Branch : unsigned {split_off, game_off, light_off, dim, lit, cone_dim, cone_short, cone_lit, session_tested, branch_count};
static_assert(unsigned(Outcome::cone_lit)==Branch::cone_lit);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Light record"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0xc3833u,0xc3838u,0xc383bu,0xc3842u,0xc384eu,0xc3852u,0xc385au,0xc3863u,0xc3869u,0xc386fu,
    0xc3879u,0xc3884u,0xc3889u,0xc3893u,0xc38aeu,0xc38b4u,0xc38b9u,0xc38cau,0xc38cfu,0xc38d8u,0xc38ddu,0xc38f4u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::threshold)+1u);
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case lights_header:case definition_flags:case variant_index:case game_address:gp[eax]=v;break;
        case datums:gp[ecx]=v;break;
        case instances:gp[eax]=v;gp[esi]=fl.plus(gp[esi],gp[ecx],4);break;                             // ADD ESI,ECX
        case light_tag:gp[ecx]=fl.shift(fl.logic(v&0xffffu,4),4u,4);break;                              // AND ECX,FFFF; SHL ECX,4
        case definition_address:gp[ebx]=v;break;
        case split_argument:set_low_byte(gp[eax],v);this->test8(v&0xffu);break;                        // MOV AL; TEST AL,AL
        case session_address:gp[ebp]=v;break;
        case variant_entry:fl.minus(v,0u,4);break;                                                     // CMP [56DFA0+EAX*4],0
        case game_enabled:fl.minus(v&0xffu,0u,1);break;                                                // CMP byte [EAX+8],0
        case light_flags:fl.logic(v&logic::light::lit,1);break;                                        // TEST byte [ESI+2],2
        case kind:fl.minus(v&0xffffu,0u,2);break;                                                      // CMP word [EDI],0
        case intensity:case cone_intensity:case cone_y:x[0]=lane0(v);break;
        case epsilon:case cone_epsilon:fl.scalar_compare(x[0],lane0(v),false);break;                   // COMISS XMM0,[46A0EC]
        case cone_x:x[1]=lane0(v);break;
        case threshold:fl.scalar_compare(x[2],lane0(v),false);break;                                   // COMISS XMM2,[46A610]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void admitted(std::uint32_t flags){set_low_byte(this->gp[ecx],1u);this->flags.logic(1u&(flags>>8),1);}   // MOV CL,1; TEST CL,AH
    void saving(){put(0xc3862u,this->gp[ebp]);this->reached(Branch::session_tested);}
    void session_flags(std::uint32_t bits,bool allowed) {                                              // MOV ECX,[EBP]; SHR ECX,1; NOT CL; AND CL,1
        auto &gp=this->gp;
        gp[ecx]=this->flags.right(bits,1u,4,false);set_low_byte(gp[ecx],this->flags.logic(allowed?1u:0u,1));
    }
    void restoring(){this->gp[ebp]=take(0xc3883u);}
    void allowed_test(bool allowed){this->test8(allowed?1u:0u);}                                       // TEST CL,CL
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[edi],this->gp[ebx]};}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    void shape(std::uint32_t handle,Kept &kept){this->gp[eax]=handle;step(0xc389bu,0xc3a20u);refresh(kept);}   // MOV EAX,EDX; CALL C3A20
    void complete(std::uint32_t source,Kept &kept) {                                                   // PUSH EBX; LEA ECX,[ESI+84]; MOV EDX,EDI; CALL C3AB0
        auto &gp=this->gp;
        put(0xc38a0u,kept.definition);gp[ecx]=source;gp[edx]=kept.record;step(0xc38a9u,0xc3ab0u);depth-=4u;refresh(kept);
    }
    void measured(std::uint32_t y_squared,std::uint32_t length){auto &x=this->s.xmm;x[2]=lane0(length);x[1]=lane0(y_squared);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        put(0xc3820u,gp[ebx]);put(0xc3821u,gp[esi]);
        gp[esi]=fl.product(fl.logic(gp[edx]&0xffffu,4),logic::light_size);                            // MOV ESI,EDX; AND ESI,FFFF; IMUL ESI,110
        put(0xc3830u,gp[edi]);gp[edi]=gp[eax];                                                         // PUSH EDI; MOV EDI,EAX
        Memory<Adapter> memory{*this};
        const auto outcome=logic::light_record::prepare(memory,*this,gp[eax],gp[edx]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::lit||outcome==Outcome::cone_lit) {
            const std::uint32_t at=outcome==Outcome::lit?0xc38c2u:0xc38fdu;
            gp[edi]=take(at);gp[esi]=take(at+1u);set_low_byte(gp[eax],1u);gp[ebx]=take(at+4u);
            this->ret(at+5u,4u);this->publish(at+5u);return;
        }
        gp[edi]=take(0xc3905u);gp[esi]=take(0xc3906u);set_low_byte(gp[eax],fl.logic(0u,1));gp[ebx]=take(0xc3909u);
        this->ret(0xc390au,4u);this->publish(0xc390au);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0xc3820u,0xc390au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_light_record_detail
