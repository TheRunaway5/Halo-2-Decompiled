#pragma once
#include "halo2/frontend.hpp"
#include <SDL.h>

namespace halo2 {
class Renderer;
class AudioEngine;
class DesktopSession;
UiStrings load_campaign_pause_objectives(const Cache &,const std::filesystem::path &);
// Native game overlay using the source-local Vista gameplay widgets/fonts.
// Session commands remain explicit callbacks of the actual game owner.
class GamePauseMenu {
public:
  GamePauseMenu(const std::filesystem::path &xbox_maps,
                const std::filesystem::path &vista_assets, Mesh &, bool campaign,
                const std::filesystem::path &preferences = {},
                const std::filesystem::path &menu_assets = {});
  ~GamePauseMenu();
  void bind(SDL_Window *, DesktopSession &, Renderer &);
  void audio(AudioEngine *);
  void party_notices(std::function<std::optional<FrontendInviteNotice>()> query,std::function<bool(bool)> action) {
    party_notice_=std::move(query);party_notice_action_=std::move(action);
  }
  void audio_layout(std::function<std::string(unsigned)> value){audio_layout_=std::move(value);}
  bool event(const SDL_Event &);
  UiDraw draw(unsigned width,unsigned height);
  bool visible() const { return visible_; }
  // TU1.5 solo238AC2 pauses the clock/audio; MP14A0D9 leaves them running.
  // Nested settings/confirmations remain part of the same solo pause owner.
  bool pauses_game_time() const { return visible_ && campaign_; }
  bool captures_game_input();
  bool quit_requested() const { return quit_; }
  void command(std::function<std::string(GamePauseAction)> value) { command_=std::move(value); }
  MenuState &state() { return *state_; }
private:
  void show();
  void refresh();
  std::optional<FrontendPointerTarget> pointer(int,int) const;
  FrontendContent content_;
  std::unique_ptr<MenuState> state_;
  std::unique_ptr<FrontendPainter> painter_;
  SDL_Window *window_{};
  DesktopSession *desktop_{};
  Renderer *renderer_{};
  std::function<std::string(unsigned)> audio_layout_;
  SDL_Cursor *previous_cursor_{},*hand_cursor_{};
  bool campaign_{},visible_{},quit_{};
  bool await_release_{};
  std::int32_t input_instance_=-1;
  unsigned screen_{};
  std::uint32_t entered_{};
  std::uint32_t axis_repeat_{};
  std::function<std::string(GamePauseAction)> command_;
  std::function<std::optional<FrontendInviteNotice>()> party_notice_;
  std::function<bool(bool)> party_notice_action_;
  std::optional<FrontendInviteNotice> drawn_notice_;
};
} // namespace halo2
