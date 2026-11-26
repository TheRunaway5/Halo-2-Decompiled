#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

namespace halo2::native_death_cycle_record_route {

using namespace native_death_cycle_opening;
using native_death_cycle_motion::update_field;

enum class Entry { primary, secondary, final_motion, reset_motion };

inline bool reference() {
    static const bool enabled = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_DEATH_CYCLE_RECORD_ROUTE");
        return value && *value;
    }();
    return enabled;
}

template<class State>
struct RecordFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    explicit RecordFrame(State &state) : Base{state} {}

    auto stack(std::uint32_t offset) { return field(esp, offset); }
    auto record(std::uint32_t offset) { return field(esi, offset); }

    bool test_local(std::uint32_t input, std::uint32_t test, std::uint32_t branch, std::uint32_t offset) {
        set_low_byte(g[eax], read(input, stack(offset), 1));
        site(test);
        flags.logic(low_byte(g[eax]), 1);
        site(branch);
        return !flags.zf;
    }

    bool test_record_status(std::uint32_t input, std::uint32_t branch) {
        flags.logic(read(input, record(4u), 1) & 1u, 1);
        site(branch);
        return !flags.zf;
    }

    void clear_record_status(std::uint32_t pc) {
        access(pc);
        update_field(s, pc, g[esi] + 4u, 1, 0xfeu, 0u);
    }

    bool has_companion_record() {
        g[edx] = read(0x10421bu, field(ebp));
        site(0x10421eu);
        g[eax] = g[edx];
        site(0x104220u);
        g[eax] = flags.right(g[eax], 9u, 4, false);
        site(0x104223u);
        set_low_byte(g[ebx], flags.logic(0u, 1));
        site(0x104225u);
        set_low_byte(g[eax], flags.logic(g[eax] & 1u, 1));
        site(0x104227u);
        if (flags.zf) {
            site(0x104229u);
            g[ecx] = g[edx];
            site(0x10422bu);
            g[ecx] = flags.right(g[ecx], 10u, 4, false);
            site(0x10422eu);
            flags.logic(g[ecx] & 1u, 1);
            site(0x104231u);
            if (flags.zf) return false;
        }
        g[ecx] = read(0x104233u, stack(0x2cu));
        flags.minus(read(0x104237u, field(ecx, 0x2d0u)), 1u, 4);
        site(0x10423eu);
        return !flags.zf && flags.sf == flags.of;
    }

    bool keep_primary_type() {
        site(0x104240u);
        g[ecx] = flags.logic(0u, 4);
        site(0x104242u);
        flags.logic(g[edi] & 0xffffu, 2);
        g[edi] = read(0x104245u, stack(0x34u));
        site(0x104249u);
        set_low_byte(g[ecx], flags.zf);
        site(0x10424cu);
        g[ecx] = flags.product(g[ecx], 0x34u);
        site(0x10424fu);
        g[ecx] += g[edi] + 0x1a4u;
        set_low_byte(g[ecx], read(0x104256u, field(ecx, 1u), 1));
        site(0x104259u);
        flags.minus(low_byte(g[ecx]), 1u, 1);
        site(0x10425cu);
        if (flags.zf) {
            site(0x10425eu);
            flags.logic(low_byte(g[eax]), 1);
            site(0x104260u);
            if (!flags.zf) {
                site(0x104262u);
                set_low_byte(g[ebx], g[ecx]);
            }
        }
        site(0x104264u);
        flags.minus(low_byte(g[ecx]), 3u, 1);
        site(0x104267u);
        if (flags.zf) {
            site(0x104269u);
            g[edx] = flags.right(g[edx], 10u, 4, false);
            site(0x10426cu);
            flags.logic(g[edx] & 1u, 1);
            site(0x10426fu);
            if (!flags.zf) return true;
        }
        site(0x104271u);
        flags.logic(low_byte(g[ebx]), 1);
        site(0x104273u);
        return !flags.zf;
    }

    void reset_primary_type() {
        if (has_companion_record()) {
            if (keep_primary_type()) {
                g[edi] = read(0x10427fu, stack(0x28u));
                site(0x104283u);
                return;
            }
            g[edi] = read(0x104275u, stack(0x28u));
        }
        store(0x104279u, record(1u), constant(1u), 1);
        site(0x10427du);
    }

    template<class Call>
    bool primary(Call &call) {
        if (test_local(0x104204u, 0x104208u, 0x10420au, 0x16u)) {
            clear_record_status(0x10420cu);
            site(0x104210u);
        } else {
            bool changed = true;
            if (test_record_status(0x104215u, 0x104219u)) {
                reset_primary_type();
            } else {
                g[edx] = read(0x104285u, stack(0x4cu));
                g[eax] = read(0x104289u, stack(0x20u));
                push(0x10428du, field(edx));
                push(0x10428eu, constant(0x104293u));
                call(s, 0x1074a0u, 0x10428eu);
                site(0x104293u);
                flags.logic(low_byte(g[eax]), 1);
                site(0x104295u);
                changed = !flags.zf;
            }
            if (changed) store(0x104297u, stack(0x13u), constant(1u), 1);
        }
        flags.minus(read(0x10429cu, record(1u), 1), 1u, 1);
        site(0x1042a0u);
        return flags.zf;
    }

    template<class Call>
    void stop_motion(Call &call, std::uint32_t push_index, std::uint32_t move_handle,
                     std::uint32_t call_pc, std::uint32_t resume) {
        push(push_index, field(edi));
        site(move_handle);
        g[eax] = g[ebx];
        push(call_pc, constant(resume));
        call(s, 0x108f30u, call_pc);
        site(resume);
    }

    template<class Call>
    void apply_secondary(Call &call) {
        clear_record_status(0x1042f9u);
        push(0x1042fdu, field(edi));
        push(0x1042feu, field(ebx));
        store(0x1042ffu, stack(0x1du), constant(1u), 1);
        push(0x104304u, constant(0x104309u));
        call(s, 0x108080u, 0x104304u);
        site(0x104309u);
    }

    void increment_secondary() {
        g[eax] = std::uint32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(0x104335u, record(0u), 1))));
        site(0x104338u);
        g[eax] = flags.increment(g[eax], 4, false);
        site(0x104339u);
        flags.minus(g[eax], 0x7fu, 4);
        store(0x10433cu, stack(0x15u), constant(1u), 1);
        site(0x104341u);
        if (!flags.zf && flags.sf == flags.of) {
            site(0x104343u);
            g[eax] = 0x7fu;
        }
        store(0x104348u, record(0u), field(eax), 1);
        site(0x10434au);
    }

    template<class Call>
    void secondary(Call &call) {
        set_low_byte(g[eax], read(0x1042a6u, stack(0x16u), 1));
        site(0x1042aau);
        flags.logic(low_byte(g[eax]), 1);
        store(0x1042acu, stack(0x14u), constant(0u), 1);
        store(0x1042b1u, stack(0x17u), constant(0u), 1);
        site(0x1042b6u);
        if (!flags.zf) {
            g[eax] = read(0x1042b8u, stack(0x4cu));
            push(0x1042bcu, field(edi));
            push(0x1042bdu, constant(0x1042c2u));
            call(s, 0x108f30u, 0x1042bdu);
            site(0x1042c2u);
            return;
        }

        g[ebx] = read(0x1042c7u, stack(0x4cu));
        site(0x1042cbu);
        g[eax] = g[esp] + 0x17u;
        push(0x1042cfu, field(eax));
        site(0x1042d0u);
        g[ecx] = g[esp] + 0x18u;
        push(0x1042d4u, field(ecx));
        push(0x1042d5u, field(ebx));
        site(0x1042d6u);
        g[eax] = g[edi];
        push(0x1042d8u, constant(0x1042ddu));
        call(s, 0x1071d0u, 0x1042d8u);
        site(0x1042ddu);
        flags.logic(low_byte(g[eax]), 1);
        site(0x1042dfu);
        if (!flags.zf) {
            if (test_local(0x1042e1u, 0x1042e5u, 0x1042e7u, 0x14u) &&
                !test_record_status(0x1042e9u, 0x1042edu))
                stop_motion(call, 0x1042efu, 0x1042f0u, 0x1042f2u, 0x1042f7u);
            else
                apply_secondary(call);
        } else if (test_local(0x10430bu, 0x10430fu, 0x104311u, 0x17u)) {
            stop_motion(call, 0x104313u, 0x104314u, 0x104316u, 0x10431bu);
        } else if (test_local(0x10431du, 0x104321u, 0x104323u, 0x14u) &&
                   !test_record_status(0x104325u, 0x104329u)) {
            stop_motion(call, 0x10432bu, 0x10432cu, 0x10432eu, 0x104333u);
        } else {
            increment_secondary();
        }
    }

    template<class Call>
    void finalize(Call &call) {
        flags.minus(read(0x10434cu, record(2u), 2), 0u, 2);
        site(0x104351u);
        if (!flags.zf) return;
        g[edx] = read(0x104353u, stack(0x4cu));
        push(0x104357u, field(edi));
        push(0x104358u, field(edx));
        push(0x104359u, constant(0x10435eu));
        call(s, 0x109030u, 0x104359u);
        site(0x10435eu);
    }

    template<class Call>
    void reset(Call &call) {
        flags.minus(read(0x104360u, record(2u), 2), 0u, 2);
        site(0x104365u);
        if (!flags.zf) return;
        g[eax] = read(0x104367u, stack(0x4cu));
        push(0x10436bu, field(edi));
        push(0x10436cu, constant(0x104371u));
        call(s, 0x108ea0u, 0x10436cu);
    }
};

template<class State, class Call>
bool run(State &s, Entry entry, Call call) {
    if (!selected(s) || reference()) return false;
    RecordFrame<State> frame{s};
    switch (entry) {
    case Entry::primary:
        if (!frame.primary(call)) break;
        [[fallthrough]];
    case Entry::secondary:
        frame.secondary(call);
        break;
    case Entry::final_motion:
        frame.finalize(call);
        break;
    case Entry::reset_motion:
        frame.reset(call);
        break;
    }
    return true;
}

} // namespace halo2::native_death_cycle_record_route
