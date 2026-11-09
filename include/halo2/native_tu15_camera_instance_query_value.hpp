#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_camera_instance_query_kernel.hpp"
#include "halo2/native_tu15_camera_instanced_query.hpp"
#include "halo2/native_tu15_camera_query_leaves.hpp"
#include "halo2/native_tu15_camera_dynamic_query_leaves.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include "halo2/native_tu15_bsp_segment_query.hpp"
#include <array>
#include <optional>
#include <xmmintrin.h>

namespace halo2::native_camera_instance_query_value_detail {
using namespace native_camera_instance_query_detail;
template<class Memory,class Environment>class View {
    SourcePhysicsContactMemory&memory_;Memory&m_;const NativeTu15CameraInstanceQuery&q_;
    NativeTu15CameraInstanceQueryScratch scratch_;NativeTu15PhysicsFrameFp&fp_;unsigned depth_;
    const SourcePhysicsContactRecordObserver&observe_;NativeTu15CameraInstanceQueryStats*stats_;
    std::optional<Environment>environment_;
    static __m128 packed(std::uint32_t value){return std::bit_cast<__m128>(std::array<std::uint32_t,4>{value,0,0,0});}
    static std::uint32_t bits(__m128 value){return std::bit_cast<std::array<std::uint32_t,4>>(value)[0];}
public:
    using Scalar=__m128;using Data=Progress<Scalar>;
    View(SourcePhysicsContactMemory&memory,Memory&m,const NativeTu15CameraInstanceQuery&q,
        NativeTu15CameraInstanceQueryScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
        const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraInstanceQueryStats*stats)
        :memory_(memory),m_(m),q_(q),scratch_(scratch),fp_(fp),depth_(depth),observe_(observe),stats_(stats){}
    std::uint32_t begin(const Data&){if(stats_)++stats_->calls;return q_.instance;}
    std::uint32_t word(Read kind,const Data&d){
        switch(kind){
        case Read::scenario:case Read::material_scenario:return m_.word(0x4ecc18u);
        case Read::instances:return m_.word(d.scenario+0x144u);
        case Read::definitions:return m_.word(d.scenario+0x13cu);
        case Read::mask:return q_.mask;
        case Read::definition_index:return m_.word(d.instance+0x34u,2);
        case Read::fraction:return m_.word(q_.result+4u);
        case Read::material:return m_.word(scratch_.collision+22u,2);
        case Read::material_table:return m_.word(d.scenario+0x10u);
        case Read::selected_material:return m_.word(d.material_pointer,2);
        default:throw std::runtime_error("Native instance query requires a scalar DATA field");
        }
    }
    Scalar scalar(Read kind,const Data&){
        std::uint32_t address{};
        switch(kind){
        case Read::inverse_scale:address=scratch_.inverse;break;
        case Read::origin_one:case Read::displacement_one:address=0x46a0d0u;break;
        case Read::origin_x:address=q_.origin;break;
        case Read::origin_y:address=q_.origin+4u;break;
        case Read::origin_z:address=q_.origin+8u;break;
        case Read::displacement_x:address=q_.displacement;break;
        case Read::displacement_y:address=q_.displacement+4u;break;
        case Read::displacement_z:address=q_.displacement+8u;break;
        case Read::matrix_28:address=scratch_.inverse+28u;break;
        case Read::matrix_1c:case Read::delta_matrix_1c:address=scratch_.inverse+16u;break;
        case Read::matrix_20:case Read::delta_matrix_20:address=scratch_.inverse+20u;break;
        case Read::matrix_10:case Read::delta_matrix_10:address=scratch_.inverse+4u;break;
        case Read::matrix_34:address=scratch_.inverse+40u;break;
        case Read::matrix_2c:address=scratch_.inverse+32u;break;
        case Read::matrix_14:case Read::delta_matrix_14:address=scratch_.inverse+8u;break;
        case Read::matrix_38:address=scratch_.inverse+44u;break;
        case Read::matrix_30:address=scratch_.inverse+36u;break;
        case Read::matrix_24:case Read::delta_matrix_24:address=scratch_.inverse+24u;break;
        case Read::matrix_18:case Read::delta_matrix_18:address=scratch_.inverse+12u;break;
        case Read::matrix_3c:address=scratch_.inverse+48u;break;
        default:throw std::runtime_error("Native instance query requires a word DATA field");
        }
        return packed(m_.word(address));
    }
    Scalar multiply(Scalar left,Scalar right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
    Scalar add(Scalar left,Scalar right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
    bool different(Scalar left,Scalar right,Comparison,const Data&){unsigned char unequal,unordered;asm volatile("ucomiss %3,%2;setne %0;setp %1":"=qm"(unequal),"=qm"(unordered):"x"(left),"x"(right):"cc","memory");return unequal||unordered;}
    void phase(Phase,const Data&){}
    void prepare_bitmap(const Data&){}
    void resume(Child,Data&){}
    bool admit(const Data&d){if(stats_)++stats_->admissions;return native_tu15_admit_camera_query_instance(memory_,d.index,d.definition,d.mask);}
    bool near_sphere(const Data&d){if(stats_)++stats_->sphere_calls;return native_tu15_segment_near_sphere(memory_,d.instance+0x3cu,q_.origin,q_.displacement,m_.word(d.instance+0x48u),fp_);}
    void inverse(const Data&d){if(stats_)++stats_->inverse_calls;native_tu15_camera_inverse_frame(memory_,d.instance,scratch_.inverse,fp_,observe_);environment_.emplace(fp_,depth_);}
    std::uint32_t bitmap(const Data&d){return native_tu15_camera_instance_collision_bitmap(memory_,d.index);}
    bool bsp(const Data&d){if(stats_)++stats_->bsp_calls;return environment_->dependency([&]{return native_tu15_query_bsp_segment(memory_,{q_.bsp_filter,d.definition+0x70u,scratch_.collision,d.fraction,8,d.bitmap,scratch_.local_origin,scratch_.local_displacement},scratch_.bsp_context,fp_,depth_,observe_);});}
    std::uint32_t default_material(const Data&){return 0x48a0b0u;}
    void store(Write kind,Scalar value,const Data&){
        std::uint32_t pc{},address{};
        switch(kind){
        case Write::origin_x:pc=0x16c101u;address=scratch_.local_origin;break;
        case Write::origin_y:pc=0x16c12cu;address=scratch_.local_origin+4u;break;
        case Write::origin_z:pc=0x16c173u;address=scratch_.local_origin+8u;break;
        case Write::delta_x:pc=0x16c1a7u;address=scratch_.local_displacement;break;
        case Write::delta_y:pc=0x16c1f9u;address=scratch_.local_displacement+4u;break;
        case Write::delta_z:pc=0x16c1ffu;address=scratch_.local_displacement+8u;break;
        default:throw std::runtime_error("Native instance query requires a word result field");
        }
        m_.word(pc,address,bits(value));
    }
    void store_word(Write kind,std::uint32_t value,const Data&){
        switch(kind){
        case Write::result_kind:m_.word(0x16c234u,q_.result,value);break;
        case Write::material:m_.word(0x16c258u,q_.result+0x24u,value,2);break;
        case Write::object:m_.word(0x16c25cu,q_.result+0x38u,value);break;
        case Write::region:m_.word(0x16c25fu,q_.result+0x40u,value);break;
        case Write::instance:m_.word(0x16c26au,q_.result+0x3cu,value);break;
        default:throw std::runtime_error("Native instance query requires a scalar result field");
        }
    }
    void publish(const Data&d){environment_->dependency([&]{native_tu15_publish_camera_collision(memory_,q_.result,d.instance,scratch_.collision,fp_,observe_);return false;});}
    bool finish(bool found,const Data&){if(stats_){if(found)++stats_->hits;++stats_->completed;}return found;}
};
}
#endif
