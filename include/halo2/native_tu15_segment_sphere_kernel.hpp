#pragma once
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_segment_sphere_detail {
namespace N=native_bsp_surface_2d_detail;
using Scalar=N::Scalar;
enum class Read {center_y,first_y,center_z,first_z,center_x,first_x,radius,direction_x,direction_y,direction_z,scratch_z,scratch_y,scratch_x};
enum class Write {scratch_z,scratch_y,scratch_x};
enum class Test {inside,approaching,discriminant,endpoint,endpoint_discriminant};
enum class Result {byte_false,byte_true,word_false,word_true};
struct Comparison {
    bool below{},equal{},unordered{};std::uint32_t flags{};
    bool above()const{return !below&&!equal;}
};
inline Comparison compare(Scalar first,Scalar second){
    unsigned char below,equal,unordered;
    asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(unordered):"x"(first),"x"(second):"cc","memory");
    return{below!=0,equal!=0,unordered!=0,0x02u|std::uint32_t(below)|(std::uint32_t(unordered)<<2u)|(std::uint32_t(equal)<<6u)};
}
struct Progress {
    Scalar dy{},dz{},dx{},distance{},term{},radius{},radius_squared{},zero{};
    Scalar direction_z{},direction_y{},direction_x{},dot{},product{},length{},square_y{},square_x{},discriminant{},scaled_distance{},endpoint{},endpoint_squared{};
    std::uint32_t raw_x{},raw_y{},raw_z{};
};
// Actual120FA0 segment/sphere admission. The displacement words are acquired
// only after the initial containment test; their saved scalar copies are
// reacquired through Access to preserve real caller scratch overlaps.
// Numerical policy has no virtual registers, flags bank or instruction stream.
template<class Access>bool intersects(Access&memory,Progress&d){
    d.dy=memory.scalar(Read::center_y,d);d.dy=N::subtract(d.dy,memory.scalar(Read::first_y,d));
    d.dz=memory.scalar(Read::center_z,d);d.dz=N::subtract(d.dz,memory.scalar(Read::first_z,d));
    d.dx=memory.scalar(Read::center_x,d);d.dx=N::subtract(d.dx,memory.scalar(Read::first_x,d));
    d.zero=Scalar{};d.distance=N::multiply(d.dz,d.dz);d.term=N::multiply(d.dy,d.dy);d.distance=N::add(d.distance,d.term);
    d.term=N::multiply(d.dx,d.dx);d.distance=N::add(d.distance,d.term);
    d.radius=memory.scalar(Read::radius,d);d.radius_squared=N::multiply(d.radius,d.radius);
    memory.reserve(d);d.distance=N::subtract(d.distance,d.radius_squared);
    if(memory.test(Test::inside,d,d.zero,d.distance).above()){memory.finish(Result::byte_true,d);return true;}
    d.raw_x=memory.integer(Read::direction_x,d);d.raw_y=memory.integer(Read::direction_y,d);d.raw_z=memory.integer(Read::direction_z,d);
    memory.store(Write::scratch_z,d,d.raw_z);d.direction_z=memory.scalar(Read::scratch_z,d);d.dot=N::multiply(d.direction_z,d.dz);
    memory.store(Write::scratch_y,d,d.raw_y);d.direction_y=memory.scalar(Read::scratch_y,d);d.product=N::multiply(d.direction_y,d.dy);d.dot=N::add(d.dot,d.product);
    memory.store(Write::scratch_x,d,d.raw_x);d.direction_x=memory.scalar(Read::scratch_x,d);d.product=N::multiply(d.direction_x,d.dx);d.dot=N::add(d.dot,d.product);
    if(!memory.test(Test::approaching,d,d.dot,d.zero).below){memory.finish(Result::byte_false,d);return false;}
    d.length=N::multiply(d.direction_z,d.direction_z);d.square_y=N::multiply(d.direction_y,d.direction_y);d.length=N::add(d.length,d.square_y);
    d.square_x=N::multiply(d.direction_x,d.direction_x);d.length=N::add(d.length,d.square_x);
    d.discriminant=N::multiply(d.dot,d.dot);d.scaled_distance=N::multiply(d.length,d.distance);d.discriminant=N::subtract(d.discriminant,d.scaled_distance);
    if(!memory.test(Test::discriminant,d,d.zero,d.discriminant).below){memory.finish(Result::byte_false,d);return false;}
    d.endpoint=N::subtract(d.zero,d.length);d.endpoint=N::subtract(d.endpoint,d.dot);
    if(memory.test(Test::endpoint,d,d.zero,d.endpoint).above()){memory.finish(Result::byte_true,d);return true;}
    d.endpoint_squared=N::multiply(d.endpoint,d.endpoint);
    const bool accepted=memory.test(Test::endpoint_discriminant,d,d.discriminant,d.endpoint_squared).above();
    memory.finish(accepted?Result::word_true:Result::word_false,d);return accepted;
}
} // namespace halo2::native_segment_sphere_detail
#endif
