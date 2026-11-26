#pragma once
// Native TU1.5 vector normalize, entered at 1E1010 (scene device constructor
// and map world support units): normalize a 4-lane SSE vector in place using
// RSQRTSS with one Newton-Raphson refinement step.
//
//   vector_normalize::normalize   1E1010(ECX = vector_address); RET 0
//
// The 16-byte vector at ECX is squared lane-wise and horizontally summed via
// two SHUFPS+ADDPS folds (masks 78 then 177) to form dot = x^2+y^2+z^2+w^2 in
// all four lanes. After spilling dot to the 16-byte-aligned stack slot,
// RSQRTSS estimates 1/sqrt(dot), spills and reloads the estimate through the
// stack slot, refines it via 0.5 * est * (3.0 - dot * est * est) (constants
// at 0x46A0E4 = 3.0f and 0x46A0CC = 0.5f), broadcasts the scalar scale, and
// writes scale * vector back to ECX.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_vector_normalize.hpp) maps the role to the original
// instruction, invokes the live reciprocal_sqrt service for RSQRTSS, and
// owns the 16-byte aligned frame.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <immintrin.h>

namespace halo2::native_vector_normalize {
namespace globals {inline constexpr std::uint32_t three=0x46a0e4u,half=0x46a0ccu;}
inline constexpr std::uint32_t stack_align_mask=0xfffffff0u,frame_size=0x10u;

namespace vector_normalize {
enum class Access {vector_load,three_load,norm_store,estimate_store,estimate_reload,half_load,vector_store};

// Frame: see native_tu15_vector_normalize.hpp.
template<class Memory,class Frame> void normalize(Memory &memory,Frame &frame,std::uint32_t vec_addr) {
    using enum Access;
    frame.align_frame();
    const auto vec=memory.load128(vector_load,vec_addr);
    const auto three=frame.scalar(3u,memory.load32(three_load,globals::three));
    const auto sq=frame.mul_packed(1u,vec,vec);
    const auto fold1=frame.add_packed(0u,frame.shuffle(0u,sq,sq,78),sq);
    const auto fold2=frame.shuffle(1u,fold1,fold1,177);
    const auto dot=frame.add_packed(0u,fold1,fold2);
    const auto slot=frame.slot_address();
    memory.store128(norm_store,slot,dot);
    const auto est_vec=frame.estimate_rsqrt(fold2,dot);
    memory.store32(estimate_store,slot,frame.lane0(est_vec));
    const auto est=memory.load128(estimate_reload,slot);
    const auto dot_est=frame.mul_scalar(0u,dot,est);
    const auto dot_est2=frame.mul_scalar(0u,dot_est,est);
    const auto rem=frame.sub_scalar(3u,three,dot_est2);
    const auto half=frame.scalar(0u,memory.load32(half_load,globals::half));
    const auto half_est=frame.mul_scalar(0u,half,est);
    const auto refined=frame.mul_scalar(0u,half_est,rem);
    const auto scale=frame.shuffle(1u,refined,refined,0);
    const auto out=frame.mul_packed(1u,scale,vec);
    memory.store128(vector_store,vec_addr,out);
}
} // namespace vector_normalize
} // namespace halo2::native_vector_normalize
