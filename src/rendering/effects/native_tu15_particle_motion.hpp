#pragma once
#include "rendering/effects/native_tu15_effect_owners.hpp"
#include "rendering/effects/native_tu15_particle_motion_systems.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"

namespace halo2::native_particle_motion_detail {
namespace motion = native_effects::particle_motion;
using namespace native_object_tick_detail::boundary;
using native_effect_owner_detail::scalar;
using native_effects::location_update::ParticleDatum;
using U = std::uint32_t;

struct Summary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{}, coverage{};
    ~Summary() {
        if (calls)
            std::fprintf(stderr,
                         "Particle motion verify: %llu calls, %llu accesses, %llu child boundaries identical\n",
                         (unsigned long long)calls.load(), (unsigned long long)accesses.load(),
                         (unsigned long long)children.load());
    }
};
inline Summary &summary() {
    static Summary value;
    return value;
}

// Original calling convention and locals for 2C2AD0. All bank operations
// use the original site, including the stack accesses between child calls.
// Registers belong to the live state so interrupted accesses and children
// expose the state reached at that boundary.
template <class State, class Route, class Decode> class Frame {
    State &s;
    Port<State> io;
    Route route;
    Decode decode;
    decltype(s.gp) &g;
    decltype(s.flags) &flags;
    U read(U pc, U address, unsigned width = 4) { return io.read(pc, address, width); }
    U local(U pc, U offset) { return read(pc, g[esp] + offset); }
    void save(U pc, U offset, U value) { io.store(pc, g[esp] + offset, 4, value); }
    void push(U pc, U value) {
        g[esp] -= 4;
        io.store(pc, g[esp], 4, value);
    }
    U pop(U pc) {
        const U v = local(pc, 0);
        g[esp] += 4;
        return v;
    }
    void ret(U pc) {
        pop(pc);
        g[esp] += 12;
    }
    void call(U pc, U next, U target) {
        push(pc, next);
        io.gp = g;
        io.flags = flags;
        io.enter(target, [&](State &v) { route(v, target, pc); });
    }
    void load_scalar(unsigned lane, U pc, U address) {
        s.xmm[lane] = scalar(std::bit_cast<float>(read(pc, address)));
    }
    void store_scalar(unsigned lane, U pc, U address) {
        io.store(pc, address, 4, std::bit_cast<U>(_mm_cvtss_f32(s.xmm[lane])));
    }
    bool advance_channel(unsigned count) {
        g[ebp] = flags.increment(g[ebp], 4, false);
        g[esi] = flags.plus(g[esi], 0x14u, 4);
        flags.minus(g[ebp], count, 4);
        return flags.cf;
    }

  public:
    std::uint64_t hit{};
    Frame(State &state, U *cursor, native_object_tick_detail::Trace<State> *trace, Route r, Decode d)
        : s(state), io(state, cursor, trace, {}), route(r), decode(d), g(s.gp), flags(s.flags) {}
    void load() {} // State is already live.
    void run() { motion::update(*this); }
    motion::Movement select_movement() {
        using enum motion::Movement;
        g[edx] = read(0x2c2ad0, g[eax], 2);
        flags.minus(g[edx], 3, 4);
        if (g[edx] > 3)
            return none;
        // Read the authored jump table even when the movement kind is known.
        const auto target = decode(read(0x2c2ad8, 0x2c2b30 + 4 * g[edx]), s.image);
        switch (target) {
        case 0x2c2adf:
            return gravity_drag;
        case 0x2c2af4:
            return second;
        case 0x2c2b09:
            return third;
        case 0x2c2b1f:
            return fourth;
        default:
            throw std::runtime_error("Required original solver dispatch target unavailable");
        }
    }
    void dispatch_other(motion::Movement kind) {
        using enum motion::Movement;
        switch (kind) {
        case second:
            g[edx] = local(0x2c2af4, 12);
            push(0x2c2af8, g[edx]);
            g[edx] = local(0x2c2af9, 12);
            push(0x2c2afd, g[edx]);
            push(0x2c2afe, g[esi]);
            push(0x2c2aff, g[ecx]);
            push(0x2c2b00, g[eax]);
            call(0x2c2b01, 0x2c2b06, 0x2c2d90);
            ret(0x2c2b06);
            return;
        case third:
            g[edx] = local(0x2c2b09, 8);
            push(0x2c2b0d, g[edx]);
            g[edx] = local(0x2c2b0e, 8);
            push(0x2c2b12, g[esi]);
            push(0x2c2b13, g[edx]);
            push(0x2c2b14, g[eax]);
            g[edx] = g[ecx];
            call(0x2c2b17, 0x2c2b1c, 0x2c3840);
            ret(0x2c2b1c);
            return;
        case fourth:
            g[edx] = local(0x2c2b1f, 8);
            push(0x2c2b23, g[edx]);
            push(0x2c2b24, g[esi]);
            push(0x2c2b25, g[ecx]);
            push(0x2c2b26, g[eax]);
            call(0x2c2b27, 0x2c2b2c, 0x2c3e30);
            break;
        default:
            break;
        }
        ret(0x2c2b2c);
    }
    void begin_gravity_drag() {
        g[edx] = local(0x2c2adf, 8);
        save(0x2c2ae3, 12, g[edx]);
        save(0x2c2ae7, 8, g[esi]);
        save(0x2c2aeb, 4, g[eax]);
        g[esp] = flags.minus(g[esp], 0x6c, 4);
        g[eax] = local(0x2c2b43, 0x74);
        g[edx] = local(0x2c2b47, 0x70);
        s.xmm[0] = _mm_setzero_ps();
        push(0x2c2b4e, g[ebx]);
        g[ebx] = read(0x2c2b4f, g[edx] + 12);
        push(0x2c2b52, g[ebp]);
        g[ebp] = flags.logic(0, 4);
        push(0x2c2b55, g[esi]);
        save(0x2c2b56, 0x10, g[eax]);
        g[eax] = flags.logic(0, 4);
        flags.minus(g[ecx], g[ebp], 4);
        push(0x2c2b5e, g[edi]);
        g[edi] = read(0x2c2b5f, g[edx] + 8);
        save(0x2c2b62, 0x6c, 0);
        save(0x2c2b66, 0x70, 0);
        save(0x2c2b6a, 0x74, 0);
        save(0x2c2b6e, 0x78, 0);
        save(0x2c2b72, 0x1c, 0);
        save(0x2c2b78, 0x20, 0);
        save(0x2c2b7e, 0x24, 0);
        save(0x2c2b84, 0x10, g[ebx]);
        if (!flags.zf) {
            g[eax] = g[ecx];
            save(0x2c2b8c, 0x70, g[eax]);
            save(0x2c2b90, 0x6c, g[ebp]);
        }
    }
    void sample_shared_inputs() {
        g[esi] = read(0x2c2b94, g[edx] + 16);
        g[ecx] = g[esp] + 0x28;
        push(0x2c2b9b, g[ecx]);
        push(0x2c2b9c, g[ebp]);
        push(0x2c2b9d, g[ebp]);
        push(0x2c2b9e, g[eax]);
        g[esi] = flags.logic(g[esi] & motion::shared_inputs, 4);
        push(0x2c2ba5, g[esi]);
        call(0x2c2ba6, 0x2c2bab, 0x176a40);
        const U mask = local(0x2c2bab, 0x6c);
        save(0x2c2bab, 0x6c, flags.logic(mask | g[esi], 4));
        g[eax] = g[edi] + 4;
        save(0x2c2bb2, 0x18, g[eax]);
        g[esi] = g[eax];
    }
    bool constant_channels_empty() {
        flags.logic(g[ebx], 4);
        return flags.zf;
    }
    bool constant_channel_selected() {
        g[edi] = 1;
        g[ecx] = g[ebp];
        g[edi] = flags.shift(g[edi], g[ecx] & 255, 4);
        flags.logic(g[ebx] & g[edi], 4);
        return !flags.zf;
    }
    void sample_constant_channel() {
        g[ebx] = g[esp] + 0x28;
        call(0x2c2bcd, 0x2c2bd2, 0x24ffe0);
        g[eax] = local(0x2c2bd2, 0x10);
        g[edi] = ~g[edi];
        g[eax] = flags.logic(g[eax] & g[edi], 4);
        store_scalar(0, 0x2c2bda, g[esp] + 0x1c + 4 * g[ebp]);
        save(0x2c2be0, 0x10, g[eax]);
        g[ebx] = g[eax];
    }
    bool next_constant_channel(unsigned count) { return advance_channel(count); }
    bool has_particles() {
        flags.minus(local(0x2c2bef, 0x84), 0xffffffffu, 4);
        return !flags.zf;
    }
    void select_particle() {
        g[edi] = local(0x2c2c00, 0x14);
        g[edx] = read(0x2c2c04, native_effects::location_update::globals::particles);
        g[ecx] = read(0x2c2c0a, g[edx] + 0x44);
        g[edi] = flags.logic(g[edi] & 0xffff, 4);
        g[edi] = flags.shift(g[edi], 6, 4);
        g[eax] = read(0x2c2c16, g[edi] + g[ecx] + offsetof(ParticleDatum, next));
        g[edi] = flags.plus(g[edi], g[ecx], 4);
        save(0x2c2c1c, 0x14, g[eax]);
    }
    bool inactive(std::uint8_t mask) {
        flags.logic(read(0x2c2c20, g[edi] + offsetof(ParticleDatum, flags), 1) & mask, 1);
        return !flags.zf;
    }
    bool past_lifetime() {
        load_scalar(0, 0x2c2c2a, native_effects::location_update::globals::one);
        const float age = std::bit_cast<float>(read(0x2c2c32, g[edi] + offsetof(ParticleDatum, age)));
        flags.scalar_compare(s.xmm[0], scalar(age), false);
        return flags.cf;
    }
    void refresh_particle_inputs() {
        g[eax] = local(0x2c2c3c, 0x78);
        flags.minus(g[edi], g[eax], 4);
        if (!flags.zf) {
            g[ecx] = local(0x2c2c44, 0x6c);
            g[eax] = g[edi];
            g[ecx] = flags.logic(g[ecx] & motion::reusable_inputs, 4);
            save(0x2c2c50, 0x78, g[eax]);
            save(0x2c2c54, 0x6c, g[ecx]);
        }
        g[ecx] = local(0x2c2c58, 0x80);
        g[esi] = read(0x2c2c5f, g[ecx] + 16);
        g[ecx] = local(0x2c2c62, 0x70);
        g[edx] = g[esp] + 0x28;
        push(0x2c2c6a, g[edx]);
        g[edx] = local(0x2c2c6b, 0x70);
        push(0x2c2c6f, g[eax]);
        g[eax] = local(0x2c2c70, 0x7c);
        push(0x2c2c74, g[eax]);
        g[edx] = ~g[edx];
        g[esi] = flags.logic(g[esi] & motion::particle_inputs, 4);
        push(0x2c2c7d, g[ecx]);
        g[edx] = flags.logic(g[edx] & g[esi], 4);
        push(0x2c2c80, g[edx]);
        call(0x2c2c81, 0x2c2c86, 0x176a40);
        g[eax] = local(0x2c2c86, 0x6c);
        g[eax] = flags.logic(g[eax] | g[esi], 4);
        g[esi] = local(0x2c2c8c, 0x18);
        save(0x2c2c90, 0x6c, g[eax]);
        g[ebp] = flags.logic(0, 4);
    }
    bool channel_is_constant() {
        g[ecx] = g[ebp];
        g[eax] = flags.shift(1, g[ecx] & 255, 4);
        g[ecx] = local(0x2c2c9f, 0x80);
        flags.logic(read(0x2c2ca6, g[ecx] + 12) & g[eax], 4);
        return !flags.zf;
    }
    void sample_particle_channel() {
        g[ebx] = g[esp] + 0x28;
        call(0x2c2caf, 0x2c2cb4, 0x24ffe0);
        store_scalar(0, 0x2c2cb4, g[esp] + 0x1c + 4 * g[ebp]);
    }
    bool next_particle_channel(unsigned count) { return advance_channel(count); }
    void publish_scalar(motion::Scalar role, float value) {
        // XMM0..4, respectively, carry these temporaries in the source.
        s.xmm[static_cast<unsigned>(role)] = scalar(value);
    }
    bool greater(float left, float right) {
        flags.scalar_compare(scalar(left), scalar(right), false);
        return !flags.cf && !flags.zf;
    }
    float load(motion::Access role) {
        using enum motion::Access;
        switch (role) {
        case step:
            load_scalar(4, 0x2c2cc3, g[esp] + 0x88);
            return _mm_cvtss_f32(s.xmm[4]);
        case linear_drag:
            load_scalar(1, 0x2c2ccc, g[esp] + 0x20);
            return _mm_cvtss_f32(s.xmm[1]);
        case angular_drag:
            load_scalar(2, 0x2c2cd5, g[esp] + 0x24);
            return _mm_cvtss_f32(s.xmm[2]);
        case one:
            load_scalar(3, 0x2c2cdb, native_effects::location_update::globals::one);
            return _mm_cvtss_f32(s.xmm[3]);
        case gravity_scale:
            g[edx] = read(0x2c2d0f, 0x52be68);
            load_scalar(0, 0x2c2d15, g[edx]);
            return _mm_cvtss_f32(s.xmm[0]);
        case gravity_channel:
            return std::bit_cast<float>(local(0x2c2d19, 0x1c));
        case vertical_velocity:
            load_scalar(4, 0x2c2d23, g[edi] + offsetof(ParticleDatum, velocity) + 8);
            return _mm_cvtss_f32(s.xmm[4]);
        case spin_rate:
            return std::bit_cast<float>(read(0x2c2d66, g[edi] + offsetof(ParticleDatum, rotation_rate)));
        }
        throw std::logic_error("Unknown particle motion access");
    }
    void store_accelerated(float value) {
        io.store(0x2c2d2c, g[edi] + offsetof(ParticleDatum, velocity) + 8, 4, std::bit_cast<U>(value));
    }
    float load_velocity(unsigned axis) {
        constexpr U sites[] = {0x2c2d38, 0x2c2d46, 0x2c2d54};
        load_scalar(1, sites[axis], g[edi] + offsetof(ParticleDatum, velocity) + 4 * axis);
        return _mm_cvtss_f32(s.xmm[1]);
    }
    void store_velocity(unsigned axis, float value) {
        constexpr U sites[] = {0x2c2d41, 0x2c2d4f, 0x2c2d5d};
        io.store(sites[axis], g[edi] + offsetof(ParticleDatum, velocity) + 4 * axis, 4,
                 std::bit_cast<U>(value));
    }
    void store_spin(float value) {
        io.store(0x2c2d6b, g[edi] + offsetof(ParticleDatum, rotation_rate), 4, std::bit_cast<U>(value));
    }
    bool has_next_particle() {
        flags.minus(local(0x2c2d70, 0x14), 0xffffffffu, 4);
        return !flags.zf;
    }
    void finish_gravity_drag() {
        g[edi] = pop(0x2c2d7b);
        g[esi] = pop(0x2c2d7c);
        g[ebp] = pop(0x2c2d7d);
        g[ebx] = pop(0x2c2d7e);
        g[esp] = flags.plus(g[esp], 0x6c, 4);
        ret(0x2c2d82);
    }
};

