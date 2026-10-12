#include "gpu_postprocess.hpp"
#include "halo2/popup_blur.hpp"
#include <array>
#include <iostream>

namespace {
std::vector<std::byte> read(const std::filesystem::path &path, std::size_t size) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file || std::size_t(file.tellg()) != size)
    throw std::runtime_error("popup fixture size differs: " + path.string());
  std::vector<std::byte> bytes(size);
  file.seekg(0); file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
  if (!file) throw std::runtime_error("short popup fixture");
  return bytes;
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 4) throw std::runtime_error("popup fixture, shader and output directories required");
    const std::filesystem::path fixtures(argv[1]), shaders(argv[2]), output(argv[3]);
    std::filesystem::create_directories(output);
    const auto blur = halo2::frontend_popup_blur(1.f);
    const std::array events{3977u, 3991u, 4005u, 4019u};
    auto source = read(fixtures / "3977-source0.rgba", 640 * 480 * 4);
    std::size_t failures = 0;
    for (unsigned i = 0; i < events.size(); ++i) {
      const auto prefix = std::to_string(events[i]);
      if (source != read(fixtures / (prefix + "-source0.rgba"), source.size()))
        throw std::runtime_error("popup blur chain input differs at " + prefix);
      auto uniform = read(fixtures / (prefix + "-uniform.bin"), 272);
      if (std::memcmp(uniform.data(), blur.program.words.data(), 240) ||
          std::memcmp(uniform.data() + 240, &blur.radii[i], 4))
        throw std::runtime_error("popup kernel differs from captured uniform");
      const halo2_test::PostImage sampled{0, 640, 480, VK_FORMAT_R8G8B8A8_UNORM,
          VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, source};
      halo2_test::Postprocess renderer;
      source = renderer.render(shaders / "bloom.vert.spv", shaders / "bloom.frag.spv",
          1, uniform, {&sampled, 1}, 640, 480, halo2_test::PostBlend::Disabled);
      const auto original = read(fixtures / (prefix + "-output.rgba"), source.size());
      std::size_t changed = 0; unsigned maximum = 0;
      for (std::size_t j = 0; j < source.size(); ++j) {
        const auto difference = unsigned(std::abs(int(std::to_integer<unsigned>(source[j])) -
                                                int(std::to_integer<unsigned>(original[j]))));
        changed += difference != 0; maximum = std::max(maximum, difference);
      }
      failures += changed;
      std::cout << events[i] << ": 307200 RGBA pixels, " << changed << " changed channels, maximum "
                << maximum << " byte\n";
      std::ofstream file(output / (prefix + ".rgba"), std::ios::binary);
      file.write(reinterpret_cast<const char *>(source.data()), source.size());
      if (!file) throw std::runtime_error("cannot write popup blur result");
      // Keep every subsequent pass independently assessable if an earlier
      // comparison fails; a successful run also proves the connected chain.
      source = original;
    }
    if (failures) throw std::runtime_error("production popup blur differs from captured pixels");
    std::cout << "1228800 captured original popup-blur RGBA pixels checked; full native popup remains unverified\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
