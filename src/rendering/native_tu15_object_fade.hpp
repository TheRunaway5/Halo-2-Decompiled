#pragma once
// Boundary adapter for the object fade system
// (native_tu15_object_fade_systems.hpp): 4BAE0, called by 1362A0 with the
// object handle in EBX, the camera distance in XMM1 and two output pointers
// on the stack (RET 8). Its caller and its children (BCFB0, the root object,
// and the CRT's _ftol2, 32CC0C) are separate owners, so this adapter owns
// that ABI and nothing else:
//   - the 0xC-byte frame, the saved registers (EBP; EDI and ESI once the
//     model is looked up), the argument slots and every push, pop and return
//     address, with each exit's own pops;
//   - every register as the original leaves it at each access, operation,
//     child call and exit: the general registers and flags (the tag lookups'
//     SHL EAX/ECX,4 run on the host with the source flags seeded, because
//     SHL's count-dependent OF/AF reach the next access), and the XMM
//     registers (each SSE operation runs as the original instruction on the
//     original register, with the logic's operands placed in its low lane;
//     MOVSS loads clear the upper lanes, MOVAPS copies all four);
//   - the x87 stack (State's logical stack): FLD m32 and FMUL m32 run as host
//     x87 instructions under the source control word, with their actual C1;
//   - the values the logic keeps across a call (Kept), loaded into their
//     callee-saved registers before it and read back after it;
//   - the diagnostic cursor and the store stream.
//
// The children are entered through the unit's own route (the Call supplied
// by the generated owner): BCFB0 and 32CC0C have bodies in the Shadow unit.
//
// Admission: TU1.5 image on x86-64 (the logic carries floats in SSE
// registers, so signalling NaNs pass through unchanged), no MMX alias, a
// free x87 slot, the exact live instruction-cursor contract, and a certified
// required owner. HALO2_REFERENCE_OBJECT_FADE=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (the closure 4BAE0..4BCA8 holds no service PC).
#include "rendering/native_tu15_object_fade_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <xmmintrin.h>
#endif

namespace halo2::native_object_fade_detail {
namespace logic=halo2::native_object_fade;
using logic::Access;
using logic::Op;
using logic::Kept;
using logic::Exit;
using logic::Ordering;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_FADE");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>7)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x4bae0u,0x4bca8u)){
    }else if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (s.observe&&!c->observation_only_stores)||(c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return true;
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){return !reference()&&policy_admitted(s);}

inline constexpr std::uint32_t root_entry=0xbcfb0u,to_integer_entry=0x32cc0cu;

