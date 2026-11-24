#include "halo2/sdl_sound_platform.hpp"
#include "halo2/native_tu15_sound_crosstalk.hpp"
#include "halo2/sound_i3dl2_tu15.hpp"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using namespace halo2;
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
template<class Function> void rejects(Function &&function) {
  bool failed = false;
  try { function(); } catch (const std::exception &) { failed = true; }
  check(failed, "Invalid or unaccounted native service input was accepted");
}
// The routing and accounting checks use exact levels and linear
// interpolation; voice_processor_stage checks the voice processor's own.
SdlSoundPlatformConfiguration configuration(SdlSoundPlatformConfiguration::Delivery delivery) {
  return {delivery, 256, std::nullopt, true, false};
}
SdlSoundVoiceParameters parameters(float rate, int volume, bool paused) {
  return {std::bit_cast<std::uint32_t>(rate), volume, paused, {{0, 0, 1}, {1, 1, 1}}};
}
void close_packet(SdlSoundPlatform &device, SdlSoundPlatform::Voice voice,
                  std::uint64_t channel, std::uint64_t packet,
                  SdlSoundCompletionStatus reason) {
  const auto completion = device.completion(voice);
  check(completion && completion->voice == voice && completion->channel_context == channel &&
        completion->packet_context == packet && completion->status == reason,
        "Native completion lost its selected packet/context/status");
  device.acknowledge(voice, completion->submission);
}
void close_voice(SdlSoundPlatform &device, SdlSoundPlatform::Voice voice) {
  device.flush(voice);
  while (auto done = device.completion(voice)) device.acknowledge(voice, done->submission);
  device.destroy_voice(voice);
}
std::shared_ptr<const std::vector<float>> pcm(std::initializer_list<float> samples) {
  return std::make_shared<const std::vector<float>>(samples);
}
void sample_near(float actual, double expected) {
  check(std::abs(double(actual) - expected) < 0.0000001, "Native stereo/rate/attenuation sample differs");
}

void retention_and_completion() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  check(device.device_id() > 0, "No genuine native SDL device was opened");
  const auto voice = device.create_voice(0x101, parameters(1, 0, false));
  auto first = pcm({0.1f, 0.2f, 0.3f, 0.4f});
  auto second = pcm({-0.1f, -0.2f, -0.3f, -0.4f});
  const std::weak_ptr<const std::vector<float>> first_lifetime = first, second_lifetime = second;
  check(device.submit(voice, {first, 0, 2, 0x201}) == SdlSoundPlatform::Submission::Accepted &&
        device.submit(voice, {second, 0, 2, 0x202}) == SdlSoundPlatform::Submission::Accepted,
        "Two explicit selected packets were not accepted");
  check(device.submit(voice, {first, 0, 2, 0x203}) == SdlSoundPlatform::Submission::Backpressure,
        "Two attached packet slots were exceeded");
  first.reset(); second.reset();
  check(!first_lifetime.expired() && !second_lifetime.expired() && !device.completion(voice),
        "PCM was released or completed before consumption");
  rejects([&] { device.acknowledge(voice, 1); });
  rejects([&] { device.destroy_voice(voice); });
  auto samples = device.render(1);
  sample_near(samples[0], 0.1); sample_near(samples[1], 0.2);
  check(!device.completion(voice), "A partial packet completed successfully");
  samples = device.render(1);
  sample_near(samples[0], 0.3); sample_near(samples[1], 0.4);
  check(!first_lifetime.expired(), "Consumed PCM was released before source accounting");
  check(!device.status(voice).accepts_input && device.status(voice).completed_packets == 1 &&
        device.status(voice).pending_packets == 1, "Retained packet status differs");
  rejects([&] { device.acknowledge(voice, 2); });
  close_packet(device, voice, 0x101, 0x201, SdlSoundCompletionStatus::Success);
  check(first_lifetime.expired() && !second_lifetime.expired() && device.status(voice).accepts_input,
        "Packet ownership did not follow explicit ordered accounting");
  samples = device.render(1);
  sample_near(samples[0], -0.1); sample_near(samples[1], -0.2);
  device.flush(voice);
  close_packet(device, voice, 0x101, 0x202, SdlSoundCompletionStatus::Flushed);
  check(second_lifetime.expired() && !device.completion(voice), "Flushed PCM ownership leaked");
  device.destroy_voice(voice);
  rejects([&] { (void)device.status(voice); });
}

