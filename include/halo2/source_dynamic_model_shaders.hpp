#pragma once
#include "halo2/model.hpp"
#include "halo2/source_model_shader.hpp"
#include <map>

namespace halo2 {
// Resident shader banks for a caller's already selected geometry. The caller
// supplies the original lighting classification (ordinary default2 or3).
// This service does not select
// an owner, pose, geometry LOD, collector ordering or attachment lifetime.
class SourceDynamicModelShaders {
public:
    SourceDynamicModelShaders(const Cache &,RenderModel &,std::uint32_t render_model,
                              unsigned lighting_implementation = 2,
                              std::optional<float> camouflage = {});
    // Vertices remain model-local until the caller composes the actual pose.
    // Attached and first-person callers supply the root's copied collector.
    void select(std::vector<Vertex> &,const SourceModelShaderCollector &,
                const SourceModelShaderView &)const;
    // Apply the identical part/material selection while retaining each
    // expanded source ordinal in parallel. Neither bank may alias the other.
    void select_indexed(std::vector<Vertex>&,std::vector<std::uint32_t>&,
                const SourceModelShaderCollector&,const SourceModelShaderView&)const;
    // Append selected parts from a retained pose without copying that pose
    // into an intermediate mutable vector. Source must not alias output.
    void append_selected(std::vector<Vertex> &output,std::span<const Vertex> source,
                const SourceModelShaderCollector &,const SourceModelShaderView &,
                unsigned material_base = 0)const;
private:
    // (section, part) → selected slot or NONE, in identity order.
    using PartSlots=std::vector<std::pair<std::pair<unsigned,unsigned>,unsigned>>;
    PartSlots select_parts(const SourceModelShaderCollector &,const SourceModelShaderView &)const;
    // Selected slot for one vertex; consecutive vertices of a part reuse the
    // previous lookup. A part absent from the model throws.
    struct PartLookup {
        const PartSlots &slots;
        std::pair<unsigned,unsigned> identity{~0u,~0u};
        unsigned slot{};bool valid{};
        unsigned operator()(const Vertex &);
    };
    struct Part {unsigned source_material{};float scale=1;std::uint32_t shader{};unsigned input_class{};};
    std::map<std::pair<unsigned,unsigned>,Part> parts_;
    std::map<std::uint32_t,SourceShaderLodDefinition> definitions_;
    std::map<std::pair<unsigned,unsigned>,std::vector<unsigned>> slots_;
};
}
