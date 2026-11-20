#pragma once
// Boundary adapter for render-object submission
// (native_tu15_object_submit_systems.hpp): 1362A0, called with the view in ESI
// and five stack arguments (RET 20). Calls 4BAE0 (object_fade), 135E00
// (object_draw_flags), and 1365C0 (model_hierarchy) through the unit's
// dispatcher, and 4B5C0 (source_model_build) through the State's required
// boundary.
//
// Admission: TU1.5 image, no MMX alias, x87 depth <= 3 (body pushes 5 slots),
// the exact live instruction-cursor contract, and a certified required owner.
// HALO2_REFERENCE_OBJECT_SUBMIT=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (1362A0..1365BF holds no service PC).
#include "rendering/native_tu15_object_submit_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <thread>
#if defined(__x86_64__) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_object_submit_detail {
namespace logic=halo2::native_object_submit;
using logic::Access;
using logic::Kept;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_SUBMIT");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>3)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x1362a0u,0x1365bfu)){
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

inline constexpr std::uint32_t object_fade_entry=0x4bae0u;
inline constexpr std::uint32_t draw_flags_entry=0x135e00u;
inline constexpr std::uint32_t model_build_entry=0x4b5c0u;
inline constexpr std::uint32_t model_hierarchy_entry=0x1365c0u;

