#pragma once
#include "halo2/vista_loading_audio.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace halo2 {
struct NativeVistaLoadingAudioConfiguration {
  bool realtime=true;
  unsigned callback_frames=512;
  // Empty selects the real native SDL millisecond clock. Tests can bind a
  // controlled clock; this never estimates scene loading or playback cursors.
  std::function<std::uint32_t()> clock;
};
struct NativeVistaLoadingAudioSnapshot {
  VistaLoadingAudioState source;
  std::array<std::uint32_t,4> frequencies{};
  std::array<std::int32_t,4> volumes{};
  std::array<bool,4> playing{};
  std::array<std::uint64_t,4> submitted_source_bytes{};
  std::uint64_t mixed_output_frames{};
  bool device_open{},worker_running{};
  std::string failure;
};
// Dedicated loading ambience device, separate from menu/game music and gain.
// Original setup/envelope/decoder/shutdown helpers retain command ownership.
// SDL_AudioStream performs explicitly native source-rate/mono/stereo conversion;
// this does not claim the original physical DirectSound mixer/interpolation.
class NativeVistaLoadingAudio {
 public:
  explicit NativeVistaLoadingAudio(std::span<const std::uint8_t>,
      NativeVistaLoadingAudioConfiguration={});
  explicit NativeVistaLoadingAudio(const std::filesystem::path&,
      NativeVistaLoadingAudioConfiguration={});
  ~NativeVistaLoadingAudio() noexcept;
  NativeVistaLoadingAudio(const NativeVistaLoadingAudio&)=delete;
  NativeVistaLoadingAudio& operator=(const NativeVistaLoadingAudio&)=delete;
  // Mute is an explicit native admission policy, not a recovered producer of
  // original setting404544(2)/86D828. Window presence is an actual caller fact.
  // A failed partial native admission must be closed with end() before retry.
  void begin(bool actual_native_muted,bool window_present=true);
  void gain(std::uint32_t original_gain_bits);
  // Called after the graphics loading thread joins. Signals/joins the separate
  //100ms audio worker before stopping/releasing its buffers and device.
  void end();
  NativeVistaLoadingAudioSnapshot snapshot() const;
  // Manual mode only; same callback/SDL streams, without device consumption.
  std::vector<float> render(std::size_t output_frames);
 private:
  struct State;
  std::unique_ptr<State> state_;
};
} // namespace halo2
