#pragma once
#include "halo2/source_script_sound_clock_tu15.hpp"
#include <memory>

namespace halo2 {
class MultiplayerAudio;
class SourceScriptSoundClockNative64Tu15;

// A genuine platform/voice ownership receipt. Only the actual Audio adapter
// can create one, after packet acceptance and publication of its voice record.
// Reading the64-bit ID does not permit constructing or rebinding a receipt.
class SourceScriptSoundClockVoice64Tu15 {
public:
    std::uint64_t id() const;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit SourceScriptSoundClockVoice64Tu15(std::shared_ptr<const Impl>);
    static SourceScriptSoundClockVoice64Tu15 published(
        std::shared_ptr<const void> actual_audio_epoch,
        std::shared_ptr<const void> actual_platform,
        std::shared_ptr<const void> actual_voice_record,std::uint64_t,
        std::function<bool()> current);
    bool audio_owner_is(const std::shared_ptr<const void>&,
        const std::shared_ptr<const void>&) const;
    bool voice_record_is(const std::shared_ptr<const void>&) const;
    friend class MultiplayerAudio;
    friend class SourceScriptSoundClockNative64Tu15;
};

class SourceScriptSoundClockAdmission64Tu15 {
public:
    std::uint32_t sound() const;
    SourceScriptSoundClockSelectionTu15 selection() const;
private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit SourceScriptSoundClockAdmission64Tu15(std::shared_ptr<const Impl>);
    friend class SourceScriptSoundClockNative64Tu15;
};

// Distinct units prevent treating18D0E0 game ticks as the HS wrapper's30Hz
// counts.2B3CD0's later delta*30/FISTP phase is not implicit in this value.
struct SourceScriptSoundGameTicksTu15 {std::int32_t value{};};
struct SourceScriptSoundTicks30Tu15 {std::int32_t value{};};

// Native owner for the already verified16-row policy. It never serializes a
// host ID as an Xbox datum. Every operation must use its creating thread and
// the producer's executor gate; callbacks must not reenter this owner.
class SourceScriptSoundClockNative64Tu15 {
public:
    using Admission=SourceScriptSoundClockAdmission64Tu15;
    using Voice=SourceScriptSoundClockVoice64Tu15;
    using StopRequest=std::function<void(const std::optional<Voice>&)>;
    // A metadata-only owner can test/query policy and complete failures, but
    // cannot bind a voice. Runtime owners come from the actual Audio factory.
    SourceScriptSoundClockNative64Tu15();
    ~SourceScriptSoundClockNative64Tu15();
    SourceScriptSoundClockNative64Tu15(const SourceScriptSoundClockNative64Tu15&)=delete;
    SourceScriptSoundClockNative64Tu15&operator=(const SourceScriptSoundClockNative64Tu15&)=delete;

    std::optional<Admission> begin(std::uint32_t sound,
        const SourceScriptSoundClockPolicyTu15&,const StopRequest&,
        const SoundDriverFloatingPointContext&);
    // Exact later failed-admission result: row/selector/RNG stay committed,
    // while the old voice link is replaced by no voice. One finish per ticket.
    bool finish_failure(const Admission&);
    SourceScriptSoundGameTicksTu15 explicit_remaining(std::uint32_t sound,
        std::int32_t ticks_at30,const SourceScriptSoundClockTimeTu15&,
        const SoundDriverFloatingPointContext&) const;
    SourceScriptSoundTicks30Tu15 stored_remaining(std::uint32_t sound,
        std::int32_t qualified_original_maximum_ms,
        const SourceScriptSoundClockTimeTu15& query_time,
        const SourceScriptSoundClockTimeTu15& return_time,
        const SoundDriverFloatingPointContext&) const;
    SourceScriptSoundTicks30Tu15 language_remaining(std::uint32_t sound,
        const SourceScriptSoundClockPolicyTu15&,
        const SoundDriverFloatingPointContext&) const;
    bool stop_first(std::uint32_t sound,const StopRequest&);
    // Timing reset only: it does not invent a stop/destroy on the platform.
    // Actual shutdown/restore must order its genuine playback owner separately.
    void reset();
    SourceScriptSoundClockRowsTu15 rows() const;
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
    // Audio restores only source metadata into a fresh, unbound clock. Voice
    // ownership is always established later by actual packet publication.
    void initialize_metadata(const SourceScriptSoundClockRowsTu15&);
    void bind_audio_owner(std::shared_ptr<const void>,std::function<bool()> alive);
    bool finish_voice(const Admission&,const Voice&);
    bool retired(const Admission&,const Voice&);
    bool admission_pending(const Admission&) const;
    bool audio_owner_is(const std::shared_ptr<const void>&) const;
    std::function<void()> close_audio_binding() const;
    std::function<bool()> retirement_callback(const Admission&,const Voice&) const;
    friend class MultiplayerAudio;
    friend class MccScriptSoundQueryServiceTu15;
};
} // namespace halo2
