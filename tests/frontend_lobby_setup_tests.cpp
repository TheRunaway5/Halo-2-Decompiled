#include "halo2/frontend_lobby_setup.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::uint32_t hex(const std::string &value) { return std::uint32_t(std::stoul(value, nullptr, 16)); }
void original(const char *path) {
  std::ifstream file(path); check(bool(file), "missing original setup fixture");
  unsigned constructors = 0, labels = 0, focus = 0, subtitles = 0, details = 0, quick = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream input(row); char kind; input >> kind;
    if (kind == 'C') {
      int session; unsigned count; input >> session >> count;
      const auto actual = frontend_lobby_setup_rows(session);
      check(actual.size() == count, "setup row count differs from original2BB4D2");
      for (unsigned i = 0; i < count; ++i) {
        int semantic; input >> semantic;
        check(actual[i].semantic == semantic, "setup row order differs from original2BB4D2");
      }
      ++constructors;
    } else if (kind == 'L') {
      int semantic; std::string expected; input >> semantic >> expected;
      check(frontend_lobby_setup_label(semantic) == hex(expected), "setup label differs from original2BB74F");
      ++labels;
    } else if (kind == 'F') {
      int session, row_index, expected; input >> session >> row_index >> expected;
      check(frontend_lobby_setup_focused_semantic(session, row_index) == expected, "setup selection differs from original2BBAAD");
      ++focus;
    } else if (kind == 'U') {
      int session; std::string expected; input >> session >> expected;
      check(frontend_lobby_setup_subtitle(session) == hex(expected), "setup footer differs from original2BBB6D");
      ++subtitles;
    } else if (kind == 'D') {
      FrontendLobbySetupDetailInput source; int engine, frame, bank, descriptor_frame; bool map_lookup;
      std::array<std::string, 3> expected;
      input >> source.semantic >> engine >> source.difficulty >> source.settings_valid >> source.entry_valid
            >> expected[0] >> expected[1] >> expected[2] >> frame >> bank >> descriptor_frame >> map_lookup;
      if (engine != -99) source.engine = engine;
      const auto actual = frontend_lobby_setup_detail(source);
      for (unsigned i = 0; i < 3; ++i) check(actual.strings[i] == hex(expected[i]), "setup detail text differs from original2BBBF6");
      check(actual.authored_frame == frame, "setup authored preview frame differs from original2BBBF6");
      check(bank == -1 ? !actual.descriptor_bank : actual.descriptor_bank == bank, "setup preview bank differs from original2BBBF6");
      check(descriptor_frame == -1 ? !actual.descriptor_frame : actual.descriptor_frame == descriptor_frame, "setup preview override differs from original2BBBF6");
      check((actual.preview == FrontendLobbySetupPreview::Map || actual.preview == FrontendLobbySetupPreview::CampaignLevel) == map_lookup,
            "setup map preview admission differs from original2BBBF6");
      ++details;
    } else if (kind == 'Q') {
      int engine, category; input >> engine >> category;
      const auto actual = frontend_lobby_setup_quick_category(engine);
      check(category == -1 ? !actual : actual == category, "setup quick category differs from original2BB978");
      ++quick;
    } else throw std::runtime_error("unknown original setup fixture kind");
    check(!input.fail(), "invalid original setup fixture row");
  }
  check(constructors == 10 && labels == 9 && focus == 46 && subtitles == 10 && details == 32 && quick == 11,
        "incomplete original setup policy fixture");
  const auto split = frontend_lobby_setup_rows(1);
  check(split[0].destination == 206 && split[1].destination == 207 && split[2].destination == 222 &&
            split[3].action == FrontendLobbySetupAction::SwitchCoop,
        "setup dispatched destinations differ from original2BB801/loader definitions");
  const auto coop = frontend_lobby_setup_rows(0);
  check(coop[0].destination == 11 && coop[1].destination == 12 && coop[2].action == FrontendLobbySetupAction::SwitchCustom,
        "co-op setup routes differ from original alternate loaders");
  std::cout << constructors + labels + focus + subtitles + details + quick << " original lobby setup cases matched\n";
}
} // namespace
int main(int argc, char **argv) {
  try { if (argc != 2) throw std::runtime_error("original setup fixture argument required"); original(argv[1]); return 0; }
  catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
