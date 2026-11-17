#pragma once
#include "halo2/native_tu15_solver_hull_clip_kernel.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_solver_hull_clip_value_detail {
using namespace joint_constraint_numeric;
class View {
    Environment environment_;
    Memory memory_;
    std::uint32_t facet_,point_,lower_,upper_,projection_{},result_{1u};
public:
    View(SourcePhysicsContactMemory&memory,std::uint32_t facet,std::uint32_t point,
         std::uint32_t lower,std::uint32_t upper,NativeTu15PhysicsFrameFp&fp,unsigned depth,
         const SourcePhysicsContactRecordObserver&observe)
        :environment_{fp,depth},memory_{memory,observe},facet_{facet},point_{point},lower_{lower},upper_{upper}{}
    void project_and_compare_lower(){
        const auto reference=memory_.word(facet_+0x14u);
        const auto origin=memory_.vector(reference);auto difference=memory_.vector(point_);
        asm volatile("subps %1,%0":"+x"(difference):"x"(origin):"memory");
        const auto normal=memory_.vector(facet_);
        projection_=sum_three(multiply(normal,difference));
        push(projection_);const auto initial_lower=memory_.word(lower_);
        asm volatile("fcomps %0"::"m"(initial_lower):"memory","st");
        push(projection_);
    }
    bool use_upper(){
        std::uint16_t relation;
        asm volatile("fnstsw %0":"=a"(relation));
        return (relation&0x4100u)==0;
    }
    bool update_upper(){
        const auto current_upper=memory_.word(upper_);std::uint16_t relation;
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
        return (std::popcount(unsigned((relation>>8u)&5u))&1u)!=0;
    }
    void publish_upper(){memory_.word(0x314836u,upper_,projection_);}
    bool update_lower(){
        add_scalar(memory_.word(0x41d7a0u));const auto current_lower=memory_.word(lower_);
        std::uint16_t relation;
        asm volatile("fcoms %1;fnstsw %0":"=a"(relation):"m"(current_lower):"memory");
        return (relation&0x4100u)==0;
    }
    void publish_lower(){memory_.pop_float(0x314849u,lower_);}
    void discard_lower(){asm volatile("fstp %%st(0)":::"memory","st");}
    void mark_lower(){result_=2u;}
    std::uint32_t finish(){
        push(memory_.word(lower_));const auto current_upper=memory_.word(upper_);std::uint16_t relation;
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
        return relation&0x100u?result_:4u;
    }
};
} // namespace halo2::native_solver_hull_clip_value_detail
#endif
