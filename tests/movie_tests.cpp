#include "halo2/movie.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

using namespace halo2;
namespace {
void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
template <class F> void rejects(F function) {
  bool rejected = false;
  try {
    function();
  } catch (const FormatError &) {
    rejected = true;
  }
  check(rejected, "invalid movie input was accepted");
}
using Json = nlohmann::json;
std::string sha(const std::string &text) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned size{};
  check(EVP_Digest(text.data(), text.size(), hash, &size, EVP_sha256(), nullptr) == 1 && size == 32,
        "cannot hash movie fixture");
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < size; ++i) {
    result += hex[hash[i] >> 4];
    result += hex[hash[i] & 15];
  }
  return result;
}
void write(const std::filesystem::path &path, const std::string &text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}
void mcc_movie_routing() {
  const auto directory = std::filesystem::temp_directory_path() /
      ("halo2-mcc-movies-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() { std::filesystem::remove_all(path); }
  } cleanup{directory};
  const auto cache = directory / "assets/imported/mcc-test";
  const auto maps = cache / "halo2/h2_maps_win64_dx11";
  const auto bundle = directory / "resources/movies";
  const auto bank = cache / "presentation/movies";
  std::filesystem::create_directories(maps);
  write(cache / "halo2/bink/intro_60.bik", "unassigned MCC fallback");
  check(frontend_movie_path(maps, bundle, "intro").empty(), "unverified MCC Bink was selected");
  Json source{{"schema", "halo2-upscaled-movies-v1"}, {"description", "fixture"}, {"files", Json::object()}};
  Json roles = Json::object();
  for (const std::string role : {"intro", "credits"}) {
    const auto file = role + "_60.mp4";
    const auto payload = role + " fixture";
    write(bundle / file, payload);
    source["files"][file] = {{"bytes", payload.size()}, {"sha256", sha(payload)}};
    roles[role] = source["files"][file];
    roles[role]["path"] = file;
  }
  write(bundle / "manifest.json", source.dump());
  check(frontend_movie_path(maps, bundle, "intro").empty(), "unpaired bundled metadata was accepted");
  write(bundle.parent_path() / "movie-manifest.json", source.dump());
  check(frontend_movie_path(maps, bundle, "intro") == bundle / "intro_60.mp4",
        "older MCC cache did not use explicitly assigned repo intro");
  check(frontend_movie_path(maps, bundle, "credits") == bundle / "credits_60.mp4",
        "older MCC cache did not use explicitly assigned repo credits");
  check(frontend_movie_path(maps, bundle, "attract").empty(), "credits were assigned as attract");
  Json manifest{{"schema", "H2_PORT_BUNDLED_MOVIES_V1"}, {"complete", true},
                {"source_schema", "halo2-upscaled-movies-v1"},
                {"source_manifest", {{"path", "resources/movie-manifest.json"}, {"sha256", sha(source.dump())}}},
                {"roles", roles}, {"copied_without_transcoding", true},
                {"attract_assigned", false}, {"external_vista_or_xbox_required", false}};
  Json report{{"schema", "H2_MCC_TU15_PREPARATION_V1"},
              {"external_installation_requirement", "Halo MCC only"}};
  auto publish = [&](const Json &value) {
    const auto raw = value.dump();
    write(bank / "manifest.json", raw);
    report["port_bundled_movies"] = {{"schema", "H2_PORT_BUNDLED_MOVIES_V1"}, {"complete", true},
        {"path", "presentation/movies/manifest.json"}, {"sha256", sha(raw)}, {"roles", value["roles"]}};
    write(cache / "provenance.json", report.dump());
  };
  std::filesystem::create_directories(bank);
  for (const std::string role : {"intro", "credits"})
    std::filesystem::copy_file(bundle / (role + "_60.mp4"), bank / (role + "_60.mp4"),
                              std::filesystem::copy_options::none);
  publish(manifest);
  check(frontend_movie_path(maps, bundle, "intro") == bank / "intro_60.mp4", "cache bank did not take precedence");
  std::filesystem::remove_all(directory / "resources");
  check(frontend_movie_path(maps, bundle, "credits") == bank / "credits_60.mp4", "cached movie depended on repo original");
  // Payload authentication belongs to import/repair. Same-size changes are
  // deliberately left for stream validation instead of a 2.5GB startup hash.
  write(bank / "intro_60.mp4", std::string(std::filesystem::file_size(bank / "intro_60.mp4"), 'X'));
  check(frontend_movie_path(maps, {}, "intro") == bank / "intro_60.mp4", "runtime unexpectedly rehashed movie payload");
  write(bank / "intro_60.mp4", "short");
  rejects([&] { frontend_movie_path(maps, {}, "credits"); });
  write(bank / "intro_60.mp4", "intro fixture");
  write(bank / "manifest.json", manifest.dump() + " ");
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
  auto bad = manifest;
  bad["roles"]["intro"]["path"] = "../intro_60.mp4";
  publish(bad);
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
  bad = manifest;
  bad["roles"]["attract"] = bad["roles"]["credits"];
  publish(bad);
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
  publish(manifest);
  report["port_bundled_movies"]["path"] = "../movies/manifest.json";
  write(cache / "provenance.json", report.dump());
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
  publish(manifest);
  const auto outside = directory / "intro_60.mp4";
  std::filesystem::rename(bank / "intro_60.mp4", outside);
  std::filesystem::create_symlink(outside, bank / "intro_60.mp4");
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
  std::filesystem::remove(bank / "intro_60.mp4");
  std::filesystem::rename(outside, bank / "intro_60.mp4");
  std::filesystem::rename(bank, directory / "outside-bank");
  std::filesystem::create_directory_symlink(directory / "outside-bank", bank);
  rejects([&] { frontend_movie_path(maps, {}, "intro"); });
}
void routing_and_canvas() {
  const auto directory = std::filesystem::temp_directory_path() /
      ("halo2-movies-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(directory / "resources/movies");
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() { std::filesystem::remove_all(path); }
  } cleanup{directory};
  const auto maps = directory / "disc/maps";
  const auto bundle = directory / "resources/movies";
  for (const std::string name : {"intro", "credits", "attract"}) {
    check(frontend_movie_path(maps, bundle, name) == maps.parent_path() / "bink" / (name + "_60.bik"),
          "missing bundle does not retain disc movie");
    std::ofstream(bundle / (name + "_60.mp4")) << "routing fixture";
    check(frontend_movie_path(maps, bundle, name) ==
              (name == "attract" ? maps.parent_path() / "bink/attract_60.bik" : bundle / (name + "_60.mp4")),
          "bundle preference or attract fallback differs");
  }
  rejects([&] { frontend_movie_path(maps, bundle, "../intro"); });
  check(movie_canvas_rectangle(3840, 2160, 1920, 1080) == std::array<float, 4>{0, 0, 640, 480},
        "widescreen movie does not fill matching viewport");
  check(movie_canvas_rectangle(3840, 2160, 640, 480) == std::array<float, 4>{0, 60, 640, 360},
        "widescreen movie is distorted on 4:3 viewport");
  check(movie_canvas_rectangle(640, 480, 1920, 1080) == std::array<float, 4>{80, 0, 480, 480},
        "retail movie is distorted on widescreen viewport");
  check(movie_canvas_rectangle(3840, 2160, 2560, 1080) == std::array<float, 4>{80, 0, 480, 480},
        "movie is distorted on ultrawide viewport");
  rejects([] { movie_canvas_rectangle(3840, 2160, 0, 1080); });
}
void decode(const std::filesystem::path &path, unsigned count, bool upscaled, bool full) {
  Movie movie(path);
  check(movie.frame_count() == count, "movie frame count differs from source");
  check(std::abs(movie.frame_rate() - (upscaled ? 60000.0 / 1001 : 29.97)) < .000001,
        "movie frame rate differs from source");
  check(movie.image().width == (upscaled ? 3840u : 640u) &&
            movie.image().height == (upscaled ? 2160u : 480u), "movie resolution was changed");
  check(movie.image().pixels.size() == std::size_t(movie.image().width) * movie.image().height * 4,
        "movie RGBA frame is incomplete");
  check(movie.advance(2.1 / movie.frame_rate()) && movie.frame_number() == 3,
        "encoded movie frame timing was lost");
  check(!movie.advance(2.1 / movie.frame_rate()), "held movie frame was decoded twice");
  if (full) {
    movie.advance(movie.duration());
    check(movie.frame_number() == count, "movie did not decode through its final frame");
  }
  const auto audio = decode_movie_audio(path);
  check(audio.rate == 48000 && std::abs(double(audio.frames()) / audio.rate - movie.duration()) < .2,
        "movie stereo audio duration differs from video");
  check(std::all_of(audio.stereo.begin(), audio.stereo.end(), [](float v) { return std::isfinite(v); }) &&
            std::any_of(audio.stereo.begin(), audio.stereo.end(), [](float v) { return std::abs(v) > .01f; }),
        "movie audio is silent or invalid");
  check(movie.finished(movie.duration()) && !movie.finished(0), "movie completion timing differs");
  rejects([&] { movie.advance(-1); });
  std::cout << path.filename() << ": " << movie.frame_number() << "/" << count
            << " video frames, " << audio.frames() << " stereo audio frames\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    routing_and_canvas();
    mcc_movie_routing();
    if (argc >= 3 && std::string(argv[1]) == "--bundle") {
      const bool full = argc >= 4 && std::string(argv[3]) == "--full";
      decode(std::filesystem::path(argv[2]) / "intro_60.mp4", 1079, true, full);
      decode(std::filesystem::path(argv[2]) / "credits_60.mp4", 22739, true, full);
    } else if (argc >= 3 && std::string(argv[1]) == "--retail") {
      decode(std::filesystem::path(argv[2]) / "intro_60.bik", 556, false, true);
      decode(std::filesystem::path(argv[2]) / "attract_60.bik", 3588, false, true);
      decode(std::filesystem::path(argv[2]) / "credits_60.bik", 10375, false, true);
    } else {
      check(argc == 1, "usage: movie_tests [--bundle DIRECTORY [--full] | --retail DIRECTORY]");
    }
    std::cout << "Movie tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
