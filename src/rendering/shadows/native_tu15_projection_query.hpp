#pragma once
// Native TU1.5 projection-volume MOPP queries.
//
//   2E79A0  plane-volume query (44E80 frustum cull, sole call site 4520E)
//   2E7C40  sphere query        (45550 sphere cull, sole call site 456AB)
//   2E6290 / 2E6ED0  their recursive MOPP tree walkers
//   309600 / 3092A0  first/last terminal of a fully accepted subtree
//
// These are ordinary native replications of the original algorithms. The
// MOPP bytes, planes and sphere are read from the live source bank; nodes,
// quantized bounds, classifications and the two ordered output streams are
// computed by native C++ with the original x87/SSE operations, operand
// order and execution set, so results, MXCSR/x87 sticky flags and the
// x87 condition codes the translated model publishes stay identical.
//
// The walk is side-effect free; its ordered appends are committed afterwards
// to the caller's real hkArrays (direct keys at query+0, range endpoints at
// query+0x20), re-reading each header exactly like the original append. A
// full array grows through its genuine 2E3250 child at the original call
// site. Caller-clobbered EAX/ECX/EDX/EFLAGS/XMM are dead at both audited call
// continuations (45213 and 456B0) and for a native root caller (sentinel
// 60300000, e.g. the native 45550 owner), so the bridges publish only ESP,
// the callee-saved registers and the FP state. Dead frames below ESP are not
// materialized.
//
// HALO2_REFERENCE_PROJECTION_QUERY=1 keeps the translated bodies (oracle);
// HALO2_REFERENCE_PROJECTION_CULL=1 keeps only 44E80 translated.
// HALO2_NATIVE_PROJECTION_QUERY_ORACLE=1 runs native and translated on the
// same live input and compares callee-saved ABI, FP state and the caller
// frame/array memory; =strict throws on the first difference.
// HALO2_NATIVE_PROJECTION_QUERY_STATS=1 prints admission counters at exit.
#include "halo2/physics_contact_record.hpp"
#include "halo2/native_tu15_sphere_cull_45550.hpp"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
#define HALO2_NATIVE_PROJECTION_QUERY_BACKEND 1
#else
#define HALO2_NATIVE_PROJECTION_QUERY_BACKEND 0
#endif

// Offline fixture replay may count decoded opcodes/classifications; empty in builds.
#ifndef HALO2_PROJECTION_QUERY_VISIT
#define HALO2_PROJECTION_QUERY_VISIT(where, value) ((void)0)
#endif

namespace halo2::native_tu15_projection_query {

// Thrown inside the side-effect-free walk when the native route does not
// qualify (malformed program, budget). Nothing has been published except
// FP sticky flags of operations the original executes in the same order.
struct Refusal { const char *reason; };

// Read-only guest bytes. Direct committed pages are borrowed per page; every
// other address keeps the bank's virtual read (and its fault behavior).
class Guest {
public:
    explicit Guest(const SourcePhysicsContactMemory &m) : m_(m) {}
    unsigned u8(std::uint32_t a) {
        const auto base = a & ~4095u;
        if (base != base_) {
            base_ = base;
            const auto *page = m_.direct_page(base, 4096);
            page_ = page ? page->bytes : nullptr;
        }
        if (track) { low = std::min(low, a); high = std::max(high, a); }
        if (page_) return std::to_integer<unsigned>(page_[a & 4095u]);
        std::byte b{};
        m_.read(a, std::span(&b, 1));
        return std::to_integer<unsigned>(b);
    }
    std::uint32_t be(std::uint32_t a, unsigned n) {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < n; ++i) v = (v << 8) | u8(a + i);
        return v;
    }
    std::uint32_t u32(std::uint32_t a) const {
        std::array<std::byte, 4> b{};
        m_.read(a, b);
        std::uint32_t v;
        std::memcpy(&v, b.data(), 4);
        return v;
    }
    __m128 vector(std::uint32_t a) const {
        std::array<std::byte, 16> b{};
        m_.read_vector(a, b);
        __m128 v;
        std::memcpy(&v, b.data(), 16);
        return v;
    }
    // Fixture capture records the complete program extent the walk read.
    bool track = false;
    std::uint32_t low = ~0u, high = 0;
private:
    const SourcePhysicsContactMemory &m_;
    std::uint32_t base_ = 1;
    const std::byte *page_{};
};

