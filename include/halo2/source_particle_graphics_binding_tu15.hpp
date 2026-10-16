#pragma once
#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/renderer_targets.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include "halo2/renderer_particle_program.hpp"
#include "halo2/source_particle_declaration_tu15.hpp"

namespace halo2 {
// Whole original format normalization/layout/depth-scale arithmetic for the
// admitted native D24S8 surface. These are metadata values, not Xbox physical
// allocation addresses or a claim of the original406A10 constructor history.
struct SourceParticleDepthLayoutTu15 {
    std::uint32_t width{},height{},normalized_format{},format_word{},size_word{},row_pitch{},storage_bytes{},depth_scale_bits{};
};
SourceParticleDepthLayoutTu15 source_particle_d24s8_layout_tu15(std::uint32_t,std::uint32_t);

// A live native renderer owns this small source graphics ABI view. The
// original3FF361 device alias points at the same actual command allocation
// as4012B0/401410. This is not the original405CF0 constructor or a complete
// Xbox SDK device: every remaining device field is explicitly unavailable.
class SourceParticleGraphicsBindingTu15 final : public SourcePhysicsContactMemory {
public:
    SourceParticleGraphicsBindingTu15(RendererDeviceLease,RendererConstantMode,RendererTargets::Use);
    void begin(SourcePhysicsContactMemory &,std::span<const ShaderVector,192>);
    // Declared native SDK caller; EAX must carry the actual selected mode.
    // Complete400EF0 owns the source flags/mode writes, not a supplied return.
    void configure(SourcePhysicsWorldRuntimeAbi &,
        const std::function<void(std::uint32_t)> &instruction = {});
    // Bind the actual current D24S8 attachment and realized native W mode.
    // Its native identity supports admitted presence tests and the format
    // fragment built by whole original layout arithmetic. Other Xbox object
    // fields and physical addresses remain unavailable.
    void configure_depth(RendererDepthMode);
    const RendererDepthConfiguration &depth_configuration() const;
    const SourceParticleDepthLayoutTu15 &depth_layout() const;
    // Declared native program selection, backed by the actual resident
    // source-material pipeline. Only its original programmable-kind fragment
    // is exposed; no SDK declaration or current-shader boot history is forged.
    void stage_program(RendererParticleProgram,std::shared_ptr<const ParticleMaterial>);
    // Declared native raw-program caller: stack arguments name actual source
    // code and slot0. Whole400D40/400C50 own cache/packet emission. The packet
    // consumer verifies every instruction before realizing the native pipeline.
    void load_program(RendererParticleProgram,std::shared_ptr<const ParticleMaterial>,
        SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsWorldRuntimeServices & = {});
    const RendererParticleProgramConfiguration &program_configuration() const;
    // Whole original declaration arithmetic uses the actual selected tag.
    // Whole400DD0/400E30 publish its same-kind header and slot0 start packet.
    // Packed input metadata does not bind an Xbox vertex-buffer allocation.
    void configure_program_declaration(SourcePhysicsWorldRuntimeAbi &,
        const SourcePhysicsWorldRuntimeServices & = {});
    const SourceParticleDeclarationTu15 &program_declaration() const;
    void run(std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
        const SourcePhysicsWorldRuntimeServices & = {});
    bool configured() const;
    NativeRuntimeAddress device_address() const;
    void read(std::uint32_t,std::span<std::byte>) const override;
    void write(std::uint32_t,std::span<const std::byte>) override;
    std::span<const std::byte> commands() const;
    void consume_constants(std::span<ShaderVector,192>) const;
    void visit_committed(const std::function<void(std::uint32_t,
        std::span<const std::byte>)> &) const;
private:
    void check(bool require_configuration=true) const;
    bool owns_field(std::uint32_t,std::size_t) const;
    void reject_unknown_device_field(std::uint32_t,std::size_t) const;
    RendererDeviceLease renderer_;
    RendererDeviceLease::Use use_;
    RendererConstantMode control_;
    RendererTargets::Use targets_;
    RendererDepthMode depth_control_;
    RendererParticleProgram program_control_;
    RendererParticleProgramConfiguration program_configuration_;
    RendererDepthConfiguration depth_configuration_;
    SourceParticleDepthLayoutTu15 depth_layout_;
    RendererConstantConfiguration configuration_;
    SourceParticleConstantStorageTu15 constants_;
    SourceRuntimeMemory fields_;
    bool begun_{},configured_{},configuring_{},depth_configured_{},program_staged_{};
    bool program_loading_{},program_load_attempted_{};
    std::size_t program_packet_begin_{},program_packet_end_{};
    std::vector<std::byte> program_code_;
    ParticleVertexSelection selected_vertex_;
    SourceParticleDeclarationTu15 declaration_;
    bool declaration_configuring_{},declaration_attempted_{},declaration_configured_{};
    std::size_t declaration_packet_begin_{},declaration_packet_end_{};
    std::uint32_t instruction_{};
    const std::thread::id thread_=std::this_thread::get_id();
};
}
