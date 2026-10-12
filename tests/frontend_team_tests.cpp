#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_roster.hpp"
#include "halo2/multiplayer_network.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool pass, const std::string &message) {
  if (!pass) throw FormatError(message);
}
const UiLobbyMap *map(const FrontendContent &content, int id) {
  const auto p = std::find_if(content.multiplayer_maps.begin(), content.multiplayer_maps.end(),
                             [&](const auto &m) { return m.map_id == id; });
  return p == content.multiplayer_maps.end() ? nullptr : &*p;
}
unsigned count(const FrontendContent &content, int id, int type) {
  const auto p = map(content, id);
  return frontend_lobby_team_count(p ? std::span<const std::uint8_t>(p->team_counts)
                                      : std::span<const std::uint8_t>{}, type, true);
}
void oracle(const FrontendContent &content, const std::filesystem::path &fixture) {
  std::ifstream file(fixture);
  check(bool(file), "missing original team edit fixture");
  std::map<char, unsigned> cases;
  FrontendLobbyTeamEdit sequence_edit;
  std::int32_t sequence_committed{};
  int notifications{}, delegations{};
  for (std::string row; std::getline(file, row);) {
    if (row.empty() || row[0] == '#') continue;
    std::istringstream in(row); char kind; in >> kind;
    if (kind == 'N') {
      int id, type, expected; in >> id >> type;
      const auto p = map(content, id);
      for (unsigned i = 0; i < 16; ++i) {
        int byte; in >> byte;
        check(byte == (p ? p->team_counts[i] : 0), "map+C54 metadata changed or was discarded");
      }
      in >> expected;
      check(count(content, id, type) == unsigned(expected), "original251139 map team count differs");
    } else if (kind == 'Q') {
      int protocol, type, flags, expected; in >> protocol >> type >> flags >> expected;
      const auto session = frontend_lobby_session_enum({protocol, 1, -1, type, true});
      check(frontend_lobby_team_available(session, flags) == bool(expected),
            "original251364/199967 team availability differs");
    } else if (kind == 'B') {
      int slot, update, present, flags, member, valid, team;
      in >> slot >> update >> present >> flags >> member >> valid >> team;
      FrontendLobbyTeamEdit edit{bool(valid), std::int16_t(team)};
      std::int32_t committed = 100 + slot;
      bool notified = false;
      if (update) frontend_lobby_team_begin(edit, flags & 1,
          present ? std::optional<std::int8_t>(std::int8_t(member)) : std::nullopt);
      else notified = frontend_lobby_team_commit(edit, flags & 1, committed);
      int expected_valid, pad, expected_team, expected_committed, updates, bases;
      in >> expected_valid >> pad >> expected_team >> expected_committed >> updates >> bases;
      check(edit.valid == bool(expected_valid) && edit.team == expected_team &&
            committed == expected_committed && int(notified) == updates && bases == 0 &&
            pad == 0xa0 + slot, "original250CDA preview/commit differs");
    } else if (kind == 'C') {
      int id, type, allow, valid, member, delta, expected;
      in >> id >> type >> allow >> valid >> member >> delta >> expected;
      FrontendLobbyTeamEdit edit{bool(valid), std::int16_t(member)};
      frontend_lobby_team_cycle(edit, delta, count(content, id, type), bool(allow));
      check(edit.valid == bool(valid) && edit.team == expected, "original250D5C wrap/narrow differs");
    } else if (kind == 'E') {
      int slot, flags, sequence, step, type, param;
      in >> slot >> flags >> sequence >> step >> type >> param;
      if (step == 0) {
        sequence_edit = {}; sequence_committed = 100 + slot; notifications = delegations = 0;
      }
      FrontendLobbyTeamEvent event;
      event.type = type; event.param = param; event.available = flags & 1;
      event.member = 0; event.team_count = count(content, 50, 2); event.allow_none = flags & 32;
      const auto result = frontend_lobby_team_event(sequence_edit, sequence_committed, event);
      notifications += result.update_requested; delegations += !result.consumed;
      int returned, valid, pad, team, committed, updates, bases;
      in >> returned >> valid >> pad >> team >> committed >> updates >> bases;
      check(result.consumed == bool(returned) && sequence_edit.valid == bool(valid) &&
            sequence_edit.team == team && sequence_committed == committed &&
            notifications == updates && delegations == bases && pad == 0xa0 + slot,
            "original250B1E event sequence differs");
    } else if (kind == 'G') {
      int unready, busy, signed_in, returned, valid, pad, team, committed, updates, bases;
      in >> unready >> busy >> signed_in >> returned >> valid >> pad >> team >> committed >> updates >> bases;
      FrontendLobbyTeamEdit edit{false, 20}; std::int32_t value = 100;
      FrontendLobbyTeamEvent event;
      event.type = 5; event.param = 2; event.unready = unready; event.channel_busy = busy;
      event.signed_in = signed_in; event.available = true; event.member = 0;
      const auto result = frontend_lobby_team_event(edit, value, event);
      check(result.consumed == bool(returned) && edit.valid == bool(valid) && edit.team == team &&
            value == committed && result.update_requested == bool(updates) && !bases && pad == 0xa0,
            "original250B1E input admission guard differs");
    } else throw FormatError("unknown source team fixture row");
    check(!in.fail(), "truncated original team fixture"); ++cases[kind];
  }
  check(cases == std::map<char, unsigned>{{'N',143}, {'Q',80}, {'B',128}, {'C',400}, {'E',204}, {'G',3}},
        "incomplete original team edit fixture");
  std::cout << "PASS 958 original team picker/map count comparisons\n";
}
void choose(MenuState &menu, unsigned row, unsigned screen) {
  menu.select(row); menu.accept();
  check(menu.frame().screen == screen, "normal team picker route missed screen " + std::to_string(screen));
}
unsigned entry(const MenuState &menu, int value) {
  const auto &entries = menu.frame().entries;
  const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto &e) { return e.value == value; });
  check(found != entries.end(), "authored team picker route lacks entry");
  return unsigned(found - entries.begin());
}
MenuState selected(const FrontendContent &content, unsigned variant, bool system_link) {
  MenuState menu(content);
  menu.accept(); choose(menu, 0, 167); menu.text("Team Picker"); menu.commit_text(); menu.back();
  check(menu.frame().screen == 6 && !menu.signed_in_profile().empty(), "team picker profile not signed in");
  choose(menu, system_link ? 3 : 2, 30); choose(menu, 0, 14);
  choose(menu, 1, 23); choose(menu, 1, 207);
  choose(menu, entry(menu, content.variants.at(variant).engine), 208);
  choose(menu, entry(menu, variant), 14);
  return menu;
}
bool same_color(Vec3 a, Vec3 b) {
  return std::abs(a.x-b.x) < .000001f && std::abs(a.y-b.y) < .000001f && std::abs(a.z-b.z) < .000001f;
}
void flow(FrontendContent &content, const std::filesystem::path &maps) {
  const auto team = std::find_if(content.variants.begin(), content.variants.end(),
                                [](const auto &v) { return v.name.starts_with("Team Slayer "); });
  check(team != content.variants.end(), "original Team Slayer row missing");
  const auto index = unsigned(team - content.variants.begin());
  Mesh mesh; FrontendPainter painter(content, maps / "fonts", mesh);
  for (const bool system_link : {false, true}) {
    auto menu = selected(content, index, system_link);
    check(menu.lobby_view()->player_team == 0 && menu.lobby_view()->team_count == 8,
          "signed-in Slayer lobby did not expose current membership/map count");
    for (int membership = 0; membership < 8; ++membership) {
      menu.auxiliary();
      check(menu.frame().screen == 14 && menu.lobby_view()->team_edit.valid &&
            menu.lobby_view()->team_edit.team == membership &&
            !frontend_lobby_presentation(menu.lobby_view()->input).start_eligible.value(),
            "X did not open original inline team preview and suppress Start hit admission");
      menu.navigate(1, 0);
      const int preview = (membership + 1) % 8;
      check(menu.lobby_view()->team_edit.team == preview && menu.lobby_view()->player_team == membership,
            "right did not keep preview separate from committed membership");
      const auto draw = painter.draw(menu, 10);
      bool bar = false;
      for (const auto &v : draw.vertices)
        if (mesh.materials.at(v.material).name == "ui\\screens\\game_shell\\pregame_lobby\\item_background") {
          check(same_color(v.color, frontend_lobby_team_colors[preview]),
                "production lobby bar did not consume source preview team color");
          bar = true;
        }
      check(bar, "production team preview lacked authored player bar");
      // A/Start consumes the editing event; it never invokes Start Game.
      menu.select(0); menu.accept();
      check(menu.frame().screen == 14 && !menu.launch && !menu.lobby_view()->team_edit.valid &&
            menu.lobby_view()->player_team == preview,
            "A failed to commit team preview without launching");
    }
    menu.auxiliary(); menu.navigate(-1, 0);
    check(menu.lobby_view()->team_edit.team == 7, "left did not wrap across all eight Slayer teams");
    menu.back();
    check(menu.frame().screen == 14 && !menu.lobby_view()->team_edit.valid &&
          menu.lobby_view()->team_edit.team == -1 && menu.lobby_view()->player_team == 0,
          "B did not cancel preview while retaining committed membership and lobby");
    menu.auxiliary(); menu.navigate(1, 0); menu.auxiliary();
    check(!menu.lobby_view()->team_edit.valid && menu.lobby_view()->player_team == 1,
          "X did not commit active team preview");
    menu.select(0); menu.accept();
    check(menu.launch && menu.launch->multiplayer && menu.launch->local_players==1 &&
          menu.launch->selected_teams==std::vector<std::int8_t>{1} &&
          menu.launch->host_port==(system_link ? 2002u : 0u),
          "Team Slayer launch lost the committed source profile team");
  }
  auto ffa = selected(content, 0, false);
  ffa.auxiliary();
  check(ffa.frame().screen == 14 && !ffa.lobby_view()->team_edit.valid,
        "FFA X incorrectly opened Game Setup or edited membership");
  choose(ffa, 1, 23);
  // No retail sily/category exposes parameter13. This explicitly supplied
  // settings fixture covers flag5; it does not invent an authored selector.
  check(std::none_of(content.parameters.begin(), content.parameters.end(),
                     [](const auto &p) { return p.second.parameter == 13; }),
        "source parameter13 boundary changed: inspect its authored selector");
  auto custom_content = content;
  custom_content.variants[index].values.set(13,1);
  auto no_team = selected(custom_content, index, false);
  check(no_team.lobby_view()->allow_no_team, "parameter13 did not bind source session flag5");
  no_team.auxiliary(); no_team.navigate(-1, 0);
  check(no_team.lobby_view()->team_edit.team == -1, "source flag5 omitted No Team in source cycle");
  no_team.accept();
  check(no_team.lobby_view()->player_team == -1 && no_team.lobby_view()->input.status == 7,
        "No Team incorrectly counted as a ready playing team");
  no_team.auxiliary(); no_team.navigate(1, 0); no_team.accept();
  check(no_team.lobby_view()->player_team == 0 && no_team.lobby_view()->input.status == 9,
        "return from No Team did not restore original team readiness");
  std::cout << "PASS signed-in Split Screen/System Link previews, authored color draws, commit/cancel and FFA regression; supplied flag5/No Team readiness boundary\n";
  Cache cache(maps/"lockout.map");const auto gameplay=cache.load_content(0,true);
  for(const bool system_link:{false,true}) for(int team=0;team<8;++team) {
    auto menu=selected(content,index,system_link);
    menu.auxiliary();for(int n=0;n<team;++n)menu.navigate(1,0);
    menu.accept();check(!menu.launch,"Committing a profile team invoked Start");
    bool validated=false;
    menu.set_multiplayer_map_validator([&](const FrontendLaunch &launch) {
      validated=true;
      check(launch.selected_teams==std::vector<std::int8_t>{std::int8_t(team)},
            "Actual map validator lost the selected profile team");
      auto rules=slayer_rules(launch.variant,true);validate_slayer_map_options(gameplay,rules);
    });
    menu.select(0);menu.accept();
    check(validated && menu.launch && menu.launch->selected_teams==std::vector<std::int8_t>{std::int8_t(team)},
          "Normal menu Start lost the committed team handoff");
    auto rules=slayer_rules(menu.launch->variant,true);
    rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
    MultiplayerMatch match(gameplay,0,cache.spawns(),rules);
    const auto id=match.join("Selected local profile",menu.launch->selected_teams.at(0));
    std::array<Controls,multiplayer_capacity> controls{};
    std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
    const std::array<Controls,1> remote_input{};
    if(system_link) {
      host=std::make_unique<LanHost>(match,0,0x18fd94);
      const std::array<std::int8_t,1> choices{std::int8_t(7-team)};
      client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x18fd94,1,choices);
      for(unsigned n=0;n<100 && !client->ready();++n) {
        client->send(remote_input);host->receive(controls);host->broadcast();client->receive();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      check(client->ready(),"Selected-team menu session failed actual native UDP join");
    }
    for(unsigned tick=0;tick<90;++tick) {
      if(client){client->send(remote_input);host->receive(controls);}
      match.step(controls);
      if(client){host->broadcast();client->receive();}
    }
    const auto state=match.state();
    check(state.team_mode && state.tick==90 && state.players[id].connected && !state.players[id].dead &&
          state.players[id].source_membership==team && state.players[id].source_ffa_identity==id,
          "Normal menu selected team did not survive actual map join/spawn/ticks");
    if(client) {
      for(unsigned n=0;n<20 && client->state().tick!=90;++n) {
        host->broadcast();client->receive();std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      check(client->state().tick==90 && client->state().team_mode &&
            client->state().players[id].source_membership==team &&
            client->state().players[client->slots()[0]].source_membership==7-team,
            "Selected-team menu session lost membership in actual UDP replication");
    }
  }
  std::cout<<"PASS all eight committed profile teams through normal Split Screen/System Link Start, actual Lockout spawn/90 ticks and native UDP delivery\n";
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "team picker needs maps and original fixture");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::exists(maps / "mainmenu.map")) return 77;
    Cache cache(maps / "mainmenu.map"); auto content = cache.load_frontend();
    oracle(content, argv[2]); flow(content, maps); return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
