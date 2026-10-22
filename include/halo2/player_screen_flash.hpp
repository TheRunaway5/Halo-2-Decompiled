#pragma once
#include "halo2/shader_animation.hpp"
#include "halo2/transform.hpp"

namespace halo2 {
class Cache;
struct PlayerScreenFlashDefinition {
  std::int16_t type{}, priority{};
  float duration{};
  std::int16_t curve{};
  float intensity{};
  std::array<float,4> color{}; // Original ARGB, not RGBA.
};
struct PlayerScreenResponse {
  std::int16_t type{};
  PlayerScreenFlashDefinition flash;
};
std::vector<PlayerScreenResponse> load_player_screen_responses(const Cache &,std::uint32_t);
struct PlayerScreenFlash {
  std::int32_t type{};
  float intensity{};
  std::array<float,4> color{};
};
// Whole TU1.5 134960, called on intensity-scaled ARGB by40880. The
// framebuffer blend consumes these quantized premultiplied channels.
std::uint32_t player_screen_flash_argb(const PlayerScreenFlash &);
// TU1.5 156840 ->1578A0 ->156E40. The caller owns the local user, completed
// report scale, post-damage shield selector and actual signed unit+1FC seat.
class PlayerScreenFlashPlayer {
public:
  void play(std::span<const PlayerScreenResponse>,bool shielded,bool seated,
      float report_scale,const ShaderFunctionTables &);
  void submit(const PlayerScreenFlashDefinition &,float scale);
  PlayerScreenFlash evaluate(const ShaderFunctionTables &) const;
  void advance(std::uint64_t ticks=1);
  void clear();
private:
  PlayerScreenFlashDefinition request_;
  std::int16_t remaining_{};
  bool fresh_{};
};
}