void packet_boundary_and_rates() {
  const auto full = pcm({0.1f,-0.1f,0.2f,-0.2f,0.3f,-0.3f,0.4f,-0.4f,
                         0.5f,-0.5f,0.6f,-0.6f,0.7f,-0.7f,0.8f,-0.8f});
  for (float rate : {0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 16.0f}) {
    SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
    auto routed = parameters(rate, 0, false);
    // Explicit independent output routing avoids a guessed pan law.
    routed.routes = {{0, 1, 1}, {1, 0, 1}};
    const auto split = device.create_voice(0, routed);
    const auto frames = std::size_t(std::ceil(8.0 / rate));
    device.submit(split, {full, 0, 3, 11});
    device.submit(split, {full, 3, 5, 12});
    const auto output = device.render(frames);
    for (std::size_t frame = 0; frame < frames; ++frame) {
      const double position = double(frame) * rate;
      const auto index = std::size_t(position);
      const auto next = std::min(index + 1, std::size_t(7));
      const double fraction = position - double(index);
      const double left = (*full)[index*2] + fraction * (double((*full)[next*2]) - (*full)[index*2]);
      const double right = (*full)[index*2+1] + fraction * (double((*full)[next*2+1]) - (*full)[index*2+1]);
      sample_near(output[frame*2], right); sample_near(output[frame*2+1], left);
    }
    close_packet(device, split, 0, 11, SdlSoundCompletionStatus::Success);
    close_packet(device, split, 0, 12, SdlSoundCompletionStatus::Success);
    check(device.render(3) == std::vector<float>(6, 0), "Completed stream supplied unsolicited looping data");
    device.destroy_voice(split);
  }
}

void independent_parameters_and_capacity() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  const auto data = pcm({0.5f, 0.25f, 0.5f, 0.25f, 0.5f, 0.25f, 0.5f, 0.25f});
  const auto paused = device.create_voice(1, parameters(1, 0, true));
  const auto active = device.create_voice(2, parameters(1, -2000, false));
  device.submit(paused, {data, 0, 4, 1}); device.submit(active, {data, 0, 4, 2});
  const auto output = device.render(4);
  for (std::size_t i = 0; i < output.size(); i += 2) {
    sample_near(output[i], 0.05); sample_near(output[i+1], 0.025);
  }
  check(!device.completion(paused) && device.status(paused).pending_packets == 1 &&
        device.status(paused).paused, "Independent voice pause advanced a packet");
  close_packet(device, active, 2, 2, SdlSoundCompletionStatus::Success);
  auto updated = parameters(2, 0, false);
  updated.routes = {{0, 0, 0.25f}, {0, 0, 0.5f}, {1, 1, 0}};
  device.parameters(paused, updated);
  const auto resumed = device.render(2);
  for (std::size_t i = 0; i < resumed.size(); i += 2) {
    sample_near(resumed[i], 0.375); sample_near(resumed[i+1], 0);
  }
  close_packet(device, paused, 1, 1, SdlSoundCompletionStatus::Success);
  device.destroy_voice(paused); device.destroy_voice(active);

  // More than the old menu mixer's 32 voices, without choosing a source cap
  // or stealing a stream. These are explicitly native service controls.
  std::array<SdlSoundPlatform::Voice, 80> voices;
  const auto quiet = pcm({0.005f, 0.005f});
  for (std::size_t i = 0; i < voices.size(); ++i) {
    voices[i] = device.create_voice(i, parameters(1, 0, false));
    device.submit(voices[i], {quiet, 0, 1, i+1});
  }
  auto mixed = device.render(1);
  sample_near(mixed[0], double(quiet->front()) * voices.size());
  sample_near(mixed[1], double(quiet->front()) * voices.size());
  for (std::size_t i = 0; i < voices.size(); ++i) {
    close_packet(device, voices[i], i, i+1, SdlSoundCompletionStatus::Success);
    device.destroy_voice(voices[i]);
  }
}

void queued_packet_volumes() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  const auto voice=device.create_voice(0x701,parameters(1,-6000,false));
  const auto data=pcm({.5f,.25f,.5f,.25f});
  device.submit(voice,{data,0,2,71,0});device.submit(voice,{data,0,2,72,-2000});
  device.packet_volume(voice,71,-600);
  auto changed=parameters(1,-8000,false);changed.routes={{0,0,.5f},{1,1,.25f}};
  device.parameters(voice,changed);
  const auto first=device.render(2);
  for(unsigned n=0;n<2;++n){sample_near(first[n*2],.25*std::pow(10.,-.3));sample_near(first[n*2+1],.0625*std::pow(10.,-.3));}
  close_packet(device,voice,0x701,71,SdlSoundCompletionStatus::Success);
  rejects([&]{device.packet_volume(voice,71,0);});
  const auto second=device.render(1);sample_near(second[0],.025);sample_near(second[1],.00625);
  device.packet_volume(voice,72,-400);
  const auto final=device.render(1);sample_near(final[0],.25*std::pow(10.,-.2));sample_near(final[1],.0625*std::pow(10.,-.2));
  close_packet(device,voice,0x701,72,SdlSoundCompletionStatus::Success);device.destroy_voice(voice);
}

