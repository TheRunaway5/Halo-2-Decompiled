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
// GP pool caller contexts must outlive their allocations. Do not replace the
// bank or those context bindings while GP pool resources remain retained.
class SourceRuntimeSoundDmaResources final {
public:
    struct Allocation {
        std::uint32_t source{},physical{},requested{},capacity{},alignment{},protection{},source_descriptor{};
        NativeRuntimeAddress native{};
    };
    // Actual page-backed DSad pool storage requested by original38DE81.
    // Physical values use the same native namespace as the DMA descriptors;
    // they are never truncated host pointers or original Xbox addresses.
    struct GpPoolAllocation {
        std::uint32_t source{},physical{},requested{},capacity{},context{};
        NativeRuntimeAddress native{};
        bool locked{};
    };
    SourceRuntimeSoundDmaResources(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>);
    ~SourceRuntimeSoundDmaResources();
    SourceRuntimeSoundDmaResources(const SourceRuntimeSoundDmaResources&)=delete;
    SourceRuntimeSoundDmaResources& operator=(const SourceRuntimeSoundDmaResources&)=delete;
    bool dispatch(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    std::vector<Allocation> allocations() const;
    std::vector<GpPoolAllocation> gp_pool_allocations() const;
    // Synchronous creating-executor access to a single locked, actually
    // retained GP pool range. This is not a DMA scheduler or device-ready
    // result; unmapped/unlocked/crossing physical requests remain unsupported.
    void read_gp_pool(std::uint32_t physical,std::span<std::byte>) const;
    void write_gp_pool(std::uint32_t physical,std::span<const std::byte>);
    struct GpScratchBinding {
        std::uint32_t context{},table_source{},table_physical{},table_bytes{};
        std::optional<std::uint32_t> entries;
    };
    // Native resource publication by the actual original GP table stores.
    // An absent base/count stays absent; no MMIO preimage, hardware status,
    // GP acknowledgement, scheduler or processor-ready result is provided.
    void publish_gp_scratch(std::uint32_t context,std::uint32_t instruction,
        std::uint32_t address,std::uint32_t value);
    std::optional<GpScratchBinding> gp_scratch_binding() const;
    // Synchronous scatter/gather access to those exact retained pages. Table,
    // descriptors, context, backing and locks are checked on every operation.
    void read_gp_scratch(std::uint32_t offset,std::span<std::byte>) const;
    void write_gp_scratch(std::uint32_t offset,std::span<const std::byte>);
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
        std::uint64_t gp_epoch_{};
        int exceptions_;
    };
private:
    SourceRuntimeMemory& memory_;
    std::shared_ptr<SdlSoundPlatform> output_;
    std::uint32_t device_id_{};
    SourceRuntimeHostPhysicalMemory physical_;
    std::map<std::uint32_t,Allocation> allocations_;
    std::map<std::uint32_t,GpPoolAllocation> gp_pools_;
    std::optional<GpScratchBinding> gp_scratch_;
    std::uint64_t gp_pool_epoch_{};
    std::thread::id thread_=std::this_thread::get_id();
    bool fault_{};
    void require_owner() const;
    const Allocation& resolve(std::uint32_t) const;
    const GpPoolAllocation& resolve_gp_pool(std::uint32_t) const;
    bool dispatch_gp_pool(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool dispatch_gp_scratch_translation(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const Allocation& gp_descriptor(std::uint32_t descriptor) const;
    std::uint32_t gp_page_source(std::uint32_t context,std::uint32_t physical) const;
    struct GpScratchSpan {std::uint32_t source;std::size_t bytes;};
    std::vector<GpScratchSpan> gp_scratch_spans(const GpScratchBinding&,
        std::uint32_t offset,std::size_t bytes) const;
};
void mount_native_tu15_sound_dma_imports(SourceRuntimeMemory&);
}
