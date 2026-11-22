#pragma once
#include "campaign/scripting/native_campaign_script_fp_tu15.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <stdexcept>
#if HALO2_NATIVE_SCRIPT_X86_FP
#include <xmmintrin.h>
#endif

namespace halo2::native_animation_channel_postprocess_detail {

#if HALO2_NATIVE_SCRIPT_X86_FP
class ScalarScope {
    std::uint32_t &source_;
    const std::uint32_t host_;

public:
    explicit ScalarScope(std::uint32_t &source) : source_(source), host_(_mm_getcsr()) {
        if ((source_ & 0x1f80u) != 0x1f80u)
            throw std::runtime_error("Unmasked TU1.5 animation channel FP exceptions are unsupported");
        _mm_setcsr(source_);
    }
    ScalarScope(const ScalarScope &) = delete;
    ScalarScope &operator=(const ScalarScope &) = delete;
    ~ScalarScope() {
        source_ = _mm_getcsr();
        _mm_setcsr(host_);
    }
};

inline bool above(float left, float right) {
    unsigned char result;
    asm volatile("comiss %2,%1; seta %0" : "=qm"(result) : "x"(left), "x"(right) : "cc", "memory");
    return result != 0;
}

inline float multiply(float left, float right) {
    asm volatile("mulss %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}

inline float add(float left, float right) {
    asm volatile("addss %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}

inline float divide(float left, float right) {
    asm volatile("divss %1,%0" : "+x"(left) : "x"(right) : "memory");
    return left;
}

// 1CF0E0's scalar result. The frame-local return word begins at +0.0f.
// The descriptor lookup is the factory's existing 1E3150 owner.
template<class Frame, class Lookup>
float progress(Frame &frame, std::uint32_t channel, Lookup &lookup) {
    if (frame.r16(channel + 6u) == 0xffffu) return 0.0f;
    const auto tag = frame.r32(channel);
    const auto tags = frame.r32(0x4f0414u);
    const auto graph = frame.r32(tags + (tag & 0xffffu) * 16u + 8u);
    const auto animation = lookup(graph, frame.r32(channel + 4u));
    const auto encoded_frames = static_cast<std::uint16_t>(frame.r16(animation + 0x14u));
    const auto frames = static_cast<float>(std::bit_cast<std::int16_t>(encoded_frames));
    if (!above(frames, 0.0f)) return 0.0f;
    const auto ratio = divide(std::bit_cast<float>(frame.r32(channel + 0x1cu)), frames);
    if (above(0.0f, ratio)) return 0.0f;
    const auto upper = std::bit_cast<float>(frame.r32(0x46a0d0u));
    return above(ratio, upper) ? upper : ratio;
}
#endif

// 1D63C0, entered at 1D5897 after the channel-2 sample. Compute all three
// coordinates before the first store so aliased source/destination reads and
// per-store faults retain their original ordering.
template<class Frame, class Lookup>
void apply(Frame &frame, std::uint32_t manager, std::uint32_t weight,
           std::uint32_t orientation, Lookup lookup) {
    if (!(frame.r8(manager + 0x6eu) & 2u)) return;
    const auto channel = manager + 0x40u;
    if (frame.r32(channel) == 0xffffffffu || frame.r16(channel + 6u) == 0xffffu) return;
#if HALO2_NATIVE_SCRIPT_X86_FP
    std::array<std::uint32_t, 3> position;
    {
        ScalarScope fp(frame.fp().mxcsr);
        const auto fraction = progress(frame, channel, lookup);
        const auto x = std::bit_cast<float>(frame.r32(manager + 0x84u));
        const auto y = std::bit_cast<float>(frame.r32(manager + 0x88u));
        const auto z = std::bit_cast<float>(frame.r32(manager + 0x8cu));
        const auto blend = multiply(fraction, std::bit_cast<float>(weight));
        const auto dx = multiply(x, blend);
        const auto px = add(dx, std::bit_cast<float>(frame.r32(orientation + 0x10u)));
        const auto dy = multiply(y, blend);
        const auto py = add(dy, std::bit_cast<float>(frame.r32(orientation + 0x14u)));
        const auto dz = multiply(z, blend);
        const auto pz = add(dz, std::bit_cast<float>(frame.r32(orientation + 0x18u)));
        position = {std::bit_cast<std::uint32_t>(px), std::bit_cast<std::uint32_t>(py), std::bit_cast<std::uint32_t>(pz)};
    }
    frame.w32(orientation + 0x10u, position[0]);
    frame.w32(orientation + 0x14u, position[1]);
    frame.w32(orientation + 0x18u, position[2]);
#else
    (void)weight;
    (void)orientation;
    (void)lookup;
    throw std::runtime_error("TU1.5 animation channel post-process requires the x86 SSE owner");
#endif
}

} // namespace halo2::native_animation_channel_postprocess_detail