void errors_and_flush_order() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  auto input = parameters(1, 0, false);
  rejects([&] { auto p = input; p.routes.clear(); device.create_voice(0, p); });
  rejects([&] { auto p = input; p.relative_rate_bits = 0; device.create_voice(0, p); });
  rejects([&] { auto p = input; p.relative_rate_bits = 0x7fc00000; device.create_voice(0, p); });
  rejects([&] { auto p = input; p.volume_hundredths_db = 1; device.create_voice(0, p); });
  rejects([&] { auto p = input; p.volume_hundredths_db = -10001; device.create_voice(0, p); });
  rejects([&] { auto p = input; p.routes[0].output_channel = 4; device.create_voice(0, p); });
  rejects([&] { auto p = input; p.routes[0].pcm_channel = 2; device.create_voice(0, p); });
  const auto voice = device.create_voice(3, input);
  const auto good = pcm({0.1f, 0.2f, 0.3f, 0.4f});
  rejects([&] { device.submit(voice, {good, 0, 0, 1}); });
  rejects([&] { device.submit(voice, {good, 0, 2, 0}); });
  rejects([&] { device.submit(voice, {good, std::numeric_limits<std::size_t>::max(), 1, 1}); });
  rejects([&] { device.submit(voice, {good, 1, 2, 1}); });
  rejects([&] { device.submit(voice, {pcm({1, 2, 3}), 0, 1, 1}); });
  rejects([&] { device.submit(voice, {pcm({std::numeric_limits<float>::quiet_NaN(), 0}), 0, 1, 1}); });
  device.submit(voice, {good, 0, 1, 31}); device.submit(voice, {good, 1, 1, 32});
  device.render(1);
  device.flush(voice);
  close_packet(device, voice, 3, 31, SdlSoundCompletionStatus::Success);
  close_packet(device, voice, 3, 32, SdlSoundCompletionStatus::Flushed);
  check(!device.device_removed(device.device_id()+100), "Another device removal canceled this service");
  device.submit(voice, {good, 0, 2, 33});
  check(device.device_removed(device.device_id()) && device.status(voice).device_removed &&
        !device.status(voice).accepts_input, "Actual matching device removal was ignored");
  close_packet(device, voice, 3, 33, SdlSoundCompletionStatus::Failure);
  check(device.render(2) == std::vector<float>(4, 0), "Failed device rendered a canceled packet");
  rejects([&] { device.submit(voice, {good, 0, 2, 34}); });
  rejects([&] { device.create_voice(4, input); });
  device.destroy_voice(voice);
}

void real_callback_consumption() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::DeviceCallback));
  const auto voice = device.create_voice(0x401, parameters(1, 0, true));
  auto data = std::make_shared<const std::vector<float>>(2048, 0.1f);
  const std::weak_ptr<const std::vector<float>> lifetime = data;
  device.submit(voice, {data, 0, 512, 0x501});
  device.submit(voice, {data, 512, 512, 0x502});
  data.reset();
  rejects([&] { device.render(1); });
  const auto initial_frames = device.rendered_frames();
  const auto paused_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (device.rendered_frames() == initial_frames && std::chrono::steady_clock::now() < paused_deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  check(device.rendered_frames() > initial_frames, "Real SDL callback did not run");
  check(!device.completion(voice) && device.status(voice).pending_packets == 2,
        "Real SDL callback advanced a paused private voice");
  device.parameters(voice, parameters(1, 0, false));
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (device.status(voice).completed_packets < 2 && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  check(device.status(voice).completed_packets == 2 && !lifetime.expired(),
        "Real callback failed to consume or retain both PCM packets");
  close_packet(device, voice, 0x401, 0x501, SdlSoundCompletionStatus::Success);
  check(!lifetime.expired(), "Second unaccounted packet lost its shared PCM");
  close_packet(device, voice, 0x401, 0x502, SdlSoundCompletionStatus::Success);
  check(lifetime.expired(), "Accounted real callback PCM was retained");
  device.destroy_voice(voice);
}

void speaker_reconfiguration() {
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  const auto voice = device.create_voice(0x601, parameters(1, 0, false));
  const auto data = pcm({.1f, .2f, .3f, .4f, .5f, .6f, .7f, .8f});
  device.submit(voice, {data, 0, 4, 0x701});
  const auto first = device.render(1);
  sample_near(first[0], .1); sample_near(first[1], .2);
  const auto old_device = device.device_id();
  const auto failure = device.speaker_layout(3);
  if (failure.empty()) {
    check(device.output_channels() == 6, "5.1 selection did not create six native output channels");
  } else {
    check(device.device_id() == old_device && device.output_channels() == 2,
          "Failed speaker selection replaced the active endpoint");
  }
  check(device.status(voice).pending_packets == 1 && !device.completion(voice),
        "Speaker selection lost or completed a retained voice packet");
  const auto next = device.render(1);
  check(next.size() == device.output_channels(), "Speaker output extent does not match its endpoint");
  sample_near(next[0], .3); sample_near(next[1], .4);
  for (unsigned channel = 2; channel < next.size(); ++channel)
    check(next[channel] == 0, "Speaker selection fabricated surround PCM");
  check(!device.speaker_layout(99).empty(), "Invalid speaker selection was accepted");
  check(device.speaker_layout(0).empty(), "Stereo/headphone selection failed");
  const auto restored = device.render(1);
  sample_near(restored[0], .5); sample_near(restored[1], .6);
  close_voice(device, voice);
  std::cout << "speakerBackend=" << SDL_GetCurrentAudioDriver()
            << " speakerReconfiguration=" << (failure.empty() ? "sixChannels" : "rollback")
            << " retainedVoiceCursor=passed\n";
}
} // namespace