#if HALO2_NATIVE_PROJECTION_QUERY_BACKEND
// Packed/scalar SSE with the original destination/source operand order. The
// asm is volatile so no operation is speculated, merged or commuted: MXCSR
// sticky flags and NaN payloads follow the original instruction set.
namespace sse {
inline __m128 mul(__m128 d, __m128 s) { asm volatile("mulps %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 add(__m128 d, __m128 s) { asm volatile("addps %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 sub(__m128 d, __m128 s) { asm volatile("subps %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 min(__m128 d, __m128 s) { asm volatile("minps %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 add_scalar(__m128 d, __m128 s) { asm volatile("addss %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 bits(__m128 d, __m128 s) { return _mm_and_ps(d, s); }
inline __m128 mul_scalar(__m128 d, __m128 s) { asm volatile("mulss %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 sub_scalar(__m128 d, __m128 s) { asm volatile("subss %1,%0" : "+x"(d) : "x"(s)); return d; }
inline __m128 div_scalar(__m128 d, __m128 s) { asm volatile("divss %1,%0" : "+x"(d) : "x"(s)); return d; }
// UCOMISS; LAHF; TEST AH,44h; JNP: taken only for an ordered equality.
inline bool ordered_equal(__m128 a, __m128 b) {
    unsigned char zero, parity;
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1" : "=q"(zero), "=q"(parity) : "x"(a), "x"(b) : "cc");
    return zero && !parity;
}
inline __m128 scalar(std::uint32_t bits) { return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits))); }
inline std::uint32_t lane0(__m128 v) { return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v))); }
template<int Lane> inline __m128 splat(__m128 v) { return _mm_shuffle_ps(v, v, Lane * 0x55); }
inline unsigned signs(__m128 v) { return unsigned(_mm_movemask_ps(v)); }
} // namespace sse

// x87 sequences of the original. `status` is the translated model's logical
// status word: a load clears C1, a memory store publishes its actual C1 and
// FCOMP publishes the complete hardware word. Exception flags stay live in
// the host x87 unit, exactly as the translated State reads them.
namespace x87 {
inline void store_c1(std::uint16_t &status, std::uint16_t word) {
    status = std::uint16_t((status & ~0x200u) | (word & 0x200u));
}
// FILD m32; FMUL m32; FSTP m32.
inline float bound(std::int32_t value, float scale, std::uint16_t &status) {
    float out; std::uint16_t word;
    asm volatile("fildl %2\n\tfmuls %3\n\tfstps %0\n\tfnstsw %1"
                 : "=m"(out), "=m"(word) : "m"(value), "m"(scale) : "st", "memory");
    store_c1(status, word); return out;
}
// FLD m32; FDIV m32; FSTP m32.
inline float quotient(float a, float b, std::uint16_t &status) {
    float out; std::uint16_t word;
    asm volatile("flds %2\n\tfdivs %3\n\tfstps %0\n\tfnstsw %1"
                 : "=m"(out), "=m"(word) : "m"(a), "m"(b) : "st", "memory");
    store_c1(status, word); return out;
}
// FLD m32; FMUL m32; FSTP m32.
inline float product(float a, float b, std::uint16_t &status) {
    float out; std::uint16_t word;
    asm volatile("flds %2\n\tfmuls %3\n\tfstps %0\n\tfnstsw %1"
                 : "=m"(out), "=m"(word) : "m"(a), "m"(b) : "st", "memory");
    store_c1(status, word); return out;
}
// FLD m32; FADD ST,ST; FSTP m32.
inline float doubled(float a, std::uint16_t &status) {
    float out; std::uint16_t word;
    asm volatile("flds %2\n\tfadd %%st(0),%%st\n\tfstps %0\n\tfnstsw %1"
                 : "=m"(out), "=m"(word) : "m"(a) : "st", "memory");
    store_c1(status, word); return out;
}
// FLD m32; FSTP m32 (2E79A0 plane lanes1..3). Ordinary values round-trip
// exactly with no flag and C1=0; only NaN/denormal inputs need the unit.
inline std::uint32_t copy(std::uint32_t bits, std::uint16_t &status) {
    const auto exponent = bits & 0x7f800000u, mantissa = bits & 0x007fffffu;
    if ((exponent == 0x7f800000u || exponent == 0) && mantissa) {
        std::uint32_t out; std::uint16_t word;
        asm volatile("flds %2\n\tfstps %0\n\tfnstsw %1"
                     : "=m"(out), "=m"(word) : "m"(bits) : "st", "memory");
        store_c1(status, word); return out;
    }
    status = std::uint16_t(status & ~0x200u);
    return bits;
}
// FLD m32 r; FLD ST(0); FMULP; FLD m32 s; FCOMP ST(1); FNSTSW; (FSTP ST(0)).
// Returns the hardware word after the comparison.
inline std::uint16_t compare_square(float radius, float value) {
    std::uint16_t word;
    asm volatile("flds %1\n\tfld %%st(0)\n\tfmulp\n\tflds %2\n\tfcompp\n\tfnstsw %0"
                 : "=m"(word) : "m"(radius), "m"(value) : "st", "st(1)", "st(2)", "memory");
    return word;
}
} // namespace x87
#endif

struct alignas(16) Box { std::array<float, 4> lo{}, hi{}; };
// Original 24-byte VM state: key, property0, three integer offsets, shift.
struct VmState { std::uint32_t key{}, property{}; std::array<std::uint32_t, 3> offset{}; std::uint32_t shift{}; };
enum class Stream : std::uint8_t { direct, first, last };
struct Event { std::uint32_t value; Stream stream; };
enum Result : int { outside = 0, inside = 1, partial = 2 };

#if HALO2_NATIVE_PROJECTION_QUERY_BACKEND
// 2E6290 plane-volume classification (2E6627..2E6782 and its two copies).
struct PlaneVolume {
    unsigned batches{};
    // Query +0x40 (x,y,z,doubled offset plane) and +0x240 (|x|,|y|,|z|,0).
    alignas(16) __m128 plane[8][4]{}, absolute[8][4]{};
    int classify(const Box &box, std::uint16_t &) const {
        const auto lo = _mm_load_ps(box.lo.data()), hi = _mm_load_ps(box.hi.data());
        const auto center = sse::add(lo, hi), extent = sse::sub(hi, lo);
        if (!batches) return inside;
        const auto ez = sse::splat<2>(extent), ey = sse::splat<1>(extent), ex = sse::splat<0>(extent);
        const auto cz = sse::splat<2>(center), cy = sse::splat<1>(center), cx = sse::splat<0>(center);
        unsigned all = 15;
        for (unsigned n = 0; n < batches; ++n) {
            const auto &a = absolute[n], &p = plane[n];
            const auto near_z = sse::add(sse::mul(a[2], ez), a[3]);
            const auto xx = sse::mul(a[0], ex);
            const auto yy = sse::mul(a[1], ey);
            const auto radius = sse::add(sse::add(xx, yy), near_z);
            const auto far_z = sse::add(sse::mul(p[2], cz), p[3]);
            const auto px = sse::mul(p[0], cx);
            const auto py = sse::mul(p[1], cy);
            const auto distance = sse::add(sse::add(px, py), far_z);
            // MOVMSKPS observes sign bits: -0 and negative NaN count as negative.
            if (sse::signs(sse::sub(distance, radius)) != 15) return outside;
            all &= sse::signs(sse::add(distance, radius));
        }
        return all == 15 ? inside : partial;
    }
};

// 2E6ED0 sphere classification (2E726C.., 2E734E.., 2E7524..).
struct Sphere {
    alignas(16) __m128 center{};   // Query +0x30: doubled sphere-minus-origin, lane3 2r.
    __m128 magnitude{};            // [461950]
    float radius{};                // Query +0x3C (2r)
    int classify(const Box &box, std::uint16_t &status) const {
        const auto lo = _mm_load_ps(box.lo.data()), hi = _mm_load_ps(box.hi.data());
        const auto extent = sse::sub(hi, lo);
        const auto offset = sse::bits(sse::sub(sse::add(lo, hi), center), magnitude);
        const auto nearest = sse::sub(sse::min(offset, extent), offset);
        const auto squares = sse::mul(nearest, nearest);
        const auto close = sse::add_scalar(sse::splat<2>(squares), sse::add_scalar(sse::splat<1>(squares), squares));
        const auto first = x87::compare_square(radius, _mm_cvtss_f32(close));
        if (!(first & 0x4100u)) { status = std::uint16_t(first & ~0x200u); return outside; }
        const auto farthest = sse::add(offset, extent);
        const auto far_squares = sse::mul(farthest, farthest);
        const auto far = sse::add_scalar(sse::splat<2>(far_squares), sse::add_scalar(sse::splat<1>(far_squares), far_squares));
        const auto second = x87::compare_square(radius, _mm_cvtss_f32(far));
        status = std::uint16_t(second & ~0x200u);
        // TEST AH,5 / JP: exactly one of C0/C2 is strictly inside.
        const auto parity = (second >> 8u) & 5u;
        return parity == 1u || parity == 4u ? inside : partial;
    }
};

inline unsigned operand_bytes(unsigned op) {
    if (op <= 0x0b) {
        static constexpr std::array<std::uint8_t, 12> length{1, 4, 4, 4, 4, 2, 3, 4, 0, 2, 3, 5};
        return length[op];
    }
    if (op >= 0x60 && op <= 0x6b) return op < 0x64 ? 2 : op < 0x68 ? 3 : 5;
    return 0;
}

// The recursive walker shared by 2E6290 and 2E6ED0.
template<class Volume>
class Walk {
public:
    Walk(Guest &guest, const Volume &volume, float scale, std::vector<Event> &events, std::uint16_t &status)
        : g_(guest), volume_(volume), scale_(scale), events_(events), status_(status) {}
    void run(std::uint32_t pc, const VmState &state, Box &box) { walk(pc, state, box, 0); }
    std::uint64_t steps() const { return steps_; }
private:
    Guest &g_; const Volume &volume_; float scale_; std::vector<Event> &events_; std::uint16_t &status_;
    std::uint64_t steps_{};
    void step() { if (++steps_ > (1ull << 26)) throw Refusal{"step budget"}; }
    float quantized(const VmState &s, unsigned axis, std::uint32_t value) {
        return x87::bound(std::bit_cast<std::int32_t>((value << (s.shift & 31u)) + s.offset[axis]), scale_, status_);
    }
    float absolute(std::uint32_t value) { return x87::bound(std::bit_cast<std::int32_t>(value), scale_, status_); }
    // 2E643E/2E653F prefix: consecutive slab bounds of one child.
    void slabs(std::uint32_t &pc, const VmState &s, Box &box) {
        for (;;) {
            const auto op = g_.u8(pc);
            if (op < 0x26 || op > 0x2b) return;
            HALO2_PROJECTION_QUERY_VISIT(1, op);
            step();
            if (op >= 0x29) {
                const auto axis = op - 0x29;
                box.lo[axis] = absolute(g_.be(pc + 1, 3));
                box.hi[axis] = absolute(g_.be(pc + 4, 3));
                pc += 7;
            } else {
                const auto axis = op - 0x26;
                box.lo[axis] = quantized(s, axis, g_.u8(pc + 1));
                box.hi[axis] = quantized(s, axis, g_.u8(pc + 2));
                pc += 3;
            }
        }
    }
    // 309600 (last=false) / 3092A0 (last=true).
    void endpoint(std::uint32_t pc, VmState s, bool last) {
        for (;;) {
            step();
            const auto op = g_.u8(pc);
            HALO2_PROJECTION_QUERY_VISIT(last ? 3 : 2, op);
            if (op == 0) return;
            if (op >= 0x30 && op <= 0x4f) { events_.push_back({s.key + (op - 0x30), last ? Stream::last : Stream::first}); return; }
            if (op >= 0x50 && op <= 0x53) {
                events_.push_back({s.key + g_.be(pc + 1, op - 0x4f), last ? Stream::last : Stream::first}); return;
            }
            if (op >= 1 && op <= 4) { pc += 4; continue; }
            if (op == 5) { pc += g_.u8(pc + 1) + 2; continue; }
            if (op == 6) { pc += (g_.u8(pc + 1) << 8) + g_.u8(pc + 2) + 3; continue; }
            if (op == 7) { pc += (g_.be(pc + 1, 2) << 8) + g_.u8(pc + 3) + 4; continue; }
            if (op == 9) { s.key += g_.u8(pc + 1); pc += 2; continue; }
            if (op == 0x0a) { s.key += g_.be(pc + 1, 2); pc += 3; continue; }
            if (op == 0x0b) { s.key = g_.be(pc + 1, 4); pc += 5; continue; }
            if (op >= 0x10 && op <= 0x1c) { pc += last ? g_.u8(pc + 3) + 4 : 4; continue; }
            if (op >= 0x20 && op <= 0x22) { pc += last ? g_.u8(pc + 2) + 3 : 3; continue; }
            if (op >= 0x23 && op <= 0x25) { pc += (last ? g_.be(pc + 5, 2) : g_.be(pc + 3, 2)) + 7; continue; }
            if (op >= 0x26 && op <= 0x28) { pc += 3; continue; }
            if (op >= 0x29 && op <= 0x2b) { pc += 7; continue; }
            if (op >= 0x60 && op <= 0x6b) {
                // 30977A/309739 write properties1..3 above the 309600/3092A0
                // locals, into its saved frame; property0 is a dead local.
                if ((op - 0x60) % 4u) throw Refusal{"endpoint frame property"};
                pc += operand_bytes(op); continue;
            }
            throw Refusal{"invalid endpoint opcode"};
        }
    }
    void accept(std::uint32_t pc, const VmState &s) { endpoint(pc, s, false); endpoint(pc, s, true); }
    void walk(std::uint32_t pc, VmState s, Box &box, unsigned depth) {
        if (depth > 2048) throw Refusal{"recursion budget"};
        bool local = false; // State pointer already names this frame's copy.
        for (;;) {
            step();
            const auto op = g_.u8(pc);
            HALO2_PROJECTION_QUERY_VISIT(0, op);
            if (op == 0) return;
            if (op >= 1 && op <= 4) {
                for (unsigned i = 0; i < 3; ++i) s.offset[i] += g_.u8(pc + 1 + i) << (s.shift & 31u);
                s.shift -= op; local = true; pc += 4; continue;
            }
            if (op == 5) { pc += g_.u8(pc + 1) + 2; continue; }
            if (op == 6) { pc += (g_.u8(pc + 1) << 8) + g_.u8(pc + 2) + 3; continue; }
            if (op == 7) { pc += (g_.be(pc + 1, 2) << 8) + g_.u8(pc + 3) + 4; continue; }
            if (op == 9) { s.key += g_.u8(pc + 1); local = true; pc += 2; continue; }
            if (op == 0x0a) { s.key += g_.be(pc + 1, 2); local = true; pc += 3; continue; }
            if (op == 0x0b) { s.key = g_.be(pc + 1, 4); local = true; pc += 5; continue; }
            if (op >= 0x60 && op <= 0x6b) {
                // 2E6C90..2E6D1B: properties1..3 overwrite the local integer
                // offsets only once the state is local; a first copy discards
                // them. Property0 is never consumed by this query.
                const auto bytes = operand_bytes(op), slot = (op - 0x60) % 4u;
                const auto value = g_.be(pc + 1, bytes - 1);
                if (!slot) s.property = value;
                else if (local) s.offset[slot - 1] = value;
                local = true; pc += bytes; continue;
            }
            if (op >= 0x30 && op <= 0x4f) { events_.push_back({s.key + (op - 0x30), Stream::direct}); return; }
            if (op >= 0x50 && op <= 0x53) { events_.push_back({s.key + g_.be(pc + 1, op - 0x4f), Stream::direct}); return; }
            if (op >= 0x13 && op <= 0x1c) {
                // Projected split: both children, first recursively with a copy.
                const auto skip = g_.u8(pc + 3);
                Box copy = box;
                walk(pc + 4, s, copy, depth + 1);
                pc += 4 + skip; continue;
            }
            if ((op >= 0x10 && op <= 0x12) || (op >= 0x20 && op <= 0x25)) {
                Box second = box;
                std::uint32_t first_pc, second_pc;
                if (op <= 0x12) {
                    const auto axis = op - 0x10;
                    box.hi[axis] = quantized(s, axis, g_.u8(pc + 1));
                    second.lo[axis] = quantized(s, axis, g_.u8(pc + 2));
                    first_pc = pc + 4; second_pc = pc + 4 + g_.u8(pc + 3);
                } else if (op <= 0x22) {
                    // 2E63A1..2E63BB: the right lower bound precedes the left upper.
                    const auto axis = op - 0x20, plane = g_.u8(pc + 1);
                    second.lo[axis] = quantized(s, axis, plane);
                    box.hi[axis] = quantized(s, axis, plane + 1);
                    first_pc = pc + 3; second_pc = pc + 3 + g_.u8(pc + 2);
                } else {
                    const auto axis = op - 0x23;
                    box.hi[axis] = quantized(s, axis, g_.u8(pc + 1));
                    second.lo[axis] = quantized(s, axis, g_.u8(pc + 2));
                    first_pc = pc + 7 + g_.be(pc + 3, 2); second_pc = pc + 7 + g_.be(pc + 5, 2);
                }
                slabs(first_pc, s, box);
                slabs(second_pc, s, second);
                const auto right = volume_.classify(second, status_);
                if (right == inside) accept(second_pc, s);
                const auto left = volume_.classify(box, status_);
                if (left == inside) accept(first_pc, s);
                HALO2_PROJECTION_QUERY_VISIT(4, right * 3 + left);
                if (right == partial) {
                    if (left == partial) { walk(second_pc, s, second, depth + 1); pc = first_pc; }
                    else { box = second; pc = second_pc; }
                } else if (left == partial) pc = first_pc;
                else return;
                continue;
            }
            if (op >= 0x26 && op <= 0x2b) {
                slabs(pc, s, box);
                const auto result = volume_.classify(box, status_);
                HALO2_PROJECTION_QUERY_VISIT(5, result);
                if (result == outside) return;
                if (result == inside) { accept(pc, s); return; }
                continue;
            }
            throw Refusal{"invalid opcode"};
        }
    }
};

// 2E79A0 prologue: x87 scale/bound, SoA plane banks and the doubled offset
// distances. `planes` holds raw (x,y,z,w) words in source order.
struct PlaneQueryInput {
    std::uint32_t one{}, top{};          // [46A0D0] 1.0f and [41C4E8] 2^24.
    std::array<std::uint32_t, 4> mask{}; // [461950]
    std::array<std::uint32_t, 4> origin{}; // MOPP +0 (x,y,z,scale)
    std::vector<std::array<std::uint32_t, 4>> planes;
};
struct Prepared { float scale{}; Box box; };
inline Prepared prepare_planes(const PlaneQueryInput &in, PlaneVolume &volume, std::uint16_t &status) {
    Prepared out;
    out.scale = x87::quotient(std::bit_cast<float>(in.one), std::bit_cast<float>(in.origin[3]), status);
    const auto top = x87::product(out.scale, std::bit_cast<float>(in.top), status);
    const auto count = unsigned(in.planes.size());
    const auto full = count & ~3u;
    volume.batches = 0;
    // 2E7A53..2E7ACE: four planes per batch; lane0 by integer move.
    std::array<std::array<std::array<std::uint32_t, 4>, 4>, 8> soa{};
    for (unsigned n = 0; n < full; n += 4, ++volume.batches)
        for (unsigned c = 0; c < 4; ++c) {
            soa[volume.batches][c][0] = in.planes[n][c];
            for (unsigned lane = 1; lane < 4; ++lane) soa[volume.batches][c][lane] = x87::copy(in.planes[n + lane][c], status);
        }
    if (const auto rest = count - full) {
        // 2E7AD7..2E7B5E: remaining planes, padded with the first.
        const auto &p0 = in.planes[full];
        const auto &p1 = rest >= 2 ? in.planes[full + 1] : p0;
        const auto &p2 = rest == 3 ? in.planes[full + 2] : p0;
        for (unsigned c = 0; c < 4; ++c) {
            auto &lanes = soa[volume.batches][c];
            lanes[0] = p0[c];
            lanes[3] = x87::copy(p0[c], status);
            lanes[2] = x87::copy(p2[c], status);
            lanes[1] = x87::copy(p1[c], status);
        }
        ++volume.batches;
    }
    __m128 mask, origin;
    std::memcpy(&mask, in.mask.data(), 16);
    std::memcpy(&origin, in.origin.data(), 16);
    // 2E7B81..2E7C14 runs from the last batch down; order only affects FP flags.
    for (unsigned b = volume.batches; b-- > 0;) {
        auto &p = volume.plane[b]; auto &a = volume.absolute[b];
        for (unsigned c = 0; c < 4; ++c) std::memcpy(&p[c], soa[b][c].data(), 16);
        a[0] = sse::bits(p[0], mask); a[1] = sse::bits(p[1], mask); a[2] = sse::bits(p[2], mask);
        a[3] = _mm_setzero_ps();
        const auto z = sse::mul(p[2], sse::splat<2>(origin));
        const auto y = sse::mul(p[1], sse::splat<1>(origin));
        const auto x = sse::mul(p[0], sse::splat<0>(origin));
        const auto w = sse::add(p[3], sse::add(sse::add(x, y), z));
        p[3] = sse::add(w, w);
    }
    out.box.lo = {0.f, 0.f, 0.f, 0.f};
    out.box.hi = {top, top, top, top};
    return out;
}

struct SphereQueryInput {
    std::uint32_t one{}, top{};
    std::array<std::uint32_t, 4> mask{};
    std::array<std::uint32_t, 4> origin{}; // MOPP +0
    std::array<std::uint32_t, 4> sphere{}; // center xyz, radius
};
inline Prepared prepare_sphere(const SphereQueryInput &in, Sphere &volume, std::uint16_t &status) {
    Prepared out;
    out.scale = x87::quotient(std::bit_cast<float>(in.one), std::bit_cast<float>(in.origin[3]), status);
    const auto top = x87::product(out.scale, std::bit_cast<float>(in.top), status);
    volume.radius = x87::doubled(std::bit_cast<float>(in.sphere[3]), status);
    __m128 sphere, origin;
    std::memcpy(&sphere, in.sphere.data(), 16);
    std::memcpy(&origin, in.origin.data(), 16);
    const auto relative = sse::sub(sphere, origin);
    const auto twice = sse::add(relative, relative);
    alignas(16) std::array<float, 4> lanes;
    _mm_store_ps(lanes.data(), twice);
    lanes[3] = volume.radius;
    volume.center = _mm_load_ps(lanes.data());
    std::memcpy(&volume.magnitude, in.mask.data(), 16);
    out.box.lo = {0.f, 0.f, 0.f, 0.f};
    out.box.hi = {top, top, top, top};
    return out;
}
#endif

// Typed 2E7C40 for native callers (e.g. the native 45550 owner): the same
// prepare/walk, returning the two ordered streams as host rows instead of
// appending to guest hkArrays. Call it inside the guest arithmetic
// environment (x87 control word, MXCSR and x87 exception flags loaded);
// `status` is the logical x87 status word and is updated as 2E7C40 would.
// Returns false, with no effect except the sticky FP flags of operations the
// original performs identically, when the program needs the translated owner;
// the caller then keeps its guest-frame route. A caller whose original arrays
// would grow (more rows than their inline capacity) must also keep that route
// so the genuine 2E3250 allocation happens.
struct SphereQueryRows { std::vector<std::uint32_t> direct, ranges; };
#if HALO2_NATIVE_PROJECTION_QUERY_BACKEND
inline bool sphere_query_rows(const SourcePhysicsContactMemory &memory, std::uint32_t mopp,
                              const std::array<std::uint32_t, 4> &sphere, std::uint16_t &status, SphereQueryRows &rows) {
    if (mopp & 15u) return false;
    Guest g(memory);
    SphereQueryInput in;
    in.one = g.u32(0x46a0d0u); in.top = g.u32(0x41c4e8u); in.sphere = sphere;
    for (unsigned i = 0; i < 4; ++i) { in.mask[i] = g.u32(0x461950u + 4u * i); in.origin[i] = g.u32(mopp + 4u * i); }
    std::vector<Event> events;
    auto working = status;
    Sphere volume;
    try {
        auto prepared = prepare_sphere(in, volume, working);
        Walk<Sphere>(g, volume, prepared.scale, events, working).run(mopp + 0x30u, VmState{0, 0, {0, 0, 0}, 0x10u}, prepared.box);
    } catch (const Refusal &) { return false; }
    rows.direct.clear(); rows.ranges.clear();
    for (const auto &e : events) (e.stream == Stream::direct ? rows.direct : rows.ranges).push_back(e.value);
    status = working;
    return true;
}
#endif

// ---------------------------------------------------------------- bridges
struct Stats {
    std::uint64_t plane_calls{}, plane_native{}, sphere_calls{}, sphere_native{}, declined{}, refused{};
    std::uint64_t cull_calls{}, cull_native{}, cull_linear{}, cull_frustums{}, cull_releases{}, cull_masks{};
    // Decline reasons: image/reference, MMX, x87 depth, trace, continuation, alignment/count.
    std::array<std::uint64_t, 6> reasons{};
    std::uint64_t events{}, growths{}, oracle_compared{}, oracle_mismatches{}, oracle_skipped{};
    ~Stats() {
        const auto *p = std::getenv("HALO2_NATIVE_PROJECTION_QUERY_STATS");
        if (!p || !*p) return;
        std::fprintf(stderr, "Native projection query: cull=%llu/%llu linear=%llu frustums=%llu releases=%llu masks=%llu "
            "plane=%llu/%llu sphere=%llu/%llu declined=%llu refused=%llu "
            "events=%llu growths=%llu oracle=%llu mismatches=%llu skipped=%llu\n",
            (unsigned long long)cull_native, (unsigned long long)cull_calls, (unsigned long long)cull_linear,
            (unsigned long long)cull_frustums, (unsigned long long)cull_releases, (unsigned long long)cull_masks,
            (unsigned long long)plane_native, (unsigned long long)plane_calls,
            (unsigned long long)sphere_native, (unsigned long long)sphere_calls,
            (unsigned long long)declined, (unsigned long long)refused, (unsigned long long)events,
            (unsigned long long)growths, (unsigned long long)oracle_compared,
            (unsigned long long)oracle_mismatches, (unsigned long long)oracle_skipped);
        std::fprintf(stderr, "Native projection query declines: image/reference=%llu mmx=%llu x87=%llu trace=%llu continuation=%llu layout=%llu\n",
            (unsigned long long)reasons[0], (unsigned long long)reasons[1], (unsigned long long)reasons[2],
            (unsigned long long)reasons[3], (unsigned long long)reasons[4], (unsigned long long)reasons[5]);
    }
};
inline Stats &stats() { static Stats value; return value; }
inline bool environment(const char *name) {
    const auto saved = errno; const auto *p = std::getenv(name); errno = saved; return p && *p;
}
inline bool reference() { static const bool value = environment("HALO2_REFERENCE_PROJECTION_QUERY"); return value; }
// The native 458C0 owner's own reference switch keeps 458C0 at its boundary.
inline bool environment_reference_sphere_cull() { static const bool value = environment("HALO2_REFERENCE_SPHERE_CULL"); return value; }
// Keeps only the 44E80 orchestrator translated (its 2E79A0 child stays native).
inline bool reference_cull() { static const bool value = environment("HALO2_REFERENCE_PROJECTION_CULL"); return value; }
inline int oracle_mode() {
    static const int value = [] {
        const auto saved = errno; const auto *p = std::getenv("HALO2_NATIVE_PROJECTION_QUERY_ORACLE"); errno = saved;
        if (!p || !*p) return 0;
        return std::string(p) == "strict" ? 2 : 1;
    }();
    return value;
}
inline thread_local bool translated_once = false;

struct Site { std::uint32_t call, push_size, push_array, returned, data, size; };
inline constexpr Site plane_direct{0x2e6d9eu, 0x2e6d9bu, 0x2e6d9du, 0x2e6da3u, 0x2e6dabu, 0x2e6daeu};
inline constexpr Site sphere_direct{0x2e78b0u, 0x2e78adu, 0x2e78afu, 0x2e78b5u, 0x2e78bdu, 0x2e78c0u};
inline constexpr Site range_first{0x30985bu, 0x309858u, 0x30985au, 0x309860u, 0x309868u, 0x309870u};
inline constexpr Site range_last{0x309515u, 0x309512u, 0x309514u, 0x30951au, 0x309522u, 0x30952au};

#if HALO2_NATIVE_PROJECTION_QUERY_BACKEND
template<class State>
std::uint32_t word(State &s, std::uint32_t address) {
    std::array<std::byte, 4> b{}; s.memory.read(address, b);
    std::uint32_t v; std::memcpy(&v, b.data(), 4); return v;
}
template<class State>
void store(State &s, std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
    if (s.instruction_cursor && s.instruction_cursor->position) *s.instruction_cursor->position = pc;
    std::array<std::byte, 4> bytes;
    std::memcpy(bytes.data(), &value, 4);
    SourcePhysicsContactRecordStore event{pc, address, {}, {}};
    if (s.observe) { event.before.resize(4); s.memory.read(address, event.before); }
    s.memory.write(address, bytes);
    if (s.observe) { event.written.assign(bytes.begin(), bytes.end()); s.observe(event); }
}
// Ordered hkArray<uint32> appends, each with the original header reads and
// genuine 2E3250 growth at its own call site.
template<class State, class Dispatch>
void commit(State &s, Dispatch &dispatch, std::uint32_t direct, std::uint32_t ranges,
            const std::vector<Event> &events, const Site &direct_site) {
    for (const auto &e : events) {
        const auto &site = e.stream == Stream::direct ? direct_site : e.stream == Stream::first ? range_first : range_last;
        const auto array = e.stream == Stream::direct ? direct : ranges;
        const auto capacity = word(s, array + 8u) & 0x7fffffffu;
        if (word(s, array + 4u) == capacity) {
            ++stats().growths;
            auto &g = s.gp; const auto esp = g[4];
            // The original call happens deeper in the walk; frames below the
            // entry ESP are dead, so any lower genuine stack position serves.
            g[4] = esp - 0x800u;
            s.push(site.push_size, 4u); s.push(site.push_array, array); s.push(site.call, site.returned);
            dispatch(s, 0x2e3250u, site.call, false);
            g[4] = esp;
        }
        const auto size = word(s, array + 4u), data = word(s, array);
        store(s, site.data, data + size * 4u, e.value);
        store(s, site.size, array + 4u, word(s, array + 4u) + 1u);
    }
    stats().events += events.size();
}
// Whether committing would reach a full array (oracle mode cannot restore
// a genuine allocation, so it compares only growth-free queries).
template<class State>
bool grows(State &s, std::uint32_t direct, std::uint32_t ranges, const std::vector<Event> &events) {
    std::uint64_t to_direct = 0, to_ranges = 0;
    for (const auto &e : events) (e.stream == Stream::direct ? to_direct : to_ranges) += 1;
    if (direct == ranges) { to_direct += to_ranges; to_ranges = 0; }
    const auto room = [&](std::uint32_t array) {
        return std::uint64_t(word(s, array + 8u) & 0x7fffffffu) - std::min<std::uint64_t>(word(s, array + 4u), word(s, array + 8u) & 0x7fffffffu);
    };
    return (to_direct && (word(s, direct + 4u) > (word(s, direct + 8u) & 0x7fffffffu) || to_direct > room(direct))) ||
           (to_ranges && (word(s, ranges + 4u) > (word(s, ranges + 8u) & 0x7fffffffu) || to_ranges > room(ranges)));
}
inline constexpr std::uint32_t native_root_continuation = 0x60300000u;
template<class State>
bool admissible(State &s, std::uint32_t continuation) {
    auto &r = stats().reasons;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || reference()) { ++r[0]; return false; }
    if constexpr (requires { s.active_mmx; }) if (s.active_mmx) { ++r[1]; return false; }
    if (s.fp.count > 5) { ++r[2]; return false; }
    if (const auto *c = s.instruction_cursor; c && ((c->first_trace && *c->first_trace) || (c->second_trace && *c->second_trace))) {
        ++r[3]; return false;
    }
    // The audited TU1.5 continuation, or a native root frame (PhysicsLiveWorld
    // prepare_source_call's 60300000 sentinel, e.g. the native 45550 owner),
    // whose C++ consumer reads only the arrays and the FP state.
    if (const auto returned = word(s, s.gp[4]); returned != continuation && returned != native_root_continuation) { ++r[4]; return false; }
    return true;
}
template<class State>
void first_layout_decline(State &s, const char *name, std::uint32_t a, std::uint32_t b) {
    ++stats().reasons[5];
    static bool shown = false;
    if (shown || !environment("HALO2_NATIVE_PROJECTION_QUERY_STATS")) return;
    shown = true;
    std::fprintf(stderr, "Native projection query %s layout decline: esp=%08x arg1=%08x arg2=%08x x87=%u\n",
                 name, s.gp[4], a, b, unsigned(s.fp.count));
}

// Oracle: run native, restore, run the translated body, compare.
struct Window { std::uint32_t address; std::vector<std::byte> bytes; };
using Ranges = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
template<class State>
std::vector<Window> capture(State &s, const Ranges &ranges) {
    std::vector<Window> out;
    for (auto [a, n] : ranges) { Window w{a, std::vector<std::byte>(n)}; s.memory.read(a, w.bytes); out.push_back(std::move(w)); }
    return out;
}
template<class State>
Ranges oracle_windows(State &s, std::uint32_t esp, std::uint32_t caller_frame, std::uint32_t direct, std::uint32_t ranges) {
    Ranges windows{{esp, caller_frame}};
    for (auto array : {direct, ranges}) {
        const auto data = word(s, array), capacity = word(s, array + 8u) & 0x7fffffffu;
        windows.push_back({array, 12u});
        if (capacity && capacity <= 0x100000u && (data < esp || data - esp >= caller_frame)) windows.push_back({data, capacity * 4u});
    }
    return windows;
}
struct Expected { std::array<std::vector<std::uint32_t>, 2> streams; std::uint16_t status_in{}, status_out{}, control{}; std::uint32_t mxcsr_in{}, mxcsr_out{}; };
template<class State>
std::vector<std::uint32_t> appended(State &s, std::uint32_t array, std::uint32_t before) {
    std::vector<std::uint32_t> out;
    const auto size = word(s, array + 4u), data = word(s, array);
    for (auto n = before; n < size && out.size() < 0x100000u; ++n) out.push_back(word(s, data + 4u * n));
    return out;
}
template<class State, class Commit, class Translated, class Record>
void oracle(State &s, const char *name, const Ranges &ranges, std::uint32_t direct, std::uint32_t range_array,
            Commit commit_native, Translated translated, Record record) {
    const auto before = s.save();
    const auto direct_size = word(s, direct + 4u), range_size = word(s, range_array + 4u);
    const auto memory = capture(s, ranges);
    commit_native();
    const auto native_abi = s.save();
    const auto native_memory = capture(s, ranges);
    for (const auto &w : memory) s.memory.write(w.address, w.bytes);
    s.load(before);
    translated_once = true;
    translated();
    const auto actual = s.save();
    const auto actual_memory = capture(s, ranges);
    ++stats().oracle_compared;
    std::string why;
    for (unsigned r : {3u, 4u, 5u, 6u, 7u}) if (native_abi.gp[r] != actual.gp[r]) why += " gp" + std::to_string(r);
    if (native_abi.x87_status != actual.x87_status)
        why += " x87sw " + std::to_string(native_abi.x87_status) + "/" + std::to_string(actual.x87_status);
    if (native_abi.x87_depth != actual.x87_depth) why += " x87depth";
    if (native_abi.floating_point->x87_control_word != actual.floating_point->x87_control_word) why += " cw";
    if (native_abi.floating_point->mxcsr != actual.floating_point->mxcsr)
        why += " mxcsr " + std::to_string(native_abi.floating_point->mxcsr) + "/" + std::to_string(actual.floating_point->mxcsr);
    for (std::size_t w = 0; w < memory.size(); ++w)
        if (native_memory[w].bytes != actual_memory[w].bytes) {
            std::size_t at = 0;
            while (native_memory[w].bytes[at] == actual_memory[w].bytes[at]) ++at;
            char text[64]; std::snprintf(text, sizeof text, " memory@%08x", unsigned(memory[w].address + at)); why += text;
        }
    if (!why.empty()) {
        if (++stats().oracle_mismatches <= 20) std::fprintf(stderr, "Native projection query %s oracle differs:%s\n", name, why.c_str());
        if (oracle_mode() == 2) throw std::runtime_error(std::string("Native projection query oracle differs:") + why);
        return;
    }
    Expected expected;
    expected.streams[0] = appended(s, direct, direct_size);
    expected.streams[1] = direct == range_array ? std::vector<std::uint32_t>{} : appended(s, range_array, range_size);
    expected.status_in = before.x87_status; expected.status_out = actual.x87_status;
    expected.control = before.floating_point->x87_control_word;
    expected.mxcsr_in = before.floating_point->mxcsr; expected.mxcsr_out = actual.floating_point->mxcsr;
    record(expected);
}
// HALO2_NATIVE_PROJECTION_QUERY_FIXTURES=file: append oracle-confirmed
// inputs and the translated original's streams/FP state (bounded count).
class Fixtures {
public:
    static Fixtures &get() { static Fixtures value; return value; }
    bool enabled() const { return file_ != nullptr && written_ < limit_; }
    template<class Input>
    void write(std::uint32_t kind, const Input &in, std::uint32_t code, const Guest &g, SourcePhysicsContactMemory &m, const Expected &e) {
        if (!enabled() || g.high < g.low || g.high - g.low > 0x400000u) return;
        std::vector<std::byte> bytes(g.high - g.low + 1u);
        m.read(g.low, bytes);
        put(kind); put(e.control); put(e.status_in); put(e.mxcsr_in);
        put(in.one); put(in.top);
        for (auto v : in.mask) put(v);
        for (auto v : in.origin) put(v);
        if constexpr (requires { in.planes; }) {
            put(std::uint32_t(in.planes.size()));
            for (const auto &p : in.planes) for (auto v : p) put(v);
        } else for (auto v : in.sphere) put(v);
        put(code); put(g.low); put(std::uint32_t(bytes.size()));
        std::fwrite(bytes.data(), 1, bytes.size(), file_);
        for (const auto &stream : e.streams) { put(std::uint32_t(stream.size())); for (auto v : stream) put(v); }
        put(e.status_out); put(e.mxcsr_out);
        ++written_;
    }
    ~Fixtures() { if (file_) std::fclose(file_); }
private:
    Fixtures() {
        const auto saved = errno;
        if (const auto *p = std::getenv("HALO2_NATIVE_PROJECTION_QUERY_FIXTURES"); p && *p) {
            file_ = std::fopen(p, "wb");
            if (file_) put(std::uint32_t(0x31465150u));
        }
        if (const auto *p = std::getenv("HALO2_NATIVE_PROJECTION_QUERY_FIXTURE_LIMIT"); p && *p) limit_ = std::strtoull(p, nullptr, 10);
        errno = saved;
    }
    template<class T> void put(T v) { std::fwrite(&v, sizeof v, 1, file_); }
    std::FILE *file_{};
    std::uint64_t written_{}, limit_{20000};
};
// Shared tail: publish FP status, commit appends, RET (cdecl, no operand).
template<class State, class Dispatch, class Translated, class Record>
bool finish(State &s, Dispatch &dispatch, Translated &translated, const char *name, std::uint32_t caller_frame,
            std::uint32_t direct, std::uint32_t ranges, const std::vector<Event> &events, std::uint16_t status,
            const Site &direct_site, std::uint32_t last_pc, Record record) {
    const auto esp = s.gp[4];
    const auto publish = [&] {
        s.fp.status = status;
        commit(s, dispatch, direct, ranges, events, direct_site);
        s.gp[4] = esp + 4u;
        if (s.instruction_cursor && s.instruction_cursor->position) *s.instruction_cursor->position = last_pc;
    };
    if (oracle_mode() && !s.observe) {
        if (grows(s, direct, ranges, events)) ++stats().oracle_skipped;
        else {
            oracle(s, name, oracle_windows(s, esp, caller_frame, direct, ranges), direct, ranges, publish,
                   [&] { translated(s); }, record);
            return true;
        }
    }
    publish();
    return true;
}

// 2E79A0(mopp, planes{count,ptr}, direct hkArray*, range hkArray*); cdecl.
template<class State, class Dispatch, class Translated>
bool bridge_2e79a0(State &s, Dispatch dispatch, Translated translated) {
    if (translated_once) { translated_once = false; return false; }
    ++stats().plane_calls;
    if (!admissible(s, 0x45213u)) { ++stats().declined; return false; }
    const auto esp = s.gp[4];
    const auto mopp = word(s, esp + 4u), planes = word(s, esp + 8u);
    const auto direct = word(s, esp + 12u), ranges = word(s, esp + 16u);
    const auto count = word(s, planes), first = word(s, planes + 4u);
    if ((mopp & 15u) || count > 32u) { ++stats().declined; first_layout_decline(s, "2E79A0", mopp, count); return false; }
    Guest g(s.memory);
    g.track = Fixtures::get().enabled();
    PlaneQueryInput in;
    in.one = g.u32(0x46a0d0u); in.top = g.u32(0x41c4e8u);
    for (unsigned i = 0; i < 4; ++i) { in.mask[i] = g.u32(0x461950u + 4u * i); in.origin[i] = g.u32(mopp + 4u * i); }
    in.planes.resize(count);
    for (unsigned n = 0; n < count; ++n) for (unsigned c = 0; c < 4; ++c) in.planes[n][c] = g.u32(first + 16u * n + 4u * c);
    auto status = s.fp.status;
    thread_local std::vector<Event> events; events.clear();
    thread_local PlaneVolume volume;
    try {
        auto prepared = prepare_planes(in, volume, status);
        Walk<PlaneVolume> walk(g, volume, prepared.scale, events, status);
        walk.run(mopp + 0x30u, VmState{0, 0, {0, 0, 0}, 0x10u}, prepared.box);
    } catch (const Refusal &) { ++stats().refused; return false; }
    ++stats().plane_native;
    // 44E80 frame (0x74A4) above the return address holds both inline arrays.
    return finish(s, dispatch, translated, "2E79A0", 0x7400u, direct, ranges, events, status, plane_direct, 0x2e7c3fu,
                  [&](const Expected &e) { Fixtures::get().write(0u, in, mopp + 0x30u, g, s.memory, e); });
}

// 2E7C40(mopp, sphere{x,y,z,r}, direct hkArray*, range hkArray*); cdecl.
template<class State, class Dispatch, class Translated>
bool bridge_2e7c40(State &s, Dispatch dispatch, Translated translated) {
    if (translated_once) { translated_once = false; return false; }
    ++stats().sphere_calls;
    if (!admissible(s, 0x456b0u)) { ++stats().declined; return false; }
    const auto esp = s.gp[4];
    const auto mopp = word(s, esp + 4u), sphere = word(s, esp + 8u);
    const auto direct = word(s, esp + 12u), ranges = word(s, esp + 16u);
    if ((mopp & 15u) || (sphere & 15u)) { ++stats().declined; first_layout_decline(s, "2E7C40", mopp, sphere); return false; }
    Guest g(s.memory);
    g.track = Fixtures::get().enabled();
    SphereQueryInput in;
    in.one = g.u32(0x46a0d0u); in.top = g.u32(0x41c4e8u);
    for (unsigned i = 0; i < 4; ++i) {
        in.mask[i] = g.u32(0x461950u + 4u * i); in.origin[i] = g.u32(mopp + 4u * i); in.sphere[i] = g.u32(sphere + 4u * i);
    }
    auto status = s.fp.status;
    thread_local std::vector<Event> events; events.clear();
    Sphere volume;
    try {
        auto prepared = prepare_sphere(in, volume, status);
        Walk<Sphere> walk(g, volume, prepared.scale, events, status);
        walk.run(mopp + 0x30u, VmState{0, 0, {0, 0, 0}, 0x10u}, prepared.box);
    } catch (const Refusal &) { ++stats().refused; return false; }
    ++stats().sphere_native;
    // 45550's 0x10A0-byte frame above the return address holds both arrays.
    return finish(s, dispatch, translated, "2E7C40", 0x10a0u, direct, ranges, events, status, sphere_direct, 0x2e7d02u,
                  [&](const Expected &e) { Fixtures::get().write(1u, in, mopp + 0x30u, g, s.memory, e); });
}

// 44E80 per-plane transform into the instance frame (4508A..451E7).
inline std::array<std::uint32_t, 4> instance_plane(const std::array<std::uint32_t, 4> &p, const std::array<std::uint32_t, 13> *t,
                                                   std::uint32_t one) {
    using namespace sse;
    const auto x = scalar(p[0]), y = scalar(p[1]), z = scalar(p[2]);
    __m128 nx = x, ny = y, nz = z, w = scalar(p[3]);
    if (t) {
        const auto &m = *t;
        const auto scale = scalar(m[0]), unit = scalar(one);
        if (ordered_equal(scale, _mm_setzero_ps())) w = _mm_setzero_ps();
        else {
            const auto dot = add_scalar(add_scalar(mul_scalar(scalar(m[11]), y), mul_scalar(scalar(m[12]), z)), mul_scalar(scalar(m[10]), x));
            w = sub_scalar(w, dot);
            if (!ordered_equal(scale, unit)) w = div_scalar(w, scale);
        }
        __m128 sx = x, sy = y, sz = z;
        if (!ordered_equal(scale, unit)) {
            const auto inverse = div_scalar(unit, scale);
            sx = mul_scalar(inverse, x); sy = mul_scalar(inverse, y); sz = mul_scalar(inverse, z);
        }
        nx = add_scalar(add_scalar(mul_scalar(scalar(m[3]), sz), mul_scalar(scalar(m[2]), sy)), mul_scalar(scalar(m[1]), sx));
        ny = add_scalar(add_scalar(mul_scalar(scalar(m[6]), sz), mul_scalar(scalar(m[5]), sy)), mul_scalar(scalar(m[4]), sx));
        nz = add_scalar(add_scalar(mul_scalar(scalar(m[9]), sz), mul_scalar(scalar(m[8]), sy)), mul_scalar(scalar(m[7]), sx));
    }
    return {lane0(nx), lane0(ny), lane0(nz), lane0(sub_scalar(_mm_setzero_ps(), w))};
}

inline std::uint32_t sign16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v)))); }

