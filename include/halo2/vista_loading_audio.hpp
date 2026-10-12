#pragma once

#include "halo2/vista_loading.hpp"
#include <array>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <vector>

namespace halo2 {
// Original E9E2DC..E9E30F. This owner is loading ambience, independent of
// D73AA0 menu music. Handles are source service tokens, not native devices.
struct VistaLoadingAudioState {
  std::array<std::int32_t,4> cached_volumes{};
  std::array<std::uint32_t,4> buffers{};
  std::uint32_t device{};
  std::int32_t attenuation{};
  std::int32_t maximum_volume{};
  std::uint32_t event{},worker{};
  bool operator==(const VistaLoadingAudioState&) const = default;
};
static_assert(sizeof(VistaLoadingAudioState)==0x34);
static_assert(offsetof(VistaLoadingAudioState,attenuation)==0x24);
static_assert(offsetof(VistaLoadingAudioState,worker)==0x30);
struct VistaLoadingAudioDescription {
  std::uint32_t size{36},flags{0x480a0},bytes{65408},reserved{};
  std::array<std::uint32_t,4> algorithm{};
  std::uint16_t format{1},channels{1};
  std::uint32_t samples_per_second{22050},bytes_per_second{44100};
  std::uint16_t block_alignment{2},bits_per_sample{16},extra_bytes{};
  bool operator==(const VistaLoadingAudioDescription&) const = default;
};
inline constexpr std::array<std::uint32_t,4> vista_loading_audio_frequencies{21738,16290,29008,10876};
struct VistaLoadingAudioLock {
  std::uint32_t first_token{},first_bytes{},second_token{},second_bytes{};
};
// HRESULT services retain signed source success checks. No callback here
// chooses native mixing, cursor advancement, resampling, or a global gain.
struct VistaLoadingAudioServices {
  std::function<bool()> setting2;
  std::function<std::int32_t(std::uint32_t&)> create_device;
  std::function<std::int32_t(std::uint32_t,std::uint32_t,std::uint32_t)> cooperative_level;
  std::function<std::uint32_t(std::uint32_t)> allocate_pcm;
  std::function<void(std::uint32_t)> free_pcm;
  std::function<std::int32_t(std::uint32_t,const VistaLoadingAudioDescription&,std::uint32_t&)> create_buffer;
  std::function<std::int32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,VistaLoadingAudioLock&)> lock;
  std::function<void(std::uint32_t,std::span<const std::uint8_t>)> copy_pcm;
  std::function<std::int32_t(std::uint32_t,const VistaLoadingAudioLock&)> unlock;
  std::function<std::int32_t(std::uint32_t,std::uint32_t)> frequency;
  std::function<std::uint32_t()> create_event;
  std::function<std::uint32_t()> create_worker;
  std::function<std::int32_t(std::uint32_t,std::int32_t)> volume;
  std::function<std::int32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> play;
  std::function<std::uint32_t()> clock;
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> wait;
  std::function<void(std::uint32_t)> signal,close,stop,release;
};
// Bounded mono36-byte-block branch of49A688/49A722, no resampling.
std::vector<std::uint8_t> vista_loading_audio_decode(std::span<const std::uint8_t>);
void vista_loading_audio_setup(VistaLoadingAudioState&,std::uint32_t window,
    std::span<const std::uint8_t> encoded,const VistaLoadingAudioServices&);
void vista_loading_audio_gain(VistaLoadingAudioState&,std::uint32_t gain_bits,
    const VistaLoadingFloatingPoint&);
// Bounded finite clock/ordinary owner domain, both observed CRT selector
// branches compared at final integer SetVolume outputs in original fixtures.
void vista_loading_audio_envelope(VistaLoadingAudioState&,std::uint32_t seconds_bits,
    const VistaLoadingAudioServices&,const VistaLoadingFloatingPoint&);
std::uint32_t vista_loading_audio_elapsed_bits(std::uint32_t origin,std::uint32_t now,
    const VistaLoadingFloatingPoint&);
void vista_loading_audio_worker(VistaLoadingAudioState&,const VistaLoadingAudioServices&,
    const VistaLoadingFloatingPoint&);
void vista_loading_audio_shutdown(VistaLoadingAudioState&,const VistaLoadingAudioServices&);
} // namespace halo2
