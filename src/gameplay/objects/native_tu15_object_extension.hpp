#pragma once
// Boundary adapter for the object extension, BE3B0
// (native_tu15_object_extension_systems.hpp). Its callers and 191130 keep
// the original ABI, which the adapter owns: EAX the object, the field
// offset, size and alignment bits pushed (RET 0xC), the answer in AL; an
// 8-byte frame (the header's offset, the answer byte) below the saved EBP
// (and EBX, ESI and EDI past the size test); 191130 (EAX the new size, EBX
// the data pointer's address; the pool and two zeros pushed, RET 0xC)
// direct, every register it returns coming back; the grown bytes zeroed as
// REP STOSD and REP STOSB store them, in the string direction flag's
// direction. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_EXTENSION=1 restores the translated body.
// HALO2_VERIFY_OBJECT_EXTENSION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_object_extension_systems.hpp"

namespace halo2::native_object_extension_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_extension;
using Access=logic::object_extension::Access;
using Kept=logic::object_extension::Kept;
using Outcome=logic::object_extension::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_EXTENSION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_EXTENSION");return v;}
enum Branch : unsigned {empty, appended, failed, backward, branch_count};
static_assert(unsigned(Outcome::failed)==Branch::failed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object extension"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0xbe3b3u,0xbe3b9u,0xbe3c8u,0xbe3cfu,0xbe3d7u,0xbe3deu,0xbe3e1u,0xbe3eeu,0xbe3e8u,0xbe3fcu,0xbe40au,
    0xbe413u,0xbe42du,0xbe435u,0xbe43cu,0xbe440u,0xbe443u,0xbe448u,0xbe44cu,0xbe457u,0xbe45bu,0xbe460u,0xbe463u,0xbe473u,0xbe47au,0xbe488u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_load)+1u);
template<class State,class Resize> struct Adapter : Port<State> {
    Resize &resize_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Resize &r):Port<State>(s,c,t,{}),resize_child(r){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xffffffffu:1u;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:case object_address:case pool:case grown_headers:case grown_datums:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        case size_argument:set_low_word(gp[eax],v);break;
        case empty_field:case field_argument:gp[eax]=logic::widen16(v);break;                            // MOVSX EAX,word
        case alignment_argument:set_low_byte(gp[ecx],v);break;
        case header_size:gp[eax]=logic::widen16(v);break;
        case old_size:this->flags.logic(0u,4);gp[edx]=v&0xffffu;break;                                  // XOR EDX,EDX; MOV DX,[EBP+6]
        case header_again:case data_address:gp[ebp]=v;break;
        case size_again:set_low_word(gp[ecx],v);break;
        case answer_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::zero_word){gp[edi]+=4u*step();--gp[ecx];}                                        // each STOSD step
        if(a==Access::zero_byte){gp[edi]+=step();--gp[ecx];}                                            // each STOSB step
    }
    std::uint32_t indexing(std::uint32_t object_handle) {                                              // AND EAX,FFFF; LEA; SHL EAX,2
        auto &gp=this->gp;auto &fl=this->flags;
        return gp[eax]=fl.shift(3u*fl.logic(object_handle&0xffffu,4),2u,4);
    }
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){put(0xbe3c7u,gp[ebp]);gp[ebp]=gp[edx]+gp[eax];}                                        // PUSH EBP; (LEA EBP,[EDX+EAX])
        else{put(0xbe400u,gp[ebx]);put(0xbe401u,gp[esi]);put(0xbe402u,gp[edi]);}                          // PUSH EBX; PUSH ESI; PUSH EDI
    }
    void size_tested(std::uint32_t size){this->flags.logic(size&0xffffu,2);}                           // TEST AX,AX
    std::uint32_t field_found(std::uint32_t field_value,std::uint32_t object_value) {                  // ADD EAX,ECX ([ECX+EBP+8])
        return this->gp[eax]=this->flags.plus(field_value,object_value,4);
    }
    std::uint32_t masked(std::uint32_t bits) {                                                         // MOV ESI,1; SHL ESI,CL
        return this->gp[esi]=this->flags.shift(1u,bits&0xffu,4);
    }
    std::uint32_t grown(std::uint32_t size,std::uint32_t mask,std::uint32_t current) {                 // EDI=size; PUSH 0; DEC ESI; PUSH 0; ADD EDI,ESI; ADD EAX,EDI
        auto &gp=this->gp;auto &fl=this->flags;
        put(0xbe417u,0u);gp[esi]=fl.increment(mask+1u,4,true);put(0xbe41au,0u);
        gp[edi]=fl.plus(size,gp[esi],4);gp[eax]=fl.plus(current,gp[edi],4);
        return gp[edi];
    }
    std::uint32_t resize(std::uint32_t new_size,std::uint32_t pool_address,Kept &kept) {               // LEA EBX,[EBP+8]; PUSH ECX; CALL 191130
        auto &gp=this->gp;
        gp[eax]=new_size;gp[ebx]=kept.data_pointer;
        put(0xbe423u,pool_address);
        this->call(0xbe424u,0xbe429u,0x191130u,resize_child);depth-=12u;
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    std::uint32_t summed(std::uint32_t old,std::uint32_t growth){return this->gp[eax]=old+growth;}     // LEA EAX,[EDX+EDI]
    std::uint32_t placed(std::uint32_t old,std::uint32_t mask) {                                       // LEA ECX,[ESI+EDX]; NOT ESI; AND ECX,ESI
        auto &gp=this->gp;
        gp[esi]=~mask;gp[ecx]=this->flags.logic((mask+old)&gp[esi],4);
        return gp[ecx];
    }
    std::uint32_t zeroing(std::uint32_t growth,std::uint32_t old,std::uint32_t object_value) {         // MOV ECX,EDI; MOVSX EDI,DX; MOV EDX,ECX; ADD EDI,EBP; SHR ECX,2; XOR EAX,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=growth;gp[edi]=fl.plus(old,object_value,4);gp[ecx]=fl.right(growth,2u,4,false);gp[eax]=fl.logic(0u,4);
        if(step()!=1u)this->reached(Branch::backward);
        return gp[edi];
    }
    void bytes(std::uint32_t growth){this->gp[ecx]=this->flags.logic(growth&3u,4);}                   // MOV ECX,EDX; AND ECX,3
    void restoring(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){gp[edi]=take(0xbe48cu);gp[esi]=take(0xbe48du);gp[ebx]=take(0xbe48eu);gp[ebp]=take(0xbe48fu);}
        else{gp[edi]=take(0xbe47cu);gp[esi]=take(0xbe47du);gp[ebx]=take(0xbe47eu);set_low_byte(gp[eax],1u);gp[ebp]=take(0xbe481u);}
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],8u,4);depth=8u;                                                         // SUB ESP,8
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::object_extension::append(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at=0xbe482u;
        if(outcome==Outcome::empty){set_low_byte(gp[eax],answer);gp[ebp]=take(0xbe3f5u);at=0xbe3f6u;}  // MOV AL,1; POP EBP
        if(outcome==Outcome::failed)at=0xbe490u;
        gp[esp]=fl.plus(gp[esp],8u,4);                                                                   // ADD ESP,8
        this->ret(at+3u,0xcu);this->publish(at+3u);
    }
};
template<class State,class Reference,class Resize> bool bridge(State &s,Reference reference,Resize resize) {
    return run_owner(s,0xbe3b0u,0xbe493u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Resize>(v,c,t,resize);});
}
} // namespace halo2::native_object_extension_detail