template <class State, class Reference, class Second, class Inputs, class Channel, class Dispatch, class Decode>
bool bridge_2c2ad0(State &s, Reference reference, Second second, Inputs inputs, Channel channel,
                   Dispatch dispatch, Decode decode) {
    auto route = [&](State &v, U target, U site) {
        switch (target) {
        case 0x2c2d90:
            second(v);
            return;
        case 0x176a40:
            inputs(v);
            return;
        case 0x24ffe0:
            channel(v);
            return;
        default:
            dispatch(v, target, site);
            return;
        }
    };
    if (!native_effect_owner_detail::backend || !native_effect_owner_detail::masked_controls() ||
        !native_required_owner_policy_detail::preserves_instruction_policy(s.services))
        return false;
    if constexpr (requires { s.active_mmx; })
        if (s.active_mmx)
            return false;
    if (!s.services.instruction && (s.instruction_cursor || s.literal_service_sites || s.observe))
        return false;
    U *cursor;
    if (!native_object_tick_detail::admitted(s, 0x2c2ad0, 0x2c2d82, cursor,
                                             native_effect_owner_detail::owner_reference(0x2c2ad0)))
        return false;
    if (cursor) {
        const auto *policy = s.live_instruction_cursor();
        if (!policy || !policy->literal_service_sites || policy->service_sites.size() != 3 ||
            policy->service_sites[0] != 0x38846e || policy->service_sites[1] != 0x21f590 ||
            policy->service_sites[2] != 0x33b595)
            return false;
    }
    auto &stats = summary();
    native_object_tick_detail::execute(s, cursor, native_effect_owner_detail::verify_mode(), stats.coverage,
                                       stats.calls, stats, reference,
                                       [&](State &v, U *c, native_object_tick_detail::Trace<State> *t) {
                                           return Frame<State, decltype(route), Decode>{v, c, t, route, decode};
                                       });
    return true;
}
} // namespace halo2::native_particle_motion_detail
