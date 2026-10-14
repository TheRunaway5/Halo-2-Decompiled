#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <thread>
namespace halo2 {
void mount_native_tu15_object_storage_data(SourceRuntimeMemory&);
// Reached CRT formatting boundary only. Source instructions construct every
// pool, chain, binding and header around these formatted constructor names.
class NativeTu15StorageFormatting final {
public:
    explicit NativeTu15StorageFormatting(SourceRuntimeMemory& memory):memory_(memory),thread_(std::this_thread::get_id()){}
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    std::size_t calls() const noexcept{return calls_;}
private:
    SourceRuntimeMemory& memory_;
    std::thread::id thread_;
    std::size_t calls_{};
};
}
