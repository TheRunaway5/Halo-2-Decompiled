#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/world_effects.hpp"
#include <SDL.h>
#include <iostream>

using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 4)
      throw FormatError("Supply DISC_MAPS DLC_MAPS UPDATE_XBE");
    SDL_SetMainReady();
    const auto original = OriginalData::load_xbe(argv[3]);
    unsigned maps{};
    std::size_t contacts{}, particles{}, decals{}, packets{}, collisions{}, deaths{};
    const auto audit = [&](const std::filesystem::path &path) {
      auto cache = std::make_shared<Cache>(path, argv[1], original);
      const auto content = cache->load_content(0, true);
      UiVariant variant{"Slayer", 0, {{89, 5}}};
      auto rules = slayer_rules(variant);
      rules.weapon = cache->slayer_starting_weapon(rules.starting_weapon);
      MultiplayerMatch match(content, 0, cache->spawns(), rules);
      const auto player = match.join("Map effect audit");
      SdlSoundPlatformConfiguration platform{
          SdlSoundPlatformConfiguration::Delivery::Manual, 256, {}};
      std::size_t sound_packets{}, sound_requests{}, collision_sounds{}, death_sounds{};
      MultiplayerAudio audio(*cache, 0x10293847, platform,
                             [&](const auto &) { ++sound_packets; });
      Mesh mesh;
      NativeWorldEffects effects(*cache, mesh, match.simulation().collision(),
                                 0x12345678);
      std::array<Controls, 16> controls{};
      controls[player].yaw = match.unit(player).yaw;
      controls[player].pitch = -1.2f;
      std::uint64_t serial{};
      std::size_t peak_particles{}, peak_decals{};
      for (unsigned tick = 0; tick < 120; ++tick) {
        controls[player].fire = true;
        match.step(controls);
        const auto state = match.state();
        for (const auto &event : state.bullet_impacts)
          if (event.serial > serial) {
            serial = event.serial;
            effects.impact(event);
          }
        effects.advance(state.tick, tick_seconds);
        for (const auto &sound : effects.take_sounds()) {
          ++sound_requests;
          collision_sounds += sound.kind == WorldEffectSoundKind::Collision;
          death_sounds += sound.kind == WorldEffectSoundKind::Death;
          audio.impulse(sound.sound, sound.gain, MultiplayerAudioKind::Impulse,
                        sound.owner);
        }
        audio.render(1600);
        audio.update();
        const auto d = effects.diagnostics();
        peak_particles = std::max(peak_particles, d.particles);
        peak_decals = std::max(peak_decals, d.decals);
      }
      if (!serial || !peak_particles)
        throw FormatError(path.string() + ": actual SMG contact/effect absent");
      if (sound_packets != sound_requests)
        throw FormatError(path.string() + ": authored sound request did not reach SDL");
      for (const auto &gap : effects.diagnostics().unavailable)
        if (gap.find("Particle collider requires its authored child-effect "
                     "dispatcher") != std::string::npos ||
            gap.find("Particle death requires a non-effect dispatcher")!=std::string::npos)
          throw FormatError(
              path.string() +
              ": sound-only particle child suppressed its visual system");
      for (const auto &gap : effects.diagnostics().unavailable)
        std::cout << "BOUNDARY " << path.stem().string() << ' ' << gap << '\n';
      std::cout << "MAP " << path.stem().string() << " contacts=" << serial
                << " particles=" << peak_particles << " decals=" << peak_decals
                << " audio=" << sound_packets
                << " collision_sounds=" << collision_sounds
                << " death_sounds=" << death_sounds << '\n';
      ++maps;
      contacts += serial;
      particles += peak_particles;
      decals += peak_decals;
      packets += sound_packets;
      collisions += collision_sounds;
      deaths += death_sounds;
    };
    for (const auto name :
         {"ascension", "beavercreek", "burial_mounds", "coagulation",
          "colossus", "cyclotron", "foundation", "headlong", "lockout",
          "midship", "waterworks", "zanzibar"})
      audit(std::filesystem::path(argv[1]) / (std::string(name) + ".map"));
    for (const auto name :
         {"backwash", "containment", "deltatap", "dune", "elongation", "gemini",
          "triplicate", "turf", "warlock"})
      audit(std::filesystem::path(argv[2]) / (std::string(name) + ".map"));
    std::cout << "PASS " << maps
              << " actual multiplayer maps; contacts=" << contacts
              << " summed particle peaks=" << particles
              << " summed decal peaks=" << decals << " SDL packets=" << packets
              << " collision sounds=" << collisions << " death sounds=" << deaths << '\n';
    std::cout
        << "LIMIT native contacts and asset admission; this does not verify "
           "all material paths, GPU pixels or matched original frames\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
