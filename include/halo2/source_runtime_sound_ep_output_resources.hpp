#pragma once
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include <array>
#include <memory>
#include <span>

namespace halo2 {
// Prerequisite native byte resources, not a DSP writer, AC97 engine or a
// successful38E75B/38E8C9 result. The source cursor requests precede the later
//38E2E0 BASE/END/SGE construction. The bank and creating executor outlive this
// owner; explicit retirement cancels callback leases before page teardown.
// Retire callbacks before changing/reopening the shared output device.
// While leases exist, every source/physical transaction is enclosed by an
// EP CallScope nested inside the SDK/DMA/reset scopes. An exception must
// cancel/fence EP callbacks before those outer scopes fault or destroy their
// resources. Foreign callbacks never read creating-thread-only DMA/bank state.
// Source writes to the borrowed data pages require callback quiescence.
// Entering CallScope revokes/fences prepared-range leases before source code;
// the existing generic byte-transport contract remains explicitly scoped.
class SourceRuntimeSoundEpOutputResources final {
    struct Core;
    struct PreparedState;
public:
    struct RingLayout {
        std::uint32_t base{},end{},pending_source_cursor{};
        NativeRuntimeAddress native{};
    };
    struct Layout {
        SourceRuntimeSoundDmaResources::Allocation data,scatter_gather;
        std::array<RingLayout,2> rings;
        std::array<std::array<std::uint32_t,2>,6> pending_page_entries;
    };
    struct RingState {
        std::uint32_t read_cursor{},write_cursor{};
        std::size_t queued{},capacity{};
        std::uint64_t native_bytes_written{},native_bytes_read{};
        bool source_cursor_observed{};
    };
    struct State {
        Layout layout;
        std::array<RingState,2> rings;
        std::uint64_t generation{};
        std::size_t callback_leases{},active_copies{};
        std::uint32_t output_device{};
        bool retired{},faulted{},producer_ready{};
    };
    class CallbackLease final {
    public:
        ~CallbackLease();
        CallbackLease(const CallbackLease&)=delete;
        CallbackLease& operator=(const CallbackLease&)=delete;
        // Format-agnostic native byte transport only. No bytes are labelled
        // DSP output or submitted to SDL by this prerequisite. An empty
        // native queue begins at the pending source offset; this is not a
        // claim about the original AC97 consumer position or output latency.
        bool write(unsigned fifo,std::span<const std::byte>);
        bool read(unsigned fifo,std::span<std::byte>);
        RingState state(unsigned fifo) const;
    private:
        friend class SourceRuntimeSoundEpOutputResources;
        CallbackLease(std::shared_ptr<Core>,std::uint64_t);
        std::shared_ptr<Core> core_;
        const std::uint64_t generation_;
        bool registered_{};
    };
    // Read-only prepared bytes, distinct from the generic circular queue.
    // Copying does not consume source PCM, advance a source cursor or submit
    // audio. Cancellation fences every copy before source/page mutation.
    class PreparedRangeLease final {
    public:
        struct State {std::uint32_t offset{},bytes{};NativeRuntimeAddress native{};
            std::uint64_t copies{},bytes_copied{};bool cancelled{},source_pcm_available{},source_started{};};
        ~PreparedRangeLease();
        PreparedRangeLease(const PreparedRangeLease&)=delete;
        PreparedRangeLease& operator=(const PreparedRangeLease&)=delete;
        void copy(std::size_t offset,std::span<std::byte>)const;
        State state()const;
        void cancel(); // Creating executor; stale copies reject after fence.
    private:
        friend class SourceRuntimeSoundEpOutputResources;
        PreparedRangeLease(std::shared_ptr<Core>,std::shared_ptr<PreparedState>,std::uint64_t);
        std::shared_ptr<Core> core_;
        std::shared_ptr<PreparedState> state_;
        const std::uint64_t generation_;
        const std::thread::id thread_=std::this_thread::get_id();
    };
    SourceRuntimeSoundEpOutputResources(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundDmaResources>,std::shared_ptr<SdlSoundPlatform>);
    ~SourceRuntimeSoundEpOutputResources();
    SourceRuntimeSoundEpOutputResources(const SourceRuntimeSoundEpOutputResources&)=delete;
    SourceRuntimeSoundEpOutputResources& operator=(const SourceRuntimeSoundEpOutputResources&)=delete;
    // Only the two authenticated cursor stores are owned. Unknown20xx
    // scheduler stores and the whole source initializer remain unsupported.
    void apply_pending_cursor(std::uint32_t instruction,std::uint32_t address,std::uint32_t value);
    std::shared_ptr<CallbackLease> acquire_native_byte_transport();
    State diagnostics() const;
    bool source_scatter_gather_constructed() const;
    void retire(); // Empty transport or explicitly faulted/cancelled scope.
    bool quiescent() const noexcept;
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundEpOutputResources*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
        CallScope& operator=(const CallScope&)=delete;
    private:
        SourceRuntimeSoundEpOutputResources* owner_;
        int exceptions_;
    };
private:
    friend class SourceRuntimeSoundPcmPreparedRange;
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<Core> core_;
    void require_thread() const;
    void require_live() const;
    void fault_scope() noexcept;
    std::shared_ptr<PreparedRangeLease> acquire_prepared_range(NativeRuntimeAddress,std::uint32_t,std::uint32_t);
    void cancel_prepared_range();
};
}
