#pragma once
#include "rendering/models/native_tu15_prt_cache.hpp"
#include "rendering/models/native_tu15_prt_sample.hpp"
#include "rendering/models/native_tu15_prt_matrix_inverse.hpp"
#include "rendering/models/native_tu15_prt_rotation_bridge.hpp"

namespace halo2::prt_cache_build_detail {
using namespace prt_projection_detail;
// Integer and SIMD continuation values are boundary transport. The controller
// below uses native loops, direct memory transactions, and typed child kernels.
struct Continuation {
    std::uint32_t eax, ecx, edx, bx, stack, bp, si, di;
    std::array<__m128, 8> simd;
};
struct MatrixTail { float third, sixth, ninth; };
struct Determinant {
    float first, second, third, fourth, fifth, sixth, value, eighth;
};
inline float value(std::uint32_t bits) { return std::bit_cast<float>(bits); }
inline std::uint32_t low_byte(std::uint32_t previous, std::uint32_t byte) {
    return (previous & 0xffffff00u) | (byte & 255u);
}
inline std::uint32_t signed_half(std::uint32_t half) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(half))));
}
// Original six-term grouping, including the distinct first-operand NaN owner.
// The caller publishes the original two pushes before invoking the inverse.
// Their destinations lie below this matrix and cannot change its operands.
inline Determinant determinant_first(const SourcePhysicsContactMemory& memory,
    std::uint32_t frame, MatrixTail tail) {
    using prt_rotation_detail::multiply;
    using prt_rotation_detail::add;
    using prt_rotation_detail::subtract;
    const auto y = value(read(memory, frame + 0x38u));
    const auto z = value(read(memory, frame + 0x2cu));
    auto positive = multiply(tail.sixth, value(read(memory, frame + 0x24u)));
    auto negative_second = multiply(tail.sixth, value(read(memory, frame + 0x3cu)));
    auto positive_second = multiply(tail.third, value(read(memory, frame + 0x3cu)));
    auto negative_last = multiply(tail.third, value(read(memory, frame + 0x30u)));
    positive = multiply(positive, y);
    auto positive_last = multiply(tail.ninth, value(read(memory, frame + 0x30u)));
    auto negative_middle = multiply(tail.ninth, value(read(memory, frame + 0x24u)));
    positive_second = multiply(positive_second, z);
    positive = add(positive, positive_second);
    const auto first = value(read(memory, frame + 0x20u));
    positive_last = multiply(positive_last, first);
    positive = add(positive, positive_last);
    negative_second = multiply(negative_second, first);
    positive = subtract(positive, negative_second);
    negative_middle = multiply(negative_middle, z);
    positive = subtract(positive, negative_middle);
    negative_last = multiply(negative_last, y);
    return {negative_last,negative_second,negative_middle,first,z,y,
        subtract(positive, negative_last),positive_last};
}
inline Determinant determinant_second(const SourcePhysicsContactMemory& memory, std::uint32_t frame) {
    using prt_rotation_detail::multiply;
    using prt_rotation_detail::add;
    using prt_rotation_detail::subtract;
    auto sixth = value(read(memory, frame + 0x34u));
    auto third = value(read(memory, frame + 0x28u));
    const auto second = value(read(memory, frame + 0x24u));
    const auto seventh = value(read(memory, frame + 0x38u));
    const auto first = value(read(memory, frame + 0x20u));
    auto result = multiply(sixth, second);
    sixth = multiply(sixth, value(read(memory, frame + 0x3cu)));
    auto other = multiply(third, value(read(memory, frame + 0x3cu)));
    other = multiply(other, value(read(memory, frame + 0x2cu)));
    third = multiply(third, value(read(memory, frame + 0x30u)));
    result = multiply(result, seventh); result = add(result, other);
    auto ninth = value(read(memory, frame + 0x40u));
    auto last = multiply(ninth, value(read(memory, frame + 0x30u)));
    last = multiply(last, first); result = add(result, last);
    sixth = multiply(sixth, first); result = subtract(result, sixth);
    ninth = multiply(ninth, second);
    // The original has already pushed its output pointer at this read.
    ninth = multiply(ninth, value(read(memory, frame + 0x2cu)));
    result = subtract(result, ninth);
    third = multiply(third, seventh);
    return {sixth,third,ninth,second,seventh,first,subtract(result, third),last};
}