// Mixbin-13 effect bus: dry routes are unchanged, the 0 dB mono send is
// processed by native DSP record 1 in 32-sample blocks and returned to both
// front channels one block later. Without an effect voice the bus is idle.
void effect_send_bus() {
  std::vector<float> stereo;
  for (unsigned n = 0; n < 96; ++n) {
    const float value = float(0.6 * std::sin(n * 0.37) * (n % 7 == 3 ? -1.2 : 1.0));
    stereo.push_back(value); stereo.push_back(value);
  }
  const auto source = std::make_shared<const std::vector<float>>(stereo);
  const auto run = [&](std::optional<SdlSoundEffectSend> effect) {
    SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
    auto voice_parameters = parameters(1, -600, false);
    voice_parameters.effect = effect;
    const auto voice = device.create_voice(0x301, voice_parameters);
    check(device.submit(voice, {source, 0, 96, 0x401}) == SdlSoundPlatform::Submission::Accepted,
          "Effect packet was not accepted");
    auto output = device.render(160);
    close_voice(device, voice);
    return output;
  };
  const auto dry = run(std::nullopt);
  const double amplitude = std::pow(10.0, -600.0 / 2000.0);
  for (unsigned n = 0; n < 160; ++n)
    sample_near(dry[2 * n], n < 96 ? stereo[2 * n] * amplitude : 0.0);
  auto parameters_1 = tu15_sound_mono_distortion::authored_initial_parameters();
  parameters_1.gain = tu15_sound::Word24(0x200000);
  const auto wet = run(SdlSoundEffectSend{parameters_1});
  // Independent expectation through the same record-1 arithmetic.
  tu15_sound_mono_distortion::State state{};
  auto installed = parameters_1;
  constexpr tu15_sound_mono_distortion::Controls controls{tu15_sound_mono_distortion::ScalingMode::none, false, false,
      false, tu15_sound_mono_distortion::AddressingMode::linear, tu15_sound_mono_distortion::AddressingMode::linear,
      tu15_sound_mono_distortion::AddressingMode::linear, tu15_sound_mono_distortion::AddressingMode::linear,
      tu15_sound_mono_distortion::AddressingMode::linear};
  std::vector<double> returned(160 + 32, 0.0);
  for (unsigned block = 0; block < 5; ++block) {
    std::array<tu15_sound::Word24, 32> words{};
    for (unsigned i = 0; i < 32; ++i) {
      const unsigned n = block * 32 + i;
      const double send = n < 96 ? 0.5 * (stereo[2 * n] + stereo[2 * n + 1]) * amplitude : 0.0;
      const auto scaled = std::clamp(std::nearbyint(send * 8388608.0), -8388608.0, 8388607.0);
      words[i] = tu15_sound::Word24(std::uint32_t(std::int32_t(scaled)) & 0xffffffu);
    }
    tu15_sound_mono_distortion::process(controls, installed, state, words, words);
    for (unsigned i = 0; i < 32; ++i)
      returned[(block + 1) * 32 + i] = double(std::int32_t(words[i].bits << 8) >> 8) / 8388608.0;
  }
  bool audible = false;
  for (unsigned n = 0; n < 160; ++n) {
    const double expected = std::clamp(double(dry[2 * n]) + returned[n], -1.0, 1.0);
    check(std::abs(double(wet[2 * n]) - expected) < 0.000001 && std::abs(double(wet[2 * n + 1]) - expected) < 0.000001,
          "Effect bus output differs from dry plus record-1 return");
    audible |= std::abs(returned[n]) > 0.0001;
  }
  check(audible, "Effect bus returned no processed signal");
}