#if defined(__x86_64__) && !defined(_MSC_VER)
inline __m128 addss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 subss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 mulss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 divss(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline std::uint32_t comiss(__m128 a,__m128 b){
    unsigned char below,equal,unordered;
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u);
}
inline long double x87_load_m32(std::uint32_t bits){
    long double out;asm volatile("flds %1;fstpt %0":"=m"(out):"m"(bits):"memory","st");return out;
}
inline long double x87_sub_m32(long double value,std::uint32_t bits,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fsubs %3;fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(value),"m"(bits):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline long double x87_mul_m32(long double value,std::uint32_t bits,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fmuls %3;fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(value),"m"(bits):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline long double x87_mul_rr(long double a,long double b,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fldt %3;fmulp %%st,%%st(1);fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(a),"m"(b):"memory","st","st(1)");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline long double x87_add_rr(long double a,long double b,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fldt %3;faddp %%st,%%st(1);fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(a),"m"(b):"memory","st","st(1)");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline long double x87_sqrt(long double value,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fsqrt;fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(value):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline std::uint32_t x87_spill_m32(long double value,std::uint16_t&status){
    std::uint32_t out;std::uint16_t word;
    asm volatile("fldt %2;fstps %0;fnstsw %1":"=m"(out),"=m"(word):"m"(value):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    std::uint32_t frame_esp_{};
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    static __m128 loaded_float(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width,bool positioned=false){
        if(!positioned)at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    std::uint32_t before(Access a){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case pass_kind:return 0x1362a0u;
        case retry_arg_entry:return 0x1362b1u;
        case clear_force_flag:return 0x1362bau;
        case set_force_flag:return 0x1362c1u;
        case queue_ptr:return 0x1362c6u;
        case handles_ptr:return 0x1362c9u;
        case object_handle:return 0x1362d6u;
        case object_table:
            r(eax)=r(ebx);
            r(eax)=f.logic(r(eax)&0xffffu,4);
            r(edx)=r(eax)*3u;
            return 0x1362e3u;
        case object_data:return 0x1362e8u;
        case object_ptr:return 0x1362ebu;
        case bounding_radius:return 0x1362efu;
        case center_x:r(edx)=r(eax)+0x30u;return 0x1362f7u;
        case store_pos_x:return 0x1362f9u;
        case center_y:return 0x136300u;
        case center_z:return 0x136303u;
        case store_pos_y:return 0x136306u;
        case reload_pos_y:return 0x13630du;
        case plane_y:return 0x136316u;
        case store_pos_z:return 0x13631eu;
        case reload_pos_z:return 0x136325u;
        case plane_z:return 0x13632eu;
        case reload_pos_x:
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);
            return 0x13633au;
        case plane_x:return 0x136343u;
        case plane_d:
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);
            return 0x13634fu;
        case store_handle:return 0x13635du;
        case store_radius:return 0x136361u;
        case min_plane_dist:return 0x136373u;
        case screen_scale:return 0x13638au;
        case proj_scale:return 0x1363b5u;
        case lod_bias:return 0x1363c3u;
        case store_proj_size:return 0x1363dbu;
        case reload_cam_dist:return 0x136406u;
        case flags_queue:return 0x136414u;
        case flags_array:return 0x136417u;
        case row_flags:return 0x13641au;
        case sort_counter_arg:
            s_.xmm[0]=_mm_setzero_ps();
            return 0x136427u;
        case store_render_flags:return 0x13642eu;
        case store_vis_mask_byte:
            r(eax)=r(edx);
            r(eax)=f.right(r(eax),9u,4,false);
            s_.set(eax,f.logic(r(eax)&1u,1),1,0);
            return 0x13643cu;
        case initial_slot_arg:return 0x136443u;
        case store_sort_slot:
            r(ebx)=f.logic(0u,4);
            return 0x13644cu;
        case store_cam_depth:return 0x136459u;
        case store_has_sort:return 0x136462u;
        case store_sort_counter:return 0x136469u;
        case store_override_shader:return 0x136470u;
        case store_use_alpha:return 0x136477u;
        case store_override_alpha:return 0x13647eu;
        case store_override_param:return 0x136487u;
        case store_reserved_1a4:return 0x136490u;
        case store_vis_index_default:return 0x136497u;
        case vis_array:return 0x1364a1u;
        case vis_index:return 0x1364a4u;
        case store_extra_visible:return 0x1364adu;
        case store_vis_index:return 0x1364b8u;
        case store_extra_default:return 0x1364c2u;
        case post_extra_flags:return 0x136506u;
        case post_retry_arg:return 0x13650du;
        case store_post_extra:
            r(ecx)=f.logic(r(ecx)|0x208u,4);
            f.minus(r(eax)&0xffu,r(ebx)&0xffu,1);
            return 0x13651cu;
        case retry_render_flags:return 0x136526u;
        case retry_clear_skip:
            s_.set(eax,f.logic(0u,1),1,0);
            r(edx)=f.logic(r(edx)&0xfffffffdu,4);
            r(edx)=f.logic(r(edx)|4u,4);
            return 0x136535u;
        case retry_store_render_flags:return 0x13653cu;
        case check_skip_children:return 0x136545u;
        case check_pass_kind_secondary:return 0x136550u;
        case clear_render_bit2:return 0x136559u;
        case check_fade_byte:return 0x136561u;
        case check_layer_flags:return 0x13656au;
        case check_pass_kind_primary:return 0x136574u;
        case hierarchy_force_dword:return 0x13657cu;
        case hierarchy_include_arg:return 0x136580u;
        case return_skip_children:return 0x136596u;
        case cull_queue:return 0x1365a8u;
        case cull_flags_array:return 0x1365abu;
        case cull_or_bit0:return 0x1365aeu;
        default:return 0;
        }
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case pass_kind:r(eax)=v;break;
        case retry_arg_entry:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case queue_ptr:r(eax)=v;break;
        case handles_ptr:r(ecx)=v;break;
        case object_handle:r(ebx)=v;break;
        case object_table:r(eax)=v;break;
        case object_data:r(ecx)=v;break;
        case object_ptr:r(eax)=v;break;
        case bounding_radius:s_.xmm[2]=loaded_float(v);break;
        case center_x:r(ecx)=v;break;
        case center_y:r(ecx)=v;break;
        case center_z:r(edx)=v;break;
        case reload_pos_y:s_.xmm[1]=loaded_float(v);break;
        case plane_y:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case reload_pos_z:s_.xmm[0]=loaded_float(v);break;
        case plane_z:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case reload_pos_x:s_.xmm[1]=loaded_float(v);break;
        case plane_x:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case plane_d:s_.xmm[0]=addss(s_.xmm[0],loaded_float(v));break;
        case min_plane_dist:s_.xmm[1]=loaded_float(v);break;
        case screen_scale:s_.xmm[1]=loaded_float(v);break;
        case proj_scale:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case lod_bias:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case reload_cam_dist:s_.xmm[1]=loaded_float(v);break;
        case flags_queue:r(edi)=v;break;
        case flags_array:r(edx)=v;break;
        case row_flags:r(edx)=logic::widen16(v);break;
        case sort_counter_arg:r(ecx)=v;break;
        case initial_slot_arg:r(eax)=v;break;
        case vis_array:r(ecx)=v;break;
        case vis_index:s_.set(ecx,v,2,0);f.minus(v&0xffffu,r(eax)&0xffffu,2);break;
        case post_extra_flags:r(ecx)=v;break;
        case post_retry_arg:s_.set(eax,v,1,0);break;
        case retry_render_flags:r(edx)=v;break;
        case check_skip_children:s_.set(eax,v,1,0);f.minus(v&0xffu,r(ebx)&0xffu,1);break;
        case check_pass_kind_secondary:f.minus(v,1u,4);break;
        case check_fade_byte:f.minus(v&0xffu,r(ebx)&0xffu,1);break;
        case check_layer_flags:f.logic(v&2u,1);break;
        case check_pass_kind_primary:f.minus(v,r(ebx),4);break;
        case hierarchy_force_dword:r(eax)=v;break;
        case hierarchy_include_arg:r(ecx)=v;break;
        case return_skip_children:s_.set(eax,v,1,0);break;
        case cull_queue:r(ecx)=v;break;
        case cull_flags_array:r(edx)=v;break;
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
    void store8(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value&0xffu,1);}
    void store_float(Access a,std::uint32_t address,float value){
        const auto pc=before(a);
        const auto bits=std::bit_cast<std::uint32_t>(value);
        if(a==Access::store_proj_size)s_.xmm[1]=loaded_float(bits);
        write(pc,address,bits,4);
    }

    // ---- Scalar SSE arithmetic
    float mul_scalar(float a,float b){
        const auto r=mulss(_mm_set_ss(a),_mm_set_ss(b));
        return _mm_cvtss_f32(r);
    }
    float add_scalar(float a,float b){
        const auto r=addss(_mm_set_ss(a),_mm_set_ss(b));
        return _mm_cvtss_f32(r);
    }
    float div_scalar(float a,float b){
        s_.xmm[1]=divss(s_.xmm[1],s_.xmm[0]);
        return _mm_cvtss_f32(s_.xmm[1]);
    }

    // ---- Frame & ABI helpers
    std::uint32_t entry_view()const{return s_.gp[esi];}
    void allocate_frame(){
        r(esp)=s_.flags.minus(r(esp),0x1ecu,4);
        frame_esp_=r(esp);
        s_.flags.minus(r(eax),2u,4);
    }
    std::uint32_t force_flag_slot()const{return frame_esp_+3u;}
    std::uint32_t submission_address()const{return frame_esp_+8u;}
    std::uint32_t arg_retry()const{return frame_esp_+0x1f4u;}
    std::uint32_t arg_retry_with_edi()const{return frame_esp_+0x1f4u;}
    std::uint32_t arg_sort_counter()const{return frame_esp_+0x1f8u;}
    std::uint32_t arg_initial_slot()const{return frame_esp_+0x1fcu;}
    std::uint32_t arg_include_children()const{return frame_esp_+0x200u;}

    std::uint32_t enter_body_and_load_slot(){
        push(0x1362ccu,r(ebx));
        push(0x1362cdu,r(ebp));
        at(0x1362ceu);const auto v=word(frame_esp_+0x1f0u,2);
        r(ebp)=logic::widen16(v);
        return v;
    }
    bool compare_plane_zero(float){
        s_.xmm[1]=_mm_setzero_ps();
        s_.flags.load(comiss(s_.xmm[0],s_.xmm[1]));
        return s_.flags.cf;
    }
    float negate_plane(float){
        s_.xmm[1]=subss(s_.xmm[1],s_.xmm[0]);
        s_.xmm[0]=s_.xmm[1];
        return _mm_cvtss_f32(s_.xmm[0]);
    }
    bool clamp_plane_min(float,float){
        s_.flags.load(comiss(s_.xmm[0],s_.xmm[1]));
        if(s_.flags.cf||s_.flags.zf){s_.xmm[0]=s_.xmm[1];return true;}
        return false;
    }

    // ---- x87 distance sequence
    void x87_load(Access a,std::uint32_t address){
        const std::uint32_t pc=(a==Access::x87_pos_x)?0x136383u:(a==Access::x87_pos_y?0x13639cu:0x1363aeu);
        if(a==Access::x87_pos_z)s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[2]);
        at(pc);const auto bits=word(address,4);
        s_.fp.push(x87_load_m32(bits));
        s_.fp.status&=~0x200u;
    }
    void save_edi(){push(0x1363a3u,r(edi));}
    void x87_sub(Access a,std::uint32_t address){
        const std::uint32_t pc=(a==Access::x87_cam_x)?0x136392u:(a==Access::x87_cam_y?0x1363a4u:0x1363bdu);
        at(pc);const auto bits=word(address,4);
        s_.fp.st(0)=x87_sub_m32(s_.fp.st(0),bits,s_.fp.status);
    }
    void push_shadow_fade_out(){
        r(eax)=frame_esp_+7u;
        push(0x1363cfu,r(eax));
    }
    void x87_square_dz(){
        s_.fp.push(s_.fp.st(0));s_.fp.status&=~0x200u;
        r(ecx)=frame_esp_+0x1c4u;
        s_.fp.st(0)=x87_mul_rr(s_.fp.st(0),s_.fp.st(1),s_.fp.status);
    }
    void x87_dup_dy_and_push_fade_out(){
        s_.fp.push(s_.fp.st(2));s_.fp.status&=~0x200u;
        push(0x1363e6u,r(ecx));
    }
    void x87_finish_distance_and_scale(Access,std::uint32_t address){
        s_.fp.st(0)=x87_mul_rr(s_.fp.st(0),s_.fp.st(3),s_.fp.status);
        s_.fp.st(1)=x87_add_rr(s_.fp.st(1),s_.fp.st(0),s_.fp.status);s_.fp.pop();
        s_.fp.push(s_.fp.st(3));s_.fp.status&=~0x200u;
        s_.fp.st(0)=x87_mul_rr(s_.fp.st(0),s_.fp.st(4),s_.fp.status);
        s_.fp.st(1)=x87_add_rr(s_.fp.st(1),s_.fp.st(0),s_.fp.status);s_.fp.pop();
        s_.fp.st(0)=x87_sqrt(s_.fp.st(0),s_.fp.status);
        s_.fp.st(3)=s_.fp.st(0);s_.fp.status&=~0x200u;s_.fp.pop();
        s_.fp.status&=~0x200u;s_.fp.pop();
        s_.fp.status&=~0x200u;s_.fp.pop();
        at(0x1363f9u);const auto bits=word(address,4);
        s_.fp.st(0)=x87_mul_m32(s_.fp.st(0),bits,s_.fp.status);
    }
    void x87_store_distance(Access,std::uint32_t address){
        at(0x1363ffu);
        auto next_status=s_.fp.status;
        const auto bits=x87_spill_m32(s_.fp.st(0),next_status);
        Base::store(0x1363ffu,address,bits,[&]{s_.fp.status=next_status;s_.fp.pop();},true);
    }

    // ---- Child calls & RMW
    void call_object_fade(Kept&k,float){
        push(0x13640fu,0x136414u);
        call_(s_,object_fade_entry,0x13640fu,before_);
        k.view=r(esi);k.slot_index=r(ebp);k.handle=r(ebx);
    }
    std::uint32_t call_draw_flags(Kept&k){
        push(0x13641eu,r(esi));
        push(0x13641fu,0x136424u);
        call_(s_,draw_flags_entry,0x13641fu,before_);
        k.view=r(esi);k.slot_index=r(ebp);k.queue=r(edi);k.row_flags=r(edx);
        return r(eax);
    }
    bool prepare_defaults_and_test_vis(std::uint32_t){
        r(eax)=s_.flags.logic(r(eax)|0xffffffffu,4);
        const auto dh=(r(edx)>>8u)&0xffu;
        s_.flags.logic(dh&8u,1);
        return !s_.flags.zf;
    }
    void call_model_build(Kept&k){
        const auto sub=submission_address();
        at(0x1364cdu);r(edx)=word(sub+0x1a8u,4);
        push(0x1364d4u,r(edx));
        at(0x1364d5u);r(edx)=word(sub+0x1b4u,4);
        r(eax)=sub+0x194u;
        push(0x1364e3u,r(eax));
        at(0x1364e4u);r(eax)=word(sub+0x1b8u,4);
        r(ecx)=sub+4u;
        push(0x1364efu,r(ecx));
        at(0x1364f0u);r(ecx)=word(sub,4);
        push(0x1364f4u,r(edx));
        at(0x1364f5u);r(edx)=word(r(esi)+0x2a78u,4);
        push(0x1364fbu,r(eax));
        push(0x1364fcu,r(ecx));
        at(0x1364fdu);s_.set(ecx,word(frame_esp_+3u,1),1,0);
        push(0x136501u,0x136506u);
        s_.required(0x136501u,model_build_entry,false);
        k.view=r(esi);k.slot_index=r(ebp);k.zero=r(ebx);
    }
    void restore_edi(){r(edi)=pop(0x136523u);}
    void and32(Access a,std::uint32_t address,std::uint32_t mask){
        const auto pc=before(a);at(pc);
        const auto next=word(address,4)&mask;
        Base::store(pc,address,next,[&]{s_.flags.logic(next,4);},true);
    }
    void or8(Access a,std::uint32_t address,std::uint32_t mask){
        const auto pc=before(a);at(pc);
        const auto next=(word(address,1)|mask)&0xffu;
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,1>bytes{std::byte(next)};
        io([&]{s_.memory.write(address,bytes);});
        s_.flags.logic(next,1);
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
    }
    void call_model_hierarchy(Kept&k,std::uint32_t force_dword,std::uint32_t include_children){
        push(0x136587u,force_dword);
        push(0x136588u,k.zero);
        push(0x136589u,include_children);
        r(edx)=submission_address();
        push(0x13658eu,r(edx));
        r(ecx)=r(esi);
        push(0x136591u,0x136596u);
        call_(s_,model_hierarchy_entry,0x136591u,before_);
        k.view=r(esi);k.slot_index=r(ebp);k.zero=r(ebx);
    }
    void finish_submitted(std::uint32_t){
        r(ebp)=pop(0x13659du);
        r(ebx)=pop(0x13659eu);
        r(esp)=s_.flags.plus(r(esp),0x1ecu,4);
        pop(0x1365a5u);
        r(esp)+=20u;
    }
    void finish_culled(std::uint32_t cell){
        r(ecx)=cell;
        finish_skipped();
    }
    void finish_skipped(){
        r(ebp)=pop(0x1365b5u);
        r(ebx)=pop(0x1365b6u);
        r(esp)=s_.flags.plus(r(esp),0x1ecu,4);
        pop(0x1365bdu);
        r(esp)+=20u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::object_submit::run(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_submit_detail