template<class Child>
void build(SourcePhysicsContactMemory& memory, const SourcePhysicsContactRecordObserver& observe,
    Continuation& out, Child&& child) {
    const auto get = [&](std::uint32_t at, unsigned width = 4) { return read(memory, at, width); };
    const auto put = [&](std::uint32_t at, std::uint32_t word, std::uint32_t site, unsigned width = 4) {
        store(memory, at, word, width, site, observe);
    };
    const auto push = [&](std::uint32_t word, std::uint32_t site) { out.stack -= 4; put(out.stack, word, site); };
    const auto pop = [&] { auto word = get(out.stack); out.stack += 4; return word; };
    const auto call = [&](std::uint32_t entry, std::uint32_t site, std::uint32_t next) {
        push(next, site); child(entry, out);
    };
    const auto publish_determinant = [&](const Determinant& result) {
        const auto lane = [](float x) { return scalar(std::bit_cast<std::uint32_t>(x)); };
        out.simd = {lane(result.first),lane(result.second),lane(result.third),lane(result.fourth),
            lane(result.fifth),lane(result.sixth),lane(result.value),lane(result.eighth)};
    };
    const auto finish = [&](bool success_byte, bool failure) {
        if (success_byte) out.eax = low_byte(out.eax, get(out.stack + 0x13u, 1));
        out.di = pop(); out.si = pop(); out.bp = pop();
        if (failure) out.eax &= 0xffffff00u;
        out.bx = pop(); out.stack += 0x200u;
        (void)pop(); out.stack += 28u;
    };
    out.stack -= 0x200u;
    push(out.bx, 0x23676u); push(out.bp, 0x23677u);
    out.bp = get(out.stack + 0x20cu);
    push(out.si, 0x2367fu); push(out.di, 0x23680u);
    out.di = get(0x515ec8u); out.eax = low_byte(out.eax, 1);
    out.bx &= 0xffffff00u; out.edx = 0;
    put(out.stack + 0x13u, 1, 0x2368fu, 1);
    if (std::int32_t(out.di) > 0) {
        out.ecx = 0;
        do {
            out.si = get(out.stack + 0x21cu);
            out.ecx = get(0x4c51d0u + out.ecx * 104u);
            if (out.ecx == out.si) out.bx = low_byte(out.bx, 1);
            ++out.edx; out.ecx = signed_half(out.edx);
        } while (std::int32_t(out.ecx) < std::int32_t(out.di));
        if (out.bx & 255u) { finish(false, false); return; }
    }
    if (std::int32_t(out.di) >= 24) { finish(false, false); return; }
    out.edx = get(out.stack + 0x21cu); out.di *= 104u;
    out.eax = get(0x4ecc14u); out.si = get(out.stack + 0x218u);
    out.bx = 0x4c51d0u + out.di;
    put(out.bx, out.edx, 0x236edu); put(0x4c51d4u + out.di, out.si, 0x236efu, 2);
    put(out.stack + 0x12u, 0, 0x236f6u, 1);
    bool live = false;
    if (out.eax) {
        out.ecx = get(out.eax + 0x80u);
        if (std::int32_t(out.ecx) > 0) {
            out.ecx = get(0x4ecc18u);
            if (out.ecx) {
                out.eax = get(out.eax + 0x84u);
                if (get(out.eax + 0x1cu) != 0xffffffffu) {
                    out.eax = get(out.eax + 4u);
                    live = out.eax == get(out.ecx + 8u);
                }
            }
        }
    }
    if (live) put(out.stack + 0x12u, 1, 0x23725u, 1);
    else {
        out.simd[0] = _mm_setzero_ps(); out.ecx = out.stack + 0x90u;
        push(out.ecx, 0x23736u); out.edx = out.stack + 0xd4u;
        push(out.edx, 0x2373eu); out.edx = signed_half(get(out.bp, 2));
        out.eax = out.stack + 0x118u;
        push(out.eax, 0x2374au); push(0x3f800000u, 0x2374bu); push(0x3f800000u, 0x23750u);
        put(out.stack + 0x74u, 0, 0x23755u); put(out.stack + 0x78u, 0, 0x2375bu);
        out.simd[0] = scalar(get(0x46a0dcu)); push(0x3f800000u, 0x23769u);
        out.ecx = out.stack + 0x78u;
        put(out.stack + 0x80u, bits(out.simd[0]), 0x23772u);
        call(0x148b20u, 0x2377bu, 0x23780u);
    }
    out.eax = get(out.bp + 0x14u); out.ecx = get(out.bp + 0x20u); out.edx = out.eax - 1u;
    out.eax = 5u - out.si;
    if (std::int32_t(out.eax) >= std::int32_t(out.edx)) out.eax = out.edx;
    out.edx = out.eax * 3u; out.eax = get(out.bp + 0x18u);
    out.eax = get(out.eax + out.edx * 4u); out.edx = signed_half(get(out.bp + 2u, 2));
    out.ecx += out.eax * 4u; out.eax = signed_half(get(out.bp + 4u, 2));
    out.si = (4u + out.eax * 3u) * out.edx;
    push(out.si, 0x237b2u); put(out.stack + 0x54u, out.ecx, 0x237b3u);
    out.ecx = get(out.stack + 0x224u); push(out.bx, 0x237beu);
    out.bx = get(out.stack + 0x22cu); put(out.stack + 0x64u, out.si, 0x237c6u);
    call(0x23520u, 0x237cau, 0x237cfu);
    out.eax = (out.eax * out.si) << 2;
    if (!out.eax) { finish(true, false); return; }
    // Whole coefficients always request a multiple of four bytes. The source
    // signed-remainder alignment sequence consequently leaves size unchanged.
    out.ecx = get(0x494338u); out.edx = out.eax + out.ecx;
    if (std::int32_t(out.edx) > 0x27000) { finish(false, true); return; }
    out.ecx &= 0x0fffffffu; out.eax = out.ecx;
    put(0x494338u, out.edx, 0x23813u);
    out.ecx += 0x49433cu;
    put(0x4c51d8u + out.di, out.eax, 0x23833u);
    out.eax = get(out.stack + 0x224u); out.edx = 0;
    put(out.stack + 0x14u, out.ecx, 0x23844u); put(out.stack + 0x18u, 0, 0x23848u);
    if (std::int32_t(out.eax) > 0) {
        out.eax = get(out.stack + 0x220u) + 4u;
        put(out.stack + 0x54u, out.eax, 0x2385cu);
        do {
            out.ecx = get(out.eax + 2u, 2); out.eax = signed_half(get(out.eax, 2));
            out.si = get(out.stack + 0x220u); out.eax *= 48u;
            put(out.stack + 0x58u, out.ecx, 0x23876u);
            out.di = 0x44u + out.eax + out.si;
            if (out.ecx) {
                auto third = get(out.di); auto last = get(out.di + 0x2cu);
                out.eax = low_byte(out.eax, get(out.stack + 0x12u, 1));
                const bool primary_live = (out.eax & 255u) != 0;
                const auto sixth = get(out.di + 0x24u), ninth = get(out.di + 0x28u);
                put(out.stack + 0x20u, third, 0x2389du);
                constexpr std::array source{4u,8u,12u,16u,20u,24u,28u};
                constexpr std::array destination{0x2cu,0x38u,0x44u,0x24u,0x30u,0x3cu,0x48u};
                constexpr std::array sites{0x238a8u,0x238b3u,0x238beu,0x238c9u,0x238d4u,0x238dfu,0x238eau};
                for (unsigned word = 0; word < source.size(); ++word)
                    put(out.stack + destination[word], get(out.di + source[word]), sites[word]);
                third = get(out.di + 0x20u); put(out.stack + 0x4cu, last, 0x238f5u);
                last = get(0x46a0d0u); put(out.stack + 0x28u, third, 0x23903u);
                put(out.stack + 0x34u, sixth, 0x23909u); put(out.stack + 0x40u, ninth, 0x2390fu);
                put(out.stack + 0x1cu, last, 0x23915u);
                out.simd[0] = scalar(third); out.simd[1] = scalar(sixth);
                out.simd[2] = scalar(ninth); out.simd[3] = scalar(last);
                MatrixTail tail{value(third), value(sixth), value(ninth)};
                if (primary_live) {
                    out.ecx = get(0x4ecc14u); out.si = get(out.ecx + 0x84u);
                    out.ecx = get(out.stack + 0x21cu); out.eax = get(out.stack + 0x22cu);
                    push(0, 0x2393bu); push(out.edx, 0x2393du);
                    out.edx = get(out.stack + 0x230u); push(out.edx, 0x23945u);
                    call(0x235e0u, 0x23946u, 0x2394bu);
                    if (out.eax) {
                        out.ecx = get(0x515ec8u) * 104u;
                        out.edx = get(out.bp, 1); out.ecx += 0x4c522cu;
                        push(out.ecx, 0x23963u); out.ecx = out.stack + 0x94u;
                        out.si += 0x38u; push(out.edx, 0x2396eu); push(out.ecx, 0x2396fu);
                        out.edx = out.stack + 0xdcu; push(out.edx, 0x23977u);
                        out.ecx = out.stack + 0x120u; push(out.ecx, 0x2397fu);
                        push(out.si, 0x23980u); out.edx = out.eax;
                        call(0x22cd0u, 0x23983u, 0x23988u);
                    }
                    tail = {value(get(out.stack + 0x28u)), value(get(out.stack + 0x34u)), value(get(out.stack + 0x40u))};
                }
                const auto determinant = determinant_first(memory, out.stack, tail);
                publish_determinant(determinant);
                out.edx = out.stack + 0x6cu; push(out.edx, 0x239e9u);
                push(out.ecx, 0x23a06u); out.eax = out.stack + 0x28u;
                put(out.stack, std::bit_cast<std::uint32_t>(determinant.value), 0x23a0fu);
                call(0x1456f0u, 0x23a14u, 0x23a19u);
                const auto rotate = [&](bool second) {
                    constexpr std::array first_push{0x23a19u,0x23a32u,0x23a4bu};
                    constexpr std::array second_push{0x23bb5u,0x23bceu,0x23be7u};
                    constexpr std::array first_call{0x23a2du,0x23a46u,0x23a5fu};
                    constexpr std::array second_call{0x23bc9u,0x23be2u,0x23bfbu};
                    constexpr std::array source_offset{0x114u,0xd4u,0x94u};
                    constexpr std::array destination_offset{0x1d4u,0x194u,0x154u};
                    for (unsigned channel = 0; channel < 3; ++channel) {
                        push(3, second ? second_push[channel] : first_push[channel]);
                        out.ecx = out.stack + source_offset[channel]; out.eax = out.stack + 0x70u;
                        out.si = out.stack + destination_offset[channel];
                        const auto site = second ? second_call[channel] : first_call[channel];
                        call(0x146100u, site, site + 5u);
                    }
                };
                rotate(false);
                out.eax = get(out.stack + 0x14u); push(out.eax, 0x23a68u);
                out.ecx = out.si; push(out.ecx, 0x23a6bu);
                out.edx = out.stack + 0x198u; push(out.edx, 0x23a73u);
                out.edx = get(out.stack + 0x5cu); out.eax = out.stack + 0x1dcu;
                push(out.eax, 0x23a7fu); out.eax = out.bp;
                call(0x23c80u, 0x23a82u, 0x23a87u);
                out.bx = get(out.stack + 0x5cu); out.ecx = get(out.stack + 0x14u);
                out.eax = get(out.stack + 0x58u); out.bx <<= 2; out.ecx += out.bx;
                put(out.stack + 0x14u, out.ecx, 0x23a9bu);
                if (std::int32_t(out.eax) > 1) {
                    out.ecx = out.di + 0x30u; out.eax = out.stack + 0x1cu;
                    call(0x23580u, 0x23aacu, 0x23ab1u);
                    const auto determinant2 = determinant_second(memory, out.stack);
                    publish_determinant(determinant2);
                    out.ecx = out.stack + 0x6cu; push(out.ecx, 0x23b0cu);
                    push(out.ecx, 0x23b2fu); out.eax = out.stack + 0x28u;
                    put(out.stack, std::bit_cast<std::uint32_t>(determinant2.value), 0x23b38u);
                    call(0x1456f0u, 0x23b3du, 0x23b42u);
                    out.eax = low_byte(out.eax, get(out.stack + 0x12u, 1));
                    if (out.eax & 255u) {
                        out.eax = get(out.stack + 0x18u); out.ecx = get(out.stack + 0x228u);
                        out.edx = get(0x4ecc14u); out.si = get(out.edx + 0x84u);
                        push(1, 0x23b61u); push(out.eax, 0x23b63u);
                        out.eax = get(out.stack + 0x234u); push(out.ecx, 0x23b6bu);
                        out.ecx = get(out.stack + 0x228u);
                        call(0x235e0u, 0x23b73u, 0x23b78u);
                        if (out.eax) {
                            out.edx = get(0x515ec8u) * 104u; out.ecx = get(out.bp, 1);
                            out.edx += 0x4c522cu; push(out.edx, 0x23b90u);
                            out.edx = out.stack + 0x94u; out.si += 0x38u;
                            push(out.ecx, 0x23b9bu); push(out.edx, 0x23b9cu);
                            out.ecx = out.stack + 0xdcu; push(out.ecx, 0x23ba4u);
                            out.edx = out.stack + 0x120u; push(out.edx, 0x23bacu);
                            push(out.si, 0x23badu); out.edx = out.eax;
                            call(0x22cd0u, 0x23bb0u, 0x23bb5u);
                        }
                    }
                    rotate(true);
                    out.si = get(out.stack + 0x14u); push(out.si, 0x23c04u);
                    out.eax = out.stack + 0x154u; push(out.eax, 0x23c0cu);
                    out.ecx = out.stack + 0x198u; push(out.ecx, 0x23c14u);
                    out.edx = out.stack + 0x1dcu; push(out.edx, 0x23c1cu);
                    out.edx = get(out.stack + 0x60u); out.eax = out.bp;
                    call(0x23c80u, 0x23c23u, 0x23c28u);
                    out.si += out.bx; put(out.stack + 0x14u, out.si, 0x23c2au);
                }
            }
            out.edx = get(out.stack + 0x18u); out.eax = get(out.stack + 0x54u);
            out.ecx = get(out.stack + 0x224u); ++out.edx; out.eax += 4u;
            put(out.stack + 0x18u, out.edx, 0x23c43u); put(out.stack + 0x54u, out.eax, 0x23c47u);
        } while (std::int32_t(out.edx) < std::int32_t(out.ecx));
    }
    put(0x515ec8u, get(0x515ec8u) + 1u, 0x23c51u);
    finish(true, false);
}