// The HRTF stage filters the voice's mono input per ear, delays each ear,
// then applies that ear's route gains; a changed filter takes over across
// 32 frames.
void hrtf_stage() {
  // The HRTF stage alone; record 10's crosstalk on its mixbins is below.
  auto setup = configuration(SdlSoundPlatformConfiguration::Delivery::Manual);
  setup.crosstalk = false;
  SdlSoundPlatform device(setup);
  auto input = parameters(1, 0, false);
  input.routes = {{0, 0, 1}, {1, 1, 0.5f}};
  SdlSoundHrtf hrtf;
  hrtf.left[0] = 1; hrtf.left[1] = 0.5f; hrtf.left_delay = 2;
  hrtf.right[0] = 0.25f;
  input.hrtf = hrtf;
  const auto voice = device.create_voice(0x901, input);
  // An impulse on PCM channel 0 only: the mono input is half of it.
  std::vector<float> impulse(2 * 8, 0.f);
  impulse[0] = 1.f;
  check(device.submit(voice, {std::make_shared<const std::vector<float>>(impulse), 0, 8, 0x902}) ==
        SdlSoundPlatform::Submission::Accepted, "HRTF packet was not accepted");
  const auto out = device.render(4);
  sample_near(out[0], 0); sample_near(out[2], 0); sample_near(out[4], 0.5); sample_near(out[6], 0.25);
  sample_near(out[1], 0.5 * 0.25 * 0.5); sample_near(out[3], 0); sample_near(out[5], 0);
  rejects([&] { auto p = input; p.hrtf->left_delay = 64; device.parameters(voice, p); });
  rejects([&] { auto p = input; p.hrtf->right[3] = std::numeric_limits<float>::quiet_NaN(); device.parameters(voice, p); });
  close_voice(device, voice);

  // A constant input settles each ear at its filter's DC gain. A new filter
  // moves in linearly, n/32 of the way at the transition's nth frame, as the
  // voice processor moved a tap from 7Fh to 40h on a retail console.
  SdlSoundHrtf before; before.left[0] = before.right[0] = 1;
  SdlSoundHrtf after; after.left[0] = 0.5f; after.right[0] = 0.25f;
  input.routes = {{0, 0, 1}, {1, 1, 1}};
  input.hrtf = before;
  const auto steady = device.create_voice(0x903, input);
  auto constant = std::make_shared<const std::vector<float>>(2 * 256, 0.5f);
  check(device.submit(steady, {constant, 0, 256, 0x904}) == SdlSoundPlatform::Submission::Accepted,
        "HRTF steady packet was not accepted");
  // A filter changes at the voice's next 32-sample frame.
  auto settled = device.render(64);
  sample_near(settled[126], 0.5); sample_near(settled[127], 0.5);
  input.hrtf = after;
  device.parameters(steady, input);
  const auto fading = device.render(64);
  for (const unsigned frame : {0u, 8u, 16u, 31u, 32u, 63u})
    check(std::abs(fading[2 * frame] - 0.5f * (1 - 0.5f * std::min(frame, 32u) / 32)) < 1e-6f,
          "HRTF filter change did not move linearly across 32 frames");
  sample_near(fading[127], 0.125);
  // Removing the HRTF keeps an unfiltered stage, crossfaded back.
  input.hrtf.reset();
  device.parameters(steady, input);
  const auto restored = device.render(64);
  sample_near(restored[126], 0.5); sample_near(restored[127], 0.5);
  close_voice(device, steady);

  // A changed delay moves across the frame too, reading between samples:
  // on a linear input the left ear lags by exactly n/4 at sample n of a
  // move from 0 to 8.
  SdlSoundHrtf plain; plain.left[0] = plain.right[0] = 1;
  SdlSoundHrtf held = plain; held.left_delay = 8;
  input.hrtf = plain;
  const auto delayed = device.create_voice(0x905, input);
  std::vector<float> line(2 * 256);
  for (unsigned n = 0; n < 256; ++n) line[2 * n] = line[2 * n + 1] = float(n) / 1024.f;
  check(device.submit(delayed, {std::make_shared<const std::vector<float>>(line), 0, 256, 0x906}) ==
        SdlSoundPlatform::Submission::Accepted, "HRTF delay packet was not accepted");
  (void)device.render(64);
  input.hrtf = held;
  device.parameters(delayed, input);
  const auto moving = device.render(64);
  for (unsigned n = 0; n < 64; ++n) {
    const double lag = n < 32 ? n / 4.0 : 8.0;
    check(std::abs(moving[2 * n] - (64 + n - lag) / 1024.0) < 1e-6 && std::abs(moving[2 * n + 1] - (64 + n) / 1024.0) < 1e-6,
          "HRTF delay change did not move linearly across 32 frames");
  }
  close_voice(device, delayed);
}

