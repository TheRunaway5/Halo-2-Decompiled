#include "halo2/renderer_particle_view.hpp"
#include <stdexcept>
namespace halo2 {
RendererParticleView RendererParticleView::issue(RendererDeviceLease device,
    std::function<RendererParticleViewState(std::size_t)> observe,
    std::function<bool(std::uint64_t,std::uint64_t)> current,
    std::function<void(std::size_t,std::span<const ShaderVector,12>,ShaderVector)> publish) {
    const auto use=device.acquire();
    if(!observe || !current)throw std::invalid_argument("Particle view has no native observer");
    RendererParticleView result;result.device_=std::move(device);
    result.observe_=std::move(observe);result.current_=std::move(current);
    result.publish_=std::move(publish);
    result.thread_=std::this_thread::get_id();return result;
}
bool RendererParticleView::valid() const {
    return bool(observe_) && bool(current_) && device_.valid() && thread_==std::this_thread::get_id();
}
RendererParticleView::Use RendererParticleView::acquire(std::size_t draw) const {
    if(!valid())throw std::runtime_error("Particle view has no live renderer/thread owner");
    const auto owner=device_.acquire();Use use;use.device_=device_;use.state_=observe_(draw);
    use.current_=current_;use.read_=observe_;use.publish_=publish_;use.thread_=thread_;
    if(!use.state_.pipeline || !use.state_.layout || !use.state_.descriptor || !use.state_.uniform_buffer ||
       !use.state_.uniform_backing.value || !use.state_.surface_width || !use.state_.surface_height || !use.valid())
        throw std::runtime_error("Particle view lacks recorded descriptor-backed native inputs");
    return use;
}
bool RendererParticleView::Use::valid() const {
    return bool(current_) && device_.valid() && thread_==std::this_thread::get_id() &&
        current_(state_.generation,state_.revision);
}
const RendererParticleViewState &RendererParticleView::Use::state() const & {
    if(!valid())throw std::runtime_error("Particle view uniform/target/thread owner expired");
    return state_;
}
NativeRuntimeAddress RendererParticleView::Use::device_address() const {
    if(!valid())throw std::runtime_error("Particle view uniform/target/thread owner expired");
    return device_.acquire().native_address();
}
RendererParticleViewState RendererParticleView::Use::current_state() const {
    if(!valid() || !read_ || !state_.prepared)throw std::runtime_error("Particle view has no live prepared resource");
    auto result=read_(state_.draw);
    if(!valid() || !result.prepared || result.recorded ||
       result.uniform_buffer!=state_.uniform_buffer || result.uniform_backing!=state_.uniform_backing ||
       result.descriptor!=state_.descriptor || result.pipeline!=state_.pipeline || result.layout!=state_.layout ||
       result.local_view!=state_.local_view || result.foreground!=state_.foreground)
        throw std::runtime_error("Particle view has no complete original publication in its actual allocation");
    return result;
}
RendererParticleViewState RendererParticleView::Use::published_state() const {
    auto result=current_state();
    if(!result.published)throw std::runtime_error("Prepared particle view has no complete original publication");
    return result;
}
void RendererParticleView::Use::publish(std::span<const ShaderVector,12> camera,ShaderVector clip) const {
    if(!valid() || !state_.prepared || !publish_)throw std::runtime_error("Particle view has no prepared publication owner");
    publish_(state_.draw,camera,clip);
    if(!valid())throw std::runtime_error("Particle view retired during native publication");
}
}
