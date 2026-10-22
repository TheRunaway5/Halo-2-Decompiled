#include "halo2/physics_platform_call.hpp"
#include "platform/source_process_heap_platform_bindings.hpp"
#include <algorithm>

namespace halo2 {
bool source_try_native_physics_lock_call(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsContactImage image, PhysicsPlatformLocks &locks,
    const PhysicsPlatformLockServices &services, const PhysicsPlatformLockObserver &observe) {
    if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified native physics platform call image");
    const auto &bindings=physics_platform_call_detail::bindings;
    const auto found=std::find_if(bindings.begin(),bindings.end(),[&](const auto &b) {
        return b.instruction==call.instruction && b.target==call.target;
    });
    const auto &initializer=physics_platform_call_detail::heap_initializer_binding;
    const auto *binding=found!=bindings.end()?&*found:
        (initializer.instruction==call.instruction && initializer.target==call.target?&initializer:nullptr);
    if(!binding) return false;
    if(call.tail) throw std::runtime_error("Native physics platform call requires original pushed return");
    std::array<std::byte,8> stack;
    memory.read(abi.gp[4],stack);
    const auto word=[&](unsigned offset) {
        std::uint32_t result=0;
        for(unsigned k=0;k<4;++k) result|=std::to_integer<std::uint32_t>(stack[offset+k])<<(k*8);
        return result;
    };
    const auto expected=image==SourcePhysicsContactImage::MapPack11?binding->return11:binding->return15;
    if(word(0)!=expected) throw std::runtime_error("Native physics platform call return owner mismatch");
    const auto pointer=word(4);
    if(call.target==0x80000123u) locks.initialize(memory,pointer,observe);
    else if(call.target==0x80000115u) locks.enter(memory,pointer,services,observe);
    else locks.leave(memory,pointer,services,observe);
    abi.gp[4]+=8u;
    return true;
}
void source_run_physics_world_with_native_platform(SourcePhysicsContactMemory &memory,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &abi, SourcePhysicsContactImage image,
    SourcePhysicsFactoryFloatingPoint fp, const SourcePhysicsWorldRuntimeServices &services,
    PhysicsPlatformLocks &locks, const PhysicsPlatformLockServices &platform,
    const SourcePhysicsContactRecordObserver &observe, const PhysicsPlatformLockObserver &platform_observe) {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &original_observer) {
        if(source_try_native_physics_lock_call(bank,call,state,image,locks,platform,platform_observe)) return;
        if(!services.required_owner)
            throw std::runtime_error("Required original physics world callee unavailable after native platform join");
        services.required_owner(bank,call,state,original_observer);
    };
    source_run_physics_world_runtime(memory,entry,abi,image,fp,joined,observe);
}
} // namespace halo2