// The voice processor's measured behaviour: route levels through
// DirectSound's register and the hardware curve, fading a new voice in and
// moving a changed level across one frame; 8-point Lagrange resampling; a
// pitch that moves across the frame in its log form.
void voice_processor_stage() {
  check(sound_route_attenuation_tu15(0) == 0 && sound_route_attenuation_tu15(-1) == 0 &&
        sound_route_attenuation_tu15(-600) == 384 && sound_route_attenuation_tu15(-10000) == 0xfff,
        "DirectSound's attenuation register differs");
  check(sound_route_gain_tu15(0) == 16384 && sound_route_gain_tu15(32) == 15490 && sound_route_gain_tu15(64) == 14600 &&
        sound_route_gain_tu15(48 * 64) == 64 && sound_route_gain_tu15(55 * 64) == 32 && sound_route_gain_tu15(63 * 64) == 0 &&
        sound_route_gain_tu15(0xfff) == 0, "The voice processor's level curve differs");
  const auto half = sound_resampler_weights_tu15(0x8000);
  const std::array<double, 8> lagrange{-5, 49, -245, 1225, 1225, -245, 49, -5};
  for (unsigned k = 0; k < 8; ++k)
    check(half[k] == lagrange[k] / 2048, "The resampler's half-sample weights differ");
  const auto zero = sound_resampler_weights_tu15(0);
  check(zero[3] == 1 && zero[2] == 0 && zero[4] == 0, "The resampler's whole-sample weights differ");

  auto setup = configuration(SdlSoundPlatformConfiguration::Delivery::Manual);
  setup.voice_processor = true;
  SdlSoundPlatform device(setup);
  auto input = parameters(1, -600, false);
  const auto voice = device.create_voice(0xb01, input);
  auto constant = std::make_shared<const std::vector<float>>(2 * 512, 0.5f);
  check(device.submit(voice, {constant, 0, 512, 0xb02}) == SdlSoundPlatform::Submission::Accepted,
        "Voice processor packet was not accepted");
  const double six = sound_route_gain_tu15(384), twelve = sound_route_gain_tu15(768);
  const auto first = device.render(64);
  for (unsigned n = 0; n < 64; ++n) {
    const double level = n < 32 ? std::floor(six * n / 32) : six;
    check(std::abs(first[2 * n] - 0.5 * level / 16384) < 1e-7, "A new voice did not fade in over its first frame");
  }
  input.volume_hundredths_db = -1200;
  device.parameters(voice, input);
  const auto moving = device.render(64);
  for (unsigned n = 0; n < 64; ++n) {
    const double level = n < 32 ? std::floor(six + (twelve - six) * n / 32) : twelve;
    check(std::abs(moving[2 * n] - 0.5 * level / 16384) < 1e-7, "A changed level did not move across one frame");
  }
  // The pitch: from rate 1 to rate 2 over one frame, in 4096ths of an octave.
  const double before = device.playback_position(voice).source_frames;
  input.relative_rate_bits = std::bit_cast<std::uint32_t>(2.f);
  device.parameters(voice, input);
  (void)device.render(32);
  double expected = 0;
  for (unsigned n = 0; n < 32; ++n) expected += std::floor(std::exp2(n / 32.0) * 65536) / 65536;
  check(std::abs(device.playback_position(voice).source_frames - before - expected) < 1e-9,
        "The pitch did not move across the frame in its log form");
  close_voice(device, voice);

  // Lagrange interpolation at three quarters speed, against the PCM itself.
  input = parameters(0.75f, 0, false);
  const auto slow = device.create_voice(0xb03, input);
  std::vector<float> wave(2 * 256);
  for (unsigned n = 0; n < 256; ++n) wave[2 * n] = wave[2 * n + 1] = float(0.5 * std::sin(n * 0.3));
  check(device.submit(slow, {std::make_shared<const std::vector<float>>(wave), 0, 256, 0xb04}) ==
        SdlSoundPlatform::Submission::Accepted, "Resampler packet was not accepted");
  const auto resampled = device.render(128);
  for (unsigned m = 32; m < 128; ++m) {
    const double cursor = m * 0.75;
    const auto whole = int(cursor);
    const auto weights = sound_resampler_weights_tu15(std::uint16_t((cursor - whole) * 65536));
    double value = 0;
    for (int k = 0; k < 8; ++k)
      if (const int at = whole + k - 3; at >= 0) value += weights[std::size_t(k)] * wave[std::size_t(2 * at)];
    if (cursor == whole) value = wave[std::size_t(2 * whole)];
    check(std::abs(resampled[2 * m] - value) < 1e-6, "The resampler is not the voice processor's Lagrange filter");
  }
  close_voice(device, slow);

  // Native-rate playback: a 22050 Hz sound at its unpitched register
  // (-4597) steps 2^(-4597/4096) of its own samples per output sample. A
  // band-limited tone keeps its shape at that speed; content above the
  // native Nyquist is gone.
  const auto play_tone = [&](double hz, std::uint64_t context) {
    auto native = parameters(1, 0, false);
    native.native = SdlSoundNativeRate{22050, -4597};
    const auto voice = device.create_voice(context, native);
    std::vector<float> tone(2 * 8192);
    for (unsigned n = 0; n < 8192; ++n) tone[2 * n] = tone[2 * n + 1] = float(0.5 * std::sin(2 * M_PI * hz * n / 48000.0));
    check(device.submit(voice, {std::make_shared<const std::vector<float>>(tone), 0, 8192, context + 1}) ==
          SdlSoundPlatform::Submission::Accepted, "Native-rate packet was not accepted");
    auto out = device.render(2048);
    const double frames = device.playback_position(voice).source_frames;
    close_voice(device, voice);
    return std::pair{std::move(out), frames};
  };
  const double native_step = std::floor(std::exp2(-4597 / 4096.0) * 65536) / 65536;
  const auto [low, consumed] = play_tone(500, 0xb05);
  check(std::abs(consumed - 2048 * native_step * 48000 / 22050) < 1e-6, "Native playback did not step by the pitch register");
  for (unsigned m = 64; m < 2048; ++m)
    check(std::abs(low[2 * m] - 0.5 * std::sin(2 * M_PI * 500 * m * native_step / 22050)) < 2e-3,
          "Native playback changed a band-limited tone");
  const auto [high, unused] = play_tone(16000, 0xb07);
  (void)unused;
  for (unsigned m = 64; m < 2048; ++m) check(std::abs(high[2 * m]) < 0.02, "Native playback kept content above its Nyquist");
  std::cout << "voiceProcessor=levels+fadeIn+ramps+lagrange+pitchRamp+nativeRate ";
}

