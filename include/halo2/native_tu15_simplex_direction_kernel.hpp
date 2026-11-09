#pragma once
#include <cstdint>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
// A DATA continuation inside30E930. Caller-owned private rows stay ordinary
// inputs; this kernel never fabricates the paired payload of a promoted point.
enum class NativeTu15SimplexDirectionSide { first_edge, second_edge, points };
enum class NativeTu15SimplexDirectionPhase {
    endpoint, first, opposite, projected, select_endpoint, edge_direction,
    payload, promote_endpoint, promote_payload, decrement_count,
    point_second, point_first, point_direction, second_count, publish_count,
    finish_edge, finish_points
};
namespace native_simplex_direction_numeric {
using Vector=__m128;
inline Vector subtract(Vector a,Vector b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex direction backend unavailable");
#endif
}
inline Vector multiply(Vector a,Vector b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex direction backend unavailable");
#endif
}
inline Vector add_low(Vector a,Vector b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex direction backend unavailable");
#endif
}
template<unsigned control>inline Vector shuffle(Vector a){return _mm_shuffle_ps(a,a,control);}
struct Projection {
    Vector edge{},from_first{},from_endpoint{};
    Vector endpoint_terms{},endpoint_xy{},endpoint_sum{};
    Vector first_terms{},first_xy{},first_sum{};
    Vector inner_negative{},inner_positive{},inner_cross{};
    Vector outer_negative{},outer_positive{},direction{};
    std::uint32_t endpoint_sign{},first_sign{};
};
inline Projection project(Vector endpoint,Vector first,Vector opposite){
    Projection p;
    p.edge=subtract(endpoint,first);
    p.from_first=subtract(first,opposite);
    p.from_endpoint=subtract(endpoint,opposite);
    p.endpoint_terms=multiply(p.from_endpoint,p.edge);
    p.endpoint_xy=add_low(shuffle<0x55>(p.endpoint_terms),p.endpoint_terms);
    p.endpoint_sum=add_low(shuffle<0xaa>(p.endpoint_terms),p.endpoint_xy);
    p.first_terms=multiply(p.from_first,p.edge);
    p.first_xy=add_low(shuffle<0x55>(p.first_terms),p.first_terms);
    p.first_sum=add_low(shuffle<0xaa>(p.first_terms),p.first_xy);
    p.endpoint_sign=std::uint32_t(_mm_movemask_ps(p.endpoint_sum))&1u;
    p.first_sign=std::uint32_t(_mm_movemask_ps(p.first_sum))&1u;
    return p;
}
inline void interior(Projection&p,bool first_edge){
    p.inner_negative=multiply(shuffle<0xd2>(p.from_endpoint),shuffle<0xc9>(p.from_first));
    p.inner_positive=multiply(shuffle<0xc9>(p.from_endpoint),shuffle<0xd2>(p.from_first));
    p.inner_cross=subtract(p.inner_positive,p.inner_negative);
    if(first_edge){
        p.outer_negative=multiply(shuffle<0xd2>(p.edge),shuffle<0xc9>(p.inner_cross));
        p.outer_positive=multiply(shuffle<0xc9>(p.edge),shuffle<0xd2>(p.inner_cross));
    }else{
        p.outer_negative=multiply(shuffle<0xd2>(p.inner_cross),shuffle<0xc9>(p.edge));
        p.outer_positive=multiply(shuffle<0xc9>(p.inner_cross),shuffle<0xd2>(p.edge));
    }
    p.direction=subtract(p.outer_positive,p.outer_negative);
}
} // namespace native_simplex_direction_numeric

struct NativeTu15SimplexDirection {
    using Vector=native_simplex_direction_numeric::Vector;
    NativeTu15SimplexDirectionSide side{};
    std::uint32_t work{},second_count_address{},count{},second_count{};
    Vector endpoint{},first{},opposite{},payload{},direction{};
    native_simplex_direction_numeric::Projection projection;
};
// Access supplies live reads/writes and semantic phase publication. In
// particular, the count is acquired after promotion stores, and point inputs
// are freshly acquired after the decrement. Observers may change backing DATA
// at those boundaries. GP-mutating providers require a separate ABI owner.
template<class Access>
void native_tu15_reduce_simplex_direction(Access&memory,NativeTu15SimplexDirection&s){
    using P=NativeTu15SimplexDirectionPhase;
    using Side=NativeTu15SimplexDirectionSide;
    const bool first_edge=s.side==Side::first_edge;
    if(s.side!=Side::points){
        const auto base=first_edge?0x20u:0xa0u;
        s.endpoint=memory.vector_read(P::endpoint,s,s.work+base+0x10u);
        s.first=memory.vector_read(P::first,s,s.work+base);
        s.opposite=memory.vector_read(P::opposite,s,s.work+(first_edge?0xa0u:0x20u));
        s.projection=native_simplex_direction_numeric::project(s.endpoint,s.first,s.opposite);
        memory.prepare(P::projected,s);
        if(s.projection.endpoint_sign!=s.projection.first_sign){
            native_simplex_direction_numeric::interior(s.projection,first_edge);
            s.direction=s.projection.direction;
            memory.vector_write(P::edge_direction,s,s.work+0x120u,s.direction);
            s.second_count=memory.word_read(P::second_count,s,s.second_count_address);
            memory.word_write(P::publish_count,s,s.work+8u,s.second_count);
            memory.complete(P::finish_edge,s);
            return;
        }
        memory.prepare(P::select_endpoint,s);
        if(s.projection.endpoint_sign){
            s.payload=memory.vector_read(P::payload,s,s.work+(first_edge?0x70u:0xf0u));
            memory.vector_write(P::promote_endpoint,s,s.work+base,s.endpoint);
            memory.vector_write(P::promote_payload,s,s.work+(first_edge?0x60u:0xe0u),s.payload);
        }
        const auto count_address=first_edge?s.work:s.second_count_address;
        s.count=memory.word_read(P::decrement_count,s,count_address)-1u;
        memory.word_write(P::decrement_count,s,count_address,s.count);
    }
    s.opposite=memory.vector_read(P::point_second,s,s.work+0xa0u);
    s.first=memory.vector_read(P::point_first,s,s.work+0x20u);
    s.direction=native_simplex_direction_numeric::subtract(s.opposite,s.first);
    memory.vector_write(P::point_direction,s,s.work+0x120u,s.direction);
    s.second_count=memory.word_read(P::second_count,s,s.second_count_address);
    memory.word_write(P::publish_count,s,s.work+8u,s.second_count);
    memory.complete(P::finish_points,s);
}
} // namespace halo2
