#include "halo2/frontend.hpp"
#include <iostream>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
void inspect(const std::filesystem::path &map, unsigned expected_type) {
  Cache cache(map);
  require(cache.is_mcc(), "The pause regression requires its real MCC cache");
  const Bytes scenario(cache.tag_payload(cache.scenario_tag()));
  require(scenario.u16(0x10) == expected_type, "Unexpected scenario type");
  const Bytes globals(cache.tag_payload(cache.globals_tag()));
  const TagHandle multiplayer{globals.u32(0x16c)};
  const auto *tag = cache.find_tag(multiplayer);
  require(tag && tag->group == "mulg", "Missing actual multiplayer tag identity");
  if (expected_type == 0) {
    require(tag->address && !tag->size, "Solo fixture lost its stripped multiplayer tag");
    bool rejected{};
    try { (void)cache.tag_payload(multiplayer); }
    catch (const FormatError &error) {
      rejected = std::string(error.what()) == "Source tag alias has no owned same-address payload";
    }
    require(rejected, "Stripped solo tag unexpectedly acquired a payload");
  } else {
    require(Bytes(cache.tag_payload(multiplayer)).u32(0) != 0,
            "Multiplayer fixture lost its actual universal owner");
  }
  const auto ui = expected_type == 2 ? load_vista_settings(map) : load_vista_pause(map);
  require(bool(ui), "No authored UI content");
  if (expected_type != 2) {
    require(ui->screens.contains(18), "Gameplay lost its authored pause screen");
    require(ui->screens.contains(259), "Gameplay lost its authored Settings screen");
    require(!ui->screens.contains(5), "Gameplay imported a shell background");
  }
  if (expected_type == 0) {
    require(ui->vista_pcr_team_palette.empty(), "Solo imported multiplayer team colors");
    for (const auto &name : ui->vista_team_names)
      require(name.empty(), "Solo imported multiplayer team names");
  } else {
    require(ui->vista_pcr_team_palette.size() == 9, "Multiplayer team color owner was skipped");
    for (const auto &name : ui->vista_team_names)
      require(!name.empty(), "Multiplayer team name owner was skipped");
  }
}
}
int main(int argc, char **argv) {
  if (argc != 2) return 77;
  unsigned failed{};
  for (const auto &[name, type] : {std::pair{"00a_introduction.map", 0u},
       {"01a_tutorial.map", 0u}, {"lockout.map", 1u}, {"mainmenu.map", 2u}}) {
    try {
      inspect(std::filesystem::path(argv[1]) / name, type);
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception &error) {
      ++failed;
      std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
  }
  return failed ? 1 : 0;
}
