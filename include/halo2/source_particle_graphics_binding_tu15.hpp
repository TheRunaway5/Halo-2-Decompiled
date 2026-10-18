#pragma once
#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/renderer_targets.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include "halo2/renderer_particle_program.hpp"
#include "halo2/source_particle_declaration_tu15.hpp"
#include "halo2/renderer_particle_view.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/source_particle_view_pass_tu15.hpp"

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
    // Native explicit programmable bounds, backed by an actually recorded
    // draw's descriptor inputs. Whole13430 owns reset/control/bound packets.
    // Call before constructing this view's queues. This does not reconstruct
    // original SDK boot, fixed-function projection or prior-frame history.
    void initialize_view(RendererParticleView::Use,SourcePhysicsWorldRuntimeAbi &,
        const SourcePhysicsWorldRuntimeServices & = {});
    // Ordinary2B9F0 publishes132B0/12F80 before13660/2BCB0 resets the view.
    // Stage the actual resource without resetting queues. Its camera remains
    // unavailable to drawing until both original producers have completed.
    void prepare_view(RendererParticleView::Use);
    // Native value seam for the authenticated2BCB0 packet producer, followed
    // by complete13430. ECX names real writable caller packet storage. The
    // serial comes from source4C685C; pass scheduling/queue retirement remain
    // the caller's responsibility, not inferred from this packet.
    SourceParticleViewPassTu15 initialize_view_pass(RendererParticleView::Use,
        std::uint32_t boundary_mode,std::uint32_t boundary_distance_bits,
        SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsWorldRuntimeServices & = {});
    SourceParticleViewPassTu15 initialize_prepared_view_pass(
        std::uint32_t boundary_mode,std::uint32_t boundary_distance_bits,
        SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsWorldRuntimeServices & = {});
    // Declared native complete12F80 caller: EAX names its actual projection
    // bank, ECX its submitting camera, stack has scaling/mode arguments. The
    // complete emitted c0..11 bank must match the recorded descriptor inputs.
    // This does not establish the original frame's current-camera history.
    void publish_camera(SourcePhysicsWorldRuntimeAbi &,
        const SourcePhysicsWorldRuntimeServices & = {});
    void consume_draw(ParticleDraw &) const;
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
    void stage_view(RendererParticleView::Use);
    void require_empty_view_queues() const;
    void check_view_initialization() const;
    void initialize_staged_view(SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsWorldRuntimeServices &);
    void decode_constants(std::span<ShaderVector,192>,bool pending_camera) const;
    std::size_t last_bounds_packet() const;
    void publish_view_resource();
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
    std::optional<RendererParticleView::Use> view_;
    bool view_initializing_{},view_initialized_{},view_attempted_{},control_cache_owned_{};
    std::size_t view_packet_begin_{},view_packet_end_{};
    bool camera_initializing_{},camera_attempted_{},camera_configured_{};
    std::size_t camera_bounds_begin_{},camera_bounds_end_{};
    std::uint32_t instruction_{};
    const std::thread::id thread_=std::this_thread::get_id();
};
}
