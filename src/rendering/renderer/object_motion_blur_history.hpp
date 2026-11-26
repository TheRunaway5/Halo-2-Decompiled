#pragma once

#include "object_motion_blur_pass.hpp"
#include "halo2/path_tracing/geometry.hpp"
#include "halo2/renderer.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace halo2 {

// A view's submitted WORLD positions. Camera motion never enters the history.
class ObjectMotionBlurHistory {
public:
    std::span<const ObjectMotionBlurVertex> update(std::uint64_t frame, bool discontinuity,
        std::span<const ModelObjectDraw> objects, std::span<const ModelObjectDraw> additional = {}) {
        moving_.clear();
        if (discontinuity || !frame_ || frame == 0 || *frame_ != frame - 1)
            previous_.clear();

        std::map<Owner, Pose> current;
        for (const auto draws : {objects, additional}) {
            for (const auto& draw : draws) {
                const auto owner = owner_of(draw);
                if (!owner) continue;
                const auto [entry, inserted] = current.try_emplace(*owner);
                // Repeated submissions under the same source identity are
                // ambiguous even if one happens to have the expected extent.
                if (inserted) entry->second = collect(draw, *owner);
                else entry->second.clear();
            }
        }

        for (const auto& [owner, pose] : current) {
            const auto prior = previous_.find(owner);
            if (prior == previous_.end() || !same_topology(pose, prior->second)) continue;
            for (std::size_t first = 0; first < pose.size(); first += 3) {
                bool moved = false, fully_covered = true;
                for (std::size_t n = first; n < first + 3; ++n) {
                    moved |= !same_position(pose[n].world, prior->second[n].world);
                    fully_covered &= pose[n].stipple == 16;
                }
                if (!moved || !fully_covered) continue;
                for (std::size_t n = first; n < first + 3; ++n)
                    moving_.push_back({pose[n].world, prior->second[n].world});
            }
        }
        previous_ = std::move(current);
        frame_ = frame;
        return moving_;
    }

private:
    static constexpr std::uint32_t absent = 0xffffffffu;

    struct Owner {
        std::uint32_t object{}, definition = absent, model = absent, lod = absent, section = absent;
        auto operator<=>(const Owner&) const = default;
    };
    struct SourceVertex {
        std::uint32_t section{}, part{};
        std::uint16_t part_type{}, part_flags{};
        std::array<std::uint32_t, 3> position{};
        std::array<std::uint32_t, 2> uv{};
        std::uint32_t definition = absent, model = absent, lod = absent, ordinal = absent;
        bool operator==(const SourceVertex&) const = default;
    };
    struct Sample {
        SourceVertex source;
        Vec3 world;
        std::uint32_t stipple{};
    };
    using Pose = std::vector<Sample>;

    static bool same_position(Vec3 a, Vec3 b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    static std::optional<Owner> owner_of(const ModelObjectDraw& draw) {
        if (draw.vertices.empty() || draw.vertices.size() % 3) return {};
        const auto first_section = draw.vertices.front().source_section;
        if (draw.source_model_selection) {
            const auto& selection = *draw.source_model_selection;
            if (selection.object == absent || selection.definition == absent || selection.render_model == absent ||
                (draw.source_object && *draw.source_object != selection.object)) return {};
            return Owner{selection.object, selection.definition, selection.render_model, selection.lod, first_section};
        }
        if (!draw.source_object || *draw.source_object == absent) return {};
        // Some source producers submit one section per draw. A multi-section
        // body uses its single owner key and matches its entire ordered mesh.
        const bool single_section = std::ranges::all_of(draw.vertices, [first_section](const Vertex& v) {
            return v.source_section == first_section;
        });
        return Owner{*draw.source_object, absent, absent, absent, single_section ? first_section : absent};
    }

    static Pose collect(const ModelObjectDraw& draw, const Owner& owner) {
        if (draw.camouflage.value_or(0.f) > 0.f) return {};
        const auto& selection = draw.source_model_selection;
        if (selection && !selection->source_vertices.empty() &&
            selection->source_vertices.size() != draw.vertices.size()) return {};

        auto vertices = draw.vertices;
        std::vector<Vertex> posed;
        if (!draw.deferred_skins.empty()) {
            try {
                posed = path_tracing::posed_vertices(vertices, draw.deferred_skins);
            } catch (const FormatError&) {
                return {};
            }
            vertices = posed;
        }

        Pose pose;
        pose.reserve(vertices.size());
        const auto bits = [](float value) { return std::bit_cast<std::uint32_t>(value); };
        for (std::size_t n = 0; n < vertices.size(); ++n) {
            const auto& v = vertices[n];
            if (v.source_section == absent || v.source_part == absent ||
                !std::isfinite(v.position.x) || !std::isfinite(v.position.y) || !std::isfinite(v.position.z)) return {};
            const auto local = v.source_local_position;
            SourceVertex source{v.source_section, v.source_part, v.source_part_type, v.source_part_flags,
                {bits(local.x), bits(local.y), bits(local.z)}, {bits(v.uv.x), bits(v.uv.y)}};
            if (selection && !selection->source_vertices.empty()) {
                source.ordinal = selection->source_vertices[n];
                if (source.ordinal == absent) return {};
            }
            pose.push_back({source, v.position, v.source_stipple_level});
        }
        for (const auto& submission : draw.deferred_skins) {
            // posed_vertices has checked the submission ranges. Verify that
            // each deferred bank belongs to this draw's salted source owner.
            const auto& geometry = *submission.geometry;
            if (geometry.owner != owner.object || geometry.definition == absent || geometry.render_model == absent ||
                (selection && (geometry.definition != owner.definition || geometry.render_model != owner.model ||
                    geometry.lod != owner.lod))) return {};
            for (std::size_t n = 0; n < geometry.source_vertices.size(); ++n) {
                auto& source = pose[submission.first + n].source;
                const auto ordinal = geometry.source_vertices[n];
                if (ordinal == absent || (source.ordinal != absent && source.ordinal != ordinal)) return {};
                source.definition = geometry.definition;
                source.model = geometry.render_model;
                source.lod = geometry.lod;
                source.ordinal = ordinal;
            }
        }
        return pose;
    }

    static bool same_topology(const Pose& pose, const Pose& previous) {
        return std::ranges::equal(pose, previous, [](const Sample& a, const Sample& b) {
            return a.source == b.source && a.stipple == b.stipple;
        });
    }

    std::optional<std::uint64_t> frame_;
    std::map<Owner, Pose> previous_;
    std::vector<ObjectMotionBlurVertex> moving_;
};

}
