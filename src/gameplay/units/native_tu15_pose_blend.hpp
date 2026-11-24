#pragma once
// Boundary adapter for the pose blend, 1D5970
// (native_tu15_pose_blend_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: EAX the object, EBX the references,
// ESI the vector, the value pointer pushed (RET 4); a 0x20-byte frame (the
// two poses blended) below the saved EBP (and EDI past the frozen test), the
// factor in the argument's slot; 1CEFB0 (ECX a reference, EDI a vector, a
// value pointer pushed, RET 4) and 1D61B0 (EAX the references, the factor in
// XMM0) direct, every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_POSE_BLEND=1 restores the translated body.
// HALO2_VERIFY_POSE_BLEND=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_pose_blend_systems.hpp"
#include <utility>

namespace halo2::native_pose_blend_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_pose_blend;
using Access=logic::pose_blend::Access;
using Kept=logic::pose_blend::Kept;
using Outcome=logic::pose_blend::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_POSE_BLEND");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_POSE_BLEND");return v;}
enum Branch : unsigned {frozen, third, second, blended, first, zero, branch_count};
static_assert(unsigned(Outcome::zero)==Branch::zero);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Pose blend"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1d5979u,0x1d597fu,0x1d5985u,0x1d5990u,0x1d5994u,0x1d59a2u,0x1d59acu,0x1d59b6u,0x1d59ccu,0x1d59d9u,
    0x1d59deu,0x1d59efu,0x1d59f5u,0x1d5a02u,0x1d5a0au,0x1d5a14u,0x1d5a1cu,0x1d5a29u,0x1d5a4au,0u,0x1d5a6bu,0u,0u,0u,0x1d5ae6u,0x1d5aeau,
    0u,0u,0u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::zero_write)+1u);
inline constexpr std::uint32_t first_reads[4]{0x1d5a65u,0x1d5a83u,0x1d5a9fu,0x1d5abcu},second_reads[4]{0x1d5a71u,0x1d5a8du,0x1d5aaau,0x1d5ac7u},
    vector_stores[3]{0x1d5a89u,0x1d5aa5u,0x1d5ac2u};
// The zero copy's instructions: after the first test (the object frozen), then at the end.
inline constexpr std::uint32_t zero_pointers[2]{0x1d5b28u,0x1d5b02u},zero_reads[2][3]{{0x1d5b2eu,0x1d5b35u,0x1d5b3bu},{0x1d5b08u,0x1d5b0fu,0x1d5b15u}},
    zero_writes[2][3]{{0x1d5b33u,0x1d5b38u,0x1d5b3eu},{0x1d5b0du,0x1d5b12u,0x1d5b18u}},value_stores[3]{0x1d5b41u,0x1d5b1cu,0x1d5adau};
