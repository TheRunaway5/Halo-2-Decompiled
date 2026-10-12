#pragma once
#include "halo2/sound_map_activation.hpp"
#include <array>
#include <optional>

namespace halo2 {
struct SoundDriverStartupConstants {
  std::uint32_t one_bits;
  std::array<std::byte,12> basis_forward,basis_up;
  // Original selected mutable plugin instance, requiring its retained owner.
  std::uint32_t routing_plugin_identity;
};
struct SoundDriverStartupState {
  std::uint32_t arena_base,arena_used,arena_crc,driver_address;
  std::uint32_t environment_address,runtime_environment_parameters_address;
  std::uint32_t hardware_queue_address;
  std::uint8_t hardware_queue_flag;
  std::uint32_t queue_4865dc,queue_budget,queue_4865ec;
};
struct SoundDriverDeviceCreateRequest {
  std::uint32_t guid_address,output_address,outer_identity;
};
struct SoundDriverCapabilitiesRequest {
  std::uint32_t device_identity,output_address;
  std::span<std::byte,16> output;
};
struct SoundDriverSavedSettingRequest {
  std::uint32_t device_identity;
};
struct SoundDriverSavedSettingResult {
  std::int32_t result;
  std::optional<std::uint32_t> value;
};
enum class SoundDriverDeviceOperation {
  DistanceFactor,RolloffFactor,MixBinHeadroom,DopplerFactor,LightHrtf4Channel,
};
struct SoundDriverDeviceRequest {
  SoundDriverDeviceOperation operation;
  std::uint32_t device_identity,first_word,second_word;
};
struct SoundDriverBufferCreateRequest {
  std::uint32_t device_identity,output_address,outer_identity;
  std::array<std::uint32_t,6> descriptor;
};
enum class SoundDriverBufferOperation { Play,MaxDistance,MinDistance,Volume,MixBins };
struct SoundDriverBufferRequest {
  SoundDriverBufferOperation operation;
  std::uint32_t buffer_identity;
  std::array<std::uint32_t,3> words;
  std::span<const std::uint32_t> mix_bin_pairs;
};
struct SoundDriverStartupServices {
  std::function<std::span<std::byte>(std::uint32_t,std::size_t)> memory;
  // Real native device creation owns the output identity/backing. A genuine
  // failure returns signed failure; unavailable operations throw/stop.
  std::function<std::int32_t(const SoundDriverDeviceCreateRequest &)> native_device_create;
  std::function<std::int32_t(const SoundDriverCapabilitiesRequest &)> native_device_capabilities;
  std::function<std::int32_t(const SoundDriverDeviceRequest &)> native_device_operation;
  // Actual saved audio-setting owner, not SDL channel-count inference.
  std::function<SoundDriverSavedSettingResult(const SoundDriverSavedSettingRequest &)> saved_audio_setting;
  std::function<std::int32_t(const SoundDriverBufferCreateRequest &)> native_buffer_create;
  std::function<void(const SoundDriverBufferRequest &)> native_buffer_operation;
  std::function<void(std::uint32_t)> native_stream_release,native_buffer_release,native_device_release;
  std::function<void(std::uint32_t)> native_plugin_release;
  std::function<void(const SoundMapPluginInitializeRequest &)> native_plugin_initialize;
  std::function<void(const SoundMapPluginEnvironmentRequest &)> native_plugin_environment;
  // Required original CPU helper owners, not substitute platform successes.
  // This caller module does not implement these helpers. Missing ownership
  // must stop, and a consumer may not return a chosen true/empty output.
  std::function<bool(SoundDriverStartupState &,std::uint32_t)> source_environment_construct; //192D30
  std::function<void(SoundDriverStartupState &,std::span<const std::byte,56>)> source_listener_install; //2228A0
  std::function<bool(SoundDriverStartupState &,std::uint32_t,std::uint32_t)> source_stream_construct; //2215F0 index/category
  std::function<bool(SoundDriverStartupState &,std::uint32_t)> source_static_buffer_construct; //222B80
  std::function<void(SoundDriverStartupState &,std::uint32_t)> source_effect_buffer_mapping; //192E50
  std::function<void(SoundDriverStartupState &,std::uint32_t)> source_feedback_buffer_construct; //2235A0 output
  std::function<void(SoundDriverStartupState &)> source_flush_streams; //2213A0
  std::function<void(SoundDriverStartupState &)> source_hardware_queue_shutdown; //2B4470 actual4865D8 owner
};
struct SoundDriverStartupStore {
  std::uint32_t instruction,address;
  unsigned bytes;
  std::uint32_t before,after;
  friend bool operator==(const SoundDriverStartupStore &,const SoundDriverStartupStore &)=default;
};
using SoundDriverStartupObserver=std::function<void(const SoundDriverStartupStore &)>;
// Complete2205C0 caller CPU order,220BF0 cleanup caller and12D7B0 storage
// owner. Required original CPU callees and genuine native platform operations
// remain explicitly separate. No device/admission defaults; earlier original
// prefix is differentially proven, not successful whole platform initialization.
bool source_sound_driver_startup(SoundDriverStartupState &,
    std::span<const std::byte,16> original_policy,const SoundDriverStartupConstants &,
    const SoundDriverStartupServices &,const SoundDriverStartupObserver & = {});
} // namespace halo2