// 45A70(short unit, char pairs, range hkArray*, direct hkArray*): mark every
// mask bit of the accepted accelerator subparts. Sole caller 4526F (44E80).
// Header, table and bound words are reread at their original points.
template<class State>
void mark_subparts(State &s, std::uint32_t unit, std::uint32_t geometry, bool pairs, std::uint32_t ranges, std::uint32_t direct) {
    const auto mask = word(s, unit + 0x14u);
    const auto mark = [&](std::uint32_t record, std::uint32_t pc, bool rmw) {
        for (auto bit = sign16(word(s, record + 0xau));
             std::bit_cast<std::int32_t>(bit) < std::bit_cast<std::int32_t>(sign16(word(s, record + 0xcu)) + sign16(word(s, record + 0xau)));
             ++bit) {
            const auto at = mask + std::uint32_t(std::bit_cast<std::int32_t>(bit) >> 5) * 4u;
            const auto value = word(s, at) | (1u << (bit & 31u));
            (void)rmw;
            store(s, pc, at, value);
        }
    };
    if (std::bit_cast<std::int32_t>(word(s, ranges + 4u)) > 0 || std::bit_cast<std::int32_t>(word(s, direct + 4u)) > 0)
        store(s, 0x45b32u, unit, word(s, unit) | 1u);
    for (std::uint32_t i = 0; std::bit_cast<std::int32_t>(i) < std::bit_cast<std::int32_t>(word(s, ranges + 4u)); ++i) {
        const auto data = word(s, ranges);
        if (!pairs) {
            mark(word(s, geometry + 4u) + (word(s, data + i * 4u) & 0xffffu) * 0x48u, 0x45b84u, true);
            continue;
        }
        const auto second = sign16(word(s, data + i * 4u + 4u)), base = word(s, geometry), twice = word(s, geometry + 8u) << 1;
        const auto table = word(s, geometry + 0x34u);
        const auto last = word(s, table + (second + base + twice) * 2u) & 0xffffu;
        const auto first = word(s, table + (sign16(word(s, data + i * 4u)) + base + twice) * 2u) & 0xffffu;
        for (auto v = first; std::bit_cast<std::int32_t>(v) <= std::bit_cast<std::int32_t>(last); ++v) {
            const auto subpart = word(s, word(s, geometry + 0x34u) + (sign16(v) + (word(s, geometry + 8u) << 1)) * 2u) & 0xffffu;
            mark(word(s, geometry + 4u) + subpart * 0x48u, 0x45c1cu, false);
        }
        ++i;
    }
    for (std::uint32_t i = 0; std::bit_cast<std::int32_t>(i) < std::bit_cast<std::int32_t>(word(s, direct + 4u)); ++i)
        mark(word(s, geometry + 4u) + word(s, word(s, direct) + i * 4u) * 0x48u, 0x45c94u, true);
}

