#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <optional>
#include <utility>

namespace halo2 {
// Actual typed factory INPUT, not an original GP/SIMD frame or return value.
struct NativeTu15ContactAgentFactoryInput {
    std::uint32_t owner{},first{},second{},context{},manager{};
    NativeTu15PhysicsFrameFp floating_point;
    unsigned preserved_x87_depth{};
    // Optional source call DATA from the activation parent. These are its
    // declared receiver/pop contract, never a captured GP register image.
    std::optional<std::uint32_t> activation_receiver;
    unsigned activation_return_pop{};
};
using NativeTu15ContactAgentFactoryInputObserver=std::function<void(
    const SourcePhysicsContactMemory&,const NativeTu15ContactAgentFactoryInput&)>;
namespace native_contact_agent_factory_input_detail {
struct Binding {const SourcePhysicsContactMemory*memory;NativeTu15ContactAgentFactoryInputObserver observe;};
inline thread_local Binding*active{};
}
class NativeTu15ContactAgentFactoryInputObserverBinding {
    native_contact_agent_factory_input_detail::Binding binding_;
    native_contact_agent_factory_input_detail::Binding*previous_;
public:
    NativeTu15ContactAgentFactoryInputObserverBinding(const SourcePhysicsContactMemory& memory,
        NativeTu15ContactAgentFactoryInputObserver observer)
        :binding_{&memory,std::move(observer)},previous_(native_contact_agent_factory_input_detail::active){
        native_contact_agent_factory_input_detail::active=&binding_;
    }
    ~NativeTu15ContactAgentFactoryInputObserverBinding(){native_contact_agent_factory_input_detail::active=previous_;}
    NativeTu15ContactAgentFactoryInputObserverBinding(const NativeTu15ContactAgentFactoryInputObserverBinding&)=delete;
};
inline void native_tu15_observe_contact_agent_factory_input(const SourcePhysicsContactMemory& memory,
    const NativeTu15ContactAgentFactoryInput& input){
    const auto*binding=native_contact_agent_factory_input_detail::active;
    if(binding && binding->memory==&memory && binding->observe)binding->observe(memory,input);
}
} // namespace halo2
