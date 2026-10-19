#pragma once
#include "halo2/source_transparent_sort.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include <memory>
namespace halo2 {
class PhysicsLiveWorldTu15;
// Complete owned 4C2AA8 queue after main40E80, before any drain/query reuse.
std::vector<SourceTransparentCommand> source_live_transparent_queue_tu15(
    PhysicsLiveWorldTu15 &);
struct SourceLiveTransparentLayerTu15 {
    std::uint32_t layer{};
    // Original3BAA0 descriptors, including untouched retained bytes.
    std::vector<std::array<std::uint32_t,6>> descriptors;
};
struct SourceLiveTransparentGroupGeometryTu15 {
    // Actual post-44710 native main units. Earlier base selections retain
    // palettes/owners but precede this cull and its mask-address relocation.
    std::vector<SourceCullDrawUnit> units;
    SourceBspRangeArena arena;
    std::uint32_t arena_address{};
};
struct SourceLiveTransparentGroupTu15 {
    SourceTransparentCommand queue_command;
    std::uint32_t section{},flags{},first_command{},end_command{},layers{};
    std::vector<SourcePartCommand> commands;
    double shader_clock{}; // Actual published4920C0 double, before query reuse.
    std::vector<SourceLiveTransparentLayerTu15> ordered_layers;
    std::shared_ptr<const SourceLiveTransparentGroupGeometryTu15> geometry;
};
// Collect actual queued423B0 groups over the completed main handoff, before
// secondary queries reuse its model/geometry banks. The copied command range
// belongs to this group; earlier ordinary commands retain their original bytes.
// This projects only423B0's collection corridor and executes whole44C80 with
// actual45CD0/41030 owners, then whole3BAA0/4CBE0 metadata selection/sorting
// for the callback's mask/gate-admitted layers. It does not execute scene copy,
// GPU layer setup/flush or substitute success for any graphics owner.
std::vector<SourceLiveTransparentGroupTu15> source_live_transparent_groups_tu15(
    PhysicsLiveWorldTu15 &);
}
