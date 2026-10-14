#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_map_effects.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool v,const char *s){if(!v)throw FormatError(s);}
}
int main(int argc,char **argv)try {
  if(argc!=4)throw FormatError("Supply TU15_XBE MAP EXPECTED_PREPARED");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
  MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];
  options.shared_maps=std::filesystem::path(argv[2]).parent_path().parent_path().parent_path()/"disc/maps";
  options.local_players=1;options.starting_weapon=5;options.mute=true;
  MultiplayerPreparedWorld world(options);const auto player=world.match.join("Effect acceptance");
  auto current=world.match.state();world.source_objects->set_local_player(current.players[player].source_player,0);
  CollisionWorld collision(world.content.collision);Mesh mesh;
  NativeWorldEffects effects(*world.cache,mesh,collision,0x12345678);
  SourceMapEffects attachments(*world.cache,world.content.objects,effects);
  const auto prepared=attachments.stats();
  for(const auto &gap:prepared.unavailable)std::cout<<"BOUNDARY "<<gap<<'\n';
  check(prepared.prepared==std::stoul(argv[3]),"Named map attachment resource preparation differs");
  std::uint32_t named_object=0xffffffffu;unsigned source_rows{};
  std::set<std::uint32_t> named_definitions;std::size_t named_peak{};
  for(const auto &root:current.source_scene_roots) {
    const auto type=world.content.objects.at(root.placement).type;
    if(type!=6 && type!=7 && type!=11)continue;
    const auto metadata=world.cache->load_source_tag_metadata(root.definition);
    if(metadata.size()<0x9c)continue;
    const Bytes def(metadata.bytes_at(metadata.source_address(),0x9c));
    if(!def.u32(0x94))continue;
    const Bytes rows(world.cache->read_tag_bytes(def.u32(0x98),std::size_t(def.u32(0x94))*24));
    for(const auto &effect:root.attached_effects)if(rows.u32(effect.slot*24+16) || rows.u32(effect.slot*24+20)) {
      ++source_rows;if(named_object==0xffffffffu)named_object=root.object;
      named_definitions.insert(rows.u32(effect.slot*24+4));
    }
  }
  check(named_object!=0xffffffffu && source_rows,"Actual map has no completed named attachment rows");
  MuzzleEffectView view{};view.width=640;view.height=480;view.forward={1,0,0};view.up={0,0,1};view.orientation={1,0,0,0,1,0,0,0,1};
  std::array<Controls,multiplayer_capacity> commands{};
  const auto synchronize=[&] {
    current=world.match.state();std::vector<std::vector<std::uint8_t>> permutations;
    permutations.reserve(world.match.simulation().scene().objects().size());
    std::vector<SourceSceneEffectOwner> owners;
    for(const auto &object:world.match.simulation().scene().objects())if(object.handle!=null_datum) {
      const auto type=world.content.objects.at(object.placement).type;
      if(type!=6 && type!=7 && type!=11)continue;
      if(!object.solved_nodes || object.solved_nodes->empty())continue;
      permutations.push_back(world.source_objects->object_model_regions(object.handle).selection.permutations);
      owners.push_back({object.placement,object.handle,object.created_tick,*object.solved_nodes,permutations.back(),object.position});
    }
    attachments.synchronize(owners,current.tick,current.source_scene_roots);const auto births=attachments.stats().spawned;
    attachments.synchronize(owners,current.tick,current.source_scene_roots);
    check(attachments.stats().spawned==births,"Repeated source snapshot replayed map effect births");
    effects.advance(current.tick,1.f/30.f);
    const auto a=effects.draw(view,0,1.f/30.f),b=effects.draw(view,0,1.f/30.f);
    named_peak=std::max(named_peak,std::size_t(std::count_if(a.begin(),a.end(),
        [&](const auto &draw){return named_definitions.contains(draw.effect_definition);} )));
    check(a.size()==b.size(),"Second view changed source-controlled particle simulation");
    for(unsigned n=0;n<a.size();++n)check(a[n].vertex.attributes==b[n].vertex.attributes,"Second view changed source-controlled particle values");
    (void)effects.take_sounds();
  };
  const auto step=[&] {
    const auto root=world.source_objects->object_transform(named_object);
    world.source_objects->publish_native_observer_position(0,root.position+Vec3{0,0,1});
    world.match.step(commands);synchronize();
  };
  synchronize();std::size_t peak{};
  for(unsigned i=0;i<30;++i){step();peak=std::max(peak,effects.diagnostics().particles);}
  check(attachments.stats().spawned>=source_rows,"Completed original instances did not bind their native lifetimes");
  if(std::filesystem::path(argv[2]).stem()=="turf") {
    auto vitality=world.source_objects->object_vitality(named_object);
    vitality.feedback.body={.75f,.75f,30};
    world.source_objects->deliver_vitality(named_object,vitality);
    bool active{};
    for(unsigned i=0;i<45;++i) {
      step();peak=std::max(peak,effects.diagnostics().particles);
      for(const auto &root:current.source_scene_roots)if(root.object==named_object)
        for(const auto &effect:root.attached_effects)active=active || (effect.instance!=null_datum && !(effect.flags&0xcu) && effect.scale_a>0);
    }
    check(active,"Original current_body_damage did not activate its authored trash effect");
    check(peak>0,"Admitted source damage function produced no authored particles");
    check(named_peak>0,"Admitted source damage function produced no named attachment draws");
    vitality=world.source_objects->object_vitality(named_object);vitality.feedback.body={};
    world.source_objects->deliver_vitality(named_object,vitality);
    for(unsigned i=0;i<900 && effects.diagnostics().particles;++i)step();
    for(unsigned i=0;i<30;++i){step();check(effects.diagnostics().particles==0,"Disabled source function continued particle emission");}
  }
  std::cout<<"PASS "<<std::filesystem::path(argv[2]).stem().string()<<" prepared="<<prepared.prepared
      <<" actual_named_rows="<<source_rows<<" spawned="<<attachments.stats().spawned<<" peak_particles="<<peak<<" named_peak_draws="<<named_peak
      <<"; real source owner/function/visibility, native damage-delivery stimulus, repeated views and snapshot replay\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
