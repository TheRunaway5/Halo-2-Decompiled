#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <functional>
namespace halo2 {
class SourceRuntimeMemory;
using SourceShadowAdmissionObserverTu15=std::function<void(bool,std::uint32_t,std::uint32_t,
    const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory&)>;
}
