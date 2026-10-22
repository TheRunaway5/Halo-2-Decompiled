#pragma once
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"
namespace halo2::physics_vehicle_motion_detail {
using namespace physics_vehicle_runtime_detail;
struct MotionState : VehicleState {
    using VehicleState::VehicleState;
    void trigonometric(bool cosine) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto input=fp.st(0);long double result;std::uint16_t status;
        if(cosine)asm volatile("fldt %2\n\tfcos\n\tfnstsw %1\n\tfstpt %0"
            :"=m"(result),"=m"(status):"m"(input):"st");
        else asm volatile("fldt %2\n\tfsin\n\tfnstsw %1\n\tfstpt %0"
            :"=m"(result),"=m"(status):"m"(input):"st");
        fp.st(0)=result;
        // C2 is the architecturally defined range result. Other condition
        // bits are retained; exception flags remain outside this contract.
        fp.status=std::uint16_t((fp.status&~0x400u)|(status&0x400u));
#else
        throw std::runtime_error("Required original vehicle FSIN/FCOS backend unavailable");
#endif
    }
};
} // namespace halo2::physics_vehicle_motion_detail
