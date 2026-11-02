#pragma once
#include "halo2/native_tu15_render_camera_packet.hpp"
namespace halo2 {
// Logical hardware ST(0)..ST(7), with validity retained through sparse,
// stack-overflow and rejected-store states. Arithmetic uses the real FPU.
struct NativeTu15RenderCameraX87State {
    std::array<std::array<std::byte,10>,8> values{};
    std::uint8_t occupied{};
};
class NativeTu15RenderCameraDefaultX87AbiRequired final:public std::runtime_error {
    NativeTu15RenderCameraX87State state_;
public:
    explicit NativeTu15RenderCameraDefaultX87AbiRequired(const NativeTu15RenderCameraX87State &);
    const NativeTu15RenderCameraX87State &state()const noexcept{return state_;}
};
// Actual380450 copy from46AB88. No eager FOV preset or storage allocation.
void native_tu15_render_camera_default_initialize(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete null-observer2F7E0 profile, including its common suffix. Actual
// older hardware x87 context is explicit; no guest stack or view is invented.
void native_tu15_render_camera_default_packet(SourcePhysicsContactMemory &,
    std::uint32_t destination,const NativeTu15RenderCameraGridArgument &,
    NativeTu15PhysicsFrameFp &,NativeTu15RenderCameraX87State &,
    const SourcePhysicsContactRecordObserver & = {});
}
