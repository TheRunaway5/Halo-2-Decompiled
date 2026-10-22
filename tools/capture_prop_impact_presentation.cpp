#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {void check(bool value,const char* why){if(!value)throw FormatError(why);}}
// Declared native acceptance scene. The shooter's placement is supplied;
// controls, collision, damage, response and Vulkan presentation run normally.
// This is not a matched original gameplay frame.
int main(int argc,char**argv)try{
  check(argc==5,"Supply MAP SHARED_MAPS TU15_XBE OUTPUT_DIRECTORY");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  const auto original=OriginalData::load_xbe(options.gameplay_xbe);
  check(original->image==OriginalData::Image::TitleUpdate15,"Capture requires OG Xbox TU1.5");
  Cache cache(options.map,options.shared_maps,original);const auto content=cache.load_content(0,true);
  auto rules=slayer_rules({"Slayer",0,{}});rules.weapon=cache.slayer_starting_weapon(5);rules.time_ticks=0;
  MultiplayerMatch probe(content,0,cache.spawns(),rules);probe.join("Prop contact capture");
  const auto& props=probe.simulation().props();std::uint32_t module=null_datum;
  for(const auto& state:props.states())if(state.definition==0xe6f80583u){module=state.handle;break;}
  check(module!=null_datum,"Actual Lockout power module missing");
  const auto placement=props.state(module).placement;
  CollisionWorld terrain(content.collision);Vec3 eye{},target{};bool found=false;
  for(const auto& triangle:props.triangles()){
    if(triangle.source.instance!=int(placement))continue;
    const auto normal=normalized(triangle.source_normal);if(std::abs(normal.z)>.25f)continue;
    const auto center=(triangle.a+triangle.b+triangle.c)/3.f,origin=center+normal*.8f;
    const auto hit=props.raycast(origin,center-origin);
    if(!hit || hit->handle!=module || terrain.raycast(origin,normalized(center-origin),.79f))continue;
    eye=origin;target=center;found=true;break;
  }
  check(found,"Actual module has no unobstructed authored firing face");
  const auto aim=normalized(target-eye);Spawn spawn;spawn.bsp=0;
  spawn.position=eye-Vec3{0,0,content.player.camera_height};spawn.facing=std::atan2(aim.y,aim.x);
  const auto output=std::filesystem::absolute(argv[4]);std::filesystem::create_directories(output);
  options.input_script=output/"inputs.txt";
  const auto pitch=std::atan2(aim.z,std::hypot(aim.x,aim.y));
  {std::ofstream input(options.input_script);input.precision(9);
    for(unsigned tick=0;tick<38;++tick)input<<tick<<" 0 "<<(tick>=30)<<" 0 0 "<<spawn.facing<<' '<<pitch<<'\n';}
  options.native_spawn_inputs={spawn};options.starting_weapon=5;options.width=1280;options.height=720;
  options.hidden=true;options.validation=true;options.mute=true;options.capture_tick=38;
  options.screenshot=output/"prop-impact.ppm";options.reference_tick_cadence=true;
  options.reference_render_fraction=0;options.reference_auxiliary_seed=0x12345678;
  // Use the existing complete offline activation policy with declared fixture placement.
  options.defer_initial_player_spawn=true;
  std::uint32_t player=null_datum;
  options.player_birth_owner=[&](MultiplayerPlayer& owner,unsigned){player=owner.source_datum;owner.source_flags=8;};
  options.pre_player_update_owner=[&](MultiplayerMatch& match){
    const auto* owner=match.player_from_datum(player);check(owner,"Capture player owner missing");
    if(!(owner->source_flags&1u))match.apply_source_player_activation(player,owner->source_flags,owner->source_flags|1u);
    auto& unit=const_cast<Unit&>(match.unit(0));
    unit.body.position=spawn.position;unit.body.velocity={};unit.body.grounded=false;
    unit.yaw=spawn.facing;unit.pitch=pitch;
  };
  unsigned contacts{};std::uint64_t serial{};
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state){
    for(const auto& hit:state.bullet_impacts)if(hit.serial>serial){
      serial=hit.serial;if(hit.terrain)continue;
      check(hit.material==86 && hit.presentation && hit.presentation->scale_b<1,
          "Actual prop contact lost its authored damage material or vitality scale");
      ++contacts;std::cout<<"Prop impact serial="<<hit.serial<<" material="<<hit.material
          <<" response="<<hit.presentation->response<<" scale_a="<<hit.presentation->scale_a
          <<" scale_b="<<hit.presentation->scale_b<<'\n';
    }
  };
  std::cout<<"Declared native shooter eye="<<eye.x<<','<<eye.y<<','<<eye.z
      <<" target="<<target.x<<','<<target.y<<','<<target.z<<" module="<<module<<'\n';
  const auto result=run_multiplayer(options);
  check(result==0 && contacts && std::filesystem::exists(options.screenshot),"Native prop impact capture incomplete");
  std::cout<<"PASS actual native prop impacts="<<contacts<<"; declared placement, not an original-frame comparison\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
