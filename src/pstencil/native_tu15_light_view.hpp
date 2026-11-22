#pragma once

#include "pstencil/campaign_lighting_timing.hpp"
#include "pstencil/native_tu15_light_view_systems.hpp"
#include "pstencil/native_tu15_light_visibility_frame.hpp"

#include <bit>

namespace halo2::native_light_view_detail {
namespace logic = native_light_view;

template<class State, class Call, class Before>
class Adapter : public native_light_visibility_detail::Frame<State, Call, Before> {
    using Base = native_light_visibility_detail::Frame<State, Call, Before>;
    using enum native_light_visibility_detail::Register;
    using Base::s_; using Base::at; using Base::reg; using Base::low;
    using Base::load; using Base::write; using Base::store; using Base::push; using Base::pop;
    using Base::enter; using Base::finish_return;

    static constexpr std::uint32_t main_view = 0x556e18;
    static constexpr std::uint32_t secondary_view = 0x5598f8;
    static constexpr std::uint32_t active_view = 0x4cd3a0;
    static constexpr std::uint32_t bsp_epoch = 0x4f44ec;
    static constexpr std::uint32_t collecting_bsp = 0x4f44e8;
    static constexpr std::uint32_t cluster_epoch = 0x4eabcc;
    static constexpr std::uint32_t collecting_clusters = 0x4eabc8;
    static constexpr std::uint32_t object_epoch = 0x4ecbd8;
    static constexpr std::uint32_t collecting_objects = 0x4ecbd4;

public:
    using Base::Base;

    void begin() {
        const bool secondary = s_.flags.logic(reg(eax), 4) != 0;
        push(0x134c02, reg(ebx));
        push(0x134c03, reg(ebp));
        reg(ebp) = load(0x134c04, reg(esp) + 16);
        push(0x134c08, reg(esi));
        reg(ebx) = reg(ecx);
        reg(esi) = secondary ? secondary_view : main_view;
    }

    void setup_view() {
        // Preserve the original read order and complete caller stack consumed
        // by 135690, including the adjacent sub-word ABI arguments.
        reg(ecx) = load(0x134c17, reg(esp) + 36);
        push(0x134c1b, reg(edi));
        push(0x134c1c, reg(ecx));
        reg(ecx) = load(0x134c1d, reg(esp) + 36);
        push(0x134c21, reg(edx));
        reg(edx) = load(0x134c22, reg(esp) + 36);
        push(0x134c26, reg(ecx));
        reg(ecx) = load(0x134c27, reg(esp) + 48);
        push(0x134c2b, reg(edx));
        reg(edx) = load(0x134c2c, reg(esp) + 60);
        push(0x134c30, reg(ecx));
        reg(ecx) = load(0x134c31, reg(esp) + 40);
        push(0x134c35, reg(edx));
        push(0x134c36, reg(ebp));
        push(0x134c37, reg(eax));
        push(0x134c38, reg(esi));
        enter(0x134c39, 0x135690, 0x134c3e);
    }

    void reset_queues() {
        reg(edx) = reg(esi);
        reg(edi) = reg(eax);
        write(0x134c42, active_view, reg(esi), 4);
        enter(0x134c48, 0x1373f0, 0x134c4d);
    }

    logic::VisibilityKind visibility_kind() {
        const auto result = reg(edi);
        reg(eax) = s_.flags.minus(result, 0, 4);
        reg(edi) = pop(0x134c52);
        if(result != 0) {
            reg(eax) = s_.flags.increment(reg(eax), 4, true);
            if(result != 1) reg(eax) = s_.flags.increment(reg(eax), 4, true);
        }
        return static_cast<logic::VisibilityKind>(result);
    }

    void project_visibility(unsigned bank_offset) {
        reg(ebx) = load(0x134c5b, reg(esi) + bank_offset);
        reg(eax) = load(0x134c5d, reg(esp) + 40);
        push(0x134c61, reg(ebp));
        enter(0x134c62, 0x176270, 0x134c67);
    }

    void reuse_context() {
        reg(eax) = reg(esi);
        enter(0x134c6b, 0x135870, 0x134c70);
    }