template<class State,class Pose,class Factor> struct Adapter : Port<State> {
    Pose &pose_child;
    Factor &factor_child;
    std::uint32_t depth{};      // bytes below the return address
    bool frozen_path{},blend_path{},at_first{};
    unsigned first_count{},second_count{},stores{},zero_count{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Pose &p,Factor &f):Port<State>(s,c,t,{}),pose_child(p),factor_child(f){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi]};}
    unsigned zero_path()const{return frozen_path?0u:1u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case first_read:return first_reads[first_count];
        case second_read:return second_reads[second_count];
        case vector_store:return vector_stores[stores];
        case value_store:return value_stores[blend_path?2u:zero_path()];
        case zero_pointer:return zero_pointers[zero_path()];
        case zero_read:return zero_reads[zero_path()][zero_count];
        case zero_write:return zero_writes[zero_path()][zero_count-1u];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t address,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case value_pointer:gp[ebp]=v;break;
        case headers:case datums:gp[ecx]=v;break;
        case object_address:gp[eax]=v;break;
        case frozen_byte:set_low_byte(gp[ecx],v);break;
        case overrides:case blending:set_low_byte(gp[eax],v);break;
        case third_handle:case second_handle:gp[eax]=v;gp[ecx]=address;break;                          // MOV EAX,[EBX+n]; LEA ECX,[EBX+n]
        case full:x[0]=lane0(v);break;
        case second_weight:case least:case high_weight:x[1]=lane0(v);break;
        case first_read:x[1]=lane0(v);++first_count;break;
        case factor_load:x[0]=lane0(v);break;
        case second_read:x[2]=lane0(v);++second_count;break;
        case zero_pointer:gp[frozen_path?edx:ecx]=v;break;
        case first_handle:at_first=true;break;                                                           // CMP [EBX],EDX
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::vector_store)++stores;
    }
    void none_tested(std::uint32_t object_handle) {                                                    // OR EDX,-1; CMP EAX,EDX
        auto &gp=this->gp;
        gp[edx]=this->flags.logic(logic::none,4);this->compare32(object_handle,gp[edx]);
    }
    void saving(unsigned n){if(n==0u)put(0x1d5978u,this->gp[ebp]);else put(0x1d59a9u,this->gp[edi]);}   // PUSH EBP (EDI)
    void indexing(std::uint32_t object_handle){this->gp[eax]=3u*this->flags.logic(object_handle&0xffffu,4);}   // AND EAX,FFFF; LEA
    void frozen_tested(std::uint32_t byte){this->test8(byte);if(byte&0xffu)frozen_path=true;}         // TEST CL,CL
    void flag_tested(std::uint32_t flag_byte){this->test8(flag_byte);}                                 // TEST AL,AL
    void handle_tested(std::uint32_t handle){this->compare32(handle,logic::none);}                    // CMP EAX(,[ECX]),EDX
    void high_tested(std::uint32_t high){this->flags.minus(high&0xffffu,0xffffu,2);}                  // CMP word [ECX+6],DX
    void weight_compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS
    // PUSH EBP; MOV EDI,ESI; (MOV ECX,EBX); CALL 1CEFB0: the third and second references' call (1D59BF), or the first's
    // (1D5AF5) once its test is reached.
    void pose(std::uint32_t reference,Kept &kept) {
        auto &gp=this->gp;
        put(at_first?0x1d5af0u:0x1d59bcu,kept.value);gp[edi]=kept.vector;gp[ecx]=reference;
        const std::uint32_t site=at_first?0x1d5af5u:0x1d59bfu;
        this->call(site,site+5u,0x1cefb0u,pose_child);depth-=4u;
        refresh(kept);
    }
    std::pair<std::uint32_t,std::uint32_t> factor(Kept &kept) {                                       // MOV EAX,EBX; CALL 1D61B0
        auto &gp=this->gp;
        blend_path=true;
        gp[eax]=kept.references;
        this->call(0x1d5a3cu,0x1d5a41u,0x1d61b0u,factor_child);
        refresh(kept);
        return {std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0]))),gp[ecx]};
    }
    void pushing(unsigned n,std::int32_t value_slot,std::int32_t vector_slot) {                       // LEA EDX (EAX); PUSH; LEA EDI
        auto &gp=this->gp;
        const auto value_address=local(value_slot);
        gp[n?eax:edx]=value_address;put(n?0x1d5a59u:0x1d5a45u,value_address);gp[edi]=local(vector_slot);
    }
    void blend_pose(unsigned n,std::uint32_t reference,Kept &kept) {                                   // (MOV ECX,EBX); CALL 1CEFB0
        auto &gp=this->gp;
        gp[ecx]=reference;
        this->call(n?0x1d5a60u:0x1d5a50u,n?0x1d5a65u:0x1d5a55u,0x1cefb0u,pose_child);depth-=4u;
        refresh(kept);
    }
    void lerped(unsigned,std::uint32_t result){this->s.xmm[2]=lane0(result);}
    void zero_copied(unsigned word,std::uint32_t value) {
        auto &gp=this->gp;
        ++zero_count;
        static constexpr unsigned frozen_registers[3]{eax,ecx,edx},end_registers[3]{edx,eax,ecx};
        gp[frozen_path?frozen_registers[word]:end_registers[word]]=value;
        if(word==0u)this->s.xmm[0]=_mm_setzero_ps();                                                    // XORPS XMM0,XMM0
    }
    void clearing(bool frozen){if(!frozen)this->gp[edi]=take(blend_path?0x1d5ad9u:0x1d5b1bu);}       // POP EDI
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x20u,4);depth=0x20u;                                                   // SUB ESP,20
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        logic::pose_blend::evaluate(memory,*this,gp[eax],Kept{gp[ebx],gp[ebp],gp[esi]},outcome);
        this->reached(unsigned(outcome));
        std::uint32_t at;
        switch(outcome) {
        case Outcome::third:case Outcome::second:gp[edi]=take(0x1d59c4u);at=0x1d59c5u;break;
        case Outcome::first:gp[edi]=take(0x1d5afau);at=0x1d5afbu;break;
        case Outcome::blended:at=0x1d5adfu;break;
        case Outcome::zero:at=0x1d5b21u;break;
        default:at=0x1d5b46u;break;
        }
        gp[ebp]=take(at);
        gp[esp]=fl.plus(gp[esp],0x20u,4);                                                                // ADD ESP,20
        this->ret(at+4u,4u);this->publish(at+4u);
    }
};
template<class State,class Reference,class Pose,class Factor> bool bridge(State &s,Reference reference,Pose pose,Factor factor) {
    return run_owner(s,0x1d5970u,0x1d5b4au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Pose,Factor>(v,c,t,pose,factor);});
}
} // namespace halo2::native_pose_blend_detail
