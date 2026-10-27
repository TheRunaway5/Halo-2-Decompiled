#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include "../src/ui/frontend_multiplayer_rules.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace halo2;
namespace {
void need(bool value,const char* message) {if(!value)throw FormatError(message);}
void choose(MenuState& menu,unsigned row,unsigned screen) {
  menu.select(row);menu.accept();need(menu.frame().screen==screen,"Objective Start missed an authored menu screen");
}
unsigned entry(const MenuState& menu,int value) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,value,&MenuEntry::value);
  need(found!=rows.end(),"Objective Start lost an authored variant row");return unsigned(found-rows.begin());
}
MenuState selected(const FrontendContent& content,unsigned index,bool system_link) {
  MenuState menu(content);menu.accept();choose(menu,0,167);
  menu.text("Objective Start");menu.commit_text();menu.back();
  need(menu.frame().screen==6 && !menu.signed_in_profile().empty(),"Objective Start has no signed-in profile");
  choose(menu,system_link?3:2,30);choose(menu,0,14);
  choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,content.variants.at(index).engine),208);
  choose(menu,entry(menu,index),14);return menu;
}
}
int main(int argc,char** argv)try {
  need(argc==3,"Usage: frontend_objective_start_tu15_tests MAPS ORIGINAL_PRESETS");
  const auto maps=std::filesystem::path(argv[1]);
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  Cache cache(maps/"mainmenu.map");auto content=cache.load_frontend();
  nlohmann::json fixture;std::ifstream(argv[2])>>fixture;unsigned starts{};
  unsigned edited_starts{};std::array<bool,10> edited_engines{};
  unsigned stock_admissions{};bool checked_two_round_slayer{};
  for(const auto& preset:fixture.at("variants")) {
    UiVariant stock{preset.at("name").get<std::string>(),preset.at("frontend_engine").get<unsigned>(),{}};
    for(const auto &write:preset.at("writes"))stock.values.append(write[0].get<unsigned>(),write[1].get<int>());
    const auto source=source_game_variant(stock);const auto raw=preset.at("raw").get<std::string>();
    need(raw.size()==source.bytes.size()*2,"Incomplete original stock admission buffer");
    for(unsigned n=0;n<source.bytes.size();++n)
      need(source.bytes[n]==std::stoul(raw.substr(n*2,2),nullptr,16),"Stock admission changed an original option byte");
    const auto rules=detail::frontend_multiplayer_rules(stock);
    need(rules.requires_source_engine&&rules.source_engine==preset.at("source_type").get<int>(),
        "Stock frontend admission selected a bounded engine instead of its recovered original owner");
    ++stock_admissions;
    if(stock.engine==0&&!checked_two_round_slayer) {
      auto two_round=stock;two_round.values.set(0,1);
      need(source_game_variant_valid(source_game_variant(two_round)),"Original two-round Slayer buffer was not valid");
      need(detail::frontend_multiplayer_rules(two_round).requires_source_engine,
          "Source-valid two-round Slayer lost its original frontend engine owner");
      bool bounded_rejected{};try{(void)slayer_rules(two_round,true);}catch(const FormatError&){bounded_rejected=true;}
      need(bounded_rejected,"Stock routing fix relaxed the separate bounded adapter globally");checked_two_round_slayer=true;
    }
  }
  need(stock_admissions==39&&checked_two_round_slayer,"Stock admission lost original39-preset/Slayer-round coverage");
  bool fiesta{},infection{},defusal{};
  for(const auto &custom:content.bundled_variants) {
    const auto kind=custom_slayer(custom);const bool bomb=bomb_defusal_variant(custom);
    if(kind==CustomSlayer::none&&!bomb)continue;
    const auto rules=detail::frontend_multiplayer_rules(custom);
    need(!rules.requires_source_engine&&rules.custom==kind&&rules.defusal.has_value()==bomb,
        "Stock admission routing stole an existing native custom rules owner");
    fiesta|=kind==CustomSlayer::fiesta;infection|=kind==CustomSlayer::infection;defusal|=bomb;
  }
  need(fiesta&&infection&&defusal,"Native custom admission boundary lost Fiesta/Infection/Defusal fixtures");
  std::cout<<"PASS exact39 original stock buffers, two-round Slayer source admission and native custom boundaries\n";
  for(const auto& preset:fixture.at("variants")) {
    const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
    const auto name=preset.at("name").get<std::string>();
    const auto found=std::ranges::find_if(content.variants,[&](const auto& variant) {
      std::string_view title=variant.name;
      if(title.ends_with(" \xee\x80\x88"))title.remove_suffix(4);
      return title==name;
    });
    need(found!=content.variants.end(),"Original objective preset is absent from the real menu");
    const auto index=unsigned(found-content.variants.begin());
    for(const bool system_link:{false,true})for(const int team:{0,1}) {
      auto menu=selected(content,index,system_link);
      need(menu.lobby_view()->player_team==0,"Fresh objective profile lost its actual default team");
      menu.auxiliary();if(team)menu.navigate(1,0);menu.accept();
      need(!menu.launch && menu.lobby_view()->player_team==team,"Objective team commit invoked Start or lost membership");
      bool validated{};
      menu.set_multiplayer_map_validator([&](const FrontendLaunch& launch) {
        const auto variant=source_game_variant(launch.variant);
        const auto raw=preset.at("raw").get<std::string>();
        need(raw.size()==variant.bytes.size()*2,"Incomplete original objective preset");
        for(unsigned n=0;n<variant.bytes.size();++n)
          need(variant.bytes[n]==std::stoul(raw.substr(n*2,2),nullptr,16),"Menu changed original objective preset bytes");
        const auto rules=multiplayer_source_rules(launch.variant,true);
        need(rules.requires_source_engine && rules.team_mode,"Objective launch lost its real source engine/team owner");
        need(launch.selected_teams==std::vector<std::int8_t>{std::int8_t(team)},"Objective validator lost the selected profile team");
        validated=true;
      });
      menu.select(0);menu.accept();
      if(!menu.launch)throw FormatError(name+": "+menu.frame().description);
      need(validated && menu.launch->multiplayer && menu.launch->local_players==1 &&
           menu.launch->selected_profiles.size()==1 && menu.launch->selected_teams==std::vector<std::int8_t>{std::int8_t(team)} &&
           menu.launch->host_port==(system_link?2002u:0u),"Normal objective Start did not hand off the validated profile/team/session");
      ++starts;
    }
    if(!edited_engines[type]) {
      edited_engines[type]=true;
      const auto parameter=std::ranges::find_if(content.parameters,[&](const auto& item) {
        return item.second.parameter==(type==1?50:59);
      });
      need(parameter!=content.parameters.end(),"Original flag/bomb kind selector is absent");
      for(const bool system_link:{false,true})for(const unsigned kind:{0u,1u,2u}) {
        auto menu=selected(content,index,system_link);
        // The actual223 constructor/accept handler owns the working-buffer
        // write, including the full32-bit authored parameter tag context.
        menu.open(223,parameter->first);choose(menu,entry(menu,kind),14);
        bool validated{};
        menu.set_multiplayer_map_validator([&](const FrontendLaunch& launch) {
          const auto variant=source_game_variant(launch.variant);const auto raw=preset.at("raw").get<std::string>();
          for(unsigned n=0;n<variant.bytes.size();++n) {
            const auto expected=n>=0x104 && n<0x108?((kind>>((n-0x104)*8))&255u):std::stoul(raw.substr(n*2,2),nullptr,16);
            need(variant.bytes[n]==expected,"Authored kind selector changed an unrelated original option byte");
          }
          const auto rules=multiplayer_source_rules(launch.variant,true);
          need(rules.requires_source_engine && rules.source_engine==int(type) && rules.team_mode,
              "Edited flag/bomb kind lost its actual source engine");validated=true;
        });
        menu.select(0);menu.accept();
        need(menu.launch && validated && menu.launch->selected_teams==std::vector<std::int8_t>{0} &&
            menu.launch->host_port==(system_link?2002u:0u),"Edited flag/bomb kind failed normal Start");
        ++edited_starts;
      }
    }
  }
  need(starts==36,"Objective Start coverage lost a preset/session/team");
  need(edited_starts==12,"Objective Start lost an authored flag/bomb kind");
  std::cout<<"PASS 36 original preset Starts and 12 authored kind-picker edits, including Neutral Flag, with exact option bytes, committed teams and actual validation\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
