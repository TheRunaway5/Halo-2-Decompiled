#pragma once
// Boundary adapter for render-view visibility setup
// (native_tu15_view_setup_systems.hpp): 135690, called with view_flags in ECX,
// node_count in BX, and 9 stack arguments (RET 0x24). Calls 11E600 through
// the unit's dispatcher when the stack sentinel at [ESP+0x1C] is -1. Executes
// no x87 instructions, only scalar SSE (MOVSS, ADDSS, COMISS).
//
// Admission: TU1.5 image on x86-64, no MMX alias, the exact live
// instruction-cursor contract, and a certified required owner.
// HALO2_REFERENCE_VIEW_SETUP=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (135690..13586A holds no service PC).
#include "rendering/native_tu15_view_setup_systems.hpp"
#include "rendering/native_tu15_receiver_transfers.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include <xmmintrin.h>
#endif

namespace halo2::native_view_setup_detail {
namespace logic=halo2::native_view_setup;
using logic::Access;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_VIEW_SETUP");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x135690u,0x13586au)){
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

inline constexpr std::uint32_t init_handle_entry=0x11e600u;

#if defined(__x86_64__) && !defined(_MSC_VER)
struct Multiply {std::uint32_t value,flags;};
// IMUL ECX,ECX,0x1BC; MOV EDX,ECX; SHR ECX,2 (hardware OF/AF reaches REP MOVSD at 135768).
inline Multiply multiply_node_stride(std::uint32_t count,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;imull $0x1bc,%1,%1;shrl $2,%1;lahf;seto %2":"+a"(ah),"+r"(count),"=qm"(overflow)::"cc");
    return{count,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;using Base::store;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    static __m128 float_lane(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
    static std::uint32_t lane_bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}

    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }

    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case arg_tag_high:return 0x135690u;
        case arg_threshold:return 0x135695u;
        case arg_bias:return 0x13569bu;
        // PUSH EBP (1356A1)
        case arg_view:push(0x1356a1u,r(ebp));return 0x1356a2u;
        case store_tag_high:return 0x1356a6u;
        case global_tag_low:return 0x1356aau;
        case store_tag_low:return 0x1356b1u;
        case arg_kind:return 0x1356b5u;
        case store_first_handle:return 0x1356b9u;
        // LEA EAX,[EBP+4]; PUSH ESI
        case arg_origin:return 0x1356c4u;
        case store_kind:return 0x1356c8u;
        case store_flags_initial:return 0x1356ceu;
        case store_node_count:return 0x1356d4u;
        // PUSH EDI (1356DB)
        case load_origin_x:return 0x1356dcu;
        // LEA EDX,[EBP+2A68]
        case store_origin_x:r(edx)=r(ebp)+0x2a68u;return 0x1356e4u;
        case load_origin_y:return 0x1356e6u;
        case store_origin_y:return 0x1356e9u;
        case load_origin_z:return 0x1356ecu;
        case store_origin_z:return 0x1356efu;
        case arg_use_plane:return 0x1356f2u;
        case store_threshold:return 0x1356f8u;
        case store_bias:return 0x135700u;
        case arg_source_plane:return 0x13570eu;
        // OR ECX,200
        case store_flags_plane:r(ecx)=f.logic(r(ecx)|0x200u,4);return 0x135718u;
        // LEA ECX,[ESI+74]
        case load_plane_0:r(ecx)=r(esi)+0x74u;return 0x135721u;
        // LEA EDX,[EBP+2A88]
        case store_plane_0:r(edx)=r(ebp)+0x2a88u;return 0x135729u;
        case load_plane_1:return 0x13572bu;
        case store_plane_1:return 0x13572eu;
        case load_plane_2:return 0x135731u;
        case store_plane_2:return 0x135734u;
        case load_plane_3:return 0x135737u;
        case store_plane_3:return 0x13573au;
        case load_plane_d:return 0x13573du;
        case store_plane_d:return 0x135749u;
        case arg_source_no_plane:return 0x135822u;
        // AND ECX,FFFFFDFF
        case store_flags_no_plane:r(ecx)=f.logic(r(ecx)&0xfffffdffu,4);return 0x13582cu;
        case load_node_buffer:return 0x135751u;
        case load_node_buffer_again:return 0x135771u;
        case store_buffer_count:return 0x135774u;
        case check_flag_sentinel:return 0x135777u;
        case load_bias_for_call:return 0x13577eu;
        case arg_flag_ptr:return 0x135789u;
        // XOR EAX,EAX; CMP ECX,EAX
        case store_flag_ptr:r(eax)=f.logic(0u,4);f.minus(r(ecx),r(eax),4);return 0x135791u;
        // MOV EDX,1
        case load_flags_for_clear:r(edx)=1u;return 0x13579eu;
        case store_clear_0:return 0x1357a4u;
        // OR ECX,EDX
        case store_flags_with_ptr:r(ecx)=f.logic(r(ecx)|r(edx),4);return 0x1357acu;
        case store_clear_1:return 0x1357b2u;
        case store_clear_2:return 0x1357b8u;
        case store_clear_3:return 0x1357beu;
        case store_clear_4:return 0x1357c4u;
        case store_clear_5:return 0x1357cau;
        case store_clear_6:return 0x1357d0u;
        case store_clear_7:return 0x1357d6u;
        case store_clear_8:return 0x1357dcu;
        case store_clear_9:return 0x1357e2u;
        case store_clear_10:return 0x1357e8u;
        case store_clear_11:return 0x1357eeu;
        case store_clear_12:return 0x1357f4u;
        case check_view_kind:r(edx)=1u;return 0x1357fau;
        case load_flags_low_byte:return 0x135802u;
        case store_status_two:return 0x13580du;
        case reload_status_two:return 0x135817u;
        case load_one_constant:return 0x13583bu;
        case load_flags_for_bit8:return 0x135848u;
        case store_status_zero:return 0x135855u;
        case store_status_one:return 0x135861u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case arg_tag_high:s_.set(edx,v,2,0);break;
        case arg_threshold:s_.xmm[3]=float_lane(v);break;
        case arg_bias:s_.xmm[0]=float_lane(v);break;
        case arg_view:r(ebp)=v;break;
        case global_tag_low:s_.set(edx,v,2,0);break;
        case arg_kind:r(edx)=v;break;
        case arg_origin:r(esi)=v;break;
        case load_origin_x:r(edi)=v;break;
        case load_origin_y:r(edi)=v;break;
        case load_origin_z:r(esi)=v;break;
        case arg_use_plane:s_.set(edx,v,1,0);f.logic(v&0xffu,1);break;
        case arg_source_plane:r(esi)=v;break;
        case load_plane_0:r(edi)=v;break;
        case load_plane_1:r(edi)=v;break;
        case load_plane_2:r(edi)=v;break;
        case load_plane_3:r(ecx)=v;break;
        case load_plane_d:s_.xmm[1]=float_lane(v);break;
        case arg_source_no_plane:r(esi)=v;break;
        // MOV EDI,[EBP]; ADD EDI,4; CMP ESI,EDI
        case load_node_buffer:
            r(edi)=v;
            r(edi)=f.plus(r(edi),4u,4);
            f.minus(r(esi),r(edi),4);
            break;
        case load_node_buffer_again:r(ecx)=v;break;
        case check_flag_sentinel:f.minus(v,0xffffffffu,4);break;
        case load_bias_for_call:r(edx)=v;break;
        case arg_flag_ptr:r(ecx)=v;break;
        case load_flags_for_clear:r(ecx)=v;break;
        case check_view_kind:f.minus(v,r(eax),4);break;
        case load_flags_low_byte:s_.set(ecx,v,1,0);f.logic(v&0xffu,1);break;
        case reload_status_two:r(eax)=v;break;
        case load_one_constant:s_.xmm[0]=float_lane(v);break;
        // MOV ECX,[EBP+2A60]; TEST CH,1
        case load_flags_for_bit8:
            r(ecx)=v;
            f.logic((v>>8u)&1u,1);
            break;
        default:break;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);at(pc);const auto v=word(address,width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Call&call,Before&before):Base{s,before},call_(call){}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}
    std::uint32_t load8(Access a,std::uint32_t address){return load(a,address,1);}
    float load_float(Access a,std::uint32_t address){return std::bit_cast<float>(load(a,address,4));}
    void store32(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value,4);}
    void store16(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value&0xffffu,2);}
    void store_float(Access a,std::uint32_t address,float value){write(before(a),address,std::bit_cast<std::uint32_t>(value),4);}

    // ---- Frame
    std::uint32_t entry_flags()const{return s_.gp[ecx];}
    std::uint32_t entry_node_count()const{return s_.gp[ebx]&0xffffu;}

    std::uint32_t read_arg16(Access a,std::uint32_t offset){return load(a,r(esp)+offset,2);}
    std::uint32_t read_arg32(Access a,std::uint32_t offset){return load(a,r(esp)+offset,4);}
    std::uint32_t read_arg8(Access a,std::uint32_t offset){return load(a,r(esp)+offset,1);}
    float read_arg_float(Access a,std::uint32_t offset){return std::bit_cast<float>(load(a,r(esp)+offset,4));}

    std::uint32_t read_view_arg(Access a){
        const auto pc=before(a);at(pc);const auto v=word(r(esp)+8u,4);loaded(a,v);return v;
    }
    std::uint32_t read_origin_arg(Access a,std::uint32_t first_handle_ptr){
        r(eax)=first_handle_ptr;
        push(0x1356c3u,r(esi));
        const auto pc=before(a);at(pc);const auto v=word(r(esp)+0x20u,4);loaded(a,v);return v;
    }
    std::uint32_t read_first_origin_x(Access a,std::uint32_t origin){
        push(0x1356dbu,r(edi));
        return load(a,origin,4);
    }
    float add_plane_bias(float pd,float bias){
        __m128 x=float_lane(std::bit_cast<std::uint32_t>(pd));
        __m128 y=float_lane(std::bit_cast<std::uint32_t>(bias));
        asm volatile("addss %1,%0":"+x"(x):"x"(y));
        s_.xmm[1]=x;
        return std::bit_cast<float>(lane_bits(x));
    }
    void copy_frustum_nodes(std::uint32_t src,std::uint32_t dst,std::uint32_t,std::uint32_t){
        if(src==dst)return;
        const auto total=(r(ebx)*0x1bcu)&0xffffffffu;
        const auto mul=multiply_node_stride(r(ebx),s_.flags.bits());
        r(edx)=total;r(ecx)=mul.value;s_.flags.load(mul.flags);
        const auto delta4=(s_.other_flags&0x400u)?0xfffffffcu:4u;
        if(r(ecx) && delta4==4u && r(ebp)==native_receiver_transfers::secondary_view &&
           native_raw_x87_detail::skip_hook<Before>()) {
            if(const auto *scope=native_receiver_transfers::current(s_)) {
                at(0x135768u);
                if(scope->copy_camera(r(esp),r(esi),r(edi),r(ecx))) {
                    const auto bytes=r(ecx)*4u;
                    r(esi)+=bytes;r(edi)+=bytes;r(ecx)=0;
                }
            }
        }
        while(r(ecx)){
            at(0x135768u);
            const auto v=word(r(esi),4);
            store(0x135768u,r(edi),v,[&]{r(esi)+=delta4;r(edi)+=delta4;--r(ecx);},true);
        }
        r(ecx)=s_.flags.logic(r(edx)&3u,4);
        const auto delta1=(s_.other_flags&0x400u)?0xffffffffu:1u;
        while(r(ecx)){
            at(0x13576fu);
            const auto v=word(r(esi),1);
            const auto dst_byte=r(edi);
            SourcePhysicsContactRecordStore event{0x13576fu,dst_byte,{},{}};
            if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(dst_byte,event.before);});}
            std::array<std::byte,1>byte{std::byte(v&0xffu)};
            io([&]{s_.memory.write(dst_byte,byte);});
            r(esi)+=delta1;r(edi)+=delta1;--r(ecx);
            if(s_.observe){event.written.assign(byte.begin(),byte.end());io([&]{s_.observe(event);});}
        }
    }
    void call_init_handle(std::uint32_t first_handle_ptr,std::uint32_t bias_bits){
        r(edx)=bias_bits;
        r(esi)=first_handle_ptr;
        push(0x135784u,0x135789u);
        call_(s_,init_handle_entry,0x135784u,before_);
    }
    // POP EDI; MOV DWORD [EBP+2A7C],2; MOV EAX,[EBP+2A7C]; POP ESI; POP EBP; RET 24
    void finish_status_two(std::uint32_t view){
        r(edi)=pop(0x13580cu);
        store32(Access::store_status_two,logic::at(view,offsetof(logic::RenderView,status)),2u);
        (void)load32(Access::reload_status_two,logic::at(view,offsetof(logic::RenderView,status)));
        r(esi)=pop(0x13581du);
        r(ebp)=pop(0x13581eu);
        pop(0x13581fu);
        r(esp)+=0x24u;
    }
    // CMP EBX,EAX (135837)
    bool node_count_is_zero(){
        s_.flags.minus(r(ebx),r(eax),4);
        return s_.flags.zf;
    }
    // COMISS XMM0,XMM3 (135843); JBE 135853
    bool one_not_above_threshold(float one,float){
        s_.xmm[0]=float_lane(std::bit_cast<std::uint32_t>(one));
        s_.flags.scalar_compare(s_.xmm[0],s_.xmm[3],false);
        return s_.flags.cf||s_.flags.zf;
    }
    // POP EDI; POP ESI; MOV [EBP+2A7C],EAX; POP EBP; RET 24
    void finish_status_zero(std::uint32_t view){
        r(edi)=pop(0x135853u);
        r(esi)=pop(0x135854u);
        store32(Access::store_status_zero,logic::at(view,offsetof(logic::RenderView,status)),r(eax));
        r(ebp)=pop(0x13585bu);
        pop(0x13585cu);
        r(esp)+=0x24u;
    }
    // POP EDI; POP ESI; MOV [EBP+2A7C],EDX; MOV EAX,EDX; POP EBP; RET 24
    void finish_status_one(std::uint32_t view){
        r(edi)=pop(0x13585fu);
        r(esi)=pop(0x135860u);
        store32(Access::store_status_one,logic::at(view,offsetof(logic::RenderView,status)),r(edx));
        r(eax)=r(edx);
        r(ebp)=pop(0x135869u);
        pop(0x13586au);
        r(esp)+=0x24u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::view_setup::initialize(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_view_setup_detail
