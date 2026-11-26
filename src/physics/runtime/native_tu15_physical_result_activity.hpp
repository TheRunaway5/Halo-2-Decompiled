#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_physical_result_activity {
using namespace native_object_tick_detail::boundary;
using physics_step_detail::r;
using physics_step_detail::w;

enum class Continuation { tick, blocked, reference };
enum class Alternate { apply_impulse, decay, publish, reference };

inline __m128 projection_difference(__m128 left, __m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("subss %1,%0" : "+x"(left) : "x"(right));
    return left;
#else
    return _mm_sub_ss(left, right);
#endif
}

inline __m128 contact_division(__m128 left, __m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("divss %1,%0" : "+x"(left) : "x"(right));
    return left;
#else
    return _mm_div_ss(left, right);
#endif
}

template<class State> void change_activity_byte(State &s, std::uint32_t pc, std::uint32_t offset,
                                               std::uint32_t mask, bool set) {
    const auto address = s.gp[ebp] + offset;
    const auto previous = r(s.memory, address, 1);
    auto flags = s.flags;
    const auto value = flags.logic(set ? previous | mask : previous & mask, 1);
    if (s.observe) {
        const SourcePhysicsContactRecordObserver after = [&](const SourcePhysicsContactRecordStore &store) {
            s.flags = flags;
            s.observe(store);
        };
        w(s.memory, after, pc, address, 1, value);
    } else {
        w(s.memory, {}, pc, address, 1, value);
        s.flags = flags;
    }
}

template<class State> void clear_activity_flag(State &s) {
    change_activity_byte(s, 0xe4a92u, 0x348u, 0xefu, false);
}

