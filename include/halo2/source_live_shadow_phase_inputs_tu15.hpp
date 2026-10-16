#pragma once
#include "halo2/renderer_targets.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_shadow_scene.hpp"

namespace halo2 {
// Explicit unused residue, not recovered SDK constants. The selected-program
// guard below forbids reads outside the independently completed camera,
// material/root, current palette and source compression banks. FogParams is
// dead in the admitted raw fragment ABI; the completed PSH must not read r3.
struct SourceLiveShadowPhaseResidueTu15 {
    MenuWorldSourceConstants constants;
    std::array<float,2> fog_parameters;
};

// Bounded ordinary640x480 caller owner. It never creates an SDK device, runs
// a source GPU call, constructs target10, sets4922A8 or activates shadows.
// Keep the real memory owner and attachment use alive and unchanged through
// synchronous Scene consumption. A retired/recreated lease or changed source
// camera/clip input invalidates access. No unguarded phase-bank accessor exists.
class SourceLiveShadowPhaseInputsTu15 {
public:
    std::array<ShaderVector,12> camera(const RendererTargets::Use&) const;
    const SourceShadowScenePhaseInput& phase(const RendererTargets::Use&,
        const Cache&,SourceShadowPhase,const MenuWorldSourcePass&,
        unsigned actual_vertex_class,const SourceShadowMaterial& completed) const;
private:
    friend SourceLiveShadowPhaseInputsTu15 source_live_shadow_phase_inputs_tu15(
        const SourceRuntimeMemory&,const RendererTargets::Use&,
        const SourceLiveShadowPhaseResidueTu15&);
    SourceLiveShadowPhaseInputsTu15()=default;
    struct RetainedInput {std::uint32_t address;std::vector<std::byte> bytes;};
    const SourceRuntimeMemory* memory_{};
    std::uint64_t generation_{};
    NativeRuntimeAddress device_,timed_effect_;
    std::uint32_t timed_effect_source_{};
    std::vector<RetainedInput> retained_;
    std::array<SourceShadowScenePhaseInput,2> phases_;
    void validate_live(const RendererTargets::Use&) const;
};

// Requires actual Renderer-owned BGRA8/D24S8 single-sample640x480 attachments,
// ordinary unscaled full source viewport, matching global/submitting camera
// aliases, completed native1FB90/169D0 inputs and retained offscreen-AA zero.
//960/split/timed-boundary/override paths remain explicit rejection corridors.
SourceLiveShadowPhaseInputsTu15 source_live_shadow_phase_inputs_tu15(
    const SourceRuntimeMemory&,const RendererTargets::Use&,
    const SourceLiveShadowPhaseResidueTu15&);
} // namespace halo2
