#pragma once
#include <array>
#include <cstdint>
union SDL_Event;
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;
namespace halo2 {
enum class PromptDevice { KeyboardMouse, Xbox, PlayStation };
PromptDevice controller_prompt_device(SDL_GameController *);
// Actual local-view ownership. Rebinding one device preserves all other
// views, including their last-used KBM selection. SDL instance IDs, not device
// indices or controller names, identify event owners.
class PlayerPromptDevices {
public:
  void bind(unsigned view,SDL_GameController *);
  void bind(unsigned view,std::int32_t instance,PromptDevice);
  void event(const SDL_Event &,std::uint32_t window,unsigned keyboard_view);
  PromptDevice device(unsigned view)const {return devices_.at(view);}
private:
  std::array<std::int32_t,4> instances_{-1,-1,-1,-1};
  std::array<PromptDevice,4> controllers_{},devices_{};
};
}
