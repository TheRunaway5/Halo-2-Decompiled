#pragma once
#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include "halo2/native_tu15_mopp_ray_projection_kernel.hpp"
#include "halo2/native_tu15_mopp_integer_program_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_scale_kernel.hpp"
#include "halo2/native_tu15_mopp_primitive_key_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_plane_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_branch_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_crossing_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_resume_kernel.hpp"
#include "halo2/native_tu15_mopp_ray_slab_kernel.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
namespace halo2::native_mopp_ray_detail {
// Native numerical operations and the authored-ray DATA controller are shared
// independently of the public API's memory and host-environment ownership.
// Memory/Environment supply actual I/O and genuine child handoffs. This value
// contract does not publish retained caller registers or private stack aliases.
[[noreturn]]inline void missing(NativeTu15MoppRayTraversalStats*stats,std::uint32_t owner){
    if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
    throw std::runtime_error("Native TU1.5 MOPP ray requires native owner="+std::to_string(owner));
}
struct Comparison {
    std::uint16_t status{};
    bool not_less()const{const auto pair=(status>>8u)&5u;return pair==0u||pair==5u;}
    bool less()const{return!not_less();}
    bool greater()const{return(status&0x4100u)==0u;}
};
inline void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
inline void integer(std::int32_t value){asm volatile("fildl %0"::"m"(value):"memory","st");}
inline void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
inline void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
inline void reverse_subtract(std::uint32_t bits){asm volatile("fsubrs %0"::"m"(bits):"memory","st");}
inline void multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
inline void reverse_divide(std::uint32_t bits){asm volatile("fdivrs %0"::"m"(bits):"memory","st");}
inline void subtract_second(){asm volatile(".intel_syntax noprefix; fsub st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
inline void reverse_subtract_second(){asm volatile(".intel_syntax noprefix; fsubr st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
inline void multiply_second(){asm volatile(".intel_syntax noprefix; fmul st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
inline void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
inline std::uint32_t store(bool pop=true){std::uint32_t bits;if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory","st");return bits;}
inline Comparison compare(std::uint32_t bits,bool pop=true){
    std::uint16_t status;if(pop)asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");else asm volatile("fcoms %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");return{status};
}
inline Comparison compare_second(bool third=false){
    std::uint16_t status;if(third)asm volatile(".intel_syntax noprefix; fcomp st(2); .att_syntax prefix; fnstsw %0":"=a"(status)::"memory","st","st(1)","st(2)");else asm volatile(".intel_syntax noprefix; fcomp st(1); .att_syntax prefix; fnstsw %0":"=a"(status)::"memory","st","st(1)");return{status};
}
struct ProjectionArithmetic {
    void begin(std::uint32_t value)const{push(value);}
    void add(std::uint32_t value)const{native_mopp_ray_detail::add(value);}
    void subtract(std::uint32_t value)const{native_mopp_ray_detail::subtract(value);}
};
using Vec=std::array<std::uint32_t,4>;
inline __m128 packed(Vec value){__m128 out;std::memcpy(&out,value.data(),16);return out;}
inline Vec words(__m128 value){Vec out;std::memcpy(out.data(),&value,16);return out;}
inline __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
inline __m128 broadcast(std::uint32_t value){Vec v{value,value,value,value};return packed(v);}
inline Vec interpolate(Vec start,Vec end,std::uint32_t fraction,Vec one){
    const auto weight=broadcast(fraction);auto right=multiply(weight,packed(end));
    auto left=subtract(packed(one),weight);left=multiply(left,packed(start));return words(add(left,right));
}
template<class Memory>
Vec interpolate(Memory&m,Vec start,Vec end,std::uint32_t fraction){
    const auto weight=broadcast(fraction);auto right=multiply(weight,packed(end));
    auto left=subtract(packed(m.vector(0x41c500u)),weight);left=multiply(left,packed(start));return words(add(left,right));
}
template<class Memory,class Environment>
class Traversal {
    Memory&m_;Environment&environment_;NativeTu15PhysicsFrameFp&fp_;
    const NativeTu15MoppRayTraversalServices&services_;NativeTu15MoppRayTraversalStats*stats_;
    std::uint32_t cache_;
    std::uint32_t byte(std::uint32_t p){return m_.word(p,1);}
    template<class Variables>struct IntegerFields {
        Traversal&walk;Variables&variables;
        using Progress=native_mopp_integer_program_detail::Progress;
        std::uint32_t encoded_byte(const Progress&d){return walk.byte(d.program+d.index+1u);}
        void decoded(const Progress&){}
        void ensure_variables(const Progress&){}
        std::uint32_t key_base(const Progress&){return variables.fields10_1c[2];}
        void store_key_base(const Progress&,std::uint32_t value){variables.fields10_1c[2]=value;}
        void store_property(const Progress&,unsigned slot,std::uint32_t value){variables.property(slot,value);}
        void finish_property(const Progress&){}
        void advance(const Progress&,std::uint32_t){}
    };
    struct PrimitiveKeyFields {
        Traversal&walk;
        std::uint32_t encoded_byte(const native_mopp_primitive_key_detail::Progress&d){
            return walk.byte(d.program+d.index+1u);
        }
        void decoded(const native_mopp_primitive_key_detail::Progress&){}
    };
    template<class Bounds>struct PlaneFields {
        Traversal&walk;Bounds&bounds;
        std::uint32_t scratch{},left{};
        using Read=native_mopp_ray_plane_detail::Read;
        using Write=native_mopp_ray_plane_detail::Write;
        using Progress=native_mopp_ray_plane_detail::Progress;
        std::uint32_t scalar(Read kind,const Progress&d){
            switch(kind){
            case Read::low:return walk.byte(d.program+1u);
            case Read::high:return walk.byte(d.program+2u);
            case Read::left_low:return walk.byte(d.program+4u);
            case Read::right_low:return walk.byte(d.program+6u);
            case Read::left_high:return walk.byte(d.program+3u);
            case Read::right_high:return walk.byte(d.program+5u);
            case Read::right_short:return walk.byte(d.program+(d.family==native_mopp_ray_plane_detail::Family::thin_axis?2u:3u));
            case Read::left_offset:return left;
            case Read::thin_delta:return walk.m_.word(0x46a0d0u);
            default:return scratch;
            }
        }
        void store(Write kind,std::uint32_t value,const Progress&){
            if(kind==Write::scratch_low||kind==Write::scratch_high||kind==Write::right_scratch)scratch=value;
            else if(kind==Write::left_offset)left=value;
        }
        std::uint32_t spill(Write,bool pop,const Progress&){return native_mopp_ray_detail::store(pop);}
        std::uint32_t endpoint(unsigned axis,unsigned,const Progress&d){return d.endpoint?bounds.end[axis]:bounds.start[axis];}
        void integer(std::uint32_t value){native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));}
        void floating(std::uint32_t value){push(value);}
        void add_scalar(std::uint32_t value){native_mopp_ray_detail::add(value);}
        void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
        void advance(std::uint32_t,const Progress&){}
    };
    struct BranchFields {
        std::uint32_t low,high,left,right,end;
        using Position=native_mopp_ray_branch_detail::Comparison;
        std::uint32_t end_projection(bool)const{return end;}
        native_mopp_ray_detail::Comparison compare(Position position){
            return native_mopp_ray_detail::compare(position==Position::end_high||position==Position::start_high?high:low,
                position==Position::end_high||position==Position::end_low);
        }
        void publish_comparison(Position,native_mopp_ray_detail::Comparison){}
        std::uint32_t left_offset()const{return left;}
        std::uint32_t right_offset()const{return right;}
        void advance_left(std::uint32_t){}
        void advance_right(std::uint32_t){}
        void floating(std::uint32_t value){push(value);}
        void discard(){native_mopp_ray_detail::discard();}
    };
    template<class Bounds>struct CrossingFields {
        Traversal&walk;Bounds&bounds;Bounds&child;
        std::uint32_t low,high,end,start_low{},start_high{},end_low{},end_high{},fraction{};
        using Scalar=native_mopp_ray_crossing_detail::Scalar;
        using Vectors=native_mopp_ray_crossing_detail::Vectors;
        using Store=native_mopp_ray_crossing_detail::Store;
        using Progress=native_mopp_ray_crossing_detail::Progress;
        std::uint32_t scalar(Scalar kind,const Progress&d){
            switch(kind){
            case Scalar::low_initial:case Scalar::low_end:return low;
            case Scalar::high_start:case Scalar::high_end:return high;
            case Scalar::end_for_low:case Scalar::end_for_high:return end;
            case Scalar::start_low:return start_low;
            case Scalar::direction_end_low:return end_low;
            case Scalar::product_end:case Scalar::fraction_end:return d.increasing?end_low:end_high;
            case Scalar::product_start:case Scalar::fraction_start:case Scalar::fraction_numerator:return d.increasing?start_low:start_high;
            case Scalar::epsilon:return walk.m_.word(0x46a0e8u);
            case Scalar::fraction_bits:return fraction;
            }
            throw std::logic_error("Unknown crossing scalar");
        }
        Vec vector(Vectors kind,const Progress&){
            if(kind==Vectors::one)return walk.m_.vector(0x41c500u);
            return kind==Vectors::start?bounds.start:bounds.end;
        }
        void store_vector(Store kind,Vec value,const Progress&){
            if(kind==Store::child_start)child.start=value;else child.end=value;
        }
        void spill(Store kind,const Progress&d){
            const auto value=native_mopp_ray_detail::store();
            switch(kind){
            case Store::start_low:start_low=value;break;case Store::start_high:start_high=value;break;
            case Store::end_low:end_low=value;break;case Store::end_high:end_high=value;break;
            case Store::fraction:fraction=value;if(!d.increasing)start_high=value;break;
            default:throw std::logic_error("Unknown crossing destination");
            }
        }
        void floating(std::uint32_t value){push(value);}
        void reverse_subtract_second(){native_mopp_ray_detail::reverse_subtract_second();}
        void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
        void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
        void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
        auto compare(std::uint32_t value){return native_mopp_ray_detail::compare(value);}
        void publish_comparison(native_mopp_ray_crossing_detail::Comparison,native_mopp_ray_detail::Comparison){}
        Vec broadcast(std::uint32_t value){return words(native_mopp_ray_detail::broadcast(value));}
        Vec multiply(Vec left,Vec right,native_mopp_ray_crossing_detail::Result){return words(native_mopp_ray_detail::multiply(packed(left),packed(right)));}
        Vec subtract(Vec left,Vec right,native_mopp_ray_crossing_detail::Result){return words(native_mopp_ray_detail::subtract(packed(left),packed(right)));}
        Vec add(Vec left,Vec right,native_mopp_ray_crossing_detail::Result){return words(native_mopp_ray_detail::add(packed(left),packed(right)));}
    };
    template<class Variables,class Bounds>struct ResumeFields {
        Traversal&walk;Variables&variables;Bounds&bounds;std::uint32_t&program;
        std::uint32_t axis_value,low,high,left_offset,right_offset,start_low,start_high,end_low,end_high;
        std::uint32_t fraction{},scale{},code_address{};
        using Scalar=native_mopp_ray_resume_detail::Scalar;using Vectors=native_mopp_ray_resume_detail::Vectors;
        using Store=native_mopp_ray_resume_detail::Store;using Progress=native_mopp_ray_resume_detail::Progress;
        std::uint32_t scalar(Scalar kind,const Progress&d){
            switch(kind){
            case Scalar::product_end:case Scalar::fraction_end:return d.increasing?end_high:end_low;
            case Scalar::product_start:case Scalar::fraction_start:case Scalar::fraction_numerator:return d.increasing?start_high:start_low;
            case Scalar::epsilon:return walk.m_.word(0x46a0e8u);
            case Scalar::interpolation_fraction:case Scalar::saved_fraction:return fraction;
            case Scalar::current_fraction:case Scalar::shortening_fraction:return walk.m_.word(walk.cache_+0x54u);
            case Scalar::unit_fraction:return walk.m_.word(0x46a0d0u);
            case Scalar::scale:return variables.fields10_1c[1];case Scalar::saved_scale:return scale;
            case Scalar::axis_endpoint:return bounds.end[axis_value];case Scalar::axis_plane:return d.increasing?high:low;
            }
            throw std::logic_error("Unknown resumed ray scalar");
        }
        Vec vector(Vectors kind,const Progress&){
            switch(kind){
            case Vectors::bounds_start:return bounds.start;
            case Vectors::bounds_end:case Vectors::scaled_end:case Vectors::translated_end:return bounds.end;
            case Vectors::interpolation_one:case Vectors::shortening_one:return walk.m_.vector(0x41c500u);
            case Vectors::original_end:return walk.m_.vector(walk.cache_+0x30u);
            case Vectors::original_start:return walk.m_.vector(walk.cache_+0x20u);
            case Vectors::code_info:return walk.m_.vector(code_address);
            case Vectors::translation:return variables.translation;
            }
            throw std::logic_error("Unknown resumed ray vector");
        }
        void cache(const Progress&){}
        void code(const Progress&){code_address=walk.m_.word(walk.cache_+0x10u);}
        std::uint32_t axis(const Progress&)const{return axis_value;}
        void store_scalar(Store kind,std::uint32_t value,const Progress&){if(kind==Store::saved_scale)scale=value;else fraction=value;}
        void spill(Store,const Progress&){fraction=native_mopp_ray_detail::store();}
        void store_vector(Store kind,Vec value,const Progress&){if(kind==Store::bounds_start)bounds.start=value;else bounds.end=value;}
        void floating(std::uint32_t value){push(value);}
        void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
        void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
        void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
        auto compare(std::uint32_t value){return native_mopp_ray_detail::compare(value);}
        void publish_comparison(native_mopp_ray_resume_detail::Comparison,native_mopp_ray_detail::Comparison,const Progress&){}
        Vec broadcast(std::uint32_t value,native_mopp_ray_resume_detail::Arithmetic,const Progress&){return words(native_mopp_ray_detail::broadcast(value));}
        Vec multiply(Vec left,Vec right,native_mopp_ray_resume_detail::Arithmetic,const Progress&){return words(native_mopp_ray_detail::multiply(packed(left),packed(right)));}
        Vec subtract(Vec left,Vec right,native_mopp_ray_resume_detail::Arithmetic,const Progress&){return words(native_mopp_ray_detail::subtract(packed(left),packed(right)));}
        Vec add(Vec left,Vec right,native_mopp_ray_resume_detail::Arithmetic,const Progress&){return words(native_mopp_ray_detail::add(packed(left),packed(right)));}
        void advance(const Progress&d){if(!d.increasing)program+=left_offset-right_offset;}
    };
    template<class Variables,class Bounds>struct SlabFields {
        Traversal&walk;Variables&variables;Bounds&bounds;
        std::uint32_t scratch{},start{},end{};
        using Read=native_mopp_ray_slab_detail::Read;using Write=native_mopp_ray_slab_detail::Write;
        using Progress=native_mopp_ray_slab_detail::Progress;using Vectors=native_mopp_ray_slab_detail::VectorRead;
        std::uint32_t scalar(Read kind,const Progress&d){
            switch(kind){
            case Read::low_high:return walk.byte(d.program+1u);case Read::low_middle:return walk.byte(d.program+2u);
            case Read::low_low:return walk.byte(d.program+3u);case Read::high_middle:return walk.byte(d.program+5u);
            case Read::high_high:return walk.byte(d.program+4u);case Read::high_low:return walk.byte(d.program+6u);
            case Read::cache_low:case Read::cache_high:return walk.cache_;
            case Read::cache_scale_low:case Read::cache_scale_high:return walk.m_.word(d.cache+0x14u);
            case Read::variable_scale_low:case Read::variable_scale_high:return variables.fields10_1c[1];
            case Read::translation_low:case Read::translation_high:return variables.translation[d.axis];
            case Read::start:return bounds.start[d.axis];case Read::end:return bounds.end[d.axis];
            case Read::direction_start:case Read::clip_start:return start;
            case Read::direction_end:case Read::clip_end:return end;
            case Read::lower_gate:return d.reverse?start:end;case Read::upper_gate:return d.reverse?end:start;
            case Read::epsilon:return walk.m_.word(0x46a0e8u);
            default:return scratch;
            }
        }
        void store(Write,std::uint32_t value,const Progress&){scratch=value;}
        void spill(Write kind,bool pop,const Progress&){
            const auto value=native_mopp_ray_detail::store(pop);
            if(kind==Write::start)start=value;else if(kind==Write::end)end=value;else scratch=value;
        }
        Vec vector(Vectors kind,const Progress&){if(kind==Vectors::one)return walk.m_.vector(0x41c500u);return kind==Vectors::start?bounds.start:bounds.end;}
        void store_vector(Write,Vec value,const Progress&d){if(d.reverse!=bool(d.clipping))bounds.start=value;else bounds.end=value;}
        void axis(const Progress&){}void advance(std::uint32_t,const Progress&){}void direction(const Progress&){}
        void prepare_ratio(const Progress&){}void ratio_target(const Progress&){}
        void floating(std::uint32_t value){push(value);}
        void integer(std::uint32_t value){native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));}
        void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
        void subtract_scalar(std::uint32_t value){native_mopp_ray_detail::subtract(value);}
        void subtract_second(){native_mopp_ray_detail::subtract_second();}
        void multiply_second(){native_mopp_ray_detail::multiply_second();}
        void reverse_subtract(std::uint32_t value){native_mopp_ray_detail::reverse_subtract(value);}
        void reverse_divide(std::uint32_t value){native_mopp_ray_detail::reverse_divide(value);}
        auto compare_scalar(std::uint32_t value,native_mopp_ray_slab_detail::Comparison,const Progress&){return native_mopp_ray_detail::compare(value);}
        auto compare_stack(bool third,native_mopp_ray_slab_detail::Comparison,const Progress&){return native_mopp_ray_detail::compare_second(third);}
        void discard(){native_mopp_ray_detail::discard();}
        Vec broadcast(std::uint32_t value){return words(native_mopp_ray_detail::broadcast(value));}
        Vec subtract(Vec left,Vec right,native_mopp_ray_slab_detail::VectorMath,const Progress&){return words(native_mopp_ray_detail::subtract(packed(left),packed(right)));}
        Vec multiply(Vec left,Vec right,native_mopp_ray_slab_detail::VectorMath,const Progress&){return words(native_mopp_ray_detail::multiply(packed(left),packed(right)));}
        Vec add(Vec left,Vec right,native_mopp_ray_slab_detail::VectorMath,const Progress&){return words(native_mopp_ray_detail::add(packed(left),packed(right)));}
    };
    template<class Variables,class Bounds>struct ScaleFields {
        Traversal&walk;Variables&variables;Bounds&bounds;
        std::uint32_t scratch{};Vec translation{};
        using Read=native_mopp_ray_scale_detail::Read;using Write=native_mopp_ray_scale_detail::Write;
        using Progress=native_mopp_ray_scale_detail::Progress;
        std::uint32_t scalar(Read kind,const Progress&d){
            switch(kind){
            case Read::x:return walk.byte(d.program+1u);case Read::y:return walk.byte(d.program+2u);case Read::z:return walk.byte(d.program+3u);
            case Read::variable_shift:return variables.fields10_1c[0];case Read::variable_scale:return variables.fields10_1c[1];
            case Read::variable_property:return variables.fields10_1c[3];case Read::variable_key:return variables.fields10_1c[2];
            default:return scratch;
            }
        }
        Vec vector(Read kind,const Progress&){
            switch(kind){
            case Read::start_initial:case Read::start_scaled:return bounds.start;
            case Read::end_initial:case Read::end_scaled:return bounds.end;
            case Read::variable_translation:return variables.translation;
            default:return translation;
            }
        }
        void store(Write kind,std::uint32_t value,const Progress&){
            switch(kind){
            case Write::translation_x:translation[0]=value;break;case Write::translation_y:translation[1]=value;break;
            case Write::translation_z:translation[2]=value;break;case Write::translation_w:translation[3]=value;break;
            case Write::variable_shift:variables.fields10_1c[0]=value;break;case Write::variable_scale:variables.fields10_1c[1]=value;break;
            case Write::variable_property:variables.fields10_1c[3]=value;break;case Write::variable_key:variables.fields10_1c[2]=value;break;
            default:scratch=value;break;
            }
        }
        void store_vector(Write kind,Vec value,const Progress&){
            switch(kind){
            case Write::start_translated:case Write::start_scaled:bounds.start=value;break;
            case Write::end_translated:case Write::end_scaled:bounds.end=value;break;
            default:variables.translation=value;break;
            }
        }
        void integer(std::uint32_t value){native_mopp_ray_detail::integer(std::bit_cast<std::int32_t>(value));}
        void multiply_scalar(std::uint32_t value){native_mopp_ray_detail::multiply(value);}
        void spill(Write kind,bool pop,const Progress&d){store(kind,native_mopp_ray_detail::store(pop),d);}
        Vec subtract(Vec left,Vec right){return words(native_mopp_ray_detail::subtract(packed(left),packed(right)));}
        Vec add(Vec left,Vec right){return words(native_mopp_ray_detail::add(packed(left),packed(right)));}
        Vec multiply(Vec left,Vec right){return words(native_mopp_ray_detail::multiply(packed(left),packed(right)));}
        void advance(std::uint32_t,const Progress&){}
        void finish(const Progress&){}
    };
    template<class Variables>
    void leaf(std::uint32_t key,const Variables&variables){
        if(stats_)++stats_->leaves;key+=variables.fields10_1c[2];
        const auto filter=m_.word(cache_+0x44u);
        if(filter){
            const auto collection=m_.word(cache_+0x64u),table=m_.word(filter),owner=m_.word(table);
            if(!services_.filter)missing(stats_,owner);if(stats_)++stats_->filters;
            if(!environment_.dependency([&]{return services_.filter(filter,owner,cache_+0x20u,collection,key,fp_);}))return;
        }
        const auto collection=m_.word(cache_+0x64u),table=m_.word(collection),owner=m_.word(table+0x34u);
        if(!services_.child_shape)missing(stats_,owner);if(stats_)++stats_->child_shapes;
        const auto shape=environment_.dependency([&]{return services_.child_shape(collection,owner,key,fp_);});
        const auto output=m_.word(cache_+0x58u),shape_table=m_.word(shape.identity);
        if(output){
            const auto cast_owner=m_.word(shape_table+0x20u);if(!services_.cast_direct)missing(stats_,cast_owner);if(stats_)++stats_->direct_casts;
            if(!environment_.dependency([&]{return services_.cast_direct(shape,cast_owner,cache_+0x20u,output,fp_);}))return;
            const auto current_output=m_.word(cache_+0x58u);
            m_.word(0x30b65bu,cache_+0x50u,1u,1);const auto fraction=m_.word(current_output+0x14u);
            m_.word(0x30b662u,cache_+0x54u,fraction);m_.word(0x30b665u,current_output+0x10u,key);return;
        }
        const auto parent=m_.word(cache_+0x60u),parent_field8=m_.word(parent+8u),collector=m_.word(cache_+0x5cu),cast_owner=m_.word(shape_table+0x24u);
        if(!services_.cast_collector)missing(stats_,cast_owner);if(stats_)++stats_->collector_casts;
        environment_.dependency([&]{services_.cast_collector(shape,cast_owner,cache_+0x20u,{shape.identity,key,parent_field8,parent},collector,fp_);});
        const auto current_collector=m_.word(cache_+0x5cu),fraction=m_.word(current_collector+4u);m_.word(0x30b69fu,cache_+0x54u,fraction);
    }
    // Entry has the first endpoint projection on the native x87 stack.
    // A true result is the source's early return after a closer collector hit.
    template<class Variables,class Bounds>
    bool split(std::uint32_t&code,unsigned axis,std::uint32_t low,std::uint32_t high,
        std::uint32_t left,std::uint32_t right,Variables&v,
        Bounds&bounds,unsigned depth,std::uint32_t end){
        BranchFields branch_fields{low,high,left,right,end};
        if(native_mopp_ray_branch_detail::choose(branch_fields,code)==native_mopp_ray_branch_detail::Decision::continue_program)return false;
        auto child=bounds;
        CrossingFields<Bounds>crossing_fields{*this,bounds,child,low,high,end};
        native_mopp_ray_crossing_detail::Progress crossing;
        native_mopp_ray_crossing_detail::prepare_child(crossing_fields,crossing);
        if(stats_){++stats_->branches;++stats_->recursive_calls;}
        run(crossing.increasing?code+left-right:code,v,child,depth+1u);
        ResumeFields<Variables,Bounds>resume_fields{*this,v,bounds,code,axis,low,high,left,right,
            crossing_fields.start_low,crossing_fields.start_high,crossing_fields.end_low,crossing_fields.end_high};
        native_mopp_ray_resume_detail::Progress resumed;
        return native_mopp_ray_resume_detail::resume(resume_fields,crossing.increasing,resumed);
    }
public:
    Traversal(Memory&m,Environment&e,NativeTu15PhysicsFrameFp&fp,std::uint32_t cache,
        const NativeTu15MoppRayTraversalServices&s,NativeTu15MoppRayTraversalStats*stats):m_(m),environment_(e),fp_(fp),services_(s),stats_(stats),cache_(cache){}
    // Views own their copy lifetime and field access. The public owner supplies
    // ordinary values; a retained owner must separately prove lazy field reads,
    // actual property storage, private copies and recursive alias lifetimes.
    template<class Variables,class Bounds>
    void run(std::uint32_t code,const Variables&input,
        Bounds&bounds,unsigned depth=0){
        if(depth>512u)throw std::runtime_error("Native MOPP ray requires its recursion-capacity owner");
        auto variables=input;
        for(;;){
            const auto opcode=byte(code);if(stats_)++stats_->opcodes;
            if(opcode==0u)return;
            if(opcode>=1u&&opcode<=4u){
                ScaleFields<decltype(variables),Bounds>scale_fields{*this,variables,bounds};
                native_mopp_ray_scale_detail::scale(scale_fields,code,opcode);continue;
            }
            IntegerFields<decltype(variables)>integer_fields{*this,variables};
            if(native_mopp_integer_program_detail::step(integer_fields,code,opcode))continue;
            if(opcode>=0x10u&&opcode<=0x25u){
                PlaneFields<Bounds>plane_fields{*this,bounds};native_mopp_ray_plane_detail::Progress plane;
                if(!native_mopp_ray_plane_detail::prepare(plane_fields,code,opcode,plane)){
                    if(stats_)++stats_->invalid_opcodes;
                    throw std::runtime_error("Native MOPP ray encountered invalid geometry opcode");
                }
                if(split(code,plane.axis,plane.low_bits,plane.high_bits,plane.left,plane.right,variables,bounds,depth,plane.end_bits))return;
                continue;
            }
            if(opcode>=0x26u&&opcode<=0x2bu){
                SlabFields<decltype(variables),Bounds>slab_fields{*this,variables,bounds};native_mopp_ray_slab_detail::Progress clipped;
                if(native_mopp_ray_slab_detail::clip(slab_fields,code,opcode,clipped)==native_mopp_ray_slab_detail::Decision::rejected)return;
                continue;
            }
            if(opcode>=0x30u&&opcode<=0x53u){
                PrimitiveKeyFields key_fields{*this};std::uint32_t key{};
                native_mopp_primitive_key_detail::decode(key_fields,code,opcode,key);
                leaf(key,variables);return;
            }
            if(stats_)++stats_->invalid_opcodes;throw std::runtime_error("Native MOPP ray requires its invalid-program diagnostic owner");
        }
    }
};
} // namespace halo2::native_mopp_ray_detail
#endif
