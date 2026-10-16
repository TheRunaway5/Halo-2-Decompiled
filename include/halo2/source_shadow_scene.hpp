#pragma once
#include "halo2/source_shadow_raw_plan.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/renderer_targets.hpp"
#include <functional>

namespace halo2 {
class SourceLiveShadowPhaseInputsTu15;
// Completed caller facts, in Generate/Apply order. The source GPU target,
// viewport/AA/clip lifecycle owns these values; neither the Mesh nor the
// Vulkan extent supplies them. constants retains the actual main c0..11.
struct SourceShadowScenePhaseInput {
    MenuWorldSourceConstants constants;
    SourceShadowRawRasterInput controls;
    MenuWorldRasterPacket raster;
    std::array<float,4> clip_range;
    std::array<float,2> fog_parameters;
    std::array<float,2> surface_size;
};
// Owns completed opacity bindings and every current c50 palette until GPU
// recording is finished. Moving the root preserves its material addresses.
class SourceShadowSceneRoot {
public:
    SourceShadowSceneRoot()=default;
    SourceShadowSceneRoot(SourceShadowSceneRoot&&) noexcept=default;
    SourceShadowSceneRoot& operator=(SourceShadowSceneRoot&&) noexcept=default;
    SourceShadowSceneRoot(const SourceShadowSceneRoot&)=delete;
    SourceShadowSceneRoot& operator=(const SourceShadowSceneRoot&)=delete;
    SourceShadowRawPlanRoot view() const;
    std::size_t empty_descriptors() const {return empty_descriptors_;}
private:
    friend class SourceShadowScene;
    std::uint32_t owner_=0xffffffffu;
    SourceShadowUniforms uniforms_;
    std::array<std::uint32_t,2> command_masks_{};
    std::vector<std::unique_ptr<SourceShadowMaterial>> materials_;
    std::vector<SourceShadowRawDraw> casters_,receivers_;
    std::size_t empty_descriptors_{};
};
// Immutable raw resources and fixed Generate0/Apply4 pass residency are loaded
// before Renderer construction. This owner never selects roots, runs source
// GPU calls, activates shadow storage or supplies a successful1CAA0 return.
class SourceShadowScene {
public:
    SourceShadowScene(const Cache&,Mesh&,std::size_t bsp,
        std::span<const std::uint32_t> render_models={});
    ~SourceShadowScene();
    SourceShadowScene(const SourceShadowScene&)=delete;
    SourceShadowScene& operator=(const SourceShadowScene&)=delete;
    std::span<const SourceShadowRawPlanMaterial> resident() const;
    // Consume synchronously while this completed root still owns the live
    // model contexts and geometry arena. Subsequent roots/secondary queries
    // may reuse them. Returned bindings copy the current pose and source bytes.
    // uniforms must use actual BEFORE40E80 feature state, not a post-bank guess.
    SourceShadowSceneRoot draw(PhysicsLiveWorldTu15&,const SourceLiveShadowRootTu15&,
        const SourceShadowUniforms&,
        const std::array<SourceShadowScenePhaseInput,2>&) const;
    // Production phase facts are private to their native memory/lease owner.
    // Every actual selected descriptor's pass/class/completed material must
    // pass that owner before its phase bank can reach raw draw construction.
    SourceShadowSceneRoot draw(PhysicsLiveWorldTu15&,const SourceLiveShadowRootTu15&,
        const SourceShadowUniforms&,const SourceLiveShadowPhaseInputsTu15&,
        const RendererTargets::Use&) const;
private:
    using PhaseProvider=std::function<const SourceShadowScenePhaseInput&(
        SourceShadowPhase,const MenuWorldSourcePass&,unsigned,const SourceShadowMaterial&)>;
    using CameraProvider=std::function<std::array<ShaderVector,12>()>;
    SourceShadowSceneRoot draw_with_phase(PhysicsLiveWorldTu15&,const SourceLiveShadowRootTu15&,
        const SourceShadowUniforms&,const PhaseProvider&,const CameraProvider&,
        const std::array<SourceShadowScenePhaseInput,2>* diagnostic_inputs) const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
