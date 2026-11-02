#pragma once
#include "halo2/native_tu15_array_growth.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ShaderFunctionEvaluationServices {
    // Actual13DE70: semantic EDI points to the8B {count,blob} reference inside
    // its20B shader function row at+0C; RET8 takes
    // two float-bit parameters and returns the low XMM0 float bits. No GP file.
    std::function<std::uint32_t(std::uint32_t function_record,
        std::uint32_t first_parameter_bits,std::uint32_t second_parameter_bits,
        NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth)> evaluate;
};
struct NativeTu15SurfaceConveyorStats {
    std::uint64_t shader_queries{},material_queries{},bindings{},function_calls{},
        unbound_function_calls{},published_axes{},material_vectors{};
};
struct NativeTu15SurfaceConveyorScratch {std::uint32_t first{},second{};};
// Actual355D0. Each output word is fully initialized before any function child.
bool native_tu15_shader_conveyor_derivative(SourcePhysicsContactMemory&,
    std::uint32_t shader_datum,std::uint32_t map_index,
    std::uint32_t first_output,std::uint32_t second_output,
    NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth,
    const NativeTu15ShaderFunctionEvaluationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SurfaceConveyorStats* = nullptr);
// Actual184C50. Caller owns both4B scratch outputs; they are ordinary DATA and
// are never populated with source frame/register bytes. Destination is12B.
bool native_tu15_material_surface_velocity(SourcePhysicsContactMemory&,
    std::uint32_t material,std::uint32_t destination,NativeTu15SurfaceConveyorScratch,
    NativeTu15PhysicsFrameFp&,unsigned actual_x87_depth,
    const NativeTu15ShaderFunctionEvaluationServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15SurfaceConveyorStats* = nullptr);
} // namespace halo2
