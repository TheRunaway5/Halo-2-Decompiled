#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

using namespace halo2;
namespace {
void need(bool condition,const std::string& message) {
  if(!condition)throw FormatError(message);
}
void choose(MenuState& menu,unsigned row,unsigned screen) {
  menu.select(row);menu.accept();
  need(menu.frame().screen==screen,"Match customization missed screen "+std::to_string(screen));
}
unsigned row_value(const MenuState& menu,int value) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,value,&MenuEntry::value);
  need(found!=rows.end(),"Match customization lost an authored row");
  return unsigned(found-rows.begin());
}
unsigned row_destination(const MenuState& menu,int destination) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,destination,&MenuEntry::destination);
  need(found!=rows.end(),"Match customization lost an authored action");
  return unsigned(found-rows.begin());
}
MenuState selected(const FrontendContent& content,unsigned variant,bool lan,
                   const std::filesystem::path& preferences={}) {
  MenuState menu(content,preferences);menu.accept();choose(menu,0,167);
  menu.text("Customize One");menu.commit_text();menu.back();
  need(menu.frame().screen==6,"Match customization profile did not reach the main menu");
  choose(menu,lan?3:2,30);choose(menu,0,14);
  need(menu.controller_slot(101,true)==0,"Local host input lost its slot");
  need(menu.controller_slot(102,true)==1 && menu.frame().screen==10,
       "Second local player could not enter the profile picker");
  choose(menu,0,167);menu.text("Customize Two");menu.commit_text();menu.back();
  need(menu.frame().screen==14 && menu.local_players().size()==2,
       "Second local player did not join the match lobby");
  choose(menu,row_destination(menu,23),23);
  choose(menu,row_destination(menu,207),207);
  choose(menu,row_value(menu,int(content.variants.at(variant).engine)),208);
  choose(menu,row_value(menu,int(variant)),14);
  return menu;
}
MenuState resumed(const FrontendContent& content,const std::filesystem::path& preferences) {
  MenuState menu(content,preferences);menu.accept();
  choose(menu,row_destination(menu,-3),6);
  choose(menu,3,30);choose(menu,0,14);
  need(menu.controller_slot(101,true)==0 && menu.controller_slot(102,true)==1,
       "Reloaded LAN match lost local controller join");
  choose(menu,row_value(menu,1),14);
  need(menu.local_players().size()==2,"Reloaded LAN match did not restore both profiles");
  return menu;
}
void editor(MenuState& menu,unsigned engine,bool quick,unsigned category) {
  choose(menu,row_destination(menu,23),23);
  if(quick)choose(menu,row_destination(menu,222),222);
  else {
    choose(menu,row_destination(menu,207),207);
    choose(menu,row_value(menu,int(engine)),208);
    menu.auxiliary();need(menu.frame().screen==178,"Change Rules did not expose all editing categories");
    choose(menu,category,221);
  }
}
void lobby(MenuState& menu) {
  while(menu.frame().screen!=14 && menu.stack().size()>1)menu.back();
  need(menu.frame().screen==14,"Match editing failed to return to the original lobby");
}
void start(MenuState& menu,const SourceGameVariant& expected,bool lan,const std::string& context) {
  bool validated{};
  menu.set_multiplayer_map_validator([&](const FrontendLaunch& launch) {
    need(source_game_variant(launch.variant).bytes==expected.bytes,context+": validator changed option bytes");
    validated=true;
  });
  menu.select(row_destination(menu,-4));menu.accept();
  need(menu.launch.has_value(),context+": Start refused the edited match: "+menu.frame().description);
  const auto& launch=*menu.launch;
  need(validated && source_game_variant(launch.variant).bytes==expected.bytes,
       context+": Start failed to carry the exact edited source buffer");
  need(launch.multiplayer && launch.host_port==(lan?2002u:0u) && launch.local_players==2 &&
       launch.selected_profiles.size()==2 && launch.input_instances==std::vector<std::int32_t>{101,102},
       context+": Start lost the mode, joined profiles or their controller inputs");
  const auto rules=multiplayer_source_rules(launch.variant,true);
  need(rules.requires_source_engine,"Authored match editing lost its original game engine");
  need(launch.selected_teams.size()==(rules.team_mode?2u:0u),context+": Start lost the edited team policy");
}
}
int main(int argc,char** argv)try {
  need(argc==2,"Usage: frontend_match_customization_tests MAPS");
  const auto maps=std::filesystem::path(argv[1]);
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();
  need(content.parameters.size()==92,"The complete original match parameter catalog was not loaded");
  std::set<unsigned> reached;
  unsigned starts{},quick_starts{},categories{};
  for(const bool lan:{false,true})for(unsigned engine=0;engine<7;++engine) {
    const auto preset=std::ranges::find(content.variants,engine,&UiVariant::engine);
    need(preset!=content.variants.end(),"A match engine has no selectable authored preset");
    const auto index=unsigned(preset-content.variants.begin());
    const auto base=selected(content,index,lan);
    const auto original=source_game_variant(*preset);
    for(unsigned page=0;page<7;++page) {
      const bool quick=page==6;
      auto listing=base;editor(listing,engine,quick,page);
      if(!quick)++categories;
      const auto parameters=listing.frame().entries;
      for(unsigned parameter_row=0;parameter_row<parameters.size();++parameter_row) {
        const auto datum=unsigned(parameters[parameter_row].value);
        const auto& parameter=content.parameters.at(datum);
        reached.insert(datum);
        need(!parameter.choices.empty(),"An authored match parameter has no choices");
        for(unsigned choice=0;choice<parameter.choices.size();++choice) {
          const auto context=std::string(lan?"LAN":"Split-screen")+" engine "+std::to_string(engine)+
              " / "+parameter.title+" / "+parameter.choices[choice].label+(quick?" (Quick Options)":"");
          auto menu=base;editor(menu,engine,quick,page);
          choose(menu,parameter_row,223);
          need(menu.frame().entries.size()==parameter.choices.size(),context+": picker omitted an authored choice");
          auto expected=original;
          source_game_variant_set(expected,int(parameter.parameter),parameter.choices[choice].value);
          need(source_game_variant_valid(expected),context+": authored choice produced invalid source settings");
          choose(menu,choice,quick?222:221);
          choose(menu,parameter_row,223);
          need(menu.frame().selected==choice,context+": reopening the picker lost its selected value");
          menu.back();lobby(menu);start(menu,expected,lan,context);
          ++starts;if(quick)++quick_starts;
        }
      }
    }
  }
  need(reached.size()==content.parameters.size(),"Some authored match parameters cannot be reached from LAN/split-screen Setup");
  unsigned energy_starts{};
  for(const bool lan:{false,true})for(const int map_id:{20,40}) {
    const auto stock=std::ranges::find(content.variants,0u,&UiVariant::engine);
    auto menu=selected(content,unsigned(stock-content.variants.begin()),lan);
    const auto map=std::ranges::find(content.multiplayer_maps,map_id,&UiLobbyMap::map_id);
    need(map!=content.multiplayer_maps.end(),"Midship/Waterworks is absent from the authored map catalog");
    choose(menu,row_destination(menu,23),23);
    choose(menu,row_destination(menu,206),206);
    choose(menu,row_value(menu,int(map-content.multiplayer_maps.begin())),14);
    start(menu,source_game_variant(*stock),lan,"Authored energy-weapon map "+map->name);
    ++energy_starts;
  }
  // A LAN host can adopt the full source buffer received from an existing
  // session. Later menu edits must retain fields absent from the setter list,
  // including the source title, rather than rebuilding defaults from a name.
  const auto stock=std::ranges::find(content.variants,0u,&UiVariant::engine);
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
  need(scratch,"Match customization persistence requires owned test scratch");
  const auto preferences=std::filesystem::path(scratch)/"match-customization.cfg";
  auto imported=selected(content,unsigned(stock-content.variants.begin()),true,preferences);
  UiVariant wire=*stock;wire.values={};wire.source_settings=source_game_variant(*stock);
  source_game_variant_set_title(*wire.source_settings,u"Retained LAN settings");
  imported.adopt_system_link_configuration(std::filesystem::path(imported.selected_multiplayer_configuration().map).stem().string(),wire);
  auto expected=*wire.source_settings;
  need(source_game_variant(imported.selected_multiplayer_configuration().variant).bytes==expected.bytes,
       "Adopting LAN settings discarded fields not represented by parameter writes");
  editor(imported,0,true,0);
  need(!imported.frame().entries.empty(),"Imported LAN settings lost Quick Options");
  const auto datum=unsigned(imported.frame().entries.front().value);
  const auto& parameter=content.parameters.at(datum);
  choose(imported,0,223);
  const auto choice=unsigned(parameter.choices.size()-1);
  source_game_variant_set(expected,int(parameter.parameter),parameter.choices[choice].value);
  choose(imported,choice,222);lobby(imported);
  imported.open(194,1);choose(imported,1,167);
  while(!imported.keyboard_text.empty())imported.erase_text();
  imported.text("Retained Match");imported.commit_text();lobby(imported);
  start(imported,expected,true,"Imported source settings after Quick Options edit");
  auto reopened=resumed(content,preferences);
  start(reopened,expected,true,"Imported source settings after preferences reopen");
  auto saved=resumed(content,preferences);
  choose(saved,row_destination(saved,23),23);choose(saved,row_destination(saved,207),207);
  choose(saved,row_value(saved,0),208);
  const auto stored=std::ranges::find(saved.frame().entries,std::string("Retained Match"),&MenuEntry::label);
  need(stored!=saved.frame().entries.end(),"Copied custom match was not saved in the variant catalog");
  choose(saved,unsigned(stored-saved.frame().entries.begin()),14);
  start(saved,expected,true,"Copied source settings after saved variant selection");
  auto canceled=resumed(content,preferences);
  choose(canceled,row_destination(canceled,23),23);choose(canceled,row_destination(canceled,207),207);
  choose(canceled,row_value(canceled,1),208);lobby(canceled);
  start(canceled,expected,true,"Browsing another engine and canceling with a saved source buffer");
  auto changed=resumed(content,preferences);
  choose(changed,row_destination(changed,23),23);choose(changed,row_destination(changed,207),207);
  choose(changed,row_value(changed,1),208);
  const auto other=std::ranges::find(content.variants,1u,&UiVariant::engine);
  need(other!=content.variants.end(),"Editor engine-change regression has no authored preset");
  changed.select(row_value(changed,int(other-content.variants.begin())));changed.auxiliary();
  need(changed.frame().screen==178,"Focused preset did not open its editor");
  lobby(changed);start(changed,source_game_variant(*other),true,"Opening another engine's focused preset editor");
  // A raw base may require ordered overrides before it is valid. Persistence
  // must validate the effective settings at admission, preserving the base
  // title rather than throwing away the whole base before applying writes.
  const auto overlay_preferences=std::filesystem::path(scratch)/"match-overlay.cfg";
  auto overlay=selected(content,unsigned(stock-content.variants.begin()),true,overlay_preferences);
  auto corrected=wire;source_game_variant_set(*corrected.source_settings,2,-1);
  source_game_variant_set_title(*corrected.source_settings,u"Corrected LAN source base");
  corrected.values={{2,25}};
  need(!source_game_variant_valid(*corrected.source_settings) && source_game_variant_valid(source_game_variant(corrected)),
       "Ordered override regression did not construct an invalid base with valid effective settings");
  overlay.adopt_system_link_configuration(std::filesystem::path(overlay.selected_multiplayer_configuration().map).stem().string(),corrected);
  overlay.save();
  auto corrected_reopened=resumed(content,overlay_preferences);
  start(corrected_reopened,source_game_variant(corrected),true,"Type-correct raw base with a valid ordered score override after reopen");
  std::cout<<"PASS "<<reached.size()<<" authored parameters, "<<categories<<" category routes, "<<starts
      <<" exact-buffer edited Starts ("<<quick_starts<<" through Quick Options), seven engines, LAN and two-player split-screen; imported source settings retained\n";
  std::cout<<"PASS "<<energy_starts<<" Midship/Waterworks Starts and exact imported settings after editing, copying and preferences reopen\n";
  std::cout<<"PASS canceled cross-engine browsing, focused preset editor commit and corrected raw-base persistence\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