// 44E80(short draw unit, short cluster entry); STDCALL RET 8. The accelerated
// (MOPP) branch runs natively: draw-unit geometry selection, the 45A70/458C0
// mode, per-group frustum planes transformed into the instance frame and the
// ordered 2E79A0 queries into the original inline hkArrays of its real frame.
// 2E79A0 (native bridge), 458C0/45A70 and owned-array release keep their
// genuine call boundaries; 45A70 (sole caller) runs natively in place and
// 458C0 runs through the render-command owner's native sphere-mask kernel.
// The linear-bound branch keeps its translated body.
template<class State, class Dispatch, class Translated>
bool bridge_44e80(State &s, Dispatch dispatch, Translated translated) {
    if (translated_once) { translated_once = false; return false; }
    ++stats().cull_calls;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || reference() || reference_cull()) { ++stats().declined; return false; }
    if constexpr (requires { s.active_mmx; }) if (s.active_mmx) { ++stats().declined; return false; }
    if (s.fp.count > 5) { ++stats().declined; return false; }
    if (const auto *c = s.instruction_cursor; c && ((c->first_trace && *c->first_trace) || (c->second_trace && *c->second_trace))) {
        ++stats().declined; return false;
    }
    auto &g = s.gp;
    const auto esp0 = g[4];
    const auto index = sign16(word(s, esp0 + 4u)), entry = sign16(word(s, esp0 + 8u));
    const auto unit = (index << 5) + 0x4c6958u;
    const auto owner = word(s, unit + 4u), table = word(s, 0x4f0414u);
    std::uint32_t geometry;
    if (owner != 0xffffffffu) {
        const auto model = word(s, word(s, (owner & 0xffffu) * 16u + table + 8u) + 0x28u);
        geometry = word(s, model + ((word(s, unit + 12u) >> 9) & 0x1ffu) * 0x5cu + 0x34u);
    } else {
        const auto flags = word(s, unit), root = word(s, 0x4ecc18u), section = word(s, unit + 12u);
        if (!(flags & 0x1000u)) geometry = word(s, word(s, root + 0xa0u) + ((section >> 9) & 0x1ffu) * 0xb0u + 0x50u);
        else {
            const auto instances = word(s, root + 0x144u), definitions = word(s, root + 0x13cu);
            const auto definition = sign16(word(s, ((section >> 18) & 0x7ffu) * 0x58u + instances + 0x34u));
            geometry = word(s, definition * 0xc8u + definitions + 0x50u);
        }
    }
    const auto context = word(s, 0x4cd3a0u), set = word(s, context);
    const auto row = entry * 0x1au + set + 0xa6eu;
    const auto accelerators = word(s, geometry + 0x30u);
    if (std::bit_cast<std::int32_t>(accelerators) <= 0) { ++stats().cull_linear; return false; }
    const bool subparts = !word(s, context + 0x2a78u) && (word(s, unit) & 0x1000u) &&
                          std::bit_cast<std::int32_t>(accelerators) > std::bit_cast<std::int32_t>(word(s, geometry + 8u) << 1);
    std::uint32_t mopp = word(s, geometry + 0x2cu);
    if (subparts) mopp += (word(s, mopp + 0x20u) + 0xfu) & ~0xfu;
    ++stats().cull_native;
    const auto run = [&] {
        // PUSH EBP; AND ESP,-16; __chkstk(74A4h); PUSH EBX/ESI/EDI.
        const auto frame = ((esp0 - 4u) & ~15u) - 0x74b0u;
        const auto direct = frame + 0xb0u, ranges = frame + 0x10c0u, spare = frame + 0x20d0u, spare_wide = frame + 0x40e0u;
        // 44FA0..4502D header stores in their original order.
        store(s, 0x44fa7u, ranges, frame + 0x10ccu);
        store(s, 0x44fc0u, frame + 0x48u, frame + 0x50u);
        store(s, 0x44fc6u, spare, frame + 0x20dcu);
        store(s, 0x44fd9u, direct, frame + 0xbcu);
        store(s, 0x44fe5u, ranges + 4u, 0u);
        store(s, 0x44fecu, direct + 4u, 0u);
        store(s, 0x44ff3u, spare + 4u, 0u);
        store(s, 0x44ffau, spare_wide + 4u, 0u);
        store(s, 0x4500du, spare_wide, frame + 0x40ecu);
        store(s, 0x45018u, ranges + 8u, 0x80000400u);
        store(s, 0x4501fu, direct + 8u, 0x80000400u);
        store(s, 0x45026u, spare + 8u, 0x80000800u);
        store(s, 0x4502du, spare_wide + 8u, 0x80000800u);
        store(s, 0x45034u, frame + 0x44u, 6u);
        Guest guest(s.memory);
        const auto one = guest.u32(0x46a0d0u);
        for (std::uint32_t k = 0; std::bit_cast<std::int32_t>(k) < std::bit_cast<std::int32_t>(sign16(word(s, set))); ++k) {
            const auto counts = row + 2u + 2u * k;
            for (std::uint32_t j = 0; std::bit_cast<std::int32_t>(j) < std::bit_cast<std::int32_t>(sign16(word(s, counts))); ++j) {
                const auto frustum = (sign16(word(s, counts + 12u)) + j) * 0x108u + set + 0x1974u;
                const auto transform_at = word(s, unit + 0x1cu);
                std::array<std::uint32_t, 13> transform{};
                if (transform_at) for (unsigned n = 0; n < 13; ++n) transform[n] = guest.u32(transform_at + 4u * n);
                for (unsigned n = 0; n < 6; ++n) {
                    std::array<std::uint32_t, 4> plane;
                    for (unsigned c = 0; c < 4; ++c) plane[c] = guest.u32(frustum + 0x78u + 16u * n + 4u * c);
                    const auto local = instance_plane(plane, transform_at ? &transform : nullptr, one);
                    constexpr std::array<std::uint32_t, 4> pcs{0x451d4u, 0x451d9u, 0x451deu, 0x451e2u};
                    for (unsigned c = 0; c < 4; ++c) store(s, pcs[c], frame + 0x50u + 16u * n + 4u * c, local[c]);
                }
                ++stats().cull_frustums;
                g[4] = frame;
                s.push(0x451fbu, ranges); s.push(0x45207u, direct); s.push(0x4520cu, frame + 0x44u); s.push(0x4520du, mopp);
                s.push(0x4520eu, 0x45213u);
                dispatch(s, 0x2e79a0u, 0x4520eu, false);
                g[4] = frame;
            }
            // 4523A/45258: consume both accumulated streams for this group.
            if (subparts) mark_subparts(s, unit, geometry, true, ranges, direct);
            else if (!s.observe && !environment_reference_sphere_cull()) {
                // 458C0 through the proven native sphere-mask kernel: the
                // original reads each pair list row (plus the row after an odd
                // count) and every direct row; its own stores reach none.
                const auto rows = [&](std::uint32_t array, bool pairs) {
                    const auto count = word(s, array + 4u), data = word(s, array);
                    std::vector<std::uint32_t> out;
                    if (std::bit_cast<std::int32_t>(count) > 0) {
                        out.resize(count + (pairs && (count & 1u)));
                        for (std::uint32_t n = 0; n < out.size(); ++n) out[n] = word(s, data + 4u * n);
                    }
                    return out;
                };
                const auto pairs = rows(ranges, true), leaves = rows(direct, false);
                const auto pair_count = std::bit_cast<std::int32_t>(word(s, ranges + 4u)) > 0 ? word(s, ranges + 4u) : 0u;
                ++stats().cull_masks;
                native_tu15_sphere_mask_458c0(s.memory, index, true, pairs, pair_count, leaves);
            } else {
                s.push(0x45241u, direct); s.push(0x4524du, ranges); s.push(0x4524eu, 1u); s.push(0x45250u, index);
                s.push(0x45251u, 0x45256u); dispatch(s, 0x458c0u, 0x45251u, false);
            }
            g[4] = frame;
        }
        // 452AF..45366: release owned arrays (spare_wide, spare, direct, ranges).
        // {frame offset, PUSH 12h, PUSH bytes, PUSH data, CALL [vtable+14h]}
        constexpr std::array<std::array<std::uint32_t, 5>, 4> release{{
            {0x40e0u, 0x452c0u, 0x452c5u, 0x452cdu, 0x452ceu}, {0x20d0u, 0x452f8u, 0x452fdu, 0x45305u, 0x45306u},
            {0xb0u, 0x45330u, 0x45335u, 0x45336u, 0x45337u}, {0x10c0u, 0x45359u, 0x4535bu, 0x45363u, 0x45366u}}};
        for (const auto &r : release) {
            const auto array = frame + r[0], capacity = word(s, array + 8u);
            if (std::bit_cast<std::int32_t>(capacity) < 0) continue;
            ++stats().cull_releases;
            const auto allocator = word(s, 0x48c938u), method = word(s, word(s, allocator) + 0x14u);
            g[4] = frame;
            s.push(r[1], 0x12u); s.push(r[2], (capacity & 0x7fffffffu) << 2); s.push(r[3], word(s, array));
            s.push(r[4], r[4] + 3u);
            g[1] = allocator;
            dispatch(s, method, r[4], false);
            g[4] = frame;
        }
        g[4] = esp0 + 12u;
        if (s.instruction_cursor && s.instruction_cursor->position) *s.instruction_cursor->position = 0x4536fu;
    };
    if (oracle_mode() && !s.observe) {
        const auto frame = ((esp0 - 4u) & ~15u) - 0x74b0u;
        // Unit row, the context's 512-word mask arena and the complete frame.
        Ranges windows{{frame, esp0 + 12u - frame}, {unit, 32u}, {context + 0x25cu, 0x800u}};
        const auto mask = true;
        const auto growths = stats().growths, before = s.save();
        const auto memory = capture(s, windows);
        run();
        if (stats().growths != growths) { ++stats().oracle_skipped; return true; }
        const auto native_abi = s.save();
        const auto native_memory = capture(s, windows);
        for (const auto &w : memory) s.memory.write(w.address, w.bytes);
        s.load(before);
        translated_once = true;
        translated(s);
        const auto actual = s.save();
        const auto actual_memory = capture(s, windows);
        ++stats().oracle_compared;
        std::string why;
        for (unsigned r : {3u, 4u, 5u, 6u, 7u}) if (native_abi.gp[r] != actual.gp[r]) why += " gp" + std::to_string(r);
        if (native_abi.x87_status != actual.x87_status) why += " x87sw";
        if (native_abi.floating_point->mxcsr != actual.floating_point->mxcsr) why += " mxcsr";
        if (native_abi.floating_point->x87_control_word != actual.floating_point->x87_control_word) why += " cw";
        // The unit/mask and both inline arrays are live; the remaining frame is dead.
        if (native_memory[1].bytes != actual_memory[1].bytes) why += " unit";
        // E+44..AF: plane count/pointer and the last frustum's instance planes.
        if (std::memcmp(native_memory[0].bytes.data() + 0x44u, actual_memory[0].bytes.data() + 0x44u, 0x6cu)) why += " planes";
        if (mask && native_memory[2].bytes != actual_memory[2].bytes) why += " mask";
        for (std::uint32_t at : {0xb0u, 0x10c0u}) {
            const auto size = [&](const std::vector<std::byte> &b) { std::uint32_t v; std::memcpy(&v, b.data() + at + 4u, 4); return v; };
            const auto n = std::min<std::uint32_t>(size(actual_memory[0].bytes), 0x400u);
            if (size(native_memory[0].bytes) != size(actual_memory[0].bytes) ||
                std::memcmp(native_memory[0].bytes.data() + at, actual_memory[0].bytes.data() + at, 12u + 4u * n))
                why += " array" + std::to_string(at);
        }
        if (!why.empty()) {
            if (++stats().oracle_mismatches <= 20) std::fprintf(stderr, "Native projection query 44E80 oracle differs:%s\n", why.c_str());
            if (oracle_mode() == 2) throw std::runtime_error("Native 44E80 oracle differs:" + why);
        }
        return true;
    }
    run();
    return true;
}
#endif
} // namespace halo2::native_tu15_projection_query
