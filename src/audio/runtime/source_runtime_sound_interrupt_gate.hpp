#pragma once
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include <stdexcept>

namespace halo2 {
// Internal shared state consulted by the actual retained EventPort dispatcher.
// This is a disabled source delivery gate, not a register image or route-ready
// declaration. Actual port retirement invalidates the borrowed dispatcher ID.
struct SourceRuntimeSoundInterruptGate final {
    SourceRuntimeMemory* const memory;
    const SourceRuntimeSoundResetResources::ControlLease control;
    const std::uint64_t generation;
    const std::thread::id thread=std::this_thread::get_id();
    int dispatcher;
    std::uint32_t pending_mask{};
    bool prepared{},retired{},faulted{};
    SourceRuntimeSoundInterruptGate(SourceRuntimeMemory* bank,
        SourceRuntimeSoundResetResources::ControlLease lease,std::uint64_t epoch,int native)
        :memory(bank),control(std::move(lease)),generation(epoch),dispatcher(native){}
    void require_thread()const{
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("Native source interrupt policy requires its creating owner thread");
    }
    void require_live()const{
        require_thread();if(retired||faulted||dispatcher<0||!generation)
            throw std::runtime_error("Native source interrupt policy lost its retained dispatcher");
        control.require_bank(*memory);if(control.generation()!=generation)
            throw std::runtime_error("Native source interrupt policy belongs to a retired actual control generation");
    }
    bool blocks_source_irq()const{require_thread();return retired||faulted||!(pending_mask&1);}
};
}