template<class State, class Route> class ResultFrame {
    State &s;
    Route &route;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;

    void at(std::uint32_t pc) { s.instruction_literal(pc); }
    std::uint32_t stack(std::uint32_t offset) const { return g[esp] + offset; }
    std::uint32_t object(std::uint32_t offset) const { return g[ebp] + offset; }
    std::uint32_t low(unsigned reg) const { return g[reg] & 0xffu; }
    void set_low(unsigned reg, std::uint32_t value) { s.set(reg, value, 1, 0); }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(physics_step_detail::f(read(pc, address)));
    }
    std::uint32_t offset(std::uint32_t pc, std::uint32_t address) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(pc, address, 2)))));
    }
    void store(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        w(s.memory, s.observe, pc, address, width, value);
    }
    void push(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        native_two_bone_solve::push_word(s, pc, value);
    }
    void call(std::uint32_t pc, std::uint32_t resume, std::uint32_t target) {
        push(pc, resume);
        route(s, pc, target);
    }
    bool zero(std::uint32_t pc) {
        at(pc);
        return flags.zf;
    }
    void test_byte(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        flags.logic(value, 1);
    }

    void setup() {
        at(0xe4a40u);
        g[esp] = flags.minus(g[esp], 0xd0u, 4);
        g[ecx] = read(0xe4a46u, 0x4ecbd0u);
        g[edx] = read(0xe4a4cu, g[ecx] + 0x44u);
        g[ecx] = read(0xe4a4fu, 0x4f0414u);
        push(0xe4a55u, g[ebx]);
        push(0xe4a56u, g[ebp]);
        push(0xe4a57u, g[esi]);
        g[esi] = read(0xe4a58u, stack(0xe0u));
        at(0xe4a5fu);
        g[eax] = g[esi];
        at(0xe4a61u);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        at(0xe4a66u);
        g[eax] *= 3u;
        at(0xe4a69u);
        g[eax] = flags.shift(g[eax], 2, 4);
        g[ebp] = read(0xe4a6cu, g[edx] + g[eax] + 8u);
        g[ebx] = offset(0xe4a70u, object(0x12au));
        store(0xe4a77u, stack(0x10u), g[eax]);
        g[eax] = read(0xe4a7bu, object(0));
        push(0xe4a7eu, g[edi]);
        g[edi] = offset(0xe4a7fu, object(0x33eu));
        at(0xe4a86u);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        at(0xe4a8bu);
        g[eax] = flags.shift(g[eax], 4, 4);
        g[edx] = read(0xe4a8eu, g[eax] + g[ecx] + 8u);
        at(0xe4a92u);
        clear_activity_flag(s);
        at(0xe4a99u);
        g[ebx] = flags.plus(g[ebx], g[ebp], 4);
        store(0xe4a9bu, stack(0x2cu), g[edx]);
        at(0xe4a9fu);
        g[edi] = flags.plus(g[edi], g[ebp], 4);
        store(0xe4aa1u, stack(0x28u), g[ebx]);
        store(0xe4aa5u, stack(0x13u), 0, 1);
        call(0xe4aaau, 0xe4aafu, 0xe0500u);
    }

    bool check_activity() {
        test_byte(0xe4aafu, low(eax));
        if (!zero(0xe4ab1u)) {
            flags.logic(read(0xe4ab3u, object(0xc1u), 1) & 1u, 1);
            store(0xe4abau, stack(0x13u), 1, 1);
            if (zero(0xe4abfu)) return true;
            push(0xe4ac1u, g[esi]);
            at(0xe4ac2u);
            set_low(edx, flags.logic(0, 1));
            call(0xe4ac4u, 0xe4ac9u, 0xbbbc0u);
        }
        flags.logic(read(0xe4ac9u, object(0xc1u), 1) & 1u, 1);
        if (zero(0xe4ad0u)) return true;
        set_low(eax, read(0xe4ad2u, object(0x10au), 1));
        at(0xe4ad8u);
        set_low(eax, flags.right(low(eax), 2u, 1, false));
        test_byte(0xe4adbu, low(eax) & 1u);
        if (zero(0xe4addu)) return true;
        flags.logic(read(0xe4adfu, g[ebx] + 0x6cu, 1) & 1u, 1);
        if (!zero(0xe4ae3u)) return false;
        set_low(ecx, read(0xe4ae9u, g[edi], 1));
        at(0xe4aebu);
        set_low(ecx, flags.right(low(ecx), 2u, 1, false));
        test_byte(0xe4aeeu, low(ecx) & 1u);
        return zero(0xe4af1u);
    }

    void publish_transform() {
        g[edi] = read(0xe4af7u, stack(0xecu));
        push(0xe4afeu, g[edi]);
        at(0xe4affu);
        g[esi] = object(0x3dcu);
        push(0xe4b05u, g[esi]);
        at(0xe4b06u);
        g[eax] = stack(0x38u);
        call(0xe4b0au, 0xe4b0fu, 0x1ed880u);
        g[ecx] = read(0xe4b0fu, stack(0x48u));
        g[edx] = read(0xe4b13u, stack(0x4cu));
        at(0xe4b17u);
        g[eax] = object(0x70u);
        at(0xe4b1au);
        g[ebx] = g[eax];
        store(0xe4b1cu, g[ebx], g[ecx]);
        store(0xe4b1eu, g[ebx] + 4u, g[edx]);
        g[edx] = read(0xe4b21u, stack(0x50u));
        store(0xe4b25u, g[ebx] + 8u, g[edx]);
        at(0xe4b28u);
        g[ebx] = g[edi] + 0xf4u;
        store(0xe4b2eu, g[ebx], g[ecx]);
        g[ecx] = read(0xe4b30u, stack(0x4cu));
        store(0xe4b34u, g[ebx] + 4u, g[ecx]);
        store(0xe4b37u, g[ebx] + 8u, g[edx]);
        g[ebx] = read(0xe4b3au, stack(0x54u));
        at(0xe4b3eu);
        g[ecx] = object(0x7cu);
        at(0xe4b41u);
        g[edx] = g[ecx];
        store(0xe4b43u, g[edx], g[ebx]);
        g[ebx] = read(0xe4b45u, stack(0x58u));
        store(0xe4b49u, g[edx] + 4u, g[ebx]);
        g[ebx] = read(0xe4b4cu, stack(0x5cu));
        store(0xe4b50u, g[edx] + 8u, g[ebx]);
        push(0xe4b53u, g[eax]);
        at(0xe4b54u);
        g[eax] = g[edi];
        at(0xe4b56u);
        g[edx] = g[esi];
        call(0xe4b58u, 0xe4b5du, 0x1edda0u);
        g[ebx] = read(0xe4b5du, stack(0xe8u));
        g[ecx] = read(0xe4b64u, stack(0x38u));
        g[edx] = read(0xe4b68u, stack(0x34u));
        at(0xe4b6cu);
        g[eax] = g[ebx] + 8u;
        push(0xe4b6fu, g[eax]);
        g[eax] = read(0xe4b70u, stack(0xe8u));
        push(0xe4b77u, g[ecx]);
        at(0xe4b78u);
        g[edi] = g[ebx] + 4u;
        push(0xe4b7bu, g[edx]);
        call(0xe4b7cu, 0xe4b81u, 0xe53e0u);
    }

    void project_relative_vector() {
        g[eax] = read(0xe4cc2u, stack(0x3cu));
        g[ecx] = read(0xe4cc6u, stack(0x40u));
        g[edx] = read(0xe4ccau, stack(0x44u));
        store(0xe4cceu, stack(0x18u), g[eax]);
        flags.minus(read(0xe4cd2u, object(0x3dcu), 1), 1, 1);
        store(0xe4cd9u, stack(0x1cu), g[ecx]);
        store(0xe4cddu, stack(0x20u), g[edx]);
        if (zero(0xe4ce1u)) {
            vector_component(0xe4ce3u, 0xe4ce9u, 0xe4cf2u, 0x18u, 0x94u);
            vector_component(0xe4cf8u, 0xe4cfeu, 0xe4d07u, 0x1cu, 0x98u);
            vector_component(0xe4d0du, 0xe4d13u, 0xe4d1cu, 0x20u, 0x9cu);
        }
        at(0xe4d22u);
        g[eax] = object(0x7cu);
        at(0xe4d25u);
        g[edx] = object(0x70u);
        at(0xe4d28u);
        g[ecx] = stack(0xacu);
        call(0xe4d2fu, 0xe4d34u, 0x144210u);
        at(0xe4d34u);
        g[edx] = stack(0xa0u);
        at(0xe4d3bu);
        g[eax] = stack(0x18u);
        call(0xe4d3fu, 0xe4d44u, 0x1452f0u);
        g[esi] = read(0xe4d44u, stack(0x28u));
        at(0xe4d48u);
        g[eax] = g[edx];
        push(0xe4d4au, g[eax]);
        g[eax] = read(0xe4d4bu, stack(0xe8u));
        call(0xe4d52u, 0xe4d57u, 0x1d5b50u);
        g[esi] = read(0xe4d57u, stack(0xe4u));
        at(0xe4d5eu);
    }

    void vector_component(std::uint32_t load, std::uint32_t subtract, std::uint32_t save,
                          std::uint32_t destination, std::uint32_t origin) {
        s.xmm[0] = scalar(load, stack(destination));
        s.xmm[0] = projection_difference(s.xmm[0], scalar(subtract, stack(origin)));
        store(save, stack(destination), physics_step_detail::u(_mm_cvtss_f32(s.xmm[0])));
    }

    bool movement_exceeds_limit() {
        auto &x = s.xmm;
        using native_tu15_leaves::ordered_mul_ss;
        using native_world_leaves_detail::ordered_add_ss;
        g[eax] = read(0xe4ff4u, stack(0xecu));
        x[2] = scalar(0xe4ffbu, g[eax] + 0x138u);
        x[1] = scalar(0xe5003u, g[eax] + 0x13cu);
        x[0] = scalar(0xe500bu, g[eax] + 0x140u);
        at(0xe5013u);
        x[3] = x[2];
        at(0xe5016u);
        x[3] = ordered_mul_ss(x[3], x[2]);
        at(0xe501au);
        x[2] = x[1];
        at(0xe501du);
        x[2] = ordered_mul_ss(x[2], x[1]);
        at(0xe5021u);
        x[1] = x[0];
        at(0xe5024u);
        x[3] = ordered_add_ss(x[3], x[2]);
        at(0xe5028u);
        x[1] = ordered_mul_ss(x[1], x[0]);
        at(0xe502cu);
        x[3] = ordered_add_ss(x[3], x[1]);
        flags.scalar_compare(x[3], scalar(0xe5030u, 0x46a0e8u), false);
        at(0xe5037u);
        return !flags.cf && !flags.zf;
    }

    void age_activity_request() {
        set_low(ebx, read(0xe4dc0u, object(0x3aau), 1));
        test_byte(0xe4dc6u, low(ebx));
        if (zero(0xe4dc8u)) return;
        at(0xe4dcau);
        g[eax] = g[esi];
        call(0xe4dccu, 0xe4dd1u, 0xe3310u);
        test_byte(0xe4dd1u, low(eax));
        if (!zero(0xe4dd3u)) return;
        test_byte(0xe4dd5u, low(ebx));
        at(0xe4dd7u);
        set_low(eax, flags.zf || flags.sf != flags.of);
        at(0xe4ddau);
        g[eax] = 2u * g[eax] - 1u;
        at(0xe4ddeu);
        set_low(eax, flags.plus(low(eax), low(ebx), 1));
        store(0xe4de0u, object(0x3aau), low(eax), 1);
    }

    void read_contact_status() {
        g[edi] = read(0xe4de6u, 0x4ecbd0u);
        g[ebx] = read(0xe4decu, stack(0x14u));
        g[edi] = read(0xe4df0u, g[edi] + 0x44u);
        g[eax] = read(0xe4df3u, object(0x134u));
        g[ecx] = read(0xe4df9u, stack(0x30u));
        g[edi] = read(0xe4dfdu, g[edi] + g[ebx] + 8u);
        set_low(edx, read(0xe4e01u, object(0x148u), 1));
        set_low(ebx, read(0xe4e07u, g[edi] + 0x3dcu, 1));
        at(0xe4e0du);
        g[eax] = flags.right(g[eax], 0x17u, 4u, false);
        at(0xe4e10u);
        g[ecx] = flags.right(g[ecx], 1u, 4u, false);
        at(0xe4e12u);
        set_low(eax, flags.logic(low(eax) & 1u, 1));
        at(0xe4e14u);
        set_low(edx, flags.logic(low(edx) & 1u, 1));
        at(0xe4e17u);
        set_low(ecx, flags.logic(low(ecx) & 1u, 1));
        at(0xe4e1au);
        flags.minus(low(ebx), 6u, 1);
        store(0xe4e1du, stack(0x13u), 0, 1);
        if (!zero(0xe4e22u)) return;
        set_low(ebx, read(0xe4e24u, g[edi] + 0x3f8u, 1));
        test_byte(0xe4e2au, low(ebx));
        if (zero(0xe4e2cu)) return;
        at(0xe4e2eu);
        g[ebx] = flags.logic(0, 4);
        g[ebx] = read(0xe4e30u, g[edi] + 0x348u, 2);
        at(0xe4e37u);
        g[ebx] = flags.right(g[ebx], 0xdu, 4, false);
        test_byte(0xe4e3au, low(ebx) & 1u);
        if (!zero(0xe4e3du)) store(0xe4e3fu, stack(0x13u), 1, 1);
    }

    void publish_contact_state() {
        flags.minus(read(0xe4e44u, object(0x3dcu), 1), 4u, 1);
        if (zero(0xe4e4bu)) return;
        set_low(ebx, read(0xe4e4du, stack(0x13u), 1));
        test_byte(0xe4e51u, low(ebx));
        if (!zero(0xe4e53u)) return;
        test_byte(0xe4e55u, low(eax));
        bool set_contact;
        if (zero(0xe4e57u)) {
            test_byte(0xe4e59u, low(edx));
            set_contact = !zero(0xe4e5bu);
            if (!set_contact) {at(0xe4e5du); return;}
        } else {
            test_byte(0xe4e5fu, low(edx));
            if (!zero(0xe4e61u)) return;
            test_byte(0xe4e63u, low(ecx));
            if (zero(0xe4e65u)) return;
            test_byte(0xe4e67u, low(edx));
            set_contact = !zero(0xe4e69u);
        }
        if (set_contact) {
            g[eax] = read(0xe4e6bu, object(0x134u));
            at(0xe4e71u);
            g[eax] = flags.logic(g[eax] | 0x800000u, 4);
            at(0xe4e76u);
        } else {
            g[eax] = read(0xe4e78u, object(0x134u));
            at(0xe4e7eu);
            g[eax] = flags.logic(g[eax] & 0xff7fffffu, 4);
        }
        push(0xe4e83u, 0);
        push(0xe4e85u, g[esi]);
        store(0xe4e86u, object(0x134u), g[eax]);
        call(0xe4e8cu, 0xe4e91u, 0x1cc310u);
    }

    bool start_contact_transition() {
        at(0xe4e91u);
        g[ecx] = flags.logic(0, 4);
        g[ecx] = read(0xe4e93u, object(0x348u), 2);
        at(0xe4e9au);
        g[ecx] = flags.right(g[ecx], 0xeu, 4, false);
        test_byte(0xe4e9du, low(ecx) & 1u);
        if (!zero(0xe4ea0u)) return false;
        set_low(eax, read(0xe4ea2u, stack(0x13u), 1));
        test_byte(0xe4ea6u, low(eax));
        if (zero(0xe4ea8u)) return false;
        g[edx] = read(0xe4eaau, object(0x134u));
        at(0xe4eb0u);
        g[edx] = flags.right(g[edx], 0x17u, 4, false);
        test_byte(0xe4eb3u, low(edx) & 1u);
        return zero(0xe4eb6u);
    }

    void set_contact_transition() {
        auto &x = s.xmm;
        using native_tu15_leaves::ordered_mul_ss;
        using native_world_leaves_detail::ordered_add_ss;
        g[edi] = read(0xe4eb8u, stack(0x2cu));
        x[0] = scalar(0xe4ebcu, g[edi] + 0x268u);
        x[0] = projection_difference(x[0], scalar(0xe4ec4u, g[edi] + 0x26cu));
        at(0xe4eccu);
        x[1] = _mm_setzero_ps();
        g[eax] = read(0xe4ecfu, 0x51df54u);
        at(0xe4ed4u);
        x[1] = projection_difference(x[1], x[0]);
        x[0] = scalar(0xe4ed8u, g[eax] + 4u);
        x[0] = ordered_mul_ss(x[0], scalar(0xe4eddu, 0x44a8a0u));
        push(0xe4ee5u, 0);
        at(0xe4ee7u);
        x[1] = contact_division(x[1], x[0]);
        x[1] = ordered_add_ss(x[1], scalar(0xe4eebu, 0x46a0ccu));
        push(0xe4ef3u, g[esi]);
        at(0xe4ef4u);
        g[ebx] = std::uint32_t(_mm_cvttss_si32(x[1]));
        call(0xe4ef8u, 0xe4efdu, 0x1cc310u);
        at(0xe4efdu);
        change_activity_byte(s, 0xe4efdu, 0x349u, 0x20u, true);
        store(0xe4f04u, object(0x3aau), low(ebx), 1);
        at(0xe4f0au);
        change_activity_byte(s, 0xe4f0au, 0x349u, 0x40u, true);
        at(0xe4f11u);
    }

    bool filter_transition_result() {
        g[ecx] = read(0xe4f1eu, object(0x134u));
        g[edx] = read(0xe4f24u, stack(0xe8u));
        at(0xe4f2bu);
        g[ecx] = flags.right(g[ecx], 0x17u, 4, false);
        at(0xe4f2eu);
        set_low(ecx, flags.logic(low(ecx) & 1u, 1));
        store(0xe4f31u, g[edx] + 9u, low(ecx), 1);
        g[eax] = read(0xe4f34u, g[edi] + 0x264u);
        at(0xe4f3au);
        g[eax] = flags.right(g[eax], 1u, 4, false);
        test_byte(0xe4f3cu, low(eax) & 1u);
        at(0xe4f3eu);
        s.xmm[1] = s.xmm[0];
        if (!zero(0xe4f41u)) return false;
        at(0xe4f43u);
        g[eax] = g[esi];
        call(0xe4f45u, 0xe4f4au, 0xe65a0u);
        test_byte(0xe4f4au, low(eax));
        if (!zero(0xe4f4cu)) return true;
        set_low(eax, read(0xe4f4eu, stack(0x13u), 1));
        test_byte(0xe4f52u, low(eax));
        return !zero(0xe4f54u);
    }

    void scale_contact_transition() {
        auto &x = s.xmm;
        using native_tu15_leaves::ordered_mul_ss;
        using native_world_leaves_detail::ordered_add_ss;
        g[ecx] = read(0xe4f56u, 0x51df54u);
        g[edx] = offset(0xe4f5cu, g[ecx] + 2u);
        x[3] = scalar(0xe4f60u, g[edi] + 0x268u);
        x[3] = projection_difference(x[3], scalar(0xe4f68u, g[edi] + 0x26cu));
        at(0xe4f70u);
        x[0] = _mm_cvtsi32_ss(x[0], physics_step_detail::si(g[edx]));
        at(0xe4f74u);
        x[3] = ordered_mul_ss(x[3], x[0]);
        at(0xe4f78u);
        x[3] = ordered_mul_ss(x[3], x[1]);
        x[3] = ordered_add_ss(x[3], scalar(0xe4f7cu, stack(0x44u)));
        store(0xe4f82u, stack(0x44u), physics_step_detail::u(_mm_cvtss_f32(x[3])));
        at(0xe4f88u);
    }

