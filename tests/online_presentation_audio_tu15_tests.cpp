// Online presentation audio journals from an actual TU1.5 source-object world.
// The authority world owns damage, zoom, destruction and engine events. This
// test observes which original sound requests and replicated journals those
// events produce, then round-trips the authority state through the native LAN
// snapshot codec so a non-host consumes exactly what the host published.
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_vista_announcer.hpp"
#include "halo2/menu_music.hpp"
#include <sstream>
#include "halo2/math.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Request {unsigned tick;std::uint32_t sound,flags,owner;unsigned location;};
std::optional<Spawn> firing_lane(const LevelContent &content,const CollisionWorld &terrain,
    Vec3 center,std::span<const float> distances) {
  for(float distance:distances)for(unsigned side=0;side<32;++side) {
    const float angle=float(side)*.19634954084936207f;
    const Vec3 direction{std::cos(angle),std::sin(angle),0};
    auto origin=center+direction*distance+Vec3{0,0,4};float remaining=10;
    for(unsigned layer=0;layer<4 && remaining>0;++layer) {
      const auto support=terrain.raycast(origin,{0,0,-1},remaining);if(!support)break;
      const auto position=support->position+Vec3{0,0,content.player.radius};
      const auto eye=position+Vec3{0,0,content.player.camera_height},delta=center-eye;
      if(support->normal.z>=content.player.maximum_slope_cosine && std::abs(delta.z)<2.f &&
          !terrain.overlaps(position,content.player.height,content.player.radius) &&
          !terrain.raycast(eye,normalized(delta),length(delta))){Spawn spawn;spawn.position=position;return spawn;}
      const float step=content.player.radius*.1f;remaining-=support->distance+step;
      origin=support->position-Vec3{0,0,step};
    }
  }
  return {};
}
}
int main(int argc,char **argv){try {
  check(argc==3,"Supply MCC_LOCKOUT_MAP MCC_MAPS");
  if(!std::filesystem::exists(argv[1]))return 77;
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=2;options.source_object_world=true;options.mute=true;
  options.session_random_seed=0x22de570u;
  if(const auto scratch=std::getenv("HALO2_TEST_SCRATCH"))
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"presentation-audio";
  MultiplayerPreparedWorld world(options);
  auto &match=world.match;auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto &cache=*world.cache;const auto &content=world.content;
  check(source.owns_physics() && cache.is_mcc(),"Presentation audio requires the MCC TU1.5 source world");
  const auto name=[&](std::uint32_t datum)->std::string {
    if(datum==null_datum)return "NONE";
    const auto tag=cache.query_tag({.handle=TagHandle{datum}});return tag?tag->group+":"+tag->name:"?";
  };
  unsigned tick=0;std::vector<Request> requests;
  source.observe_sound_requests([&](std::uint32_t sound,std::span<const std::byte,160> raw) {
    const Bytes r(raw);requests.push_back({tick,sound,r.u32(0),r.u32(0x50),std::to_integer<unsigned>(raw[0xf])&15u});
  });
  std::array<Controls,multiplayer_capacity> commands{};
  const auto step=[&](unsigned count) {
    for(unsigned n=0;n<count;++n){match.step(commands);source.advance_effect_frame(tick_seconds);++tick;}
  };
  for(unsigned n=0;n<600 && (match.state().players[0].source_unit==null_datum ||
      match.state().players[1].source_unit==null_datum);++n)step(1);
  auto state=match.state();
  check(state.players[0].source_unit!=null_datum && state.players[1].source_unit!=null_datum,
      "Source Slayer never spawned both local players");
  std::cout<<"source_engine="<<state.source_engine<<" ticks="<<tick<<" weapon0="<<name(state.players[0].weapon)
      <<" weapon1="<<name(state.players[1].weapon)<<'\n';
  const auto dump=[&](const char *phase,std::size_t from) {
    std::map<std::string,unsigned> counts;
    for(auto n=from;n<requests.size();++n) {
      const auto &q=requests[n];
      const auto owner=q.flags&8u?name(source.object_definition(q.owner)):std::string("-");
      std::ostringstream key;key<<name(q.sound)<<" flags="<<std::hex<<q.flags<<std::dec<<" loc="<<q.location<<" owner="<<owner;
      ++counts[key.str()];
    }
    std::cout<<"== "<<phase<<" source sound requests\n";
    for(const auto &[key,count]:counts)std::cout<<"  "<<count<<"x "<<key<<'\n';
  };
  // Engine events through the actual Xbox response banks.
  {
    std::uint32_t crt=1;
    NativeVistaAnnouncerConfiguration configuration{cache.shared_directory(),cache.scenario(),
        cache.original_data(),0,30,{0x27f,0x1f80},true,source_secondary_random_seed(1,2,source_crt_random(crt))};
    configuration.source_tu15=true;auto announcer=NativeVistaAnnouncer::open(std::move(configuration));
    unsigned selected=0;
    for(const auto &event:state.source_engine_events) {
      const auto choices=announcer->publish_source_event(event,world.local_slots);
      for(const auto &choice:choices)if(choice.sound!=null_datum)++selected;
      std::cout<<"engine event serial="<<event.serial<<" tick="<<event.tick<<" category="<<event.words[0]
          <<" type="<<event.words[1]<<" target="<<std::hex<<event.words[2]<<std::dec<<" choices="<<choices.size();
      for(const auto &choice:choices)std::cout<<' '<<name(choice.sound);
      std::cout<<'\n';
    }
    std::cout<<"announcer selections="<<selected<<" events="<<state.source_engine_events.size()<<'\n';
  }
  CollisionWorld terrain(content.collision);
  const auto aim=[&](unsigned slot,Vec3 target) {
    const auto unit=match.state().players[slot].source_unit;
    const auto eye=source.object_transform(unit).position+Vec3{0,0,content.player.camera_height};
    const auto delta=target-eye;
    commands[slot].yaw=std::atan2(delta.y,delta.x);commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  };
  // Zoom: request the first source scope level, then release it.
  {
    const auto from=requests.size();
    state=match.state();commands[0].zoom_reset_serial=state.players[0].zoom_reset_serial;
    commands[0].zoom=0;step(20);commands[0].zoom=-1;step(20);
    state=match.state();
    std::cout<<"zoom current="<<int(state.players[0].source_zoom.current)<<'\n';
    dump("zoom",from);
  }
  // Damage: slot1 shoots slot0 from a supported clear lane.
  {
    const auto from=requests.size();const auto voices_before=match.state().unit_damage_voices.size();
    const auto target=match.state().players[0].source_unit;
    const auto center=source.object_sphere(target).center;
    const std::array distances{3.f,4.f,5.f};
    const auto lane=firing_lane(content,terrain,center,distances);check(bool(lane),"No clear damage lane");
    const auto shooter=match.state().players[1].source_unit;
    test::place_player(match,1,*lane);native.set_object_position(shooter,lane->position);
    step(10);
    std::set<std::uint64_t> voices;
    for(unsigned n=0;n<240;++n) {
      state=match.state();if(state.players[0].dead)break;
      aim(1,source.object_sphere(state.players[0].source_unit).center);
      commands[1].fire=(n%8)<4;commands[1].reload=state.players[1].loaded==0;step(1);
      for(const auto &voice:match.state().unit_damage_voices)voices.insert(voice.serial);
    }
    commands[1].fire=false;step(30);
    state=match.state();
    std::cout<<"damage target_dead="<<state.players[0].dead<<" health="<<state.players[0].health
        <<" voices_before="<<voices_before<<" voices_seen="<<voices.size()<<'\n';
    dump("damage",from);
  }
  // Explosive prop: shoot an authored Lockout fusion coil until it dies.
  {
    std::vector<std::uint32_t> coils;
    for(const auto &object:source.live_objects()) {
      if(object.parent!=null_datum || object.type!=11)continue;
      if(name(object.definition).ends_with("\\fusion_coil"))coils.push_back(object.object);
    }
    std::cout<<"fusion coils="<<coils.size()<<'\n';
    check(!coils.empty(),"Lockout has no authored fusion coil");
    for(unsigned n=0;n<600;++n){state=match.state();if(!state.players[0].dead && state.players[0].source_unit!=null_datum)break;step(1);}
    const auto from=requests.size();
    const auto detonations_before=match.state().grenade_detonations.empty()?0:match.state().grenade_detonations.back().serial;
    std::optional<std::uint32_t> coil;std::optional<Spawn> lane;
    for(const auto candidate:coils) {
      const std::array distances{7.f,6.f,5.f};
      if((lane=firing_lane(content,terrain,source.object_sphere(candidate).center,distances))){coil=candidate;break;}
    }
    check(bool(coil),"No clear coil lane");
    const auto shooter=match.state().players[0].source_unit;
    test::place_player(match,0,*lane);native.set_object_position(shooter,lane->position);step(10);
    const auto alive=[&]{return std::ranges::any_of(source.live_objects(),[&](const auto &o){return o.object==*coil;}) &&
        source.object_vitality(*coil).body>0;};
    unsigned shots=0;
    for(unsigned n=0;n<600 && alive();++n) {
      state=match.state();if(state.players[0].dead)break;
      aim(0,source.object_sphere(*coil).center);
      commands[0].fire=(n%8)<4;commands[0].reload=state.players[0].loaded==0;shots+=commands[0].fire;step(1);
    }
    commands[0].fire=false;step(60);
    state=match.state();unsigned detonations=0;
    for(const auto &event:state.grenade_detonations)if(event.serial>detonations_before) {
      ++detonations;std::cout<<"detonation effect="<<name(event.effect)<<" owner="<<name(event.definition)<<'\n';
    }
    std::cout<<"coil destroyed="<<!alive()<<" detonations="<<detonations<<'\n';
    dump("coil",from);
  }
  source.observe_sound_requests({});
  std::cout<<"PASS diagnostic\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
