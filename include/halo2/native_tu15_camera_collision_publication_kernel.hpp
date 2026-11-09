#pragma once
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <cstring>
namespace halo2::native_camera_collision_publication_detail {
using Scalar=native_bsp_surface_2d_detail::Scalar;
namespace N=native_bsp_surface_2d_detail;
enum class Read {fraction,plane,plane_y,plane_z,row_x_z,row_x_y,plane_x,row_x_x,
    row_y_z,row_y_y,row_y_x,row_z_z,row_z_y,row_z_x,distance_z,plane_w,scale,
    distance_y,stored_y,stored_x,distance_x,index,negated_value,copied_value,
    point,kind,surface,material};
enum class Write {fraction,normal,negated_value,copied_value,point,kind,surface,material};
struct Progress {
    std::uint32_t output{},transform{},collision{},plane{},index{},lane{},value{};
    bool transformed{},flipped{};
    Scalar plane_x{},plane_y{},plane_z{},normal{},term{},other_term{},distance{},negative{};
};
inline std::uint32_t bits(Scalar value){std::uint32_t result{};std::memcpy(&result,&value,4);return result;}
// Publish a local collision plane and payload into the caller's result. The
// normal/plane operands are cached only where the original caches them; all
// later output and collision fields remain lazy to preserve overlap.
// Access owns raw saved-word transport and observable source ABI, if requested.
template<class Access>void publish(Access&memory,Progress&d){
    memory.begin(d);d.transformed=d.transform!=0u;
    d.value=memory.integer(Read::fraction,d);memory.store(Write::fraction,d,d.value);
    if(d.transformed){
        d.plane=memory.integer(Read::plane,d);
        d.plane_y=memory.scalar(Read::plane_y,d);d.plane_z=memory.scalar(Read::plane_z,d);
        d.normal=memory.scalar(Read::row_x_z,d);d.term=memory.scalar(Read::row_x_y,d);
        d.plane_x=memory.scalar(Read::plane_x,d);
        d.normal=N::multiply(d.normal,d.plane_z);d.term=N::multiply(d.term,d.plane_y);d.normal=N::add(d.normal,d.term);
        d.term=memory.scalar(Read::row_x_x,d);d.term=N::multiply(d.term,d.plane_x);d.normal=N::add(d.normal,d.term);
        d.lane=0;memory.store(Write::normal,d,bits(d.normal));
        d.normal=memory.scalar(Read::row_y_z,d);d.term=memory.scalar(Read::row_y_y,d);
        d.normal=N::multiply(d.normal,d.plane_z);d.term=N::multiply(d.term,d.plane_y);d.normal=N::add(d.normal,d.term);
        d.term=memory.scalar(Read::row_y_x,d);d.term=N::multiply(d.term,d.plane_x);d.normal=N::add(d.normal,d.term);
        d.lane=1;memory.store(Write::normal,d,bits(d.normal));
        d.normal=memory.scalar(Read::row_z_z,d);d.normal=N::multiply(d.normal,d.plane_z);
        d.term=memory.scalar(Read::row_z_y,d);d.term=N::multiply(d.term,d.plane_y);
        d.other_term=memory.scalar(Read::row_z_x,d);d.normal=N::add(d.normal,d.term);
        d.other_term=N::multiply(d.other_term,d.plane_x);d.normal=N::add(d.normal,d.other_term);
        d.lane=2;memory.store(Write::normal,d,bits(d.normal));
        d.distance=memory.scalar(Read::distance_z,d);d.distance=N::multiply(d.distance,d.normal);
        d.term=memory.scalar(Read::plane_w,d);d.term=N::multiply(d.term,memory.scalar(Read::scale,d));d.distance=N::add(d.distance,d.term);
        d.term=memory.scalar(Read::distance_y,d);d.term=N::multiply(d.term,memory.scalar(Read::stored_y,d));d.distance=N::add(d.distance,d.term);
        d.term=memory.scalar(Read::stored_x,d);d.term=N::multiply(d.term,memory.scalar(Read::distance_x,d));d.distance=N::add(d.distance,d.term);
        d.lane=3;memory.store(Write::normal,d,bits(d.distance));
    }
    d.index=memory.integer(Read::index,d);d.flipped=(d.index&0x8000u)!=0u;
    if(d.flipped){
        if(!d.transformed)d.plane=memory.integer(Read::plane,d);
        for(d.lane=0;d.lane<4;++d.lane){
            d.negative=N::subtract(Scalar{},memory.scalar(Read::negated_value,d));
            memory.store(Write::negated_value,d,bits(d.negative));
        }
    }else if(!d.transformed){
        d.plane=memory.integer(Read::plane,d);memory.copy_begin(d);
        for(d.lane=0;d.lane<4;++d.lane){d.value=memory.integer(Read::copied_value,d);memory.store(Write::copied_value,d,d.value);}
        memory.copy_end(d);
    }
    for(d.lane=0;d.lane<3;++d.lane){d.value=memory.integer(Read::point,d);memory.store(Write::point,d,d.value);}
    d.value=memory.integer(Read::kind,d);memory.store(Write::kind,d,d.value);
    d.value=memory.integer(Read::surface,d);memory.store(Write::surface,d,d.value);
    d.value=memory.integer(Read::material,d);memory.store(Write::material,d,d.value);
    memory.finish(d);
}
} // namespace halo2::native_camera_collision_publication_detail
#endif
