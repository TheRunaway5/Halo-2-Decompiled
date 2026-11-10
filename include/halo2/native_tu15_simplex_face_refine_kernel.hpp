#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include "halo2/native_tu15_simplex_projection_kernel.hpp"

namespace halo2 {
enum class NativeTu15SimplexFaceRefinePhase {
    first_index,second_index,cursor,origin,apex,first_vertex,second_vertex,
    point_argument,point,along_first,along_second,apex_first,apex_second,
    invalidate_first,invalidate_second,advance
};
struct NativeTu15SimplexFaceRefine {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t vertices{},scores{},index{},first{},second{},cursor{},point{};
    Vector origin{},apex{},first_vertex{},first_direction{},apex_delta{},second_vertex{},ray{},ray_delta{},second_direction{};
    Vector first_products{},first_xy{},first_z{},first_sum{},second_products{},second_xy{},second_sum{};
    Vector apex_first_products{},apex_first_xy{},apex_first_sum{},apex_second_products{},apex_second_xy{},apex_second_sum{};
};
// Actual30D1A0: stream the three source-defined face pairs, invalidate the
// geometrically inconsistent candidate, then return the first positive face.
// Access provides caller-owned scratch, immutable signed-byte table rows and
// delayed argument/score reads. This is geometry policy, not a CPU program.
template<class Access>
std::uint32_t native_tu15_refine_simplex_faces(Access&memory,NativeTu15SimplexFaceRefine&d){
    using P=NativeTu15SimplexFaceRefinePhase;
    namespace N=native_simplex_direction_numeric;
    namespace X=native_simplex_projection_numeric;
    memory.begin(d);
    for(d.index=0;d.index<3u;++d.index){
        d.first=memory.index(P::first_index,d,0x41d762u+d.index);
        d.second=memory.index(P::second_index,d,0x41d760u+d.index);
        if(memory.nonnegative(d,false)&&memory.nonnegative(d,true)){
            d.cursor=memory.cursor(d);
            d.origin=memory.vector_read(P::origin,d,d.cursor);
            d.apex=memory.vector_read(P::apex,d,d.vertices+0x30u);
            d.first_vertex=memory.vector_read(P::first_vertex,d,d.vertices+d.first*16u);
            d.first_direction=N::subtract(d.first_vertex,d.origin);
            d.apex_delta=N::subtract(d.apex,d.origin);
            d.second_vertex=memory.vector_read(P::second_vertex,d,d.vertices+d.second*16u);
            d.point=memory.point_argument(d);
            d.ray=memory.vector_read(P::point,d,d.point);
            d.ray_delta=N::subtract(d.ray,d.origin);
            d.second_direction=N::subtract(d.second_vertex,d.origin);
            d.first_products=N::multiply(d.ray_delta,d.first_direction);
            d.first_xy=N::add_low(N::shuffle<0x55>(d.first_products),d.first_products);
            d.second_products=N::multiply(d.ray_delta,d.second_direction);
            d.first_z=N::shuffle<0xaa>(d.first_products);
            d.second_xy=N::add_low(N::shuffle<0x55>(d.second_products),d.second_products);
            d.first_sum=N::add_low(d.first_z,d.first_xy);
            d.second_sum=N::add_low(N::shuffle<0xaa>(d.second_products),d.second_xy);
            memory.score_write(P::along_first,d,X::low(d.first_sum));
            d.apex_first_products=N::multiply(d.apex_delta,d.first_direction);
            d.apex_first_xy=N::add_low(N::shuffle<0x55>(d.apex_first_products),d.apex_first_products);
            memory.score_write(P::along_second,d,X::low(d.second_sum));
            d.apex_second_products=N::multiply(d.apex_delta,d.second_direction);
            d.apex_first_sum=N::add_low(N::shuffle<0xaa>(d.apex_first_products),d.apex_first_xy);
            memory.score_write(P::apex_first,d,X::low(d.apex_first_sum));
            d.apex_second_xy=N::add_low(N::shuffle<0x55>(d.apex_second_products),d.apex_second_products);
            d.apex_second_sum=N::add_low(N::shuffle<0xaa>(d.apex_second_products),d.apex_second_xy);
            memory.score_write(P::apex_second,d,X::low(d.apex_second_sum));
            const bool discard_second=memory.compare_products(d);
            memory.invalidate(discard_second?P::invalidate_second:P::invalidate_first,d);
        }
        memory.advance(d);
    }
    for(unsigned face=0;face<3u;++face){
        if(memory.positive(d,face)){memory.finish(face);return face;}
    }
    memory.finish(0xffffffffu);return 0xffffffffu;
}
} // namespace halo2
