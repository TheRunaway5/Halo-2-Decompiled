#pragma once

#include "physics/vehicles/vehicle_marker_emission_instruction.hpp"

namespace halo2 {
namespace vehicle_marker_matrix_instruction_detail {
inline constexpr std::uint32_t actual_first = 0xbad60u;
inline constexpr std::uint32_t actual_last = 0xbaf05u;
inline constexpr std::uint32_t image_delta = 0x1140u;
inline constexpr std::uint32_t canonical_sites[]{
    0xb9c20u, 0xb9c23u, 0xb9c24u, 0xb9c25u, 0xb9c29u, 0xb9c2bu, 0xb9c2du,
    0xb9c2eu, 0xb9c30u, 0xb9c32u, 0xb9c33u, 0xb9c38u, 0xb9c3bu, 0xb9c41u,
    0xb9c47u, 0xb9c4au, 0xb9c4fu, 0xb9c52u, 0xb9c56u, 0xb9c58u, 0xb9c5eu,
    0xb9c64u, 0xb9c67u, 0xb9c6bu, 0xb9c6eu, 0xb9c71u, 0xb9c73u, 0xb9c7au,
    0xb9c81u, 0xb9c87u, 0xb9c8au, 0xb9c8eu, 0xb9c91u, 0xb9c98u, 0xb9c9bu,
    0xb9c9eu, 0xb9ca0u, 0xb9ca2u, 0xb9ca6u, 0xb9cabu, 0xb9cadu, 0xb9cb0u,
    0xb9cb4u, 0xb9cb7u, 0xb9cbbu, 0xb9cbfu, 0xb9cc4u, 0xb9cc9u, 0xb9ccdu,
    0xb9cceu, 0xb9ccfu, 0xb9cd0u, 0xb9cd4u, 0xb9cd5u, 0xb9cd7u, 0xb9cd8u,
    0xb9cddu, 0xb9cdfu, 0xb9ce2u, 0xb9ce8u, 0xb9cecu, 0xb9cefu, 0xb9cf5u,
    0xb9cf8u, 0xb9d00u, 0xb9d06u, 0xb9d08u, 0xb9d0bu, 0xb9d0eu, 0xb9d12u,
    0xb9d17u, 0xb9d1cu, 0xb9d21u, 0xb9d26u, 0xb9d2bu, 0xb9d30u, 0xb9d35u,
    0xb9d3au, 0xb9d3fu, 0xb9d44u, 0xb9d49u, 0xb9d4eu, 0xb9d53u, 0xb9d58u,
    0xb9d5bu, 0xb9d5fu, 0xb9d66u, 0xb9d68u, 0xb9d6bu, 0xb9d70u, 0xb9d72u,
    0xb9d77u, 0xb9d7au, 0xb9d7du, 0xb9d80u, 0xb9d82u, 0xb9d85u, 0xb9d8au,
    0xb9d8fu, 0xb9d92u, 0xb9d97u, 0xb9d9cu, 0xb9da1u, 0xb9da6u, 0xb9daau,
    0xb9dacu, 0xb9daeu, 0xb9dafu, 0xb9db0u, 0xb9db5u, 0xb9db6u, 0xb9db9u,
    0xb9dbcu, 0xb9dbdu, 0xb9dbeu, 0xb9dc1u, 0xb9dc2u, 0xb9dc5u,
};
} // namespace vehicle_marker_matrix_instruction_detail

// Translate only BAD60's own literal PCs. The child call and store adapters
// continue to use their existing vehicle ownership boundaries.
class VehicleMarkerMatrixInstruction {
    const SourcePhysicsContactRuntimeServices &source;

public:
    explicit VehicleMarkerMatrixInstruction(const SourcePhysicsContactRuntimeServices &source) : source(source) {}

    void operator()(std::uint32_t pc) const {
        using namespace vehicle_marker_matrix_instruction_detail;
        source.instruction(pc >= actual_first && pc <= actual_last ? pc - image_delta : pc);
    }

    bool native_position(std::uint32_t *&position) const {
        using namespace vehicle_marker_matrix_instruction_detail;
        return vehicle_marker_emission_instruction_detail::native_marker_position(source, canonical_sites,
                                                                                    image_delta, position);
    }
};
} // namespace halo2
