#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "pstencil/native_tu15_light_point_transform_systems.hpp"
#include "pstencil/native_tu15_light_visibility_frame.hpp"

namespace halo2::native_light_point_transform_detail {
#if defined(__x86_64__) && !defined(_MSC_VER)
template<class State, class Before>
class Adapter : public native_mopp_ray_caller_detail::Frame<State, Before> {
    using Base = native_mopp_ray_caller_detail::Frame<State, Before>;
    using Base::at; using Base::s_; using Base::word; using Base::push; using Base::pop;
    using enum native_light_visibility_detail::Register;
    std::uint32_t& reg(native_light_visibility_detail::Register r) { return s_.gp[r]; }

    std::uint32_t load(std::uint32_t pc, std::uint32_t address) {
        at(pc);
        return word(address);
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t value) {
        return _mm_set_ss(std::bit_cast<float>(load(pc, value)));
    }

    __m128 pair(std::uint32_t pc, std::uint32_t address, __m128 lower) {
        at(pc);
        alignas(8) std::array<std::byte, 8> upper;
        if(!native_raw_x87_detail::direct_read(s_.memory, address, upper)) s_.memory.read(address, upper);
        const auto half = _mm_castsi128_ps(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(upper.data())));
        return _mm_movelh_ps(lower, half);
    }

    static __m128 broadcast(__m128 scalar) { return _mm_shuffle_ps(scalar, scalar, 0); }

    void write(std::uint32_t pc, std::uint32_t address, std::uint64_t value, unsigned width) {
        at(pc);
        physics_world_runtime_detail::w(s_.memory, s_.observe, pc, address, width, value);
    }

public:
    using Base::Base;

    void read_transform(unsigned scale, unsigned x, unsigned y, unsigned z, unsigned translation) {
        using native_tu15_leaves::ordered_mul_ps;
        using native_light_point_transform::Point;
        auto& v = s_.xmm;
        push(0x1454d0, reg(ebp));
        reg(ebp) = reg(esp);
        reg(ecx) = load(0x1454d3, reg(ebp) + 8);
        const auto matrix = reg(ecx);
        v[0] = scalar(0x1454d6, matrix + x);
        v[1] = scalar(0x1454db, matrix + y);
        v[2] = scalar(0x1454e0, matrix + z);
        v[3] = scalar(0x1454e5, matrix + translation);
        v[0] = pair(0x1454ea, matrix + x + offsetof(Point, y), v[0]);
        v[1] = pair(0x1454ee, matrix + y + offsetof(Point, y), v[1]);
        v[2] = pair(0x1454f2, matrix + z + offsetof(Point, y), v[2]);
        v[3] = pair(0x1454f6, matrix + translation + offsetof(Point, y), v[3]);
        v[4] = broadcast(scalar(0x1454fa, matrix + scale));
        v[0] = ordered_mul_ps(v[0], v[4]);
        v[1] = ordered_mul_ps(v[1], v[4]);
        v[2] = ordered_mul_ps(v[2], v[4]);
    }

    std::int32_t read_count() {
        reg(ecx) = load(0x14550b, reg(ebp) + 12);
        s_.flags.minus(reg(ecx), 0, 4);
        return std::bit_cast<std::int32_t>(reg(ecx));
    }

    void begin_array() {
        reg(edx) = load(0x145513, reg(ebp) + 16);
        reg(eax) = load(0x145516, reg(ebp) + 20);
        reg(eax) = s_.flags.minus(reg(eax), reg(edx), 4);
    }

    void transform_point(unsigned x, unsigned y, unsigned z) {
        using native_tu15_leaves::ordered_add_ps;
        using native_tu15_leaves::ordered_mul_ps;
        auto& v = s_.xmm;
        const auto point = reg(edx);
        v[4] = scalar(0x14551b, point + x);
        v[5] = scalar(0x14551f, point + y);
        v[6] = scalar(0x145524, point + z);
        v[4] = ordered_mul_ps(broadcast(v[4]), v[0]);
        v[5] = ordered_mul_ps(broadcast(v[5]), v[1]);
        v[6] = ordered_mul_ps(broadcast(v[6]), v[2]);
        v[4] = ordered_add_ps(v[4], v[5]);
        v[6] = ordered_add_ps(v[6], v[3]);
        v[4] = ordered_add_ps(v[4], v[6]);
    }

    void write_point(unsigned x, unsigned y) {
        const auto point = reg(edx) + reg(eax);
        const auto words = std::bit_cast<std::array<std::uint32_t, 4>>(s_.xmm[4]);
        write(0x145547, point + x, words[0], 4);
        write(0x14554c, point + y, std::uint64_t(words[2]) | (std::uint64_t(words[3]) << 32), 8);
    }

    bool next_point(unsigned width) {
        reg(edx) = s_.flags.plus(reg(edx), width, 4);
        reg(ecx) = s_.flags.increment(reg(ecx), 4, true);
        return reg(ecx) != 0;
    }

    void finish() {
        reg(ebp) = pop(0x145557);
        pop(0x145558);
        reg(esp) += 16;
    }
};

template<class State, class Before = native_light_visibility_detail::Noop>
bool bridge(State& state, Before before = {}) {
    if(!native_light_visibility_detail::admitted(state, 0x1454d0, 0x145558)) return false;
    Adapter<State, Before> frame(state, before);
    native_light_point_transform::transform(frame);
    return true;
}
#else
template<class State, class... Args> bool bridge(State&, Args&&...) { return false; }
#endif
} // namespace halo2::native_light_point_transform_detail
