#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/damage_amount.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <future>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid or active native object continuation accepted");}
struct Directory {
  std::filesystem::path path;
  Directory(){auto name=(std::filesystem::temp_directory_path()/"halo2-object-continuation-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create native files directory");path=name;}
  ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
  std::uint32_t time{};
  mutable std::atomic<std::uint64_t> counter{};
  std::uint32_t milliseconds()const noexcept override{return time;}
  std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
  std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
void same_report(const MultiplayerObjectRuntime::RetainedDeathReport &a,
                 const MultiplayerObjectRuntime::RetainedDeathReport &b) {
  check(a.damage_class==b.damage_class && a.force==b.force &&
      std::bit_cast<std::uint32_t>(a.direction.x)==std::bit_cast<std::uint32_t>(b.direction.x) &&
      std::bit_cast<std::uint32_t>(a.direction.y)==std::bit_cast<std::uint32_t>(b.direction.y) &&
      std::bit_cast<std::uint32_t>(a.direction.z)==std::bit_cast<std::uint32_t>(b.direction.z),"Retained native fatal report differs");
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;check(argc==3,"Supply owned TU1.5 XBE and Xbox map");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  Cache cache(argv[2],{},OriginalData::load_xbe(argv[1]));const auto content=cache.load_content(0,true);
  Directory source_files,target_files;
  auto source_clock=std::make_shared<Clock>(),target_clock=std::make_shared<Clock>();
  source_clock->time=1000;source_clock->counter=100000;target_clock->time=900000;target_clock->counter=90000000;
  MultiplayerObjectRuntime source(argv[1],cache,0,source_files.path,source_clock);
  MultiplayerObjectRuntime target(argv[1],cache,0,target_files.path,target_clock);
  rejects([&]{source.authority_checkpoint();});
  auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
  source.configure_session(variant,1,30,0x12345678);target.configure_session(variant,1,30,0x12345678);
  const auto object=source.create_biped(content.player.datum,cache.spawns().front().position,{1,0,0});
  check(object!=null_datum,"Actual source biped birth failed");
  NativeObjectLifetimes source_objects(source.memory(),source.object_pool()),target_objects(target.memory(),target.object_pool());
  source_objects.observe_source_birth(object);
  DatumPool players(source.memory(),source.player_pool()),imported(target.memory(),target.player_pool());
  const auto player=players.allocate_at(0);check(player!=null_datum,"Actual source player allocation failed");
  source.construct_player(player,0,source_default_player_configuration());source.bind_player(player,object);
  source.initialize_player_equipment(player,object);
  std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
  for(unsigned tick=0;tick<10;++tick){source_clock->time+=33;source.deliver_command(player,object,command);source.step_objects();}
  rejects([&]{source.source_object_body(object^0x10000u);}); // Rejected read does not fault authority.
  std::async(std::launch::async,[&]{rejects([&]{source.step_objects();});rejects([&]{source.authority_checkpoint();});}).get();
  // This source write occurs before a nested physics entry. The outer bridge
  // must reject capture of that multi-call public operation, not just a VM call.
  bool in_guard=false,guard_checked=false;
  int guard_identity=0;
  auto &source_bank=source.memory();auto &target_bank=target.memory();
  source_runtime_register_write_guard(source_bank,&guard_identity,[&](auto,auto){
    if(in_guard)return;in_guard=true;rejects([&]{source.authority_checkpoint();});guard_checked=true;in_guard=false;
  });
  try {source.deliver_vitality(object,source.object_vitality(object));}
  catch(...) {source_runtime_unregister_write_guard(source_bank,&guard_identity);throw;}
  source_runtime_unregister_write_guard(source_bank,&guard_identity);
  check(guard_checked,"Outer native object write guard was not exercised");
  auto saved=source.authority_checkpoint();const auto saved_objects=source_objects.source_authority_checkpoint();
  const auto saved_players=players.source_binding();const auto before=target_bank.checkpoint();
  auto services=target.observer_world_services();
  {
    auto prepared=target.prepare_authority(saved);
    auto objects=target_objects.prepare_source_authority(saved_objects,prepared.replacement_memory());
    check(prepared.for_owner(target) && !source.commit_authority(std::move(prepared)),"Foreign bridge accepted prepared authority");
    rejects([&]{target.authority_checkpoint();});rejects([&]{target.memory();});
    rejects([&]{target.prepare_authority(saved);});rejects([&]{services.locate({0,0,0});});
    check(target_bank.checkpoint()==before,"Staging changed current native bridge bank");
  }
  check(target_bank.checkpoint()==before,"Cancelled native bridge import changed backing");
  auto bad=saved;bad.version=2;rejects([&]{target.prepare_authority(bad);});
  bad=saved;bad.death_reports.emplace(object,MultiplayerObjectRuntime::RetainedDeathReport{});
  rejects([&]{target.prepare_authority(bad);}); // A live unit has no actual fatal transition/report.
  auto publish=[&](const auto &checkpoint) {
    auto prepared=target.prepare_authority(checkpoint);
    auto objects=target_objects.prepare_source_authority(source_objects.source_authority_checkpoint(),prepared.replacement_memory());
    auto player_view=imported.prepare_source_view(players.source_binding(),prepared.replacement_memory());
    check(target.commit_authority(std::move(prepared)) && target_objects.commit_source_authority(std::move(objects)) &&
        imported.commit_source_view(std::move(player_view)),"Native bridge/pool authority publication failed");
    check(target_bank.checkpoint()==source_bank.checkpoint(),"Published native bridge bank differs");
  };
  publish(saved);
  for(unsigned tick=0;tick<30;++tick) {
    source_clock->time+=33;target_clock->time+=33;
    source.deliver_command(player,object,command);source.step_objects();
    target.deliver_command(player,object,command);target.step_objects();
    check(target_bank.checkpoint()==source_bank.checkpoint(),"Future complete source object ticks diverged");
  }
  const auto source_location=source.observer_world_services().locate(cache.spawns().front().position);
  const auto target_location=services.locate(cache.spawns().front().position);
  check(source_location.cluster()==target_location.cluster() && source_bank.checkpoint()==target_bank.checkpoint(),
      "Retained target observer closure did not rebind to imported authority");
  // Real tag definitions and completed native vitality arithmetic supply this
  // admitted fatal stimulus; the original request/report/reaction graph must
  // produce the retained report before its ordinary cleanup consumes it.
  const auto weapon=std::ranges::find_if(content.weapons,[](const auto &entry){return entry.second.smg.has_value();});
  check(weapon!=content.weapons.end() && weapon->second.damage_amount &&
      weapon->second.damage_amount->upper_minimum==weapon->second.damage_amount->upper_maximum,"Deterministic authored SMG damage unavailable");
  const auto initial=source.object_vitality(object);RechargeState vitality;vitality.body=initial.body;vitality.shield=initial.shield;
  auto feedback=initial.feedback;UnitDamageResult last;
  const auto amount=damage_amount(weapon->second.damage_amount->lower,weapon->second.damage_amount->upper_minimum,1.f,1.f,1.f);
  for(unsigned tick=0;tick<240 && !vitality.dead;++tick) {
    last=apply_unit_damage(vitality,content.player.vitality,weapon->second,content.damage_groups,content.damage_materials,amount);
    report_damage_feedback(feedback,last.body,last.shield,last.report_flags);
  }
  check(vitality.dead && (last.report_flags&1u),"Actual native damage did not produce fatal admission");
  source.deliver_vitality(object,{vitality.body,vitality.shield,feedback});
  source.deliver_direct_impact(object,weapon->second.damage,{},last,{1,0,0},cache.spawns().front().position);
  const auto fatal=source.authority_checkpoint();check(fatal.death_reports.size()==1 && fatal.death_reports.contains(object),"Actual native fatal report was not captured");
  std::cout<<"Actual source damage graph produced a pending fatal cleanup report\n";
  bad=fatal;bad.death_reports.begin()->second.force=2;rejects([&]{target.prepare_authority(bad);});
  bad=fatal;bad.death_reports.begin()->second.direction.x=std::numeric_limits<float>::infinity();rejects([&]{target.prepare_authority(bad);});
  bad=fatal;const auto report=bad.death_reports.begin()->second;bad.death_reports.clear();bad.death_reports.emplace(object^0x10000u,report);
  rejects([&]{target.prepare_authority(bad);});
  publish(fatal);same_report(source.authority_checkpoint().death_reports.at(object),target.authority_checkpoint().death_reports.at(object));
  // Explicit original retirement can supersede pending cleanup. Its native
  // sidecar must disappear with that full identity and never follow slot reuse.
  target.destroy_object(object);target_objects.observe_source_retirement(object);
  check(target.authority_checkpoint().death_reports.empty(),"Original retirement left a stale native damage report");
  publish(fatal); // Restore the admitted pre-cleanup frontier before comparing consumption.
  source.kill_biped(object);target.kill_biped(object);
  check(source.authority_checkpoint().death_reports.empty() && target.authority_checkpoint().death_reports.empty() &&
      source_bank.checkpoint()==target_bank.checkpoint(),"Imported pending report changed actual fatal cleanup or was replayed");
  for(unsigned tick=0;tick<20;++tick) {
    source_clock->time+=33;target_clock->time+=33;source.step_objects();target.step_objects();
    check(source_bank.checkpoint()==target_bank.checkpoint(),"Post-cleanup source world continuation diverged");
  }
  const auto again=source.authority_checkpoint();
  rejects([&]{target.call(0x12345678u);});rejects([&]{target.authority_checkpoint();});
  publish(again);check(imported.lookup(player).size()==0x21c,"Recovery lost retained source player allocation");
  std::cout<<"PASS composed native object bridge, pending actual damage report/cleanup, outer-call barrier, local services, rollback/recovery and fifty future source ticks\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
