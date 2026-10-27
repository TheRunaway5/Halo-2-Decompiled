#include "helpers/objective_match_completion_tu15.hpp"
#include "halo2/system_link_session.hpp"
#include "halo2/frontend_session.hpp"
#include "helpers/system_link_proxy.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
using namespace std::chrono_literals;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
void choose(MenuState& menu,unsigned row,unsigned screen){menu.select(row);menu.accept();need(menu.frame().screen==screen,"System Link missed its actual menu screen");}
unsigned entry(const MenuState& menu,int value){const auto& rows=menu.frame().entries;const auto at=std::ranges::find(rows,value,&MenuEntry::value);need(at!=rows.end(),"System Link authored choice absent");return unsigned(at-rows.begin());}
FrontendLaunch select(FrontendSession& frontend,const FrontendContent& content,
    const std::filesystem::path& map,const std::string& preset,const char* name,int team) {
  frontend.retain_content(content);auto& menu=frontend.create_state();menu.accept();choose(menu,0,167);
  menu.text(name);menu.commit_text();menu.back();choose(menu,3,30);choose(menu,0,14);
  const auto m=std::ranges::find_if(content.multiplayer_maps,[&](const auto& v){return v.scenario_path.ends_with(map.stem().string());});
  const auto v=std::ranges::find_if(content.variants,[&](const auto& v){std::string_view n=v.name;if(n.ends_with(" \xee\x80\x88"))n.remove_suffix(4);return n==preset;});
  need(m!=content.multiplayer_maps.end() && v!=content.variants.end(),"System Link original map/preset missing");
  choose(menu,1,23);choose(menu,0,206);choose(menu,entry(menu,int(m-content.multiplayer_maps.begin())),14);
  choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,v->engine),208);choose(menu,entry(menu,int(v-content.variants.begin())),14);
  menu.auxiliary();if(team)menu.navigate(1,0);menu.accept();
  auto launch=menu.selected_multiplayer_configuration();need(launch.host_port && launch.selected_profiles.size()==1 && launch.selected_teams==std::vector<std::int8_t>{std::int8_t(team)},"System Link lost actual profile/team selection");
  return launch;
}
template<class F>void until(F operation,const char* why,std::chrono::seconds limit=30s) {
  const auto end=std::chrono::steady_clock::now()+limit;
  while(!operation()){need(std::chrono::steady_clock::now()<end,why);std::this_thread::sleep_for(2ms);}
}
}
int main(int argc,char** argv)try {
#ifdef _WIN32
  return 77;
#else
  std::cout<<std::unitbuf;need(argc==4,"Supply MCC_MAP ORIGINAL_PRESETS REPORT");
  const std::filesystem::path map=argv[1];if(!std::filesystem::is_regular_file(map))return 77;
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"System Link regression needs owned scratch");
  auto content=Cache(map.parent_path()/"mainmenu.map").load_frontend();content.vista=load_vista_settings(map.parent_path()/"mainmenu.map");
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"transport_profile","native complete snapshots"},{"cases",Json::array()},{"scope","Nine original objective presets, actual raw encrypted UDP peers, natural source match completion, independently checked final snapshots, world-free retained sessions, native PCR Done/leave policy, fresh lobby, source countdown/cancel and next source-world admission. Positions/controller samples are declared inputs; no objective/timer/score/winner/readiness/completion result supplied. Physical/Xbox/cross-machine comparisons remain separate."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';};unsigned count{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto type=preset.at("source_type").get<int>();if(type!=1 && type!=9)continue;
    const auto name=preset.at("name").get<std::string>();if(const auto* only=std::getenv("HALO2_OBJECTIVE_VARIANT");only && name!=only)continue;
    Json row={{"name",name},{"passed",false},{"stage","native selection"}};std::cout<<"System Link return: "<<name<<'\n';
    try {
      FrontendSession host_frontend,client_frontend,late_frontend;
      const bool extended=name=="Single Bomb";
      std::shared_ptr<SystemLinkSession> late_session;std::unique_ptr<LanClient> late;FrontendLaunch late_launch;
      unsigned late_slot{};
      const auto h=select(host_frontend,content,map,name,"Link Host",0),c=select(client_frontend,content,map,name,"Link Client",1);
      MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.variant=h.variant;
      options.local_players=1;options.selected_teams=h.selected_teams;options.selected_profiles=h.selected_profiles;
      options.source_object_world=true;options.source_object_savegame_directory=std::filesystem::path(scratch)/("link-"+std::to_string(count));
      auto world=std::make_shared<MultiplayerPreparedWorld>(options);world->bind_source_local_views(world->local_slots);
      const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,map.parent_path(),0,world->network_starting_weapon_identity()),h.variant);
      auto hs=std::make_shared<SystemLinkSession>(true),cs=std::make_shared<SystemLinkSession>(false);
      auto host=hs->enter_host(world->match,0,fingerprint,h.variant,map.stem().string());const auto port=host->port();
      const auto host_address=host->join_address();test::SystemLinkProxy proxy(port);const auto address=host->join_address("127.0.0.1",proxy.port());
      auto client=cs->enter_client(address,fingerprint,1,c.selected_teams,c.selected_profiles);
      std::array<Controls,multiplayer_capacity> desired{},actual{};std::array<Controls,1> remote{};
      row["stage"]="authenticated admission";
      until([&]{client->send(remote);proxy.pump();host->receive(actual);host->broadcast();proxy.pump();client->receive();return client->ready() && cs->state();},"System Link native admission timed out");
      const auto remote_slot=client->slots()[0];need(remote_slot!=world->local_slots[0],"System Link reused the host slot");
      row["stage"]="complete original match";
      const auto step=[&] {
        remote[0]=desired[remote_slot];client->send(remote);proxy.pump();host->receive(actual);actual[world->local_slots[0]]=desired[world->local_slots[0]];
        world->match.step(actual);host->broadcast();proxy.pump();client->receive();
      };
      row["match"]=test::complete_objective_match_tu15(*world,h.variant,desired,step);
      row["stage"]="confirmed original results";
      until([&]{client->send(remote);proxy.pump();host->receive(actual);host->broadcast();proxy.pump();client->receive();hs->world_update(world->match.state());cs->world_update(client->state());
        return hs->state()->phase==SystemLinkPhase::postgame && cs->state()->phase==SystemLinkPhase::postgame;},"System Link result confirmation timed out");
      const auto identities=hs->state()->roster;const auto source_tick=world->match.state().tick;
      need(hs->state()->report==cs->state()->report,"System Link clients retained different final reports");
      // Delay genuine last-world packets until the retained next match. The
      // proxy never decrypts/edits them, so authentication alone cannot assign
      // a new world generation to this old source snapshot/input.
      proxy.hold_host=true;host->broadcast();proxy.pump();proxy.hold_host=false;
      proxy.hold_client=true;remote[0].fire=remote[0].pickup=remote[0].grenade=true;client->send(remote);proxy.pump();proxy.hold_client=false;remote={};
      hs->retain(std::move(host),{});cs->retain({},std::move(client));
      const std::weak_ptr<MultiplayerPreparedWorld> departed=world;world.reset();need(departed.expired(),"System Link session retained the departed native world");
      row["world_released"]=true;row["stage"]="native PCR";
      host_frontend.enter(FrontendOptions::Entry::postgame_report,hs->report(host_frontend));
      client_frontend.enter(FrontendOptions::Entry::postgame_report,cs->report(client_frontend));
      auto& hm=host_frontend.state();auto& cm=client_frontend.state();
      need(hm.frame().vista_pcr->results->player_count==2 && hm.frame().vista_pcr->results->team_count==2,"System Link PCR lost actual original team/player rows");
      const auto old_report=hm.frame().vista_pcr;
      const auto host_pcr=hm.frame().instance,client_pcr=cm.frame().instance;
      proxy.pump();hs->update_frontend(hm,host_frontend.ui_clock(),{});proxy.pump();cs->update_frontend(cm,client_frontend.ui_clock(),{});proxy.pump();
      cm.back();need(!cm.frame().vista_pcr,"System Link nonhost Done skipped its original leave dialog");cm.back();
      need(bool(cm.frame().vista_pcr),"System Link leave cancellation lost its report");
      hm.back();need(hm.frame().screen==14 && hm.frame().instance!=host_pcr,"System Link host PCR Done did not allocate a fresh lobby");
      until([&]{proxy.pump();hs->update_frontend(hm,host_frontend.ui_clock(),{});proxy.pump();cs->update_frontend(cm,client_frontend.ui_clock(),{});proxy.pump();return cm.frame().screen==14;},"System Link remote PCR did not follow actual host Done");
      need(cm.frame().instance!=client_pcr && hs->state()->roster.size()==2 && cs->state()->roster.size()==2,"System Link return lost its actual remote roster");
      const auto lobby=hm.frame().instance;
      for(unsigned i=0;i<120;++i){proxy.pump();hs->update_frontend(hm,host_frontend.ui_clock(),{});proxy.pump();cs->update_frontend(cm,client_frontend.ui_clock(),{});proxy.pump();}
      need(hm.frame().instance==lobby && !hm.launch && hm.lobby_view()->input.player_count==2 && !cm.lobby_view()->input.host,"System Link resumed a stale screen or fabricated host membership");
      if(extended) {
        late_launch=select(late_frontend,content,map,name,"Link Late",1);
        late_session=std::make_shared<SystemLinkSession>(false);
        late=late_session->enter_client(host_address,fingerprint,1,late_launch.selected_teams,late_launch.selected_profiles);
        until([&]{proxy.pump();hs->pump();proxy.pump();cs->pump();late->service_system_link_lobby();return late_session->state()!=nullptr;},"World-free System Link lobby admission timed out");
        need(!late->ready() && late_session->state()->phase==SystemLinkPhase::lobby && late_session->state()->report.empty(),"Lobby admission manufactured a gameplay snapshot or old results");
        late_slot=late->slots()[0];need(late_slot!=remote_slot && late_slot!=0,"Late System Link client did not get a distinct actual seat");
        late_session->retain({},std::move(late));row["world_free_late_join"]=true;
      }
      hm.select(0);hm.accept();need(hs->state()->countdown==10 && !hm.launch,"System Link next Start bypassed source countdown");hm.back();need(hs->state()->countdown==-1 && !hm.launch,"System Link countdown cancel failed");
      row["next_cancel"]=true;row["stage"]="real next countdown";
      unsigned beeps{};hm.select(0);hm.accept();const auto start=std::chrono::steady_clock::now();
      until([&]{proxy.pump();hs->update_frontend(hm,host_frontend.ui_clock(),[&](auto s){if(s==FrontendUiSound::PregameCountdown)++beeps;});proxy.pump();cs->update_frontend(cm,client_frontend.ui_clock(),{});proxy.pump();if(late_session)late_session->update_frontend(late_frontend.state(),late_frontend.ui_clock(),{});return hm.launch.has_value();},"System Link source countdown did not launch",15s);
      need(!cs->closed() && (!late_session || !late_session->closed()),"Authenticated world-free lobby traffic did not keep the countdown participants alive");
      need(std::chrono::steady_clock::now()-start>=10s && beeps==4,"System Link source countdown lost its actual duration/beep stages");
      const auto next=*hm.launch;hm.launch.reset();need(source_game_variant(next.variant).bytes==source_game_variant(h.variant).bytes && next.selected_profiles==h.selected_profiles,"System Link changed selected source gameplay/profile on return");
      row["stage"]="fresh source world";
      options.source_object_savegame_directory=std::filesystem::path(scratch)/("link-next-"+std::to_string(count));world=std::make_shared<MultiplayerPreparedWorld>(options);
      host=hs->enter_host(world->match,port,fingerprint,next.variant,map.stem().string());need(host->port()==port && host->join_address()==host_address,"System Link replaced its actual retained socket/key");
      until([&]{proxy.pump();host->receive(actual);host->broadcast();proxy.pump();cs->update_frontend(cm,client_frontend.ui_clock(),{});proxy.pump();if(late_session)late_session->update_frontend(late_frontend.state(),late_frontend.ui_clock(),{});return cm.launch.has_value() && (!late_session || late_frontend.state().launch.has_value());},"System Link next host Start was not delivered");
      bool refused_mismatch=false;try{auto rejected=cs->enter_client(address,fingerprint^1u,1,c.selected_teams,c.selected_profiles);}
      catch(const FormatError&){refused_mismatch=true;}
      need(refused_mismatch && cs->state() && cs->state()->phase==SystemLinkPhase::starting,"A mismatched native world consumed the retained connection");
      row["mismatched_preparation_rejected"]=true;
      client=cs->enter_client(address,fingerprint,1,c.selected_teams,c.selected_profiles);cm.launch.reset();remote={};
      if(late_session){late=late_session->enter_client(host_address,fingerprint,1,late_launch.selected_teams,late_launch.selected_profiles);late_frontend.state().launch.reset();}
      until([&]{client->send(remote);if(late)late->send(remote);proxy.pump();host->receive(actual);host->broadcast();proxy.pump();client->receive();if(late)late->receive();return client->ready() && (!late || late->ready()) && hs->state()->phase==SystemLinkPhase::running;},"System Link next world did not admit the retained remote player");
      need(client->slots()[0]==remote_slot && hs->state()->generation==2 && hs->state()->roster.size()==identities.size()+unsigned(extended),"System Link restart changed admitted seats/generation");
      for(unsigned i=0;i<identities.size();++i)need(hs->state()->roster[i].identity==identities[i].identity,"System Link restart replaced a retained native identity");
      const auto delayed=proxy.replay();need(delayed[0]>0 && delayed[1]>0,"System Link delayed-packet fixture retained no real packets");
      client->receive();host->receive(actual);
      need(client->state().tick<=world->match.state().tick && !client->state().finished && !actual[remote_slot].fire && !actual[remote_slot].grenade && !actual[remote_slot].pickup,
          "A delayed previous-world snapshot/input entered the next source match");
      bool stale_rejected=false;try{old_report->input.continue_session();}catch(const FormatError&){stale_rejected=true;}
      need(stale_rejected,"A stale PCR owner continued the new live match");row["stale_pcr_rejected"]=true;
      row["delayed_old_world_packets_rejected"]=delayed;
      const auto before=world->match.state().tick;world->match.step(actual);host->broadcast();proxy.pump();client->receive();need(world->match.state().tick==before+1,"System Link next source world did not advance normally");
      row["passed"]=true;row["pcr_done"]=true;row["remote_followed"]=true;row["idle_frames"]=120;row["countdown_beeps"]=beeps;
      row["next_start"]=true;row["generation"]=2;row["completed_source_tick"]=source_tick;row["same_socket_and_key"]=true;
      if(extended) {
        row["stage"]="natural three-player second match";desired={};remote={};
        const auto three_step=[&]{
          std::array<Controls,1> first{desired[remote_slot]},second{desired[late_slot]};
          client->send(first);late->send(second);proxy.pump();host->receive(actual);actual[0]=desired[0];world->match.step(actual);
          host->broadcast();proxy.pump();client->receive();late->receive();
        };
        row["three_player_match"]=test::complete_objective_match_tu15(*world,h.variant,desired,three_step);
        until([&]{client->send(remote);late->send(remote);proxy.pump();host->receive(actual);host->broadcast();proxy.pump();client->receive();late->receive();
          hs->world_update(world->match.state());cs->world_update(client->state());late_session->world_update(late->state());
          return hs->state()->phase==SystemLinkPhase::postgame && cs->state()->phase==SystemLinkPhase::postgame && late_session->state()->phase==SystemLinkPhase::postgame;},"Second native System Link match did not confirm");
        hs->retain(std::move(host),{});cs->retain({},std::move(client));late_session->retain({},std::move(late));world.reset();
        cs->close();
        until([&]{proxy.pump();hs->pump();late_session->pump();return !(hs->state()->active_players&(1u<<remote_slot));},"Authenticated postgame leave did not remove actual membership");
        hs->done();until([&]{hs->pump();late_session->pump();return late_session->state()->phase==SystemLinkPhase::lobby;},"Retained sparse lobby was not delivered");
        const auto changed=std::ranges::find_if(content.variants,[](const auto& v){std::string_view n=v.name;if(n.ends_with(" \xee\x80\x88"))n.remove_suffix(4);return n=="1 Flag CTF Fast";});
        need(changed!=content.variants.end(),"Original changed-preset choice absent");
        choose(hm,1,23);choose(hm,1,207);choose(hm,entry(hm,changed->engine),208);choose(hm,entry(hm,int(changed-content.variants.begin())),14);
        const auto changed_launch=hm.selected_multiplayer_configuration();options.variant=changed_launch.variant;
        options.source_object_savegame_directory=std::filesystem::path(scratch)/("link-sparse-"+std::to_string(count));world=std::make_shared<MultiplayerPreparedWorld>(options);
        const auto changed_fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,map.parent_path(),0,world->network_starting_weapon_identity()),options.variant);
        host=hs->enter_host(world->match,port,changed_fingerprint,options.variant,map.stem().string());
        until([&]{host->receive(actual);host->broadcast();late_session->pump();return late_session->state()->phase==SystemLinkPhase::starting;},"Sparse third-world Start was not delivered");
        late=late_session->enter_client(host_address,changed_fingerprint,1,late_launch.selected_teams,late_launch.selected_profiles);
        until([&]{late->send(remote);host->receive(actual);host->broadcast();late->receive();return late->ready() && hs->state()->phase==SystemLinkPhase::running;},"Sparse third-world admission timed out");
        need(late->slots()[0]==late_slot && !world->match.player(remote_slot).connected && hs->state()->roster.size()==2,"Departed player was recreated in the next native world");
        desired={};actual={};
        const auto sparse_step=[&]{std::array<Controls,1> input{desired[late_slot]};late->send(input);host->receive(actual);actual[0]=desired[0];world->match.step(actual);host->broadcast();late->receive();};
        row["sparse_match"]=test::complete_objective_match_tu15(*world,options.variant,desired,sparse_step);row["changed_variant"]=options.variant.name;
        until([&]{late->send(remote);host->receive(actual);host->broadcast();late->receive();hs->world_update(world->match.state());late_session->world_update(late->state());
          return hs->state()->phase==SystemLinkPhase::postgame && late_session->state()->phase==SystemLinkPhase::postgame;},"Sparse native results did not confirm");
        hs->retain(std::move(host),{});late_session->retain({},std::move(late));world.reset();
        const auto sparse_report=hs->report(host_frontend);
        need(sparse_report->results->player_count==2 && hs->state()->report[0x370+0x94]==std::byte{} && hs->state()->report[0x370+late_slot*0x94]==std::byte{1},"Sparse native PCR compacted or resurrected a source slot");
        row["three_player_match_completed"]=true;row["authenticated_leave"]=true;row["sparse_source_slot"]=late_slot;row["sparse_pcr_completed"]=true;
        late_session->close();
      }
      // End this finite second match through ordinary owner destruction.
      client.reset();host.reset();world.reset();cs->close();hs->close();
    }catch(const std::exception& error){row["passed"]=false;row["error"]=error.what();++failures;std::cerr<<"FAIL "<<name<<": "<<error.what()<<'\n';}
    report["cases"].push_back(std::move(row));++count;save();
  }
  need(count>0,"System Link objective cohort selected no presets");report["passed"]=failures==0;save();return failures?1:0;
#endif
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
