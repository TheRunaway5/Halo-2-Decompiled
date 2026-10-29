#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <memory>
#include <vector>

namespace halo2 {
struct NativeTu15InputWrite {std::uint32_t address;std::vector<std::byte> bytes;};
struct NativeTu15InputCall {
    std::uint32_t instruction,target,entry_esp,return_esp,return_eax;
    std::vector<std::uint32_t> arguments;
    std::vector<NativeTu15InputWrite> writes;
};
// Native instance selection, separate from source handles. Empty/unrestricted
// selects attached SDL game controllers; a restricted list selects only those
// actual instance IDs (also useful for deterministic SDL virtual-device tests).
struct NativeTu15InputSelection {bool restricted=false;std::vector<std::int32_t> instances;};
class NativeTu15InputDevices final {
public:
    explicit NativeTu15InputDevices(SourceRuntimeMemory&,NativeTu15InputSelection = {});
    ~NativeTu15InputDevices();
    NativeTu15InputDevices(const NativeTu15InputDevices&)=delete;
    NativeTu15InputDevices& operator=(const NativeTu15InputDevices&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    std::span<const NativeTu15InputCall> calls()const noexcept;
    // Diagnostic receipts are owned by one caller phase. Production callers
    // discard the previous phase before sampling, keeping storage bounded.
    void clear_calls()noexcept;
    std::size_t live_controllers()const noexcept;
    std::vector<std::uintptr_t> native_controller_addresses()const;
private:
    struct State;std::unique_ptr<State> state_;
};
}
