#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>
namespace halo2 {
class NativeTu15RenderCameraGridArgument {
public:
    virtual ~NativeTu15RenderCameraGridArgument()=default;
    virtual std::uint32_t grid()const=0;
};
class NativeTu15RenderCameraDivideError final:public std::runtime_error {
    std::int32_t dividend_,divisor_;
public:
    NativeTu15RenderCameraDivideError(std::int32_t,std::int32_t);
    std::int32_t dividend()const noexcept{return dividend_;}
    std::int32_t divisor()const noexcept{return divisor_;}
};
class NativeTu15RenderCameraDefaultRequired final:public std::runtime_error {
public:
    NativeTu15RenderCameraDefaultRequired();
    std::uint32_t original_branch()const noexcept{return 0x2f8a5u;}
};
// Actual2F7E0 nonnull observer profile: lazy vector/angle publication, signed
// split ratio and real hardware SIMD scaling. The null-observer default
// projection retains its explicit unqualified native capability boundary.
void native_tu15_render_camera_packet(SourcePhysicsContactMemory &,std::uint32_t observer,
    std::uint32_t destination,std::uint32_t spans,const NativeTu15RenderCameraGridArgument &,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
}
