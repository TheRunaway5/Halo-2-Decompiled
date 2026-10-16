#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include <thread>

namespace halo2 {
// Actual native page resources for original38E71C/3879CB DMA allocation.
// Source CPU code retains every clear, statistics increment and descriptor
// publication. The physical address is a logical native resource namespace,
// never a truncated host pointer or a claim of host PCI/DMA addressability.
// This component does not complete MCPX reset, DSP, HRTF or sound startup.
// Bank and resource lifetimes stay on the creating thread; the bank outlives
// this owner and its retained native pages, including shared callback leases.
class SourceRuntimeSoundDmaResources final {
public:
    struct Allocation {
        std::uint32_t source{},physical{},requested{},capacity{},alignment{},protection{},source_descriptor{};
        NativeRuntimeAddress native{};
    };
    SourceRuntimeSoundDmaResources(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>);
    ~SourceRuntimeSoundDmaResources();
    SourceRuntimeSoundDmaResources(const SourceRuntimeSoundDmaResources&)=delete;
    SourceRuntimeSoundDmaResources& operator=(const SourceRuntimeSoundDmaResources&)=delete;
    bool dispatch(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    std::vector<Allocation> allocations() const;
    NativeRuntimeAddress native_address(std::uint32_t source) const;
    std::uint32_t capacity(std::uint32_t source) const;
    std::uint32_t device_id() const;
    bool quiescent() const noexcept; // This page owner only; output has its own lifetime.
    class CallScope {
    public:
        explicit CallScope(SourceRuntimeSoundDmaResources*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
    private:
        SourceRuntimeSoundDmaResources* owner_;
        std::size_t count_;
        int exceptions_;
    };
private:
    SourceRuntimeMemory& memory_;
    std::shared_ptr<SdlSoundPlatform> output_;
    std::uint32_t device_id_{};
    SourceRuntimeHostPhysicalMemory physical_;
    std::map<std::uint32_t,Allocation> allocations_;
    std::thread::id thread_=std::this_thread::get_id();
    bool fault_{};
    void require_owner() const;
    const Allocation& resolve(std::uint32_t) const;
};
void mount_native_tu15_sound_dma_imports(SourceRuntimeMemory&);
}
