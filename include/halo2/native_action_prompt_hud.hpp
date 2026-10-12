#pragma once
#include "halo2/hud.hpp"
#include "halo2/action_prompt_bindings.hpp"
namespace halo2 {
struct NativeActionPromptFrame {
  std::string text;
  std::vector<Vertex> vertices;
};
// Unit action messages without multiplayer score/radar or weapon widgets.
// Reconstruct after the destination Mesh is replaced.
class NativeActionPromptHud {
public:
  NativeActionPromptHud(const Cache &,std::uint32_t unit,const std::filesystem::path &fonts,
      Mesh &,const std::filesystem::path &ui_assets);
  NativeActionPromptFrame frame(NativeActionPrompt,const PcKeyboardSettings &,
      const ControllerSettings &,bool gamepad,unsigned width,unsigned height);
private:
  MultiplayerHud hud_;
};
}
