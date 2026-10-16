#include "halo2/renderer_particle_program.hpp"
#include <stdexcept>
namespace halo2 {
RendererParticleProgram RendererParticleProgram::issue(RendererDeviceLease device,
    std::function<RendererParticleProgramConfiguration(std::shared_ptr<const ParticleMaterial>)> select) {
    const auto use=device.acquire();if(!select)throw std::invalid_argument("Native particle program implementation is unavailable");
    RendererParticleProgram result;result.device_=std::move(device);result.select_=std::move(select);result.thread_=std::this_thread::get_id();return result;
}
bool RendererParticleProgram::valid() const {return bool(select_) && device_.valid() && thread_==std::this_thread::get_id();}
NativeRuntimeAddress RendererParticleProgram::device_address() const {
    if(!valid())throw std::runtime_error("Native particle program renderer/thread has retired");
    return device_.acquire().native_address();
}
RendererParticleProgramConfiguration RendererParticleProgram::select(std::shared_ptr<const ParticleMaterial> material) const {
    if(!valid() || !material)throw std::runtime_error("Native particle program has no live renderer/thread/material owner");
    const auto use=device_.acquire();return select_(std::move(material));
}
}
