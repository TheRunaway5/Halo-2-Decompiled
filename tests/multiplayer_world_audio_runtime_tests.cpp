#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <iostream>
#include <set>

using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 6 && argc != 7)
      throw FormatError("Supply MAP SHARED UPDATE INPUTS OUTPUT [--split4]");
    if (!std::getenv("DISPLAY"))
      return 77;
    SDL_SetMainReady();
    MultiplayerOptions options;
    options.map = argv[1];
    options.shared_maps = argv[2];
    options.gameplay_xbe = argv[3];
    options.input_script = argv[4];
    options.screenshot = argv[5];
    options.capture_tick = 110;
    options.reference_render_fraction = 0;
    options.starting_weapon = 5;
    options.width = 1280;
    options.height = 720;
    options.hidden = true;
    options.validation = true;
    options.local_players = argc == 7 ? 4 : 1;
    if (argc == 7 && std::string(argv[6]) != "--split4")
      throw FormatError("Unknown world audio runtime option");
    unsigned explosions{}, impacts{};
    std::set<std::uint64_t> voices;
    MultiplayerState observed;
    options.input_observer = [&](unsigned, std::span<const Controls>,
                                 const MultiplayerState &state) {
      observed = state;
    };
    options.audio_observer = [&](const MultiplayerAudioSubmission &sound) {
      if (sound.kind != MultiplayerAudioKind::Impulse)
        return;
      if (!sound.voice || !sound.frames || !voices.insert(sound.voice).second)
        throw FormatError("World audio lost or duplicated an actual SDL voice");
      if (sound.sound == 0xe67304feu) {
        ++explosions;
        if(!sound.position || observed.grenade_detonations.empty())
          throw FormatError("Explosion audio discarded its emitted world position");
        const auto &point=observed.grenade_detonations.back().position;
        if(sound.position->x!=point.x || sound.position->y!=point.y || sound.position->z!=point.z)
          throw FormatError("Explosion audio used a different world position");
        if (observed.grenade_detonations.empty() ||
            sound.owner != observed.grenade_detonations.back().projectile)
          throw FormatError("Explosion audio lost the actual projectile owner");
      } else if (std::any_of(observed.bullet_impacts.begin(),
                             observed.bullet_impacts.end(),
                             [&](const auto &hit) {
                               return hit.projectile == sound.owner;
                             })) {
        if(!sound.position)throw FormatError("Impact audio discarded its emitted world position");
        ++impacts;
      }
    };
    if (run_multiplayer(options) || explosions != 1 || !impacts ||
        observed.grenade_detonations.size() != 1)
      throw FormatError(
          "Actual impact/grenade SDL delivery was absent or duplicated");
    std::cout << "PASS actual runtime: views=" << options.local_players
              << " impact SDL packets=" << impacts
              << " explosion SDL packets=" << explosions << '\n';
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
