#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/cache.hpp"
#include "halo2/collision.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_shadow.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-death-match-XXXXXX").string();
    const auto p=::mkdtemp(name.data());check(p,"Create native source savegame directory");path=p;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==3,"Supply owned English TU1.5 XBE and Xbox map, or native-mcc and prepared MCC map");
  const bool native_mcc=std::string_view(argv[1])=="native-mcc";
  if((!native_mcc&&!std::filesystem::exists(argv[1])) || !std::filesystem::exists(argv[2]))return 77;
  Cache cache(argv[2],{},native_mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[1]));
  check(!native_mcc||cache.is_mcc(),"native-mcc requires the prepared imported MCC cache");
  const auto original=cache.original_data();auto content=cache.load_content(0,true);
  const std::string mode=std::getenv("HALO2_TU15_TEST_DAMAGE")?std::getenv("HALO2_TU15_TEST_DAMAGE"):"direct";
  const bool canonical_voice=std::getenv("HALO2_TU15_TEST_DAMAGE_VOICE")!=nullptr;
  check(!canonical_voice || mode=="direct","Canonical damage voice acceptance requires the actual direct weapon route");
  const bool automatic_spectate=std::getenv("HALO2_TU15_TEST_AUTO_SPECTATE")!=nullptr;
  const auto respawn_seconds=automatic_spectate?20:5;
  CollisionWorld collision(content.collision);
  std::array<Spawn,2> selected{};float closest=std::numeric_limits<float>::infinity();
  for(const auto &a:cache.spawns())for(const auto &b:cache.spawns()) {
    const auto delta=b.position-a.position;const float distance=length(delta);
    if(distance<2 || distance>8 || distance>=closest)continue;
    const auto from=a.position+Vec3{0,0,content.player.camera_height};
    const auto to=b.position+Vec3{0,0,content.player.camera_height};
    const auto ray=to-from;
    if(collision.raycast(from,ray/length(ray),length(ray)))continue;
    selected={a,b};closest=distance;
  }
  check(std::isfinite(closest),"No nearby unobstructed pair in the authored spawn bank");
  float walk_yaw{};
  if(mode=="fall") {
    float edge=std::numeric_limits<float>::infinity();
    // Select an actual authored start with a clear walk to a BSP ledge. This
    // produces input only; world contact, falling and death remain source-owned.
    for(const auto &start:cache.spawns())for(unsigned angle=0;angle<64;++angle) {
      const float yaw=float(angle)*6.28318530718f/64.f;const Vec3 heading{std::cos(yaw),std::sin(yaw),0};
      for(float distance=.5f;distance<20 && distance<edge;distance+=.25f) {
        const auto point=start.position+heading*distance;
        if(collision.raycast(start.position+Vec3{0,0,1.5f},heading,distance))break;
        if(!collision.raycast(point+Vec3{0,0,.3f},{0,0,-1},12.f)) {
          selected[1]=start;walk_yaw=yaw;edge=distance;break;
        }
      }
    }
    check(std::isfinite(edge),"No authored start with an unobstructed BSP ledge");
    // Match chooses from this real bank; both starts use the same authored
    // location so victim placement does not depend on spawn-selection RNG.
    selected[0]=selected[1];
    std::cout<<"Authored ledge distance="<<edge<<" yaw="<<walk_yaw<<'\n';
  }
  Files files;MultiplayerObjectRuntime source(native_mcc?std::filesystem::path{}:std::filesystem::path(argv[1]),cache,0,files.path);
  auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
  source_game_variant_set(variant,0x11,respawn_seconds);
  source.configure_session(variant,1,30,0x12345678u);
  auto rules=slayer_rules({"Slayer",0,{{89,5},{17,respawn_seconds}}});rules.weapon=cache.slayer_starting_weapon(5);
  rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,canonical_voice?std::span<const Spawn>(cache.spawns()):std::span<const Spawn>(selected),rules,{}, {},&source);
  if(canonical_voice)check(match.source_engine_owned(),"Damage voice fixture bypassed the canonical source engine");
  const auto resident=std::shared_ptr<const Cache>(&cache,[](const Cache*){});
  if(mode=="melee")match.configure_melee_animation(resident,*original->melee_constants);
  match.join("Shooter");
  const bool network=std::getenv("HALO2_TU15_TEST_NETWORK")!=nullptr;
  std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
  std::array<Controls,1> peer_input{};
  if(network) {
    host=std::make_unique<LanHost>(match,0,0x12345678u);
    client=std::make_unique<LanClient>(host->join_address(),0x12345678u,1);
    std::array<Controls,multiplayer_capacity> incoming{};
    for(unsigned attempt=0;attempt<50 && !client->ready();++attempt) {
      client->send(peer_input);host->receive(incoming);host->broadcast();client->receive();
    }
    check(client->ready() && client->slots().size()==1 && client->slots()[0]==1,"Source UDP peer did not join the real victim slot");
  } else match.join("Victim");
  std::array<Controls,multiplayer_capacity> input{};
  const auto advance=[&] {
    if(client) {peer_input[0]=input[1];client->send(peer_input);host->receive(input);}
    match.step(input);
    if(host){host->broadcast();client->receive();check(client->error().empty(),"Source UDP snapshot rejected");}
  };
  bool positioned=!canonical_voice;
  const auto aim_canonical=[&] {
    if(!canonical_voice || !positioned || match.player(0).source_current_object==null_datum || match.player(1).source_current_object==null_datum)return;
    const auto look=source.player_look(match.player(0).source_datum,match.player(0).source_current_object);if(!look)return;
    const auto &shooter=match.unit(0),&target=match.unit(1);
    const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,shooter.definition.crouch_camera_height,shooter.crouch);
    const auto delta=target.body.position+Vec3{0,0,target.body.height*.5f}-eye;
    input[0].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,Bytes(source.memory().view(0x44923cu,4)).f32(0));
    input[0].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
  };
  for(unsigned tick=0;tick<100;++tick) {
    if(!positioned && match.player(0).source_current_object!=null_datum && match.player(1).source_current_object!=null_datum) {
      // Declared authored B95D0 positions only; genuine player factory,
      // original equipment, weapon damage and credited death remain live.
      source.tu15_world().set_object_position(match.player(0).source_current_object,selected[0].position);
      source.tu15_world().set_object_position(match.player(1).source_current_object,selected[1].position);positioned=true;
    }
    aim_canonical();advance();
  }
  check(positioned,"Canonical source factory never birthed the damage witness pair");
  check(!match.unit(0).vitality.dead && !match.unit(1).vitality.dead,"Authored warmup killed a participant");
  const auto victim=match.unit(1).source_datum,owner=match.player(1).source_datum;
  std::uint32_t random=0x12345678u;NativeDeathCamera camera(source,*original,random);
  Mesh mesh;MultiplayerScoreboard board(cache,gameplay_font_directory(cache),mesh,{"Slayer",0,{{89,5}}});
  check(board.available(),"Original scoreboard providers unavailable");
  const auto ordinary=[&] {
    const auto &u=match.unit(1);
    return NativeDeathCameraView{ordinary_unit_eye(u.body.position,u.definition.camera_height,
        u.definition.crouch_camera_height,u.crouch),{std::cos(u.yaw),std::sin(u.yaw),0},{0,0,1},u.definition.camera_field_of_view};
  };
  camera.update(0,owner,match.player(1).source_flags,false,tick_seconds,false,ordinary());
  const bool moving_victim=std::getenv("HALO2_TU15_TEST_MOVING_VICTIM")!=nullptr;
  for(unsigned tick=0;tick<900 && !match.unit(1).vitality.dead;++tick) {
    if(moving_victim)input[1].forward=.1f;
    const auto &shooter=match.unit(0),&target=match.unit(1);
    const auto eye=ordinary_unit_eye(shooter.body.position,shooter.definition.camera_height,
        shooter.definition.crouch_camera_height,shooter.crouch);
    const auto delta=target.body.position+Vec3{0,0,target.body.height*.5f}-eye;
    if(canonical_voice)aim_canonical();
    else {input[0].yaw=std::atan2(delta.y,delta.x);input[0].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));}
    if(mode=="melee") {
      input[0].forward=length(target.body.position-shooter.body.position)>.75f?.2f:0.f;
      input[0].melee=!shooter.melee_action.active() && input[0].forward==0;
    } else if(mode=="radius") {
      // Admitted detonation seam, actual globals grenade0 -> projectile ->
      // authored explosion definition. No damage amount/reaction is supplied.
      const auto globals=cache.load_source_tag_metadata(cache.query_tag({.group=tag_groups::globals})->datum);
      const Bytes root(globals.bytes_at(globals.source_address(),0x108));
      const Bytes grenade(globals.bytes_at(root.u32(0x104),0x2c));
      const auto projectile=cache.load_source_tag_metadata(grenade.u32(0x28));
      const Bytes definition(projectile.bytes_at(projectile.source_address(),0x108));
      const auto explosion=load_prop_area_damage_definition(cache,definition.u32(0x104));
      check(explosion!=nullptr,"Authored grenade explosion is unavailable");
      const auto position=source.object_sphere(victim).center;
      const_cast<Simulation&>(match.simulation()).area_damage(*explosion,position,
          {match.player(0).source_datum,shooter.source_datum,shooter.source_membership},-1,
          null_datum,grenade.u32(0x28));
    } else if(mode=="fall") {
      input[1].forward=1;input[1].yaw=walk_yaw;
      input[1].jump=tick%60==0;
    } else input[0].fire=true;
    advance();
  }
  if(mode=="fall") {
    const auto &body=match.unit(1).body;
    std::cout<<"Walk-off position="<<body.position.x<<','<<body.position.y<<','<<body.position.z
        <<" velocity="<<body.velocity.x<<','<<body.velocity.y<<','<<body.velocity.z<<'\n';
  }
  check(match.unit(1).vitality.dead && match.player(1).deaths==1 &&
      (mode=="fall" || (match.player(0).kills==1 && (canonical_voice || match.simulation().hits()>0))),
      "Actual authored-spawn damage did not produce a fatal hit");
  if(std::getenv("HALO2_TU15_TEST_DAMAGE_VOICE")) {
    const auto state=client?client->state():match.state();
    const auto voices=std::count_if(state.unit_damage_voices.begin(),state.unit_damage_voices.end(),
        [&](const auto &event){return event.target==victim;});
    const auto lethal=std::count_if(state.unit_damage_voices.begin(),state.unit_damage_voices.end(),
        [&](const auto &event){return event.target==victim && (event.report_flags&1u);});
    std::cout<<"Actual native damage voice journal: target="<<victim<<" requests="<<voices<<" lethal="<<lethal<<'\n';
    check(voices && lethal,"Actual native fatal hit omitted the original player hurt/death voice requests");
  }
  input={};bool observer{},automatic{},replaced{},restored{},ragdoll{};
  const bool spectate=automatic_spectate || std::getenv("HALO2_TU15_TEST_SPECTATE")!=nullptr;
  bool followed{},remote_followed{};Vec3 watched_start{};float watched_travel{};
  if(spectate) {
    const auto watched=source.camera_target(match.unit(0).source_datum);
    check(watched.has_value(),"Living watched player has no WORLD target");watched_start=watched->node0;
  }
  const bool long_lifetime=automatic_spectate || std::getenv("HALO2_TU15_TEST_LONG_CORPSE")!=nullptr;
  bool retired{};
  std::vector<Transform> first;bool moved{};
  const std::array<unsigned,2> ids{0,1};
  for(unsigned tick=0;tick<(automatic_spectate?900u:long_lifetime?450u:300u);++tick) {
    const auto state=client?client->state():match.state();
    if(client) {
      check(state.tick==match.state().tick,"Source UDP did not publish the complete current tick");
      check(state.players[1].source_death_camera.has_value()==state.players[1].dead,
          "Remote source camera missing during death or retained after respawn");
      check(!state.players[0].source_death_camera,"Remote snapshot leaked another peer's camera");
    }
    const auto corpse=std::find_if(state.corpses.begin(),state.corpses.end(),[&](const auto &c){return c.object==victim;});
    if(long_lifetime && tick>=150 && corpse==state.corpses.end()) {
      check(match.simulation().object_lifetimes().header(victim).empty() && !match.unit_from_datum(victim),
          "Corpse disappeared before its full source datum retired");retired=true;break;
    }
    check(corpse!=state.corpses.end() && corpse->player==owner,"Match retired or reassigned the detached corpse");
    const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &p){return p.object==victim;});
    check(pose!=state.source_body_poses.end(),"Match omitted the retained corpse WORLD nodes");
    if(first.empty())first=pose->nodes;
    else for(unsigned n=0;n<first.size();++n)moved|=length(first[n].position-pose->nodes[n].position)>.0001f;
    const auto view=camera.update(0,owner,state.players[1].source_flags,false,tick_seconds,
        spectate && !automatic_spectate && tick==36,ordinary());
    if(state.players[1].source_unit==null_datum){check(view.has_value(),"Detached Match player did not select the source observer");observer=true;}
    else {check(state.players[1].source_unit!=victim && !view,"Respawn did not restore camera ownership");replaced=true;}
    if(spectate && state.players[1].dead) {
      const auto watched=source.camera_target(match.unit(0).source_datum);
      check(watched.has_value(),"Living watched player lost its actual WORLD target");
      watched_travel=std::max(watched_travel,length(watched->node0-watched_start));
      // After the original three-second transition, the completed camera's
      // forward ray must point at the live player's current WORLD node0.
      // A camera that remained on the corpse fails this geometric check.
      if(tick>=(automatic_spectate?550u:130u)) {
        const auto follows=[&](const NativeDeathCameraView &v) {
          const auto delta=watched->node0-v.position;
          return dot(delta,v.forward)>0 && length(cross(delta,v.forward))<.05f;
        };
        check(follows(*view),"Local spectator stopped following the living player before respawn");
        followed=true;
        if(client) {
          check(follows(*state.players[1].source_death_camera),
              "Remote spectator stopped following the living player before respawn");
          remote_followed=true;
        }
      }
    }
    for(unsigned i=0;i<2;++i)board.step(i,false,state,ids[i]);
    const auto draw=board.draw(state,ids,1,1280,720);
    // Original automatic admission can have a short slot-stagger interval
    // before respawn. A visible partial fade is still the real popup; it need
    // not reach the separate full-opacity HUD-suppression threshold.
    automatic|=!draw.paint.draw.vertices.empty();
    restored|=replaced && automatic && draw.normal_hud && draw.paint.draw.vertices.empty();
    const auto word=[&](auto p){const auto bytes=source.memory().view(p,4);std::uint32_t v{};
      for(unsigned n=0;n<4;++n)v|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);return v;};
    const auto component=word(source.object_body(victim)+0xb4u),pool=word(0x52be5cu);
    if(tick==0) {
      const auto p=source.object_body(victim),manager=p+std::uint16_t(word(p+0x12au));
      const auto activity=cache.string_id(word(manager+8u));const auto packed=word(manager+4u);
      if(mode=="direct" && !moving_victim)check(activity=="s_kill" && std::uint16_t(packed)==0xffffu,
          "Stationary source hit did not install its authored soft-kill channel");
      const auto frames=packed==0xffffffffu?0:cache.load_animation(word(manager),packed>>16).frame_count;
      if(mode=="direct")check(frames>0,"Natural fatal reaction installed an empty authored channel");
      if(mode=="radius")check(activity=="airborne_dead" && frames==37,
          "The admitted explosion lost its natural authored death animation");
      std::cout<<"Natural fatal activity="<<activity<<" channel="<<std::hex<<packed
          <<std::dec<<" frames="<<frames<<'\n';
    }
    const auto row=word(pool+0x44u)+(component&65535u)*word(pool+0x24u);
    ragdoll|=word(row+0x74u)==10 && word(row+0x80u)==9;
    if(tick==0 || tick==1 || tick==8 || tick==30 || tick==149) {
      const auto p=source.object_body(victim);
      const auto byte=[&](unsigned offset){return std::to_integer<unsigned>(source.memory().view(p+offset,1)[0]);};
      std::cout<<"Corpse tick "<<tick<<" bodies="<<word(row+0x74u)<<" constraints="<<word(row+0x80u)
          <<" mode="<<byte(0x34b)<<" flags="<<byte(0x349)<<" sleep_flags="<<byte(0xc1)
          <<" state="<<byte(0x3dc)<<" contact="<<byte(0x3fc)<<','<<byte(0x3fd)<<','<<byte(0x3fe)<<'\n';
    }
    if(spectate) {
      // Real movement and a short Jump press, rather than supplied camera or
      // target state. The remote path consumes the transmitted press serial.
      input[0].forward=tick>=45 && tick<55?.1f:0.f;
      input[1].jump=!automatic_spectate && tick==35;
    }
    advance();
  }
  std::cout<<"Source Match gates observer="<<observer<<" automatic="<<automatic
      <<" replaced="<<replaced<<" restored="<<restored<<" ragdoll="<<ragdoll
      <<" moved="<<moved<<'\n';
  if(long_lifetime)std::cout<<"Corpse extended lifetime retired="<<retired<<'\n';
  check(observer && automatic && replaced && restored && ragdoll && moved,
      "Source Match death/ragdoll/camera/scoreboard/respawn cycle incomplete");
  if(spectate) {
    std::cout<<"Spectator gates followed="<<followed<<" remote="<<remote_followed
        <<" watched_travel="<<watched_travel<<'\n';
    check(followed && (!network || remote_followed) && watched_travel>.01f,
        "Cycle input did not follow the moving live player until respawn");
  }
  std::cout<<"PASS authored-spawn "<<mode<<" fatal hit, "<<(moving_victim?"moving":"stationary")
      <<" victim, ten-body ragdoll, source observer, automatic scoreboard, respawn and retained corpse"
      <<(network?" over UDP":"")<<'\n';
  std::cout<<"LIMIT headless acceptance; GPU corpse/camera and remote observer delivery are separate checks\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
