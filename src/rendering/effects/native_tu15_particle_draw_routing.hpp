#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_particle_draw_routing_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum class Stage { visibility_input, skip_view, advance_view };
enum class Resume : std::uint32_t {
    retained = 0xffffffffu, visibility = 0x177594u, next_view = 0x177453u, descendants = 0x1777c4u,
};
enum Branch : unsigned { use_view_mask, force_from_query, force_from_definition, reset_skipped_view,
                        advance_views, visit_descendants, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Query>
struct Frame : Port<State> {
    Stage stage;
    Query &query;
    Resume &next;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Stage position,
          Query &state_query, Resume &continuation)
        : Port<State>(state, cursor, trace, {}), stage(position), query(state_query), next(continuation) {}

    void visibility_input() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        const auto cluster = this->read(0x17756au, gp[ebp] + 0x20u, 2);
        gp[edx] = static_cast<std::uint32_t>(static_cast<std::int16_t>(cluster));
        this->call(0x17756eu, 0x177573u, 0x3eb10u, [&](State &state) {
            try {
                query(state);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
        this->test8(gp[eax]);
        const bool query_visible = !flags.zf;
        if (!query_visible) {
            gp[ecx] = flags.logic(0, 4);
            gp[ecx] = this->read(0x177579u, gp[ebp] + 0xcu, 2);
            gp[ecx] = flags.right(gp[ecx], 10u, 4, false);
            flags.logic(gp[ecx] & 1u, 1);
        }
        const auto view_mask = gp[esp] + 0x12u;
        if (flags.zf) {
            this->reached(use_view_mask);
            this->store(0x177585u, view_mask, 1, low_byte(gp[eax]));
            this->publish(0x177589u);
        } else {
            this->reached(query_visible ? force_from_query : force_from_definition);
            this->store(0x17758bu, view_mask, 1, 1u);
            set_low_byte(gp[eax], this->read(0x177590u, view_mask, 1));
            this->publish(0x177590u);
        }
        next = Resume::visibility;
    }

    void advance_view() {
        auto &gp = this->gp;
        if (stage == Stage::skip_view) {
            this->reached(reset_skipped_view);
            gp[eax] = this->read(0x1777afu, 0x4c6854u, 4);
            this->store(0x1777b4u, gp[esi] + 0x30u, 4, gp[eax]);
        }
        gp[eax] = this->read(0x1777b7u, gp[esi] + 0xcu, 4);
        this->compare32(gp[eax], 0xffffffffu);
        if (!this->flags.zf) {
            this->reached(advance_views);
            next = Resume::next_view;
            this->publish(0x1777bdu);
            return;
        }
        gp[ebx] = this->pop(0x1777c3u);
        this->reached(visit_descendants);
        next = Resume::descendants;
        this->publish(0x1777c3u);
    }

    void run() {
        try {
            if (stage == Stage::visibility_input) visibility_input();
            else advance_view();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Query>
Resume bridge(State &state, Stage stage, Reference reference, Query query) {
    static const bool reference_mode = flag("HALO2_REFERENCE_PARTICLE_DRAW_ROUTING");
    static const bool verify_mode = flag("HALO2_VERIFY_PARTICLE_DRAW_ROUTING");
    static VerifySummary summary{"Particle draw routing"};
    constexpr std::array first{0x17756au, 0x1777afu, 0x1777b7u};
    constexpr std::array last{0x177590u, 0x1777c3u, 0x1777c3u};
    const auto index = static_cast<unsigned>(stage);
    auto next = Resume::retained;
    const auto verify_reference = [&](State &ref) {
        if (reference(ref) != next)
            throw std::runtime_error("Particle draw routing continuation differed");
    };
    run_owner(state, first[index], last[index], reference_mode, verify_mode, coverage(), summary, verify_reference,
        [&](State &value, std::uint32_t *cursor, Trace<State> *trace) {
            return Frame<State, Query>{value, cursor, trace, stage, query, next};
        });
    return next;
}
} // namespace halo2::native_particle_draw_routing_detail