public:
    ResultFrame(State &state, Route &children) : s(state), route(children), g(s.gp), flags(s.flags) {}

    void update_contact_transition() {
        age_activity_request();
        read_contact_status();
        publish_contact_state();
        if (start_contact_transition()) set_contact_transition();
        else g[edi] = read(0xe4f13u, stack(0x2cu));
        at(0xe4f17u);
        g[eax] = g[esi];
        call(0xe4f19u, 0xe4f1eu, 0xdfd20u);
        if (filter_transition_result()) scale_contact_transition();
        else s.xmm[3] = scalar(0xe4f8au, stack(0x44u));
    }

    Continuation begin() {
        setup();
        if (!check_activity()) return Continuation::blocked;
        publish_transform();
        return Continuation::tick;
    }

    void project_result() {
        constexpr std::uint32_t absent = 0xffffffffu;
        g[eax] = read(0xe4c58u, stack(0x28u));
        g[ecx] = read(0xe4c5cu, g[eax] + 0x68u);
        at(0xe4c5fu);
        g[edi] = flags.logic(absent, 4);
        at(0xe4c62u);
        flags.minus(g[ecx], g[edi], 4);
        if (zero(0xe4c64u)) return;
        flags.minus(read(0xe4c6au, g[eax]), g[edi], 4);
        if (zero(0xe4c6cu)) return;
        flags.minus(read(0xe4c72u, g[eax] + 6u, 2), g[edi] & 0xffffu, 2);
        if (zero(0xe4c76u)) return;
        flags.minus(read(0xe4c7cu, g[eax] + 0x7cu), 0xe0000c2u, 4);
        if (zero(0xe4c83u)) return;
        at(0xe4c89u);
        flags.minus(g[esi], g[edi], 4);
        if (!zero(0xe4c8bu)) {
            g[ecx] = read(0xe4c8du, 0x4ecbd0u);
            g[edx] = read(0xe4c93u, g[ecx] + 0x44u);
            g[ecx] = read(0xe4c96u, stack(0x14u));
            g[edx] = read(0xe4c9au, g[edx] + g[ecx] + 8u);
            set_low(ecx, read(0xe4c9eu, g[edx] + 0xb3u, 1));
            test_byte(0xe4ca4u, low(ecx));
            at(0xe4ca6u);
            if (!flags.cf && !flags.zf) return;
        }
        g[ecx] = read(0xe4cacu, stack(0x2cu));
        g[edx] = read(0xe4cb0u, g[ecx] + 0x264u);
        at(0xe4cb6u);
        g[edx] = flags.right(g[edx], 2, 4, false);
        test_byte(0xe4cb9u, low(edx) & 1u);
        if (zero(0xe4cbcu)) {
            at(0xe4d60u);
            g[ecx] = object(0x1b0u);
            push(0xe4d66u, g[ecx]);
            push(0xe4d67u, g[esi]);
            call(0xe4d68u, 0xe4d6du, 0x1d5db0u);
            return;
        }
        project_relative_vector();
    }

    Alternate update_activity() {
        set_low(eax, read(0xe4f90u, stack(0x30u), 1));
        at(0xe4f94u);
        change_activity_byte(s, 0xe4f94u, 0x349u, 0x8u, true);
        test_byte(0xe4f9bu, low(eax) & 1u);
        if (zero(0xe4f9du)) {
            at(0xe4fa8u);
            change_activity_byte(s, 0xe4fa8u, 0x348u, 0xfdu, false);
        } else {
            at(0xe4f9fu);
            change_activity_byte(s, 0xe4f9fu, 0x348u, 0x2u, true);
            at(0xe4fa6u);
        }
        set_low(eax, read(0xe4fafu, object(0xc0u), 1));
        at(0xe4fb5u);
        set_low(eax, flags.right(low(eax), 6, 1, false));
        test_byte(0xe4fb8u, low(eax) & 1u);
        if (zero(0xe4fbau)) return Alternate::apply_impulse;
        g[eax] = read(0xe4fc0u, object(0xb4u));
        g[edx] = read(0xe4fc6u, 0x52be5cu);
        at(0xe4fccu);
        g[eax] = flags.logic(g[eax] & 0xffffu, 4);
        at(0xe4fd1u);
        g[ecx] = g[eax] * 5u;
        g[eax] = read(0xe4fd4u, g[edx] + 0x44u);
        at(0xe4fd7u);
        g[ecx] = flags.shift(g[ecx], 5, 4);
        g[ecx] = read(0xe4fdau, g[ecx] + g[eax] + 4u);
        at(0xe4fdeu);
        g[ecx] = flags.right(g[ecx], 0xbu, 4, false);
        test_byte(0xe4fe1u, low(ecx) & 1u);
        if (!zero(0xe4fe4u)) return Alternate::decay;
        set_low(eax, read(0xe4feau, object(0x34bu), 1));
        test_byte(0xe4ff0u, low(eax));
        if (!zero(0xe4ff2u) || !movement_exceeds_limit()) {
            at(0xe503du);
            set_low(ebx, flags.logic(0, 1));
        } else {
            at(0xe5039u);
            set_low(ebx, 1);
            at(0xe503bu);
        }
        g[esi] = read(0xe503fu, stack(0xe4u));
        call(0xe5046u, 0xe504bu, 0xe0500u);
        test_byte(0xe504bu, low(eax));
        bool refresh = !zero(0xe504du);
        if (!refresh) {
            test_byte(0xe504fu, low(ebx));
            store(0xe5051u, stack(0x24u), low(eax), 1);
            refresh = !zero(0xe5055u);
        }
        if (refresh) store(0xe5057u, stack(0x24u), 1, 1);
        g[edx] = read(0xe505cu, 0x4ecbd0u);
        g[eax] = read(0xe5062u, g[edx] + 0x44u);
        g[ecx] = read(0xe5065u, stack(0x14u));
        g[eax] = read(0xe5069u, g[eax] + g[ecx] + 8u);
        g[edx] = read(0xe506du, stack(0x3cu));
        g[ecx] = read(0xe5071u, stack(0x40u));
        at(0xe5075u);
        g[eax] = flags.plus(g[eax], 0x88u, 4);
        store(0xe507au, g[eax], g[edx]);
        g[edx] = read(0xe507cu, stack(0x44u));
        store(0xe5080u, g[eax] + 4u, g[ecx]);
        store(0xe5083u, g[eax] + 8u, g[edx]);
        g[eax] = read(0xe5086u, stack(0x24u));
        push(0xe508au, g[eax]);
        at(0xe508bu);
        g[ebx] = stack(0x40u);
        at(0xe508fu);
        return Alternate::publish;
    }
};

inline bool force_reference() {
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_ACTIVITY");
    return reference;
}

template<class State> bool admitted(const State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return s.image == SourcePhysicsContactImage::TitleUpdate15 && !force_reference();
#else
    (void)s;
    return false;
#endif
}

template<class State, class Route>
Continuation begin(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (admitted(s))
        return ResultFrame<State, Route>{s, route}.begin();
#endif
    return Continuation::reference;
}

template<class State, class Route>
bool project_result(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (admitted(s)) {
        ResultFrame<State, Route>{s, route}.project_result();
        return true;
    }
#endif
    return false;
}

template<class State, class Route>
bool update_contact_transition(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    static const bool reference = native_small_queries_detail::flag("HALO2_REFERENCE_PHYSICAL_RESULT_CONTACT_E4DC0");
    if (admitted(s) && !reference) {
        ResultFrame<State, Route>{s, route}.update_contact_transition();
        return true;
    }
#endif
    return false;
}

template<class State, class Route>
Alternate update_activity(State &s, Route route) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (admitted(s)) return ResultFrame<State, Route>{s, route}.update_activity();
#endif
    return Alternate::reference;
}
}