// The back speaker pair: folded into the sides at -3 dB on stereo, as the
// EP's encoder does, and played on its own speakers by a quad device; an
// HRTF voice's rear level sends its ears there.
void back_pair() {
  auto setup = configuration(SdlSoundPlatformConfiguration::Delivery::Manual);
  setup.crosstalk = false;
  SdlSoundPlatform device(setup);
  auto input = parameters(1, 0, false);
  input.routes = {{0, 0, 0.5f}, {0, 2, 1}};
  const auto voice = device.create_voice(0xc01, input);
  auto constant = std::make_shared<const std::vector<float>>(2 * 512, 0.5f);
  check(device.submit(voice, {constant, 0, 512, 0xc02}) == SdlSoundPlatform::Submission::Accepted,
        "Back pair packet was not accepted");
  const auto stereo = device.render(1);
  sample_near(stereo[0], 0.25 + 0.5 * (0x5a8279 / 8388608.0)); sample_near(stereo[1], 0);
  const bool quad = device.speaker_layout(2).empty() && device.output_channels() == 4;
  if (quad) {
    const auto four = device.render(1);
    sample_near(four[0], 0.25); sample_near(four[1], 0); sample_near(four[2], 0.5); sample_near(four[3], 0);
    SdlSoundHrtf rear; rear.left[0] = rear.right[0] = 1; rear.front = -10000; rear.rear = 0;
    auto spatial = parameters(1, 0, false);
    spatial.hrtf = rear;
    const auto behind = device.create_voice(0xc03, spatial);
    check(device.submit(behind, {constant, 0, 512, 0xc04}) == SdlSoundPlatform::Submission::Accepted,
          "Rear HRTF packet was not accepted");
    const auto both = device.render(1);
    sample_near(both[0], 0.25); sample_near(both[1], 0); sample_near(both[2], 1.0); sample_near(both[3], 0.5);
    close_voice(device, behind);
  }
  close_voice(device, voice);
  std::cout << "backPair=" << (quad ? "stereoFold+quad " : "stereoFold ");
}

// A room send feeds both listener reverbs; their tail reaches the output
// through the speaker gains, and a dry voice leaves none.
void reverb_bus() {
  const auto tail = [](bool send) {
    SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
    std::array<SdlSoundReverb, 2> reverbs{};
    reverbs[0].parameters.room = 0;
    reverbs[0].parameters.reverb = 0;
    reverbs[0].speaker_gains = {1, 1, 1, 1};
    device.reverb(reverbs);
    auto input = parameters(1, 0, false);
    if (send) input.room = SdlSoundI3dl2Path{1, 0};
    const auto voice = device.create_voice(0xa01, input);
    std::vector<float> impulse(2 * 4, 0.f);
    impulse[0] = impulse[1] = 1.f;
    check(device.submit(voice, {std::make_shared<const std::vector<float>>(impulse), 0, 4, 0xa02}) ==
          SdlSoundPlatform::Submission::Accepted, "Reverb packet was not accepted");
    (void)device.render(2400);
    const auto later = device.render(9600);
    double energy = 0;
    for (const auto value : later) energy += double(value) * value;
    close_voice(device, voice);
    return energy;
  };
  check(tail(true) > 1e-6 && tail(false) == 0, "Room send did not reach the listener reverbs");
  SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
  std::array<SdlSoundReverb, 2> bad{};
  bad[1].speaker_gains[2] = 2.f;
  rejects([&] { device.reverb(bad); });
}

// An HRTF voice reaches the speakers through DSP record 10 on mixbins 6/7:
// the mixer's per-sample crosstalk matches the record's native 24-bit port
// fed the same mixbins in 32-sample blocks.
void crosstalk_stage() {
  const auto render = [](bool crosstalk) {
    auto setup = configuration(SdlSoundPlatformConfiguration::Delivery::Manual);
    setup.crosstalk = crosstalk;
    SdlSoundPlatform device(setup);
    auto input = parameters(1, 0, false);
    SdlSoundHrtf hrtf;
    hrtf.left[0] = 0.8f; hrtf.left[3] = 0.2f;
    hrtf.right[0] = 0.3f; hrtf.right_delay = 5;
    input.hrtf = hrtf;
    const auto voice = device.create_voice(0xb01, input);
    std::vector<float> pcm(2 * 512);
    for (unsigned n = 0; n < 512; ++n) pcm[2 * n] = pcm[2 * n + 1] = float(0.4 * std::sin(n * 0.05) + 0.2 * std::sin(n * 0.9));
    check(device.submit(voice, {std::make_shared<const std::vector<float>>(pcm), 0, 512, 0xb02}) ==
          SdlSoundPlatform::Submission::Accepted, "Crosstalk packet was not accepted");
    const auto out = device.render(512);
    close_voice(device, voice);
    return out;
  };
  const auto dry = render(false), wet = render(true);
  const auto word = [](double sample) {
    return tu15_sound::Word24(std::uint32_t(std::int32_t(std::nearbyint(std::clamp(sample, -1.0, 1.0 - 1.0 / 8388608) * 8388608.0))) & 0xffffffu);
  };
  tu15_sound_crosstalk::State state;
  state.flags = tu15_sound::Word24(2);
  const tu15_sound_crosstalk::Controls controls{tu15_sound::Word24(0), tu15_sound_crosstalk::AddressingMode::linear,
                                                tu15_sound_crosstalk::AddressingMode::linear,
                                                tu15_sound_crosstalk::AddressingMode::linear};
  std::array<tu15_sound_crosstalk::ArithmeticTrace, 64> trace{};
  double worst = 0, difference = 0;
  for (unsigned block = 0; block < 16; ++block) {
    std::array<std::array<tu15_sound::Word24, 32>, 4> in{}, out{};
    for (unsigned n = 0; n < 32; ++n) {
      in[0][n] = word(dry[2 * (32 * block + n)]);
      in[1][n] = word(dry[2 * (32 * block + n) + 1]);
    }
    tu15_sound_crosstalk::process(controls, state, {in[0], in[1], in[2], in[3]}, {out[0], out[1], out[2], out[3]}, trace);
    for (unsigned n = 0; n < 32; ++n)
      for (unsigned side = 0; side < 2; ++side) {
        const double original = double(std::int32_t(out[side][n].bits << 8) >> 8) / 8388608.0;
        const double mixed = wet[2 * (32 * block + n) + side];
        worst = std::max(worst, std::abs(original - mixed));
        difference = std::max(difference, std::abs(mixed - dry[2 * (32 * block + n) + side]));
      }
  }
  check(difference > 0.05, "Record 10 left the 3D mixbins unchanged");
  check(worst < 1e-5, "The mixer's crosstalk differs from DSP record 10");
}

