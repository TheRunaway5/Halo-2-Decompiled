#pragma once
#include "rendering/models/native_tu15_prt_interpolation.hpp"
#include "rendering/models/native_tu15_prt_scaled_basis.hpp"

namespace halo2::prt_sample_detail {
using namespace prt_projection_detail;
struct Caller {
    std::uint32_t point, stack, saved_bx, saved_bp, saved_si, saved_di;
    std::array<__m128, 8> simd;
};
struct Sample {
    std::uint32_t eax{}, ecx{}, edx{}, bx{}, bp{}, si{}, di{}, stack{};
    std::array<__m128, 8> simd;
    bool changed_c1{};
    std::uint16_t c1{};
};
// COMISS, unlike a C++ comparison, raises the original invalid exception for
// either kind of NaN. The resulting native predicate preserves unordered CF.
inline bool above(__m128 a, __m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char value;
    asm volatile("comiss %2,%1; seta %0" : "=qm"(value) : "x"(a), "x"(b) : "cc");
    return value;
#else
    return _mm_comigt_ss(a, b);
#endif
}
inline bool below(__m128 a, __m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char value;
    asm volatile("comiss %2,%1; setb %0" : "=qm"(value) : "x"(a), "x"(b) : "cc");
    return value;
#else
    return _mm_comilt_ss(a, b);
#endif
}
inline __m128 subtract(__m128 a, __m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("subss %1,%0" : "+x"(a) : "x"(b));
    return a;
#else
    return _mm_sub_ss(a, b);
#endif
}
// Complete22CD0 sampling transaction. Its native loops retain the nearest and
// opposite-probe tie rules, source SSE grouping and original observable frame
// stores. Escaped integer/SIMD values are caller transport, never an instruction
// interpreter. Fault-prefix publication is a separate qualification domain.
inline Sample sample(SourcePhysicsContactMemory& memory, const Caller& caller,
    bool backwards, const SourcePhysicsContactRecordObserver& observe) {
    Sample out{0, 0, caller.point, caller.saved_bx, caller.saved_bp,
        caller.saved_si, caller.saved_di, caller.stack - 0x2cu, caller.simd};
    auto& x = out.simd;
    const auto get = [&](std::uint32_t at, unsigned width = 4) { return read(memory, at, width); };
    const auto put = [&](std::uint32_t at, std::uint32_t value, std::uint32_t pc) {
        store(memory, at, value, 4, pc, observe);
    };
    const auto push = [&](std::uint32_t value, std::uint32_t pc) {
        out.stack -= 4u; put(out.stack, value, pc);
    };
    const auto pop = [&] { const auto value = get(out.stack); out.stack += 4u; return value; };
    const auto finish = [&] {
        out.stack += 0x2cu;
        (void)pop(); out.stack += 24u;
    };
    const auto copy = [&](std::uint32_t site) {
        while (out.ecx) {
            const auto value = get(out.si);
            put(out.di, value, site);
            const auto step = backwards ? -4u : 4u;
            out.si += step; out.di += step; --out.ecx;
        }
    };
    out.eax = get(get(0x4ecc14u) + 0x84u);
    push(out.bx, 0x22cdeu);
    out.bx = get(out.eax + 0x38u);
    push(out.si, 0x22ce4u);
    out.si = get(out.stack + 0x38u);
    put(out.stack + 0x14u, out.bx, 0x22ce9u);
    if (std::int32_t(out.bx) <= 0) {
        out.edx = get(out.stack + 0x44u);
        out.eax = get(out.stack + 0x40u);
        out.ecx = get(out.stack + 0x3cu);
        x[0] = _mm_setzero_ps();
        push(out.edx, 0x234deu);
        out.edx = get(out.stack + 0x4cu, 1);
        push(out.eax, 0x234e4u); push(out.ecx, 0x234e5u);
        push(0, 0x234e6u); push(0x3f800000u, 0x234e8u);
        put(out.stack + 0x3cu, 0, 0x234edu); put(out.stack + 0x40u, 0, 0x234f3u);
        x[0] = scalar(get(0x46a0dcu));
        push(0, 0x23501u); out.ecx = out.stack + 0x40u;
        put(out.stack + 0x48u, bits(x[0]), 0x23507u);
        push(0x23512u, 0x2350du);
        const auto child_stack = out.stack;
        const auto scaled = prt_scaled_basis_detail::evaluate_scaled(memory,
            {out.ecx, out.edx, child_stack, out.bx, out.bp, out.si, out.di}, observe);
        out.eax = (scaled.last_output & 0xffffff00u) | 1u;
        out.ecx = scaled.processed; out.edx = scaled.remaining;
        out.di = get(child_stack - 0x50u); out.si = get(child_stack - 0x4cu);
        out.bp = get(child_stack - 0x48u); out.bx = get(child_stack - 0x44u);
        x[0] = scaled.scale; x[1] = scaled.factor; x[2] = scaled.accumulated;
        x[3] = scaled.basis.z; x[4] = scaled.basis.x; x[5] = scaled.basis.y;
        if (scaled.basis.quadratic) x[6] = scaled.basis.sixth;
        x[7] = scaled.basis.half;
        out.changed_c1 = true; out.c1 = scaled.basis.final_status & 0x200u;
        (void)get(child_stack); out.stack = child_stack + 28u;
        out.si = pop(); out.bx = pop(); finish(); return out;
    }
    out.eax = (out.eax & 0xffffff00u) | get(0x515ed4u, 1);
    push(out.di, 0x22cfau);
    if (std::uint8_t(out.eax)) {
        constexpr std::array source{0x4c5160u, 0x4c5184u, 0x4c51a8u};
        constexpr std::array sites{0x234a4u, 0x234b4u, 0x234c4u};
        for (unsigned channel = 0; channel < 3; ++channel) {
            out.di = get(out.stack + 0x40u + channel * 4u);
            out.ecx = 9; out.si = source[channel]; copy(sites[channel]);
        }
        out.di = pop(); out.si = pop(); out.bx = pop(); finish(); return out;
    }
    if (out.bx == 1u) {
        const auto order = get(out.stack + 0x4cu, 1);
        out.edx = get(out.si + 4u);
        out.eax = order * order * 4u;
        out.di = get(out.stack + 0x40u);
        out.bx = out.eax; out.ecx = out.eax / 4u;
        out.si = out.edx + 12u; copy(0x22d22u); out.ecx = out.bx & 3u;
        // Squared byte orders always produce whole words; the original byte
        // tails therefore have zero iterations on every reachable input.
        out.di = get(out.stack + 0x44u); out.si = out.edx + 0x30u;
        out.ecx = out.eax / 4u; copy(0x22d37u); out.ecx = out.bx & 3u;
        out.di = get(out.stack + 0x48u); out.si = out.edx + 0x54u;
        out.ecx = out.eax / 4u; copy(0x22d4cu);
        out.ecx = out.eax & 3u; out.eax = get(out.stack + 0x50u);
        out.edx += 0xa4u; out.ecx = get(out.edx);
        put(out.eax, out.ecx, 0x22d61u); out.ecx = get(out.edx + 4u);
        out.di = pop(); put(out.eax + 4u, out.ecx, 0x22d67u);
        out.edx = get(out.edx + 8u); out.si = pop();
        put(out.eax + 8u, out.edx, 0x22d6eu); out.bx = pop(); finish(); return out;
    }

    x[4] = scalar(get(0x46a680u)); out.di = 0; out.eax = 0;
    push(out.bp, 0x22d87u); x[0] = x[4];
    const auto frame = out.stack;
    put(frame + 0x14u, 0, 0x22d8bu);
    const auto nearest_distance = [&](std::uint32_t point) {
        x[2] = scalar(get(out.edx)); x[2] = subtract(x[2], scalar(get(point)));
        x[3] = scalar(get(out.edx + 4u)); x[3] = subtract(x[3], scalar(get(point + 4u)));
        x[1] = scalar(get(out.edx + 8u)); x[1] = subtract(x[1], scalar(get(point + 8u)));
        x[5] = multiply(x[2], x[2], false);
        x[1] = multiply(x[1], x[1], false); x[2] = x[3];
        x[1] = add(x[1], x[5], false); x[2] = multiply(x[2], x[3], false);
        x[1] = add(x[1], x[2], false);
    };
    if (std::int32_t(out.bx) >= 4) {
        out.ecx = get(out.si + 4u); out.si = out.ecx + 8u;
        out.bp = out.bx - 3u; out.ecx += 0xe4u;
        do {
            for (unsigned lane = 0; lane < 4; ++lane) {
                nearest_distance(out.si - 8u + lane * 0xdcu);
                if (above(x[0], x[1])) { x[0] = x[1]; out.di = out.eax + lane; }
            }
            out.eax += 4u; out.si += 0x370u; out.ecx += 0x370u;
        } while (std::int32_t(out.eax) < std::int32_t(out.bp));
        out.si = get(frame + 0x40u); put(frame + 0x14u, out.di, 0x22ee5u);
    }
    if (std::int32_t(out.eax) < std::int32_t(out.bx)) {
        out.bp = get(out.si + 4u); out.ecx = out.eax * 0xdcu + out.bp;
        do {
            nearest_distance(out.ecx);
            if (above(x[0], x[1])) { x[0] = x[1]; out.di = out.eax; }
            ++out.eax; out.ecx += 0xdcu;
        } while (std::int32_t(out.eax) < std::int32_t(out.bx));
        put(frame + 0x14u, out.di, 0x22f4bu);
    }
    out.ecx = get(out.si + 4u);
    x[7] = scalar(get(out.edx + 8u)); x[5] = scalar(get(out.edx)); x[6] = scalar(get(out.edx + 4u));
    out.eax = out.di * 0xdcu;
    x[7] = subtract(x[7], scalar(get(out.ecx + out.eax + 8u)));
    x[5] = subtract(x[5], scalar(get(out.ecx + out.eax)));
    x[6] = subtract(x[6], scalar(get(out.ecx + out.eax + 4u)));
    out.bp = out.ecx + out.eax; out.si = 0xffffffffu; out.eax = 0;
    put(frame + 0x40u, bits(x[4]), 0x22f84u);
    put(frame + 0x20u, out.ecx, 0x22f8au);
    put(frame + 0x38u, bits(x[7]), 0x22f8eu);
    put(frame + 0x10u, out.si, 0x22f94u);
    const auto opposite = [&](std::uint32_t point, std::uint32_t index,
        std::uint32_t scratch_site, std::uint32_t distance_site, std::uint32_t index_site,
        bool reload_z, bool tail) {
        if (index == out.di) return;
        x[2] = scalar(get(out.edx + 4u)); x[3] = scalar(get(out.edx + 8u));
        x[2] = subtract(x[2], scalar(get(point + 4u)));
        x[3] = subtract(x[3], scalar(get(point + 8u)));
        x[1] = scalar(get(out.edx)); x[1] = subtract(x[1], scalar(get(point)));
        x[4] = multiply(x[2], x[6], false); x[0] = x[3];
        x[0] = multiply(x[0], reload_z ? scalar(get(frame + 0x38u)) : x[7], false);
        x[0] = add(x[0], x[4], false); x[4] = multiply(x[5], x[1], false);
        x[0] = add(x[0], x[4], false); x[4] = _mm_setzero_ps();
        const bool positive = below(x[4], x[0]);
        put(frame + 0x24u, bits(x[1]), scratch_site);
        if (positive) return;
        x[0] = x[3]; x[3] = multiply(x[2], x[2], false);
        x[0] = multiply(x[0], x[0], false); x[2] = multiply(x[1], x[1], false);
        x[1] = scalar(get(frame + 0x40u));
        x[0] = add(x[0], x[3], false); x[0] = add(x[0], x[2], false);
        if (!above(x[1], x[0])) return;
        if (tail) out.di = get(frame + 0x14u);
        put(frame + 0x40u, bits(x[0]), distance_site); put(frame + 0x10u, index, index_site);
    };
    if (std::int32_t(out.bx) >= 4) {
        out.si = out.ecx + 8u; put(frame + 0x18u, 2, 0x22fa1u); out.ecx += 0xe4u;
        do {
            opposite(out.si - 8u, out.eax, 0x22ff3u, 0x23023u, 0x23029u, false, false);
            out.bx = get(frame + 0x18u) - 1u;
            opposite(out.si - 8u + 0xdcu, out.bx, 0x2307eu, 0x230aeu, 0x230b4u, true, false);
            out.bx = get(frame + 0x18u);
            opposite(out.si - 8u + 0x1b8u, out.bx, 0x2310fu, 0x2313fu, 0x23145u, true, false);
            ++out.bx;
            opposite(out.si - 8u + 0x294u, out.bx, 0x2319du, 0x231cdu, 0x231d3u, true, false);
            out.di = get(frame + 0x18u); out.bx = get(frame + 0x1cu);
            x[7] = scalar(get(frame + 0x38u)); out.di += 4u;
            put(frame + 0x18u, out.di, 0x231e8u); out.di = out.bx - 3u;
            out.eax += 4u; out.si += 0x370u; out.ecx += 0x370u;
            const bool again = std::int32_t(out.eax) < std::int32_t(out.di);
            out.di = get(frame + 0x14u); if (!again) break;
        } while (true);
        out.ecx = get(frame + 0x20u); out.si = get(frame + 0x10u);
    }
    if (std::int32_t(out.eax) < std::int32_t(out.bx)) {
        out.si = out.eax * 0xdcu + out.ecx;
        do {
            opposite(out.si, out.eax, 0x23273u, 0x232a7u, 0x232adu, false, true);
            ++out.eax; out.si += 0xdcu;
            if (std::int32_t(out.eax) >= std::int32_t(out.bx)) break;
            x[7] = scalar(get(frame + 0x38u));
        } while (true);
        out.si = get(frame + 0x10u);
    }
    bool use_opposite = false, interpolate = false;
    if (out.si != 0xffffffffu) {
        out.si = out.si * 0xdcu + out.ecx; out.bx = out.si;
        if (out.si) {
            out.ecx = out.bx; out.eax = out.bp;
            push(0x232deu, 0x232d9u);
            const auto weight = prt_interpolation_detail::projection_weight(memory,
                out.eax, out.ecx, out.edx, out.stack, observe);
            x[0] = weight.value;
            if (weight.normalized) { x[1] = weight.dx; x[2] = weight.dy; x[3] = weight.dz; }
            x[4] = weight.nx; x[5] = weight.ny; x[6] = weight.nz;
            out.changed_c1 = true; out.c1 = 0;
            (void)pop(); x[1] = _mm_setzero_ps();
            const bool positive = below(x[1], x[0]); put(frame + 0x40u, bits(x[0]), 0x232e4u);
            if (positive) { interpolate = below(x[0], scalar(get(0x46a0d0u))); use_opposite = !interpolate; }
        }
    }
    if (!interpolate) {
        out.eax = get(frame + 0x50u, 1); out.di = get(frame + 0x44u);
        out.eax = out.eax * out.eax * 4u; out.edx = out.eax;
        const auto record = use_opposite ? out.bx : out.bp;
        constexpr std::array nearest_sites{0x23305u, 0x2331au, 0x2332fu};
        constexpr std::array opposite_sites{0x2337fu, 0x23394u, 0x233a9u};
        constexpr std::array offsets{12u, 0x30u, 0x54u};
        for (unsigned channel = 0; channel < 3; ++channel) {
            if (channel) out.di = get(frame + 0x44u + channel * 4u);
            out.si = record + offsets[channel]; out.ecx = out.eax / 4u;
            copy(use_opposite ? opposite_sites[channel] : nearest_sites[channel]);
            out.ecx = channel == 2 ? out.eax & 3u : out.edx & 3u;
        }
        out.eax = get(frame + 0x54u);
        if (use_opposite) {
            out.bx += 0xa4u; out.ecx = get(out.bx); put(out.eax, out.ecx, 0x233beu);
            out.edx = get(out.bx + 4u); out.bp = pop(); out.di = pop();
            put(out.eax + 4u, out.edx, 0x233c5u); out.ecx = get(out.bx + 8u);
            out.si = pop(); put(out.eax + 8u, out.ecx, 0x233ccu);
        } else {
            out.bp += 0xa4u; out.ecx = get(out.bp); put(out.eax, out.ecx, 0x23345u);
            out.edx = get(out.bp + 4u); put(out.eax + 4u, out.edx, 0x2334au);
            out.ecx = get(out.bp + 8u); out.bp = pop(); out.di = pop(); out.si = pop();
            put(out.eax + 8u, out.ecx, 0x23353u);
        }
        out.bx = pop(); finish(); return out;
    }
    out.edx = get(frame + 0x44u); out.si = get(frame + 0x50u, 1);
    constexpr std::array output_sites{0x233dfu, 0x233f8u, 0x23411u};
    constexpr std::array reference_sites{0x233e3u, 0x233ffu, 0x23418u};
    constexpr std::array call_sites{0x233e9u, 0x23402u, 0x2341bu};
    constexpr std::array returns{0x233eeu, 0x23407u, 0x23420u};
    constexpr std::array channel_offsets{12u, 0x30u, 0x54u};
    for (unsigned channel = 0; channel < 3; ++channel) {
        if (channel) {
            const auto output = get(frame + 0x44u + channel * 4u);
            if (channel == 1) out.ecx = output; else out.eax = output;
            x[0] = scalar(get(frame + 0x40u)); push(output, output_sites[channel]);
            out.edx = out.bp + channel_offsets[channel];
        } else {
            push(out.edx, output_sites[channel]); out.eax = out.bp + 12u;
        }
        out.ecx = out.bx + channel_offsets[channel];
        push(channel ? out.edx : out.eax, reference_sites[channel]); out.eax = out.si;
        push(returns[channel], call_sites[channel]);
        const auto child_stack = out.stack;
        const auto coefficients = prt_interpolation_detail::interpolate_coefficients(memory,
            {out.eax, out.ecx, child_stack, out.bx, out.si, out.di, out.bp, x[0]}, observe);
        out.eax = coefficients.reference_cursor; out.ecx = coefficients.output_cursor;
        out.edx = coefficients.processed; if (coefficients.bulk) out.bp = coefficients.restored_bp;
        out.di = get(child_stack - 20u); out.si = get(child_stack - 16u); out.bx = get(child_stack - 12u);
        if (coefficients.written) x[1] = coefficients.value;
        (void)get(child_stack); out.stack = child_stack + 12u;
    }
    constexpr std::array incident_sites{0x23446u, 0x23466u, 0x2348au};
    for (unsigned component = 0; component < 3; ++component) {
        const auto offset = 0xa4u + component * 4u;
        x[1] = scalar(get(out.bx + offset)); x[1] = subtract(x[1], scalar(get(out.bp + offset)));
        if (!component) { x[0] = scalar(get(frame + 0x40u)); out.eax = get(frame + 0x54u); }
        x[1] = multiply(x[1], x[0], false); x[1] = add(x[1], scalar(get(out.bp + offset)), false);
        if (component == 2) { out.bp = pop(); out.di = pop(); out.si = pop(); }
        put(out.eax + component * 4u, bits(x[1]), incident_sites[component]);
    }
    out.bx = pop(); finish(); return out;
}
template<class State> void bridge(State& state) {
    Caller caller{state.gp[2], state.gp[4], state.gp[3], state.gp[5], state.gp[6], state.gp[7], {}};
    std::copy_n(state.xmm, 8, caller.simd.begin());
    const auto result = sample(state.memory, caller, state.other_flags & 1024u, state.observe);
    state.gp[0] = result.eax; state.gp[1] = result.ecx; state.gp[2] = result.edx;
    state.gp[3] = result.bx; state.gp[4] = result.stack; state.gp[5] = result.bp;
    state.gp[6] = result.si; state.gp[7] = result.di;
    std::copy(result.simd.begin(), result.simd.end(), state.xmm);
    if (result.changed_c1) state.fp.status = (state.fp.status & ~0x200u) | result.c1;
    state.flags.plus(caller.stack - 0x2cu, 0x2cu, 4);
}
}
