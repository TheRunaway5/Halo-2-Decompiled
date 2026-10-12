#pragma once
#include "halo2/model.hpp"
#include "halo2/transform.hpp"
#include <cfenv>
#include <cstring>
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace halo2 {
// The model's source vertices, skin bindings and hierarchy stay immutable and
// outlive this cache. Always sample current WORLD nodes before calling update.
// Returned storage stays valid until this cache is updated or destroyed.
class ModelWorldPoseCache {
    const RenderModel *model_{};
    std::vector<Transform> nodes_;
    std::vector<Vertex> vertices_;
    int rounding_{};
    unsigned mxcsr_{};
public:
    struct Result {std::span<const Vertex> vertices;bool reused{};};
    Result update(const RenderModel &model,std::span<const Transform> nodes) {
        const auto rounding=std::fegetround();unsigned mxcsr=0;
#if defined(__SSE2__) || defined(_M_X64)
        // Ignore sticky flags; preserve rounding, exception masks, DAZ and FTZ.
        mxcsr=_mm_getcsr()&0xffc0u;
#endif
        if(model_==&model && rounding_==rounding && mxcsr_==mxcsr && nodes_.size()==nodes.size() &&
            (nodes.empty() || !std::memcmp(nodes_.data(),nodes.data(),nodes.size_bytes())))
            return {vertices_,true};
        // Publish only a complete successful skin; failed inputs leave the
        // preceding immutable result and its key together.
        auto vertices=skin_model_world_nodes(model,nodes);
        std::vector<Transform> retained(nodes.begin(),nodes.end());
        vertices_=std::move(vertices);nodes_=std::move(retained);
        model_=&model;rounding_=rounding;mxcsr_=mxcsr;
        return {vertices_,false};
    }
};
}
