#pragma once
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <utility>

namespace halo2::pstencil {
enum class CampaignLightPhase : std::size_t {
    collection, native_selection, point_receivers, point_overlays,
    projected_receiver, projected_geometry, projected_setup, projected_casters,
    projected_layer_selection, projected_shadow_setup, projected_packets, projected_material,
    projected_visibility_query, projected_receiver_classify, projected_bsp_units,
    projected_model_units, projected_cull, projected_commands, projected_snapshot,
    projected_visibility_native, projected_visibility_view_setup, projected_visibility_queue_reset,
    projected_visibility_portals, projected_visibility_auxiliary, projected_visibility_context_reuse,
    projected_visibility_projection, projected_visibility_bsp_admission, projected_visibility_cluster_contents,
    projected_visibility_cluster_traversal, projected_visibility_cluster_fog, projected_visibility_object_admission,
    projected_visibility_object_geometry, projected_visibility_cluster_other_children,
    count
};
inline constexpr std::array<std::string_view,std::size_t(CampaignLightPhase::count)> campaign_light_phase_names{
    "collection","native_selection","point_receivers","point_overlays",
    "projected_receiver","projected_geometry","projected_setup","projected_casters",
    "projected_layer_selection","projected_shadow_setup","projected_packets","projected_material",
    "projected_visibility_query","projected_receiver_classify","projected_bsp_units",
    "projected_model_units","projected_cull","projected_commands","projected_snapshot",
    "projected_visibility_native","projected_visibility_view_setup","projected_visibility_queue_reset",
    "projected_visibility_portals","projected_visibility_auxiliary","projected_visibility_context_reuse",
    "projected_visibility_projection","projected_visibility_bsp_admission","projected_visibility_cluster_contents",
    "projected_visibility_cluster_traversal","projected_visibility_cluster_fog","projected_visibility_object_admission",
    "projected_visibility_object_geometry","projected_visibility_cluster_other_children"
};
inline constexpr std::array<std::string_view,5> campaign_light_work_names{
    "projected_visibility_native_object_admissions","projected_visibility_stamped_scans",
    "projected_visibility_stamped_bulk_scans","projected_visibility_stamped_scalar_probes",
    "projected_visibility_stamped_bulk_probes"
};
struct CampaignLightingPassTimings {
    struct Phase {std::uint64_t ns{};unsigned calls{};};
    std::array<Phase,std::size_t(CampaignLightPhase::count)> phases;
    struct Work {
        unsigned native_object_admissions{},stamped_scans{},stamped_bulk_scans{};
        std::uint64_t stamped_scalar_probes{},stamped_bulk_probes{};
    } work;
};
// Only the synchronous projected receiver root binds a timing packet. The
// bridge suspends it during that call so nested or unrelated views stay out.
inline thread_local CampaignLightingPassTimings* projected_visibility_timing{};
inline thread_local CampaignLightingPassTimings* projected_cluster_timing{};
inline thread_local CampaignLightingPassTimings* projected_object_timing{};
class ProjectedVisibilityTimingBinding {
    CampaignLightingPassTimings*& slot_;
    CampaignLightingPassTimings* previous_;
public:
    explicit ProjectedVisibilityTimingBinding(CampaignLightingPassTimings* timing,
        CampaignLightingPassTimings*& slot=projected_visibility_timing) noexcept
        :slot_(slot),previous_(std::exchange(slot,timing)){}
    ProjectedVisibilityTimingBinding(const ProjectedVisibilityTimingBinding&)=delete;
    ProjectedVisibilityTimingBinding& operator=(const ProjectedVisibilityTimingBinding&)=delete;
    ~ProjectedVisibilityTimingBinding(){slot_=previous_;}
};
using CampaignLightingClock=std::chrono::steady_clock;
inline CampaignLightingClock::time_point campaign_lighting_now() noexcept {
    const int saved_errno=errno;
    const auto now=CampaignLightingClock::now();
    errno=saved_errno;
    return now;
}
inline std::uint64_t campaign_lighting_elapsed(CampaignLightingClock::time_point begin) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(campaign_lighting_now()-begin).count();
}
class CampaignLightPhaseTimer {
    CampaignLightingPassTimings::Phase* phase_;
    CampaignLightingClock::time_point begin_;
public:
    CampaignLightPhaseTimer(CampaignLightingPassTimings* timing,CampaignLightPhase phase) noexcept
        :phase_(timing?&timing->phases[std::size_t(phase)]:nullptr),
         begin_(phase_?campaign_lighting_now():CampaignLightingClock::time_point{}){}
    CampaignLightPhaseTimer(const CampaignLightPhaseTimer&)=delete;
    CampaignLightPhaseTimer& operator=(const CampaignLightPhaseTimer&)=delete;
    ~CampaignLightPhaseTimer() {finish();}
    void finish() noexcept {
        if(!phase_)return;
        phase_->ns+=campaign_lighting_elapsed(begin_);
        ++phase_->calls;
        phase_=nullptr;
    }
};
template<class F> decltype(auto) time_campaign_light_phase(CampaignLightingPassTimings* timing,CampaignLightPhase phase,F&& work) {
    const CampaignLightPhaseTimer timer(timing,phase);
    return std::forward<F>(work)();
}
}