#if defined(__x86_64__) && !defined(_MSC_VER)
struct Hardware {std::uint32_t value,flags;};
// SHL reg,4 with the source flags seeded (SAHF; OF does not reach SHL by 4).
inline Hardware shift4(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
// AND reg,FFFF; SHL reg,4.
inline Hardware tag_row(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
// The SSE scalar instructions on the host (MXCSR is the source's).
inline __m128 lane(__m128 reg,float value){return _mm_move_ss(reg,_mm_set_ss(value));}
inline float low(__m128 reg){return _mm_cvtss_f32(reg);}
inline __m128 addss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 subss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 mulss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 divss(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline std::uint32_t comiss(__m128 a,__m128 b){
    unsigned char below,equal,unordered;
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u);
}
// FLD m32 and FMUL m32 under the source control word; FMUL reports its C1.
inline long double x87_load(std::uint32_t bits){
    long double out;asm volatile("flds %1;fstpt %0":"=m"(out):"m"(bits):"memory","st");return out;
}
inline long double x87_multiply(long double value,std::uint32_t bits,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fmuls %3;fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(value),"m"(bits):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    __m128&x(unsigned n){return s_.xmm[n];}
    void byte(unsigned n,std::uint32_t value){s_.set(n,value,1,0);}
    static __m128 loaded_float(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width){at(pc);return word(address,width);}
    void call(std::uint32_t site,std::uint32_t target){push(site,site+5u);call_(s_,target,site,before_);}
    void tag_offset(unsigned n){const auto row=tag_row(r(n),s_.flags.bits());r(n)=row.value;s_.flags.load(row.flags);}

    // The original instruction of each role, with the register work between
    // the previous access and it.
    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case object_headers:r(esp)=f.minus(r(esp),0xcu,4);return 0x4bae3u;            // SUB ESP,0C
        case object_rows:return 0x4bae9u;
        case fades_disabled:return 0x4baecu;
        // MOV EAX,EBX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH EBP
        case object_data:r(eax)=f.logic(r(ebx)&0xffffu,4)*3u;push(0x4bafcu,r(ebp));return 0x4bafdu;
        case object_flags:return 0x4bb01u;
        case object_fade_out:return 0x4bb0bu;
        case no_fade_store:return 0x4bb0fu;
        case shadow_fade_out:return 0x4bb13u;
        case clear_object_fade:return 0x4bb17u;
        case clear_shadow_fade:return 0x4bb1au;
        case full_object_fade:return 0x4bb1fu;
        case full_shadow_fade:return 0x4bb22u;
        case object_definition_tag:return 0x4bb2cu;
        case tag_instances:push(0x4bb35u,r(edi));return 0x4bb36u;
        case object_definition:{const auto row=shift4(r(ecx),f.bits());r(ecx)=row.value;f.load(row.flags);return 0x4bb3fu;}
        case model_tag:return 0x4bb43u;
        // AND EAX,FFFF; PUSH ESI; SHL EAX,4
        case model_definition:{
            r(eax)=f.logic(r(eax)&0xffffu,4);push(0x4bb54u,r(esi));
            const auto row=shift4(r(eax),f.bits());r(eax)=row.value;f.load(row.flags);return 0x4bb58u;
        }
        // AND EAX,FFFF; LEA EDX,[EAX+EAX*2]
        case root_headers:r(eax)=f.logic(r(eax)&0xffffu,4);r(edx)=r(eax)*3u;return 0x4bb72u;
        case root_rows:return 0x4bb77u;
        case root_data:return 0x4bb7au;
        case root_definition_tag:return 0x4bb7eu;
        case root_definition:tag_offset(eax);return 0x4bb88u;
        case root_model_tag:return 0x4bb8cu;
        case root_model_definition:tag_offset(eax);return 0x4bb9cu;
        case shadow_fade_level:return 0x4bba3u;
        case one_constant:return 0x4bba7u;
        case bounding_radius:return 0x4bbafu;
        case shadow_band_scale:return 0x4bbb4u;
        // MOV EAX,4; SUB EAX,ECX; TEST EAX,EAX
        case shadow_fade_store:r(eax)=f.minus(4u,r(ecx),4);f.logic(r(eax),4);return 0x4bbc5u;
        case object_fade_store:return 0x4bbcbu;
        // CMP EAX,4; JLE; MOV EAX,4
        case shadow_lod:f.minus(r(eax),4u,4);if(std::int32_t(r(eax))>4)r(eax)=4u;return 0x4bbddu;
        case no_shadow_lod:return 0x4bbe5u;
        case faded_shadow_store:return 0x4bc16u;
        case zero_shadow_store:return 0x4bc38u;
        case full_shadow_store:return 0x4bc23u;
        case shadow_cutoff:return 0x4bc2bu;
        case disappear_distance:return 0x4bc3eu;
        case zero_object_store:return 0x4bc4cu;
        case begin_fade_compare:return 0x4bc54u;
        case disappear_again:return 0x4bc5au;
        case begin_fade_excess:return 0x4bc5eu;
        case begin_fade_span:return 0x4bc63u;
        case faded_object_store:return 0x4bc70u;
        case object_fade_load:return 0x4bc76u;
        case object_fade_scale:return 0x4bc7au;
        case shadow_fade_load:return 0x4bc85u;
        case object_fade_out_again:return 0x4bc89u;
        case shadow_fade_scale:return 0x4bc8du;
        case object_byte_store:return 0x4bc93u;
        case shadow_fade_out_again:return 0x4bc9au;
        case shadow_byte_store:return 0x4bc9eu;
        }
        return 0;
    }
    // The access's own effect (the destination register, a compare's flags).
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case object_headers:r(ecx)=v;break;
        case object_rows:r(edx)=v;break;
        case fades_disabled:byte(ecx,v);break;
        case object_data:r(ebp)=v;break;
        // SHR EAX,13; AND AL,1; OR AL,CL
        case object_flags:r(eax)=f.right(v,0x13u,4,false);byte(eax,f.logic(r(eax)&1u,1));byte(eax,f.logic((r(eax)|r(ecx))&0xffu,1));break;
        case object_fade_out:r(ecx)=v;break;
        case shadow_fade_out:r(eax)=v;break;
        case object_definition_tag:r(ecx)=f.logic(v&0xffffu,4);break;              // AND ECX,FFFF
        case tag_instances:r(edi)=v;break;
        case object_definition:case root_definition_tag:case root_definition:case root_headers:r(eax)=v;break;
        case model_tag:case root_model_tag:r(eax)=v;f.minus(v,0xffffffffu,4);break;
        // MOV ECX,EBX; ADD ESI,28
        case model_definition:r(ecx)=r(ebx);r(esi)=f.plus(v,0x28u,4);break;
        case root_rows:r(ecx)=v;break;
        case root_data:r(edx)=v;break;
        case root_model_definition:r(esi)=f.plus(v,0x28u,4);break;
        case shadow_fade_level:r(ecx)=v;break;
        case one_constant:x(3)=loaded_float(v);break;
        case bounding_radius:x(2)=loaded_float(v);break;
        case shadow_lod:case no_shadow_lod:case disappear_distance:case disappear_again:x(0)=loaded_float(v);break;
        case shadow_cutoff:x(2)=loaded_float(v);break;
        case object_fade_out_again:r(edx)=v;break;
        case shadow_fade_out_again:r(ecx)=v;break;
        default:break;
        }
    }
    struct Slot {std::uint32_t offset;unsigned width;};
    static Slot slot(Access a){
        using enum Access;
        switch(a){
        case object_fade_out:return{0x14u,4};
        case shadow_fade_out:return{0x18u,4};
        case no_fade_store:return{7u,1};
        case shadow_fade_store:case faded_shadow_store:case zero_shadow_store:case full_shadow_store:case shadow_fade_load:return{0x10u,4};
        case object_fade_store:case zero_object_store:case faded_object_store:case object_fade_load:return{0x14u,4};
        case object_fade_out_again:return{0x1cu,4};
        case shadow_fade_out_again:return{0x20u,4};
        default:return{0u,0};
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);const auto v=read(pc,address,width);loaded(a,v);return v;
    }
    Ordering ordering(std::uint32_t flags){s_.flags.load(flags);return{(flags&1u)!=0,(flags&64u)!=0,(flags&4u)!=0};}
    // The original register pair of each operation: the destination (after
    // its MOVAPS copy, if any) and the source register.
    struct Operation {unsigned destination,source;int copy_from;};
    static Operation operation(Op op){
        switch(op){
        case Op::shadow_band:return{2,8,-1};             // MULSS XMM2,[46A2B8]
        case Op::shadow_end:return{4,2,0};               // MOVAPS XMM4,XMM0; ADDSS XMM4,XMM2
        case Op::shadow_excess:return{5,0,1};            // MOVAPS XMM5,XMM1; SUBSS XMM5,XMM0
        case Op::shadow_ratio:return{5,2,-1};            // DIVSS XMM5,XMM2
        case Op::shadow_fade:return{0,5,3};              // MOVAPS XMM0,XMM3; SUBSS XMM0,XMM5
        case Op::object_excess:return{1,8,-1};           // SUBSS XMM1,[ESI+4]
        case Op::object_span:return{0,8,-1};             // SUBSS XMM0,[ESI+4]
        case Op::object_ratio:return{1,0,-1};            // DIVSS XMM1,XMM0
        case Op::object_fade:return{3,1,-1};             // SUBSS XMM3,XMM1
        case Op::past_shadow_end:return{1,4,-1};         // COMISS XMM1,XMM4
        case Op::past_shadow_lod:return{1,0,-1};         // COMISS XMM1,XMM0
        case Op::shadow_negative:return{4,0,-1};         // COMISS XMM4,XMM0
        case Op::shadow_above_one:return{0,3,-1};        // COMISS XMM0,XMM3
        case Op::shadow_cut:return{2,0,-1};              // COMISS XMM2,XMM0
        case Op::has_disappear:return{0,4,-1};           // COMISS XMM0,XMM4
        case Op::disappeared:return{1,0,-1};             // COMISS XMM1,XMM0
        case Op::past_begin_fade:return{1,8,-1};         // COMISS XMM1,[ESI+4]
        }
        return{0,0,-1};
    }
    // Places the logic's operands in the original registers (8: a memory
    // operand) and returns the destination and source values.
    std::pair<__m128,__m128> operands(Op op,float a,float b){
        const auto o=operation(op);
        if(o.copy_from>=0){x(unsigned(o.copy_from))=lane(x(unsigned(o.copy_from)),a);x(o.destination)=x(unsigned(o.copy_from));}
        else x(o.destination)=lane(x(o.destination),a);
        if(o.source<8)x(o.source)=lane(x(o.source),b);
        return{x(o.destination),o.source<8?x(o.source):_mm_set_ss(b)};
    }
    template<class Instruction>float arithmetic(Op op,float a,float b,Instruction instruction){
        const auto [destination,source]=operands(op,a,b);const auto o=operation(op);
        x(o.destination)=instruction(destination,source);return low(x(o.destination));
    }
