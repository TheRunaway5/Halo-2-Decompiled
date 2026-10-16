#pragma once
#include "halo2/render_program.hpp"
#include <optional>

namespace halo2 {
struct CinematicCrossfadeFrame {
  bool capture{};
  std::optional<PixelProgram> composite;
};
// Admitted crossfade slice of the original3FC timed-effect bank. Timed controls
// survive screen start; reset only clears its AC-byte output bank. Clock and
// physical capture ownership remain outside this game-state value.
struct CinematicCrossfadeState {
  bool active{},initialized{},pending{},capture{};
  float strength{},start{},end{},alpha{},output_strength{};
  void screen_start(bool reset);
  void screen_stop();
  void set(float seconds,double render_clock);
  void update(double render_clock);
  void capture_complete(){capture=false;}
  CinematicCrossfadeFrame frame()const;
};
// Complete22050 CPU program construction, including its float-rounded power
// and nearest-even8-bit alpha. Raster/copy ownership is a separate renderer.
std::optional<PixelProgram> cinematic_crossfade_program(float alpha,float strength);
}
