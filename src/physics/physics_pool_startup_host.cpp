#include "halo2/physics_pool_host_storage.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_physics_pool_startup_with_native_storage(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourcePhysicsPoolHostStorage &host,const SourcePhysicsContactRecordObserver &observe) {
  if(!host.backs(m)) throw std::runtime_error("Original physics pool backing identity mismatch");
  auto joined=services;
  joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &original_observer) {
    if(host.try_call(bank,call,state)) return;
    if(!services.required_owner)
      throw std::runtime_error("Required original physics pool callee unavailable after native storage join");
    services.required_owner(bank,call,state,original_observer);
  };
  source_run_physics_pool_startup_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
      fp,joined,observe);
}
} // namespace halo2
