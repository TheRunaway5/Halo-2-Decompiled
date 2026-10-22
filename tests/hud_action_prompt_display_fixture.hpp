#pragma once
#include "halo2/action_prompt_bindings.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "mcc_native_prompt_hud_fixture.hpp"
#include <nlohmann/json.hpp>

// Exercise the real HUD text/mesh consumer with actual map messages and fonts,
// independently of player movement and source interaction admission.
inline int action_prompt_display_test(const std::filesystem::path &maps,
    const std::filesystem::path &assets) {
  using namespace halo2;
  const auto check=[](bool ok,const char *why){if(!ok)throw FormatError(why);};
  if(!std::filesystem::is_regular_file(maps/"zanzibar.map"))return 77;
  Cache cache(maps/"zanzibar.map");
  const auto bipeds=cache.select_tags({.group=tag_groups::biped,.require_address=true});
  check(bipeds.begin()!=bipeds.end(),"Prompt display fixture has no actual biped");
  auto content=cache.load_hud((*bipeds.begin()).datum,null_datum);
  content.widgets.clear();
  const auto authored_strings=content.strings;
  std::ifstream reference(HALO2_ACTION_PROMPT_TEMPLATE_REFERENCE);
  check(bool(reference),"Missing qualified action template reference");
  const auto templates=nlohmann::json::parse(reference).at("templates");
  unsigned template_id=0xfff00000u;
  for(const auto &row:templates)content.strings.emplace(template_id++,row.at("mcc").get<std::string>());
  const auto unrelated=template_id;
  content.strings.emplace(unrelated,"Original look token \xee\x91\x9a");
  const auto weapons=cache.select_tags({.group=tag_groups::weapon,.require_address=true});
  const auto needler=std::find_if(weapons.begin(),weapons.end(),[](const auto &tag){
    return tag.name.ends_with("\\needler");
  });
  check(needler!=weapons.end(),"Prompt display fixture has no actual Needler");
  const auto fonts=gameplay_font_directory(cache);
  const bool colored=cache.is_mcc() || !assets.empty();
  const auto key=[&](const std::string &name){return colored?std::string("\xee\x84\xb2")+name:"["+name+"]";};
  const auto pad=[&](unsigned code,const char *name){return colored?mcc_native_prompt_test::utf8(code):std::string("[")+name+"]";};
  MultiplayerState state;
  for(auto &player:state.players) {
    player.connected=true;
    player.action_prompt={{1,5,0xface0001},needler->datum,0};
  }
  Mesh mesh;MultiplayerHud hud(content,fonts,mesh,4,{},std::nullopt,assets);
  const auto expected=[&](const std::string &primary,const std::string &secondary){
    return "Hold "+primary+" to pick up \xee\x84\x9b\r\nHold "+secondary+" to dual-wield";
  };
  unsigned draws{};
  unsigned boarding_messages{};
  // Boarding owners publish their authored message ID directly. Check any
  // actual map message containing grenade/melee tokens before rebinding.
  for(const auto &[sid,text]:authored_strings)if(text.find("\xee\x91\x8c")!=std::string::npos ||
      text.find("\xee\x91\x8a")!=std::string::npos) {
    auto boarding=state;boarding.players[0].action_prompt={{0,0,0xface0001},null_datum,sid};
    for(bool gamepad:{false,true}) {
      hud.prompt_gamepad(0,gamepad);auto expected_text=text;
      for(const auto &row:templates)if(row.at("mcc").get<std::string>()==text) {
        expected_text=row.at("original").get<std::string>();break;
      }
      const std::array labels=gamepad?
          std::array{pad(0xe102,"X"),pad(0xe103,"Y"),pad(0xe106,"LT"),pad(0xe101,"B")}:
          std::array{key("E"),key("X"),key("Q"),key("F")};
      const std::array<std::string,4> tokens{"\xee\x91\x88","\xee\x91\x89","\xee\x91\x8c","\xee\x91\x8a"};
      for(unsigned n=0;n<tokens.size();++n)
        for(std::size_t at=0;(at=expected_text.find(tokens[n],at))!=std::string::npos;at+=labels[n].size())
          expected_text.replace(at,tokens[n].size(),labels[n]);
      const auto actual=hud.action_text(boarding,0,0);
      if(actual!=expected_text)throw FormatError("Default boarding prompt differs from native grenade/melee bindings: actual=["+
          actual+"] expected=["+expected_text+"]");
    }
    ++boarding_messages;
  }
  hud.prompt_gamepad(0,false);
  for(unsigned view=0;view<4;++view) {
    const auto keyboard_text=hud.action_text(state,view,view);
    if(keyboard_text!=expected(key("E"),key("X")))throw FormatError(
        "Default keyboard prompt differs from native E/X bindings: actual=["+keyboard_text+"] expected=["+
        expected(key("E"),key("X"))+"]");
    hud.prompt_gamepad(view,true);
    check(hud.action_text(state,view,view)==expected(pad(0xe102,"X"),pad(0xe103,"Y")),
        "Default controller prompt differs from native X/Y bindings");
    PcKeyboardSettings keyboard;
    keyboard.bindings[unsigned(PcAction::Pickup)]=0;
    keyboard.secondary[unsigned(PcAction::Pickup)]=pc_mouse_button(4);
    keyboard.bindings[unsigned(PcAction::DualPickup)]=0;
    keyboard.secondary[unsigned(PcAction::DualPickup)]=pc_wheel_up;
    hud.prompt_keyboard_buttons(view,native_action_prompt_keyboard_buttons(keyboard));
    hud.prompt_gamepad(view,false);
    check(hud.action_text(state,view,view)==expected(key("Mouse 4"),key("Mouse Wheel")),
        "Secondary keyboard/mouse bindings were not substituted");
    for(unsigned players:{1u,2u,4u})if(view<players)
      for(const auto size:{std::array{640u,480u},std::array{1280u,800u},std::array{1920u,1080u}}) {
        const auto vertices=hud.draw(state,view,view,players,size[0],size[1],0,0);
        check(!vertices.empty(),"Actual action message produced no HUD vertices");
        const auto viewport=hud_viewport(players,view,size[0]*3>size[1]*4);
        for(const auto &v:vertices)check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&
            v.position.x>=viewport.outer.left&&v.position.x<=viewport.outer.right&&
            v.position.y>=viewport.outer.top&&v.position.y<=viewport.outer.bottom,
            "Remapped action prompt escaped its viewport");
        ++draws;
      }
  }
  auto hidden=state;hidden.players[0].dead=true;
  check(hud.action_text(hidden,0,0).empty(),"Dead player retained an action prompt");
  hidden=state;hidden.players[0].connected=false;
  check(hud.action_text(hidden,0,0).empty(),"Disconnected player retained an action prompt");
  hidden=state;hidden.players[0].action_prompt.target.object=null_datum;
  check(hud.action_text(hidden,0,0).empty(),"Consumed target retained an action prompt");
  hud.prompt_keyboard_buttons(0,{"E","X"});
  template_id=0xfff00000u;unsigned template_checks{};
  for(const auto &row:templates) {
    auto stimulus=state;stimulus.players[0].action_prompt={{0,0,0xface0001},null_datum,template_id++};
    for(bool gamepad:{false,true}) {
      hud.prompt_gamepad(0,gamepad);auto text=row.at("original").get<std::string>();
      const std::array labels=gamepad?
          std::array{pad(0xe102,"X"),pad(0xe103,"Y"),pad(0xe106,"LT"),pad(0xe101,"B")}:
          std::array{key("E"),key("X"),key("Q"),key("F")};
      const std::array<std::string,4> tokens{"\xee\x91\x88","\xee\x91\x89","\xee\x91\x8c","\xee\x91\x8a"};
      for(unsigned n=0;n<tokens.size();++n)
        for(std::size_t at=0;(at=text.find(tokens[n],at))!=std::string::npos;at+=labels[n].size())
          text.replace(at,tokens[n].size(),labels[n]);
      check(hud.action_text(stimulus,0,0)==text,"Qualified MCC action template did not resolve to its original message and current buttons");
      ++template_checks;
    }
  }
  auto look=state;look.players[0].action_prompt={{0,0,0xface0001},null_datum,unrelated};
  check(hud.action_text(look,0,0)==content.strings.at(unrelated),"MCC action compatibility reinterpreted an original thumbstick token");
  if(cache.is_mcc())mcc_native_prompt_test::verify_native_mcc_prompt_hud(fonts);
  std::cout<<"PASS actual Needler prompt display, native default buttons, secondary keyboard/mouse bindings, "
      "full/split/wide viewport bounds and stale-target suppression: draws="<<draws
      <<" boarding_messages="<<boarding_messages<<" template_checks="<<template_checks<<'\n';
  return 0;
}
