#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/unit_damage_audio.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <nlohmann/json.hpp>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <future>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-remote-death-XXXXXX").string();
    const auto p=::mkdtemp(name.data());check(p,"Create remote death test directory");path=p;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
// Read114EC0's authored dialogue namespace independently of the playback
// observer. No cue, request, chance result or PCM admission is supplied.
std::array<std::uint32_t,12> authored_damage_sounds(const Cache &cache,std::uint32_t biped) {
  const auto root=[](const SourceTagMetadata &tag){return Bytes(tag.bytes_at(tag.source_address(),tag.size()));};
  const auto block=[](const SourceTagMetadata &tag,const Bytes &b,unsigned at,unsigned stride) {
    const auto d=read_tag_block_descriptor(b,at);
    return Bytes(d.count?tag.bytes_at(d.address,d.extent(stride,65536)):std::span<const std::byte>{});
  };
  std::array<std::uint32_t,12> out;out.fill(null_datum);
  const auto unit=cache.load_source_tag_metadata(biped);const auto u=root(unit);
  const auto model=cache.load_source_tag_metadata(u.u32(0x38));const auto m=root(model);
  auto dialogue=m.u32(0x90);const auto variants=block(model,m,0x50,0x38);
  for(unsigned n=0;n<m.u32(0x50);++n) {
    const auto at=n*0x38,candidate=variants.u32(at+0x34);
    if(candidate!=null_datum && (variants.u32(at)==0 || dialogue==null_datum))dialogue=candidate;
    if(candidate!=null_datum && variants.u32(at)==0)break;
  }
  if(dialogue==null_datum)return out;
  const auto voice=cache.load_source_tag_metadata(dialogue);const auto v=root(voice);
  const auto sounds=block(voice,v,0xc,16);
  const auto globals=cache.query_tag({.group=TagGroup::from_string("matg"),.require_address=true});
  check(bool(globals),"Damage playback witness lost actual globals");
  const auto global=cache.load_source_tag_metadata(globals->datum);const auto g=root(global);
  check(g.u32(0xc8)>0,"Damage playback witness lost authored dialogue selectors");
  const auto ai=block(global,g,0xc8,0x88);
  const auto definition=cache.load_source_tag_metadata(ai.u32(0x64));const auto d=root(definition);
  const auto lookup=block(definition,d,0x24,4);
  for(unsigned n=0;n<out.size() && n<d.u32(0x24);++n) {
    const auto index=std::int16_t(lookup.u16(n*4));
    if(index>=0 && unsigned(index)<v.u32(0xc) && sounds.u32(unsigned(index)*16+8)==TagGroup{"snd!"}.value)
      out[n]=sounds.u32(unsigned(index)*16+12);
  }
  return out;
}
struct DamageAudioEvidence {
  std::map<std::uint64_t,UnitDamageVoiceEvent> requests;
  std::vector<MultiplayerAudioSubmission> packets;
  std::uint32_t biped=null_datum;
  void capture(const MultiplayerState &state,std::uint32_t victim,const LevelContent &content) {
    if(victim==null_datum)return;
    for(const auto &request:state.unit_damage_voices)if(request.target==victim)requests.emplace(request.serial,request);
    const auto &player=state.players[1];
    if(player.source_unit==victim || player.source_previous_unit==victim)
      biped=multiplayer_biped(content,source_session_player_representation(player.source_configuration,state.source_game_type)).datum;
  }
  void packet(const MultiplayerAudioSubmission &value) {
    if(value.kind!=MultiplayerAudioKind::DamageVoice)return;
    check(packets.size()<256,"Damage playback witness exceeded its bounded cycle");packets.push_back(value);
  }
  void finish(const Cache &cache,std::uint32_t victim)const {
    check(biped!=null_datum && !requests.empty(),"Actual UDP damage journal omitted the fatal victim");
    const auto sounds=authored_damage_sounds(cache,biped);
    unsigned hurt_requests{},lethal_requests{};bool authored_lethal{};
    std::set<std::uint64_t> hurt_packets,lethal_packets;
    for(const auto &[serial,request]:requests) {
      (void)serial;
      if(request.report_flags&1u)++lethal_requests;else ++hurt_requests;
      std::optional<unsigned> lethal;
      if(request.report_flags&1u) {
        int type{};bool special{};
        if(request.definition!=null_datum) {
          const auto damage=cache.load_source_tag_metadata(request.definition);
          const Bytes fields(damage.bytes_at(damage.source_address(),damage.size()));
          type=std::int16_t(fields.u16(0x12));special=(fields.u32(0x14)&0x80u)!=0;
        }
        DamageVoiceState unused;std::uint32_t untouched_random{};
        // The hard branch is deterministic and consumes no chance draw.
        // The actual playback owner still decides admission and submits PCM.
        lethal=select_player_damage_voice(unused,request,type,special,0,0,untouched_random);
        check(untouched_random==0,"Lethal witness unexpectedly consumed a chance draw");
        authored_lethal|=lethal && sounds.at(*lethal)!=null_datum;
      }
      for(const auto &packet:packets)if(packet.owner==victim) {
        check(packet.voice && packet.frames && packet.position && packet.sound!=null_datum,
            "Damage voice did not submit an actual spatial authored PCM packet");
        check(bool(cache.query_tag({.handle=TagHandle{packet.sound},.group=TagGroup{"snd!"},.require_address=true})),
            "Damage PCM packet lost its exact authored sound identity");
        if(length(*packet.position-request.position)>.0001f)continue;
        if(lethal && sounds.at(*lethal)==packet.sound)lethal_packets.insert(packet.voice);
        if(!(request.report_flags&1u) && std::find(sounds.begin()+7,sounds.end(),packet.sound)!=sounds.end())
          hurt_packets.insert(packet.voice);
      }
    }
    std::cout<<"UDP damage audio: victim="<<victim<<" hurt_requests="<<hurt_requests
        <<" lethal_requests="<<lethal_requests<<" hurt_packets="<<hurt_packets.size()
        <<" lethal_packets="<<lethal_packets.size()<<'\n';
    check(lethal_requests && authored_lethal && !lethal_packets.empty(),
        "Actual native fatal request did not deliver its authored lethal voice to joined-client PCM");
    std::cout<<"LIMIT authored NONE/chance-suppressed hurt cues remain silent; physical speaker output is separate\n";
  }
};
// Observe received WORLD endpoints and the joined corpse branch's final
// nodes/selected vertices. The same evidence can follow future skin backends;
// backend-specific verification and displayed frame pacing remain separate.
struct CorpsePoseRate {
  std::uint32_t tick{},owner=null_datum,model=null_datum;
  unsigned root{};
  bool initialized{},moving{},articulating{};
  std::vector<Transform> previous,current;
  unsigned interior_samples{},geometry_pairs{},articulated_completed_pairs{},articulated_interior_samples{},articulated_geometry_pairs{};
  unsigned geometry_same_tick{},geometry_same_layout{},geometry_separated{},geometry_layout_changes{};
  unsigned cross_tick_geometry_changes{},cross_tick_geometry_held{},same_tick_endpoint_resets{},endpoint_gap_resets{};
  unsigned verified_geometry_samples{},fractional_geometry_samples{},articulated_fractional_geometry_samples{};
  float maximum_skin_error{},maximum_fractional_geometry_departure{},maximum_relative_geometry_departure{};
  float maximum_same_tick_fraction_separation{};
  std::map<std::uint32_t,unsigned> interior_by_tick;
  nlohmann::json geometry_examples=nlohmann::json::array();
  float maximum_relative_change{};
  struct Model {std::uint32_t datum;unsigned root,count;std::shared_ptr<const RenderModel> geometry;};
  std::shared_ptr<const RenderModel> authored_geometry;
  std::map<std::uint32_t,Model> models;
  struct Received {std::uint32_t owner,model;std::vector<Transform> relative;};
  std::map<std::uint32_t,Received> received;
  struct Geometry {
    std::uint32_t tick{};float fraction{};
    std::vector<Vec3> positions;
    std::vector<std::array<std::uint32_t,3>> identities;
    std::vector<Transform> relative;
  };
  std::optional<Geometry> geometry;
  static float difference(const Transform &a,const Transform &b) {
    return std::max({std::abs(a.scale-b.scale),length(a.position-b.position),
        length(a.basis[0]-b.basis[0]),length(a.basis[1]-b.basis[1]),length(a.basis[2]-b.basis[2])});
  }
  static bool different(std::span<const Transform> a,std::span<const Transform> b,float tolerance=0) {
    if(a.size()!=b.size())return true;
    for(unsigned n=0;n<a.size();++n)if(difference(a[n],b[n])>tolerance)return true;
    return false;
  }
  std::vector<Transform> relative(std::span<const Transform> nodes)const {
    check(root<nodes.size(),"Corpse articulation lost its authored render root");
    const auto inverse=inverse_transform(nodes[root]);std::vector<Transform> result;result.reserve(nodes.size()-1);
    for(unsigned n=0;n<nodes.size();++n)if(n!=root)result.push_back(compose_transform(inverse,nodes[n]));
    return result;
  }
  void completed(std::uint32_t completed_tick,std::uint32_t object,Model selected,std::span<const Transform> nodes) {
    if(initialized && completed_tick==tick && different(current,nodes)) {
      same_tick_endpoint_resets+=geometry.has_value();geometry.reset();
    }
    if(!initialized || object!=owner || selected.datum!=model || completed_tick<tick || completed_tick-tick>1) {
      endpoint_gap_resets+=geometry.has_value();
      previous.clear();current.clear();geometry.reset();
    } else if(completed_tick!=tick)previous=std::move(current);
    const bool new_tick=!initialized || completed_tick!=tick;
    tick=completed_tick;owner=object;model=selected.datum;root=selected.root;initialized=true;
    authored_geometry=selected.geometry;
    check(nodes.size()==selected.count,"Received corpse WORLD hierarchy differs from its actual render model");
    current.assign(nodes.begin(),nodes.end());moving=articulating=false;
    if(received.size()<128 || received.contains(tick))received.insert_or_assign(tick,Received{owner,model,relative(current)});
    if(previous.size()==current.size()) {
      moving=different(previous,current,.0001f);
      const auto before=relative(previous),after=relative(current);
      for(unsigned n=0;n<before.size();++n)maximum_relative_change=std::max(maximum_relative_change,difference(before[n],after[n]));
      articulating=different(before,after,.0001f);
      if(new_tick && articulating)++articulated_completed_pairs;
    }
  }
  void capture(const Cache &cache,const MultiplayerState &state,std::uint32_t victim) {
    const auto corpse=std::find_if(state.corpses.begin(),state.corpses.end(),[&](const auto &c){return c.object==victim;});
    if(corpse==state.corpses.end())return;
    const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &p){return p.object==victim;});
    check(pose!=state.source_body_poses.end(),"Joined corpse omitted its completed WORLD endpoint");
    auto selected=models.find(corpse->definition);
    if(selected==models.end()) {
      const auto biped=cache.load_source_tag_metadata(corpse->definition);
      const auto model=cache.load_source_tag_metadata(Bytes(biped.bytes_at(biped.source_address(),biped.size())).u32(0x38));
      const auto datum=Bytes(model.bytes_at(model.source_address(),model.size())).u32(4);
      const auto render=cache.load_source_tag_metadata(datum);const Bytes fields(render.bytes_at(render.source_address(),render.size()));
      const auto block=read_tag_block_descriptor(fields,0x48);check(block.count>0,"Corpse render skeleton has no authored nodes");
      const Bytes bones(render.bytes_at(block.address,block.extent(0x60,256)));
      std::optional<unsigned> root;
      for(unsigned n=0;n<unsigned(block.count);++n)if(std::int16_t(bones.u16(n*0x60+4))<0) {
        check(!root,"Corpse render skeleton has multiple roots");root=n;
      }
      check(root.has_value(),"Corpse render skeleton has no root");
      // The runtime's corpse Visual uses the same object/variant0 decoder.
      // Omitting textures changes material resources, not geometry or skin.
      auto geometry=std::make_shared<RenderModel>(cache.load_model(corpse->definition,false,0));
      check(geometry->bones.size()==unsigned(block.count),"Corpse reference model changed its authored hierarchy");
      selected=models.emplace(corpse->definition,Model{datum,*root,unsigned(block.count),std::move(geometry)}).first;
    }
    completed(state.tick,victim,selected->second,pose->nodes);
  }
  void draw(const AnimationRenderObservation &sample) {
    const auto fraction=sample.fraction;
    if(!initialized || sample.tick!=tick || sample.unit!=owner || !moving || fraction<=.05f || fraction>=.95f)return;
    check(sample.body_nodes.size()==current.size(),"Fractional corpse render changed its hierarchy");
    check(different(sample.body_nodes,current) && different(sample.body_nodes,previous),
        "Moving corpse WORLD render palette held a completed endpoint at an interior fraction");
    ++interior_samples;
    ++interior_by_tick[tick];
    const auto posed=relative(sample.body_nodes);
    if(articulating) {
      check(different(posed,relative(previous),.000001f) && different(posed,relative(current),.000001f),
          "Actual articulating WORLD endpoints were held in the rendered root-relative pose");
      ++articulated_interior_samples;
    }
    const auto vertices=sample.vertices;
    if(vertices.empty())return;
    verify_geometry(sample);
    std::vector<std::array<std::uint32_t,3>> identities;identities.reserve(vertices.size());
    for(const auto &vertex:vertices)identities.push_back({vertex.source_section,vertex.source_part,vertex.material});
    const bool same_layout=geometry && geometry->identities==identities;
    bool changed{};
    if(same_layout)for(unsigned n=0;n<vertices.size();++n)
      changed|=length(vertices[n].position-geometry->positions[n])>.00001f;
    if(geometry) {
      geometry_same_tick+=geometry->tick==tick;geometry_same_layout+=same_layout;
      geometry_layout_changes+=!same_layout;
      if(geometry->tick==tick && same_layout) {
        const auto separation=std::abs(geometry->fraction-fraction);
        maximum_same_tick_fraction_separation=std::max(maximum_same_tick_fraction_separation,separation);
        geometry_separated+=separation>.05f;
      }
      if(geometry->tick!=tick && same_layout) {
        cross_tick_geometry_changes+=changed;cross_tick_geometry_held+=!changed;
      }
    }
    if(geometry_examples.size()<16)geometry_examples.push_back({{"tick",tick},{"fraction",fraction},
        {"vertices",vertices.size()},{"previous_tick",geometry?geometry->tick:0u},
        {"previous_fraction",geometry?geometry->fraction:0.f},{"same_layout",same_layout},
        {"vertices_changed",changed},{"articulating",articulating}});
    if(geometry && geometry->tick==tick && geometry->identities==identities &&
        std::abs(geometry->fraction-fraction)>.05f) {
      check(changed,"Submitted corpse geometry held despite changing fractional WORLD palette");
      ++geometry_pairs;
      if(articulating) {
        check(different(posed,geometry->relative,.000001f),
            "Rendered relative articulation held across qualified fractions of actual moving WORLD endpoints");
        ++articulated_geometry_pairs;
      }
    }
    geometry=Geometry{tick,fraction,{},std::move(identities),posed};geometry->positions.reserve(vertices.size());
    for(const auto &vertex:vertices)geometry->positions.push_back(vertex.position);
  }
  void verify_geometry(const AnimationRenderObservation &sample) {
    check(bool(authored_geometry),"Corpse geometry lost its actual authored model owner");
    // These are already WORLD nodes. The original inverse-bind skin includes
    // the object root; neither this reference nor ModelObjectDraw adds it again.
    const auto expected=skin_model_world_nodes(*authored_geometry,sample.body_nodes);
    const auto before=skin_model_world_nodes(*authored_geometry,previous);
    const auto after=skin_model_world_nodes(*authored_geometry,current);
    check(expected.size()==before.size() && expected.size()==after.size(),"Corpse reference skin changed its source topology");
    const auto inverse=inverse_transform(sample.body_nodes[root]);
    const auto previous_inverse=inverse_transform(previous[root]);
    const auto current_inverse=inverse_transform(current[root]);
    const auto relative_point=[](const Transform &inverse,Vec3 position) {
      Transform point;point.position=position;return compose_transform(inverse,point).position;
    };
    std::size_t cursor{};
    float departure_before{},departure_after{},relative_before{},relative_after{},selected_endpoint_motion{};
    for(const auto &vertex:sample.vertices) {
      // append_selected drops complete parts and retains their expanded source
      // order. Shader LOD may alter material slots; it cannot alter this identity.
      while(cursor<expected.size() && (expected[cursor].source_section!=vertex.source_section ||
          expected[cursor].source_part!=vertex.source_part))++cursor;
      check(cursor<expected.size(),"Corpse submission contains a part outside its actual authored model");
      const auto &source=expected[cursor];
      check(!std::memcmp(&source.source_local_position,&vertex.source_local_position,sizeof(Vec3)) &&
          !std::memcmp(&source.uv,&vertex.uv,sizeof(Vec2)) &&
          source.source_part_type==vertex.source_part_type && source.source_part_flags==vertex.source_part_flags,
          "Corpse submission changed the selected expanded source vertex identity/order");
      const auto error=length(source.position-vertex.position);maximum_skin_error=std::max(maximum_skin_error,error);
      check(error<=.00001f,"Submitted corpse geometry differs from its actual fractional WORLD skin");
      selected_endpoint_motion=std::max(selected_endpoint_motion,length(before[cursor].position-after[cursor].position));
      departure_before=std::max(departure_before,length(vertex.position-before[cursor].position));
      departure_after=std::max(departure_after,length(vertex.position-after[cursor].position));
      const auto point=relative_point(inverse,vertex.position);
      relative_before=std::max(relative_before,length(point-relative_point(previous_inverse,before[cursor].position)));
      relative_after=std::max(relative_after,length(point-relative_point(current_inverse,after[cursor].position)));
      ++cursor;
    }
    ++verified_geometry_samples;
    if(selected_endpoint_motion>.0001f) {
      check(departure_before>.000001f && departure_after>.000001f,
          "Actual moving corpse geometry held a completed endpoint at an interior fraction");
      ++fractional_geometry_samples;
      maximum_fractional_geometry_departure=std::max(maximum_fractional_geometry_departure,std::min(departure_before,departure_after));
    }
    if(articulating && relative_before>.000001f && relative_after>.000001f) {
      ++articulated_fractional_geometry_samples;
      maximum_relative_geometry_departure=std::max(maximum_relative_geometry_departure,std::min(relative_before,relative_after));
    }
  }
  nlohmann::json geometry_diagnostic()const {
    unsigned maximum_per_tick{};
    for(const auto &[tick,count]:interior_by_tick)maximum_per_tick=std::max(maximum_per_tick,count);
    return {{"interior_ticks",interior_by_tick.size()},{"maximum_interior_callbacks_per_tick",maximum_per_tick},
        {"same_tick_pairs",geometry_same_tick},{"same_layout_pairs",geometry_same_layout},
        {"separated_same_tick_layout_pairs",geometry_separated},{"layout_changes",geometry_layout_changes},
        {"cross_tick_geometry_changes",cross_tick_geometry_changes},{"cross_tick_geometry_held",cross_tick_geometry_held},
        {"same_tick_endpoint_resets",same_tick_endpoint_resets},{"endpoint_gap_resets",endpoint_gap_resets},
        {"maximum_same_tick_fraction_separation",maximum_same_tick_fraction_separation},{"examples",geometry_examples}};
  }
  void finish()const {
    std::cout<<"Corpse submitted pose rate: interior_palettes="<<interior_samples<<" changing_geometry_pairs="<<geometry_pairs
        <<" articulated_completed_pairs="<<articulated_completed_pairs<<" articulated_interior_samples="<<articulated_interior_samples
        <<" articulated_geometry_pairs="<<articulated_geometry_pairs<<" maximum_relative_change="<<maximum_relative_change<<'\n';
    std::cout<<"Corpse authored geometry verification: samples="<<verified_geometry_samples<<" fractional="<<fractional_geometry_samples
        <<" articulated_fractional="<<articulated_fractional_geometry_samples<<" maximum_skin_error="<<maximum_skin_error
        <<" maximum_endpoint_departure="<<maximum_fractional_geometry_departure
        <<" maximum_relative_departure="<<maximum_relative_geometry_departure<<'\n';
    std::cout<<"Corpse geometry pairing diagnostic: "<<geometry_diagnostic().dump()<<'\n';
    check(interior_samples && verified_geometry_samples && fractional_geometry_samples && articulated_fractional_geometry_samples,
        "Corpse pose-rate test did not obtain exact fractional native geometry and selected relative articulation");
    std::cout<<"LIMIT relative-articulation counters measure actual source motion; no required OG limb trajectory or displayed frame pacing is supplied\n";
  }
};
// Bounded read-only samples of the real physical owner. Physical-to-WORLD
// differences are diagnostic: source masks/blends can intentionally admit
// only some nodes. Render preservation above uses actual received endpoints.
struct HostCorpsePhysics {
  struct Definition {std::uint32_t model;unsigned root;std::vector<int> rigid_nodes;};
  struct Sample {
    std::uint32_t tick{},owner{},model{},component=null_datum,component_flags{},rows{};
    unsigned mode{},object_flags{};std::int32_t bodies{},constraints{};bool dead{},physical_gate{};
    std::vector<unsigned> nodes;
    std::vector<Transform> physical,world,all_world;
  };
  std::map<std::uint32_t,Definition> definitions;
  std::vector<Sample> samples;
  static std::uint64_t hash(std::span<const Transform> poses) {
    std::uint64_t h=14695981039346656037ull;
    for(const auto &p:poses)for(float value:{p.scale,p.position.x,p.position.y,p.position.z,
        p.basis[0].x,p.basis[0].y,p.basis[0].z,p.basis[1].x,p.basis[1].y,p.basis[1].z,
        p.basis[2].x,p.basis[2].y,p.basis[2].z})h=(h^std::bit_cast<std::uint32_t>(value))*1099511628211ull;
    return h;
  }
  static float change(std::span<const Transform> a,std::span<const Transform> b) {
    if(a.size()!=b.size())return 0;
    float out{};for(unsigned n=0;n<a.size();++n)out=std::max(out,CorpsePoseRate::difference(a[n],b[n]));return out;
  }
  void capture(const Cache &cache,MultiplayerObjectRuntime &source,const MultiplayerState &state,std::uint32_t victim) {
    if(victim==null_datum || samples.size()>=128)return;
    const auto &memory=source.memory();const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto bytes){return memory.view(at,bytes);}};
    const auto header=source_raw_datum_lookup(word(0x4ecbd0u),victim,storage);if(!header)return;
    const auto body=word(header+8u);if(!body)return;
    const auto definition=word(body);auto known=definitions.find(definition);
    if(known==definitions.end()) {
      const auto unit=cache.load_source_tag_metadata(definition);const Bytes u(unit.bytes_at(unit.source_address(),unit.size()));
      const auto model=cache.load_source_tag_metadata(u.u32(0x38));const Bytes m(model.bytes_at(model.source_address(),model.size()));
      const auto render=cache.load_source_tag_metadata(m.u32(4));const Bytes r(render.bytes_at(render.source_address(),render.size()));
      const auto skeleton=read_tag_block_descriptor(r,0x48);const Bytes bones(render.bytes_at(skeleton.address,skeleton.extent(0x60,256)));
      std::optional<unsigned> root;
      for(unsigned n=0;n<unsigned(skeleton.count);++n)if(std::int16_t(bones.u16(n*0x60+4))<0){check(!root,"Physical diagnostic has multiple authored roots");root=n;}
      check(root.has_value(),"Physical diagnostic has no authored root");
      const auto physics=cache.load_source_tag_metadata(m.u32(0x24));const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
      const auto rigid=read_tag_block_descriptor(p,0x38);const Bytes rows(physics.bytes_at(rigid.address,rigid.extent(0x90,256)));
      Definition value{m.u32(4),*root,{}};
      for(unsigned n=0;n<unsigned(rigid.count);++n)value.rigid_nodes.push_back(std::int16_t(rows.u16(n*0x90)));
      known=definitions.emplace(definition,std::move(value)).first;
    }
    const auto &d=known->second;Sample sample;sample.tick=state.tick;sample.owner=victim;sample.model=d.model;
    sample.mode=std::to_integer<unsigned>(memory.view(body+0x34bu,1)[0]);
    sample.object_flags=std::to_integer<unsigned>(memory.view(body+0xc0u,1)[0]);sample.component=word(body+0xb4u);
    sample.dead=std::ranges::any_of(state.corpses,[&](const auto &c){return c.object==victim;});
    const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &p){return p.object==victim;});
    if(pose==state.source_body_poses.end())return;
    check(d.root<pose->nodes.size(),"Physical diagnostic lost its completed WORLD root");
    const auto world_inverse=inverse_transform(pose->nodes[d.root]);
    for(unsigned n=0;n<pose->nodes.size();++n)if(n!=d.root)sample.all_world.push_back(compose_transform(world_inverse,pose->nodes[n]));
    if(sample.component!=null_datum) {
      const auto pool=word(0x52be5cu);check(word(pool+0x24u)==0xa0u,"Physical diagnostic component stride differs from source owner");
      const auto component=source_raw_datum_lookup(pool,sample.component,storage);check(component,"Physical diagnostic lost full component generation");
      sample.component_flags=word(component+4u);sample.bodies=std::bit_cast<std::int32_t>(word(component+0x74u));
      sample.constraints=std::bit_cast<std::int32_t>(word(component+0x80u));sample.rows=word(component+0x70u);
      check(sample.bodies<=256,"Physical diagnostic body bank exceeds its bounded biped evidence");
      sample.physical_gate=sample.dead && sample.mode!=2 && (sample.object_flags&0x40u) && !(sample.component_flags&0x200u) && sample.bodies>1;
      std::map<unsigned,Transform> physical;
      for(int slot=0;slot<sample.bodies;++slot) {
        const auto row=sample.rows+unsigned(slot)*0x60u;if(std::bit_cast<std::int32_t>(word(row+0x4cu))<=0)continue;
        const auto index=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(memory.view(word(row+0x48u),1)[0]));
        if(index<0 || unsigned(index)>=d.rigid_nodes.size())continue;
        const auto node=d.rigid_nodes[unsigned(index)];if(node<0 || unsigned(node)>=pose->nodes.size())continue;
        // Exact1D8B90 owner path; the first rigid node needs no child frame.
        const auto rigid_body=word(word(row+0x40u)+0x3cu);const Bytes matrix(memory.view(rigid_body+0x80u,64));
        const auto vector=[&](unsigned at){return Vec3{matrix.f32(at),matrix.f32(at+4),matrix.f32(at+8)};};
        physical.emplace(unsigned(node),Transform{1,{vector(0),vector(16),vector(32)},vector(48)});
      }
      if(const auto root=physical.find(d.root);root!=physical.end()) {
        const auto inverse=inverse_transform(root->second);
        for(const auto &[node,transform]:physical)if(node!=d.root) {
          sample.nodes.push_back(node);sample.physical.push_back(compose_transform(inverse,transform));
          sample.world.push_back(compose_transform(world_inverse,pose->nodes[node]));
        }
      }
    }
    samples.push_back(std::move(sample));
  }
  void finish(const CorpsePoseRate &rendered)const {
    unsigned gate_frames{},physical_pairs{},world_pairs{},physical_moving_world_held{},matched_received{},received_differences{};
    float maximum_physical{},maximum_world{},maximum_delivery{};nlohmann::json examples=nlohmann::json::array();
    const Sample *previous{};
    for(const auto &s:samples) {
      gate_frames+=s.physical_gate;
      if(const auto received=rendered.received.find(s.tick);received!=rendered.received.end() && received->second.owner==s.owner && received->second.model==s.model) {
        ++matched_received;
        if(s.all_world.size()!=received->second.relative.size())++received_differences;
        else {const auto delta=change(s.all_world,received->second.relative);maximum_delivery=std::max(maximum_delivery,delta);received_differences+=delta>.0001f;}
      }
      float p{},w{};
      if(previous && s.tick==previous->tick+1 && s.owner==previous->owner && s.component==previous->component && s.nodes==previous->nodes && s.physical_gate && previous->physical_gate) {
        p=change(s.physical,previous->physical);w=change(s.world,previous->world);
        physical_pairs+=p>.0001f;world_pairs+=w>.0001f;physical_moving_world_held+=p>.0001f && w<=.0001f;
        maximum_physical=std::max(maximum_physical,p);maximum_world=std::max(maximum_world,w);
      }
      const bool transition=!previous || s.mode!=previous->mode || s.bodies!=previous->bodies || s.component_flags!=previous->component_flags;
      if(examples.size()<8 && (transition || p>.0001f))examples.push_back({{"tick",s.tick},{"dead",s.dead},{"biped_34b",s.mode},{"object_c0",s.object_flags},
          {"component",s.component},{"component_flags",s.component_flags},{"rows_70",s.rows},{"bodies_74",s.bodies},{"constraints_80",s.constraints},
          {"mapped_nonroot_nodes",s.nodes},{"physical_delta",p},{"world_delta",w},{"physical_fnv1a64",hash(s.physical)},{"world_fnv1a64",hash(s.world)}});
      previous=&s;
    }
    const nlohmann::json report{{"scope","Read-only completed native physical owner, WORLD output, received endpoints and final relative render motion; no trajectory supplied"},
        {"samples",samples.size()},{"physical_gate_frames",gate_frames},{"physical_articulating_pairs",physical_pairs},{"world_articulating_pairs",world_pairs},
        {"physical_moving_world_held_pairs",physical_moving_world_held},{"maximum_physical_relative_change",maximum_physical},{"maximum_world_relative_change",maximum_world},
        {"matched_received_ticks",matched_received},{"received_relative_differences",received_differences},{"maximum_received_relative_difference",maximum_delivery},
        {"render_articulated_interior_samples",rendered.articulated_interior_samples},{"render_articulated_geometry_pairs",rendered.articulated_geometry_pairs},
        {"render_verified_geometry_samples",rendered.verified_geometry_samples},{"render_fractional_geometry_samples",rendered.fractional_geometry_samples},
        {"render_articulated_fractional_geometry_samples",rendered.articulated_fractional_geometry_samples},
        {"maximum_render_skin_error",rendered.maximum_skin_error},{"maximum_render_endpoint_departure",rendered.maximum_fractional_geometry_departure},
        {"maximum_render_relative_departure",rendered.maximum_relative_geometry_departure},
        {"render_geometry_pairing",rendered.geometry_diagnostic()},
        {"physical_to_world_asserted",false},{"node_mask_blend_admission_observed",false},{"examples",examples},{"runtime_emulation",false},{"retained_banks",false}};
    if(const auto path=std::getenv("HALO2_DEATH_PHYSICS_REPORT")){std::ofstream out(path);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain bounded native physical diagnostic");}
    std::cout<<"Native corpse physics: samples="<<samples.size()<<" physical_gate="<<gate_frames<<" physical_articulation="<<physical_pairs
        <<" WORLD_articulation="<<world_pairs<<" physical_moving_WORLD_held="<<physical_moving_world_held
        <<" matched_received="<<matched_received<<" render_relative="<<rendered.articulated_interior_samples<<'\n';
  }
};
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply TU1.5 XBE|native-mcc and prepared map");
  const bool native_mcc=std::string_view(argv[1])=="native-mcc";
  if(!std::getenv("DISPLAY") || (!native_mcc && !std::filesystem::exists(argv[1])) || !std::filesystem::exists(argv[2]))return 77;
  SDL_SetMainReady();Files files;
  const bool seated_component=std::getenv("HALO2_DEATH_SEATED_COMPONENT");
  const bool death_audio=std::getenv("HALO2_DEATH_AUDIO")!=nullptr;
  check(!death_audio || !seated_component,"Death audio requires the actual weapon-hit route");
  const auto original=native_mcc?nullptr:OriginalData::load_xbe(argv[1]);Cache cache(argv[2],{},original);
  check(!native_mcc || cache.is_mcc(),"native-mcc requires the prepared imported MCC cache");
  const std::filesystem::path source_image=native_mcc?std::filesystem::path{}:argv[1];
  const auto content=cache.load_content(0,true);CollisionWorld collision(content.collision);
  std::array<Spawn,2> selected{};float closest=std::numeric_limits<float>::infinity();
  for(const auto &a:cache.spawns())for(const auto &b:cache.spawns()) {
    const auto distance=length(b.position-a.position);if(distance<2 || distance>8 || distance>=closest)continue;
    const auto from=a.position+Vec3{0,0,content.player.camera_height};
    const auto delta=b.position+Vec3{0,0,content.player.camera_height}-from;
    if(collision.raycast(from,delta/length(delta),length(delta)))continue;
    selected={a,b};closest=distance;
  }
  check(std::isfinite(closest),"No clear pair of authored starts");
  const UiVariant variant{"Slayer",0,{{89,5},{17,1},{2,0},{10,0}}};
  auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(5);rules.random_seed=0x12345678u;
  const bool pose_rate=std::getenv("HALO2_DEATH_POSE_RATE")!=nullptr;
  HostCorpsePhysics host_physics;
  std::promise<std::string> port;auto pending=port.get_future();std::atomic<bool> advance{},server_failed{};
  std::exception_ptr server_failure;
  std::jthread server([&](std::stop_token stop) {
    bool published{};
    try {
      // Construct and run every source physics/observer owner on this thread.
      MultiplayerOptions host_options;host_options.map=argv[2];host_options.gameplay_xbe=source_image;
      host_options.variant=variant;host_options.starting_weapon=5;host_options.local_players=1;
      host_options.session_random_seed=rules.random_seed;host_options.source_object_world=true;
      host_options.source_object_savegame_directory=files.path;host_options.mute=true;
      // Use the production owner to register the immutable Cache, local view,
      // native birth and recipient packet phase together before joining.
      MultiplayerPreparedWorld host_world(host_options);auto &match=host_world.match;
      check(host_world.source_objects && host_world.source_objects->owns_physics(),
          "Remote death authority lacks its actual source world");
      auto &source=*host_world.source_objects;const auto &host_cache=*host_world.cache;
      const auto &host_content=host_world.content;
      check(match.source_engine_owned(),"Remote death fixture bypassed the canonical source engine");
      check(host_world.local_slots.size()==1 && host_world.local_slots.front()==0 &&
          match.player(0).connected && !match.player(1).connected,
          "Remote death authority must begin with exactly its one genuine local player");
      check(bool(match.session_configuration()),"Remote death authority lost its admitted configuration");
      const auto &configuration=*match.session_configuration();
      // Match the ordinary loading handshake: MCC identifies the admitted
      // selector, before native player birth can choose a live weapon tag.
      const auto weapon_identity=host_world.network_starting_weapon_identity();
      const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(argv[2],
          host_cache.shared_directory(),0,weapon_identity,source_image),configuration);
      LanHost host(match,0,fingerprint,configuration);port.set_value(host.join_address());published=true;
      std::array<Controls,multiplayer_capacity> input{};unsigned warmup{};bool killed{},seated{},positioned{};std::uint32_t original_victim=null_datum;
      const auto yaw_period=Bytes(source.memory().view(0x44923cu,4)).f32(0);
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(input);
        if(advance && match.player(1).connected) {
          ++warmup;
          if(!positioned && match.player(0).source_current_object!=null_datum && match.player(1).source_current_object!=null_datum) {
            // Declare only initial positions through the actual B95D0 input.
            // Original factory/equipment, damage, credit and death remain live.
            source.tu15_world().set_object_position(match.player(0).source_current_object,selected[0].position);
            source.tu15_world().set_object_position(match.player(1).source_current_object,selected[1].position);
            original_victim=match.player(1).source_current_object;positioned=true;
          }
          if(seated_component && warmup==20) {
            const auto snapshot=match.state();std::uint32_t vehicle=null_datum;
            for(const auto &v:snapshot.world_vehicles) {
              const auto definition=host_content.objects.at(v.placement).definition;
              const auto tag=std::find_if(host_cache.tags().begin(),host_cache.tags().end(),[&](const auto &t){return t.datum==definition;});
              if(tag!=host_cache.tags().end() && tag->name=="objects\\vehicles\\ghost\\ghost"){vehicle=v.handle;break;}
            }
            if(vehicle==null_datum)for(unsigned i=0;i<host_content.objects.size();++i) {
              const auto &placement=host_content.objects[i];if(placement.type!=1 || placement.model_attachment)continue;
              const auto tag=std::find_if(host_cache.tags().begin(),host_cache.tags().end(),[&](const auto &t){return t.datum==placement.definition;});
              if(tag!=host_cache.tags().end() && tag->name=="objects\\vehicles\\ghost\\ghost") {
                vehicle=match.create_world_vehicle(i);break;
              }
            }
            check(vehicle!=null_datum,"Remote seated camera component has no actual Ghost");
            auto a=source.tu15_world().prepare_source_call(std::array{vehicle,0u,1u,0x1fu,0u,1u});
            a.gp[0]=match.player(1).source_current_object;source.tu15_world().run_actual(0xec780u,a);seated=true;
          }
          if(seated_component && warmup>=100 && !killed) {
            check(seated && match.state().players[1].source_parent!=null_datum,"Remote fatal cleanup lost its seated source unit");
            // Deliver the original fatal stimulus before the next world pass.
            // Its world owner completesCCF40; eager cleanup here would detach
            // the source player before Match has sampled the completed frame.
            const auto unit=match.player(1).source_current_object;
            source.tu15_world().call_actual(0xd8c20u,std::array{unit,0x60040000u});
            source.tu15_world().call_actual(0xd8d50u,std::array{unit,0x60040000u,1u,0u});killed=true;
          }
          if(!seated_component && positioned && !killed) {
            const auto &shooter=match.unit(0),&victim=match.unit(1);
            const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,shooter.definition.crouch_camera_height,shooter.crouch);
            const auto delta=victim.body.position+Vec3{0,0,victim.body.height*.5f}-eye;
            const auto look=source.player_look(match.player(0).source_datum,match.player(0).source_current_object);
            if(look) {
              // Controls retain raw look accumulators. Correct actual cooked
              // source look instead of replacing the raw value with atan2.
              input[0].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,yaw_period);
              input[0].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
              input[0].fire=warmup>=100;
            } else check(warmup<100,"Canonical shooter never constructed its actual source look");
          }
          check(warmup<100 || positioned,"Canonical factory never birthed the declared firing pair");
          match.step(input);
          if(pose_rate && warmup>=95)host_physics.capture(host_cache,source,match.state(),original_victim);
          killed|=match.player(1).deaths!=0;if(killed)input[0].fire=false;
        }
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){server_failure=std::current_exception();if(!published)port.set_exception(server_failure);server_failed=true;}
  });
  MultiplayerOptions options;options.map=argv[2];options.gameplay_xbe=source_image;options.variant=variant;
  options.starting_weapon=5;options.width=640;options.height=480;options.mute=!death_audio;
  options.join=pending.get();
  options.validation=std::getenv("HALO2_DEATH_VALIDATION");
  MultiplayerState state;std::uint32_t victim=null_datum;
  bool dead{},corpse{},camera{},board{},respawn{},restored{},retained{},submitted{},quitting{};
  bool seated_received{},seated_rendered{},on_foot_restored{};
  bool look_injected{};
  bool mouse_raw_observed{};
  unsigned respawn_input_checks{};
  std::optional<float> initial_raw_yaw,seated_raw_yaw;
  std::map<std::uint32_t,unsigned> body_draws;
  DamageAudioEvidence damage_audio;
  if(death_audio)options.audio_observer=[&](const MultiplayerAudioSubmission &packet){damage_audio.packet(packet);};
  CorpsePoseRate corpse_pose_rate;
  options.input_observer=[&](unsigned,std::span<const Controls> inputs,const MultiplayerState &snapshot) {
    if(server_failed)std::rethrow_exception(server_failure);
    state=snapshot;advance=true;
    if(state.tick>=450) {
      std::cerr<<"Remote stages: dead="<<dead<<" corpse="<<corpse<<" camera="<<camera<<" board="<<board
        <<" respawn="<<respawn<<" restored="<<restored<<" retained="<<retained<<" submitted="<<submitted
        <<" seated_received="<<seated_received<<" seated_rendered="<<seated_rendered
        <<" on_foot_restored="<<on_foot_restored<<'\n';
      throw FormatError("Remote rendered cycle timed out");
    }
    const auto &p=state.players[1];if(!p.connected)return;
    if(seated_component && !initial_raw_yaw && !inputs.empty())initial_raw_yaw=inputs.front().yaw;
    if(seated_component && look_injected && initial_raw_yaw && !inputs.empty())
      mouse_raw_observed|=inputs.front().yaw!=*initial_raw_yaw;
    if(seated_component && look_injected && !dead && !p.dead && p.source_parent!=null_datum && !inputs.empty())
      seated_raw_yaw=inputs.front().yaw;
    if(seated_component && respawn && seated_raw_yaw && !inputs.empty()) {
      check(inputs.front().yaw==*seated_raw_yaw,"Respawn rebased the native relative-look accumulator");
      ++respawn_input_checks;
    }
    if(seated_component && p.source_parent!=null_datum && !p.dead) {
      check(p.source_vehicle_camera.has_value(),"UDP seated original director was omitted");
      seated_received|=p.source_vehicle_camera->camera_mode==1;
    }
    if(!dead && !p.dead)victim=p.source_unit;
    if(death_audio)damage_audio.capture(state,victim,content);
    if(p.dead){
      if(!seated_component)check(p.deaths==1 && state.players[0].kills==1,
          "Remote death did not originate from the actual credited source weapon hit");
      dead=true;check(p.source_death_camera.has_value(),"UDP death camera was omitted");
    }
    if(dead && !p.dead){respawn=true;check(p.source_unit!=null_datum && p.source_unit!=victim && !p.source_death_camera,"Remote respawn retained the old camera or body");}
    if(pose_rate)corpse_pose_rate.capture(cache,state,victim);
  };
  options.view_state_observer=[&](unsigned,const MultiplayerState &rendered,const Camera &view) {
    const auto &own=rendered.players[1];
    on_foot_restored|=respawn && !own.dead && !own.source_death_camera && !own.source_vehicle_camera;
    if(seated_component && own.source_vehicle_camera && !own.dead) {
      const auto &expected=*own.source_vehicle_camera;
      check(length(view.position-expected.position)<.0001f && length(view.forward-expected.forward)<.0001f,
        "Remote seated render discarded the original authority camera");seated_rendered=true;
    }
    for(const auto &[unit,count]:body_draws) {
      check(count==1,"Remote death/respawn submitted a duplicate player model");
      const bool living=std::any_of(rendered.players.begin(),rendered.players.end(),[&](const auto &p){
        return p.connected && !p.dead && p.source_unit==unit;
      });
      const bool retained_corpse=std::any_of(rendered.corpses.begin(),rendered.corpses.end(),[&](const auto &p){return p.object==unit;});
      check(living!=retained_corpse,"Remote death/respawn submitted an unowned or both live and corpse model");
    }
    body_draws.clear();
    if(rendered.players[1].source_death_camera) {
      const auto &expected=*rendered.players[1].source_death_camera;
      check(length(view.position-expected.position)<.0001f && length(view.forward-expected.forward)<.0001f,
          "Remote renderer did not submit the host's completed observer view");camera=true;
    }
  };
  options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &draw) {
    if(!draw.first_person) {
      check(++body_draws[draw.unit]==1,"Remote death/respawn submitted a duplicate player model");
    }
    if(dead && !draw.first_person && draw.unit==victim){
      check(!draw.vertices.empty() && !draw.body_nodes.empty(),"Remote corpse geometry unavailable");
      if(pose_rate)corpse_pose_rate.draw(draw);
      corpse=true;retained|=respawn;
    }
  };
  options.scoreboard_observer=[&](unsigned,bool normal,const FullScoreboardLayout &layout,const FullScoreboardPaintResult &paint) {
    if(dead && !normal){check(layout.rows.size()==2 && !paint.draw.vertices.empty(),"Remote automatic scoreboard missing");board=true;}
    if(respawn && normal && paint.draw.vertices.empty())restored=true;
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
    if(seated_component && !look_injected && state.players[1].source_unit!=null_datum && !state.players[1].dead) {
      auto *window=SDL_GetKeyboardFocus();if(!window)window=SDL_GetMouseFocus();
      if(window) {
        check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Capture native camera mouse input");
        SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=SDL_GetWindowID(window);motion.motion.xrel=137;
        check(SDL_PushEvent(&motion)==1,"Queue native camera mouse input");look_injected=true;
      }
    }
    // The declared fatal component does not publish a credited weapon kill.
    // Automatic scoreboard admission remains with the weapon-hit test.
    submitted|=frame.submitted && corpse && camera && (board || seated_component);
    if(submitted && restored && retained && (!seated_component || (on_foot_restored && respawn_input_checks>=3)) && !quitting){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Queue remote test completion");quitting=true;}
  };
  int result=-1;std::exception_ptr runtime_failure;
  try{result=run_multiplayer(options);}catch(...){runtime_failure=std::current_exception();}
  server.request_stop();server.join();
  if(pose_rate)try{host_physics.finish(corpse_pose_rate);}catch(const std::exception &error) {
    if(!runtime_failure && !server_failure)throw;
    std::cerr<<"Physical diagnostic after runtime failure: "<<error.what()<<'\n';
  }
  if(server_failure)std::rethrow_exception(server_failure);
  if(runtime_failure)std::rethrow_exception(runtime_failure);
  check(result==0 && dead && corpse && camera && (board || seated_component) && respawn && restored && retained && submitted,
      "Remote source death/camera/scoreboard/respawn GPU acceptance failed");
  if(pose_rate)corpse_pose_rate.finish();
  if(death_audio)damage_audio.finish(cache,victim);
  if(seated_component)check(seated_received && seated_rendered && on_foot_restored && seated_raw_yaw.has_value() && mouse_raw_observed,"Remote seated camera handoff or native mouse input was not rendered");
  if(seated_component){std::cout<<"PASS UDP seated original director, source fatal cleanup, corpse skin, death camera and salted respawn on GPU\n";return 0;}
  std::cout<<"PASS UDP source fatal hit, replicated corpse WORLD skin, authoritative obstructed camera, automatic scoreboard, salted respawn and HUD restoration on GPU\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
