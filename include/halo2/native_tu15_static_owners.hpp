#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// The three CRT owners used by fresh world construction: a static rigid-body
// deactivator, borrowed Havok scratch storage, and engine-class registration.
// All source pointers below remain opaque source identities. Native access
// uses checked addresses; no executable or captured constructor state is read.
void mount_native_tu15_static_data(SourceRuntimeMemory&);
class NativeTu15StaticOwners final {
public:
    explicit NativeTu15StaticOwners(SourceRuntimeMemory&,
        const SourcePhysicsContactRecordObserver& = {});
    ~NativeTu15StaticOwners();
    NativeTu15StaticOwners(const NativeTu15StaticOwners&)=delete;
    NativeTu15StaticOwners& operator=(const NativeTu15StaticOwners&)=delete;
    NativeTu15StaticOwners(NativeTu15StaticOwners&&)=delete;
    NativeTu15StaticOwners& operator=(NativeTu15StaticOwners&&)=delete;
    // Execute the actual source callbacks in reverse registration order.
    // Explicit close exposes a failure before enclosing bank teardown.
    void close();
    NativeRuntimeAddress scratch()const noexcept{return scratch_;}
    NativeRuntimeAddress deactivator()const noexcept{return deactivator_;}
    NativeRuntimeAddress engine_table()const noexcept{return engine_table_;}
private:
    SourceRuntimeMemory& memory_;
    SourcePhysicsContactRecordObserver observe_;
    std::vector<std::uint32_t> exits_;
    NativeRuntimeAddress scratch_,deactivator_,engine_table_;
    SourcePhysicsWorldRuntimeServices services();
    void run(std::uint32_t);
};
}
