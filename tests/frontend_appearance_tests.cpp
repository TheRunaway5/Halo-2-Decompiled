#include "halo2/frontend.hpp"
#include "halo2/model.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
FrontendAppearance unpack(const std::string &hex){
 check(hex.size()==18,"invalid original appearance bytes");
 std::array<std::uint8_t,9> bytes;
 for(unsigned i=0;i<9;++i)bytes[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
 return std::bit_cast<FrontendAppearance>(bytes);
}
void flow(const FrontendContent &content,const std::filesystem::path &preferences){
 std::filesystem::remove(preferences);
 MenuState menu(content,preferences);
 menu.accept();menu.select(0);menu.accept();menu.commit_text();
 check(menu.frame().screen==38,"appearance regression must enter new-profile customization");
 menu.select(1);menu.accept();check(menu.frame().screen==47,"appearance page route differs from original");
 check(menu.appearance()==FrontendAppearance{},"new profile lacks original appearance defaults");
 menu.select(1);menu.accept();
 check(menu.frame().screen==50 && menu.frame().selected==10,"primary color page does not focus original default10");
 menu.select(7);
 check(menu.appearance().colors[0]==7,"focused color does not update original working preview");
 check(menu.profile_appearance().colors[0]==10,"hover changed committed profile");
 menu.back();check(menu.appearance().colors[0]==10,"B did not restore original child snapshot");
 menu.accept();menu.select(7);menu.accept();
 check(menu.appearance().colors[0]==7 && menu.profile_appearance().colors[0]==10,"accepted child did not retain edit separately from saved profile");
 menu.accept();check(menu.frame().selected==7,"color reopening lost accepted edit");menu.select(3);menu.back();
 check(menu.appearance().colors[0]==7,"cancel did not restore accepted color");
 for(unsigned slot=1;slot<4;++slot){
  menu.select(slot+1);menu.accept();check(menu.frame().context==slot,"color page lost source slot mapping");
  check(menu.frame().selected==unsigned(FrontendAppearance{}.colors[slot]),"secondary/emblem color default focus differs");
  menu.select(6+slot);check(menu.appearance().colors[slot]==int(6+slot),"secondary/emblem focus preview differs");
  menu.back();check(menu.appearance().colors[slot]==FrontendAppearance{}.colors[slot],"secondary/emblem cancel snapshot differs");
  menu.accept();menu.select(6+slot);menu.accept();
 }
 menu.select(0);menu.accept();menu.select(1);
 check(menu.appearance().species==1,"model focus does not preview Elite");menu.back();
 check(menu.appearance().species==0,"model cancellation did not restore Spartan");
 menu.accept();menu.select(1);menu.accept();check(menu.appearance().species==1,"model acceptance lost Elite");
 menu.select(5);menu.accept();menu.select(9);menu.auxiliary();
 check(menu.appearance().foreground==9 && menu.appearance().flags==1,"foreground preview/X flag does not match original");
 menu.back();check(menu.appearance().foreground==0 && menu.appearance().flags==0,"foreground B failed full appearance restore");
 menu.accept();menu.select(9);menu.auxiliary();menu.accept();
 menu.select(6);menu.accept();menu.select(4);menu.auxiliary();
 check(menu.appearance().background==4 && menu.appearance().flags==1,"background X incorrectly toggled foreground flag");
 menu.accept();menu.back();check(menu.frame().screen==38,"appearance return did not restore profile settings");
 menu.back();check(menu.frame().screen==6,"completed profile settings did not sign in");
 const auto expected=menu.appearance();check(expected.colors[0]==7 && expected.species==1 && expected.foreground==9 && expected.background==4 && expected.flags==1,"finished appearance fields were lost");
 MenuState loaded(content,preferences);check(loaded.profile_appearance()==expected,"appearance does not persist after edit ends");
 check(!loaded.model_appearance(MenuFrame{30}),"saved profile alone should not bind occupied slot");
 loaded.accept();loaded.select(1);loaded.accept();
 check(loaded.model_appearance(MenuFrame{30})==expected,"signed-in slot did not bind selected profile appearance");
 // Editing another saved profile must not repaint player1's signed-in slot.
 const auto signed_in=*loaded.model_appearance(MenuFrame{30});
 loaded.open(193);loaded.select(0);loaded.accept();loaded.text("Other profile");loaded.commit_text();
 check(loaded.frame().screen==38,"second profile did not enter original settings route");
 loaded.select(1);loaded.accept();loaded.select(1);loaded.accept();loaded.select(12);loaded.accept();
 check(loaded.appearance().colors[0]==12 && loaded.model_appearance(MenuFrame{30})==signed_in,"uncommitted other-profile edit changed signed-in body colors");
 loaded.back();loaded.back();
 const auto profile_picker=loaded.frame();
 check(profile_picker.screen==193,"other profile completion did not return to Edit Profile");
 const auto focus=loaded.focused_profile_appearance(profile_picker);
 check(focus &&focus->colors[0]==12 &&loaded.model_appearance(profile_picker)==focus,"focused saved-profile bank does not follow edited profile");
 loaded.select(0);check(!loaded.focused_profile_appearance(loaded.frame()),"Create New unexpectedly bound saved appearance");
 std::filesystem::remove(preferences);
}
void rename_edited_profile(const FrontendContent &content,const std::filesystem::path &preferences){
 std::filesystem::remove(preferences);
 MenuState menu(content,preferences);
 menu.accept();menu.select(0);menu.accept();menu.commit_text();
 menu.back();check(menu.frame().screen==6,"rename regression did not finish profile creation");
 menu.select(4);menu.accept();menu.select(0);menu.accept();
 check(menu.frame().screen==193,"rename regression did not enter the retained profile picker");
 menu.select(1);menu.accept();check(menu.frame().screen==194,"rename regression did not choose the saved profile");
 menu.select(0);menu.accept();check(menu.frame().screen==38,"rename regression did not enter saved profile settings");
 menu.select(1);menu.accept();menu.select(1);menu.accept();menu.select(5);menu.accept();
 check(menu.appearance().colors[0]==5 &&menu.profile_appearance().colors[0]==10,
       "rename regression lacks a pending original appearance edit");
 menu.back();check(menu.frame().screen==38,"rename regression lost profile settings parent");
 menu.auxiliary();check(menu.frame().screen==167,"rename regression did not use profile name callback");
 menu.text("Renamed profile");menu.commit_text();
 check(menu.profile_name()=="Renamed profile" &&menu.profile_appearance().colors[0]==5,
       "successful rename discarded the original pending appearance edit");
 MenuState loaded(content,preferences);
 check(loaded.profile_name()=="Renamed profile" &&loaded.profile_appearance().colors[0]==5,
       "renamed saved profile does not retain accepted appearance edits");
 menu.back();check(menu.frame().screen==194,"rename regression lost the retained profile actions");
 menu.back();check(menu.frame().screen==193,"rename regression did not return to the retained profile picker");
 // Original238742 ends the edit;24A01F detects the cleared profile index and
 // 24A150 rebuilds names while retaining the focused datum's slot.
 check(menu.frame().selected==1 &&menu.frame().first_visible==0 &&
       menu.frame().entries.at(1).label=="Renamed profile" &&
       menu.frame().entries.at(1).value==0,
       "retained profile picker still displays the old name after rename");
 const auto preview=menu.focused_profile_appearance(menu.frame());
 check(preview &&preview->colors[0]==5 &&preview->species==0,
       "retained renamed profile lost its focused appearance preview");
 std::filesystem::remove(preferences);
}
}
int main(int argc,char**argv){try{
 if(argc!=3)throw FormatError("appearance check needs original fixture and maps");
 const auto maps=std::filesystem::path(argv[2]);
 if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(maps/"mainmenu.map"))return 77;
 Cache cache(maps/"mainmenu.map");const auto content=cache.load_frontend();
 check(content.appearance_palette.size()==18,"retail authored palette count differs");
 std::ifstream input(argv[1]);unsigned colors=0,palette=0,previews=0,names=0;
 for(std::string line;std::getline(input,line);){if(line.empty()||line[0]=='#')continue;std::istringstream row(line);std::string first;row>>first;
  if(first=="palette"){
   unsigned i;row>>i;const auto actual=std::bit_cast<std::array<std::uint32_t,3>>(content.appearance_palette.at(i));
   for(const auto word:actual){std::string value;row>>value;check(word==std::stoul(value,nullptr,16),"palette words differ from literal original cache");}++palette;
  }else if(first=="name"){
   unsigned i;row>>i;FrontendAppearance appearance;appearance.colors[0]=std::int8_t(i);
   const auto actual=std::bit_cast<std::array<std::uint32_t,3>>(frontend_appearance_name_color(appearance,content.appearance_palette));
   for(const auto word:actual){std::string value;row>>value;check(word==std::stoul(value,nullptr,16),"name tint differs from original2B00F0/HSV corridor");}++names;
  }else if(first=="default"){std::string bytes;row>>bytes;check(FrontendAppearance{}==unpack(bytes),"new appearance defaults differ from original1A0750");
  }else if(first=="preview"){
   unsigned page,slot,gate;int focus;std::string before,after;row>>page>>slot>>focus>>gate>>before>>after;
   auto value=unpack(before);preview_frontend_appearance(value,page,slot,focus,gate&2);
   check(value==unpack(after),"focus producer differs from original child update");++previews;
  }else{
   const auto appearance=unpack(first);
   const auto actual=std::bit_cast<std::array<std::uint32_t,12>>(frontend_appearance_colors(appearance,content.appearance_palette));
   for(const auto word:actual){std::string value;row>>value;check(word==std::stoul(value,nullptr,16),"profile colors differ from original7F790/1A06A0");}++colors;
  }
  check(bool(row),"original appearance fixture is incomplete");
 }
 check(colors==41 &&palette==18 &&previews==192 &&names==18,"appearance source fixture coverage changed");
 // The production binding writes each model's own shader color bank; original
 // object colors pass through the same established131ED0/131BB0 boundary.
 for(const auto datum:{0xe23400bfu,0xe3110154u}){
  auto model=cache.load_model(datum,true);FrontendAppearance value;value.colors={1,7,13,17};
  apply_frontend_appearance(model.mesh.materials,value,content.appearance_palette);
  const auto expected=source_object_render_colors(frontend_appearance_colors(value,content.appearance_palette));
  for(auto&m:model.mesh.materials){check(m.object_render_colors.has_value(),"model shader lacks raw profile color bank");
   check(std::bit_cast<std::array<std::uint32_t,12>>(*m.object_render_colors)==std::bit_cast<std::array<std::uint32_t,12>>(expected),"model shader bank does not use source packed draw colors");
   reapply_object_colors(m);check(std::bit_cast<std::array<std::uint32_t,12>>(*m.object_render_colors)==std::bit_cast<std::array<std::uint32_t,12>>(expected),"shader animation lost current profile bank");}
 }
 flow(content,std::filesystem::temp_directory_path()/"halo2-frontend-appearance-test.cfg");
 rename_edited_profile(content,std::filesystem::temp_directory_path()/"halo2-frontend-appearance-rename-test.cfg");
 std::cout<<colors<<" original color cases, "<<palette<<" palette entries, "<<previews<<" preview cases; creation/focus/cancel/accept/save/reload and model materials passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
