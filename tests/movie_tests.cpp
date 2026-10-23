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
  // Observe exact private fixture bytes, link targets and modification times.
  // Routing must not read/rewrite/repair an unassigned bank, even when it has
  // valid old metadata or points to a foreign owner.
  const auto tree = [&] {
    Json rows = Json::array();
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
      const auto state = entry.symlink_status();
      Json row{{"path", entry.path().lexically_relative(directory).generic_string()},
               {"type", int(state.type())}};
      if (std::filesystem::is_symlink(state))
        row["target"] = std::filesystem::read_symlink(entry.path()).generic_string();
      else if (std::filesystem::is_regular_file(state)) {
        std::ifstream input(entry.path(), std::ios::binary);
        row["bytes"] = std::string(std::istreambuf_iterator<char>(input), {});
        row["mtime"] = entry.last_write_time().time_since_epoch().count();
      }
      rows.push_back(std::move(row));
    }
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
      return a.at("path").template get<std::string>() < b.at("path").template get<std::string>();
    });
    return rows;
  };
  const auto unavailable = [&] {
    const auto before = tree();
    for (const std::string role : {"intro", "credits", "attract"}) {
      check(frontend_movie_path(maps, bundle, role).empty(), "MCC selected an unassigned supplied movie role");
      check(frontend_movie_path(maps, {}, role).empty(), "MCC selected an unassigned cached/disc movie role");
    }
    check(tree() == before, "MCC movie routing mutated an unassigned retained bank");
  };
  for (const std::string role : {"intro", "credits", "attract"}) {
    write(cache / "halo2/bink" / (role + "_60.bik"), "unassigned MCC Bink fixture");
    write(cache / "halo2/prebuild/video" / (role + ".bk2"), "unassigned collection movie fixture");
  }
  unavailable();
  // The imported user-supplied bank explicitly assigns intro and credits.
  Json source{{"schema", "halo2-upscaled-movies-v1"}, {"description", "fixture"}, {"files", Json::object()}};
  Json roles = Json::object();
  for (const std::string role : {"intro", "credits"}) {
    const auto file = role + "_60.mp4", payload = role + " supplied fixture";
    write(bundle / file, payload);
    write(bank / file, payload);
    source["files"][file] = {{"bytes", payload.size()}, {"sha256", sha(payload)}};
    roles[role] = source["files"][file];
    roles[role]["path"] = file;
  }
  write(bundle / "manifest.json", source.dump());
  write(bundle.parent_path() / "movie-manifest.json", source.dump());
  const Json manifest{{"schema", "H2_PORT_BUNDLED_MOVIES_V1"}, {"complete", true},
      {"source_schema", "halo2-upscaled-movies-v1"},
      {"source_manifest", {{"path", "resources/movie-manifest.json"}, {"sha256", sha(source.dump())}}},
      {"roles", roles}, {"copied_without_transcoding", true},
      {"attract_assigned", false}, {"external_vista_or_xbox_required", false}};
  const Json report{{"schema", "H2_MCC_TU15_PREPARATION_V1"},
      {"external_installation_requirement", "Halo MCC only"},
      {"port_bundled_movies", {{"schema", "H2_PORT_BUNDLED_MOVIES_V1"}, {"complete", true},
       {"path", "presentation/movies/manifest.json"}, {"sha256", sha(manifest.dump())}, {"roles", roles}}}};
  write(bank / "manifest.json", manifest.dump());
  write(cache / "provenance.json", report.dump());
  const auto assigned = tree();
  for (const std::string role : {"intro", "credits"})
    check(frontend_movie_path(maps, bundle, role) == bank / (role + "_60.mp4"),
          "Play skipped the authenticated supplied intro/credits bank");
  check(frontend_movie_path(maps, bundle, "attract").empty(), "supplied bank assigned an attract reel");
  check(tree() == assigned, "movie selection rewrote the supplied bank");
  write(bank / "intro_60.mp4", "modified intro fixture");
  check(frontend_movie_path(maps, bundle, "intro").empty(), "modified movie payload was accepted");
  write(cache / "provenance.json", "malformed foreign provenance");
  write(bank / "manifest.json", "malformed retained movie manifest");
  write(bundle / "manifest.json", "malformed supplied bundle");
  unavailable();
  const auto foreign = directory / "foreign";
  write(foreign / "provenance.json", report.dump());
  write(foreign / "manifest.json", manifest.dump());
  write(foreign / "intro_60.mp4", "foreign intro fixture");
  std::filesystem::remove(cache / "provenance.json");
  std::filesystem::create_symlink(foreign / "provenance.json", cache / "provenance.json");
  std::filesystem::remove(bank / "manifest.json");
  std::filesystem::create_symlink(foreign / "manifest.json", bank / "manifest.json");
  std::filesystem::remove(bank / "intro_60.mp4");
  std::filesystem::create_symlink(foreign / "intro_60.mp4", bank / "intro_60.mp4");
  unavailable();
  std::filesystem::remove_all(bank);
  std::filesystem::create_directory_symlink(foreign, bank);
  std::filesystem::remove_all(bundle);
  std::filesystem::create_directory_symlink(foreign, bundle);
  unavailable();
  for (const std::string name : {"../intro", "Intro", "intro_60.mp4", ""})
    rejects([&] { frontend_movie_path(maps, bundle, name); });
  std::cout << "PASS supplied intro/credits playback; missing, modified and foreign banks remain unavailable\n";
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
bool compare_decoders(const std::filesystem::path &path, bool deep = false) {
  Movie accelerated(path);
  if (std::string(accelerated.decode_device()) == "software") {
    std::cout << "Hardware movie decoding unavailable: " << path << '\n';
    return false;
  }
  Movie reference(path, MovieDecodeMode::Software);
  check(accelerated.duration() == reference.duration() &&
            accelerated.frame_rate() == reference.frame_rate() &&
            accelerated.frame_count() == reference.frame_count(),
        "hardware decoding changed movie timing");
  const auto start = std::chrono::steady_clock::now();
  for (unsigned frame = 0; frame < 180; ++frame) {
    const auto seconds = (frame + .1) / reference.frame_rate();
    reference.advance(seconds);
    accelerated.advance(seconds);
    check(reference.frame_number() == accelerated.frame_number() &&
              reference.image().pixels == accelerated.image().pixels,
          "hardware movie pixels differ from software decoding");
  }
  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  // Reproduce a delayed render frame. Intermediate frames still feed the
  // codec, but only the final display frame needs download/color conversion.
  for (unsigned attempt = 0; attempt < 3; ++attempt) {
    Movie jumped(path);
    Movie software_jump(path, MovieDecodeMode::Software);
    const auto seconds = 179.1 / reference.frame_rate();
    check(jumped.advance(seconds) && software_jump.advance(seconds), "movie catch-up did not advance");
    check(jumped.frame_number() == reference.frame_number() &&
              software_jump.frame_number() == reference.frame_number(),
          "movie catch-up changed the frame number");
    check(jumped.image().pixels == reference.image().pixels,
          "hardware movie catch-up changed pixels");
    check(software_jump.image().pixels == reference.image().pixels,
          "software movie catch-up changed pixels");
    check(!jumped.advance(seconds), "held hardware frame changed");
  }
  if (deep) {
    for (const auto first : {reference.frame_count() / 2, reference.frame_count() - 16}) {
      for (unsigned frame = first; frame < first + 16; ++frame) {
        const auto time = (frame + .1) / reference.frame_rate();
        reference.advance(time);
        accelerated.advance(time);
        check(reference.frame_number() == accelerated.frame_number() &&
                  reference.image().pixels == accelerated.image().pixels,
              "hardware movie pixels differ in middle/final frames");
      }
    }
  }
  std::cout << path.filename() << ": " << accelerated.decode_device()
            << ", 180 byte-identical RGBA frames and catch-up"
            << (deep ? "; middle/final 32 frames identical" : "")
            << "; initial comparison seconds=" << elapsed << '\n';
  return true;
}
} // namespace
int main(int argc, char **argv) {
  try {
    routing_and_canvas();
    mcc_movie_routing();
    if (argc == 3 && std::string(argv[1]) == "--compare-file") {
      if (!compare_decoders(argv[2]))
        return 77;
    } else if ((argc == 3 || (argc == 4 && std::string(argv[3]) == "--deep")) &&
        std::string(argv[1]) == "--compare-decoders") {
      const auto directory = std::filesystem::path(argv[2]);
      if (!compare_decoders(directory / "intro_60.mp4", argc == 4) ||
          !compare_decoders(directory / "credits_60.mp4", argc == 4))
        return 77;
    } else if (argc >= 3 && std::string(argv[1]) == "--bundle") {
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
