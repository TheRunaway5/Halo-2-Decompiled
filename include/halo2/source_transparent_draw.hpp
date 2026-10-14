#pragma once
#include "halo2/cache.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_transparent_sort.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include <variant>

namespace halo2 {
// Completed41020/40F60 geometry service. The resident material must bind the
// selected source shader's LOD0/layer15/implementation0. Pose and per-owner
// animation are already resolved; the renderer never constructs a collector.
struct SourceTransparentGeometryDraw {
    std::uint32_t shader=0xffffffffu,draw_token{};
    std::span<const Vertex> vertices;
    std::optional<ObjectLightingState> lighting;
    std::uint32_t render_cache_flags{}; //Actual0x800 selects12D50 FP projection.
};
// Completed41490 shader callback and40F60 pass dispatch. Unlike ordinary
//41020 part commands, the callback identity belongs to the submitting engine
//adapter; payload words remain selected shader and actual context token.
//The caller owns the completed geometry/material service through this draw.
struct SourceTransparentShaderGeometryDraw {
    std::uint32_t callback{};
    SourceTransparentGeometryDraw geometry;
};
// Completed173C90/174F80 service. One source location can emit several sprite
// draws. The range refers to this ModelView's world/foreground particle bank.
struct SourceTransparentParticleRange {
    std::uint32_t system{},location{};
    unsigned first{},count{};
};
struct SourceContrailDraw;
struct SourceTransparentContrailDraw {const SourceContrailDraw *draw{};};
struct SourceTransparentLightVolumeDraw {const SourceLightVolumeDraw *draw{};};
using SourceTransparentDrawBinding=std::variant<std::monostate,
    SourceTransparentGeometryDraw,SourceTransparentParticleRange,SourceTransparentShaderGeometryDraw,SourceTransparentLightVolumeDraw,SourceTransparentContrailDraw>;
// Snapshot of one1E370 drain, before40870 restores count. Bindings are indexed
// by absolute command ordinal, including the prefix and null callbacks. Every
// nonnull callback in[first,count) requires its actual completed service.
// Group callbacks41CC0/423C0 remain unsupported rather than silently skipped.
struct SourceTransparentDrawBank {
    std::span<const SourceTransparentCommand> commands;
    unsigned first{};
    std::span<const SourceTransparentDrawBinding> bindings;
};
}
