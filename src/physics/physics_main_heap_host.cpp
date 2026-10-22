#include "halo2/physics_main_heap_runtime.hpp"
#include <stdexcept>
namespace halo2 {
void source_run_physics_main_heap_with_native_storage(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    SourcePhysicsPoolHostStorage &host,const SourcePhysicsContactRecordObserver &observe) {
  if(!host.backs(m))throw std::runtime_error("Original main heap backing identity mismatch");
  auto joined=services;
  joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
    if(host.try_call(bank,call,state))return;
    if(call.instruction==0x1482b0u && call.target==0x33b3acu && !call.tail) {
      source_run_physics_pool_startup_runtime(bank,0x33785c,state,
          SourcePhysicsContactImage::TitleUpdate15,fp,joined,observer);
      return;
    }
    if(!services.required_owner)
      throw std::runtime_error("Required original main heap callee unavailable after native storage join");
    services.required_owner(bank,call,state,observer);
  };
  if(entry==0x1482a0u) {
    source_run_physics_main_heap_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
        fp,joined,observe);return;
  }
  switch(entry) {
    case 0x148190:case 0x22e4e0:case 0x2dd590:case 0x22e950:case 0x22eb20:
    case 0x148a10:case 0x148a70:case 0x22e1d0:case 0x22e380:case 0x22e400:
    case 0x22e420:case 0x22e430:
      source_run_physics_world_runtime(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
          fp,joined,observe);return;
    default:throw std::runtime_error("Unverified original main heap source entry");
  }
}
} // namespace halo2