    void traverse_portals(unsigned bank_offset, unsigned cluster_offset) {
        reg(eax) = load(0x134c72, reg(esi) + bank_offset);
        const auto cluster = std::uint16_t(load(0x134c74, reg(esi) + cluster_offset, 2));
        reg(ecx) = std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(cluster)));
        push(0x134c78, reg(eax));
        push(0x134c79, reg(ecx));
        push(0x134c7a, reg(ebp));
        push(0x134c7b, reg(ebx));
        enter(0x134c7c, 0x175f90, 0x134c81);
    }

    std::uint32_t auxiliary(unsigned offset) {
        reg(eax) = load(0x134c81, reg(esi) + offset);
        return s_.flags.logic(reg(eax), 4);
    }

    void merge_auxiliary(std::uint32_t context, unsigned bank_offset) {
        reg(edx) = load(0x134c8b, context + bank_offset);
        reg(ebx) = load(0x134c8d, reg(esi) + bank_offset);
        push(0x134c8f, reg(edx));
        push(0x134c90, reg(esi));
        enter(0x134c91, 0x135410, 0x134c96);
    }

    void begin_bsp() {
        const auto epoch = load(0x134c96, bsp_epoch);
        auto flags = s_.flags;
        const auto next_epoch = flags.increment(epoch, 4, false);
        store(0x134c96, bsp_epoch, next_epoch, [&] { s_.flags = flags; }, true);
        low(ebx, 1);
        push(0x134c9e, reg(esi));
        write(0x134c9f, collecting_bsp, low(ebx), 1);
    }

    void admit_bsp() { enter(0x134ca5, 0x136ea0, 0x134caa); }

    void begin_cluster_contents() {
        reg(edx) = load(0x134caa, cluster_epoch);
        reg(ecx) = load(0x134cb0, object_epoch);
        reg(edx) = s_.flags.increment(reg(edx), 4, false);
        reg(ecx) = s_.flags.increment(reg(ecx), 4, false);
        push(0x134cb8, reg(esi));
        write(0x134cb9, collecting_bsp, 0, 1);
        write(0x134cc0, cluster_epoch, reg(edx), 4);
        write(0x134cc6, collecting_clusters, low(ebx), 1);
        write(0x134ccc, object_epoch, reg(ecx), 4);
        write(0x134cd2, collecting_objects, low(ebx), 1);
    }

    void collect_cluster_contents() { enter(0x134cd8, 0x134d30, 0x134cdd); }

    void finish() {
        reg(eax) = reg(esi);
        reg(esi) = pop(0x134cdf);
        reg(ebp) = pop(0x134ce0);
        write(0x134ce1, collecting_clusters, 0, 1);
        write(0x134ce8, collecting_objects, 0, 1);
        reg(ebx) = pop(0x134cef);
        finish_return(0x134cf0, 28);
    }
};

template<class State, class Call, class Before = native_light_visibility_detail::Noop>
bool bridge(State& state, Call call, Before before = {}) {
    if(!native_light_visibility_detail::admitted(state, 0x134c00, 0x134cf0)) return false;
    if(auto* timing=pstencil::projected_visibility_timing) {
        using Phase=pstencil::CampaignLightPhase;
        const pstencil::ProjectedVisibilityTimingBinding nested(nullptr);
        const pstencil::CampaignLightPhaseTimer native(timing,Phase::projected_visibility_native);
        auto measured=[&](State& child, std::uint32_t target, std::uint32_t pc) {
            const auto phase=[&] {
                switch(target) {
                case 0x135690: return Phase::projected_visibility_view_setup;
                case 0x1373f0: return Phase::projected_visibility_queue_reset;
                case 0x175f90: return Phase::projected_visibility_portals;
                case 0x135410: return Phase::projected_visibility_auxiliary;
                case 0x135870: return Phase::projected_visibility_context_reuse;
                case 0x176270: return Phase::projected_visibility_projection;
                case 0x136ea0: return Phase::projected_visibility_bsp_admission;
                case 0x134d30: return Phase::projected_visibility_cluster_contents;
                default: return Phase::count;
                }
            }();
            const pstencil::CampaignLightPhaseTimer child_timer(phase==Phase::count?nullptr:timing,phase);
            if(phase==Phase::projected_visibility_cluster_contents) {
                const pstencil::ProjectedVisibilityTimingBinding cluster(timing,pstencil::projected_cluster_timing);
                call(child,target,pc);
            } else call(child,target,pc);
        };
        Adapter<State, decltype(measured), Before> frame(state, measured, before);
        logic::collect(frame);
        return true;
    }
    Adapter<State, Call, Before> frame(state, call, before);
    logic::collect(frame);
    return true;
}
} // namespace halo2::native_light_view_detail
