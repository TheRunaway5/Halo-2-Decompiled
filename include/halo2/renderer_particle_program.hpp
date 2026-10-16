#pragma once
#include "halo2/renderer_device_lease.hpp"
#include "halo2/particle_material.hpp"
#include <functional>
#include <thread>
namespace halo2 {
class Renderer;
struct RendererParticleProgramConfiguration {
    std::uint64_t pipeline{},vertex_module{},layout{},program_hash{},target_generation{};
    std::uint32_t classification{},instructions{};
    NativeRuntimeAddress state_backing;
};
// A live renderer alone issues this program residency control. Selection
// realizes the same validated source-material pipeline used by record(). It
// stages a native program interface; it does not claim a GPU draw or Xbox SDK
// current-shader/boot history. Target generation is retained by its borrower.
class RendererParticleProgram {
public:
    RendererParticleProgram()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererParticleProgramConfiguration select(std::shared_ptr<const ParticleMaterial>) const;
private:
    friend class Renderer;
    static RendererParticleProgram issue(RendererDeviceLease,
        std::function<RendererParticleProgramConfiguration(std::shared_ptr<const ParticleMaterial>)>);
    RendererDeviceLease device_;
    std::function<RendererParticleProgramConfiguration(std::shared_ptr<const ParticleMaterial>)> select_;
    std::thread::id thread_;
};
}