public:
    Adapter(State&s,Call&call,Before&before):Base{s,before},call_(call){}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}
    std::uint32_t load8(Access a,std::uint32_t address){return load(a,address,1);}
    float load_float(Access a,std::uint32_t address){return std::bit_cast<float>(load(a,address,4));}
    void store8(Access a,std::uint32_t address,std::uint32_t value){
        const auto pc=before(a);write(pc,address,value&0xffu,1);
    }

    // ---- Arithmetic
    float multiply(Op op,float a,float b){return arithmetic(op,a,b,mulss);}
    float add(Op op,float a,float b){return arithmetic(op,a,b,addss);}
    float subtract(Op op,float a,float b){return arithmetic(op,a,b,subss);}
    float divide(Op op,float a,float b){return arithmetic(op,a,b,divss);}
    Ordering compare(Op op,float a,float b){
        const auto [left,right]=operands(op,a,b);const auto result=ordering(comiss(left,right));
        if(op==Op::past_shadow_end)x(4)=_mm_setzero_ps();                         // XORPS XMM4,XMM4
        return result;
    }
    void scale(Access a,std::uint32_t address){
        const auto bits=load(a,address,4);s_.fp.st(0)=x87_multiply(s_.fp.st(0),bits,s_.fp.status);
    }

    // ---- Frame
    std::uint32_t object_handle(){return r(ebx);}
    float distance(){return low(x(1));}
    std::uint32_t argument(Access a){return load(a,r(esp)+slot(a).offset,4);}
    void set_local(Access a,float value){
        const auto pc=before(a);write(pc,r(esp)+slot(a).offset,std::bit_cast<std::uint32_t>(value),4);
    }
    void store_local_byte(Access a,std::uint32_t value){const auto pc=before(a);write(pc,r(esp)+slot(a).offset,value&0xffu,1);}
    // FLD DWORD [ESP+n].
    void push_local(Access a){
        const auto bits=load(a,r(esp)+slot(a).offset,4);s_.fp.push(x87_load(bits));s_.fp.status&=~0x200u;
    }
    // BCFB0(ECX object) -> EAX root; CMP EAX,EBX.
    std::uint32_t root_object(Kept&k){
        r(ebx)=k.handle;r(ebp)=k.object;r(esi)=k.fade;r(edi)=k.tags;r(ecx)=r(ebx);x(1)=lane(x(1),k.distance);
        call(0x4bb61u,root_entry);
        k.handle=r(ebx);k.object=r(ebp);k.fade=r(esi);k.tags=r(edi);k.distance=low(x(1));
        s_.flags.minus(r(eax),r(ebx),4);return r(eax);
    }
    // _ftol2 (32CC0C) of ST(0) -> EAX (AL is stored).
    std::uint32_t to_integer(Kept&k){
        r(ebx)=k.handle;r(ebp)=k.object;r(esi)=k.fade;r(edi)=k.tags;
        const auto site=to_integer_site_;to_integer_site_=0x4bc95u;
        call(site,to_integer_entry);
        k.handle=r(ebx);k.object=r(ebp);k.fade=r(esi);k.tags=r(edi);return r(eax)&0xffu;
    }
    void finish(Exit exit){
        auto&f=s_.flags;
        switch(exit){
        case Exit::unfaded:r(ebp)=pop(0x4bb25u);r(esp)=f.plus(r(esp),0xcu,4);pop(0x4bb29u);break;
        case Exit::no_model:r(edi)=pop(0x4bca1u);r(ebp)=pop(0x4bca2u);r(esp)=f.plus(r(esp),0xcu,4);pop(0x4bca6u);break;
        case Exit::faded:r(esi)=pop(0x4bca0u);r(edi)=pop(0x4bca1u);r(ebp)=pop(0x4bca2u);r(esp)=f.plus(r(esp),0xcu,4);pop(0x4bca6u);break;
        }
        r(esp)+=8u;
    }
private:
    std::uint32_t to_integer_site_{0x4bc80u};
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::object_fade::fade(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_fade_detail
