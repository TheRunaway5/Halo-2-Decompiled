#pragma once
#include "halo2/audio.hpp"
#include <memory>

namespace halo2 {
// Decode the original Bink stream at its authored frame rate. Only one RGBA
// frame is resident; audio uses the existing bounded stereo mixer.
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
} // namespace halo2
