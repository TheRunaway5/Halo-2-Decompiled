#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::string ascii(std::u16string_view s){std::string out;for(auto c:s){need(c<128,"End title unexpectedly requires a different locale provider");out+=char(c);}return out;}
Json rect(const FullScoreboardRect &r){return {r.top,r.left,r.bottom,r.right};}
void relative_rect(const FullScoreboardRect &actual,const FullScoreboardRect &origin,
    const Json &expected,const Json &expected_origin){
  const std::array<int,4> a{actual.top-origin.top,actual.left-origin.left,actual.bottom-origin.top,actual.right-origin.left};
  const std::array<int,4> e{expected[0].get<int>()-expected_origin[0].get<int>(),expected[1].get<int>()-expected_origin[1].get<int>(),
    expected[2].get<int>()-expected_origin[0].get<int>(),expected[3].get<int>()-expected_origin[1].get<int>()};
  need(a==e,"Submitted end-screen widget geometry differs from unchanged original layout fixture");
}
}
int main(int argc,char **argv)try{
  std::cout<<std::unitbuf;need(argc==7,"Supply MCC_MAP PRESETS ORIGINAL_LAYOUT_REPORT REPORT CAPTURES PRESET");
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"End presentation requires owned scratch");
  const std::filesystem::path map=argv[1],captures=argv[5];const std::string name=argv[6];
  Json presets,reference;std::ifstream(argv[2])>>presets;std::ifstream(argv[3])>>reference;
  need(reference.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","End-screen original fixture pin differs");
  const auto selected=std::find_if(presets.at("variants").begin(),presets.at("variants").end(),[&](const auto &p){return p.at("name")==name;});
  need(selected!=presets.at("variants").end(),"Requested original end-screen preset is missing");
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.source_object_world=true;
  options.variant={name,selected->at("frontend_engine").get<unsigned>(),{}};
  for(const auto &write:selected->at("writes"))options.variant.values.append(write[0].template get<unsigned>(),write[1].template get<int>());
  const bool two_round=name=="Slayer";options.variant.values.append(0,two_round?1:0);options.variant.values.append(10,3);
  const auto variant=source_game_variant(options.variant);need(source_game_variant_valid(variant),"Original timed round settings are invalid");
  const bool team=source_game_variant_get(variant,91)!=0;
  options.local_players=2;if(team)options.selected_teams={0,1};
  options.width=640;options.height=480;options.hidden=options.mute=true;options.frames=900;
  options.reference_tick_cadence=true;options.validation=false;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/("end-screen-"+std::to_string(selected->at("source_type").get<unsigned>()));
  std::filesystem::create_directories(options.source_object_savegame_directory);std::filesystem::create_directories(captures);
  std::set<std::string> captured;std::map<std::string,Json> observations;std::map<std::pair<int,int>,std::uint32_t> entry_ticks;
  Json transitions=Json::array();Json fade_observations=Json::object();bool opening_black{},world_clear{},ending_black{};
  MultiplayerState current;bool actual_source{},quit{},second_hud{},completed{};unsigned submitted{},draws{};
  std::pair<int,int> previous{-1,-1};
  options.session_world_loaded=[&](const MultiplayerMatch &m,std::span<const unsigned> slots){
    need(m.source_engine_owned() && m.source_game_type()==2 && slots.size()==2,"End runtime did not use its actual multiplayer source owner");
    need(m.source_engine()==selected->at("source_type"),"End runtime changed the selected original engine");actual_source=true;};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState &s){
    current=s;need(s.source_round_lifecycle_available,"Actual end world omitted round lifecycle publication");
    const auto phase=std::pair{int(s.round_phase),int(s.round_index)};
    if(phase!=previous){transitions.push_back({{"tick",s.tick},{"phase",s.round_phase},{"index",s.round_index},
      {"countdown",s.round_countdown},{"finished",s.finished}});entry_ticks.try_emplace(phase,s.tick);previous=phase;}
    if(s.game_over_complete()){completed=true;if(!quit){SDL_Event e{};e.type=SDL_QUIT;need(SDL_PushEvent(&e)==1,"Could not leave completed end-screen test");quit=true;}}
  };
  options.gameplay_fade_observer=[&](unsigned view,std::uint32_t tick,std::optional<float> alpha,const SourceGameplayFadeStateTu15 &state) {
    if(view!=0)return;
    need(current.source_gameplay_fade.has_value(),"Actual original world omitted pointer-free viewport fade inputs");
    const auto observe=[&](const char *name){if(!fade_observations.contains(name))fade_observations[name]={{"tick",tick},
      {"phase",current.round_phase},{"round_index",current.round_index},{"phase_counter",state.phase_counter},
      {"alpha_bits",alpha?Json(std::bit_cast<std::uint32_t>(*alpha)):Json(nullptr)},
      {"game_over_countdown",current.game_over_countdown},{"source_sequence",current.source_gameplay_fade->newest}};};
    if(!opening_black && alpha && *alpha==1.f && tick<30){opening_black=true;observe("opening-black");}
    if(!current.finished && alpha && *alpha>0.f && *alpha<1.f)observe("viewport-partial");
    if(!alpha){world_clear=true;observe("world-clear");}
    if(current.finished && alpha && *alpha==1.f && current.game_over_countdown>0){ending_black=true;observe("ending-world-black");}
  };
  options.scoreboard_observer=[&](unsigned view,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint){
    ++draws;if(view!=0)return;
    if(two_round && current.round_phase==1 && current.round_index==1 && normal && paint.draw.vertices.empty())second_hud=true;
    if(normal || layout.status!=FullScoreboardStatus::Ready || paint.draw.vertices.empty())return;
    const auto title=ascii(layout.title.text);
    std::string phase;
    if(current.finished){need(title==reference.at("labels")[0].get<std::string>(),"Actual final scoreboard omitted original GAME OVER title");phase="game-over";}
    else if(current.round_phase==2){need(title==reference.at("labels")[1].get<std::string>(),"Actual intermission omitted original ROUND OVER title");phase="round-over";}
    else if(current.round_phase==3){need(title==reference.at("labels")[2].get<std::string>(),"Actual preparation omitted original next-round title");phase="preparation";}
    else return;
    // Ready geometry can be submitted during original zero-opacity fade-in.
    // Capture only a genuinely visible automatic scoreboard, without changing
    // its held/fade input or source phase clock.
    if(layout.input_alpha<.9f)return;
    need(!paint.emblems_unavailable && layout.rows.size()==2 && layout.team_rows.size()==(team?2u:0u),"Actual end-screen lost original roster/teams/emblem pass");
    const Json *expected=nullptr;
    for(const auto &item:reference.at("layouts"))if(item.at("team_mode")==team && item.at("game_over")==current.finished &&
        item.at("rounds")==bool(current.round_mode) && item.at("score_one")==bool(current.score_limit==1)) {expected=&item;break;}
    need(expected,"No unchanged original column-layout control matches the actual submitted end screen");
    need(expected->at("columns")==Json(layout.columns),"Actual end-screen column flags differ from unchanged original controls");
    const auto &widgets=expected->at("headers");const auto &origin=expected->at("title").at("rectangle");
    relative_rect(layout.title.rectangle,layout.title.rectangle,origin,origin);
    relative_rect(layout.title.text_rectangle,layout.title.rectangle,expected->at("title").at("text_rectangle"),origin);
    const std::array<const char*,4> keys{"place","name","wins","score"};
    for(unsigned i=0;i<4;++i){
      const auto &e=widgets.at(keys[i]);need(layout.headers[i].valid==e.at("valid").template get<bool>(),"Original end-screen header visibility differs");
      if(layout.headers[i].valid){relative_rect(layout.headers[i].rectangle,layout.title.rectangle,e.at("rectangle"),origin);
        relative_rect(layout.headers[i].text_rectangle,layout.title.rectangle,e.at("text_rectangle"),origin);
        need(layout.headers[i].right_justified==e.at("right_justified").template get<bool>(),"Original end-screen header justification differs");
        for(unsigned c=1;c<4;++c){
          need(std::bit_cast<std::uint32_t>(layout.headers[i].background[c])==std::stoul(e.at("background_bits")[c].template get<std::string>(),nullptr,16),"Original end-screen header RGB differs");
          need(std::bit_cast<std::uint32_t>(layout.headers[i].foreground[c])==std::stoul(e.at("text_color_bits")[c].template get<std::string>(),nullptr,16),"Original end-screen text RGB differs");
        }
      }
    }
    if(!observations.contains(phase))observations[phase]={{"tick",current.tick},{"phase",current.round_phase},{"index",current.round_index},{"round_countdown",current.round_countdown},
      {"clock_seconds",current.clock_seconds},{"game_over_countdown",current.game_over_countdown},{"title",title},{"columns",layout.columns},
      {"title_rectangle",rect(layout.title.rectangle)},{"rows",layout.rows.size()},{"teams",layout.team_rows.size()},{"vertices",paint.draw.vertices.size()}};
  };
  options.frame_capture=[&](const MultiplayerState &s)->std::filesystem::path{
    for(const auto &[phase,observation]:fade_observations.items())if(observation.at("tick")==s.tick && captured.insert(phase).second)return captures/(phase+".ppm");
    for(const auto &[phase,observation]:observations)if(observation.at("tick")==s.tick && captured.insert(phase).second)return captures/(phase+".ppm");
    return {};
  };
  options.frame_observer=[&](const auto &f){submitted+=f.submitted;};
  MultiplayerRuntimeOutcome outcome;need(run_multiplayer(options,outcome)==0,"Actual end runtime failed");
  need(actual_source && completed && outcome.state && outcome.state->game_over_complete() && submitted>30 && draws>30 && captured.contains("game-over"),
    "Actual rendered source ending never completed original game-over presentation");
  need(opening_black && world_clear && ending_black && fade_observations.contains("viewport-partial"),
    "Actual source viewport did not render original first-black, exact fade, visible world and final-black-before-completion");
  for(const auto &phase:captured)need(std::filesystem::is_regular_file(captures/(phase+".ppm")) &&
      std::filesystem::file_size(captures/(phase+".ppm"))>=640u*480u*3u,"Original end-screen frame was not captured by the actual GPU submission");
  if(two_round){need(captured.contains("round-over") && captured.contains("preparation") && second_hud,"Original two-round intermission/preparation/HUD restoration was not rendered");
    need(entry_ticks.at({3,1})-entry_ticks.at({2,0})==150 && entry_ticks.at({1,1})-entry_ticks.at({3,1})==30,"Actual original5s intermission/1s preparation timing differs");}
  Json report={{"passed",true},{"preset",name},{"source_engine",selected->at("source_type")},{"protocol",native_multiplayer_protocol},
    {"actual_source_owner",actual_source},{"submitted_frames",submitted},{"draw_observations",draws},{"transitions",transitions},{"observations",observations},{"viewport_fades",fade_observations},
    {"scope","Actual MCC source world and Vulkan draw with natural original timed endings, automatic scoreboard held/fade kernel and original title/column/relative geometry controls. Two-round Slayer includes5s intermission/1s preparation/HUD restoration; selected source family ends naturally. No score/phase/UI status is supplied. This local GPU fixture does not prove ONLINE consent/PCR navigation or original Xbox framebuffer equality."}};
  std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';need(bool(out),"Could not retain actual end-screen report");
  std::cout<<"PASS actual source "<<name<<" -> automatic original end/round titles, geometry, GPU frames and completion\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
