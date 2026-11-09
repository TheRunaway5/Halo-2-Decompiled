#pragma once
#include "rendering/models/native_tu15_prt_basis.hpp"

namespace halo2::model_vector_length_detail {
struct Vector { long double x,y,z; };
// Both original functions retain extended inputs and the same (xx+yy)+zz
// grouping. The caller's x87 precision and rounding control own each operation.
inline long double length(const Vector& vector) {
    using physics_step_detail::mul;using physics_step_detail::add;
    const auto xx=mul(vector.x,vector.x),yy=mul(vector.y,vector.y);
    const auto xy=add(xx,yy),zz=mul(vector.z,vector.z);
    return physics_solver_detail::root(add(xy,zz));
}
inline long double magnitude(const SourcePhysicsContactMemory& memory,std::uint32_t input) {
    const auto z=prt_basis_detail::single(memory,input+8u);
    const auto y=prt_basis_detail::single(memory,input+4u);
    const auto x=prt_basis_detail::single(memory,input);
    return length({x,y,z});
}
inline long double distance(const SourcePhysicsContactMemory& memory,std::uint32_t first,std::uint32_t second) {
    using prt_basis_detail::single;using physics_step_detail::sub;
    const auto ax=single(memory,first);const auto bx=single(memory,second);
    const auto x=sub(ax,bx);
    const auto ay=single(memory,first+4u);const auto by=single(memory,second+4u);
    const auto y=sub(ay,by);
    const auto az=single(memory,first+8u);const auto bz=single(memory,second+8u);
    const auto z=sub(az,bz);
    return length({x,y,z});
}
template<class State>void bridge(State& state,std::uint32_t entry) {
    const auto result=entry==0x24570u?magnitude(state.memory,state.gp[0]):
        distance(state.memory,state.gp[0],state.gp[1]);
    // The final FSTP instructions clear C1 and leave exactly one new ST0.
    state.fp.push(result);
    (void)prt_helpers_detail::read(state.memory,state.gp[4]);state.gp[4]+=4u;
}
}
