#include "halo2/native_tu15_bsp_surface_query.hpp"
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#include "halo2/native_tu15_bsp_surface_polygon_kernel.hpp"
#include "halo2/native_tu15_bsp_leaf_surface_kernel.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using namespace joint_constraint_numeric;
using F=Vector;
F scalar(std::uint32_t bits){F value{};std::memcpy(&value,&bits,4);return value;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Compare {bool below,equal;bool above()const{return!below&&!equal;}};
Compare compare(F a,F b){unsigned char below,equal;asm volatile("comiss %3,%2;setb %0;sete %1":"=qm"(below),"=qm"(equal):"x"(a),"x"(b):"cc","memory");return{below!=0,equal!=0};}
std::uint32_t signed_word(std::uint32_t value){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
struct MemoryPoint {Memory&m;std::uint32_t identity;std::uint32_t word(unsigned lane)const{return m.word(identity+lane*4u);}};
struct HostPoint {const NativeTu15BspProjectedPoint&value;std::uint32_t word(unsigned lane)const{return value[lane];}};
template<class Point>std::uint32_t tree(Memory&m,std::uint32_t descriptor,std::uint32_t node,const Point&point){
    return native_bsp_surface_2d_detail::find(m,descriptor,node,point,[](auto,const auto&){});
}
template<class Point>bool polygon(Memory&m,std::uint32_t descriptor,std::uint32_t bitmap,
    std::uint16_t bitmap_limit,std::uint32_t surface,std::uint16_t axis,bool winding,const Point&point){
    struct Inputs {
        std::uint32_t limit_value,axis_value,winding_value;
        std::uint32_t limit()const{return limit_value;}
        std::uint32_t axis()const{return axis_value;}
        std::uint32_t winding()const{return winding_value;}
        std::uint32_t edges(std::uint32_t value)const{return value;}
        std::uint32_t vertices(std::uint32_t value)const{return value;}
        std::uint32_t surface(std::uint32_t value)const{return value;}
        std::uint32_t first_edge(std::uint32_t value)const{return value;}
    }inputs{bitmap_limit,axis,std::uint32_t(winding)};
    struct ProjectedPoint {
        const Point&point;
        std::uint32_t address()const{return 0;}
        std::uint32_t word(std::uint32_t,unsigned lane)const{return point.word(lane);}
    }projected{point};
    return native_bsp_surface_polygon_detail::contains(m,descriptor,bitmap,surface,inputs,projected,[](auto,const auto&){});
}
std::uint32_t query(Memory&m,const NativeTu15BspSurfaceQuery&q){
    struct Inputs {
        Memory&memory;const NativeTu15BspSurfaceQuery&q;
        std::uint32_t descriptor()const{return q.descriptor;}
        std::uint32_t candidate()const{return q.candidate_leaf;}
        std::uint32_t fraction()const{return q.fraction_bits;}
        std::uint32_t plane_index()const{return q.plane_index;}
        std::uint32_t cursor(std::uint32_t value)const{return value;}
        std::uint32_t abs_x(std::uint32_t value)const{return value;}
        std::uint32_t abs_y(std::uint32_t value)const{return value;}
        std::uint32_t abs_z(std::uint32_t value)const{return value;}
        std::uint32_t axis(std::uint32_t value)const{return value;}
        std::uint32_t displacement()const{return q.displacement;}
        std::uint32_t origin()const{return q.origin;}
        std::uint32_t component(std::uint32_t index,const std::array<std::uint32_t,3>&values)const{return values.at(index);}
        std::uint32_t verify()const{return q.verify_polygon;}
        std::uint32_t retry_index(std::uint32_t value)const{return value;}
        std::uint32_t retry_end(std::uint32_t value)const{return value;}
        std::uint32_t tree(const native_bsp_leaf_surface_detail::Progress&data){
            return halo2::tree(memory,q.descriptor+0x20u,data.node,HostPoint{data.point});
        }
        bool polygon(const native_bsp_leaf_surface_detail::Progress&data){
            return halo2::polygon(memory,q.descriptor,q.bitmap,q.bitmap_limit,data.result,std::uint16_t(data.axis),data.side!=0,HostPoint{data.point});
        }
    }inputs{m,q};
    return native_bsp_leaf_surface_detail::query(m,inputs,[](auto,const auto&){});
}
}
#endif
std::uint32_t native_tu15_find_bsp_surface_2d(SourcePhysicsContactMemory&memory,
    std::uint32_t descriptor,std::uint32_t node,std::uint32_t point,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);const SourcePhysicsContactRecordObserver empty;
    joint_constraint_numeric::Memory m(memory,empty);return tree(m,descriptor,node,MemoryPoint{m,point});
#else
    (void)memory;(void)descriptor;(void)node;(void)point;(void)fp;(void)depth;throw std::runtime_error("Native BSP surface backend unavailable");
#endif
}
std::uint32_t native_tu15_find_bsp_surface_2d_value(SourcePhysicsContactMemory&memory,
    std::uint32_t descriptor,std::uint32_t node,const NativeTu15BspProjectedPoint&point,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);const SourcePhysicsContactRecordObserver empty;
    joint_constraint_numeric::Memory m(memory,empty);return tree(m,descriptor,node,HostPoint{point});
#else
    (void)memory;(void)descriptor;(void)node;(void)point;(void)fp;(void)depth;throw std::runtime_error("Native BSP surface backend unavailable");
#endif
}
bool native_tu15_point_in_bsp_surface(SourcePhysicsContactMemory&memory,
    std::uint32_t descriptor,std::uint32_t bitmap,std::uint16_t bitmap_limit,std::uint32_t surface,
    std::uint16_t axis,bool winding,std::uint32_t point,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);const SourcePhysicsContactRecordObserver empty;
    joint_constraint_numeric::Memory m(memory,empty);return polygon(m,descriptor,bitmap,bitmap_limit,surface,axis,winding,MemoryPoint{m,point});
#else
    (void)memory;(void)descriptor;(void)bitmap;(void)bitmap_limit;(void)surface;(void)axis;(void)winding;(void)point;(void)fp;(void)depth;throw std::runtime_error("Native BSP surface backend unavailable");
#endif
}
bool native_tu15_point_in_bsp_surface_value(SourcePhysicsContactMemory&memory,
    std::uint32_t descriptor,std::uint32_t bitmap,std::uint16_t bitmap_limit,std::uint32_t surface,
    std::uint16_t axis,bool winding,const NativeTu15BspProjectedPoint&point,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);const SourcePhysicsContactRecordObserver empty;
    joint_constraint_numeric::Memory m(memory,empty);return polygon(m,descriptor,bitmap,bitmap_limit,surface,axis,winding,HostPoint{point});
#else
    (void)memory;(void)descriptor;(void)bitmap;(void)bitmap_limit;(void)surface;(void)axis;(void)winding;(void)point;(void)fp;(void)depth;throw std::runtime_error("Native BSP surface backend unavailable");
#endif
}
std::uint32_t native_tu15_query_bsp_leaf_surface(SourcePhysicsContactMemory&memory,
    const NativeTu15BspSurfaceQuery&q,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    joint_constraint_numeric::Environment environment(fp,depth);const SourcePhysicsContactRecordObserver empty;
    joint_constraint_numeric::Memory m(memory,empty);return query(m,q);
#else
    (void)memory;(void)q;(void)fp;(void)depth;throw std::runtime_error("Native BSP surface backend unavailable");
#endif
}
} // namespace halo2
