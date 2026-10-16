#pragma once
#include "halo2/source_runtime_sound_pcm_buffers.hpp"

namespace halo2 {
// A bounded paused native byte resource. The authentic EP pending cursor600
// and descriptor start0 select a prepared range; they do not establish valid
// queued DSP PCM, a running consumer, output latency or source availability.
// Analog/default-SPDIF mirror one physical EP0 domain. A borrower may copy
// these bytes, but cannot submit, write or advance source/native ring cursors.
class SourceRuntimeSoundPcmPreparedRange final {
public:
    using Kind=SourceRuntimeSoundPcmBuffers::Kind;
    using Lease=SourceRuntimeSoundEpOutputResources::PreparedRangeLease;
    struct State {Kind kind{};std::uint32_t descriptor_start_offset{},pending_source_cursor{},bytes{};
        NativeRuntimeAddress native{};bool retired{},source_pcm_available{},source_started{},producer_ready{};};
    SourceRuntimeSoundPcmPreparedRange(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundPcmBuffers>,Kind);
    ~SourceRuntimeSoundPcmPreparedRange();
    SourceRuntimeSoundPcmPreparedRange(const SourceRuntimeSoundPcmPreparedRange&)=delete;
    SourceRuntimeSoundPcmPreparedRange& operator=(const SourceRuntimeSoundPcmPreparedRange&)=delete;
    std::shared_ptr<Lease> borrow()const; // Callback copy API; no source reads.
    State diagnostics()const;
    void retire(); // Creating executor, cancels/fences borrowed copies.
    bool quiescent()const noexcept;
private:
    const std::thread::id thread_=std::this_thread::get_id();
    const Kind kind_;
    NativeRuntimeAddress native_{};
    std::shared_ptr<SourceRuntimeSoundPcmBuffers> buffers_;
    std::shared_ptr<Lease> lease_;
    bool retired_{};
    void require_thread()const;
};
}