template<class State> void bridge(State& state) {
    const auto stack = state.gp[4];
    Continuation out{state.gp[0],state.gp[1],state.gp[2],state.gp[3],stack,
        state.gp[5],state.gp[6],state.gp[7],{}};
    std::copy(std::begin(state.xmm), std::end(state.xmm), out.simd.begin());
    const auto publish = [&](const Continuation& next) {
        state.gp = {next.eax,next.ecx,next.edx,next.bx,next.stack,next.bp,next.si,next.di};
        std::copy(next.simd.begin(), next.simd.end(), std::begin(state.xmm));
    };
    const auto child = [&](std::uint32_t entry, Continuation& next) {
        publish(next);
        switch (entry) {
        case 0x23520u: case 0x23580u: prt_helpers_detail::bridge(state, entry); break;
        case 0x148b20u: prt_scaled_basis_detail::bridge(state); break;
        case 0x235e0u: prt_cache_detail::bridge(state); break;
        case 0x22cd0u: prt_sample_detail::bridge(state); break;
        case 0x1456f0u: prt_matrix_inverse_detail::bridge(state); break;
        case 0x146100u: prt_rotation_detail::bridge(state); break;
        case 0x23c80u: prt_projection_detail::bridge(state); break;
        }
        next.eax=state.gp[0]; next.ecx=state.gp[1]; next.edx=state.gp[2]; next.bx=state.gp[3];
        next.stack=state.gp[4]; next.bp=state.gp[5]; next.si=state.gp[6]; next.di=state.gp[7];
        std::copy(std::begin(state.xmm),std::end(state.xmm),next.simd.begin());
    };
    build(state.memory, state.observe, out, child); publish(out);
    state.flags.plus(stack - 0x200u, 0x200u, 4);
}
}
