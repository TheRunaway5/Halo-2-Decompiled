#pragma once
// Boundary adapter for the record placement, 1DC680
// (native_tu15_record_place_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the record, ECX the frame,
// the placement's address on the stack (RET 4); EBX, ESI and EDI saved;
// E32C0 gets EAX the object and the argument's slot pushed (RET 4); 145560
// ECX the placement, EDX the node's matrix and the placement pushed (RET 4);
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_RECORD_PLACE=1 restores the translated body.
// HALO2_VERIFY_RECORD_PLACE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_record_place_systems.hpp"

namespace halo2::native_record_place_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_record_place;
using Access=logic::record_place::Access;
using Kept=logic::record_place::Kept;
using Outcome=logic::record_place::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RECORD_PLACE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RECORD_PLACE");return v;}
enum Branch : unsigned {raised, not_biped, no_offset, unmarked, no_node, transformed, branch_count};
static_assert(unsigned(Outcome::transformed)==Branch::transformed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Record place"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1dc681u,0x1dc688u,0u,0u,0x1dc69du,0x1dc6a0u,0x1dc6b0u,0x1dc6b3u,0x1dc6dau,0x1dc6dfu,0x1dc6e7u,
    0x1dc6f0u,0x1dc6fbu,0x1dc71cu,0x1dc724u,0x1dc727u,0x1dc72du,0x1dc738u,0x1dc73cu,0x1dc73eu,0x1dc74cu,0x1dc750u,0x1dc75cu,
    0x1dc760u,0x1dc76cu,0x1dc773u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::nodes)+1u);
template<class State,class Offset,class Transform> struct Adapter : Port<State> {
    Offset &offset_child;
    Transform &transform_child;
    std::uint32_t depth{};      // bytes below the return address
    std::uint32_t copy_site{};  // this path's REP MOVSD
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Offset &o,Transform &tr):Port<State>(s,c,t,{}),offset_child(o),transform_child(tr){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    // AND reg,FFFF; SHL reg,4 of a tag handle.
    void instance_of(unsigned reg){this->gp[reg]=this->flags.shift(this->flags.logic(this->gp[reg]&0xffffu,4),4u,4);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case copy_read:case copy_write:return copy_site;
        case object_address:gp[eax]=3u*this->flags.logic(gp[eax]&0xffffu,4);break;                    // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case object_definition:case model_definition:case render_definition:instance_of(edx);break;
        case nodes:gp[eax]=logic::widen8(gp[ecx]&0xffu);break;                                        // MOVSX EAX,CL
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case placement:gp[ebx]=v;put(0x1dc685u,gp[esi]);gp[esi]=gp[ecx];break;                         // PUSH ESI; MOV ESI,ECX
        case flags:gp[ecx]=fl.right(v,logic::raised_bit,4,false);this->test8(gp[ecx]&1u);put(0x1dc691u,gp[edi]);break;   // SHR ECX,9; TEST CL,1; PUSH EDI
        case object:gp[esi]=v;break;
        case headers:gp[eax]=v;gp[ecx]=fl.logic(gp[esi]&0xffffu,4);gp[edx]=3u*gp[ecx];break;           // MOV ECX,ESI; AND ECX,FFFF; LEA EDX,[ECX+ECX*2]
        case datums:gp[ecx]=v;break;
        case type:set_low_byte(gp[ecx],v);gp[eax]=fl.shift(1u,v&0xffu,4);this->test8(gp[eax]&1u);break;   // MOV EAX,1; SHL EAX,CL; TEST AL,1
        case height:this->s.xmm[0]=lane0_only(v);break;
        case marker:set_low_byte(gp[ecx],v);break;
        case markers:gp[edi]=v;gp[edx]=logic::widen8(gp[ecx]&0xffu);break;                            // MOVSX EDX,CL
        case node:set_low_byte(gp[ecx],v);fl.minus(v&0xffu,logic::no_node,1);break;                     // CMP CL,FF
        case object_again:case object_address:case instances:gp[eax]=v;break;
        case headers_again:case datums_again:case object_tag:case object_definition:case model_tag:case model_definition:
        case render_tag:case render_definition:gp[edx]=v;break;
        case nodes:                                                                                   // LEA EAX,[EAX+EAX*2]; SHL EAX,5; LEA EDX,[ECX+EAX+28]
            gp[ecx]=v;gp[eax]=fl.shift(3u*gp[eax],5u,4);break;                                            // (EDX the matrix: transform())
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        if(a==Access::copy_write){const auto step=copy_step();gp[esi]+=step;gp[edi]+=step;--gp[ecx];}   // each MOVSD step
        if(a==Access::height_store)this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0_only(v));            // ADDSS XMM0,[ESP+10]
    }
    std::uint32_t copy_step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    // MOV ECX,D; MOV EDI,EBX (then REP MOVSD).
    void copy_started() {
        auto &gp=this->gp;
        if(!copy_site)copy_site=0x1dc69bu;
        gp[ecx]=logic::matrix_words;gp[edi]=gp[ebx];
    }
    // TEST CL,CL; CMP EDX,EDI; MOVSX CX,CL; CMP CX,-1: as far as the original goes.
    void compared(std::uint32_t marker_value) {
        auto &gp=this->gp;auto &fl=this->flags;
        this->test8(marker_value&0xffu);
        copy_site=0x1dc795u;
        if(std::int32_t(marker_value)<0)return;
        fl.minus(gp[edx],gp[edi],4);
        if(std::int32_t(gp[edx])>=std::int32_t(gp[edi]))return;
        set_low_word(gp[ecx],marker_value);fl.minus(marker_value&0xffffu,0xffffu,2);
        if((marker_value&0xffffu)!=0xffffu)copy_site=0x1dc71au;
    }
    // LEA ECX,[ESP+10]; PUSH ECX; MOV EAX,ESI; CALL E32C0; TEST AL,AL.
    std::uint32_t offset(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=argument();put(0x1dc6cau,gp[ecx]);gp[eax]=gp[esi];
        this->call(0x1dc6cdu,0x1dc6d2u,0xe32c0u,offset_child);depth-=4u;
        this->test8(gp[eax]&0xffu);kept.placement=gp[ebx];
        return gp[eax];
    }
    void restoring(){this->gp[edi]=take(0x1dc6e5u);this->gp[esi]=take(0x1dc6e6u);}
    // LEA EDX,[ECX+EAX+28] (the node's matrix); PUSH EBX; MOV ECX,EBX; CALL 145560.
    void transform(std::uint32_t,std::uint32_t matrix) {
        auto &gp=this->gp;
        gp[edx]=matrix;put(0x1dc780u,gp[ebx]);gp[ecx]=gp[ebx];
        this->call(0x1dc783u,0x1dc788u,0x145560u,transform_child);depth-=4u;
    }
    void run() {
        auto &gp=this->gp;
        put(0x1dc680u,gp[ebx]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::record_place::place(memory,*this,gp[eax],gp[ecx]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::raised){gp[ebx]=take(0x1dc6ecu);this->ret(0x1dc6edu,4u);this->publish(0x1dc6edu);return;}
        const std::uint32_t at=outcome==Outcome::transformed?0x1dc788u:0x1dc797u;
        gp[edi]=take(at);gp[esi]=take(at+1u);gp[ebx]=take(at+2u);
        this->ret(at+3u,4u);this->publish(at+3u);
    }
};
template<class State,class Reference,class Offset,class Transform> bool bridge(State &s,Reference reference,Offset offset,Transform transform) {
    return run_owner(s,0x1dc680u,0x1dc79au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Offset,Transform>(v,c,t,offset,transform);});
}
} // namespace halo2::native_record_place_detail
