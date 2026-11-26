#pragma once
// Boundary adapter for the light volume shape builder
// (native_tu15_light_shape_systems.hpp): C3AB0, pure leaf called with the
// LightTransform in ECX, LightVolumeShape in EDX, and LightDefinition on the
// stack (RET 4).
//
// Admission: TU1.5 image, no MMX alias, x87 depth <= 6 (body pushes 2 slots),
// the exact live instruction-cursor contract, and a certified required owner.
// HALO2_REFERENCE_LIGHT_SHAPE=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (C3AB0..C3DB4 holds no service PC).
#include "rendering/native_tu15_light_shape_systems.hpp"
#include "pstencil/native_tu15_light_shape_written.hpp"
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

namespace halo2::native_light_shape_detail {
namespace logic=halo2::native_light_shape;
using logic::Access;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_LIGHT_SHAPE");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if defined(__x86_64__) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>6)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0xc3ab0u,0xc3db4u)){
    }else if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (s.observe&&!c->observation_only_stores&&!pstencil::LightShapeWrittenObserver::qualified(s.observe))||
           (c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return true;
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){return !reference()&&policy_admitted(s);}

#if defined(__x86_64__) && !defined(_MSC_VER)
struct Hardware {std::uint32_t value,flags;};
inline Hardware tag_row(std::uint32_t value,std::uint32_t flags){
    auto ah=(flags&0xd5u)<<8u;unsigned char overflow;
    asm volatile("sahf;andl $0xffff,%1;shll $4,%1;lahf;seto %2":"+a"(ah),"+r"(value),"=qm"(overflow)::"cc");
    return{value,((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u)};
}
inline __m128 addss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 subss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 mulss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 divss(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 cvtsi2ss(__m128 dst,std::int32_t value){
    asm volatile("cvtsi2ss %1,%0":"+x"(dst):"r"(value):"memory");return dst;
}
inline std::uint32_t comiss(__m128 a,__m128 b){
    unsigned char below,equal,unordered;
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(a),"x"(b):"cc","memory");
    return std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u);
}
inline long double x87_load_m32(std::uint32_t bits){
    long double out;asm volatile("flds %1;fstpt %0":"=m"(out):"m"(bits):"memory","st");return out;
}
inline long double x87_fabs(long double value){
    long double out;asm volatile("fldt %1;fabs;fstpt %0":"=m"(out):"m"(value):"memory","st");return out;
}
inline std::uint32_t x87_fcomip(long double a,long double b){
    unsigned char below,equal,unordered;
    asm volatile("fldt %4;fldt %3;fcomip %%st(1),%%st;setb %0;sete %1;setp %2;fstp %%st(0)"
                 :"=qm"(below),"=qm"(equal),"=qm"(unordered):"m"(a),"m"(b):"cc","memory","st","st(1)");
    return std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u);
}
inline long double x87_mul_m32(long double value,std::uint32_t bits,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fmuls %3;fnstsw %1;fstpt %0":"=m"(out),"=m"(word):"m"(value),"m"(bits):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}
inline long double x87_fptan(long double value,std::uint16_t&status){
    long double out;std::uint16_t word;
    asm volatile("fldt %2;fptan;fnstsw %1;testw $0x400,%1;jnz 1f;fstp %%st(0);1: fstpt %0"
                 :"=m"(out),"=m"(word):"m"(value):"memory","st","st(1)");
    status=word;return out;
}
inline std::uint32_t x87_spill_m32(long double value,std::uint16_t&status){
    std::uint32_t out;std::uint16_t word;
    asm volatile("fldt %2;fstps %0;fnstsw %1":"=m"(out),"=m"(word):"m"(value):"memory","st");
    status=std::uint16_t((status&~0x200u)|(word&0x200u));return out;
}

template<class State,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
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
        case sphere_ra:return 0xc3abfu;
        case sphere_scale_a:return 0xc3ac4u;
        case store_sphere_a:return 0xc3ac9u;
        case sphere_rb:return 0xc3aceu;
        case sphere_scale_b:return 0xc3ad3u;
        case reload_sphere_a:return 0xc3ad8u;
        case store_sphere_b:return 0xc3ae0u;
        case store_sphere_max:return 0xc3aeau;
        case target_x:return 0xc3af4u;
        case origin_x:return 0xc3af9u;
        case store_dir_x:r(eax)=r(edx)+0x38u;return 0xc3b00u;
        case target_y:return 0xc3b05u;
        case origin_y:return 0xc3b0au;
        case store_dir_y:r(esi)=r(eax)+4u;return 0xc3b12u;
        case target_z:return 0xc3b17u;
        case origin_z:return 0xc3b1cu;
        case store_dir_z:return 0xc3b21u;
        case fwd_z:return 0xc3b26u;
        case dot_dir_z:return 0xc3b2bu;
        case fwd_y:return 0xc3b30u;
        case dot_dir_y:return 0xc3b35u;
        case fwd_x:s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);return 0xc3b3eu;
        case dot_dir_x:return 0xc3b43u;
        case one_constant:s_.xmm[0]=addss(s_.xmm[0],s_.xmm[1]);return 0xc3b4bu;
        case spill_proj:return 0xc3b53u;
        case store_length:return 0xc3b5du;
        case fallback_fwd_x:return 0xc3b75u;
        case store_fb_x:r(edi)=r(esi);return 0xc3b7au;
        case fallback_fwd_y:return 0xc3b7cu;
        case store_fb_y:return 0xc3b7fu;
        case fallback_fwd_z:return 0xc3b82u;
        case store_fb_z:return 0xc3b85u;
        case store_fb_len:return 0xc3b88u;
        case reload_length:return 0xc3b8du;
        case def_arg:return 0xc3b92u;
        case norm_dir_x:s_.xmm[0]=s_.xmm[1];return 0xc3b99u;
        case store_norm_x:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3ba1u;
        case norm_dir_y:return 0xc3ba5u;
        case store_norm_y:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3baeu;
        case norm_dir_z:return 0xc3bb3u;
        case store_norm_z:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3bbcu;
        case shape_aspect:return 0xc3bc1u;
        case store_aspect:return 0xc3bc6u;
        case gel_bitmap:return 0xc3bcbu;
        case tag_instances:return 0xc3bd3u;
        case gel_tag_def:{
            const auto row=tag_row(r(edi),f.bits());r(edi)=row.value;f.load(row.flags);
            return 0xc3be2u;
        }
        case gel_bitmap_data:return 0xc3be6u;
        case gel_height:return 0xc3be9u;
        case gel_width:return 0xc3bedu;
        case store_gel_aspect:
            s_.xmm[1]=cvtsi2ss(s_.xmm[1],std::int32_t(r(ebx)));
            s_.xmm[3]=cvtsi2ss(s_.xmm[3],std::int32_t(r(edi)));
            s_.xmm[1]=divss(s_.xmm[1],s_.xmm[3]);
            s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);
            return 0xc3c01u;
        case kind_parallel:return 0xc3c06u;
        case parallel_ptr:return 0xc3c0bu;
        case parallel_tan_x:return 0xc3c11u;
        case store_par_tan_x:return 0xc3c13u;
        case parallel_tan_y:return 0xc3c16u;
        case store_par_tan_y:return 0xc3c19u;
        case near_width_bits:return 0xc3c34u;
        case reload_aspect_near:return 0xc3c37u;
        case store_near_x:return 0xc3c3cu;
        case near_width_mul:return 0xc3c3fu;
        case store_near_y:return 0xc3c44u;
        case far_dist:return 0xc3c49u;
        case scale_far:return 0xc3c4eu;
        case store_far_clip:return 0xc3c53u;
        case near_dist:return 0xc3c58u;
        case scale_near:return 0xc3c5du;
        case store_near_clip:return 0xc3c62u;
        case reload_far_clip:return 0xc3c67u;
        case reload_tan_x:return 0xc3c6cu;
        case reload_near_x:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3c75u;
        case store_far_x:return 0xc3c7au;
        case reload_tan_y:return 0xc3c7fu;
        case reload_near_y:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3c88u;
        case store_far_y:return 0xc3c8du;
        case reload_far_clip2:return 0xc3c92u;
        case far_dir_x:return 0xc3c97u;
        case far_origin_x:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3c9fu;
        case store_far_cx:return 0xc3ca3u;
        case far_dir_y:return 0xc3ca8u;
        case far_origin_y:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3cb1u;
        case store_far_cy:return 0xc3cb6u;
        case far_dir_z:return 0xc3cbbu;
        case far_origin_z:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3cc4u;
        case store_far_cz:return 0xc3cc9u;
        case cross_up_y:return 0xc3cceu;
        case cross_fwd_y:return 0xc3cd3u;
        case cross_up_x:return 0xc3cd8u;
        case cross_fwd_x:return 0xc3cddu;
        case cross_up_z:return 0xc3ce2u;
        case cross_fwd_x2:return 0xc3ce7u;
        case cross_fwd_z:return 0xc3cecu;
        case cross_up_y2:return 0xc3cf1u;
        case cross_fwd_z2:s_.xmm[0]=subss(s_.xmm[0],s_.xmm[1]);return 0xc3cfau;
        case cross_up_x2:return 0xc3cffu;
        case cross_fwd_y2:s_.xmm[1]=subss(s_.xmm[1],s_.xmm[3]);return 0xc3d08u;
        case cross_up_z2:return 0xc3d0du;
        case store_right_y:return 0xc3d12u;
        case store_right_z:return 0xc3d17u;
        case store_right_x:s_.xmm[3]=subss(s_.xmm[3],s_.xmm[4]);return 0xc3d20u;
        case store_apex_offset_zero:return 0xc3d25u;
        case copy_origin_x:r(esi)=r(edx)+0x2cu;r(edi)=r(ecx);return 0xc3d2fu;
        case store_apex_x:r(ebx)=r(esi);return 0xc3d33u;
        case copy_origin_y:return 0xc3d35u;
        case store_apex_y:return 0xc3d38u;
        case copy_origin_z:return 0xc3d3bu;
        case store_apex_z:return 0xc3d3eu;
        case kind_spot:return 0xc3d41u;
        case spot_tan_x:return 0xc3d46u;
        case spot_epsilon:return 0xc3d4bu;
        case spot_near_width:return 0xc3d54u;
        case spot_div_tan_x:return 0xc3d59u;
        case spot_half:return 0xc3d5eu;
        case store_apex_offset:return 0xc3d66u;
        case spot_dir_x:return 0xc3d6bu;
        case spot_origin_x:
            s_.xmm[2]=subss(s_.xmm[2],s_.xmm[0]);
            s_.xmm[0]=s_.xmm[2];
            s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);
            return 0xc3d7bu;
        case store_spot_ax:return 0xc3d7fu;
        case spot_dir_y:return 0xc3d83u;
        case spot_origin_y:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3d8cu;
        case store_spot_ay:return 0xc3d91u;
        case spot_dir_z:return 0xc3d96u;
        case spot_origin_z:s_.xmm[1]=mulss(s_.xmm[1],s_.xmm[0]);return 0xc3d9fu;
        case store_spot_az:return 0xc3da4u;
        default:return 0;
        }
    }
    void loaded(Access a,std::uint32_t v){
        using enum Access;auto&f=s_.flags;
        switch(a){
        case sphere_ra:s_.xmm[0]=loaded_float(v);break;
        case sphere_scale_a:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case sphere_rb:s_.xmm[0]=loaded_float(v);break;
        case sphere_scale_b:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case reload_sphere_a:s_.xmm[1]=loaded_float(v);break;
        case target_x:s_.xmm[0]=loaded_float(v);break;
        case origin_x:s_.xmm[0]=subss(s_.xmm[0],loaded_float(v));break;
        case target_y:s_.xmm[0]=loaded_float(v);break;
        case origin_y:s_.xmm[0]=subss(s_.xmm[0],loaded_float(v));break;
        case target_z:s_.xmm[0]=loaded_float(v);break;
        case origin_z:s_.xmm[0]=subss(s_.xmm[0],loaded_float(v));break;
        case fwd_z:s_.xmm[0]=loaded_float(v);break;
        case dot_dir_z:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case fwd_y:s_.xmm[1]=loaded_float(v);break;
        case dot_dir_y:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case fwd_x:s_.xmm[1]=loaded_float(v);break;
        case dot_dir_x:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case one_constant:s_.xmm[1]=loaded_float(v);break;
        case fallback_fwd_x:r(ebx)=v;break;
        case fallback_fwd_y:r(ebx)=v;break;
        case fallback_fwd_z:r(ebx)=v;break;
        case reload_length:s_.xmm[1]=divss(s_.xmm[1],loaded_float(v));break;
        case def_arg:r(edi)=v;break;
        case norm_dir_x:s_.xmm[1]=loaded_float(v);break;
        case norm_dir_y:s_.xmm[1]=loaded_float(v);break;
        case norm_dir_z:s_.xmm[1]=loaded_float(v);break;
        case shape_aspect:s_.xmm[0]=loaded_float(v);break;
        case gel_bitmap:r(edi)=v;f.minus(v,0xffffffffu,4);break;
        case tag_instances:r(ebx)=v;break;
        case gel_tag_def:r(edi)=v;break;
        case gel_bitmap_data:r(edi)=v;break;
        case gel_height:r(ebx)=std::uint32_t(logic::widen16(v));break;
        case gel_width:r(edi)=std::uint32_t(logic::widen16(v));break;
        case kind_parallel:f.minus(v,1u,4);break;
        case parallel_ptr:r(edi)=v;break;
        case parallel_tan_x:r(ebx)=v;break;
        case parallel_tan_y:r(edi)=v;break;
        case near_width_bits:r(edi)=v;break;
        case reload_aspect_near:s_.xmm[0]=loaded_float(v);break;
        case near_width_mul:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case far_dist:s_.xmm[0]=loaded_float(v);break;
        case scale_far:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case near_dist:s_.xmm[0]=loaded_float(v);break;
        case scale_near:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case reload_far_clip:s_.xmm[0]=loaded_float(v);break;
        case reload_tan_x:s_.xmm[1]=loaded_float(v);break;
        case reload_near_x:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case reload_tan_y:s_.xmm[1]=loaded_float(v);break;
        case reload_near_y:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case reload_far_clip2:s_.xmm[0]=loaded_float(v);break;
        case far_dir_x:s_.xmm[1]=loaded_float(v);break;
        case far_origin_x:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case far_dir_y:s_.xmm[1]=loaded_float(v);break;
        case far_origin_y:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case far_dir_z:s_.xmm[1]=loaded_float(v);break;
        case far_origin_z:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case cross_up_y:s_.xmm[0]=loaded_float(v);break;
        case cross_fwd_y:s_.xmm[1]=loaded_float(v);break;
        case cross_up_x:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case cross_fwd_x:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case cross_up_z:s_.xmm[3]=loaded_float(v);break;
        case cross_fwd_x2:s_.xmm[3]=mulss(s_.xmm[3],loaded_float(v));break;
        case cross_fwd_z:s_.xmm[4]=loaded_float(v);break;
        case cross_up_y2:s_.xmm[4]=mulss(s_.xmm[4],loaded_float(v));break;
        case cross_fwd_z2:s_.xmm[1]=loaded_float(v);break;
        case cross_up_x2:s_.xmm[1]=mulss(s_.xmm[1],loaded_float(v));break;
        case cross_fwd_y2:s_.xmm[3]=loaded_float(v);break;
        case cross_up_z2:s_.xmm[3]=mulss(s_.xmm[3],loaded_float(v));break;
        case copy_origin_x:r(ebp)=v;break;
        case copy_origin_y:r(ebp)=v;break;
        case copy_origin_z:r(edi)=v;break;
        case kind_spot:f.minus(v,2u,4);break;
        case spot_tan_x:s_.xmm[0]=loaded_float(v);break;
        case spot_epsilon:f.load(comiss(s_.xmm[0],loaded_float(v)));break;
        case spot_near_width:s_.xmm[0]=loaded_float(v);break;
        case spot_div_tan_x:s_.xmm[0]=divss(s_.xmm[0],loaded_float(v));break;
        case spot_half:s_.xmm[0]=mulss(s_.xmm[0],loaded_float(v));break;
        case spot_dir_x:s_.xmm[1]=loaded_float(v);break;
        case spot_origin_x:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case spot_dir_y:s_.xmm[1]=loaded_float(v);break;
        case spot_origin_y:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        case spot_dir_z:s_.xmm[1]=loaded_float(v);break;
        case spot_origin_z:s_.xmm[1]=addss(s_.xmm[1],loaded_float(v));break;
        default:break;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);at(pc);const auto v=word(address,width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Before&before):Base{s,before}{}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}
    float load_float(Access a,std::uint32_t address){return std::bit_cast<float>(load(a,address,4));}
    void store32(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value,4);}
    void store_float(Access a,std::uint32_t address,float value){
        const auto pc=before(a);
        const auto bits=std::bit_cast<std::uint32_t>(value);
        if(a==Access::store_sphere_max&&!s_.flags.cf&&!s_.flags.zf)s_.xmm[0]=s_.xmm[1];
        write(pc,address,bits,4);
    }

    // ---- Scalar SSE helpers (pure value ops for systems layer)
    float mul_scalar(float a,float b)const{return _mm_cvtss_f32(mulss(_mm_set_ss(a),_mm_set_ss(b)));}
    float add_scalar(float a,float b)const{return _mm_cvtss_f32(addss(_mm_set_ss(a),_mm_set_ss(b)));}
    float sub_scalar(float a,float b)const{return _mm_cvtss_f32(subss(_mm_set_ss(a),_mm_set_ss(b)));}
    float div_scalar(float a,float b)const{return _mm_cvtss_f32(divss(_mm_set_ss(a),_mm_set_ss(b)));}
    float int_to_float(std::int32_t v)const{return _mm_cvtss_f32(cvtsi2ss(_mm_setzero_ps(),v));}

    // ---- Frame
    std::uint32_t entry_transform()const{return s_.gp[ecx];}
    std::uint32_t entry_shape()const{return s_.gp[edx];}
    std::uint32_t enter_and_load_kind(std::uint32_t shape){
        push(0xc3ab0u,r(ecx));
        s_.xmm[2]=_mm_setzero_ps();
        push(0xc3ab4u,r(ebx));
        push(0xc3ab5u,r(ebp));
        r(eax)=s_.flags.logic(0u,4);
        at(0xc3ab8u);const auto kind=word(shape,2);
        s_.flags.minus(kind,r(eax)&0xffffu,2);
        push(0xc3abbu,r(esi));
        push(0xc3abcu,r(edi));
        return kind;
    }
    std::uint32_t proj_spill_slot()const{return s_.gp[esp]+0x10u;}
    std::uint32_t arg_definition()const{return s_.gp[esp]+0x18u;}
    bool sphere_a_greater(float,float){
        s_.flags.load(comiss(s_.xmm[1],s_.xmm[0]));
        return !s_.flags.cf&&!s_.flags.zf;
    }
    void x87_load_proj(Access,std::uint32_t address){
        at(0xc3b59u);const auto bits=word(address,4);
        s_.fp.push(x87_load_m32(bits));
        s_.fp.status&=~0x200u;
    }
    bool x87_abs_gt_eps_and_positive(Access,std::uint32_t eps_addr){
        s_.fp.st(0)=x87_fabs(s_.fp.st(0));s_.fp.status&=~0x200u;
        at(0xc3b64u);const auto bits=word(eps_addr,4);
        s_.fp.push(x87_load_m32(bits));s_.fp.status&=~0x200u;
        s_.flags.load(x87_fcomip(s_.fp.st(0),s_.fp.st(1)));s_.fp.pop();
        s_.fp.status&=~0x200u;s_.fp.pop();
        if(!s_.flags.cf&&!s_.flags.zf)return false;
        s_.flags.load(comiss(s_.xmm[2],s_.xmm[0]));
        return s_.flags.cf||s_.flags.zf;
    }
    void x87_compute_tan_xy(std::uint32_t fov_addr,std::uint32_t half_addr,
                            std::uint32_t tan_x_addr,std::uint32_t aspect_addr,std::uint32_t tan_y_addr){
        at(0xc3c1eu);s_.fp.push(x87_load_m32(word(fov_addr,4)));s_.fp.status&=~0x200u;
        at(0xc3c21u);s_.fp.st(0)=x87_mul_m32(s_.fp.st(0),word(half_addr,4),s_.fp.status);
        std::uint16_t status{};
        s_.fp.st(0)=x87_fptan(s_.fp.st(0),status);
        if(!(status&0x400u))s_.fp.push(1.0L);
        s_.fp.status=std::uint16_t((s_.fp.status&~0x600u)|(status&0x600u));
        s_.fp.status&=~0x200u;s_.fp.pop();
        at(0xc3c2bu);
        auto st1=s_.fp.status;
        const auto tx=x87_spill_m32(s_.fp.st(0),st1);
        Base::store(0xc3c2bu,tan_x_addr,tx,[&]{s_.fp.status=st1;},true);
        at(0xc3c2eu);s_.fp.st(0)=x87_mul_m32(s_.fp.st(0),word(aspect_addr,4),s_.fp.status);
        at(0xc3c31u);
        auto st2=s_.fp.status;
        const auto ty=x87_spill_m32(s_.fp.st(0),st2);
        Base::store(0xc3c31u,tan_y_addr,ty,[&]{s_.fp.status=st2;s_.fp.pop();},true);
    }
    bool gt_scalar(float,float)const{return !s_.flags.cf&&!s_.flags.zf;}
    void finish(std::uint32_t shape){
        r(edi)=pop(0xc3da9u);
        r(esi)=pop(0xc3daau);
        r(ebp)=pop(0xc3dabu);
        write(0xc3dacu,shape+0x18u,1u,1);
        r(ebx)=pop(0xc3db0u);
        r(ecx)=pop(0xc3db1u);
        pop(0xc3db2u);
        r(esp)+=4u;
    }
};
template<class State,class Before=Noop>bool bridge(State&s,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,before};logic::light_shape::build(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_light_shape_detail
