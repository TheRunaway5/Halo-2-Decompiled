#pragma once
#include "halo2/source_runtime_sound_ep_output_resources.hpp"
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include <array>

namespace halo2 {
// Actual retained record ranges for the five authenticated38E8C9 table-limit
// requests. These are capacities of existing page resources, not working
// voice/HRTF/SGE processing, a hardware register bank, or audio startup ready.
// Spatial128/64 name only the proven record widths; no filter is inferred.
class SourceRuntimeSoundTableLimits final {
public:
    enum class Table { Voice,Spatial128,Spatial64,StreamingScatterGather,InputScatterGather };
    enum class Context { StreamingScatterGather,InputScatterGather,OutputScatterGather };
    struct ContextWindow {
        Context context{};
        std::uint32_t base{},length{}; // Declared address window, not backing.
        bool base_observed{},length_observed{};
    };
    struct Range {
        Table table{};
        SourceRuntimeSoundDmaResources::Allocation allocation;
        std::uint32_t records{},record_bytes{};
        bool source_limit_observed{},source_base_observed{};
    };
    struct RecordView {
        std::uint32_t source{};
        NativeRuntimeAddress native{};
        std::span<const std::byte> bytes;
    };
    struct State {
        std::array<Range,5> ranges;
        std::array<ContextWindow,3> contexts;
        std::uint64_t generation{};
        std::uint32_t output_device{};
        bool retired{},faulted{},configuration_ready{};
    };
    SourceRuntimeSoundTableLimits(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundDmaResources>,
        std::shared_ptr<SourceRuntimeSoundEpOutputResources>,
        SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundTableLimits();
    SourceRuntimeSoundTableLimits(const SourceRuntimeSoundTableLimits&)=delete;
    SourceRuntimeSoundTableLimits& operator=(const SourceRuntimeSoundTableLimits&)=delete;
    // Unknown requests return false without mutation. Known requests require
    // the exact original site/address/value and cannot be applied twice.
    bool apply_limit(std::uint32_t instruction,std::uint32_t address,std::uint32_t value);
    // Exact source SSL/input/output context base/length requests. The window
    // declares valid logical physical addresses; it allocates no dummy pages.
    bool apply_context_bound(std::uint32_t instruction,std::uint32_t address,std::uint32_t value);
    // Five exact38E0CE table-base publications connect the already observed
    // capacities to their actual retained allocations. No method FIFO space
    // or other frontend method is supplied by this bounded resource owner.
    bool apply_physical_base(std::uint32_t instruction,std::uint32_t address,std::uint32_t physical);
    // Borrowed read-only view of the actual native64 record, only after its
    // genuine limit request. Bank/executor outlive this owner; do not retain
    // the span across source writes, reset, retirement or owner destruction.
    RecordView record(Table,std::uint32_t index)const;
    // A view admitted for source processing additionally needs its genuine
    // physical-base publication. This does not execute voices or SGE records.
    RecordView bound_record(Table,std::uint32_t index)const;
    // Resolve only a single actually owned retained DMA allocation inside the
    // fully observed context window. Unmapped/crossing requests fail closed.
    // This is an address resource, not execution or validation of SGE contents.
    RecordView resolve_physical(Context,std::uint32_t physical,std::uint32_t bytes)const;
    // Authenticate the exact retained worker before mutable/page access.
    void require_control(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    State diagnostics()const;
    void retire();
    bool quiescent()const noexcept;
    // Nest inside EP/SDK/DMA/reset scopes while executing source CPU work.
    // A partial source transaction faults this capacity owner; source stores
    // remain diagnostics and the parent must discard that cold assembly.
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundTableLimits*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
    private:
        SourceRuntimeSoundTableLimits* owner_;
        int exceptions_;
    };
private:
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma_;
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const std::uint64_t generation_;
    const std::uint32_t device_;
    std::array<Range,5> ranges_;
    std::array<ContextWindow,3> contexts_{};
    bool retired_{},fault_{};
    void require_thread()const;
    void require_live()const;
};
}
