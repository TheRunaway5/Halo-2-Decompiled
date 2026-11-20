#pragma once
// Boundary adapter for the render-model hierarchy & layer submission owner
// (native_tu15_model_hierarchy_systems.hpp): 1365C0, called with the view in
// ECX and four stack arguments (RET 16). Calls 136A80 (native_model_draw_submission)
// and itself (1365C0) through the unit's dispatcher, and 4B5C0
// (source_model_build) through the State's required boundary. Executes no
// x87/MMX instructions.
//
// Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
// contract, and a certified required owner. HALO2_REFERENCE_MODEL_HIERARCHY=1
// restores the translated body. HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the
// uncursored census callbacks (1365C0..136A76 holds no service PC).
#include "rendering/native_tu15_model_hierarchy_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_model_hierarchy_detail {
namespace logic=halo2::native_model_hierarchy;
using logic::Access;
using logic::Kept;
using logic::LocalCopy;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_MODEL_HIERARCHY");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x1365c0u,0x136a76u)){
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

inline constexpr std::uint32_t hierarchy_entry=0x1365c0u;
inline constexpr std::uint32_t submit_model_entry=0x136a80u;
inline constexpr std::uint32_t model_build_entry=0x4b5c0u;

#if defined(__x86_64__) && !defined(_MSC_VER)
inline __m128 addss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 subss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 mulss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Hardware {std::uint32_t value,flags;};
// SHR reg,1 (exact OF/AF before PUSH EDX at 13682E and 1368A6).
inline Hardware shift_right1(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;shrl $1,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
// AND reg,FFFF; SHL reg,4 (tag row offset, hardware OF/AF at 136986).
inline Hardware tag_row(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;using Base::store;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
    Call&call_;
    std::uint32_t base_esp_{};
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
        case save_view:return 0x1365dau;
        case direct_arg:return 0x1365e4u;
        case clear_skip:return 0x1365e9u;
        // Translucent pass (0x80)
        case t_flags:return 0x1365f4u;
        case t_pos_y:return 0x136602u;
        case t_pos_z:return 0x13660au;
        case t_cam_z:return 0x136612u;
        case t_pos_x:return 0x13661au;
        case t_cam_y:return 0x136622u;
        case t_fwd_z:return 0x13662au;
        case t_cam_x:return 0x136632u;
        case t_fwd_y:return 0x13663au;
        case t_fwd_x:return 0x136642u;
        case t_has_sort:return 0x13664au;
        case t_store_depth:
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[2]);
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);
            return 0x13665au;
        case t_clear_skip:return 0x136662u;
        case t_sort_counter_ptr:return 0x136669u;
        case t_set_has_sort:return 0x13666fu;
        case t_read_counter:return 0x136676u;
        case t_store_slot:return 0x136678u;
        case t_inc_counter:return 0x13667eu;
        case t_store_kind:return 0x136680u;
        case t_pass_arg:return 0x13669au;
        case t_copy_extra_flags:return 0x13669du;
        case t_copy_shader:return 0x1366a4u;
        case t_copy_weight:return 0x1366abu;
        case t_after_view:return 0x1366ecu;
        // Alpha pass (0x02)
        case a_flags:return 0x1366f0u;
        case a_pos_y:return 0x1366fdu;
        case a_pos_z:return 0x136705u;
        case a_cam_z:return 0x13670du;
        case a_pos_x:return 0x136715u;
        case a_cam_y:return 0x13671du;
        case a_fwd_z:return 0x136725u;
        case a_has_sort:return 0x13672du;
        case a_cam_x:return 0x136735u;
        case a_fwd_y:return 0x13673du;
        case a_fwd_x:return 0x136745u;
        case a_set_skip:
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[2]);
            s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);
            return 0x136755u;
        case a_sort_counter_ptr:return 0x13675cu;
        case a_set_has_sort:return 0x136762u;
        case a_read_counter:return 0x136769u;
        case a_store_slot:return 0x13676bu;
        case a_inc_counter:return 0x136771u;
        case a_store_kind:return 0x136773u;
        case a_extra_flags:return 0x13677du;
        case a_pass_arg:return 0x136793u;
        case a_copy_shader:return 0x136796u;
        case a_copy_weight:return 0x13679du;
        // Overlay A pass (0x01)
        case o1_flags:return 0x1367efu;
        case o1_pass_arg:return 0x1367f9u;
        case o1_copy_render_flags:return 0x13680fu;
        case o1_copy_shader:return 0x136816u;
        case o1_copy_weight:return 0x13681du;
        case o1_store_flags:return 0x136826u;
        // Overlay B pass (0x20)
        case o2_flags:return 0x136867u;
        case o2_pass_arg:return 0x136871u;
        case o2_copy_render_flags:return 0x136887u;
        case o2_copy_shader:return 0x13688eu;
        case o2_copy_weight:return 0x136895u;
        case o2_store_flags:return 0x13689eu;
        // Base submission
        case check_skip:return 0x1368dfu;
        case submit_include_children:return 0x1368e7u;
        // Attached children
        case children_include_arg:return 0x1368f7u;
        case skip_children_flag:return 0x136902u;
        case first_object_table:return 0x136910u;
        case parent_handle:return 0x136916u;
        case first_object_data:return 0x136918u;
        case parent_object:
            r(eax)=f.logic(r(eax)&0xffffu,4);
            r(edx)=f.plus(r(edx),0x44u,4);
            r(eax)=r(eax)*3u;
            return 0x136926u;
        case first_child:return 0x13692au;
        case save_child_handle:return 0x136930u;
        case reload_child_handle:return 0x13693cu;
        case child_sort_kind:return 0x13694du;
        case child_store_handle:return 0x136955u;
        case child_data_ptr:return 0x13695bu;
        case child_object:
            r(ecx)=r(eax);
            r(ecx)=f.logic(r(ecx)&0xffffu,4);
            r(ecx)=r(ecx)*3u;
            return 0x136968u;
        case child_object_type:return 0x13696cu;
        case child_tag_handle:return 0x136975u;
        case child_tag_instances:return 0x136977u;
        case child_tag_def:{
            const auto row=tag_row(r(ecx),f.bits());r(ecx)=row.value;f.load(row.flags);
            return 0x136986u;
        }
        case child_shadow_override:return 0x13698au;
        case child_clear_has_sort:
            s_.xmm[0]=_mm_setzero_ps();
            r(ecx)=0xffffffffu;
            return 0x1369a1u;
        case child_reset_slot:return 0x1369a9u;
        case child_reset_kind:return 0x1369b4u;
        case child_reset_shader:return 0x1369bbu;
        case child_clear_use_alpha:return 0x1369c2u;
        case child_clear_alpha:return 0x1369cau;
        case child_extra_flags:return 0x1369d8u;
        case child_reload_view:return 0x1369dfu;
        case child_store_vis_index:
            r(edx)=f.logic(r(edx)&0xffffffbfu,4);
            r(edx)=f.logic(r(edx)|1u,4);
            return 0x1369e9u;
        case child_vis_mask:return 0x1369f1u;
        case next_child_handle:return 0x136a42u;
        case next_object_table:return 0x136a46u;
        case next_object_data:return 0x136a4cu;
        case next_child_object:
            r(eax)=f.logic(r(eax)&0xffffu,4);
            r(edx)=f.plus(r(edx),0x44u,4);
            r(eax)=r(eax)*3u;
            return 0x136a5au;
        case next_sibling:return 0x136a5eu;
        case store_next_child:return 0x136a64u;
        default:return 0;
        }
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case direct_arg:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case t_flags:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case t_pos_y:s_.xmm[2]=loaded_float(v);break;
        case t_pos_z:s_.xmm[0]=loaded_float(v);break;
        case t_cam_z:s_.xmm[0]=subss(s_.xmm[0],loaded_float(v));break;
        case t_pos_x:s_.xmm[1]=loaded_float(v);break;
        case t_cam_y:s_.xmm[2]=subss(s_.xmm[2],loaded_float(v));break;
        case t_fwd_z:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case t_cam_x:s_.xmm[1]=subss(s_.xmm[1],loaded_float(v));break;
        case t_fwd_y:s_.xmm[2]=mulss(s_.xmm[2],loaded_float(v));break;
        case t_fwd_x:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case t_has_sort:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case t_sort_counter_ptr:r(eax)=v;break;
        case t_read_counter:r(ecx)=v;break;
        case t_pass_arg:r(ecx)=v;break;
        case t_copy_extra_flags:r(edx)=v;break;
        case t_copy_shader:r(eax)=v;break;
        case t_copy_weight:s_.xmm[0]=loaded_float(v);break;
        case t_after_view:r(ecx)=v;break;
        case a_flags:f.logic(v&2u,1);break;
        case a_pos_y:s_.xmm[2]=loaded_float(v);break;
        case a_pos_z:s_.xmm[0]=loaded_float(v);break;
        case a_cam_z:s_.xmm[0]=subss(s_.xmm[0],loaded_float(v));break;
        case a_pos_x:s_.xmm[1]=loaded_float(v);break;
        case a_cam_y:s_.xmm[2]=subss(s_.xmm[2],loaded_float(v));break;
        case a_fwd_z:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case a_has_sort:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case a_cam_x:s_.xmm[1]=subss(s_.xmm[1],loaded_float(v));break;
        case a_fwd_y:s_.xmm[2]=mulss(s_.xmm[2],loaded_float(v));break;
        case a_fwd_x:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case a_sort_counter_ptr:r(eax)=v;break;
        case a_read_counter:r(ecx)=v;break;
        case a_extra_flags:r(edx)=v;break;
        case a_pass_arg:r(ecx)=v;break;
        case a_copy_shader:r(eax)=v;break;
        case a_copy_weight:s_.xmm[1]=loaded_float(v);break;
        case o1_flags:r(eax)=v;f.logic(v&1u,1);break;
        case o1_pass_arg:r(edx)=v;r(eax)=f.logic(r(eax)&0xfffffffeu,4);break;
        case o1_copy_render_flags:r(esi)=v;break;
        case o1_copy_shader:r(ecx)=v;break;
        case o1_copy_weight:s_.xmm[0]=loaded_float(v);break;
        case o2_flags:r(eax)=v;f.logic(v&0x20u,1);break;
        case o2_pass_arg:r(edx)=v;r(eax)=f.logic(r(eax)&0xffffffdfu,4);break;
        case o2_copy_render_flags:r(esi)=v;break;
        case o2_copy_shader:r(ecx)=v;break;
        case o2_copy_weight:s_.xmm[0]=loaded_float(v);break;
        case check_skip:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case submit_include_children:
            s_.set(ecx,v,1,0);f.logic(v&0xffu,1);s_.set(ecx,f.zf?1u:0u,1,0);
            break;
        case children_include_arg:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case skip_children_flag:s_.set(eax,v,1,0);f.logic(v&0xffu,1);break;
        case first_object_table:r(edx)=v;break;
        case parent_handle:r(eax)=v;break;
        case first_object_data:r(ecx)=v;break;
        case parent_object:r(eax)=v;break;
        case first_child:r(eax)=v;f.minus(v,0xffffffffu,4);break;
        case reload_child_handle:r(eax)=v;break;
        case child_sort_kind:f.minus(v,1u,4);break;
        case child_data_ptr:r(edx)=v;break;
        case child_object:r(ecx)=v;break;
        case child_object_type:f.minus(v&0xffu,2u,1);break;
        case child_tag_handle:r(ecx)=v;break;
        case child_tag_instances:r(edx)=v;break;
        case child_tag_def:r(ecx)=v;break;
        case child_shadow_override:
            f.minus(v&0xffffu,0u,2);
            s_.set(ecx,f.zf?0u:1u,1,0);
            f.logic(r(ecx)&0xffu,1);
            break;
        case child_extra_flags:r(edx)=v;break;
        case child_reload_view:r(esi)=v;break;
        case child_vis_mask:r(ecx)=v;break;
        case next_child_handle:r(eax)=v;break;
        case next_object_table:r(edx)=v;break;
        case next_object_data:r(ecx)=v;break;
        case next_child_object:r(eax)=v;break;
        case next_sibling:r(eax)=v;f.minus(v,0xffffffffu,4);break;
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
        if(a==Access::t_store_depth)s_.xmm[0]=loaded_float(bits);
        write(pc,address,bits,4);
    }

    // ---- Scalar SSE arithmetic
    float sub_scalar(float a,float b)const{return _mm_cvtss_f32(subss(_mm_set_ss(a),_mm_set_ss(b)));}
    float mul_scalar(float a,float b)const{return _mm_cvtss_f32(mulss(_mm_set_ss(a),_mm_set_ss(b)));}
    float add_scalar(float a,float b)const{return _mm_cvtss_f32(addss(_mm_set_ss(a),_mm_set_ss(b)));}

    // ---- Frame & RMW
    std::uint32_t entry_view()const{return s_.gp[ecx];}
    std::int16_t enter(Kept&k){
        auto&f=s_.flags;
        push(0x1365c0u,r(ebp));
        r(ebp)=r(esp);
        r(esp)=f.logic(r(esp)&0xfffffff8u,4);
        r(esp)=f.minus(r(esp),0x994u,4);
        push(0x1365ccu,r(ebx));
        at(0x1365cdu);r(ebx)=word(r(ebp)+8u,4);
        at(0x1365d0u);const auto count=word(r(ebx)+0x166u,2);
        f.minus(count,0u,2);
        push(0x1365d8u,r(esi));
        push(0x1365d9u,r(edi));
        base_esp_=r(esp);
        k.submission=r(ebx);
        return std::int16_t(count);
    }
    std::uint32_t skip_slot()const{return base_esp_+0xfu;}
    std::uint32_t view_slot()const{return base_esp_+0x10u;}
    std::uint32_t child_handle_slot()const{return base_esp_+0x14u;}
    std::uint32_t arg_include_children()const{return s_.gp[ebp]+0xcu;}
    std::uint32_t arg_direct()const{return s_.gp[ebp]+0x10u;}
    std::uint32_t arg_pass_param()const{return s_.gp[ebp]+0x14u;}
    std::uint32_t copy_address(LocalCopy which)const{
        switch(which){
        case LocalCopy::child:return base_esp_+0x18u;
        case LocalCopy::alpha:return base_esp_+0x200u;
        case LocalCopy::translucent:return base_esp_+0x3e8u;
        case LocalCopy::overlay_a:return base_esp_+0x5d0u;
        case LocalCopy::overlay_b:return base_esp_+0x7b8u;
        }
        return base_esp_;
    }
    void increment32(Access a,std::uint32_t address){
        const auto pc=before(a);
        at(pc);
        const auto cur=word(address,4);
        store(pc,address,cur+1u,[&]{s_.flags.increment(cur,4,false);},true);
    }
    void copy_submission(LocalCopy which,std::uint32_t src,std::uint32_t dst){
        std::uint32_t pc=0;
        switch(which){
        case LocalCopy::translucent:pc=0x136698u;break;
        case LocalCopy::alpha:pc=0x136791u;break;
        case LocalCopy::overlay_a:pc=0x13680du;break;
        case LocalCopy::overlay_b:pc=0x136885u;break;
        case LocalCopy::child:pc=0x13694bu;break;
        }
        r(ecx)=0x79u;r(esi)=src;r(edi)=dst;
        const auto delta=(s_.other_flags&0x400u)?0xfffffffcu:4u;
        while(r(ecx)){
            at(pc);
            const auto value=word(r(esi),4);
            store(pc,r(edi),value,[&]{r(esi)+=delta;r(edi)+=delta;--r(ecx);},true);
        }
    }
    void submit_translucent(Kept&k,std::uint32_t dst,std::uint32_t pass_param,std::uint32_t extra,
                            std::uint32_t shader,float weight){
        r(ecx)=pass_param;r(edx)=extra;r(eax)=shader;s_.xmm[0]=loaded_float(std::bit_cast<std::uint32_t>(weight));
        push(0x1366b4u,r(ecx));
        at(0x1366b5u);r(ecx)=word(base_esp_+0x10u,4);
        push(0x1366b9u,1u);
        write(0x1366bbu,dst+0x1acu,r(edx),4);
        push(0x1366c2u,1u);
        r(edx)=dst;
        push(0x1366cbu,r(edx));
        write(0x1366ccu,dst+0x1b0u,0u,4);
        write(0x1366d7u,dst+0x1d0u,r(eax),4);
        write(0x1366deu,dst+0x1dcu,std::bit_cast<std::uint32_t>(weight),4);
        push(0x1366e7u,0x1366ecu);
        call_(s_,hierarchy_entry,0x1366e7u,before_);
        k.submission=r(ebx);
    }
    void submit_alpha(Kept&k,std::uint32_t dst,std::uint32_t pass_param,std::uint32_t extra,
                      std::uint32_t shader,float weight,float depth){
        r(ecx)=pass_param;r(edx)=extra;r(eax)=shader;
        s_.xmm[1]=loaded_float(std::bit_cast<std::uint32_t>(weight));
        s_.xmm[0]=loaded_float(std::bit_cast<std::uint32_t>(depth));
        push(0x1367a6u,r(ecx));
        at(0x1367a7u);r(ecx)=word(base_esp_+0x10u,4);
        push(0x1367abu,1u);
        write(0x1367adu,dst+0x1acu,r(edx),4);
        push(0x1367b4u,1u);
        r(edx)=dst;
        push(0x1367bdu,r(edx));
        write(0x1367beu,dst+0x1b0u,0u,4);
        write(0x1367c9u,dst+0x1d0u,r(eax),4);
        write(0x1367d0u,dst+0x1d4u,1u,1);
        write(0x1367d8u,dst+0x1d8u,std::bit_cast<std::uint32_t>(weight),4);
        write(0x1367e1u,dst+0x1e0u,std::bit_cast<std::uint32_t>(depth),4);
        push(0x1367eau,0x1367efu);
        call_(s_,hierarchy_entry,0x1367eau,before_);
        k.submission=r(ebx);
    }
    void submit_overlay_a(Kept&k,std::uint32_t dst,std::uint32_t pass_param,std::uint32_t cleared_flags,
                          std::uint32_t rflags,std::uint32_t shader,float weight){
        auto&f=s_.flags;
        const auto shifted=shift_right1(cleared_flags,f.bits());r(eax)=shifted.value;f.load(shifted.flags);
        push(0x13682eu,pass_param);
        s_.set(eax,f.logic(r(eax)&1u,1),1,0);
        push(0x136831u,1u);
        write(0x136833u,base_esp_+0xfu,r(eax)&0xffu,1);
        push(0x136837u,0u);
        r(eax)=dst;
        r(esi)=f.logic(rflags,4);
        write(0x136846u,dst+0x1d0u,shader,4);
        at(0x13684du);r(ecx)=word(base_esp_+0x10u,4);
        push(0x136851u,r(eax));
        write(0x136852u,dst+0x1acu,r(esi),4);
        write(0x136859u,dst+0x1dcu,std::bit_cast<std::uint32_t>(weight),4);
        push(0x136862u,0x136867u);
        call_(s_,hierarchy_entry,0x136862u,before_);
        k.submission=r(ebx);
    }
    void submit_overlay_b(Kept&k,std::uint32_t dst,std::uint32_t pass_param,std::uint32_t cleared_flags,
                          std::uint32_t rflags,std::uint32_t shader,float weight){
        auto&f=s_.flags;
        const auto shifted=shift_right1(cleared_flags,f.bits());r(eax)=shifted.value;f.load(shifted.flags);
        push(0x1368a6u,pass_param);
        s_.set(eax,f.logic(r(eax)&1u,1),1,0);
        push(0x1368a9u,1u);
        write(0x1368abu,base_esp_+0xfu,r(eax)&0xffu,1);
        push(0x1368afu,0u);
        r(eax)=dst;
        r(esi)=f.logic(rflags,4);
        write(0x1368beu,dst+0x1d0u,shader,4);
        at(0x1368c5u);r(ecx)=word(base_esp_+0x10u,4);
        push(0x1368c9u,r(eax));
        write(0x1368cau,dst+0x1acu,r(esi),4);
        write(0x1368d1u,dst+0x1dcu,std::bit_cast<std::uint32_t>(weight),4);
        push(0x1368dau,0x1368dfu);
        call_(s_,hierarchy_entry,0x1368dau,before_);
        k.submission=r(ebx);
    }
    void submit_base(Kept&k,std::uint32_t suppress_children){
        s_.set(ecx,suppress_children,1,0);
        r(eax)=k.submission;
        push(0x1368f1u,r(ecx));
        push(0x1368f2u,0x1368f7u);
        call_(s_,submit_model_entry,0x1368f2u,before_);
        k.submission=r(ebx);
    }
    void set_default_child_index(){
        r(ecx)=s_.flags.logic(r(ecx)|0xffffffffu,4);
    }
    void build_and_submit_child(Kept&k,std::uint32_t dst,std::uint32_t child_handle,
                                std::uint32_t updated_extra,std::uint32_t vis_mask){
        r(eax)=child_handle;r(edx)=updated_extra;r(ecx)=vis_mask;
        push(0x1369f8u,r(ecx));
        write(0x1369f9u,dst+0x1b0u,r(edx),4);
        r(edx)=dst+0x194u;
        push(0x136a07u,r(edx));
        at(0x136a08u);r(edx)=word(dst+0x1b4u,4);
        r(ecx)=dst+4u;
        push(0x136a13u,r(ecx));
        at(0x136a14u);r(ecx)=word(dst+0x1b8u,4);
        push(0x136a1bu,r(edx));
        at(0x136a1cu);r(edx)=word(r(esi)+0x2a78u,4);
        push(0x136a22u,r(ecx));
        at(0x136a23u);s_.set(ecx,word(r(ebp)+0x14u,1),1,0);
        push(0x136a26u,r(eax));
        push(0x136a27u,0x136a2cu);
        s_.required(0x136a27u,model_build_entry,false);
        at(0x136a2cu);r(edx)=word(r(ebp)+0x14u,4);
        at(0x136a2fu);r(eax)=word(r(ebp)+0x10u,4);
        push(0x136a32u,r(edx));
        push(0x136a33u,r(eax));
        push(0x136a34u,1u);
        r(ecx)=dst;
        push(0x136a3au,r(ecx));
        r(ecx)=r(esi);
        push(0x136a3du,0x136a42u);
        call_(s_,hierarchy_entry,0x136a3du,before_);
        k.submission=r(ebx);
    }
    void finish(){
        r(edi)=pop(0x136a6eu);
        r(esi)=pop(0x136a6fu);
        r(ebx)=pop(0x136a70u);
        r(esp)=r(ebp);
        r(ebp)=pop(0x136a73u);
        pop(0x136a74u);
        r(esp)+=16u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::model_hierarchy::submit(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_model_hierarchy_detail
