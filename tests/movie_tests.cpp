#include "halo2/movie.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

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
