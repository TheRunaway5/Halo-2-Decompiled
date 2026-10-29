#include "helpers/gameplay_fixture.hpp"
#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/frontend_model_ik.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void key(Uint32 window,SDL_Scancode code,bool down) {
 SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
 e.key.keysym.scancode=code;e.key.keysym.sym=SDL_GetKeyFromScancode(code);
 e.key.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&e)==1,"Queue body action key");
}
void mouse(Uint32 window,int x,int y) {
 SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window;e.motion.xrel=x;e.motion.yrel=y;
 check(SDL_PushEvent(&e)==1,"Queue body aim motion");
}
void fire(Uint32 window,bool down) {
 SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
 e.button.windowID=window;e.button.button=SDL_BUTTON_LEFT;e.button.state=down?SDL_PRESSED:SDL_RELEASED;
 check(SDL_PushEvent(&e)==1,"Queue sword trigger");
}
int geometry_probe(MultiplayerOptions options,bool roam=false) {
 options.local_players=1;
 if(const auto *width=std::getenv("HALO2_GEOMETRY_WIDTH")) {
  options.width=unsigned(std::stoul(width));options.height=options.width*9/16;
 }
 if(const auto *views=std::getenv("HALO2_GEOMETRY_VIEWS")) {
  options.local_players=unsigned(std::stoul(views));
  check(options.local_players>=1 && options.local_players<=4,"Geometry probe requires one to four views");
 }
 std::vector<Spawn> spawns;
 if(roam) {
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto bank=options.prepared_world->cache->spawns();
  for(unsigned i=0;i<12;++i)spawns.push_back(bank[i*bank.size()/12]);
 }
 options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(options.width,options.height,false);
 const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
 std::optional<unsigned> start;unsigned input_step{},draws{};bool raised{},walked{},looked{};
 float distance{};std::optional<Vec3> last;
 options.input_observer=[&](unsigned step,std::span<const Controls> controls,const MultiplayerState &state) {
  input_step=step;check(step<120 || start,"Geometry probe never obtained SDL focus");if(!start)return;
  const auto age=step-*start;const auto &p=state.players[0];
  if(roam) {
   if(age==1)fire(window_id,true);
   if(age==2)fire(window_id,false);
   if(last && !p.dead)distance+=length(p.position-*last);last=p.position;
   walked|=controls[0].forward!=0 || controls[0].right!=0;
   const auto phase=age%180;
   if(phase==5) {
    key(window_id,SDL_SCANCODE_W,false);
    const auto &at=spawns[(age/180)%spawns.size()];
    if(!p.dead)test::place_player(options.prepared_world->match,0,at);
    std::cout<<"Roam tick="<<step<<" spawn="<<(age/180)%spawns.size()<<" at="
        <<at.position.x<<','<<at.position.y<<','<<at.position.z<<'\n';
   }
   if(phase==10)key(window_id,SDL_SCANCODE_W,true);
   if(phase==100)key(window_id,SDL_SCANCODE_W,false);
   if(phase>=10) {
    mouse(window_id,phase<100?8:32,int(std::lround(3*std::sin(age*.04f))));looked=true;
   }
   if(age==180*spawns.size()) {
    SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"End roaming geometry probe");
   }
   return;
  }
  if(age==1)fire(window_id,true);
  if(age==2)fire(window_id,false);
  if(last && !p.dead)distance+=length(p.position-*last);last=p.position;
  walked|=controls[0].forward!=0 || controls[0].right!=0;
  if(age==10 || age==230)key(window_id,SDL_SCANCODE_W,true);
  if(age==140 || age==350)key(window_id,SDL_SCANCODE_W,false);
  if(age>=150 && age<210){mouse(window_id,12,0);looked=true;}
  if(age>=240 && age<340)mouse(window_id,4,0);
  if(age==370)key(window_id,SDL_SCANCODE_A,true);
  if(age==450){key(window_id,SDL_SCANCODE_A,false);key(window_id,SDL_SCANCODE_S,true);}
  if(age==540)key(window_id,SDL_SCANCODE_S,false);
  if(age==600){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"End geometry probe");}
 };
 options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample) {
  if(!sample.first_person)return;
  if(!raised){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);raised=true;}
  if(!start && SDL_GetKeyboardFocus()==window)start=input_step;
  ++draws;check(!sample.weapon_nodes.empty(),"Geometry probe lost actual first-person nodes");
  for(const auto &v:sample.vertices) {
   const auto p=v.position-sample.weapon_nodes[0].position;
   check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&dot(p,p)<25,
       "First-person geometry contains an exploded vertex");
  }
 };
 check(run_multiplayer(options)==0,"Geometry runtime failed");
 check(draws>100 && walked && looked && distance>2,"Geometry probe did not exercise actual walking and turning");
 std::cout<<"PASS walking, stationary camera turn and combined movement; views="<<options.local_players<<"; "<<draws
     <<" bounded first-person draws, traveled="<<distance<<"; visual flicker remains a separate observation\n";
 return 0;
}
}
int main(int argc,char **argv){try {
 check(argc==4 || argc==5,"Supply MAP SHARED XBE [--elite|--geometry|--roam]");
 if(argc==5)check(std::string_view(argv[4])=="--elite" || std::string_view(argv[4])=="--geometry" || std::string_view(argv[4])=="--roam","Unknown body probe option");
 if(!std::getenv("DISPLAY"))return 77;
 SDL_SetMainReady();MultiplayerOptions options;
 options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
 if(std::filesystem::equivalent(options.gameplay_xbe,options.shared_maps.parent_path()/"default.xbe"))options.gameplay_xbe.clear();
 options.local_players=2;options.starting_weapon=5;options.width=800;options.height=600;
 options.mute=true;options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
 if(argc==5 && std::string_view(argv[4])=="--geometry")return geometry_probe(options);
 if(argc==5 && std::string_view(argv[4])=="--roam")return geometry_probe(options,true);
 const unsigned representation=argc==5?3:2;
 Cache cache(options.map,options.shared_maps,options.gameplay_xbe.empty()?nullptr:OriginalData::load_xbe(options.gameplay_xbe));
 const auto content=cache.load_content(0,true);
 const auto sword=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &entry){return entry.second.inventory_flags&(1u<<24);});
 check(sword!=content.weapons.end(),"Actual cache has no sword");
 const auto row=std::find_if(content.netgame_equipment.begin(),content.netgame_equipment.end(),[&](const auto &r){
  return r.items.size()==1 && r.items.front().definition==sword->first;
 });
 check(row!=content.netgame_equipment.end(),"Actual map has no authored sword placement");
 auto spawn=cache.spawns().front();spawn.position=row->position-Vec3{.2f,0,0};spawn.facing=0;spawn.player_type=0;
 for(unsigned i=0;i<2;++i) {
  const std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(i?representation==2:representation==3),0,0,0,0};
  options.selected_profiles.push_back(native_selected_player_configuration("Body "+std::to_string(i),appearance,-1));
 }
 options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
 options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(options.width,options.height,false);
 const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
 const auto model=cache.load_model(content.player_representations[representation].biped.datum,false);
 const auto smg=cache.slayer_starting_weapon(5);
 const auto support_ik=cache.load_unit_weapon_ik(content.player_representations[representation].biped.datum,
     smg,0x06000086u);
 NativePlayerAnimation smg_pose(cache,model.animation,smg),sword_pose(cache,model.animation,sword->first);
 std::vector<unsigned> upper;
 for(unsigned n=0;n<model.bones.size();++n) {
  const auto name=cache.string_id(model.bones[n].name);
  if(name=="head" || name.ends_with("upperarm") || name.ends_with("hand"))upper.push_back(n);
 }
 check(upper.size()>=5,"Authored body has no aim/hand node bank");
 MultiplayerState current;std::optional<unsigned> start;unsigned input_step{},draws{};
 bool raised{},picked_up{},ground_special{},air_special{},up{},down{},yaw_aim{},melee_key{};
 auto safe_spawn=cache.spawns().front();bool returned_from_pickup{};
 unsigned ordinary_requests{};
 float observed_facing{};
 std::array<std::set<unsigned>,2> turn_frames;
 std::map<std::uint32_t,std::set<std::uint64_t>> melee_poses;
 std::set<std::uint32_t> ordinary_starts;std::set<std::uint64_t> crouch_special_poses;
 std::set<std::uint64_t> neutral_aim,up_aim,down_aim,yaw_poses;
 unsigned support_hand_checks{},reload_pose_checks{};
 options.input_observer=[&](unsigned step,std::span<const Controls>,const MultiplayerState &state) {
  current=state;input_step=step;
  check(step<120 || start,"Body probe never obtained SDL focus");if(!start)return;
  const auto age=step-*start;
  if(age==1)fire(window_id,true);
  if(age==2)fire(window_id,false);
  if(age==3)mouse(window_id,0,-300);
  if(age==20)mouse(window_id,0,600);
  if(age==40)mouse(window_id,0,-300);
  if(age==50)mouse(window_id,160,0);
  if(age==70 || age==110) {
   const float gain=.0025f*float(options.desktop->keyboard_settings.sensitivity)/3.f;
   const float target=observed_facing+(age==70?1.8f:-1.8f);
   mouse(window_id,int(std::lround(-std::remainder(target-state.players[0].yaw,6.283185307f)/gain)),0);
  }
  if(age==123)key(window_id,SDL_SCANCODE_R,true);
  if(age==124)key(window_id,SDL_SCANCODE_R,false);
  // Declared diagnostic placement uses the real teleport owner. Inventory,
  // action admission, clocks and all animation selection remain live.
  const auto &p=state.players[0];
  if(age==154){safe_spawn.position=p.position;safe_spawn.facing=0;}
  if(age>=155 && age<=180 && !picked_up)
   test::place_player(options.prepared_world->match,0,spawn);
  if(age==156)key(window_id,SDL_SCANCODE_E,true);
  if(age==180)key(window_id,SDL_SCANCODE_E,false);
  if(age==200 || age==265 || age==350)fire(window_id,true);
  if(age==201 || age==266 || age==351)fire(window_id,false);
  if(age==260)key(window_id,SDL_SCANCODE_SPACE,true);
  if(age==261)key(window_id,SDL_SCANCODE_SPACE,false);
  if(age==320)key(window_id,SDL_SCANCODE_LCTRL,true);
  if(age==390)key(window_id,SDL_SCANCODE_LCTRL,false);
  picked_up|=p.weapon==sword->first;
  if(picked_up && !returned_from_pickup) {
   test::place_player(options.prepared_world->match,0,safe_spawn);returned_from_pickup=true;
  }
  if(melee_key){key(window_id,SDL_SCANCODE_Q,false);melee_key=false;}
  else if(age>=420 && ordinary_requests<3 &&
      (ordinary_requests==0 || (p.melee_action.active() && p.melee_action.recovery>=0 &&
       p.melee_action.progress>=p.melee_action.recovery))) {
   key(window_id,SDL_SCANCODE_Q,true);melee_key=true;++ordinary_requests;
  }
  ground_special|=p.melee_action.activity==0x0b0005b2u;air_special|=p.melee_action.activity==0x140005b3u;
  up|=p.pitch>.25f;down|=p.pitch<-.25f;
  if(age==530){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"End body probe");}
 };
 options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &sample) {
  if(view==0 && sample.first_person) {
   if(!raised){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);raised=true;}
   if(!start && SDL_GetKeyboardFocus()==window)start=input_step;
  }
  if(sample.first_person || player!=0 || !start)return;
  const auto &p=current.players[0];if(!p.connected)return;
  observed_facing=sample.body_facing;
  ++draws;const auto age=input_step-*start;
  check(sample.body_nodes.size()==model.bones.size(),"Actual body draw lost its node bank");
  if(p.weapon==smg && !p.reload_seconds && age>=14 && age<150)for(const auto &constraint:support_ik.definition.constraints) {
   check(!sample.weapon_nodes.empty(),"Third-person held weapon lost its actual node bank");
   const auto hand=compose_transform(sample.body_nodes[constraint.arms_node],constraint.arms_local);
   const auto target=compose_transform(sample.weapon_nodes[constraint.weapon_node],constraint.weapon_local);
   check(length(hand.position-target.position)<.002f,"Third-person SMG support hand missed the authored weapon marker");
   ++support_hand_checks;
  }
  if(p.weapon==smg && p.reload_seconds>0 && age>=140 && age<150) {
   const auto *reload=smg_pose.action(0x08000002u,p.native_crouching,false);
   check(reload,"Actual SMG reload lost its body channel");
   const auto primary=cache.load_animation(sample.graph,sample.animation);
   const auto tick=sample.interpolated?double(sample.tick)-1+sample.fraction:double(sample.tick);
   const bool moving=sample.body_activity==0x0a000014u || sample.body_activity==0x09000015u ||
       sample.body_activity==0x09000016u || sample.body_activity==0x0a000017u;
   const auto locals=smg_pose.pose(model,primary,sample.frame,p.pitch,p.native_crouching,false,moving,
       reload,float(tick-double(p.reload_tick)),nullptr,0,std::remainder(p.yaw-sample.body_facing,6.283185307f),
       0,nullptr,sample.body_activity,(reload->authored_flags&16)?0.f:1.f,false);
   const auto expected=pose_model_locals_nodes(model,locals);
   const auto actual_root=inverse_transform(sample.body_nodes[0]),expected_root=inverse_transform(expected[0]);
   for(unsigned n=0;n<model.bones.size();++n) {
    const auto a=compose_transform(actual_root,sample.body_nodes[n]),b=compose_transform(expected_root,expected[n]);
    for(unsigned axis=0;axis<3;++axis)check(length(a.basis[axis]-b.basis[axis])<.0003f,
        "Actual reload retained forbidden aim/hand IK or overwrote its final grip");
   }
   ++reload_pose_checks;
  }
  for(const auto &v:sample.vertices) {
   const auto delta=v.position-sample.body_nodes[0].position;
   if(!(std::isfinite(delta.x)&&std::isfinite(delta.y)&&std::isfinite(delta.z)&&dot(delta,delta)<25))
    std::cerr<<"Body bound age="<<age<<" tick="<<sample.tick<<" activity="<<std::hex<<sample.body_activity
        <<" clip="<<sample.animation<<std::dec<<" frame="<<sample.frame<<" vertex="<<v.position.x<<','<<v.position.y<<','<<v.position.z
        <<" player="<<p.position.x<<','<<p.position.y<<','<<p.position.z
        <<" root="<<sample.body_nodes[0].position.x<<','<<sample.body_nodes[0].position.y<<','<<sample.body_nodes[0].position.z<<'\n';
   check(std::isfinite(delta.x)&&std::isfinite(delta.y)&&std::isfinite(delta.z)&&dot(delta,delta)<25,
       "Actual animation submitted an exploded body triangle");
  }
  std::uint64_t hash=14695981039346656037ull;
  const float c=std::cos(sample.body_facing),s=std::sin(sample.body_facing);
  for(auto n:upper)for(const auto axis:sample.body_nodes[n].basis)
   for(float component:{axis.x*c+axis.y*s,-axis.x*s+axis.y*c,axis.z}) {
    hash^=std::uint32_t(std::int32_t(std::lround(component*100000)));hash*=1099511628211ull;
   }
  if(age>=14 && age<20)up_aim.insert(hash);
  if(age>=30 && age<40)down_aim.insert(hash);
  if(age>=44 && age<50)neutral_aim.insert(hash);
  if(age>=51 && age<70){yaw_poses.insert(hash);yaw_aim|=std::abs(std::remainder(p.yaw-sample.body_facing,6.283185307f))>.1f;}
  if(sample.body_activity==0x0900000eu)turn_frames[0].insert(unsigned(sample.frame));
  if(sample.body_activity==0x0a00000fu)turn_frames[1].insert(unsigned(sample.frame));
  if(p.melee_action.active()) {
   const auto *clip=(p.weapon==sword->first?sword_pose:smg_pose).action(p.melee_action.activity,p.native_crouching,false);
   check(clip,"Admitted melee lost its authored body binding");
   if(clip->animation_type==0)check(sample.graph==clip->source_graph && sample.animation==clip->source_index,
       "Full-body melee never replaced the rendered primary channel");
   melee_poses[p.melee_action.activity].insert(hash);
   if(age>=420)ordinary_starts.insert(p.melee_action.started);
   if(age>=350 && p.native_crouching && p.melee_action.activity==0x0b0005b2u)crouch_special_poses.insert(hash);
  }
 };
 check(run_multiplayer(options)==0,"Body runtime failed");
 check(picked_up&&ground_special&&air_special,"Actual SDL sword pickup/ground/air special admission missing");
 std::cout<<"Aim observations: up="<<up<<" down="<<down<<" yaw="<<yaw_aim<<" node banks="
     <<neutral_aim.size()<<'/'<<up_aim.size()<<'/'<<down_aim.size()<<'/'<<yaw_poses.size()<<'\n';
 check(up&&down&&yaw_aim && !neutral_aim.empty() && !up_aim.empty() && !down_aim.empty() && !yaw_poses.empty(),"Actual SDL aim never reached the rendered body");
 check(*neutral_aim.begin()!=*up_aim.begin() && *neutral_aim.begin()!=*down_aim.begin() && *up_aim.begin()!=*down_aim.begin() && *neutral_aim.begin()!=*yaw_poses.begin(),
     "Third-person aim submitted unchanged upper-body nodes");
 std::cout<<"Turn observations: left="<<turn_frames[0].size()<<" right="<<turn_frames[1].size()<<'\n';
 for(const auto &frames:turn_frames)check(frames.size()>1,"Third-person stationary turn stayed frozen");
 for(auto activity:{0x0b0005b2u,0x140005b3u}) {
  if(melee_poses[activity].size()<=3)std::cerr<<"Melee activity="<<std::hex<<activity<<std::dec<<" poses="<<melee_poses[activity].size()<<'\n';
  check(melee_poses[activity].size()>3,"Third-person special melee did not animate");
 }
 check(ordinary_starts.size()==3 && crouch_special_poses.size()>3,"Actual ordinary/crouching sword strikes did not animate");
 check(!support_ik.world_callback || support_ik.definition.constraints.empty() || support_hand_checks>10,
     "Actual third-person SMG support-hand alignment was not exercised");
 std::cout<<"Authored SMG support-hand marker checks="<<support_hand_checks<<'\n';
 check(reload_pose_checks>5,"Actual body reload admission/aim/IK composition was not exercised");
 std::cout<<"Actual reload body-pose/aim/IK checks="<<reload_pose_checks<<'\n';
 std::cout<<"PASS representation="<<representation<<" actual SDL yaw/pitch aim, left/right turns ("<<turn_frames[0].size()<<'/'<<turn_frames[1].size()
     <<" frames), retained sword pickup, ground/air/crouch specials and three ordinary strikes; "<<draws<<" bounded body draws\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
