#pragma once
#include "halo2/audio.hpp"
#include <array>
#include <memory>

namespace halo2 {
enum class MovieDecodeMode { Automatic, Software };
// Decode original Bink or bundled H.264/AV1 movies at their encoded frame rate.
// Automatic mode admits an optional verified lossless HEVC intro copy and
// hardware decoding, with software fallback. A bounded worker prepares frames;
// one stable RGBA frame is exposed. Audio retains the existing stereo mixer.
class Movie {
public:
  explicit Movie(const std::filesystem::path &);
  Movie(const std::filesystem::path &, MovieDecodeMode);
  ~Movie();
  Movie(const Movie &) = delete;
  const Texture &image() const;
  bool advance(double seconds); // True when the displayed frame changed.
  bool finished(double seconds) const;
  unsigned frame_count() const;
  unsigned frame_number() const;
  double duration() const;
  double frame_rate() const;
  // Reports the device actually used for decoded frames, or "software".
  const char *decode_device() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
PcmSound decode_movie_audio(const std::filesystem::path &);
// Imported MCC: empty until an authenticated MCC source is assigned to the
// requested frontend role. Retained supplied banks are not MCC role owners.
// Explicit non-MCC reference sources retain their bundled/disc routing.
std::filesystem::path frontend_movie_path(const std::filesystem::path &maps,
                                        const std::filesystem::path &bundle,
                                        const std::string &name);
// Aspect-preserving rectangle in the full-surface 640x480 UI coordinates.
std::array<float, 4> movie_canvas_rectangle(unsigned width, unsigned height,
                                          unsigned viewport_width,
                                          unsigned viewport_height);
} // namespace halo2
