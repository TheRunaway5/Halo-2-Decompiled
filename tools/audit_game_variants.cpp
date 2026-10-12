#include "halo2/frontend.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

using namespace halo2;
namespace {
void check(bool pass,const std::string &message) {
  if (!pass) throw std::runtime_error(message);
}
void choose(MenuState &menu,unsigned row,unsigned expected) {
  menu.select(row);menu.accept();
  check(menu.frame().screen==expected,"actual menu route missed screen "+std::to_string(expected));
}
unsigned entry(const MenuState &menu,int value) {
  const auto &rows=menu.frame().entries;
  const auto found=std::find_if(rows.begin(),rows.end(),
      [&](const auto &row){return row.value==value;});
  check(found!=rows.end(),"authored menu omitted requested variant/category");
  return unsigned(found-rows.begin());
}
MenuState selected(const FrontendContent &content,unsigned index,unsigned protocol) {
  MenuState menu(content);
  // Follow the actual profile creation/customization/sign-in flow. Opening
  // the main menu directly does not supply the lobby's signed-in readiness.
  menu.accept();check(menu.frame().screen==10,"title did not open profile picker");
  choose(menu,0,167);menu.text("Variant Audit");menu.commit_text();
  check(menu.frame().screen==38,"profile creation missed customization");
  menu.back();check(menu.frame().screen==6 && !menu.signed_in_profile().empty(),
      "profile customization did not sign in a local player");
  choose(menu,protocol ? 3 : 2,30);choose(menu,0,14);
  choose(menu,1,23);choose(menu,1,207);
  choose(menu,entry(menu,int(content.variants.at(index).engine)),208);
  choose(menu,entry(menu,int(index)),14);
  const auto lobby=menu.lobby_view();
  check(lobby && lobby->player_appearance && lobby->input.host && lobby->input.status==9,
      "variant audit lobby is not a genuinely signed-in ready host");
  return menu;
}
struct Runtime {
  unsigned local_players{},local_ticks{},lan_players{},lan_ticks{};
};
Runtime run(const Cache &cache,const LevelContent &content,const UiVariant &variant) {
  auto rules=slayer_rules(variant,true);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  Runtime result;
  std::array<Controls,multiplayer_capacity> controls{};
  {
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    // Explicit audit selections, not an automatic session team producer.
    for (unsigned n=0;n<4;++n) match.join("Variant Local "+std::to_string(n),
        rules.team_mode ? std::int8_t(n/2) : -1);
    for (unsigned tick=0;tick<90;++tick) match.step(controls);
    const auto state=match.state();result.local_ticks=state.tick;
    result.local_players=unsigned(std::count_if(state.players.begin(),state.players.end(),
        [](const auto &p){return p.connected && !p.dead;}));
    check(result.local_ticks==90 && result.local_players==4,
          "admitted variant failed actual four-player map join/tick path");
  }
  {
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    match.join("Variant Host",rules.team_mode ? 0 : -1);
    // The same native transport used by the runtime; ephemeral loopback port.
    LanHost host(match,0,0x76617269616e74u);
    const std::array<std::int8_t,2> selections{1,0};
    LanClient client("127.0.0.1:"+std::to_string(host.port()),0x76617269616e74u,2,
        rules.team_mode ? std::span<const std::int8_t>(selections) : std::span<const std::int8_t>{});
    std::array<Controls,2> inputs{};
    for (unsigned attempt=0;attempt<100 && !client.ready();++attempt) {
      client.send(inputs);host.receive(controls);host.broadcast();client.receive();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    check(client.ready(),"admitted variant failed actual UDP join");
    for (unsigned tick=0;tick<90;++tick) {
      client.send(inputs);host.receive(controls);match.step(controls);
      host.broadcast();client.receive();
    }
    for (unsigned attempt=0;attempt<20 && client.state().tick!=match.state().tick;++attempt) {
      host.broadcast();client.receive();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto state=client.state();result.lan_ticks=state.tick;
    result.lan_players=unsigned(std::count_if(state.players.begin(),state.players.end(),
        [](const auto &p){return p.connected;}));
    check(result.lan_ticks==90 && result.lan_players==3 &&
          state.variant_name==variant.name && state.score_limit==rules.score_limit &&
          state.time_ticks==rules.time_ticks && state.team_mode==rules.team_mode,
          "admitted variant failed authoritative UDP state replication");
    if(rules.team_mode)check(state.players[0].source_membership==0 &&
        state.players[client.slots()[0]].source_membership==1 &&
        state.players[client.slots()[1]].source_membership==0,
        "admitted team variant lost explicit profile choices in UDP delivery");
  }
  return result;
}
} // namespace
int main(int argc,char **argv) {
  try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="--require-all-admitted"),
          "usage: audit_game_variants MAPS OUTPUT.json [--require-all-admitted]");
    const std::filesystem::path maps(argv[1]);
    const Cache menu_cache(maps/"mainmenu.map");const auto frontend=menu_cache.load_frontend();
    check(frontend.variants.size()==39,"original built-in catalogue is incomplete");
    const Cache gameplay_cache(maps/"lockout.map");const auto gameplay=gameplay_cache.load_content(0,true);
    std::ofstream output(argv[2]);check(bool(output),"cannot write game mode audit");
    output<<"{\n  \"scope\": \"Actual Split Screen/System Link menu start, followed by Lockout production match/UDP startup for admitted variants. Full objective gameplay, rendered split-screen, complete matches and physical LAN remain unverified.\",\n  \"variants\": [\n";
    unsigned admitted=0;
    for (unsigned index=0;index<frontend.variants.size();++index) {
      const auto &variant=frontend.variants[index];
      const auto source=source_game_variant(variant);
      std::array<bool,2> launched{};std::array<std::string,2> errors;
      for (unsigned protocol=0;protocol<2;++protocol) {
        auto menu=selected(frontend,index,protocol);menu.select(0);menu.accept();
        launched[protocol]=bool(menu.launch);
        if (menu.launch) {
          const auto &actual=menu.launch->variant;
          check(actual.engine==variant.engine && actual.name==variant.name &&
                actual.values==variant.values,"menu changed authored variant before launch");
          const bool teams=source_game_variant_get(source,91)!=0;
          check(menu.launch->selected_teams==(teams ? std::vector<std::int8_t>{0} : std::vector<std::int8_t>{}),
              "menu did not preserve the actual local profile selection");
        } else {
          errors[protocol]=menu.frame().description;
          check(!errors[protocol].empty(),"ready lobby did not launch or explain variant rejection");
        }
      }
      check(launched[0]==launched[1],"local/System Link variant admission differs");
      Runtime runtime;
      if (launched[0]) {runtime=run(gameplay_cache,gameplay,variant);++admitted;}
      if(index) output<<",\n";
      output<<"    {\"index\": "<<index<<", \"name\": "<<std::quoted(variant.name)
        <<", \"frontend_engine\": "<<variant.engine<<", \"source_type\": "
        <<(unsigned(source.bytes[0x44])|unsigned(source.bytes[0x45])<<8)
        <<", \"menu_admitted\": "<<(launched[0]?"true":"false")
        <<", \"split_screen_blocker\": "<<std::quoted(errors[0])
        <<", \"system_link_blocker\": "<<std::quoted(errors[1])
        <<", \"local_players\": "<<runtime.local_players<<", \"local_ticks\": "<<runtime.local_ticks
        <<", \"udp_players\": "<<runtime.lan_players<<", \"udp_ticks\": "<<runtime.lan_ticks<<'}';
    }
    output<<"\n  ],\n  \"menu_admitted\": "<<admitted<<",\n  \"menu_blocked\": "
          <<frontend.variants.size()-admitted<<"\n}\n";
    check(bool(output),"game mode report write failed");
    std::cout<<"Audited 39 authored variants through 78 actual menu start actions: "
             <<admitted<<" admitted, "<<39-admitted<<" blocked\n";
    return argc==4 && admitted!=39 ? 1 : 0;
  } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 2;}
}
