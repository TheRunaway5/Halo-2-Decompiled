#pragma once
#include "halo2/audio.hpp"
#include <array>
#include <memory>

namespace halo2 {
// Decode original Bink or bundled H.264/AV1 movies at their encoded frame rate.
// One RGBA frame is exposed; audio uses the existing bounded stereo mixer.
class Movie {
public:
  explicit Movie(const std::filesystem::path &);
  ~Movie();
  Movie(const Movie &) = delete;
  const Texture &image() const;
  bool advance(double seconds); // True when the displayed frame changed.
  bool finished(double seconds) const;
  unsigned frame_count() const;
  unsigned frame_number() const;
  double duration() const;
  double frame_rate() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
PcmSound decode_movie_audio(const std::filesystem::path &);
// Prefer a bundled intro/credits, retaining the disc movie when absent.
std::filesystem::path frontend_movie_path(const std::filesystem::path &maps,
                                        const std::filesystem::path &bundle,
                                        const std::string &name);
// Aspect-preserving rectangle in the full-surface 640x480 UI coordinates.
std::array<float, 4> movie_canvas_rectangle(unsigned width, unsigned height,
                                          unsigned viewport_width,
                                          unsigned viewport_height);
} // namespace halo2
