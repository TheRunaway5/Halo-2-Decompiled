#pragma once
#include "halo2/renderer_device_lease.hpp"
#include "halo2/render_program.hpp"
#include "halo2/particle_material.hpp"
#include <functional>
#include <thread>
#include <span>

namespace halo2 {
class Renderer;
class SourceParticleGraphicsBindingTu15;
// Camera-only native upload. No particle geometry, simulation record or draw
// is invented when reserving its real descriptor-backed uniform allocation.
struct RendererParticleViewInput {
    std::shared_ptr<const ParticleMaterial> material;
    std::array<ShaderVector,192> constants{};
    ShaderVector clip_range{},depth_parameters{},surface_fog{};
    std::array<std::int32_t,4> rectangle{};
    unsigned local_view{}; // Submitted ModelView index; not simulation-player registration.
    bool foreground{};
};
// Inputs read from an actual descriptor-backed uniform allocation. Prepared
// resources are distinguished from recorded commands and GPU completion.
struct RendererParticleViewState {
    std::uint64_t generation{},revision{},draw{},pipeline{},layout{},descriptor{},uniform_buffer{};
    NativeRuntimeAddress uniform_backing;
    std::uint64_t vertex_offset{},depth_offset{};
    std::uint32_t surface_width{},surface_height{};
    std::array<float,6> viewport{};
    std::array<std::int32_t,4> scissor{};
    std::array<ShaderVector,4> viewport_constants{};
    std::array<ShaderVector,12> camera_constants{};
    ShaderVector vertex_clip{},depth_clip{},depth_parameters{};
    bool prepared{},published{},recorded{};
    unsigned local_view{};
    bool foreground{};
};
// Only a live renderer issues this view. state() is its immutable input
// snapshot; published_state() reads the produced allocation. A use expires on prepare(),
// attachment recreation, renderer retirement, or a change of owning thread.
// Recording is not a submission/completion guarantee.
class RendererParticleView {
public:
    class Use {
    public:
        bool valid() const;
        const RendererParticleViewState &state() const &;
        const RendererParticleViewState &state() const &&=delete;
        RendererParticleViewState published_state() const;
        NativeRuntimeAddress device_address() const;
    private:
        friend class RendererParticleView;
        friend class SourceParticleGraphicsBindingTu15;
        RendererParticleViewState current_state() const;
        void publish(std::span<const ShaderVector,12>,ShaderVector) const;
        RendererDeviceLease device_;
        RendererParticleViewState state_;
        std::function<bool(std::uint64_t,std::uint64_t)> current_;
        std::function<RendererParticleViewState(std::size_t)> read_;
        std::function<void(std::size_t,std::span<const ShaderVector,12>,ShaderVector)> publish_;
        std::thread::id thread_;
    };
    RendererParticleView()=default;
    bool valid() const;
    Use acquire(std::size_t draw) const;
private:
    friend class Renderer;
    static RendererParticleView issue(RendererDeviceLease,
        std::function<RendererParticleViewState(std::size_t)>,
        std::function<bool(std::uint64_t,std::uint64_t)>,
        std::function<void(std::size_t,std::span<const ShaderVector,12>,ShaderVector)> = {});
    RendererDeviceLease device_;
    std::function<RendererParticleViewState(std::size_t)> observe_;
    std::function<bool(std::uint64_t,std::uint64_t)> current_;
    std::function<void(std::size_t,std::span<const ShaderVector,12>,ShaderVector)> publish_;
    std::thread::id thread_;
};
}
