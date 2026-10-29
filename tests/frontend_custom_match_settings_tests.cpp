#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/bomb_defusal.hpp"
#include "../src/ui/frontend_custom_variant_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
void need(bool condition,const std::string& message) {
  if(!condition)throw FormatError(message);
}
void choose(MenuState& menu,unsigned row,unsigned destination) {
  need(row<menu.frame().entries.size(),"Custom match route omitted a requested row");
  const auto source=menu.frame().screen;
  menu.select(row);menu.accept();
  need(menu.frame().screen==destination,"Custom match route "+std::to_string(source)+
       " missed screen "+std::to_string(destination)+": "+menu.frame().description);
}
unsigned row_value(const MenuState& menu,int value) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,value,&MenuEntry::value);
  need(found!=rows.end(),"Custom match menu omitted value "+std::to_string(value));
  return unsigned(found-rows.begin());
}
unsigned row_destination(const MenuState& menu,int value) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,value,&MenuEntry::destination);
  need(found!=rows.end(),"Custom match menu omitted destination "+std::to_string(value));
  return unsigned(found-rows.begin());
}
unsigned row_name(const MenuState& menu,const std::string& name) {
  const auto& rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,name,&MenuEntry::label);
  need(found!=rows.end(),"Custom match menu omitted "+name);
  return unsigned(found-rows.begin());
}
void lobby(MenuState& menu) {
  while(menu.frame().screen!=14 && menu.stack().size()>1)menu.back();
  need(menu.frame().screen==14,"Custom match editor failed to return to the lobby");
}
void defending_host(MenuState& menu) {
  need(menu.lobby_view().has_value(),"Custom match has no real lobby");
  if(menu.lobby_view()->player_team==1)return;
  need(menu.lobby_view()->player_team==0,"Custom match fixture lost its default committed team");
  menu.auxiliary();menu.navigate(1,0);menu.accept();
  need(!menu.launch && menu.frame().screen==14 && menu.lobby_view()->player_team==1,
       "Defusal host could not commit its actual blue-team selection");
}
MenuState selected(const FrontendContent& content,unsigned variant,bool lan,
                   const std::filesystem::path& preferences={},bool defending=true) {
  MenuState menu(content,preferences);menu.accept();choose(menu,0,167);
  menu.text("Custom One");menu.commit_text();menu.back();
  need(menu.frame().screen==6,"Custom profile failed to reach the main menu");
  choose(menu,lan?3:2,30);choose(menu,0,14);
  need(menu.controller_slot(21,true)==0 && menu.controller_slot(22,true)==1 && menu.frame().screen==10,
       "Custom match could not join its second local controller");
  choose(menu,0,167);menu.text("Custom Two");menu.commit_text();menu.back();
  need(menu.frame().screen==14 && menu.local_players().size()==2,
       "Custom match lost its second selected local profile");
  choose(menu,row_destination(menu,23),23);choose(menu,row_destination(menu,207),207);
  const auto& preset=content.bundled_variants.at(variant);
  choose(menu,row_value(menu,int(preset.engine)),208);
  choose(menu,row_value(menu,int(content.variants.size()+variant)),14);
  if(defending && bomb_defusal_variant(preset))defending_host(menu);
  return menu;
}
MenuState resumed(const FrontendContent& content,const std::filesystem::path& preferences) {
  MenuState menu(content,preferences);menu.accept();choose(menu,row_destination(menu,-3),6);
  choose(menu,3,30);choose(menu,0,14);
  need(menu.controller_slot(21,true)==0 && menu.controller_slot(22,true)==1,
       "Reloaded custom match could not join its local controllers");
  choose(menu,row_value(menu,1),14);
  defending_host(menu);return menu;
}
void editor(MenuState& menu,const UiVariant& variant,bool quick) {
  choose(menu,row_destination(menu,23),23);
  if(quick)choose(menu,row_destination(menu,222),222);
  else {
    choose(menu,row_destination(menu,207),207);
    choose(menu,row_value(menu,int(variant.engine)),208);
    menu.select(row_name(menu,variant.name));menu.auxiliary();
    need(menu.frame().screen==178,"Custom match did not expose its mode-aware categories");
    need(menu.custom_match_settings_frame(menu.frame()),"Custom match category page lost its painter policy");
    need(!menu.frame().description.empty(),"Custom match editor omitted its fixed mode rules");
  }
}
unsigned parameter_id(const FrontendContent& content,unsigned datum) {
  if(datum==detail::frontend_defusal_fuse_datum)return native_defusal_fuse_parameter;
  if(datum==detail::frontend_defusal_plant_datum)return native_defusal_plant_parameter;
  if(datum==detail::frontend_defusal_disarm_datum)return native_defusal_disarm_parameter;
  return content.parameters.at(datum).parameter;
}
void defining_control(const UiVariant& variant,unsigned parameter) {
  need(parameter!=15,variant.name+": editor exposed a second ineffective player cap");
  if(parameter==90)return;
  const auto kind=custom_slayer(variant);
  if(kind!=CustomSlayer::none)
    need(parameter!=0 && parameter!=28 && parameter!=89 && parameter!=94 && parameter!=91,
         variant.name+": editor exposed a defining loadout, round, or team control");
  if(kind==CustomSlayer::infection)
    need(!(parameter>=1 && parameter<=9) && parameter!=16 && parameter!=88,
         "Infection editor exposed fixed endings, lives, or faction equipment");
  if(bomb_defusal_variant(variant)) {
    const std::set<unsigned> fixed{0,1,2,3,4,5,6,7,8,9,16,17,18,23,25,26,28,29,30,
        76,77,78,79,80,81,82,83,88,89,91,94};
    need(!fixed.contains(parameter),"Defusal editor exposed a fixed round, loadout, or map-equipment control");
  }
}
void same_variant(const UiVariant& actual,const UiVariant& expected,const std::string& context) {
  need(actual.name==expected.name && actual.engine==expected.engine && actual.values==expected.values,
       context+": option order, native extensions, or selected variant identity changed");
  need(source_game_variant(actual).bytes==source_game_variant(expected).bytes,
       context+": an unrelated original option byte changed");
  need(actual.source_settings.has_value()==expected.source_settings.has_value() &&
       (!actual.source_settings || actual.source_settings->bytes==expected.source_settings->bytes),
       context+": the complete source base was discarded");
  need(custom_slayer(actual)==custom_slayer(expected) &&
       bomb_defusal_variant(actual)==bomb_defusal_variant(expected),context+": mode marker changed");
}
void start(MenuState& menu,const UiVariant& expected,bool lan,const std::string& context,bool defending=true) {
  bool validated{};
  menu.set_multiplayer_map_validator([&](const FrontendLaunch& launch) {
    same_variant(launch.variant,expected,context);validated=true;
  });
  menu.select(row_destination(menu,-4));menu.accept();
  need(menu.launch.has_value(),context+": normal Start refused the offered settings: "+menu.frame().description);
  const auto& launch=*menu.launch;
  same_variant(launch.variant,expected,context);
  const auto rules=slayer_rules(launch.variant,true);
  need(validated && !rules.requires_source_engine && rules.maximum_players>=2 && launch.multiplayer &&
       launch.local_players==2 && launch.selected_profiles.size()==2 &&
       launch.input_instances==std::vector<std::int32_t>{21,22} && launch.host_port==(lan?2002u:0u),
       context+": Start lost its custom owner, joined players, controller inputs, or mode");
  need(launch.selected_teams==(bomb_defusal_variant(expected)?
       (defending?std::vector<std::int8_t>{1,0}:std::vector<std::int8_t>{0,0}):
       std::vector<std::int8_t>{}),context+": Start changed the committed team policy");
  menu.set_multiplayer_map_validator({});
}
void painted_row(FrontendPainter& painter,const Mesh& mesh,MenuState& menu,unsigned row,const std::string& mode) {
  menu.select(row);
  const auto output=painter.draw(menu,5.f,-1,20000,{640,480});
  need(!output.vertices.empty(),"Custom settings page produced no painter output");
  for(const auto& vertex:output.vertices)
    need(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) && vertex.material<mesh.materials.size(),
         "Custom settings page produced invalid painter geometry");
  bool hit{};
  std::map<unsigned,std::array<int,4>> sampled;
  for(int y=0;y<480 && !hit;y+=2)for(int x=0;x<640 && !hit;x+=2)
    if(const auto target=painter.pointer_hit(float(x),float(y));target &&
       target->action==FrontendPointerAction::Entry) {
      const auto [entry,inserted]=sampled.try_emplace(target->index,std::array{x,y,x,y});
      if(!inserted) {
        entry->second[0]=std::min(entry->second[0],x);entry->second[1]=std::min(entry->second[1],y);
        entry->second[2]=std::max(entry->second[2],x);entry->second[3]=std::max(entry->second[3],y);
      }
      hit=target->index==row;
    }
  std::ostringstream diagnostic;
  diagnostic<<mode<<" screen="<<menu.frame().screen<<" title="<<menu.frame().title<<" row="<<row
      <<" label="<<menu.frame().entries.at(row).label<<" first_visible="<<menu.frame().first_visible
      <<" viewport=640x480: row is not reachable through the authored pointer layout; sampled entry rectangles";
  for(const auto& [index,bounds]:sampled)
    diagnostic<<" "<<index<<"=["<<bounds[0]<<","<<bounds[1]<<","<<bounds[2]<<","<<bounds[3]<<"]";
  need(hit,diagnostic.str());
}
void mode_settings(MenuState& menu,const UiVariant& variant) {
  editor(menu,variant,false);
  choose(menu,row_value(menu,int(detail::frontend_custom_variant_category)),221);
  need(menu.frame().entries.size()==3,"Defusal Mode Settings omitted a native timer");
}
}
int main(int argc,char** argv)try {
  need(argc==2,"Usage: frontend_custom_match_settings_tests MAPS");
  const std::filesystem::path maps=argv[1];
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();
  need(content.parameters.size()==92,"Custom metadata changed the original 92-parameter catalog");
  unsigned starts{},quick_starts{},mode_starts{},groups{};
  std::array<bool,3> modes{};
  for(unsigned index=0;index<content.bundled_variants.size();++index) {
    const auto& preset=content.bundled_variants[index];
    const auto kind=custom_slayer(preset);const bool defusal=bomb_defusal_variant(preset);
    if(kind==CustomSlayer::none && !defusal)continue;
    const auto mode=defusal?2u:kind==CustomSlayer::fiesta?0u:1u;modes[mode]=true;
    for(const bool lan:{false,true}) {
      const auto base=selected(content,index,lan);
      auto categories=base;editor(categories,preset,false);
      need(!categories.frame().entries.empty() && categories.frame().entries.size()<=6,
           preset.name+": settings categories do not fit the authored menu buttons");
      const auto category_rows=categories.frame().entries;
      for(unsigned page=0;page<=category_rows.size();++page) {
        const bool quick=page==category_rows.size();
        auto listing=base;editor(listing,preset,quick);
        if(!quick){choose(listing,page,221);++groups;}
        const auto parameters=listing.frame().entries;
        for(unsigned row=0;row<parameters.size();++row) {
          const auto datum=unsigned(parameters[row].value),parameter=parameter_id(content,datum);
          defining_control(preset,parameter);
          auto picker=listing;choose(picker,row,223);
          need(!picker.frame().entries.empty(),preset.name+": offered setting has no selectable values");
          int current{};
          if(parameter<112)current=source_game_variant_get(source_game_variant(preset),int(parameter));
          else {
            const auto rules=bomb_defusal_settings(preset);
            if(parameter==native_defusal_fuse_parameter)current=int(rules.fuse_ticks);
            else if(parameter==native_defusal_plant_parameter)current=int(rules.plant_ticks);
            else if(parameter==native_defusal_disarm_parameter)current=int(rules.defuse_ticks);
            else throw FormatError("Custom settings exposed an unknown native parameter");
          }
          need(picker.frame().entries.at(picker.frame().selected).value==current,
               preset.name+": opening a picker did not show its actual committed value");
          if(parameter==10 && defusal)
            need(parameters[row].label=="Plant Window" && picker.frame().title=="Plant Window",
                 "Defusal round clock was not presented as its plant window");
          const auto choices=picker.frame().entries;
          for(unsigned choice=0;choice<choices.size();++choice) {
            if(parameter==15 || parameter==90)
              need(choices[choice].value>=2,"Custom player cap picker offered fewer seats than its joined local roster");
            auto menu=listing;choose(menu,row,223);
            const auto context=preset.name+" / "+parameters[row].label+" / "+choices[choice].label+
                (lan?" / LAN":" / Split-screen")+(quick?" / Quick Options":"");
            auto expected=preset;expected.values.set(parameter,choices[choice].value);
            (void)slayer_rules(expected,true);
            choose(menu,choice,quick?222:221);
            choose(menu,row,223);
            need(menu.frame().entries.at(menu.frame().selected).value==choices[choice].value,
                 context+": reopening the picker discarded its exact value");
            menu.back();lobby(menu);start(menu,expected,lan,context);
            ++starts;if(quick)++quick_starts;if(parameter>=native_slayer_ruleset_parameter)++mode_starts;
          }
        }
      }
    }
  }
  need(std::ranges::all_of(modes,[](bool value){return value;}) && starts>100 && quick_starts>0 && mode_starts>40,
       "Custom settings coverage lost a mode, shared picker, Quick Options, or native timer");
  const auto preset=std::ranges::find_if(content.bundled_variants,bomb_defusal_variant);
  need(preset!=content.bundled_variants.end(),"Defusal persistence fixture has no bundled definition");
  const auto index=unsigned(preset-content.bundled_variants.begin());
  auto same_team=selected(content,index,false,{},false);
  need(std::ranges::all_of(same_team.local_players(),[](const auto& player){return player.team==0;}),
       "Defusal team guard fixture did not begin with the default all-Red local roster");
  same_team.select(row_destination(same_team,-4));same_team.accept();
  const auto& team_error=same_team.frame().description;
  need(!same_team.launch && team_error.find("Red")!=std::string::npos &&
       team_error.find("Blue")!=std::string::npos && team_error.find("Team")!=std::string::npos,
       "Split-screen Defusal did not explain its two-team requirement before launch: "+team_error);
  lobby(same_team);defending_host(same_team);
  start(same_team,*preset,false,"Defusal split-screen actual Blue/Red team selection");
  auto awaiting_remote=selected(content,index,true,{},false);
  start(awaiting_remote,*preset,true,"Defusal LAN all-Red locals awaiting a remote Blue player",false);
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Custom persistence requires owned scratch");
  const auto preferences=std::filesystem::path(scratch)/"custom-match-settings.cfg";
  auto saved=selected(content,index,true,preferences);
  auto expected=*preset;expected.values.set(native_defusal_fuse_parameter,1367);
  saved.adopt_system_link_configuration(std::filesystem::path(saved.selected_multiplayer_configuration().map).stem().string(),expected);
  mode_settings(saved,expected);choose(saved,row_value(saved,int(detail::frontend_defusal_fuse_datum)),223);
  need(saved.frame().entries.at(saved.frame().selected).value==1367,
       "Native timer picker rounded or discarded a valid nonintegral-second fuse");
  need(saved.frame().entries.at(saved.frame().selected).label.find("seconds")!=std::string::npos,
       "Native timer picker displayed raw tick units to the player");
  saved.back();lobby(saved);
  const auto fonts=cache.is_mcc()?maps.parent_path()/"h2_fonts":maps/"fonts";
  need(std::filesystem::is_directory(fonts),"Custom settings painter requires its actual menu font directory");
  Mesh mesh;FrontendPainter painter(content,fonts,mesh);
  auto category_page=saved;editor(category_page,expected,false);
  need(category_page.frame().entries.at(row_value(category_page,int(detail::frontend_custom_variant_category))).label=="Mode Settings",
       "Defusal category page lost its native settings label");
  for(unsigned row=0;row<category_page.frame().entries.size();++row)painted_row(painter,mesh,category_page,row,expected.name);
  for(int y=0;y<480;y+=2)for(int x=0;x<640;x+=2)
    if(const auto target=painter.pointer_hit(float(x),float(y));target && target->action==FrontendPointerAction::Entry)
      need(target->index<category_page.frame().entries.size(),"Custom category painter exposed a hidden unused button");
  mode_settings(saved,expected);
  for(unsigned row=0;row<3;++row) {
    painted_row(painter,mesh,saved,row,expected.name);
    auto picker=saved;choose(picker,row,223);painted_row(painter,mesh,picker,picker.frame().selected,expected.name);
  }
  choose(saved,row_value(saved,int(detail::frontend_defusal_plant_datum)),223);
  choose(saved,row_value(saved,180),221);expected.values.set(native_defusal_plant_parameter,180);
  choose(saved,row_value(saved,int(detail::frontend_defusal_disarm_datum)),223);
  choose(saved,row_value(saved,900),221);expected.values.set(native_defusal_disarm_parameter,900);
  choose(saved,row_value(saved,int(detail::frontend_defusal_fuse_datum)),223);
  for(const auto& choice:saved.frame().entries)
    need(choice.value>900,"Fuse picker offered a value that cannot outlast the committed defuse time");
  need(saved.frame().entries.at(saved.frame().selected).value==1367,
       "Editing the other native timers discarded the custom fuse value");
  saved.back();lobby(saved);
  auto short_fuse=saved;auto short_variant=expected;short_variant.values.set(native_defusal_fuse_parameter,300);
  short_variant.values.set(native_defusal_disarm_parameter,180);
  short_fuse.adopt_system_link_configuration(std::filesystem::path(short_fuse.selected_multiplayer_configuration().map).stem().string(),short_variant);
  mode_settings(short_fuse,short_variant);
  choose(short_fuse,row_value(short_fuse,int(detail::frontend_defusal_disarm_datum)),223);
  for(const auto& choice:short_fuse.frame().entries)
    need(choice.value<300,"Defuse picker offered a duration at or above the committed fuse");
  saved.open(194,1);choose(saved,1,167);
  while(!saved.keyboard_text.empty())saved.erase_text();
  saved.text("Custom Bomb");saved.commit_text();lobby(saved);expected.name="Custom Bomb";
  start(saved,expected,true,"Copied Defusal native timers");
  auto reopened=resumed(content,preferences);start(reopened,expected,true,"Reloaded Defusal native timers");
  auto chosen=resumed(content,preferences);editor(chosen,expected,false);lobby(chosen);
  choose(chosen,row_destination(chosen,23),23);choose(chosen,row_destination(chosen,207),207);
  choose(chosen,row_value(chosen,int(expected.engine)),208);choose(chosen,row_name(chosen,expected.name),14);
  start(chosen,expected,true,"Saved named Defusal selection");
  std::cout<<"PASS three custom modes, "<<groups<<" category routes, "<<starts<<" exact ordered/buffer edited Starts ("
      <<quick_starts<<" Quick Options, "<<mode_starts<<" native timers), LAN and two local controllers\n";
  std::cout<<"PASS fixed defining rules, roster-aware player caps, split-screen two-team guard, LAN pending remote teams, exact nonintegral timers, safe timer combinations, named copy/reopen and reachable CPU-painted timer rows\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
