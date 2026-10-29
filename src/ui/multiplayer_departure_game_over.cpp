#include "halo2/multiplayer_departure.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/original_data.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/renderer.hpp"
#include <SDL_vulkan.h>
#include <chrono>

namespace halo2 {
MultiplayerDepartureReturn run_multiplayer_departure_game_over(
    const MultiplayerOptions& options,const MultiplayerState& snapshot,std::uint16_t departed) {
  if(!options.online_match || !options.desktop || !departed)return MultiplayerDepartureReturn::unavailable;
  Cache cache(options.map,options.shared_maps,options.gameplay_xbe.empty()?nullptr:OriginalData::load_xbe(options.gameplay_xbe));
  if(!cache.original_data() || !cache.original_data()->game_over_constants)return MultiplayerDepartureReturn::unavailable;
  const auto constants=*cache.original_data()->game_over_constants;
  auto display=multiplayer_departure_game_over(snapshot,options.online_match->roster(),options.online_match->self(),departed,constants);
  if(!display)return MultiplayerDepartureReturn::unavailable;
  if(display->game_over_complete())return MultiplayerDepartureReturn::completed;
  Mesh mesh;mesh.vertices.resize(3);
  MultiplayerScoreboard scoreboard(cache,gameplay_font_directory(cache),mesh,options.variant,options.ui_assets);
  if(!scoreboard.available())return MultiplayerDepartureReturn::unavailable;
  auto* window=options.desktop->window(options.width,options.height,options.hidden);
  auto renderer=options.desktop->make_renderer(mesh,options.validation);
  renderer->scene_clear_color({0,0,0});
  options.desktop->renderer_ready(renderer);options.desktop->finish_loading();
  const auto ids=options.online_match->local_slots();
  SourceGameOverState ending{true,display->game_over_fade_started,display->game_over_countdown};
  SoundClassFades fades;
  using Clock=std::chrono::steady_clock;
  auto next=Clock::now();bool submitted=false;
  while(!ending.complete() || !submitted) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type==SDL_QUIT)return MultiplayerDepartureReturn::window_quit;
      if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)renderer->resized();
    }
    const auto now=Clock::now();
    while(now>=next && !ending.complete()) {
      source_game_over_tick(ending,tick_seconds,fades,true,constants);
      display->game_over_countdown=ending.countdown;display->game_over_fade_started=ending.fade_started;
      for(unsigned view=0;view<ids.size();++view)scoreboard.step(view,true);
      next+=std::chrono::nanoseconds(1000000000/30);
    }
    int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);
    if(width>0 && height>0) {
      std::vector<Vertex> ui;
      for(unsigned view=0;view<ids.size();++view) {
        const auto draw=scoreboard.draw(*display,ids,view,unsigned(width),unsigned(height),options.desktop->display_settings.two_player_split);
        if(options.scoreboard_observer)options.scoreboard_observer(view,draw.normal_hud,draw.layout,draw.paint);
        ui.insert(ui.end(),draw.paint.draw.vertices.begin(),draw.paint.draw.vertices.end());
      }
      renderer->update_material_constants(mesh.materials);
      const auto capture=options.frame_capture?options.frame_capture(*display):std::filesystem::path{};
      submitted|=renderer->draw(Camera{},capture,{},false,{},{},ui);
    }
    SDL_Delay(1);
  }
  return MultiplayerDepartureReturn::completed;
}
} // namespace halo2
