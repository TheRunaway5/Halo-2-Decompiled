#pragma once
#include "halo2/temporal_upscaler.hpp"
#include "halo2/path_tracing/geometry.hpp"

namespace halo2 {
// Shared renderer handoff for temporal input and its GPU regression. Deferred
// source spans contain local positions: use the same exact WORLD skinning graph
// already consumed by the Path Tracing scene, and retain its source identities.
struct TemporalGeometry {
    std::vector<Vertex> vertices;
    std::vector<TemporalVertexIdentity> identities;
    void append(std::span<const Vertex> source,std::span<const ModelSkinSubmission> submissions={}) {
        const auto first=vertices.size();
        if(submissions.empty())vertices.insert(vertices.end(),source.begin(),source.end());
        else {
            auto posed=path_tracing::posed_vertices(source,submissions);
            vertices.insert(vertices.end(),posed.begin(),posed.end());
        }
        identities.resize(vertices.size());
        for(const auto& submission:submissions) {
            // posed_vertices validated the complete range and parallel banks.
            const auto& geometry=*submission.geometry;
            for(std::size_t index=0;index<geometry.vertices.size();++index)
                identities[first+submission.first+index]={geometry.owner,geometry.definition,geometry.render_model,
                    geometry.lod,geometry.source_vertices[index]};
        }
    }
};
}
