#include "halo2/native_tu15_bsp_surface_query.hpp"
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
    if(!(node&0x8000u)){
        const auto nodes=m.word(descriptor+4u);const auto zero=F{};
        do{
            const auto row=nodes+(node&0x7fffu)*16u;
            auto y=scalar(m.word(row+4u));y=mul(y,scalar(point.word(1)));
            auto x=scalar(m.word(row));x=mul(x,scalar(point.word(0)));
            y=add_scalar(y,x);y=sub(y,scalar(m.word(row+8u)));
            const auto side=compare(y,zero).below?0u:1u;
            node=signed_word(m.word(row+12u+side*2u,2));
        }while(!(node&0x8000u));
    }
    return node==0xffffffffu?node:node&0x7fffu;
}
template<class Point>bool polygon(Memory&m,std::uint32_t descriptor,std::uint32_t bitmap,
    std::uint16_t bitmap_limit,std::uint32_t surface,std::uint16_t axis,bool winding,const Point&point){
    const auto surfaces=m.word(descriptor+0x2cu),record=surfaces+surface*8u;
    if(bitmap&&(m.word(record+4u,1)&8u)){
        const auto material=m.word(record+5u,1);
        if(std::int32_t(material)<std::int32_t(std::bit_cast<std::int16_t>(bitmap_limit))){
            const auto current=m.word(record+5u,1),word=m.word(bitmap+(current>>5u)*4u);
            if(!(word&(1u<<(current&31u))))return false;
        }
    }
    const auto edges=m.word(descriptor+0x34u),vertices=m.word(descriptor+0x3cu);
    const auto signed_axis=signed_word(axis),first_edge=m.word(record+2u,2);const auto zero=F{};
    const auto table=0x44ac8cu+(std::uint32_t(winding)+signed_axis*2u)*6u;
    const auto first_offset=signed_word(m.word(table,2))*4u,second_offset=signed_word(m.word(table+2u,2))*4u;
    auto index=first_edge;
    for(;;){
        const auto edge=edges+index*12u;
        const auto selected=signed_word(m.word(edge+10u,2))==surface?1u:0u;
        const auto first_vertex=m.word(edge+selected*2u,2),position=vertices+first_vertex*16u;
        const auto first_x=scalar(m.word(position+first_offset)),first_y=scalar(m.word(position+second_offset));
        const auto px=scalar(point.word(0)),py=scalar(point.word(1));
        const auto dx=sub(px,first_x),dy=sub(py,first_y);
        const auto second_vertex=m.word(edge+(selected?0u:1u)*2u,2),other=vertices+second_vertex*16u;
        const auto target_x=scalar(m.word(other+first_offset)),target_y=scalar(m.word(other+second_offset));
        const auto edge_y=sub(target_y,first_y),edge_x=sub(target_x,first_x);
        auto cross=mul(edge_y,dx);cross=sub(cross,mul(dy,edge_x));
        if(compare(cross,zero).above())return false;
        index=m.word(edge+selected*2u+4u,2);if(index==first_edge)return true;
    }
}
std::uint32_t absolute(std::uint32_t bits){std::uint32_t value;push(bits);asm volatile("fabs;fstps %0":"=m"(value)::"memory","st");return value;}
std::uint32_t query(Memory&m,const NativeTu15BspSurfaceQuery&q){
    const auto leaves=m.word(q.descriptor+0x14u),leaf=leaves+q.candidate_leaf*4u;
    auto index=m.word(leaf+2u,2);const auto count=m.word(leaf+1u,1),end=index+count;
    if(index>=end)return 0xffffffffu;
    const auto entries=m.word(q.descriptor+0x1cu);auto cursor=entries+index*4u;
    const auto fraction=scalar(q.fraction_bits);
    for(;index<end;++index,cursor+=4u){
        const auto plane=signed_word(m.word(cursor,2));if((plane&0x7fffu)!=q.plane_index)continue;
        const auto planes=m.word(q.descriptor+12u),coefficients=planes+q.plane_index*16u;
        const auto abs_x=absolute(m.word(coefficients));const auto abs_y=absolute(m.word(coefficients+4u));
        const auto abs_z=absolute(m.word(coefficients+8u));
        push(abs_y);push(abs_z);unsigned char below;
        asm volatile("fcomip %%st(1),%%st;fstp %%st(0);setb %0":"=qm"(below)::"memory","cc","st","st(1)");
        std::uint16_t axis;
        if(!below&&!compare(scalar(abs_z),scalar(abs_x)).below)axis=2;
        else axis=compare(scalar(abs_y),scalar(abs_x)).below?0u:1u;
        const auto coefficient=scalar(m.word(coefficients+std::uint32_t(axis)*4u));
        const bool positive=compare(coefficient,scalar(m.word(0x46a0e8u))).above();
        auto x=scalar(m.word(q.displacement));const bool winding=positive!=bool((plane>>15u)&1u);
        x=mul(x,fraction);x=add_scalar(x,scalar(m.word(q.origin)));
        auto y=scalar(m.word(q.displacement+4u));y=mul(y,fraction);y=add_scalar(y,scalar(m.word(q.origin+4u)));
        auto z=scalar(m.word(q.displacement+8u));
        const auto table=0x44ac8cu+(std::uint32_t(winding)+std::uint32_t(axis)*2u)*6u;
        const auto first_component=signed_word(m.word(table,2)),second_component=signed_word(m.word(table+2u,2));
        const auto initial_node=signed_word(m.word(cursor+2u,2));
        z=mul(z,fraction);z=add_scalar(z,scalar(m.word(q.origin+8u)));
        const std::array values{scalar_bits(x),scalar_bits(y),scalar_bits(z)};
        const NativeTu15BspProjectedPoint point{values.at(first_component),values.at(second_component)};
        const auto result=tree(m,q.descriptor+0x20u,initial_node,HostPoint{point});
        if(!q.verify_polygon||polygon(m,q.descriptor,q.bitmap,q.bitmap_limit,result,axis,winding,HostPoint{point}))return result;
    }
    return 0xffffffffu;
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
