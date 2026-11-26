#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_keyed_marker_pose_systems.hpp"
#include <cmath>

namespace halo2::native_keyed_marker_pose_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;
namespace logic = native_keyed_marker_pose;
namespace fp_math = physics_step_detail;
using logic::Access;
using logic::Local;

enum Branch : unsigned {
    missing_animation, missing_graph, missing_channel, missing_other,
    inactive_transition, below_margin, posed, branch_count
};
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Transition, class Markers>
struct Adapter : Port<State> {
    Transition &transition;
    Markers &markers;

    Adapter(State &s, std::uint32_t *cursor, Trace<State> *trace, Transition &blend, Markers &pose)
        : Port<State>(s, cursor, trace, {}), transition(blend), markers(pose) {}

    std::uint32_t local(Local slot) const { return this->gp[esp] + std::uint32_t(slot); }
    void reserve_frame() {
        auto &gp = this->gp;
        gp[esp] = this->flags.minus(gp[esp], 0x14u, 4);
        gp[eax] = this->flags.logic(gp[ebx] & 0xffffu, 4);
        this->push(0x116293u, gp[ebp]);
    }
    std::uint32_t instruction(Access access, std::uint32_t) {
        auto &gp = this->gp;
        using enum Access;
        switch (access) {
        case object_table: return 0x116280u;
        case object_rows: return 0x116286u;
        case other_object: return 0x116294u;
        case object_data:
            gp[eax] *= 3u;
            return 0x11629bu;
        case channels_offset: return 0x11629fu;
        case state_offset:
            this->push(0x1162a6u, gp[esi]);
            return 0x1162a7u;
        case animation_tag:
            gp[esi] = this->flags.plus(gp[esi], gp[eax], 4);
            gp[ecx] = this->flags.plus(gp[ecx], gp[eax], 4);
            return 0x1162b2u;
        case graph: return 0x1162bdu;
        case channel: return 0x1162c1u;
        case duration: return 0x1162cbu;
        case weight_store: return 0x1162e0u;
        case weight_read: return 0x1162e6u;
        case margin: return 0x1162ecu;
        case matrices: return 0x1162f8u;
        case poses: return 0x1162fcu;
        case weight_argument: return 0x116301u;
        case record_first: return 0x116311u;
        case record_second: return 0x116315u;
        case record_index: return 0x116319u;
        case record_stage: return 0x11631eu;
        case record_first_key: return 0x116325u;
        case record_second_key: return 0x116329u;
        }
        return 0;
    }
    void loaded(Access access, std::uint32_t, std::uint32_t value) {
        auto &gp = this->gp;
        using enum Access;
        switch (access) {
        case object_table:
        case poses: gp[ecx] = value; break;
        case object_rows: gp[edx] = value; break;
        case other_object: gp[ebp] = value; break;
        case object_data:
        case matrices:
        case weight_argument: gp[eax] = value; break;
        case channels_offset: gp[ecx] = logic::widen16(value); break;
        case state_offset: gp[esi] = logic::widen16(value); break;
        case animation_tag:
            gp[eax] = value;
            this->push(0x1162b5u, gp[edi]);
            gp[edi] = this->flags.logic(logic::animation::none, 4);
            this->compare32(value, gp[edi]);
            if (this->flags.zf) this->reached(missing_animation);
            break;
        case graph:
            this->compare32(value, gp[edi]);
            if (this->flags.zf) this->reached(missing_graph);
            break;
        case channel:
            this->flags.minus(value, gp[edi] & 0xffffu, 2);
            if (this->flags.zf) this->reached(missing_channel);
            break;
        case duration:
            set_low_byte(gp[eax], value);
            this->test8(value);
            if (this->flags.zf) this->reached(inactive_transition);
            break;
        default: break;
        }
    }
    void stored(Access, std::uint32_t, std::uint32_t) {}

    bool has_other_object(std::uint32_t other) {
        this->compare32(other, this->gp[edi]);
        if (this->flags.zf) this->reached(missing_other);
        return !this->flags.zf;
    }
    std::uint32_t transition_weight(std::uint32_t address) {
        this->gp[eax] = address;
        this->call(0x1162dbu, 0x1162e0u, 0x1e1620u, transition);
        return fp_math::u(_mm_cvtss_f32(this->s.xmm[0]));
    }
    void load_absolute_weight(std::uint32_t value) {
        auto &fp = this->s.fp;
        fp.push(fp_math::ext(fp_math::f(value)));
        fp.st(0) = std::fabs(fp.st(0));
    }
    bool passes_margin(std::uint32_t margin) {
        auto &state = this->s;
        state.fp.push(fp_math::ext(fp_math::f(margin)));
        state.compare_and_pop(state.fp.st(1));
        state.fp.pop();
        this->flags = state.flags;
        const bool passes = this->flags.cf || this->flags.zf;
        if (!passes) this->reached(below_margin);
        return passes;
    }

    void push_matrices(std::uint32_t matrices) { this->push(0x116300u, matrices); }
    std::uint32_t push_pose_arguments(std::uint32_t poses, std::uint32_t weight) {
        auto &gp = this->gp;
        this->push(0x116305u, poses);
        const auto iterator = gp[edx] = gp[esp] + 0x14u;
        this->push(0x11630au, iterator);
        this->push(0x11630bu, gp[esi]);
        this->push(0x11630cu, weight);
        this->push(0x11630du, 1u);
        this->push(0x11630fu, gp[ebp]);
        this->push(0x116310u, gp[ebx]);
        return iterator;
    }
    std::uint32_t iterator_sentinel() const { return this->gp[edi]; }
    void pose_chains() {
        this->reached(posed);
        this->call(0x11632du, 0x116332u, 0x116060u, markers);
    }
    void run() {
        auto &gp = this->gp;
        Memory<Adapter> memory{*this};
        logic::pose(memory, *this, gp[ebx]);
        gp[edi] = this->pop(0x116332u);
        gp[esi] = this->pop(0x116333u);
        gp[ebp] = this->pop(0x116334u);
        gp[esp] = this->flags.plus(gp[esp], 0x14u, 4);
        this->ret(0x116338u, 12u);
        this->publish(0x116338u);
    }
};

template<class State, class Reference, class Transition, class Markers>
bool bridge(State &s, Reference reference, Transition transition, Markers markers) {
    static const bool use_reference = native_small_queries_detail::flag("HALO2_REFERENCE_KEYED_MARKER_POSE");
    static const bool verify = native_small_queries_detail::flag("HALO2_VERIFY_KEYED_MARKER_POSE");
    static VerifySummary summary{"Keyed marker pose"};
    return run_owner(s, 0x116280u, 0x116338u, use_reference, verify, coverage(), summary, reference,
        [&](State &v, std::uint32_t *c, Trace<State> *t) { return Adapter<State, Transition, Markers>(v, c, t, transition, markers); });
}
} // namespace halo2::native_keyed_marker_pose_detail
