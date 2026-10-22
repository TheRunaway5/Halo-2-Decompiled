#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace halo2 {
// The importer/loader authenticates source media and parses its canonical
// float32 PCM container before calling this owner. This descriptor is not a
// provenance seal. The caller keeps the supplied span quiescent during copy.
struct NativeMccLoadingPcm {
    std::span<const float> samples;
    std::uint64_t frames{};
    unsigned sample_rate=44100,channels=2;
};
struct NativeMccLoadingAudioConfiguration {
    bool realtime=true;
    unsigned callback_frames=512;
    // Empty selects the default real SDL output. A named unavailable device
    // fails admission; there is no silent fallback to a different output.
    std::string device_name;
};
struct NativeMccLoadingAudioSnapshot {
    bool active{},muted{},device_open{},stream_open{},audio_subsystem_owned{},eof{},callback_failed{};
    unsigned source_rate=44100,output_rate=48000,channels=2;
    std::uint64_t source_frames{},prepared_frames{},position_frames{},rendered_frames{},render_calls{};
    std::int64_t conversion_frame_delta{};
    std::uint32_t gain_bits=0x3f800000;
    std::int32_t attenuation_centibels{};
    float amplitude=1.f;
    std::string failure;
};
// Native playback policy: one genuine supplied stereo track, no pitch change
// or four-voice ambience. Consume the complete SDL-converted extent once,
// including its actual flush tail, then output silence until end(). This does
// not claim the MCC frontend's selector/loop/transition or original-tone parity.
class NativeMccLoadingAudio {
public:
    // Explicit native resource limits, not recovered Xbox/MCC constants.
    static constexpr std::uint64_t maximum_source_frames=44100ull*600;
    static constexpr std::uint64_t maximum_conversion_tail_frames=8192;
    explicit NativeMccLoadingAudio(NativeMccLoadingPcm,NativeMccLoadingAudioConfiguration={});
    ~NativeMccLoadingAudio()noexcept;
    NativeMccLoadingAudio(const NativeMccLoadingAudio&)=delete;
    NativeMccLoadingAudio& operator=(const NativeMccLoadingAudio&)=delete;
    // begin/gain/end bind directly to DesktopLoadingAudioServices. The bridge
    // begins before its graphics worker and ends after that worker joins.
    // Conversion/allocation/device admission happens here, never in callback.
    // Mute is native caller policy and opens no device or conversion stream.
    void begin(bool actual_native_muted);
    void gain(std::uint32_t original_gain_bits);
    void end()noexcept;
    NativeMccLoadingAudioSnapshot snapshot()const;
    // Manual mode only: same allocation-free stereo callback consumer, with
    // the real SDL device paused. Caller owns an even-sized 48kHz float span.
    void render(std::span<float> output_stereo);
private:
    struct State;
    std::unique_ptr<State> state_;
};
}