// The DLS2 stage is the voice processor's state-variable low-pass on its
// coefficient words: the voice passes unchanged at 8 kHz and 0 dB, and at
// Cairo's 1200 Hz and 5 dB it follows the filter's closed-form response,
// 5 dB up at the cutoff.
void dls2_stage() {
  const auto through = [](SdlSoundDls2 words, double hz) {
    SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::Manual));
    auto input = parameters(1, 0, false);
    input.dls2 = words;
    const auto voice = device.create_voice(0xb01, input);
    std::vector<float> pcm(2 * 4800);
    for (std::size_t n = 0; n < 4800; ++n)
      pcm[2 * n] = pcm[2 * n + 1] = float(0.5 * std::sin(2 * M_PI * hz * double(n) / 48000));
    check(device.submit(voice, {std::make_shared<const std::vector<float>>(pcm), 0, 4800, 0xb02}) ==
          SdlSoundPlatform::Submission::Accepted, "DLS2 packet was not accepted");
    (void)device.render(2400);
    const auto out = device.render(480);
    std::complex<double> sum{};
    for (std::size_t n = 0; n < 480; ++n) sum += double(out[2 * n]) * std::polar(1.0, -2 * M_PI * hz * double(n) / 48000);
    close_voice(device, voice);
    return 2 * std::abs(sum) / 480 / 0.5;
  };
  const auto response = [](SdlSoundDls2 words, double hz) {
    const double f = sound_dls2_frequency_tu15(words.cutoff), q = words.resonance / 32768.0;
    const auto z = std::polar(1.0, -2 * M_PI * hz / 48000);
    return std::abs(f * f / (1.0 - (2 - f * q - f * f) * z + (1 - f * q) * z * z));
  };
  // The voice processor's coefficient steps, as measured on a console.
  check(sound_dls2_frequency_tu15(0xd97e) == 0x3043 / 65536.0 && sound_dls2_frequency_tu15(0) == 0xffff / 65536.0,
        "DLS2 frequency words differ from the voice processor's");
  const SdlSoundDls2 open{sound_dls2_cutoff_tu15(8000), sound_dls2_resonance_tu15(0)};
  check(open == SdlSoundDls2{0, 0x8000}, "The DLS2 words for 8 kHz and 0 dB differ");
  for (const double hz : {1200.0, 15000.0})
    check(std::abs(through(open, hz) - 1) < 1e-3, "The open DLS2 filter changed the voice");
  const SdlSoundDls2 cairo{sound_dls2_cutoff_tu15(1200), sound_dls2_resonance_tu15(5)};
  check(std::abs(20 * std::log10(response(cairo, 1200)) - 5) < 0.1, "The DLS2 resonance is not its gain at the cutoff");
  for (const double hz : {300.0, 1200.0, 4000.0})
    check(std::abs(through(cairo, hz) / response(cairo, hz) - 1) < 0.01, "The DLS2 stage differs from its response");
  check(through(cairo, 4000) < 0.2, "The DLS2 stage did not filter");
}

int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--device-failure") {
      SDL_setenv("SDL_AUDIODRIVER", "halo2_nonexistent_audio_driver", 1);
      rejects([] { SdlSoundPlatform device(configuration(SdlSoundPlatformConfiguration::Delivery::DeviceCallback)); });
      std::cout << "actualDeviceFailure=propagated sourceAvailability=notModified\n";
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--speakers") {
      speaker_reconfiguration();
      return 0;
    }
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    retention_and_completion();
    packet_boundary_and_rates();
    independent_parameters_and_capacity();
    queued_packet_volumes();
    errors_and_flush_order();
    real_callback_consumption();
    speaker_reconfiguration();
    effect_send_bus();
    hrtf_stage();
    crosstalk_stage();
    reverb_bus();
    dls2_stage();
    voice_processor_stage();
    back_pair();
    std::cout << "hrtfStage=passed crosstalk=record10 reverbBus=records8-9 dls2=passed effectBus=record1 nativeDevice=dummy realCallback=passed attachedPackets=2 rateControls=8 uncappedVoices=80 "
                 "retentionOrderedAccounting=passed independentPause=passed explicitRouting=passed "
                 "flushAndDeviceErrors=passed sourceAdmissionAndHardwareAudibility=unverified\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
