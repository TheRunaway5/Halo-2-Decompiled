#include "halo2/frontend_lobby.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace halo2;
namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t hex(const std::string &value) {
  return std::uint32_t(std::stoul(value, nullptr, 16));
}
void original(const char *path) {
  std::ifstream file(path);
  check(bool(file), "missing original lobby header fixture");
  unsigned sessions = 0, panes = 0, visibility = 0, subtitles = 0,
           statuses = 0, privacy = 0, protocols = 0, prior_stats = 0;
  for (std::string row; std::getline(file, row);) {
    std::istringstream input(row);
    char kind;
    input >> kind;
    if (kind == 'S') {
      FrontendLobbySessionInput source;
      int variant, expected;
      input >> source.session_class >> source.live_subtype >> source.campaign_id
            >> variant >> source.settings_valid >> expected;
      if (variant != -99) source.variant_state = variant;
      check(frontend_lobby_session_enum(source) == expected,
            "lobby session enum differs from original19989D");
      ++sessions;
    } else if (kind == 'P') {
      int session, previous, expected;
      input >> session >> previous >> expected;
      check(frontend_lobby_pane(session, previous) == expected,
            "lobby pane differs from original24FF0A");
      ++panes;
    } else if (kind == 'V') {
      FrontendLobbyInput source;
      int button0, button1, leader, leader_visible, eligible;
      input >> source.previous_pane >> source.host >> source.local_team_selection
            >> source.countdown >> source.start_permission >> source.update_guard
            >> button0 >> button1 >> leader >> leader_visible >> eligible;
      const auto actual = frontend_lobby_presentation(source);
      if (source.update_guard) {
        check(!actual.buttons && !actual.leader_visible && !actual.start_eligible,
              "guarded original250DAE update changed existing visibility");
      } else {
        check(actual.buttons && (*actual.buttons)[0] == bool(button0) &&
                  (*actual.buttons)[1] == bool(button1),
              "lobby button visibility differs from original250DAE");
        check(actual.leader_text == unsigned(leader) &&
                  actual.leader_visible == bool(leader_visible),
              "lobby leader text visibility/index differs from original250DAE");
        check(actual.start_eligible == bool(eligible),
              "lobby focus eligibility differs from original250DAE/24FA23");
      }
      ++visibility;
    } else if (kind == 'F') {
      FrontendLobbyInput source;
      std::string expected;
      input >> source.host >> source.team_mode >> source.countdown >> expected;
      check(frontend_lobby_presentation(source).subtitle_string == hex(expected),
            "lobby subtitle differs from original24FF61");
      ++subtitles;
    } else if (kind == 'T') {
      FrontendLobbyInput source;
      std::string overflow, expected;
      bool warning;
      input >> source.previous_pane >> source.session_enum >> source.host
            >> source.status >> source.delayed_player >> source.countdown_cancelled
            >> source.loading_map >> source.editing_options >> source.team_count
            >> source.player_count >> source.max_players >> overflow >> expected >> warning;
      source.overflow_status_string = hex(overflow);
      const auto actual = frontend_lobby_presentation(source);
      check(actual.status_string == hex(expected),
            "lobby status binding differs from original24FFC9/19A50F");
      check(actual.status_warning == warning,
            "lobby warning text flag differs from original251188");
      ++statuses;
    } else if (kind == 'C') {
      FrontendLobbyInput source;
      unsigned index;
      bool visible;
      std::string expected;
      input >> source.previous_pane >> source.session_enum >> source.party_privacy
            >> index >> visible >> expected;
      const auto actual = frontend_lobby_presentation(source);
      check(actual.privacy_text == index && actual.privacy_visible == visible,
            "lobby privacy visibility/index differs from original250155");
      check(visible ? actual.privacy_string == hex(expected) : !actual.privacy_string,
            "lobby privacy string differs from original250155");
      ++privacy;
    } else if (kind == 'Q') {
      FrontendLobbyInput source;
      unsigned index;
      bool visible;
      input >> source.previous_pane >> source.prior_stats_count >> index >> visible;
      const auto actual = frontend_lobby_presentation(source);
      check(actual.prior_stats_text == index && actual.prior_stats_visible == visible,
            "lobby prior-stat prompt differs from original2501C0/2503C0");
      ++prior_stats;
    } else if (kind == 'R') {
      int session;
      std::string expected;
      input >> session >> expected;
      check(frontend_lobby_protocol_string(session) == hex(expected),
            "lobby protocol string differs from original22D93B");
      ++protocols;
    } else throw std::runtime_error("unknown original lobby fixture row");
    check(!input.fail(), "invalid original lobby policy fixture row");
  }
  check(sessions == 720 && panes == 30 && visibility == 96 && subtitles == 8 &&
            statuses == 222 && privacy == 63 && protocols == 9 && prior_stats == 24,
        "incomplete original lobby header policy fixture");
  // Guard unknowns explicitly: these original paths leave their current pane,
  // bindings, or visibility in place rather than synthesize a replacement.
  FrontendLobbyInput unknown;
  unknown.previous_pane = -1;
  unknown.update_guard = true;
  const auto retained = frontend_lobby_presentation(unknown);
  check(retained.pane == -1 && !retained.buttons && !retained.leader_text &&
            !retained.status_string && !retained.privacy_text,
        "unknown original lobby context invented a pane or bound widget");
  std::cout << sessions + panes + visibility + subtitles + statuses + privacy + protocols + prior_stats
            << " original lobby header policy cases matched\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("original lobby fixture argument required");
    original(argv[1]);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
