#include "halo2/multiplayer_world.hpp"
#include "halo2/source_map_effects.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);}
}
int main(int argc,char **argv)try {
  if(argc!=4)throw FormatError("Supply TU15_XBE TERMINAL_MAP PRESENTATION");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
  const bool presentation=std::stoul(argv[3]);
  MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.shared_maps="assets/disc/maps";
  options.local_players=1;options.starting_weapon=5;options.mute=true;options.session_random_seed=0x78a8;
  MultiplayerPreparedWorld world(options);const auto slot=world.match.join("Original train effect");
  auto current=world.match.state();world.source_objects->set_local_player(current.players[slot].source_player,0);
  const auto biped=world.source_objects->object_definition(current.players[slot].source_unit);
  CollisionWorld collision(world.content.collision);Mesh mesh;NativeWorldEffects effects(*world.cache,mesh,collision,0x12345678);
  SourceMapEffects attachments(*world.cache,world.content.objects,effects);
  check(attachments.stats().attachments==8 && attachments.stats().prepared==8,"Terminal's original-owned damage graphs did not prepare");
  std::set<std::uint32_t> damage_effects;
  for(const auto &tag:world.cache->select_tags({.group=tag_groups::damage_effect}))
    if(tag.name.find("ze_train\\damage_effects\\cow_catcher")!=std::string::npos)damage_effects.insert(tag.datum);
  check(damage_effects.size()==2,"Actual train damage definitions are missing");
  std::uint32_t near=null_datum,far=null_datum,near_body{};bool hit{};unsigned calls{},hit_tick{};
  struct Before {SourceEffectDamageCallTu15 call;std::uint32_t body;};std::vector<Before> pending;
  world.source_objects->observe_effect_damage([&](bool before,const auto &call,const SourceRuntimeMemory &bank) {
    check(word(bank,call.request)==call.damage,"Actual original damage request changed its definition");
    if(before) {pending.push_back({call,near_body?word(bank,near_body+0xecu):0u});return;}
    check(!pending.empty() && pending.back().call.instance==call.instance && pending.back().call.request==call.request,
        "Original damage observation lost its nested caller");
    const auto previous=pending.back().body;pending.pop_back();++calls;
    if(near_body && damage_effects.contains(call.damage) && std::bit_cast<float>(previous)>0 &&
        std::bit_cast<float>(word(bank,near_body+0xecu))<std::bit_cast<float>(previous)) {
      hit=true;hit_tick=call.tick;
      check(call.owner!=null_datum && call.instance!=null_datum,"Train damage lost its actual effect/object allocation");
    }
  });
  MuzzleEffectView view{};view.width=640;view.height=480;view.forward={1,0,0};view.up={0,0,1};view.orientation={1,0,0,0,1,0,0,0,1};
  const auto retained=[&] {
    const auto &bank=world.source_objects->memory();std::vector<std::byte> bytes;
    const auto append=[&](std::uint32_t address,std::size_t size){const auto data=bank.view(address,size);bytes.insert(bytes.end(),data.begin(),data.end());};
    append(word(bank,0x4f3cd8u),8);append(word(bank,0x51df54u)+8u,4);
    append(word(bank,word(bank,0x4f720cu)+0x44u),256u*400u);
    append(word(bank,word(bank,0x4f7208u)+0x44u),512u*60u);
    if(near_body)append(near_body,0x128);
    return bytes;
  };
  const auto synchronize=[&] {
    if(!presentation)return;
    std::vector<std::vector<std::uint8_t>> permutations;std::vector<SourceSceneEffectOwner> owners;
    const auto &scene=world.match.simulation().scene();permutations.reserve(scene.objects().size());
    for(const auto &object:scene.objects())if(object.handle!=null_datum) {
      const auto type=world.content.objects.at(object.placement).type;
      if((type!=6 && type!=7 && type!=11) || !object.solved_nodes || object.solved_nodes->empty())continue;
      permutations.push_back(world.source_objects->object_model_regions(object.handle).selection.permutations);
      owners.push_back({object.placement,object.handle,object.created_tick,*object.solved_nodes,permutations.back(),object.position});
    }
    const auto saved=retained();const auto count=calls;
    attachments.synchronize(owners,current.tick,current.source_scene_roots);const auto starts=attachments.stats().spawned;
    attachments.synchronize(owners,current.tick,current.source_scene_roots);check(attachments.stats().spawned==starts,"Snapshot replay duplicated source-only bindings");
    effects.advance(current.tick,1.f/30.f);for(unsigned i=0;i<4;++i)check(effects.draw(view,0,1.f/30.f).empty(),"Pure damage graph manufactured a native particle");
    check(effects.diagnostics().effects==0 && retained()==saved && calls==count,"Presentation duplicated an original damage clock, allocation, RNG or vitality write");
  };
  std::array<Controls,multiplayer_capacity> commands{};
  for(unsigned tick=0;tick<240 && !hit;++tick) {
    current=world.match.state();synchronize();
    if(near==null_datum)for(const auto &root:current.source_scene_roots) {
      const auto tag=world.cache->query_tag({.handle=TagHandle{root.definition}});
      if(!tag || tag->name.find("ze_train")==std::string::npos)continue;
      for(const auto &effect:root.attached_effects)if(effect.instance!=null_datum && !(effect.flags&0xcu) && !effect.locations.empty()) {
        auto frame=effect.locations.front().local;const auto node=effect.locations.front().node;
        if(node!=-1){const auto nodes=world.source_objects->world_nodes(root.object);check(node>=0 && unsigned(node)<nodes.size(),"Original train location lost its WORLD node");frame=compose_transform(nodes[unsigned(node)],frame);}
        near=world.source_objects->create_biped(biped,frame.position,frame.basis[0]);
        far=world.source_objects->create_biped(biped,frame.position+Vec3{0,0,64},frame.basis[0]);
        near_body=world.source_objects->source_object_body(near);
        world.source_objects->publish_native_observer_position(0,frame.position+Vec3{0,0,1});break;
      }
      if(near!=null_datum)break;
    }
    world.match.step(commands);
  }
  current=world.match.state();synchronize();
  check(hit && near!=null_datum && pending.empty() && calls,"No completed original cow-catcher call damaged the constructed target");
  const auto near_vital=world.source_objects->object_vitality(near),far_vital=world.source_objects->object_vitality(far);
  check(near_vital.body<=0 && near_vital.shield==0 && far_vital.body==1 && far_vital.shield==1,
      "Original train radius owner did not kill the near target and preserve the far target");
  if(presentation)check(attachments.stats().spawned>=8,"Actual train effect generations did not bind");
  std::cout<<"PASS original train jpt! damage calls="<<calls<<" hit_tick="<<hit_tick<<" near_body="<<near_vital.body
      <<" far_body="<<far_vital.body<<" presentation="<<presentation<<" bindings="<<attachments.stats().spawned
      <<" near_bits="<<std::hex<<std::bit_cast<std::uint32_t>(near_vital.body)<<" far_bits="<<std::bit_cast<std::uint32_t>(far_vital.body)<<std::dec
      <<"; real constructors/location, repeated snapshots/four views, unchanged source pools/RNG/vitality; declared target placement is not player-driven traversal\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
