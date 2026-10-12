#pragma once
#include "halo2/model.hpp"
#include "halo2/source_model_shader.hpp"
#include <map>

namespace halo2 {
// Literal41040 override compatibility. The selected ordinal comes from the
// override's1BF50, and indexes the original bank without reselection/clamping.
// Invalid banks are an unresolved resident-input contract, not a rejected draw.
bool source_model_shader_override_part(std::uint16_t part_type,
    std::uint32_t original_shader,std::int32_t selected_override_lod,
    std::span<const std::uint32_t> original_layers,
    std::span<const std::uint32_t> override_layers);

struct SourceShieldPartBinding {
    unsigned material{};
    std::uint32_t shader=0xffffffffu,selected_template_layers{};
    std::int32_t selected_lod{};
    std::uint16_t shader_priority_flags{},shader_command_flags{};
};
// Resident actual plasma-shield override, over the caller's copied accepted
// posed geometry. This owns material/input banks and41040's part admission;
// it does not choose geometry, clone an owner, or schedule transparent draws.
class SourceShieldModelShaders {
public:
    SourceShieldModelShaders(const Cache &,RenderModel &,std::uint32_t render_model,
                            std::uint32_t shield_shader);
    // Separate resident output bank for a submitting renderer. The accepted
    // source model and its ordinary materials remain owned by that renderer.
    SourceShieldModelShaders(const Cache &,const RenderModel &,std::uint32_t render_model,
                            std::uint32_t shield_shader,std::vector<Material> &output);
    std::optional<SourceShieldPartBinding> binding(unsigned section,unsigned part,
        const SourceModelShaderCollector &,const SourceModelShaderView &)const;
    void select(std::vector<Vertex> &,const SourceModelShaderCollector &,
                const SourceModelShaderView &)const;
private:
    struct Part {
        std::uint16_t type{};
        std::uint32_t original_shader=0xffffffffu;
        std::vector<std::uint32_t> original_layers;
        float scale=1;
        unsigned material{};
    };
    std::map<std::pair<unsigned,unsigned>,Part> parts_;
    SourceShaderLodDefinition definition_;
    std::vector<std::uint32_t> layers_;
    std::uint32_t shader_=0xffffffffu;
    std::uint16_t priority_{},commands_{};
};
}
