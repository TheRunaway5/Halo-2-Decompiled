#include "halo2/sdl_sound_platform.hpp"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

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
SdlSoundPlatformConfiguration configuration(SdlSoundPlatformConfiguration::Delivery delivery) {
  return {delivery, 256, std::nullopt};
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
  rejects([&] { auto p = input; p.routes[0].output_channel = 2; device.create_voice(0, p); });
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
    std::cout << "nativeDevice=dummy realCallback=passed attachedPackets=2 rateControls=8 uncappedVoices=80 "
                 "retentionOrderedAccounting=passed independentPause=passed explicitRouting=passed "
                 "flushAndDeviceErrors=passed sourceAdmissionAndHardwareAudibility=unverified\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
