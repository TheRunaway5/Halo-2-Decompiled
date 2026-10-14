#include "halo2/multiplayer_world.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/native_player_profile.hpp"
#include <bit>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <set>
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_model_palette.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc<5 || argc>7)throw FormatError("Supply MAP SHARED TU15 OUTPUT_DIRECTORY [human|elite] [STARTING_WEAPON]");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  if(argc>=6 && std::string_view(argv[5])!="human" && std::string_view(argv[5])!="elite")
    throw FormatError("Invalid player species");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  options.starting_weapon=argc==7?std::stoul(argv[6]):5;options.session_random_seed=0x12345678;options.defer_initial_player_spawn=true;
  const bool elite=argc>=6 && std::string_view(argv[5])=="elite";
  std::array<std::uint8_t,9> appearance{};appearance[4]=elite?1:0;
  options.selected_profiles={native_selected_player_configuration("Source animation",appearance,-1)};
  const unsigned representation=elite?3:2;
  if(multiplayer_representation(options.selected_profiles[0])!=representation)
    throw FormatError("Native profile did not select the requested source representation");
  Cache placement_cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto content=placement_cache.load_content(0,true);CollisionWorld geometry(content.collision);
  bool placed=false;Spawn spawn{};
  for(const auto &candidate:placement_cache.spawns()) {
    for(unsigned direction=0;direction<4 && !placed;++direction) {
      const float heading=float(direction)*1.5707963267948966f;const Vec3 d{std::cos(heading),std::sin(heading),0};
      const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
      if(geometry.raycast(eye,d,3.f) || geometry.raycast(eye,d*-1.f,3.f))continue;
      bool floor=true;
      for(float distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
        const auto hit=geometry.raycast(candidate.position+d*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
        floor&=hit && hit->normal.z>.9f;
      }
      if(floor){spawn=candidate;spawn.facing=heading;placed=true;}
    }
    if(placed)break;
  }
  if(!placed)throw FormatError("No clear authored locomotion corridor/floor");
  options.native_spawn_inputs={spawn};
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
  options.pre_player_update_owner=[&](MultiplayerMatch &match) {
    for(const auto handle:players){const auto *p=match.player_from_datum(handle);
      if(p && !(p->source_flags&1u))match.apply_source_player_activation(handle,p->source_flags,p->source_flags|1u);}
  };
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  if(!world.source_objects || !world.source_objects->owns_physics())throw FormatError("Actual source physics owner missing");
  std::array<Controls,multiplayer_capacity> controls{};float yaw=spawn.facing;
  for(unsigned tick=0;tick<100;++tick) {controls[0].yaw=yaw;match.step(controls);if(!tick)yaw=match.unit(0).yaw;}
  const auto unit=match.unit(0).source_datum;
  nlohmann::json report;
  world.source_objects->begin_model_frame();
  world.source_objects->render_model_view([&](PhysicsLiveWorldTu15& source) {
    const auto& bank=source.memory();
    const auto word=[&](std::uint32_t p){return Bytes(bank.view(p,4)).u32(0);};
    const auto definition=world.source_objects->object_definition(unit);
    const auto model=world.cache->load_model(definition,false);
    const auto nodes=world.source_objects->world_nodes(unit);
    const auto tag_table=word(0x4f0414u);
    const auto meta=[&](std::uint32_t t){return word(tag_table+(t&65535u)*16u+8u);};
    const auto mode=word(meta(word(meta(definition)+0x38u))+4u);
    const auto m=meta(mode),count=word(m+0x48u),records=word(m+0x4cu);
    std::vector<Transform> inverses(count);
    for(unsigned n=0;n<count;++n)std::memcpy(&inverses[n],bank.view(records+n*96u+0x28u,52).data(),52);
    auto physical=source_model_skin_palette(nodes,inverses);
    const auto collector=source.build_world_model(unit,0,1,false,0,0);
    const auto completed=source.rendered_world_nodes(unit);
    if(!completed || completed->size()!=nodes.size())throw FormatError("Original rendered body node handoff missing");
    auto completed_palette=source_model_skin_palette(*completed,inverses);
    const Bytes c(collector.bytes);const auto token=c.u32(4);
    const auto lod=c.u16(0x168u);
    if(lod>=6)throw FormatError("Original rendered body selected an invalid geometry LOD");
    const auto palette_definition=world.cache->load_source_model_palette_definition(mode);
    append_source_model_compounds(physical,palette_definition.compounds[lod]);
    append_source_model_compounds(completed_palette,palette_definition.compounds[lod]);
    if(token==null_datum)throw FormatError("Original render builder omitted biped palette");
    const auto output=0x49433cu+(token&0x0fffffffu);
    const auto rendered_count=Bytes(bank.view(output,2)).u16(0);
    nlohmann::json changed=nlohmann::json::array();
    const auto regions=Bytes(bank.view(output+2,2)).u16(0);
    for(unsigned region=0;region<regions;++region) {
      const auto selected=std::to_integer<unsigned>(c.slice(0x75u+region,1).front());
      if(selected==255)continue;
      const auto section=word(m+0x28u)+selected*92u,resident=word(section+0x34u);
      const auto maps=word(resident+0x64u),mapping=word(resident+0x68u);
      const Bytes range(bank.view(output+4u+region*4u,4));const auto first=range.u16(0),n=range.u16(2);
      for(unsigned i=0;i<n;++i) {
        const auto node=maps?std::to_integer<unsigned>(bank.view(mapping+i,1)[0]):i;
        if(node>=physical.size())throw FormatError("Original body node map escapes its complete palette");
        std::array<float,12> actual;std::memcpy(actual.data(),bank.view(output+0x44u+(first+i)*48u,48).data(),48);
        if(std::memcmp(actual.data(),completed_palette[node].data(),48))
          throw FormatError("Retained render pose differs from the original uploaded palette");
        if(std::memcmp(actual.data(),physical[node].data(),48))changed.push_back({{"region",region},{"index",node},
          {"name",node<model.bones.size()?world.cache->string_id(model.bones[node].name):"compound"},
          {"physical",physical[node]},{"rendered",actual}});
      }
    }
    report={{"unit",unit},{"weapon",match.unit(0).weapon.definition},{"node_count",count},{"rendered_count",rendered_count},{"changed",changed},{"scope","Original render-builder palette versus the same current physics WORLD-node palette; not a framebuffer comparison"}};
  });
  world.source_objects->begin_model_frame();
  if(world.source_objects->rendered_world_nodes(unit))throw FormatError("Prior frame's rendered pose leaked into a new frame");
  std::filesystem::create_directories(argv[4]);std::ofstream(std::filesystem::path(argv[4])/"nodes.json")<<report.dump(2)<<'\n';
  std::cout<<"Original deferred render changes "<<report["changed"].size()<<" of "<<report["node_count"]<<" nodes\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
