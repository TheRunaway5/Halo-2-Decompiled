#include "halo2/audio.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "Supply the owned Lockout map");
    if (!std::filesystem::exists(argv[1])) return 77;
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    const halo2::Cache cache(argv[1]);
    // Explicit decoder/provider control for the independently observed reload
    // event datum. These indices do not select a live gameplay permutation.
    const auto encoded = cache.load_sound(0xe5ad0438u, 0, 0);
    const auto decoded = halo2::decode_sound(encoded);
    check(decoded.rate == 48000 && decoded.frames() == 73422,
          "The retained original reload resource differs");
    auto data = std::make_shared<const std::vector<float>>(decoded.stereo);
    const std::weak_ptr<const std::vector<float>> lifetime = data;
    halo2::SdlSoundPlatform device({halo2::SdlSoundPlatformConfiguration::Delivery::Manual,
                                   256, std::nullopt});
    const auto voice = device.create_voice(0x51,
      {std::bit_cast<std::uint32_t>(1.0f), 0, false, {{0, 0, 1}, {1, 1, 1}}});
    const auto first = decoded.frames() / 2;
    device.submit(voice, {data, 0, first, 0x101});
    device.submit(voice, {data, first, decoded.frames()-first, 0x102});
    data.reset();
    const auto output = device.render(decoded.frames());
    check(output.size() == decoded.stereo.size() && !lifetime.expired(),
          "Actual decoded reload PCM was lost before source accounting");
    double peak = 0;
    for (std::size_t i = 0; i < output.size(); ++i) {
      check(std::isfinite(output[i]), "Native reload output contains non-finite PCM");
      check(output[i] == std::clamp(decoded.stereo[i], -1.0f, 1.0f),
            "Explicit native unity matrix altered or omitted a reload PCM sample");
      peak = std::max(peak, std::abs(double(output[i])));
    }
    check(peak > 0, "Actual reload resource rendered to silence");
    for (auto packet : {0x101u, 0x102u}) {
      const auto done = device.completion(voice);
      check(done && done->packet_context == packet && done->channel_context == 0x51 &&
            done->status == halo2::SdlSoundCompletionStatus::Success,
            "Actual reload completion did not preserve the selected packet order");
      device.acknowledge(voice, done->submission);
    }
    check(lifetime.expired(), "Accounted actual reload PCM backing leaked");
    device.destroy_voice(voice);
    std::cout << "ownedOriginalReloadFrames=" << decoded.frames()
              << " explicitDecodedPackets=2 comparedNativeOutputSamples=" << output.size()
              << " nonzeroNativePeak=" << peak
              << " orderedPcmLifetime=passed liveSelectionAdmissionAndHardwareAudibility=unverified\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
