#pragma once
#include <array>
#include <cstdint>
#include <initializer_list>

namespace halo2::native_sphere_frustum_detail {
enum class Visibility:std::uint32_t {outside=0,intersecting=1,contained=2};
enum class Read {
    lower_center,upper_bound,upper_center,lower_bound,
    normal_z,center_z,normal_y,center_y,normal_x,center_x,plane_distance
};
enum class Comparison {axis_lower,axis_upper,plane_rejection,contained};
template<class Scalar>struct Work {
    Scalar radius{},scratch{},bound{},accumulator{};
    std::array<Scalar,6>distances{};
    unsigned axis{},plane{};
};
// The shared sphere classification algorithm. A value view or a retained
// memory view supplies lazy scalar acquisition and arithmetic. Register/flag
// publication and source PC ownership belong exclusively to the retained view.
// Plane4 rejects, but deliberately does not participate in containment.
template<class Frame>Visibility classify(Frame&frame){
    using Scalar=typename Frame::Scalar;Work<Scalar>d{};d.radius=frame.radius();
    for(unsigned axis=0;axis<3;++axis){
        d.axis=axis;d.scratch=frame.read(Read::lower_center,d);
        d.scratch=frame.subtract(d.scratch,d.radius);
        const auto maximum=frame.read(Read::upper_bound,d);
        if(frame.above(d.scratch,maximum,Comparison::axis_lower,d))return frame.finish(Visibility::outside,d);
    }
    d.axis=0;d.scratch=frame.read(Read::upper_center,d);d.bound=frame.read(Read::lower_bound,d);
    d.scratch=frame.add(d.scratch,d.radius);
    if(frame.above(d.bound,d.scratch,Comparison::axis_upper,d))return frame.finish(Visibility::outside,d);
    for(unsigned axis=1;axis<3;++axis){
        d.axis=axis;d.bound=frame.read(Read::lower_bound,d);d.scratch=d.radius;
        const auto coordinate=frame.read(Read::upper_center,d);d.scratch=frame.add(d.scratch,coordinate);
        if(frame.above(d.bound,d.scratch,Comparison::axis_upper,d))return frame.finish(Visibility::outside,d);
    }
    for(unsigned plane=0;plane<6;++plane){
        d.plane=plane;d.accumulator=frame.read(Read::normal_z,d);
        const auto z=frame.read(Read::center_z,d);d.accumulator=frame.multiply(d.accumulator,z);
        d.scratch=frame.read(Read::normal_y,d);const auto y=frame.read(Read::center_y,d);
        d.scratch=frame.multiply(d.scratch,y);d.accumulator=frame.add(d.accumulator,d.scratch);
        // The final plane's X product reads center first and keeps it on the
        // left. This order also fixes native NaN payload/operand ownership.
        if(plane==5){d.scratch=frame.read(Read::center_x,d);const auto normal=frame.read(Read::normal_x,d);d.scratch=frame.multiply(d.scratch,normal);}
        else {d.scratch=frame.read(Read::normal_x,d);const auto x=frame.read(Read::center_x,d);d.scratch=frame.multiply(d.scratch,x);}
        d.accumulator=frame.add(d.accumulator,d.scratch);
        const auto distance=frame.read(Read::plane_distance,d);d.accumulator=frame.subtract(d.accumulator,distance);d.distances[plane]=d.accumulator;
        if(frame.above(d.accumulator,d.radius,Comparison::plane_rejection,d))return frame.finish(Visibility::outside,d);
    }
    d.scratch=frame.subtract(frame.zero(),d.radius);
    for(unsigned plane:{0u,1u,2u,3u,5u})
        if(!frame.above(d.scratch,d.distances[plane],Comparison::contained,d))return frame.finish(Visibility::intersecting,d);
    return frame.finish(Visibility::contained,d);
}
}
