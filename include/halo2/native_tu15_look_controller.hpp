#pragma once
#include "halo2/native_tu15_look_context.hpp"
namespace halo2 {
struct NativeTu15LookControllerScratch {
    // At least0xB4 caller-owned writable DATA bytes, separate from the input
    // control record and every service's downward-growing control frame.
    std::uint32_t identity{};
};
struct NativeTu15LookControllerServices {
    NativeTu15LookContextServices context;
    // ActualBAD60 with direct=false, capacity=1. Must execute all marker
    // writes and FP, including fallback output when a named marker is absent.
    std::function<void(std::uint32_t object,std::uint32_t marker,
        std::uint32_t output,NativeTu15PhysicsFrameFp&)> marker;
    // Actual11F7E0 scalar result bits. Ordinary vector DATA addresses, never
    // an XMM postimage. Its CRT acos child remains an explicit dependency
    // until the native leaf has independent original qualification.
    std::function<std::uint32_t(std::uint32_t first,std::uint32_t second,
        NativeTu15PhysicsFrameFp&,unsigned incoming_depth)> vector_angle;
};
struct NativeTu15LookControllerStats {
    std::uint64_t calls{},completed{},refusals{},marker_requests{},angle_calls{};
    NativeTu15LookContextStats context;
};
// Actual334-site18A300 / sourceEAX=index, stack(dyaw,dpitch), RET8.
// All floating operands and retained writes come from current banks and
// explicit typed source services. This does not use the client-only look API.
void native_tu15_update_look_controller(SourcePhysicsContactMemory&,
    std::uint32_t index,std::uint32_t yaw_delta_bits,std::uint32_t pitch_delta_bits,
    NativeTu15LookControllerScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15LookControllerServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15LookControllerStats* = nullptr);
} // namespace halo2
