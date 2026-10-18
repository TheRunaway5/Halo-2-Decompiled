#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include <bit>
#include <filesystem>
#include <iostream>

namespace {
using namespace halo2;
void need(bool yes,const char* why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F&& f,const char* why){bool no=false;try{f();}catch(const std::exception&){no=true;}need(no,why);}
LevelContent arena(){LevelContent c;c.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};c.player.datum=1;c.player.height=1;c.player.radius=.2f;c.player.crouch_height=.6f;c.player.camera_height=.8f;c.player.vitality={100,100,2,.1f,0};c.movement={2,2,2,10,1,1,1,5,2,1};return c;}
struct Program {
  ScriptProgram p;
  std::uint32_t node(ScriptNode n){const auto i=unsigned(p.nodes.size());n.salt=std::uint16_t(0xe000+i);p.nodes.push_back(n);return (unsigned(n.salt)<<16)|i;}
  std::uint32_t lit(unsigned bits,unsigned type){return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,bits});}
  std::uint32_t call(const std::string& name,unsigned op,unsigned type,std::vector<std::uint32_t> args){const auto at=unsigned(p.strings.size());p.strings+=name;p.strings+='\0';for(unsigned i=0;i<args.size();++i)p.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;const auto fn=node({0,2,2,9,args.empty()?null_datum:args[0],at,0});return node({0,std::uint16_t(op),std::uint16_t(type),8,null_datum,0,fn});}
  explicit Program(unsigned sound){std::vector<std::uint32_t> commands;for(unsigned i=0;i<3;++i){p.globals.push_back({"query"+std::to_string(i),8,lit(0,8)});const auto target=node({0,8,8,13,null_datum,0,i});std::vector<std::uint32_t> args{lit(sound,32)};if(i==1)args.push_back(lit(30,8));const auto q=call(i==2?"sound_impulse_language_time":"sound_impulse_time",0x258+i,8,args);commands.push_back(call("set",4,8,{target,q}));}p.scripts.push_back({"sample",0,4,call("begin",0,4,commands)});}
};
SourceScriptSoundClockQueriesTu15 queries(const Cache& cache,MccScriptSoundWorldAccess access,
    std::shared_ptr<SourceScriptSoundClockNative64Tu15> clock){
  SourceScriptSoundClockQueriesTu15 q;
  q.stored_time=[&cache,access,clock](auto sound){access.require_current(cache);const auto fp=access.floating_point_context(cache);const auto maximum=access.stored_milliseconds(cache,{sound});const auto matched=source_script_sound_clock_find_first_tu15(clock->rows(),sound);const auto now=matched?access.time(cache):SourceScriptSoundClockTimeTu15{};const auto returned=access.time(cache);return clock->stored_remaining(sound,maximum,now,returned,fp).value;};
  q.explicit_time=[&cache,access,clock](auto sound,auto ticks){access.require_current(cache);const auto fp=access.floating_point_context(cache);const auto matched=source_script_sound_clock_find_first_tu15(clock->rows(),sound);const auto now=matched?access.time(cache):SourceScriptSoundClockTimeTu15{};const auto game=clock->explicit_remaining(sound,ticks,now,fp);const auto returned=access.time(cache);return source_script_sound_clock_hs_explicit_tail_tu15(game.value,returned.delta_time_bits,fp);};
  q.language_time=[&cache,access,clock](auto sound){access.require_current(cache);return clock->language_remaining(sound,access.policy(cache),access.floating_point_context(cache)).value;};return q;
}
}
int main(int argc,char** argv)try{
  if(argc!=3||!std::filesystem::is_regular_file(argv[1]))return 77;
  std::filesystem::create_directories(argv[2]);const Cache cache(argv[1]);
  auto world=std::make_unique<PhysicsLiveWorldTu15>(cache,argv[2]);world->initialize_map();world->configure_session(source_game_variant_default(0),1,30,0x12345678u);
  auto access=world->script_sound_access(cache);const auto provider=world->sound_variation_provider(cache);
  std::optional<TagHandle> selected;
  for(const auto& tag:cache.tags())if(tag.group=="snd!"&&tag.address){try{const auto definition=provider.load(cache,tag.handle());if(definition.permutations.size()!=1)continue;(void)access.stored_milliseconds(cache,tag.handle());(void)access.selected_seconds(cache,tag.handle(),{0,0});selected=tag.handle();break;}catch(const FormatError&) {}}
  need(bool(selected),"No actual finite original stored/selected duration fixture");
  auto clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();const auto before=access.clock_rows(cache);
  const auto admission=clock->begin(selected->value,access.policy(cache),{},access.floating_point_context(cache));need(admission&&clock->finish_failure(*admission),"Actual source selection did not admit metadata-only timing row");
  access.publish_clock_rows(cache,before,clock->rows());need(access.clock_rows(cache)==clock->rows(),"Actual World metadata differs native64 clock");
  auto content=arena();content.scripts=Program(selected->value).p;Simulation simulation(content,0,Spawn{});
  unsigned pcm=0;simulation.cinematic_text_services([](auto){return true;},[&](auto){++pcm;return 100.f;});
  std::array<std::int32_t,3> expected;const auto fp=access.floating_point_context(cache);
  const auto now=access.time(cache),returned=access.time(cache);const auto ms=access.stored_milliseconds(cache,*selected);const auto duration=access.selected_seconds(cache,*selected,admission->selection());const auto row=clock->rows()[0];
  expected[0]=source_script_sound_stored_remaining_tu15(true,row.start_tick_bits,ms,now,returned,fp);
  expected[1]=source_script_sound_clock_hs_explicit_tail_tu15(source_script_sound_explicit_remaining_tu15(true,row.start_tick_bits,30,now,fp),returned.delta_time_bits,fp);
  expected[2]=source_script_sound_language_remaining_tu15(true,row.start_tick_bits,duration,now,fp);
  const auto saved=world->authority_checkpoint();
  {
    SourceScriptSoundClockQueryBindingTu15 bound(simulation,queries(cache,access,clock));clock.reset();
    simulation.start_level();simulation.step({});need(simulation.scripts().diagnostics().empty()&&pcm==0,"Actual World query binding fell back or faulted");
    for(unsigned i=0;i<3;++i)need(simulation.scripts().globals()[i].bits==std::bit_cast<std::uint32_t>(expected[i]),"Actual World/native64/VM query differs source numeric control");
    need(source_query_script_sound_explicit_time_tu15(simulation,0xffffffffu,30)==0&&source_query_script_sound_language_time_tu15(simulation,0xffffffffu)==0,"Actual NONE/no-match query differs");
    rejects([&]{source_query_script_sound_stored_time_tu15(simulation,0xffffffffu);},"Stored NONE invented finite original identity");
    {auto stage=world->prepare_authority(saved);need(world->commit_authority(std::move(stage)),"Actual authority restore failed");}
    rejects([&]{source_query_script_sound_language_time_tu15(simulation,selected->value);},"Stale query service touched restored World");
  }
  auto fresh=world->script_sound_access(cache);auto fresh_clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();
  {SourceScriptSoundClockQueryBindingTu15 rebound(simulation,queries(cache,fresh,fresh_clock));need(source_query_script_sound_language_time_tu15(simulation,selected->value)==0,"Fresh empty owner retained stale metadata");world.reset();rejects([&]{source_query_script_sound_explicit_time_tu15(simulation,selected->value,30);},"Retained query service touched destroyed World");}
  std::cout<<"PASS "<<cache.name()<<" actual native World + genuine selector/duration + native64 metadata + three HS/VM queries; restore/teardown guards, no PCM fallback, all source voices NONE. No Audio/Main/playthrough/timing parity claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
