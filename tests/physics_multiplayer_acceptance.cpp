#include "halo2/source_match_authority.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/source_contrail_draw.hpp"
#include "halo2/physics_velocity_observation.hpp"
#include "helpers/source_device_script_input_tu15.hpp"
#include "helpers/live_phantom_original_capture_tu15.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "helpers/live_native_physics_frame_capture_tu15.hpp"
#include "helpers/original_support_carry_tu15.hpp"
#include "helpers/destruction_firing_lane_inputs_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "source_device_snapshot_check.hpp"
#include <atomic>
#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/posed_collision.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/hud.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/animation_presentation.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <future>
#include <thread>
#include <SDL_syswm.h>

// Inputs are ordinary SDL events. Location overrides use declared authored
// lift or vehicle-entry frames before tick zero. Nothing supplies a
// contact, velocity, force, solver result or post-step transform.
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
bool same_dynamic(const SourceDynamicObject &a,const SourceDynamicObject &b) {
  return a.object==b.object && a.definition==b.definition && a.parent==b.parent && a.type==b.type &&
    a.runtime_flags==b.runtime_flags && a.control_flags==b.control_flags && a.visible==b.visible &&
    a.frozen==b.frozen && a.has_world_nodes==b.has_world_nodes && a.camouflage==b.camouflage &&
    !std::memcmp(&a.transform,&b.transform,sizeof(Transform)) && !std::memcmp(&a.velocity,&b.velocity,sizeof(Vec3)) &&
    !std::memcmp(&a.sphere,&b.sphere,sizeof(a.sphere)) &&
    a.regions.selection.variant_index==b.regions.selection.variant_index &&
    a.regions.selection.permutations==b.regions.selection.permutations &&
    a.regions.selection.states==b.regions.selection.states && a.regions.looping_effects==b.regions.looping_effects &&
    a.colors.size()==b.colors.size() && (a.colors.empty() || !std::memcmp(a.colors.data(),b.colors.data(),a.colors.size()*sizeof(Vec3)));
}
std::string hex_bytes(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
  for(const auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);result+=digits[value>>4];result+=digits[value&15];}
  return result;
}
Json impulse_json(const SourceBodyImpulseTu15 &event) {
  return {{"object",event.object},{"tick",event.tick},{"body_index",event.body_index},
      {"motion_before",hex_bytes(event.before)},{"motion_after",hex_bytes(event.after)},
      {"impulse",hex_bytes(event.impulse)},{"point",hex_bytes(event.point)}};
}
void focus(SDL_Window *window);
void key(Uint32 window,SDL_Scancode scan,bool down) {
  if(down) {
    auto *surface=SDL_GetWindowFromID(window);check(surface,"Acceptance key lost its actual window");
    focus(surface);
  }
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;e.key.keysym.scancode=scan;
  e.key.keysym.sym=SDL_GetKeyFromScancode(scan);
  check(SDL_PushEvent(&e)==1,"SDL rejected acceptance key input");
}
void focus(SDL_Window *window) {
#if defined(HALO2_ACCEPTANCE_X11_FOCUS) && defined(SDL_VIDEO_DRIVER_X11)
  SDL_SysWMinfo native{};SDL_VERSION(&native.version);
  if(SDL_GetWindowWMInfo(window,&native) && native.subsystem==SDL_SYSWM_X11) {
    // The finite UI test represents the user's requested interaction. Ask the
    // window manager to activate its real surface, rather than forging SDL
    // focus events or relying on a raise that the compositor may ignore.
    const auto display=native.info.x11.display;
    XEvent event{};event.xclient.type=ClientMessage;event.xclient.display=display;
    event.xclient.window=native.info.x11.window;event.xclient.format=32;
    event.xclient.message_type=XInternAtom(display,"_NET_ACTIVE_WINDOW",False);
    event.xclient.data.l[0]=2;event.xclient.data.l[1]=CurrentTime;
    check(XSendEvent(display,DefaultRootWindow(display),False,
        SubstructureRedirectMask|SubstructureNotifyMask,&event)!=0,"Acceptance WM activation request failed");
    XFlush(display);
  }
#endif
  SDL_RaiseWindow(window);
  check(SDL_SetWindowInputFocus(window)==0,"Acceptance window could not acquire OS focus");
  if(!SDL_GetRelativeMouseMode()) {
    int w{},h{};SDL_GetWindowSize(window,&w,&h);SDL_WarpMouseInWindow(window,w/2,h/2);
    check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Acceptance window could not capture actual mouse input");
  }
  // Deliver actual backend focus transitions before inserting keys. A delayed
  // lost event can otherwise clear the press, and loading releases capture.
  SDL_PumpEvents();
  const auto deadline=SDL_GetTicks()+1000u;
  while(!(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS) && SDL_GetTicks()<deadline) {
    SDL_RaiseWindow(window);
    check(SDL_SetWindowInputFocus(window)==0,"Acceptance focus request failed after window presentation");
    SDL_Delay(10);SDL_PumpEvents();
  }
  check(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS,"Acceptance window did not retain actual OS input focus");
}
void quit() {SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"SDL rejected acceptance quit");}
struct ObserverController {
  SDL_Joystick *joystick{};SDL_JoystickID instance=-1;
  ObserverController() {
    const int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    check(device>=0 && SDL_IsGameController(device),"Could not attach the visible observer's SDL gamepad");
    joystick=SDL_JoystickOpen(device);check(joystick,"Could not open the actual virtual observer device");
    instance=SDL_JoystickInstanceID(joystick);
    axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT,-1.f);axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-1.f);
    axis(SDL_CONTROLLER_AXIS_LEFTY,1.f);
  }
  void button(SDL_GameControllerButton button,bool down) {
    check(SDL_JoystickSetVirtualButton(joystick,int(button),down?1:0)==0,"SDL rejected observer button input");
  }
  void axis(SDL_GameControllerAxis axis,float value) {
    check(SDL_JoystickSetVirtualAxis(joystick,int(axis),Sint16(std::lround(
        std::clamp(value,-1.f,1.f)*32767.f)))==0,"SDL rejected observer stick input");
  }
  ~ObserverController() {
    SDL_JoystickClose(joystick);
    for(int n=0;n<SDL_NumJoysticks();++n)if(SDL_JoystickGetDeviceInstanceID(n)==instance) {
      SDL_JoystickDetachVirtual(n);break;
    }
  }
};
Spawn lift_entrance(const Cache &cache,const LevelContent &content,Vec3 *geometry_input=nullptr) {
  const auto requested=std::getenv("HALO2_TU15_LIFT_DEFINITION");
  const std::string name=requested?requested:
      "scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube";
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag&t){
    return (t.group=="bloc" || t.group=="mach") && t.name==name;
  });
  check(tag!=cache.tags().end(),"Authored selected lift definition absent");
  unsigned instance=0;
  if(const auto selected=std::getenv("HALO2_TU15_LIFT_INSTANCE")) {
    const std::string text=selected;std::size_t used{};instance=std::stoul(text,&used);
    check(used==text.size(),"Invalid declared lift placement ordinal");
  }
  auto at=content.objects.end();
  for(auto candidate=content.objects.begin();candidate!=content.objects.end();++candidate)
    if(candidate->definition==tag->datum && instance--==0) {at=candidate;break;}
  check(at!=content.objects.end(),"Authored selected lift placement absent");
  const auto object=cache.load_source_tag_metadata(at->definition);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
  const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
  const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
  check(p.u32(0x108)>0,"Selected lift has no authored phantom shape");
  const auto basis=authored_basis(at->rotation);
  const auto root=source_object_transform(at->position,basis.forward,basis.up,at->scale,false);
  Vec3 position{},lower_floor{};float field_radius{};bool selected=false;
  for(unsigned n=0;n<p.u32(0x108);++n) {
    const Bytes phantom(physics.bytes_at(p.u32(0x10c)+n*0x20u,0x20));
    const auto type=phantom.u16(0xc),index=phantom.u16(0xe);
    Vec3 candidate;float radius{},floor_z{};
    if(type==4) {
      // Original1F31E0 type4 selects phmo+70, stride100, shape+20.
      // The authored hull center supplies the same pre-tick input as Lockout.
      check(index<p.u32(0x70),"Lift phantom selects an absent authored hull");
      const Bytes hull(physics.bytes_at(p.u32(0x74)+index*0x100u,0x100));
      radius=hull.f32(0x2c);
      Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
      candidate=compose_transform(root,local).position;floor_z=candidate.z;
    } else if(type==1) {
      // Original1F31E0 type1 selects phmo+58, stride50, shape+20.
      // Declare the lower authored capsule endpoint as the test spawn input.
      check(index<p.u32(0x58),"Lift phantom selects an absent authored capsule");
      const Bytes capsule(physics.bytes_at(p.u32(0x5c)+index*0x50u,0x50));
      radius=capsule.f32(0x2c);
      Transform first,second;
      first.position={capsule.f32(0x30),capsule.f32(0x34),capsule.f32(0x38)};
      second.position={capsule.f32(0x40),capsule.f32(0x44),capsule.f32(0x48)};
      const auto a=compose_transform(root,first).position,b=compose_transform(root,second).position;
      candidate=a.z<=b.z?a:b;floor_z=candidate.z-radius;
    } else if(type==2) {
      // Whole1F31E0 returns phmo+64 + index*90 +40. Whole1F2C70
      // binds its child box at+20 and transform wrapper at+40. Actual
      // wrapper2E8A00 subtracts its translation+40 (=record+80).
      check(index<p.u32(0x60),"Lift phantom selects an absent authored box");
      const Bytes box(physics.bytes_at(p.u32(0x64)+index*0x90u,0x90));
      radius=std::max(box.f32(0x30),box.f32(0x34));
      Transform local;local.position={box.f32(0x80),box.f32(0x84),box.f32(0x88)};
      for(unsigned axis=0;axis<3;++axis)local.basis[axis]={
          box.f32(0x50+axis*0x10),box.f32(0x54+axis*0x10),box.f32(0x58+axis*0x10)};
      const auto placed=compose_transform(root,local);candidate=placed.position;
      floor_z=candidate.z-std::abs(placed.scale)*(
          std::abs(placed.basis[0].z*box.f32(0x30))+
          std::abs(placed.basis[1].z*box.f32(0x34))+
          std::abs(placed.basis[2].z*box.f32(0x38)));
    } else throw FormatError("Selected lift test shape requires its original geometry input owner");
    // Distinct exit/intake phantoms retain authored order. Choose the
    // lowest geometry input so this case begins in the lower intake field.
    if(!selected || candidate.z<position.z) {position=candidate;lower_floor={candidate.x,candidate.y,floor_z};field_radius=radius;selected=true;}
  }
  check(selected,"Selected lift has no admitted authored geometry input");
  Spawn spawn;spawn.position=position;spawn.facing=at->rotation.z;
  if(geometry_input) {
    // A convex hull's center can be above the lower intake floor. Retain
    // its authored horizontal center and declare the placement's lower Z
    // as the approach floor-query anchor; the source world solves entry.
    if(field_radius==0)position.z=std::min(position.z,root.position.z);
    *geometry_input=position;
    check(field_radius>=0 && std::isfinite(field_radius),"Lift approach has invalid authored shape radius");
    // Declared input only: approach from outside the authored lower field.
    // The source controller, collisions and force owners solve the walk/lift.
    const CollisionWorld terrain(content.collision);bool found=false;
    // Convex hulls can have zero convex radius. For those, declare a start
    // two player heights away and seek the floor below their authored center.
    float approach_distance=field_radius>0?field_radius+2*content.player.radius:2*content.player.height;
    if(const auto declared=std::getenv("HALO2_TU15_LIFT_APPROACH_HEIGHTS")) {
      const std::string text=declared;std::size_t used{};const float heights=std::stof(text,&used);
      check(used==text.size() && std::isfinite(heights) && heights>0,"Invalid declared lift approach clearance");
      approach_distance=field_radius+heights*content.player.height;
    }
    const auto floor_distance=(field_radius>0?4.f:8.f)*content.player.height;
    // Deterministic declared approach directions around the actual field.
    // Some authored lifts border a void on their forward side.
    std::vector<Vec3> directions{basis.forward,basis.left,basis.forward*-1.f,basis.left*-1.f,
        normalized(basis.forward+basis.left),normalized(basis.forward-basis.left),
        normalized(basis.left-basis.forward),normalized((basis.forward+basis.left)*-1.f)};
    const bool clear_approach=std::getenv("HALO2_TU15_LIFT_CLEAR_APPROACH");
    if(clear_approach)for(unsigned n=0;n<32;++n) {
      const float angle=6.283185307179586f*float(n)/32.f;
      directions.push_back(normalized(basis.forward*std::cos(angle)+basis.left*std::sin(angle)));
    }
    for(const auto direction:directions) {
      auto candidate=position+direction*approach_distance;
      auto query_origin=candidate+Vec3{0,0,2*content.player.height};
      if(std::getenv("HALO2_TU15_LIFT_LOWER_FLOOR"))
        query_origin.z=lower_floor.z+.5f*content.player.height;
      const auto floor=terrain.raycast(query_origin,{0,0,-1},floor_distance);
      if(!floor || floor->normal.z<=0)continue;
      candidate.z=floor->position.z+content.player.radius;
      if(terrain.overlaps(candidate,content.player.height,content.player.radius))continue;
      if(clear_approach) {
        const auto from=candidate+Vec3{0,0,.5f*content.player.height};
        const auto displacement=Vec3{position.x,position.y,from.z}-from;
        const float reach=length(displacement);
        if(reach>0)if(const auto hit=terrain.raycast(from,displacement/reach,reach);
            hit && hit->distance<reach-content.player.radius)continue;
      }
      spawn.position=candidate;found=true;break;
    }
    check(found,"Declared lift approach has no clear original cached BSP floor");
    spawn.facing=std::atan2(position.y-spawn.position.y,position.x-spawn.position.x);
  }
  return spawn;
}


struct SupportEntrance {
  std::vector<Spawn> spawns;
  std::uint32_t definition;
  Vec3 center,local_top;
  unsigned node{};
  std::optional<float> phase;
  bool narrow{};
  std::vector<Transform> authored_nodes;
};
SupportEntrance wheel_support_entrance(const Cache &cache,const LevelContent &content) {
  if(const auto selected=std::getenv("HALO2_TU15_SUPPORT_DEFINITION")) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag&t){return t.group=="mach" && t.name==selected;});
    check(tag!=cache.tags().end(),"Declared original carrier definition absent");
    const auto placed=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){return o.definition==tag->datum;});
    check(placed!=content.objects.end(),"Declared original carrier placement absent");
    const auto object=cache.load_source_tag_metadata(placed->definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
    const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
    const Bytes phmo(physics.bytes_at(physics.source_address(),physics.size()));
    const auto body_index=std::getenv("HALO2_TU15_SUPPORT_BODY_INPUT")?std::stoul(std::getenv("HALO2_TU15_SUPPORT_BODY_INPUT")):0u;
    check(body_index<phmo.u32(0x38),"Carrier input has no authored selected PHMO body");
    const Bytes body(physics.bytes_at(phmo.u32(0x3c)+std::uint32_t(body_index)*0x90u,0x90));
    const auto node=body.u16(0),shape=body.u16(0x38),index=body.u16(0x3a);
    Vec3 local_top;float side_offset=2*content.player.radius;
    if(shape==1) {
      check(index<phmo.u32(0x58),"Declared carrier input has no authored capsule");
      const Bytes pill(physics.bytes_at(phmo.u32(0x5c)+index*0x50u,0x50));
      const Vec3 bottom{pill.f32(0x30),pill.f32(0x34),pill.f32(0x38)},top{pill.f32(0x40),pill.f32(0x44),pill.f32(0x48)};
      const auto radius=pill.f32(0x2c);
      check(radius>3*content.player.radius && std::abs(top.z-bottom.z)<content.player.radius,"Carrier capsule has no admitted wide horizontal roof input");
      local_top=(bottom+top)*.5f+Vec3{0,0,radius};
    } else {
      check(shape==4 && index<phmo.u32(0x70),"Declared carrier input requires an original capsule or flat convex paddle");
      const Bytes hull(physics.bytes_at(phmo.u32(0x74)+index*0x100u,0x100));
      const Vec3 center{hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)},half{hull.f32(0x30),hull.f32(0x34),hull.f32(0x38)};
      check(half.x>content.player.radius && half.y>content.player.radius && half.z<content.player.radius,"Original carrier hull has no clear flat paddle input");
      local_top=center+Vec3{0,0,half.z};side_offset=0.f;
    }
    const auto placement=std::size_t(placed-content.objects.begin());
    check(placement<content.devices.size() && content.devices[placement] && content.devices[placement]->machine && content.devices[placement]->collision,
        "Original carrier input has no authored device hierarchy");
    const auto &device=*content.devices[placement];const auto basis=authored_basis(placed->rotation);
    const auto root=source_object_transform(placed->position,basis.forward,basis.up,placed->scale,false);
    const auto phase=std::getenv("HALO2_TU15_SUPPORT_PHASE_INPUT")?std::optional<float>{std::stof(std::getenv("HALO2_TU15_SUPPORT_PHASE_INPUT"))}:std::nullopt;
    check(!phase || (*phase>=0.f && *phase<=1.f),"Carrier declared phase must be a normalized original device input");
    const auto locals=source_machine_local_pose(device.collision->hierarchy,*device.machine,phase.value_or(device.placement.initial_position),0.f);
    const auto nodes=pose_model_locals_nodes_world(device.collision->hierarchy,root,locals);
    check(node<nodes.size(),"Carrier capsule has no authored initial model node");
    SupportEntrance result{{},tag->datum,nodes[node].position,local_top,node,phase,shape==4};
    result.authored_nodes=nodes;
    for(const float side:{-1.f,1.f}) {
      Transform point;point.position=result.local_top+Vec3{0,side*side_offset,content.player.radius};
      Spawn spawn;spawn.position=compose_transform(nodes[node],point).position;
      const auto aim=nodes[node].position-spawn.position;spawn.facing=std::atan2(aim.y,aim.x);result.spawns.push_back(spawn);
    }
    return result;
  }
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const Tag &t){return t.group=="mach" &&
      t.name=="scenarios\\objects\\multi\\zanzibar\\big_wheel\\big_wheel";});
  check(tag!=cache.tags().end(),"Original rotating wheel definition absent");
  const auto at=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &o){return o.definition==tag->datum;});
  check(at!=content.objects.end(),"Original rotating wheel placement absent");
  const auto object=cache.load_source_tag_metadata(at->definition);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
  const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
  const Bytes phmo(physics.bytes_at(physics.source_address(),physics.size()));
  check(phmo.u32(0x70)>0,"Original wheel has no authored hull0 paddle input");
  const Bytes hull(physics.bytes_at(phmo.u32(0x74),0x100));
  const Vec3 center{hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
  const Vec3 half{hull.f32(0x30),hull.f32(0x34),hull.f32(0x38)};
  check(half.x>3*content.player.radius && half.y>3*content.player.radius && half.z<content.player.radius,
      "Original wheel hull0 is not the admitted flat paddle input");
  const auto basis=authored_basis(at->rotation);
  const auto root=source_object_transform(at->position,basis.forward,basis.up,at->scale,false);
  SupportEntrance result{{},tag->datum,root.position,center+Vec3{0,0,half.z}};
  for(const float side:{-1.f,1.f}) {
    Transform point;point.position=result.local_top+Vec3{0,side*2*content.player.radius,content.player.radius};
    Spawn spawn;spawn.position=compose_transform(root,point).position;
    const auto target=root.position-spawn.position;spawn.facing=std::atan2(target.y,target.x);result.spawns.push_back(spawn);
  }
  return result;
}

struct ConveyorEntrance {Spawn spawn;CollisionSurfaceIdentity surface;};
ConveyorEntrance conveyor_entrance(const LevelContent &content) {
  const CollisionWorld terrain(content.collision);
  for(const auto &triangle:content.collision) {
    if(!(triangle.flags&0x20u) || triangle.source_normal.z<content.player.maximum_slope_cosine)continue;
    const auto center=(triangle.a+triangle.b+triangle.c)*(1.f/3.f);
    const auto normal=normalized(triangle.source_normal);bool clear=true;
    for(const auto edge:std::array{std::pair{triangle.a,triangle.b},std::pair{triangle.b,triangle.c},std::pair{triangle.c,triangle.a}})
      clear&=std::abs(dot(cross(edge.second-edge.first,center-edge.first),normal))/length(edge.second-edge.first)>content.player.radius;
    if(!clear)continue;
    const auto floor=terrain.raycast(center+Vec3{0,0,content.player.height},{0,0,-1},2*content.player.height);
    if(!floor || !(floor->surface_flags&0x20u) || floor->normal.z<content.player.maximum_slope_cosine)continue;
    Spawn spawn;spawn.position=floor->position+Vec3{0,0,content.player.radius};
    if(terrain.overlaps(spawn.position,content.player.height,content.player.radius))continue;
    return {spawn,floor->source};
  }
  throw FormatError("Original conveyor has no clear upward authored surface approach");
}
struct PropWatchEntrance {Spawn spawn;Vec3 target;std::set<unsigned> placements;};
PropWatchEntrance object_field_entrance(const Cache &cache,const LevelContent &content) {
  const auto name=std::getenv("HALO2_TU15_OBJECT_FIELD_TARGET");check(name,"Object field input requires its authored target definition");
  unsigned ordinal=std::getenv("HALO2_TU15_OBJECT_FIELD_INSTANCE")?std::stoul(std::getenv("HALO2_TU15_OBJECT_FIELD_INSTANCE")):0u;
  for(unsigned n=0;n<content.objects.size();++n) {
    const auto &placed=content.objects[n];const auto *tag=cache.find_tag(TagHandle{placed.definition});
    if(placed.type!=11 || !tag || tag->name!=name || ordinal--!=0)continue;
    const auto basis=authored_basis(placed.rotation);
    const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
    Transform offset;offset.position=placed.bounding_offset;auto radius=placed.bounding_radius*placed.scale;
    if(const auto selected=std::getenv("HALO2_TU15_OBJECT_FIELD_BODY_INPUT")) {
      const auto object=cache.load_source_tag_metadata(placed.definition);
      const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
      const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
      const Bytes phmo(physics.bytes_at(physics.source_address(),physics.size()));
      const auto index=std::stoul(selected);check(index<phmo.u32(0x38),"Declared moving body approach is outside original PHMO rows");
      const Bytes body(physics.bytes_at(phmo.u32(0x3c)+std::uint32_t(index)*0x90u,0x90));
      check(body.u16(0x1a)<4,"Interaction approach requires an authored dynamic body");
      offset.position={body.f32(8),body.f32(12),body.f32(16)};radius=body.f32(0x14)*placed.scale;
    }
    const auto target=compose_transform(root,offset).position;
    const CollisionWorld terrain(content.collision);
    check(radius>0,"Original physical target has no authored bounds");
    const unsigned approach=std::getenv("HALO2_TU15_OBJECT_FIELD_APPROACH")?std::stoul(std::getenv("HALO2_TU15_OBJECT_FIELD_APPROACH")):0u;
    check(approach<32,"Object approach must be one of the declared32 survey directions");
    for(float clearance:{1.f,2.f,3.f})for(unsigned candidate=0;candidate<32;++candidate) {
      const auto angle=(approach+candidate)%32;
      const float radians=6.283185307f*float(angle)/32.f;
      const auto direction=basis.forward*std::cos(radians)+basis.left*std::sin(radians);
      auto position=target+direction*(radius+clearance*content.player.height);
      const auto floor=terrain.raycast(position+Vec3{0,0,2*content.player.height},{0,0,-1},8*content.player.height);
      if(!floor || floor->normal.z<=0 || floor->position.z<placed.position.z-content.player.height)continue;position.z=floor->position.z+content.player.radius;
      if(terrain.overlaps(position,content.player.height,content.player.radius))continue;
      const auto eye=position+Vec3{0,0,content.player.camera_height};const auto delta=target-eye;
      if(const auto hit=terrain.raycast(eye,normalized(delta),length(delta));hit && hit->distance<length(delta)-content.player.radius)continue;
      Spawn spawn;spawn.position=position;spawn.facing=std::atan2(delta.y,delta.x);
      return {spawn,target,{n}};
    }
    throw FormatError("Original physical object has no clear BSP-grounded approach");
  }
  throw FormatError("Declared physical object has no authored placement");
}
PropWatchEntrance prop_watch_entrance(const Cache &cache,const LevelContent &content) {
  PropWatchEntrance result;std::optional<Vec3> origin;
  const std::string prop_name=std::getenv("HALO2_TU15_PROP_WATCH_NAME")?std::getenv("HALO2_TU15_PROP_WATCH_NAME"):"gas_container";
  for(unsigned n=0;n<content.objects.size();++n) {
    const auto &placed=content.objects[n];const auto *tag=cache.find_tag(TagHandle{placed.definition});
    if(placed.type!=11 || (placed.flags&0x100u) || !tag || tag->name.find(prop_name)==std::string::npos)continue;
    if(!origin)origin=placed.position;
    if(std::abs(placed.position.x-origin->x)<content.player.height)result.placements.insert(n);
  }
  check(origin.has_value(),"Original scripted prop spawn placement is absent");
  result.target=*origin;
  result.target=lift_entrance(cache,content).position;
  const CollisionWorld terrain(content.collision);
  for(float radius:{4.f,6.f,8.f})for(unsigned angle=0;angle<32;++angle) {
    const float radians=6.283185307f*float(angle)/32.f;
    const Vec3 direction{std::cos(radians),std::sin(radians),0};
    auto position=result.target+direction*radius;
    const auto floor=terrain.raycast(position+Vec3{0,0,content.player.height},{0,0,-1},8*content.player.height);
    if(!floor || floor->normal.z<=0)continue;
    position.z=floor->position.z+content.player.radius;
    if(terrain.overlaps(position,content.player.height,content.player.radius))continue;
    const auto eye=position+Vec3{0,0,content.player.camera_height};const auto delta=result.target-eye;
    if(const auto blocked=terrain.raycast(eye,normalized(delta),length(delta));blocked && length(blocked->position-eye)<length(delta)-.1f)continue;
    result.spawn.position=position;result.spawn.facing=std::atan2(delta.y,delta.x);return result;
  }
  throw FormatError("Original scripted prop has no visible cached-BSP observer floor");
}
struct ControlEntrance {Spawn spawn;Vec3 target;std::uint32_t definition;Vec3 gate;};
ControlEntrance control_entrance(const Cache &cache,const LevelContent &content) {
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const Tag &t) {
    return t.group=="ctrl" && t.name=="scenarios\\objects\\multi\\zanzibar\\gate_control\\house_gate_control";
  });
  check(tag!=cache.tags().end(),"Original gate-control definition absent");
  for(std::size_t i=0;i<content.objects.size();++i) {
    const auto &placed=content.objects[i];if(placed.definition!=tag->datum)continue;
    check(i<content.devices.size() && content.devices[i] && content.devices[i]->control,
        "Original gate control has no loaded control definition");
    const auto &d=*content.devices[i];check(bool(d.control_hierarchy),"Original gate control has no hierarchy");
    const auto basis=authored_basis(placed.rotation);
    const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
    const auto nodes=pose_model_nodes_world(*d.control_hierarchy,root);
    check(!nodes.empty(),"Original gate control has no actual model nodes");
    const auto sphere=source_shadow_sphere(d.control_bounds,nodes[0],root.scale);
    const auto markers=pose_model_markers(d.control_markers,nodes,d.control_permutations,{},1);
    const auto facing=markers.empty()?root.basis[0]:markers.front().world.basis[0];
    const auto target=markers.empty()?sphere.center:markers.front().world.position;
    Spawn spawn;spawn.position=sphere.center+facing*(sphere.radius+content.player.radius);
    const CollisionWorld terrain(content.collision);
    const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},8*content.player.height);
    check(floor.has_value(),"Original gate control approach has no cached BSP floor");
    spawn.position.z=floor->position.z+content.player.radius;
    spawn.facing=std::atan2(target.y-spawn.position.y,target.x-spawn.position.x);
    for(std::size_t gate=0;gate<content.objects.size();++gate) {
      if(gate>=content.devices.size() || !content.devices[gate] || !content.devices[gate]->machine ||
          d.placement.position_group<0 || content.devices[gate]->placement.position_group!=d.placement.position_group)continue;
      const auto &placed_gate=content.objects[gate];const auto gate_basis=authored_basis(placed_gate.rotation);
      const auto gate_root=source_object_transform(placed_gate.position,gate_basis.forward,gate_basis.up,placed_gate.scale,false);
      Transform offset;offset.position=placed_gate.bounding_offset;
      return {spawn,target,tag->datum,compose_transform(gate_root,offset).position};
    }
    throw FormatError("Original control has no authored linked machine");
  }
  throw FormatError("Original gate control placement absent");
}

struct VehicleEntrance {Spawn spawn;Vec3 target;std::size_t placement;unsigned seat;std::uint32_t flags;Transform root;};
VehicleEntrance vehicle_entrance(const Cache &cache,const LevelContent &content) {
  check(bool(content.unit_interactions),"Vehicle acceptance has no authored unit interactions");
  const auto &assets=*content.unit_interactions;
  const auto actor=assets.units.find(content.player.datum);
  check(actor!=assets.units.end(),"Vehicle acceptance has no authored player seat animations");
  const auto selected=std::getenv("HALO2_TU15_VEHICLE_NAME");
  const std::string vehicle_name=selected?selected:"warthog";
  std::optional<unsigned> requested_seat;
  if(const auto selected_seat=std::getenv("HALO2_TU15_VEHICLE_SEAT")) {
    const std::string text=selected_seat;std::size_t consumed{};const auto index=std::stoi(text,&consumed);
    check(consumed==text.size() && index>=0,"Invalid declared vehicle seat");requested_seat=unsigned(index);
  }
  std::optional<unsigned> requested_equipment;
  if(const auto selected_row=std::getenv("HALO2_TU15_VEHICLE_EQUIPMENT_INPUT")) {
    const std::string text=selected_row;std::size_t consumed{};const auto row=std::stoi(text,&consumed);
    check(consumed==text.size() && row>=0 && unsigned(row)<content.netgame_equipment.size(),
        "Declared vehicle equipment input is outside the authored rows");
    requested_equipment=unsigned(row);
  }
  check(vehicle_name=="warthog" || vehicle_name=="ghost" || vehicle_name=="banshee" ||
      vehicle_name=="scorpion" || vehicle_name=="wraith" || vehicle_name=="spectre" || vehicle_name=="spectre_secondary" || vehicle_name=="warthog_gauss" || vehicle_name=="h_turret_ap" || vehicle_name=="c_turret_ap",
      "Vehicle acceptance selection has no authored fixture");
  for(const auto &[row,placement]:assets.netgame_placements) {
    if(requested_equipment && row!=*requested_equipment)continue;
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag&t){return t.datum==object.definition;});
    const auto definition_name=vehicle_name=="warthog_gauss"?"warthog":
        vehicle_name=="spectre_secondary"?"spectre":vehicle_name;
    if(tag==cache.tags().end() || tag->name.find("\\"+definition_name+"\\")==std::string::npos)continue;
    if(vehicle_name=="h_turret_ap" || vehicle_name=="c_turret_ap") {
      const unsigned classification=std::getenv("HALO2_TU15_TURRET_CLASS")?std::stoul(std::getenv("HALO2_TU15_TURRET_CLASS")):6u;
      check(classification==6 || classification==7,"Fixed turret requires authored class6/7");
      if(content.netgame_equipment.at(row).classification!=int(classification))continue;
    }
    if(vehicle_name=="warthog_gauss")check(cache.string_id(object.variant)=="gauss",
        "Gauss fixture did not select the same named variant in its actual cache namespace");
    if(vehicle_name=="spectre_secondary")
      check(content.netgame_equipment.at(row).classification==2,"Secondary Spectre did not use its authored class2 row");
    const auto basis=authored_basis(object.rotation);
    auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    auto target_placement=placement;
    if(std::getenv("HALO2_TU15_VEHICLE_CHILD")) {
      const auto child=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &candidate){
        return candidate.model_attachment && candidate.model_attachment->parent==placement;
      });
      check(child!=content.objects.end(),"Selected fixture has no authored vehicle child");
      target_placement=std::size_t(child-content.objects.begin());
      const auto &attachment=*child->model_attachment;
      const auto &parent_definition=assets.units.at(object.definition),&child_definition=assets.units.at(child->definition);
      const auto parent_nodes=pose_model_nodes_world(parent_definition.hierarchy,root);
      const auto parent_markers=pose_model_markers(parent_definition.markers.at(attachment.parent_marker),
          parent_nodes,parent_definition.permutations,{},1);
      check(!parent_markers.empty(),"Selected child has no authored parent attachment marker");
      Transform local;
      if(attachment.child_marker) {
        const auto child_nodes=pose_model_nodes_world(child_definition.hierarchy,Transform{});
        const auto child_markers=pose_model_markers(child_definition.markers.at(attachment.child_marker),
            child_nodes,child_definition.permutations,{},1);
        if(!child_markers.empty())local=child_markers.front().world;
      }
      root=compose_transform(parent_markers.front().world,inverse_transform(local));
    }
    const auto &definition=assets.units.at(content.objects.at(target_placement).definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];
      const auto controlling_flag=(vehicle_name=="h_turret_ap" || vehicle_name=="c_turret_ap")?8u:4u;
      if(requested_seat?index!=*requested_seat:!(seat.flags&controlling_flag))continue;
      // The source admission/animation owners decide whether this seat needs
      // an entry clip. An absent generic clip cannot suppress a test approach.
      const auto marker=definition.markers.find(seat.marker),entry=definition.markers.find(seat.entry_marker);
      if(marker==definition.markers.end() || entry==definition.markers.end())continue;
      const auto seats=pose_model_markers(marker->second,nodes,definition.permutations,{},1);
      const auto entrances=pose_model_markers(entry->second,nodes,definition.permutations,{},1);
      if(seats.empty() || entrances.empty())continue;
      // This is an explicitly declared pre-tick test location, not an
      // inferred retail spawn or an injected post-step contact. Approach the
      // actual entry marker from its facing side, above the cached BSP floor.
      check(seat.radius>0,"Vehicle entry marker has no authored approach radius");
      Spawn spawn;spawn.position=entrances.front().world.position+
          entrances.front().world.basis[0]*(seat.radius*.5f);
      const CollisionWorld terrain(content.collision);
      const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},
          {0,0,-1},4*content.player.height);
      if(!floor)continue;
      spawn.position.z=floor->position.z+content.player.radius;
      const auto eye=spawn.position+Vec3{0,0,content.player.camera_height};
      const auto look=entrances.front().world.position-eye;
      spawn.facing=std::atan2(look.y,look.x);
      return {spawn,entrances.front().world.position,target_placement,index,seat.flags,root};
    }
  }
  throw FormatError("Vehicle acceptance found no authored "+vehicle_name+" selected seat entrance");
}
RayHit vehicle_wall_target(const VehicleEntrance &entry,const LevelContent &content) {
  const CollisionWorld terrain(content.collision);
  // Preserve the original approach preference, then survey the rest of the
  // authored horizontal directions. Heavy placements have walls at25..27.
  std::vector<int> directions{2,-2,4,-4,6,-6,8,-8,0,1,-1,3,-3,5,-5,7,-7,9,-9,10,-10,11,-11,12,-12,13,-13,14,-14,15,-15,16};
  if(const auto requested=std::getenv("HALO2_TU15_VEHICLE_WALL_DIRECTION_INPUT")) {
    const std::string text=requested;std::size_t used{};const auto offset=std::stoi(text,&used);
    check(used==text.size() && offset>=-16 && offset<=16,
        "Declared wall survey direction must be an integer in-16..16");
    directions={offset};
  }
  for(int offset:directions) {
    const float angle=float(offset)*6.283185307f/32.f;
    const auto direction=normalized(entry.root.basis[0]*std::cos(angle)+entry.root.basis[1]*std::sin(angle));
    const auto hit=terrain.raycast(entry.root.position+Vec3{0,0,.7f},direction,30.f);
    if(!hit || hit->distance<4.f || std::abs(hit->normal.z)>.35f)continue;
    const auto upper=terrain.raycast(entry.root.position+Vec3{0,0,1.2f},direction,30.f);
    if(!upper || std::hypot(upper->position.x-hit->position.x,upper->position.y-hit->position.y)>content.player.height)continue;
    return *hit;
  }
  throw FormatError("Selected original vehicle has no reachable tall cached BSP wall in the declared direction survey");
}
Json inventory(const MultiplayerPreparedWorld &world) {
  Json out;out["authored_vehicle_candidates"]=Json::array();out["scene_roots"]=Json::array();
  for(unsigned n=0;n<world.content.netgame_equipment.size();++n) {
    const auto &row=world.content.netgame_equipment[n];
    for(const auto &item:row.items) {
      const auto tag=std::find_if(world.cache->tags().begin(),world.cache->tags().end(),[&](const Tag&t){return t.datum==item.definition;});
      if(tag!=world.cache->tags().end() && tag->group=="vehi")
        out["authored_vehicle_candidates"].push_back({{"row",n},{"classification",row.classification},
            {"definition",tag->datum},{"name",tag->name},{"position",xyz(row.position)}});
    }
  }
  unsigned vehicles=0;
  for(const auto &root:world.match.simulation().scene().objects()) {
    const auto &placement=world.content.objects.at(root.placement);
    const auto tag=std::find_if(world.cache->tags().begin(),world.cache->tags().end(),[&](const Tag&t){return t.datum==placement.definition;});
    if(tag==world.cache->tags().end())continue;
    vehicles+=tag->group=="vehi";
    out["scene_roots"].push_back({{"handle",root.handle},{"placement",root.placement},{"definition",tag->datum},
        {"group",tag->group},{"name",tag->name},{"position",xyz(root.position)}});
  }
  out["live_scene_vehicle_roots"]=vehicles;
  const auto count=world.source_objects ? world.source_objects->physical_body_count() : std::nullopt;
  out["physical_body_count_available"]=bool(count);
  if(count)out["source_physical_body_count"]=*count;
  if(world.source_objects) {
    const auto native_frames=world.source_objects->tu15_world().native_physics_frame_activity();
    out["native_physics_frame_activity"]={{"frame_calls",native_frames.frame_calls},
        {"residual_calls",native_frames.residual_calls},{"world_integrations",native_frames.world_integrations},
        {"island_integrations",native_frames.island_integrations},{"native_force_calls",native_frames.native_force_calls},{"native_pose_calls",native_frames.native_pose_calls},{"native_deactivation_calls",native_frames.native_deactivation_calls},
        {"native_kinematic_pose_calls",native_frames.native_kinematic_pose_calls},
        {"native_fixed_pose_calls",native_frames.native_fixed_pose_calls},
        {"native_prescribed_force_calls",native_frames.native_prescribed_force_calls},
        {"native_body_policy_calls",native_frames.native_body_policy_calls},
        {"native_island_transition_calls",native_frames.native_island_transition_calls},
        {"native_array_growth_calls",native_frames.native_array_growth_calls},
        {"native_array_reserve_calls",native_frames.native_array_reserve_calls},
        {"native_array_pool_allocations",native_frames.native_array_pool_allocations},
        {"native_array_pool_releases",native_frames.native_array_pool_releases},
        {"native_world_collision_calls",native_frames.native_world_collision_calls},
        {"native_world_collision_listener_calls",native_frames.native_world_collision_listener_calls},
        {"native_island_collision_listener_calls",native_frames.native_island_collision_listener_calls},
        {"native_constraint_calls",native_frames.native_constraint_calls},
        {"native_dynamic_accumulator_calls",native_frames.native_dynamic_accumulator_calls},
        {"native_constraint_setup_calls",native_frames.native_constraint_setup_calls},
        {"native_island_collision_calls",native_frames.native_island_collision_calls},
        {"native_island_agent_flag_clear_calls",native_frames.native_island_agent_flag_clear_calls},
        {"native_island_agent_flag_set_calls",native_frames.native_island_agent_flag_set_calls},
        {"native_island_agent_process_calls",native_frames.native_island_agent_process_calls},
        {"native_island_agent_finish_calls",native_frames.native_island_agent_finish_calls},
        {"native_island_contact_minimum_calls",native_frames.native_island_contact_minimum_calls},
        {"native_island_contact_time_calls",native_frames.native_island_contact_time_calls},
        {"native_island_contact_buffers_constructed",native_frames.native_island_contact_buffers_constructed},
        {"native_island_contact_buffers_destroyed",native_frames.native_island_contact_buffers_destroyed},
        {"native_contact_advance_calls",native_frames.native_contact_advance_calls},
        {"native_contact_advance_motion_calls",native_frames.native_contact_advance_motion_calls},
        {"native_contact_advance_process_calls",native_frames.native_contact_advance_process_calls},
        {"native_contact_advance_single_iterations",native_frames.native_contact_advance_single_iterations},
        {"native_contact_advance_paired_iterations",native_frames.native_contact_advance_paired_iterations},
        {"native_contact_advance_negative_single_restores",native_frames.native_contact_advance_negative_single_restores},
        {"native_contact_advance_negative_paired_restores",native_frames.native_contact_advance_negative_paired_restores},
        {"native_motion_commit_calls",native_frames.native_motion_commit_calls},
        {"native_motion_commit_kinds",native_frames.native_motion_commit_kinds},
        {"native_broad_phase_calls",native_frames.native_broad_phase_calls},
        {"native_shape_bounds_box_calls",native_frames.native_shape_bounds_box_calls},
        {"native_shape_bounds_convex_calls",native_frames.native_shape_bounds_convex_calls},
        {"native_shape_bounds_transformed_calls",native_frames.native_shape_bounds_transformed_calls},
        {"native_shape_bounds_forward_calls",native_frames.native_shape_bounds_forward_calls},
        {"native_shape_bounds_motion_calls",native_frames.native_shape_bounds_motion_calls},
        {"native_shape_bounds_centered_box_calls",native_frames.native_shape_bounds_centered_box_calls},
        {"native_shape_bounds_centered_radius_calls",native_frames.native_shape_bounds_centered_radius_calls},
        {"shape_bounds_residual_calls",native_frames.shape_bounds_residual_calls},
        {"shape_bounds_residual_owners",native_frames.shape_bounds_residual_owners},
        {"named_residual_owner_calls",native_frames.residual_owner_calls},
        {"native_shape_bounds_endpoint_calls",native_frames.native_shape_bounds_endpoint_calls},
        {"native_shape_bounds_compound_calls",native_frames.native_shape_bounds_compound_calls},
        {"native_shape_bounds_compound_child_calls",native_frames.native_shape_bounds_compound_child_calls},
        {"native_solver_calls",native_frames.native_solver_calls},
        {"native_solver_initializations",native_frames.native_solver_initializations},
        {"native_solver_iterations",native_frames.native_solver_iterations},
        {"native_solver_primary_rows",native_frames.native_solver_primary_rows},
        {"native_solver_secondary_rows",native_frames.native_solver_secondary_rows},
        {"native_solver_exported_rows",native_frames.native_solver_exported_rows},
        {"native_solver_custom_refusals",native_frames.native_solver_custom_refusals},
        {"native_solver_custom_calls",native_frames.native_solver_custom_calls},
        {"native_solver_clock_calls",native_frames.native_solver_clock_calls},
        {"native_solver_custom_kinds",native_frames.native_solver_custom_kinds},
        {"native_constraint_producer_axis_calls",native_frames.native_constraint_producer_axis_calls},
        {"native_constraint_producer_scaled_calls",native_frames.native_constraint_producer_scaled_calls},
        {"native_constraint_producer_base_calls",native_frames.native_constraint_producer_base_calls},
        {"native_constraint_producer_base_rows",native_frames.native_constraint_producer_base_rows},
        {"native_constraint_producer_property_lookups",native_frames.native_constraint_producer_property_lookups},
        {"native_constraint_producer_emissions",native_frames.native_constraint_producer_emissions},
        {"native_constraint_producer_completed_emissions",native_frames.native_constraint_producer_completed_emissions},
        {"native_vehicle_rate_acceleration_calls",native_frames.native_vehicle_rate_acceleration_calls},
        {"native_vehicle_rate_approach_calls",native_frames.native_vehicle_rate_approach_calls},
        {"native_vehicle_rate_cursor_calls",native_frames.native_vehicle_rate_cursor_calls},
        {"native_vehicle_rate_refusals",native_frames.native_vehicle_rate_refusals},
        {"native_schema_emitter_calls",native_frames.native_schema_emitter_calls},
        {"native_schema_emitter_completed_calls",native_frames.native_schema_emitter_completed_calls},
        {"native_schema_emitter_cursor_calls",native_frames.native_schema_emitter_cursor_calls},
        {"native_schema_emitter_refusals",native_frames.native_schema_emitter_refusals},
        {"native_schema_emitter_caller_kinds",native_frames.native_schema_emitter_caller_kinds},
        {"native_schema_emitter_refusal_kinds",native_frames.native_schema_emitter_refusal_kinds},
        {"world_residual_calls",native_frames.world_residual_calls},{"native_only_closure",false}};
    out["actual_source_objects"]=Json::array();
    out["selected_phantom_source_owners"]=Json::array();
    for(const auto &object:world.source_objects->live_objects()) {
      const auto tag=world.cache->query_tag({.handle=TagHandle{object.definition}});
      check(tag!=nullptr,"Live source inventory has no actual cache definition");
      if(const auto selected=std::getenv("HALO2_TU15_LIFT_DEFINITION");selected && tag->name==selected) {
        const auto &memory=world.source_objects->memory();
        const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
        const auto body=world.source_objects->object_body(object.object);
        const auto physical=word(body+0xb4u);
        Json owner{{"object",object.object},{"physical_component",physical},
            {"body_bytes",hex_bytes(memory.view(body,0x12c))}};
        if(physical!=null_datum) {
          const auto pool=word(0x52be5cu),index=physical&0xffffu;
          check(word(pool+0x24u)==0xa0u && index<word(pool+0x20u),"Selected phantom owner lost its original component pool");
          const auto record=word(pool+0x44u)+index*0xa0u;
          owner["component_bytes"]=hex_bytes(memory.view(record,0xa0));
        }
        out["selected_phantom_source_owners"].push_back(std::move(owner));
      }
      const auto regions=world.source_objects->object_model_regions(object.object);
      out["actual_source_objects"].push_back({{"object",object.object},{"definition",object.definition},
          {"type",object.type},{"parent",object.parent},{"name",tag->name},
          {"variant",regions.selection.variant_index},{"permutations",regions.selection.permutations}});
    }
  }
  return out;
}
struct Host {
  std::string join_address;unsigned port{},host_slot{};
  Json initial_inventory;
  NativeTu15PhysicsFrameExecutionStats native_frames;
  Json impulses=Json::array();
  Json scene_lifecycle=Json::array();
  Json script_stages=Json::array();
  std::atomic_bool start_device_script{};
  std::atomic_bool resume_support{};
  std::atomic_bool support_contact_observed{};
  std::atomic_bool support_client_ready{};
  std::atomic_bool capture_world_requested{};
  bool vehicle_death_observed{};
  bool source_authority_restored{};
  std::atomic_uint32_t vehicle_attack_target{null_datum},turret_damage_target{null_datum};std::atomic_bool vehicle_attack_dead{};
  Json support_samples=Json::array();
  Json vehicle_vitality_samples=Json::array();
  Json script_status;
  std::uint32_t first_prop_generation=null_datum;
  std::shared_ptr<const LevelContent> content;
  std::mutex mutex;std::vector<MultiplayerState> history;std::exception_ptr error;std::jthread worker;
  Host(MultiplayerOptions options,std::optional<std::uint32_t> recreate_placement={},
      std::optional<ScriptProgram> script_input={}) {
    std::promise<void> startup;auto ready=startup.get_future();
    worker=std::jthread([this,options=std::move(options),startup=std::move(startup),recreate_placement,script_input=std::move(script_input)](std::stop_token stop)mutable {
      bool started=false;try {
      // Construction, all source calls and retirement share one executor.
      const bool scripted=script_input.has_value();
      const auto source_scene_spawns=std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT")?
          std::exchange(options.native_spawn_inputs,std::vector<Spawn>{}):std::vector<Spawn>{};
      MultiplayerPreparedWorld world(options,std::move(script_input));
      if(!source_scene_spawns.empty()) {
        check(world.match.source_engine_owned() && world.source_objects,
            "Vehicle destruction acceptance requires the complete original multiplayer engine");
        world.bind_source_local_views(world.local_slots);
      }
      world.match.configure_replicated_first_person(world.cache->load_first_person_binding(world.rules.weapon,2));
      std::array<Controls,multiplayer_capacity> controls{};
      std::set<std::uint32_t> scene_positioned_units;
      std::set<std::string> observed_scene_action_targets;
      const auto save_scene_evidence=[&] {
        std::ofstream file(options.source_object_savegame_directory.parent_path()/"authority-source-scene-inputs.json");
        file<<scene_lifecycle.dump(2)<<'\n';
        check(bool(file),"Could not retain original scene-input/action-target observations");
      };
      const auto place_source_scene_units=[&] {
        if(source_scene_spawns.empty())return false;
        bool moved=false;const auto state=world.match.state();
        for(unsigned slot=0;slot<state.players.size();++slot) {
          const auto &player=state.players[slot];
          if(!player.connected || player.dead || player.source_unit==null_datum ||
              !scene_positioned_units.insert(player.source_unit).second)continue;
          if(slot==0)continue; // The authority's own player keeps its authored start.
          check(player.source_parent==null_datum,"New source player already has a parent before scene placement");
          auto position=source_scene_spawns.front().position;
          if(slot>=2) {
            // Separate real colliding bipeds. The watcher receives a declared
            // supported approach rather than the driver's identical point.
            const CollisionWorld terrain(world.content.collision);bool supported=false;
            for(unsigned direction=0;direction<32 && !supported;++direction) {
              const float angle=float(direction)*.19634954084936207f;
              const auto candidate=position+Vec3{std::cos(angle),std::sin(angle),0}*3.f;
              const auto floor=terrain.raycast(candidate+Vec3{0,0,2},{0,0,-1},4);
              if(!floor || floor->normal.z<world.content.player.maximum_slope_cosine)continue;
              const auto placed=floor->position+Vec3{0,0,world.content.player.radius};
              if(terrain.overlaps(placed,world.content.player.height,world.content.player.radius))continue;
              const auto delta=position-placed;
              if(terrain.raycast(placed+Vec3{0,0,world.content.player.camera_height},
                  normalized(delta),length(delta)))continue;
              position=placed;supported=true;
            }
            check(supported,"No supported separated watcher position for vehicle destruction scene");
          }
          world.source_objects->tu15_world().set_object_position(player.source_unit,position);
          controls[slot].yaw=player.yaw;controls[slot].pitch=player.pitch;
          scene_lifecycle.push_back({{"phase","declared_position_after_original_player_birth"},
              {"tick",state.tick},{"slot",slot},{"unit",player.source_unit},{"requested",xyz(position)},
              {"original_source_engine",true},{"damage_or_physics_result_supplied",false}});
          moved=true;
        }
        if(moved)save_scene_evidence();
        return moved;
      };
      std::ofstream support_field_trace;
      struct SupportStoreBinding {
        MultiplayerObjectRuntime *owner{};
        ~SupportStoreBinding(){if(owner)owner->observe_physics_stores({});}
      } support_store_binding;
      if(std::getenv("HALO2_TU15_SUPPORT_FIELD_STORES")) {
        check(std::getenv("HALO2_TU15_SUPPORT_INPUT") && !std::getenv("HALO2_TU15_PHANTOM_TRACE"),
            "Support field tracing requires its own read-only store observer");
        support_field_trace.open(options.source_object_savegame_directory.parent_path()/"authority-support-field-stores.jsonl");
        check(bool(support_field_trace),"Cannot retain original biped field stores");
        support_store_binding.owner=world.source_objects.get();
        world.source_objects->observe_physics_stores_read_only([&](std::uint32_t instruction,const SourcePhysicsContactRecordStore &store) {
          const auto &player=world.match.player(1);
          if(!player.connected || player.source_current_object==null_datum)return;
          const auto &memory=world.source_objects->memory();
          const SourceRawDatumPoolServices readback{.read=[&](auto p,auto n){return memory.view(p,n);}};
          const auto header=source_raw_datum_lookup(world.source_objects->source_object_pool(),player.source_current_object,readback);
          if(!header || Bytes(memory.view(header+6,2)).u16(0)<0x44c)return;
          const auto body=Bytes(memory.view(header+8,4)).u32(0);
          if(!body)return;
          for(auto offset:{0x394u,0x40cu,0x448u}) {
            const auto address=body+offset;
            if(std::uint64_t(store.address)+store.written.size()<=address || store.address>=address+4u)continue;
            support_field_trace<<Json{{"tick",world.match.simulation().ticks()},{"unit",player.source_current_object},
                {"body",body},{"offset",offset},{"actual_instruction",instruction},{"record_instruction",store.instruction},
                {"address",store.address},{"before",hex_bytes(store.before)},{"written",hex_bytes(store.written)}}.dump()<<'\n';
            support_field_trace.flush();check(bool(support_field_trace),"Cannot write original biped field store evidence");
          }
        });
      }
      std::optional<std::uint32_t> support_object;bool support_resumed=false;
      unsigned support_node{};
      bool support_start_barrier{};
      const auto support_power=[&](float value) {
        check(support_object.has_value(),"Original support command lost its current source object");
        world.source_objects->script_device("device_set_power",{ScriptValue{54,*support_object,{}},ScriptValue::numeric(value,6)});
      };
      if(std::getenv("HALO2_TU15_SUPPORT_INPUT")) {
        const auto entrance=wheel_support_entrance(*world.cache,world.content);support_node=entrance.node;
        support_start_barrier=std::getenv("HALO2_TU15_SUPPORT_DEFINITION");
        for(const auto &object:world.source_objects->live_objects()) {
          const auto *tag=world.cache->find_tag(TagHandle{object.definition});
          if(tag && tag->datum==entrance.definition)support_object=object.object;
        }
        check(support_object.has_value(),"Original support fixture has no current wheel body");
        if(entrance.phase) {
          support_start_barrier=true;
          const auto initial=*support_object;
          // Let the original startup scripts own their ordinary retirement
          // and runtime birth. Stage the actual current generation only.
          world.match.step({});support_object.reset();
          for(unsigned tick=0;tick<900 && !support_object;++tick) {
            for(const auto &object:world.source_objects->live_objects())if(object.definition==entrance.definition)support_object=object.object;
            if(!support_object)world.match.step({});
          }
          check(support_object.has_value(),"Original map scripts produced no current carrier generation for the declared approach");
          support_power(0);
          world.source_objects->script_device("device_set_position_immediate",{ScriptValue{54,*support_object,{}},ScriptValue::numeric(*entrance.phase,6)});
          scene_lifecycle.push_back({{"phase","declared_original_carrier_device_input"},{"old",initial},{"new",*support_object},{"position_input",*entrance.phase},{"tick",world.match.simulation().ticks()}});
          world.match.step({});
        } else {support_power(0);world.match.step({});}
      }
      if(std::getenv("HALO2_TU15_LIFT_CREATE_INPUT")) {
        const auto selected=std::getenv("HALO2_TU15_LIFT_DEFINITION");check(selected,"Field creation requires a declared definition");
        unsigned ordinal=std::getenv("HALO2_TU15_LIFT_INSTANCE")?std::stoul(std::getenv("HALO2_TU15_LIFT_INSTANCE")):0u;
        std::optional<unsigned> placement;
        for(unsigned n=0;n<world.content.objects.size();++n) {
          const auto *tag=world.cache->find_tag(TagHandle{world.content.objects[n].definition});
          if(tag && tag->name==selected && ordinal--==0){placement=n;break;}
        }
        check(placement.has_value(),"Declared field birth has no authored placement");
        check(std::none_of(world.match.simulation().scene().objects().begin(),world.match.simulation().scene().objects().end(),
            [&](const auto &o){return o.handle!=null_datum && o.placement==*placement;}),"Declared initially absent field already exists");
        const auto fresh=world.match.create_world_object(*placement);
        check(fresh!=null_datum,"Original field birth did not return its actual full generation");
        world.match.step({});
        scene_lifecycle.push_back({{"phase","declared_initially_absent_birth_before_join"},{"new",fresh},
            {"placement",*placement},{"tick",world.match.simulation().ticks()}});
      }
      if(recreate_placement) {
        const auto &objects=world.match.simulation().scene().objects();
        const auto prior=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.handle!=null_datum && o.placement==*recreate_placement;});
        check(prior!=objects.end(),"Late-join fixture has no authored source prop");
        const auto old=prior->handle;world.match.destroy_world_object(old);
        first_prop_generation=world.match.create_world_object(*recreate_placement);
        check(first_prop_generation!=old,"Actual source recreation reused the retired full datum");
        world.match.step({}); // Complete the actual new source body's first world pass.
        scene_lifecycle.push_back({{"phase","before_join"},{"old",old},{"new",first_prop_generation},
            {"tick",world.match.simulation().ticks()}});
      }
      std::ofstream impulse_trace;
      struct BodyImpulseObservationScope {
        MultiplayerObjectRuntime *runtime{};
        ~BodyImpulseObservationScope(){if(runtime)runtime->observe_body_impulses({});}
      } body_impulse_scope;
      if(world.source_objects && (std::getenv("HALO2_TU15_PROP_GRENADE") ||
          std::getenv("HALO2_TU15_TURRET_DAMAGE_INPUT") || std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT"))) {
        impulse_trace.open(options.source_object_savegame_directory.parent_path()/"authority-point-impulses.jsonl");
        check(bool(impulse_trace),"Could not retain actual authority point impulses");
        world.source_objects->observe_body_impulses([this,&impulse_trace](const auto &event){
          auto row=impulse_json(event);impulse_trace<<row.dump()<<'\n';impulse_trace.flush();
          check(bool(impulse_trace),"Could not retain completed authority point impulse");
          impulses.push_back(std::move(row));
        });
        body_impulse_scope.runtime=world.source_objects.get();
      }
      auto effective_variant=options.variant;
      if(options.starting_weapon)effective_variant.values.set(89,int(options.starting_weapon));
      if(place_source_scene_units()) {
        world.match.step(controls);world.source_objects->advance_effect_frame(tick_seconds);
      }
      LanHost transport(world.match,0,multiplayer_fingerprint(options.map,world.cache->shared_directory(),0,world.network_starting_weapon_identity(),options.gameplay_xbe),effective_variant);
      initial_inventory=inventory(world);content=std::make_shared<LevelContent>(world.content);
      initial_inventory["original_source_engine_owned"]=world.match.source_engine_owned();
      port=transport.port();join_address=transport.join_address();host_slot=world.local_slots.front();
      started=true;startup.set_value();
      std::set<std::uint32_t> observed_source_generations;
      std::ofstream machine_trace;
      if(scripted || support_object || std::getenv("HALO2_TU15_MACHINE_TRACE")) {
        machine_trace.open(options.source_object_savegame_directory.parent_path()/"authority-machine-state.jsonl");
        check(bool(machine_trace),"Could not retain actual source/native device comparison");
      }
      std::ofstream source_birth_trace;
      if(std::getenv("HALO2_TU15_PROP_GRENADE")) {
        source_birth_trace.open(options.source_object_savegame_directory.parent_path()/"authority-source-object-births.jsonl");
        check(bool(source_birth_trace),"Could not retain actual source object births");
        for(const auto &object:world.source_objects->live_objects())observed_source_generations.insert(object.object);
      }
      std::ofstream binding_trace;
      if(std::getenv("HALO2_TU15_SEAT_DRIVER")) {
        binding_trace.open(options.source_object_savegame_directory.parent_path()/"authority-player-bindings.jsonl");
        check(bool(binding_trace),"Could not retain actual authority player bindings");
      }
      std::ofstream phantom_trace,velocity_trace;std::optional<std::uint32_t> phantom_component;
      std::unique_ptr<test::LivePhantomOriginalCaptureTu15> original_force_capture;
      if(std::getenv("HALO2_TU15_PHANTOM_ORIGINAL_CAPTURE")) {
        check(std::getenv("HALO2_TU15_PHANTOM_TRACE"),"Original force capture requires its actual phase observer");
        original_force_capture=std::make_unique<test::LivePhantomOriginalCaptureTu15>(
            options.source_object_savegame_directory.parent_path()/"original-force-capture");
      }
      std::optional<test::LiveNativePhysicsFrameCaptureTu15> native_frame_capture;
      std::optional<NativeTu15PhysicsFrameObserverBinding> native_frame_observer;
      if(std::getenv("HALO2_TU15_CAPTURE_NATIVE_FRAME_SEMANTICS")) {
        native_frame_capture.emplace(options.source_object_savegame_directory.parent_path()/"original-native-frame-input");
        std::optional<std::uint32_t> requested_capture_tick;
        if(const auto requested=std::getenv("HALO2_TU15_NATIVE_FRAME_CAPTURE_TICK")) {
          const std::string text=requested;std::size_t used{};const auto value=std::stoul(text,&used);
          check(used==text.size()&&value<=9000,"Native frame capture tick must be a declared0..9000 authority tick");
          requested_capture_tick=std::uint32_t(value);
          std::ofstream trigger(options.source_object_savegame_directory.parent_path()/"native-frame-capture-trigger.json");
          trigger<<Json{{"authority_tick",*requested_capture_tick},{"scope","Read-only native-frame observation trigger; no gameplay input or physics result"}}.dump(2)<<'\n';
          check(bool(trigger),"Could not retain declared native frame capture trigger");
        }
        native_frame_observer.emplace(world.source_objects->memory(),[&,requested_capture_tick](const auto &event) {
          const auto tick=world.match.simulation().ticks();
          if(requested_capture_tick?tick>=*requested_capture_tick:(capture_world_requested.load() || tick>=180))
            native_frame_capture->observe(event,world.source_objects->memory());
        });
      }
      std::map<std::uint32_t,std::unique_ptr<test::LivePhysicsEntryCaptureTu15>> physics_entry_captures;
      std::map<std::uint32_t,std::unique_ptr<test::LivePhysicsEntryCaptureTu15>> post_impulse_captures;
      if(std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT"))for(auto entry:{0xb8f40u,0x1cc8d0u})
        post_impulse_captures.emplace(entry,std::make_unique<test::LivePhysicsEntryCaptureTu15>(
            options.source_object_savegame_directory.parent_path()/(entry==0xb8f40u?"original-post-impulse-object-capture":"original-post-impulse-world-capture")));
      std::map<std::uint32_t,std::unique_ptr<test::LivePhysicsEntryCaptureTu15>> post_death_captures;
      if(std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT"))for(auto entry:{0xb8f40u,0x1cc8d0u})
        post_death_captures.emplace(entry,std::make_unique<test::LivePhysicsEntryCaptureTu15>(
            options.source_object_savegame_directory.parent_path()/(entry==0xb8f40u?"original-post-death-object-capture":"original-post-death-world-capture")));
      std::map<std::uint32_t,std::unique_ptr<test::LivePhysicsEntryCaptureTu15>> post_restore_captures;
      if(std::getenv("HALO2_TU15_AUTHORITY_RESTORE"))for(auto entry:{0xb8f40u,0x1cc8d0u})
        post_restore_captures.emplace(entry,std::make_unique<test::LivePhysicsEntryCaptureTu15>(
            options.source_object_savegame_directory.parent_path()/(entry==0xb8f40u?"original-post-restore-object-capture":"original-post-restore-world-capture")));
      std::optional<SourcePhysicsEntryObserverBinding> physics_entry_observer;
      const auto add_capture=[&](std::uint32_t entry) {
        const auto name=entry==0x14fec0u?"original-player-update-capture":entry==0x1cc8d0u?"original-world-capture":entry==0xb8f40u?"original-object-update-capture":entry==0x2ee660u?"original-broadphase-capture":entry==0x3205b0u?"original-contact-time-capture":"original-query-capture";
        physics_entry_captures.emplace(entry,std::make_unique<test::LivePhysicsEntryCaptureTu15>(
            options.source_object_savegame_directory.parent_path()/name));
      };
      if(std::getenv("HALO2_TU15_VEHICLE_PHASE_CAPTURE")) {
        add_capture(0xb8f40u);add_capture(0x1cc8d0u);
        if(std::getenv("HALO2_TU15_TURRET_WEAPON_TRACE"))add_capture(0x14fec0u);
      } else if(std::getenv("HALO2_TU15_BROADPHASE_ORIGINAL_CAPTURE")) {
        const auto selected=std::getenv("HALO2_TU15_QUERY_CAPTURE_ENTRY");
        const auto entry=selected?std::uint32_t(std::stoul(selected,nullptr,0)):0x2ee660u;
        check(entry==0x2f9d70u || entry==0x2ee660u || entry==0x2f1930u || entry==0x2fc5d0u || entry==0x1cc8d0u || entry==0xb8f40u || entry==0x3205b0u,"Select an authenticated live physics entry");
        add_capture(entry);
      }
      if(!physics_entry_captures.empty())physics_entry_observer.emplace(world.source_objects->memory(),[&](bool entering,std::uint32_t entry,const auto &state) {
        const auto found=physics_entry_captures.find(entry);
        if(source_authority_restored)if(const auto post=post_restore_captures.find(entry);post!=post_restore_captures.end())
          post->second->observe(entering,state,world.source_objects->memory(),entry);
        // Every export is read-only. The additional pair observes actual
        // source impulse consequences; neither capture provides game state.
        if(found!=physics_entry_captures.end() &&
            ((entry!=0x1cc8d0u && entry!=0xb8f40u && entry!=0x14fec0u) || capture_world_requested.load() || !impulses.empty()))
          found->second->observe(entering,state,world.source_objects->memory(),entry);
        if(!impulses.empty())if(const auto post=post_impulse_captures.find(entry);post!=post_impulse_captures.end())
          post->second->observe(entering,state,world.source_objects->memory(),entry);
        if(vehicle_death_observed)if(const auto post=post_death_captures.find(entry);post!=post_death_captures.end())
          post->second->observe(entering,state,world.source_objects->memory(),entry);
      });
      std::ofstream turret_weapon_trace,turret_effect_trace;
      if(std::getenv("HALO2_TU15_TURRET_WEAPON_TRACE")) {
        turret_weapon_trace.open(options.source_object_savegame_directory.parent_path()/"authority-turret-weapons.jsonl");
        turret_effect_trace.open(options.source_object_savegame_directory.parent_path()/"authority-turret-effect-calls.jsonl");
        check(bool(turret_weapon_trace) && bool(turret_effect_trace),"Cannot retain original turret weapon inputs");
        world.source_objects->observe_weapon_effect_phase([&](bool entering,const auto &a,const auto &memory,const auto &,const auto &) {
          const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
          Json row{{"entering",entering},{"tick",word(word(0x51df54u)+8u)},{"eax",a.gp[0]},{"ecx",a.gp[1]}};
          if(entering) {
            const auto body=word(word(word(0x4ecbd0u)+0x44u)+(a.gp[1]&0xffffu)*0xcu+8u);
            row["weapon"]=a.gp[1];row["unit"]=word(body+0x14u);row["effect"]=a.gp[0];
          }
          turret_effect_trace<<row.dump()<<'\n';turret_effect_trace.flush();
          check(bool(turret_effect_trace),"Cannot retain original turret effect phase");
        });
      }
      bool contrail_bank_captured{},nonempty_contrail_bank_captured{};
      std::map<std::uint32_t,std::pair<unsigned,std::string>> velocity_targets;
      std::ofstream contrail_store_trace;
      struct PhysicsObservationScope {
        MultiplayerObjectRuntime *runtime{};
        ~PhysicsObservationScope() {
          if(!runtime)return;
          // Detach while captured streams/targets still exist. Original
          // world teardown executes more stores after those locals retire.
          runtime->observe_body_velocity_inputs({});runtime->observe_physics_stores({});
          runtime->observe_phantom_phase({});
          if(std::getenv("HALO2_TU15_TURRET_WEAPON_TRACE"))runtime->observe_weapon_effect_phase({});
        }
      } physics_observation_scope{(std::getenv("HALO2_TU15_PHANTOM_TRACE") || std::getenv("HALO2_TU15_CONTRAIL_STORES"))?world.source_objects.get():nullptr};
      if(std::getenv("HALO2_TU15_CONTRAIL_STORES")) {
        check(!std::getenv("HALO2_TU15_PHANTOM_TRACE"),"Contrail diagnostic requires its independent store observer");
        contrail_store_trace.open(options.source_object_savegame_directory.parent_path()/"authority-contrail-stores.jsonl");
        check(bool(contrail_store_trace),"Cannot retain original attachment allocation stores");
        world.source_objects->observe_physics_stores_read_only([&](std::uint32_t,const SourcePhysicsContactRecordStore &store) {
          if(store.instruction!=0xc0e54u && store.instruction!=0x17e97eu && store.instruction!=0x17e9d6u &&
              store.instruction!=0xc0f25u && store.instruction!=0x16e71cu)return;
          const auto &memory=world.source_objects->memory();const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
          const auto pool=word(0x4f7214u);const auto tag=word(word(0x4f0414u)+0xf4au*16u+8u);
          contrail_store_trace<<Json{{"tick",word(word(0x51df54u)+8u)},{"instruction",store.instruction},
              {"address",store.address},{"before",hex_bytes(store.before)},{"written",hex_bytes(store.written)},
              {"contrail_pool",hex_bytes(memory.view(pool,0x4c))},{"human_bitmap",word(tag+0x34u)}}.dump()<<'\n';
          contrail_store_trace.flush();check(bool(contrail_store_trace),"Cannot retain original attachment store");
        });
      }
      if(std::getenv("HALO2_TU15_PHANTOM_TRACE")) {
        phantom_trace.open(options.source_object_savegame_directory.parent_path()/"authority-phantom-phase.jsonl");
        check(bool(phantom_trace),"Could not retain actual source phantom phase");
        velocity_trace.open(options.source_object_savegame_directory.parent_path()/"authority-velocity-stores.jsonl");
        check(bool(velocity_trace),"Could not retain actual source velocity stores");
        world.source_objects->observe_physics_stores_read_only([&](std::uint32_t actual_instruction,const SourcePhysicsContactRecordStore &store) {
          for(const auto &[address,owner]:velocity_targets) {
            if(store.address>=address+16u || std::uint64_t(store.address)+store.written.size()<=address)continue;
            const auto &memory=world.source_objects->memory();
            const auto clock=Bytes(memory.view(0x51df54u,4)).u32(0);
            Json record{{"tick",Bytes(memory.view(clock+8u,4)).u32(0)},
                {"slot",owner.first},{"kind",owner.second},{"target",address},
                {"instruction",store.instruction},{"actual_instruction",actual_instruction},{"address",store.address},
                {"before",hex_bytes(store.before)},{"written",hex_bytes(store.written)}};
            if(source_velocity_observation_stack && owner.second=="motion_velocity") {
              const auto sp=source_velocity_observation_stack;
              record["source_stack"]=sp;record["source_return"]=Bytes(memory.view(sp,4)).u32(0);
              record["source_stack_bytes"]=hex_bytes(memory.view(sp,0x180));
              record["source_registers"]=source_velocity_observation_registers;
              const auto input=source_velocity_observation_registers[3];
              record["caller_vector_address"]=input;
              if(input)record["caller_vector_bytes"]=hex_bytes(memory.view(input,12));
            }
            velocity_trace<<record.dump()<<'\n';
          }
          check(bool(velocity_trace),"Could not retain source velocity store");
        });
        world.source_objects->observe_body_velocity_inputs([&](std::uint32_t caller,const auto &abi,const SourceRuntimeMemory &memory) {
          const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
          const auto component=word(abi.gp[4]+4u),object=word(component+8u);
          for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
            const auto &p=world.match.player(slot);
            if(!p.connected || p.source_current_object!=object)continue;
            velocity_trace<<Json{{"tick",word(word(0x51df54u)+8u)},{"slot",slot},
                {"kind","velocity_input"},{"caller",caller},{"component",component},{"object",object},
                {"body_index",abi.gp[1]},{"vector",hex_bytes(memory.view(abi.gp[0],12))}}.dump()<<'\n';
          }
        });
        world.source_objects->observe_phantom_phase([&](bool entering,const auto &abi,const SourceRuntimeMemory &memory) {
          const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
          if(entering)phantom_component=word(abi.gp[4]+4u);
          check(phantom_component.has_value(),"Missing actual phantom entry component");
          const auto component=*phantom_component;
          const auto object=word(component+8u);
          const auto body=world.source_objects->object_body(object);
          const auto definition=word(body);
          const auto *tag=world.cache->find_tag(TagHandle{definition});
          const auto selected=std::getenv("HALO2_TU15_LIFT_DEFINITION");
          if(!tag || (selected?tag->name!=selected:tag->name.find("lift")==std::string::npos))return;
          Json members=Json::array();std::uint32_t capture_target=world.match.player(1).source_current_object;
          const auto requested_member=std::getenv("HALO2_TU15_OBJECT_FIELD_TARGET");
          if(requested_member) {
            unsigned ordinal=std::getenv("HALO2_TU15_OBJECT_FIELD_INSTANCE")?std::stoul(std::getenv("HALO2_TU15_OBJECT_FIELD_INSTANCE")):0u;
            capture_target=null_datum;
            for(const auto &root:world.match.simulation().scene().objects()) {
              const auto *target=world.cache->find_tag(TagHandle{world.content.objects.at(root.placement).definition});
              if(target && target->name==requested_member && ordinal--==0){capture_target=root.handle;break;}
            }
            check(capture_target!=null_datum,"Declared original force capture lost its current authored target");
          }
          if(std::getenv("HALO2_TU15_PHANTOM_OBJECT_TRACE")) {
            const auto header=word(component+0x94u);
            if(header) {
              const auto rows=word(header),count=word(header+4u);check(count<=4096,"Original force membership extent is invalid");
              for(unsigned n=0;n<count;++n) {
                const auto row=memory.view(rows+n*8u,8);const auto other=Bytes(row).u32(4);
                Json member{{"object",other},{"row",hex_bytes(row)}};
                try {
                  const auto target=world.source_objects->object_body(other);const auto physical=word(target+0xb4u);
                  const auto *tag=world.cache->find_tag(TagHandle{word(target)});
                  member["definition"]=tag?tag->name:"";
                  const auto object_pool=word(0x4ecbd0u);
                  member["type"]=std::to_integer<unsigned>(memory.view(word(object_pool+0x44u)+(other&65535u)*12u+3u,1)[0]);
                  member["body_bytes"]=hex_bytes(memory.view(target,0x12cu));
                  const auto pool=word(0x52be5cu),stride=word(pool+0x24u);
                  if(physical!=null_datum && (physical&65535u)<word(pool+0x20u)) {
                    const auto address=word(pool+0x44u)+(physical&65535u)*stride;
                    const auto index=std::to_integer<unsigned>(row[1])<128?std::to_integer<unsigned>(row[1]):0u;
                    if(index<word(address+0x74u)) {
                      const auto rigid=word(word(address+0x70u)+index*0x60u+0x40u),motion=word(rigid+0x3cu);
                      member["motion_bytes"]=hex_bytes(memory.view(motion,512));member["motion_address"]=motion;
                      if(!requested_member && std::getenv("HALO2_TU15_PHANTOM_CAPTURE_ANY_MEMBER") && capture_target==world.match.player(1).source_current_object &&
                          member["type"]!=0)capture_target=other;
                    }
                  }
                }catch(const FormatError &error){member["unavailable_before_original_member_cleanup"]=error.what();}
                members.push_back(std::move(member));
              }
            }
          }
          if(original_force_capture)original_force_capture->observe(entering,abi,memory,component,capture_target);
          Json units=Json::array();
          for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
            const auto &p=world.match.player(slot);if(!p.connected || p.source_current_object==null_datum)continue;
            const auto address=world.source_objects->object_body(p.source_current_object);
            const auto physical=word(address+0xb4u),pool=word(0x52be5cu);
            const auto object_pool=word(0x4ecbd0u);
            check(word(object_pool+0x24u)==0xcu,"Unexpected original object header stride in trace");
            const auto raw_body=word(word(object_pool+0x44u)+(p.source_current_object&0xffffu)*0xcu+8u);
            velocity_targets.insert_or_assign(raw_body+0x88u,std::pair{slot,std::string("object_velocity")});
            Json unit{{"slot",slot},{"object",p.source_current_object},
                {"body_bytes",hex_bytes(memory.view(address,0x400))},
                {"physical_component",physical}};
            if(physical!=null_datum && (physical&0xffffu)<word(pool+0x20u)) {
              const auto physical_address=word(pool+0x44u)+(physical&0xffffu)*word(pool+0x24u);
              unit["component_bytes"]=hex_bytes(memory.view(physical_address,0xa0));
              const auto list=word(physical_address+0x70u),count=word(physical_address+0x74u);
              if(list && count>0 && count<=64) {
                const auto rigid=word(list+0x40u),motion=word(rigid+0x3cu);
                if(motion) {
                  unit["motion_bytes"]=hex_bytes(memory.view(motion,512));
                  velocity_targets.insert_or_assign(motion+0x40u,std::pair{slot,std::string("motion_velocity")});
                }
              }
            }
            units.push_back(std::move(unit));
          }
          phantom_trace<<Json{{"entering",entering},{"tick",word(word(0x51df54u)+8u)},
              {"component",component},{"object",object},{"definition",tag->name},
              {"component_bytes",hex_bytes(memory.view(component,0xa0))},
              {"source_player_count",word(word(0x4f54f0u))},
              {"physics_constants",hex_bytes(memory.view(word(0x52be68u),0x1c))},{"players",units},{"members",members}}.dump()<<'\n';
          phantom_trace.flush();check(bool(phantom_trace),"Could not retain source phantom observation");
        });
      }
      std::ofstream collision_trace;
      if(std::getenv("HALO2_TU15_LIFT_COLLISION_TRACE")) {
        collision_trace.open(options.source_object_savegame_directory.parent_path()/"authority-lift-structure-queries.jsonl");
        check(bool(collision_trace),"Could not retain original lift structure queries");
      }
      std::optional<std::uint32_t> joined_tick;bool recreated_after_blast=false;
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        transport.receive(controls);
        if(support_start_barrier && !support_client_ready.load()) {
          // Keep the finite fixture at its completed staged source frame
          // while the real client prepares graphics and joins. Network
          // admission/publication still run. All actual physics resumes
          // when that client can display and deliver ordinary controls.
          {std::lock_guard lock(mutex);history.push_back(world.match.state());}
          transport.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);continue;
        }
        if(resume_support.exchange(false)) {support_power(1);support_resumed=true;}
        if(start_device_script.exchange(false)) {
          check(scripted,"Device script start has no declared input");
          world.match.run_script("device_source_sequence");
        }
        // HELLO can publish a new participant at the existing world clock,
        // before the next physics frame. Retain that actual sent-state phase.
        const auto before=world.match.state();
        if(!source_scene_spawns.empty() && scene_lifecycle.size()<40u)
          for(unsigned slot=0;slot<before.players.size();++slot) {
            const auto &p=before.players[slot];
            if(!p.connected || p.source_unit==null_datum)continue;
            const auto target=world.source_objects->tu15_world().player_action_target(
                p.source_player,p.source_unit).value_or(SourcePlayerActionTarget{});
            const auto &published=p.action_prompt.target;
            const auto key=std::to_string(slot)+":"+std::to_string(target.action)+":"+
                std::to_string(target.index)+":"+std::to_string(target.object)+":"+
                std::to_string(published.action)+":"+std::to_string(published.object);
            if(!observed_scene_action_targets.insert(key).second)continue;
            scene_lifecycle.push_back({{"phase","observed_original_action_target"},{"tick",before.tick},
                {"slot",slot},{"unit",p.source_unit},
                {"original_target",{{"action",target.action},{"index",target.index},{"object",target.object}}},
                {"published_target",{{"action",published.action},{"index",published.index},{"object",published.object}}},
                {"position",xyz(p.position)}, {"yaw",p.yaw},{"pitch",p.pitch}});
            save_scene_evidence();
          }
        if(std::getenv("HALO2_TU15_AUTHORITY_RESTORE") && !source_authority_restored &&
            std::ranges::any_of(before.players,[](const auto &p){return p.connected && p.source_parent!=null_datum && p.source_seat>=0;}) &&
            std::ranges::any_of(before.world_vehicles,[](const auto &v){return length(v.lighting_velocity)>.1f;})) {
          // An executor-owned transaction restores completed authority only.
          // No physics input, bank byte, contact, camera, or solved pose changes.
          constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x53}};
          SourceMatchAuthority authority(world.match,*world.source_objects);
          const auto saved=authority.checkpoint(binding);
          auto prepared=authority.prepare(saved,binding);
          check(authority.commit(std::move(prepared)),"Live occupied-vehicle authority restore failed");
          const auto restored=authority.checkpoint(binding);
          check(saved.world.world.bank==restored.world.world.bank &&
              saved.match.simulation.ticks==restored.match.simulation.ticks &&
              saved.match.source_players==restored.match.source_players,
              "Live occupied-vehicle restore changed source memory, clock or player binding");
          const auto after=world.match.state();
          for(unsigned slot=0;slot<before.players.size();++slot) {
            const auto &a=before.players[slot],&b=after.players[slot];
            check(a.source_player==b.source_player && a.source_unit==b.source_unit &&
                a.source_parent==b.source_parent && a.source_seat==b.source_seat &&
                !std::memcmp(&a.position,&b.position,sizeof(Vec3)) &&
                !std::memcmp(&a.velocity,&b.velocity,sizeof(Vec3)),
                "Live occupied-vehicle restore changed its published participant state");
          }
          scene_lifecycle.push_back({{"phase","source_authority_restore_while_vehicle_occupied"},
              {"tick",before.tick},{"full_bank_equal",true},{"source_player_bindings_equal",true},
              {"source_static_agent_vtable",Bytes(world.source_objects->memory().view(0x48cdc4u,4)).u32(0)}});
          source_authority_restored=true;
        }
        if(machine_trace.is_open()) {
          const auto &memory=world.source_objects->memory();
          const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
          const auto group_pool=word(0x4ecbf8u);
          const auto group=[&](std::uint32_t datum)->Json {
            if(datum==null_datum)return nullptr;
            check(word(group_pool+0x24u)==12u && (datum&0xffffu)<word(group_pool+0x20u),
                "Actual original device group has an invalid pool extent");
            const auto row=word(group_pool+0x44u)+(datum&0xffffu)*12u;
            check(Bytes(memory.view(row,2)).u16(0)==(datum>>16u),"Actual original device group lost its full generation");
            return {{"datum",datum},{"bytes",hex_bytes(memory.view(row,12))}};
          };
          Json owners=Json::array();
          for(const auto &entry:before.devices.owners) {
            const auto &owner=entry.state;const auto body=world.source_objects->object_body(owner.handle);
            const auto *definition=world.cache->find_tag(TagHandle{entry.definition});
            check(definition,"Actual device snapshot has no source definition");
            const auto &motion=owner.motion;
            owners.push_back({{"object",owner.handle},{"placement",owner.placement},{"definition",definition->name},
                {"native_motion_bits",std::array{std::bit_cast<std::uint32_t>(motion.position),
                    std::bit_cast<std::uint32_t>(motion.position_velocity),std::bit_cast<std::uint32_t>(motion.power),
                    std::bit_cast<std::uint32_t>(motion.power_velocity)}},
                {"source_motion_bits",std::array{word(body+0x140u),word(body+0x144u),word(body+0x134u),word(body+0x138u)}},
                {"native_flags",motion.flags},{"source_flags",word(body+0x12cu)},
                {"native_delay",motion.delay_ticks},{"source_delay",std::int16_t(Bytes(memory.view(body+0x148u,2)).u16(0))},
                {"power_group",group(word(body+0x130u))},{"position_group",group(word(body+0x13cu))},
                {"source_device_bytes",hex_bytes(memory.view(body+0x12cu,0xa0u))}});
          }
          machine_trace<<Json{{"tick",before.tick},{"phase","completed prior source/native tick"},
              {"source_tick",word(word(0x51df54u)+8u)},{"owners",owners}}.dump()<<'\n';
          machine_trace.flush();check(bool(machine_trace),"Could not retain actual device comparison");
        }

        if(binding_trace.is_open() && std::count_if(before.players.begin(),before.players.end(),[](const auto &p){return p.connected;})>=3) {
          Json bindings=Json::array();
          for(unsigned slot=0;slot<before.players.size();++slot) {
            const auto &p=before.players[slot];if(!p.connected || p.source_unit==null_datum)continue;
            const auto body=world.source_objects->object_body(p.source_unit);
            const auto read=[&](unsigned offset,unsigned size=4) {
              std::array<std::byte,4> bytes{};world.source_objects->memory().read(body+offset,std::span(bytes).first(size));
              std::uint32_t value{};for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);return value;
            };
            const auto &c=controls[slot];
            bindings.push_back({{"slot",slot},{"player",p.source_player},{"unit",p.source_unit},
                {"unit_actor",read(0x12c)},{"unit_player",read(0x13c)},{"unit_team",std::int16_t(read(0x138,2))},
                {"unit_parent",read(0x14)},{"unit_seat",std::int16_t(read(0x1fc,2))},
                {"native_parent",p.source_parent},{"native_seat",p.source_seat},
                {"native_configuration_team",p.source_configuration?Json((*p.source_configuration)[0x7c]):Json()},
                {"pickup",c.pickup},{"pickup_ms",c.pickup_milliseconds},{"forward",c.forward},
                {"delivered_vehicle_intent",c.vehicle_interaction_target?Json{{"action",c.vehicle_interaction_target->action},
                  {"index",c.vehicle_interaction_target->index},{"object",c.vehicle_interaction_target->object}}:Json()},
                {"prompt_object",p.action_prompt.target.object},{"prompt_seat",p.action_prompt.target.index},
                {"prompt_action",p.action_prompt.target.action},
                {"delivered_source_command",hex_bytes(world.source_objects->memory().view(0x6002800cu+slot*0x5cu,0x5c))}});
          }
          binding_trace<<Json{{"tick",before.tick},{"players",bindings}}.dump()<<'\n';binding_trace.flush();
          check(bool(binding_trace),"Could not retain live source player-binding readback");
        }
        if(recreate_placement) {
          if(!joined_tick && std::count_if(before.players.begin(),before.players.end(),[](const auto &p){return p.connected;})>=3)
            joined_tick=before.tick;
          if(joined_tick && !recreated_after_blast && before.tick>=*joined_tick+140u &&
              std::none_of(before.source_scene_roots.begin(),before.source_scene_roots.end(),
                  [&](const auto &root){return root.placement==*recreate_placement;})) {
            const auto fresh=world.match.create_world_object(*recreate_placement);
            scene_lifecycle.push_back({{"phase","after_blast"},{"old",first_prop_generation},{"new",fresh},{"tick",before.tick}});
            recreated_after_blast=true;
          }
        }
        if(collision_trace.is_open() && before.tick%15u==0) {
          const float height=world.content.player.height;
          for(unsigned slot=1;slot<before.players.size();++slot) {
            const auto &player=before.players[slot];
            if(!player.connected || player.source_unit==null_datum)continue;
            const auto body=world.source_objects->object_body(player.source_unit);
            const auto &memory=world.source_objects->memory();
            const auto pose=memory.view(body+0x64u,0x40);
            const std::vector<std::byte> untouched(pose.begin(),pose.end());
            const auto origin=player.position+Vec3{0,0,.5f*height};
            Json rays=Json::array();
            for(const auto &[name,delta]:std::array<std::pair<const char *,Vec3>,3>{{
                {"up",{0,0,2*height}},{"down",{0,0,-4*height}},
                {"forward",direction(controls[slot].yaw)*2*height}}}) {
              // Original16C620, with the actual collider-controller terrain
              // and instanced-terrain flags from2C32's native owner. These
              // diagnostic queries supply no collision/motion outcome.
              const auto hit=world.source_objects->particle_collision(0x800005u,origin,delta);
              Json ray{{"name",name},{"origin",xyz(origin)},{"displacement",xyz(delta)}};
              if(hit)ray["hit"]={{"fraction",hit->fraction},{"position",xyz(hit->position)},
                  {"normal",xyz(hit->normal)},{"type",hit->type},{"material",hit->material}};
              else ray["hit"]=nullptr;
              rays.push_back(std::move(ray));
            }
            check(std::ranges::equal(untouched,memory.view(body+0x64u,0x40)),
                "Original diagnostic structure queries changed the actual player pose/velocity");
            collision_trace<<Json{{"tick",before.tick},{"slot",slot},{"unit",player.source_unit},
                {"position",xyz(player.position)},{"rays",rays}}.dump()<<'\n';
          }
          collision_trace.flush();check(bool(collision_trace),"Could not retain original structure query results");
        }
        {std::lock_guard lock(mutex);history.push_back(before);}
        const auto source_tick_before=world.match.simulation().ticks();
        world.match.step(controls);
        // Use the ordinary dedicated host's complete post-tick frame owner.
        // Game ticks create contrail points with their original state=-1;
        //13A9B0's17EAD0 phase must initialize/advance them before publication.
        // Omitting this phase left real points permanently invisible while
        // the test's rendered client correctly consumed their source state.
        if(world.source_objects && world.source_objects->owns_physics())
          world.source_objects->advance_effect_frame(
              world.match.simulation().ticks()==source_tick_before+1u?tick_seconds:0.f);
        if(place_source_scene_units()) {
          // Publish only after the next genuine source frame has solved and
          // synchronized the declared position. No stale Match/source pair
          // leaves this boundary, and no clock or solved pose is assigned.
          next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);continue;
        }
        if(turret_weapon_trace.is_open() && vehicle_attack_target.load()!=null_datum) {
          const auto target=vehicle_attack_target.load();const auto &memory=world.source_objects->memory();
          const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
          const auto body=world.source_objects->object_body(target);
          Json row{{"tick",world.match.state().tick},{"vehicle",target},{"unit_bytes",hex_bytes(memory.view(body+0x12cu,0x118u))},{"weapons",Json::array()},{"commands",Json::array()}};
          for(const auto &object:world.source_objects->live_objects())if(object.type==2 && object.parent==target) {
            const auto weapon=world.source_objects->object_body(object.object);
            row["weapons"].push_back({{"object",object.object},{"definition",object.definition},{"body_bytes",hex_bytes(memory.view(weapon,0x200u))}});
          }
          for(unsigned n=0;n<controls.size();++n)if(world.match.player(n).connected)
            row["commands"].push_back({{"slot",n},{"fire",controls[n].fire},{"fire_secondary",controls[n].fire_secondary}});
          turret_weapon_trace<<row.dump()<<'\n';turret_weapon_trace.flush();check(bool(turret_weapon_trace),"Cannot retain actual turret weapon states");
        }
        if(std::getenv("HALO2_TU15_TURRET_DAMAGE_INPUT") && turret_damage_target.load()!=null_datum) {
          const auto target=turret_damage_target.load();const auto vital=world.source_objects->object_vitality(target);
          vehicle_vitality_samples.push_back({{"tick",world.match.state().tick},{"object",target},{"body",vital.body},{"shield",vital.shield}});
        }
        if(std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT")) {
          const auto state=world.match.state();
          for(const auto &vehicle:state.world_vehicles) {
            const auto vital=world.source_objects->object_vitality(vehicle.handle);
            const auto body=world.source_objects->object_body(vehicle.handle);
            const auto flags=std::to_integer<unsigned>(world.source_objects->memory().view(body+0x10au,1)[0]);
            vehicle_death_observed|=vital.body<=0.f && bool(flags&4u);
            if(vehicle.handle==vehicle_attack_target.load() && vital.body<=0.f && (flags&4u))vehicle_attack_dead.store(true);
            vehicle_vitality_samples.push_back({{"tick",state.tick},{"object",vehicle.handle},{"placement",vehicle.placement},
                {"body",vital.body},{"shield",vital.shield},{"original_damage_flags",flags}});
          }
        }
        if(support_object && world.match.player(1).connected) {
          const auto &player=world.match.player(1);const auto &memory=world.source_objects->memory();
          const auto live=world.source_objects->live_objects();
          const auto exists=[&](std::uint32_t handle){return std::any_of(live.begin(),live.end(),[&](const auto &o){return o.object==handle;});};
          const auto support_state=world.match.state();
          const bool released=!exists(player.source_current_object) && player.source_current_object==null_datum &&
              support_state.players[1].dead && support_state.players[1].source_unit==null_datum;
          if(!exists(*support_object) || (!exists(player.source_current_object) && !released)) {
            const auto state=world.match.state();
            const Json identity{{"tick",state.tick},{"carrier",*support_object},{"carrier_live",exists(*support_object)},
                {"current_object",player.source_current_object},{"current_live",exists(player.source_current_object)},
                {"unit",state.players[1].source_unit},{"dead",state.players[1].dead}};
            std::ofstream(options.source_object_savegame_directory.parent_path()/"support-identity-failure.json")<<identity.dump(2)<<'\n';
            throw FormatError("Original support diagnostic lost identity: "+identity.dump());
          }
          if(released) {
            support_samples.push_back({{"tick",support_state.tick},{"unit",null_datum},{"wheel",*support_object},
                {"released_player_body",true},{"dead",true}});
          } else {
          const auto nodes=world.source_objects->world_nodes(*support_object);check(support_node<nodes.size(),"Original carrier lost its selected WORLD node");
          const auto &node=nodes[support_node];
          const auto unit=world.source_objects->object_body(player.source_current_object);
          const auto flags=Bytes(memory.view(unit+0x348u,2)).u16(0);
          const auto state=world.match.state();
          check(state.players[1].native_grounded==((flags&1u)==0),"Rotating support grounded readback differs from original biped");
          const auto wheel_body=world.source_objects->object_body(*support_object);
          const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
          const auto bodies=[&](auto object_body) {
            Json rows=Json::array();const auto component=word(object_body+0xb4u);
            if(component==null_datum)return rows;
            const auto pool=word(0x52be5cu),row=word(pool+0x44u)+(component&65535u)*word(pool+0x24u);
            const auto count=word(row+0x74u),list=word(row+0x70u);check(count<=64,"Support diagnostic has an invalid body extent");
            for(unsigned i=0;i<count;++i) {
              const auto rigid=word(list+i*0x60u+0x40u),motion=word(rigid+0x3cu);
              const Bytes velocity(memory.view(motion+0x40u,32));
              rows.push_back({{"index",i},{"rigid",rigid},{"motion",motion},{"motion_bytes",hex_bytes(memory.view(motion,0x100))},
                  {"linear",Json::array({velocity.f32(0),velocity.f32(4),velocity.f32(8),velocity.f32(12)})},
                  {"angular",Json::array({velocity.f32(16),velocity.f32(20),velocity.f32(24),velocity.f32(28)})}});
            }
            return rows;
          };
          Json contact_readback;
          if(std::getenv("HALO2_TU15_SUPPORT_CONTACT_BYTES")) {
            const SourceRawDatumPoolServices readback{.read=[&](auto p,auto n){return memory.view(p,n);}};
            const auto header=source_raw_datum_lookup(world.source_objects->source_object_pool(),player.source_current_object,readback);
            check(header!=0,"Support readback lost its actual full biped header");
            const auto extent=Bytes(memory.view(header+6,2)).u16(0);
            check(extent>=0x34a && extent<=0x7fff,"Support readback has an invalid constructed body extent");
            contact_readback={{"unit_body",world.source_objects->source_object_body(player.source_current_object)},
                {"native_unit_body",unit.value},{"unit_extent",extent},{"unit_bytes",hex_bytes(memory.view(unit,extent))}};
            const auto component=word(unit+0xb4u);
            contact_readback["component"]=component;
            if(component!=null_datum) {
              const auto pool=word(0x52be5cu),row=source_raw_datum_lookup(pool,component,readback);
              check(row!=0,"Support readback lost its full physics component");
              const auto stride=word(pool+0x24u);
              check(stride>=0x78 && stride<=65536,"Support readback has an invalid physical component stride");
              contact_readback["component_row"]=row;
              contact_readback["component_bytes"]=hex_bytes(memory.view(row,stride));
            }
          }
          if(support_resumed)capture_world_requested.store(true);
          const auto carrier_bodies=bodies(wheel_body);
          Json matrix_words=Json::array();
          for(unsigned i=0;i<13;++i)matrix_words.push_back(word(unit+0x410u+i*4u));
          // Original 1FA1A0 stores owner and PHYS body index at controller
          // +5C/+20. Its +24 matrix comes from original 1D8B90, not a guessed
          // authored-node association. This is read-only post-step state.
          Json support_controller{{"owner",word(unit+0x448u)},
              {"body_index",std::bit_cast<std::int32_t>(word(unit+0x40cu))},
              {"body_count",carrier_bodies.size()},{"matrix_words",std::move(matrix_words)}};
          if(support_controller.at("owner").get<std::uint32_t>()==*support_object && state.players[1].native_grounded)
            support_contact_observed.store(true);
          support_samples.push_back({{"tick",state.tick},{"unit",player.source_current_object},{"wheel",*support_object},
              {"resumed",support_resumed},{"position",xyz(state.players[1].position)},{"velocity",xyz(state.players[1].velocity)},
              {"carrier_bodies",carrier_bodies},{"player_bodies",bodies(unit)},
              {"support_controller",std::move(support_controller)},
              {"delivered_controls",{{"forward",controls[1].forward},{"right",controls[1].right},{"jump",controls[1].jump}}},
              {"contact_readback",std::move(contact_readback)},
              {"grounded",state.players[1].native_grounded},{"source_support_flags",flags},
              {"wheel_position_bits",Bytes(memory.view(wheel_body+0x140u,4)).u32(0)},
              {"wheel_power_bits",Bytes(memory.view(wheel_body+0x134u,4)).u32(0)},
              {"node_scale",node.scale},{"node_position",xyz(node.position)},
              {"node_basis",Json::array({xyz(node.basis[0]),xyz(node.basis[1]),xyz(node.basis[2])})}});
          }
        }
        if(scripted) {
          const auto &vm=world.match.simulation().scripts();
          check(vm.diagnostics().empty(),"Visible source device script faulted");
          const auto &globals=vm.globals();const auto stage=globals.at(0).bits;
          if(stage && (script_stages.empty() || script_stages.back().at("stage")!=stage)) {
            Json bits=Json::array();for(const auto &value:globals)bits.push_back(value.bits);
            Json owners=Json::array();const auto state=world.match.state();
            const auto &memory=world.source_objects->memory();
            const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
            for(const auto &entry:state.devices.owners) {
              const auto body=world.source_objects->object_body(entry.state.handle);
              const auto &motion=entry.state.motion;
              const std::array native{std::bit_cast<std::uint32_t>(motion.position),
                  std::bit_cast<std::uint32_t>(motion.position_velocity),std::bit_cast<std::uint32_t>(motion.power),
                  std::bit_cast<std::uint32_t>(motion.power_velocity)};
              const std::array source{word(body+0x140u),word(body+0x144u),word(body+0x134u),word(body+0x138u)};
              check(native==source,"Visible scripted device differs from its original body");
              owners.push_back({{"object",entry.state.handle},{"native",native},{"source",source}});
            }
            script_stages.push_back({{"stage",stage},{"tick",state.tick},{"globals",bits},{"owners",owners}});
          }
        }
        if(source_birth_trace.is_open())for(const auto &object:world.source_objects->live_objects())
          if(observed_source_generations.insert(object.object).second) {
            const auto tag=world.cache->query_tag({.handle=TagHandle{object.definition}});
            check(tag!=nullptr,"Source-created object has no resident definition");
            const auto root=world.source_objects->object_transform(object.object);
            const auto regions=world.source_objects->object_model_regions(object.object);
            source_birth_trace<<Json{{"tick",world.match.simulation().ticks()},{"object",object.object},
                {"definition",object.definition},{"type",object.type},{"parent",object.parent},
                {"name",tag->name},{"position",xyz(root.position)},
                {"variant",regions.selection.variant_index},{"permutations",regions.selection.permutations}}.dump()<<'\n';
            source_birth_trace.flush();check(bool(source_birth_trace),"Could not retain completed source birth readback");
          }
        if(std::getenv("HALO2_TU15_CONTRAIL_CAPTURE") && !nonempty_contrail_bank_captured) {
          const auto &memory=world.source_objects->memory();
          const auto word=[&](auto p,unsigned n=4){const Bytes b(memory.view(p,n));return n==2?b.u16(0):b.u32(0);};
          const auto pool=word(0x4f7214u);bool chain_ready{};
          for(unsigned n=0;n<word(pool+0x38u);++n)if(word(word(pool+0x48u)+n/32u*4u)&(1u<<(n%32u))) {
            const auto row=word(pool+0x44u)+n*72u;
            for(unsigned c=0;c<4;++c)if(word(row+0x30u+c*2u,2)>=2) {
              const auto points=word(0x4f7210u);auto datum=word(row+0x38u+c*4u);
              for(unsigned q=0;q<word(row+0x30u+c*2u,2) && datum!=null_datum;++q) {
                const auto point=word(points+0x44u)+(datum&65535u)*56u;
                chain_ready|=std::int8_t(std::to_integer<unsigned>(memory.view(point+3u,1).front()))>=0;
                datum=word(point+0x34u);
              }
            }
          }
          const auto objects=world.source_objects->live_objects();
          const auto projectile_alive=std::any_of(objects.begin(),objects.end(),[](const auto &o){return o.type==5;});
          if(chain_ready || (projectile_alive && !contrail_bank_captured)) {
            const auto directory=options.source_object_savegame_directory.parent_path()/(chain_ready?"original-contrail-geometry-input":"original-contrail-input");
            std::filesystem::create_directories(directory);
            Json record{{"tick",world.match.simulation().ticks()},{"regions",Json::array()},
                {"reference","English TU1.5 actual live source bank; read-only diagnostic"}};
            memory.visit_committed([&](auto address,auto bytes) {
              const auto file="bank-"+std::to_string(address)+".bin";
              std::ofstream bank(directory/file,std::ios::binary);bank.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
              check(bool(bank),"Cannot retain actual contrail source bank");
              record["regions"].push_back({{"address",address},{"size",bytes.size()},{"file",file}});
            });
            std::ofstream file(directory/"capture.json");file<<record.dump(2)<<'\n';
            check(bool(file),"Cannot retain contrail source capture manifest");contrail_bank_captured=true;nonempty_contrail_bank_captured=chain_ready;
          }
        }
        {std::lock_guard lock(mutex);history.push_back(world.match.state());}
        transport.broadcast();
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
      native_frames=world.source_objects->tu15_world().native_physics_frame_activity();
      Json objects=Json::array();
      for(const auto &object:world.match.simulation().scene().objects())if(object.handle!=null_datum) {
        const auto *tag=world.cache->find_tag(TagHandle{world.content.objects.at(object.placement).definition});
        objects.push_back({{"object",object.handle},{"placement",object.placement},{"name_index",world.content.objects.at(object.placement).name},
            {"definition",tag?tag->name:""},{"created_tick",object.created_tick},{"position",xyz(object.position)}});
        if(std::getenv("HALO2_TU15_PHANTOM_OBJECT_TRACE") && tag && tag->group=="bloc" && world.content.objects.at(object.placement).name>=0) {
          const auto &memory=world.source_objects->memory();const auto body=world.source_objects->object_body(object.handle);
          const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
          auto &entry=objects.back();entry["source_body_bytes"]=hex_bytes(memory.view(body,0x12cu));
          entry["source_position"]=xyz(world.source_objects->object_transform(object.handle).position);
          const auto physical=word(body+0xb4u),pool=word(0x52be5cu);
          if(physical!=null_datum && (physical&65535u)<word(pool+0x20u)) {
            const auto component=word(pool+0x44u)+(physical&65535u)*word(pool+0x24u);
            entry["physical_component_bytes"]=hex_bytes(memory.view(component,0xa0));entry["physical_component"]=physical;
            const auto count=word(component+0x74u);check(count<=64,"Original object body bank exceeds diagnostic bound");
            entry["rigid_bodies"]=Json::array();
            for(unsigned n=0;n<count;++n) {
              const auto rigid=word(word(component+0x70u)+n*0x60u+0x40u),motion=word(rigid+0x3cu);
              Json row{{"index",n},{"rigid",rigid},{"rigid_bytes",hex_bytes(memory.view(rigid,0x80u))}};
              if(motion)row["motion_bytes"]=hex_bytes(memory.view(motion,512u));
              entry["rigid_bodies"].push_back(std::move(row));
            }
          }
        }
      }
      script_status={{"tick",world.match.simulation().ticks()},{"running",world.match.simulation().scripts().running()},
          {"diagnostics",world.match.simulation().scripts().diagnostics()},{"live_scene_objects",objects}};
    }catch(...){
      try {throw;}catch(const std::exception &e){std::cerr<<"acceptance_host_error="<<e.what()<<std::endl;}
      catch(...){std::cerr<<"acceptance_host_error=non-standard exception"<<std::endl;}
      if(!started)startup.set_exception(std::current_exception());
      else {std::lock_guard lock(mutex);error=std::current_exception();}
    }});
    ready.get();
  }
  void finish(){worker.request_stop();if(worker.joinable())worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) try {
  const auto native_rates_before=source_vehicle_native_rate_activity();
  check(argc==7,"Supply MCC_MAP MCC_MAPS native-mcc MCC_ROOT OUTPUT_DIR movement|lift|vehicle|objects|lan-movement|lan-lift|lan-vehicle|lan-objects");
  check(std::string_view(argv[3])=="native-mcc","Live physics acceptance requires the prepared MCC cache; Xbox executables and Vista UI are offline references");
  const std::string scenario=argv[6];const bool lan=scenario.starts_with("lan-"),
      lift=scenario=="lift"||scenario=="lan-lift",vehicle=scenario=="vehicle"||scenario=="lan-vehicle",objects=scenario=="objects"||scenario=="lan-objects";
  const bool player_rearm_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_PLAYER_REARM_INPUT");
  std::optional<unsigned> rearm_first_press,rearm_second_press;
  std::optional<std::uint32_t> rearm_last_tick;
  bool rearm_first_released{},rearm_second_released{},rearm_previous_grounded{};
  unsigned rearm_held_launches{},rearm_second_launches{},rearm_landed_ticks{},rearm_held_commands{},rearm_missing_held_commands{};
  const auto vehicle_selection=std::getenv("HALO2_TU15_VEHICLE_NAME");
  const std::string_view vehicle_name=vehicle_selection?vehicle_selection:"warthog";
  const bool fixed_turret_test=vehicle && (vehicle_name=="h_turret_ap" || vehicle_name=="c_turret_ap");
  const bool turret_damage_test=fixed_turret_test && std::getenv("HALO2_TU15_TURRET_DAMAGE_INPUT");
  check(!turret_damage_test || vehicle_name=="h_turret_ap","Turret damage input selects the original human gun");
  const bool accompanied_driver=vehicle && std::getenv("HALO2_TU15_SEAT_DRIVER");
  std::optional<float> takeoff_pitch_input;
  if(const auto pitch=std::getenv("HALO2_TU15_VEHICLE_TAKEOFF_PITCH_INPUT")) {
    takeoff_pitch_input=std::stof(pitch);
    check(vehicle && vehicle_name=="banshee" && std::isfinite(*takeoff_pitch_input) &&
        *takeoff_pitch_input>=0.f && *takeoff_pitch_input<=.6f,
        "Declared takeoff look requires Banshee and a finite0..0.6-radian SDL input");
  }
  // Driving stimulus is budgeted in authority ticks, independent of render
  // cadence and repeated/skipped desktop observations.
  unsigned source_drive_ticks=vehicle && !fixed_turret_test && std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT")?300u:0u;
  if(const auto duration=std::getenv("HALO2_TU15_VEHICLE_DRIVE_TICKS")) {
    source_drive_ticks=std::stoul(duration);
    check(vehicle && !fixed_turret_test && source_drive_ticks>0 && source_drive_ticks<=600,
        "Declared drive duration requires a moving vehicle and 1..600 source ticks");
  }
  const bool vehicle_collision_test=vehicle && std::getenv("HALO2_TU15_VEHICLE_COLLISION_INPUT");
  const bool vehicle_rate_reversal_input=std::getenv("HALO2_TU15_VEHICLE_RATE_REVERSAL_INPUT");
  if(vehicle_rate_reversal_input) {
    check(vehicle&&!fixed_turret_test&&!vehicle_collision_test,
        "Rate reversal requires an ordinary moving driver and no wall-approach override");
    if(!source_drive_ticks)source_drive_ticks=240;
    check(source_drive_ticks>=180,"Rate reversal needs at least180 completed authority drive ticks");
  }

  const bool vehicle_blast_test=vehicle && std::getenv("HALO2_TU15_VEHICLE_BLAST_INPUT");
  check(!vehicle_blast_test || lan,"Vehicle blast diagnostics require the actual LAN authority; select lan-vehicle");
  std::optional<unsigned> vehicle_blast_started;
  const bool vehicle_weapon_test=vehicle_blast_test && std::getenv("HALO2_TU15_VEHICLE_WEAPON_INPUT");
  bool vehicle_weapon_pressed{},vehicle_reload_pressed{};unsigned vehicle_weapon_commands{},vehicle_reload_commands{};
  unsigned vehicle_weapon_press_tick{},vehicle_last_shot{},vehicle_loaded_before{};
  std::optional<std::uint32_t> vehicle_blast_aim_tick;
  std::optional<std::uint32_t> vehicle_entry_aim_tick;

  const unsigned vehicle_blast_count=vehicle_blast_test && std::getenv("HALO2_TU15_VEHICLE_BLAST_COUNT")?std::stoul(std::getenv("HALO2_TU15_VEHICLE_BLAST_COUNT")):1u;
  check(!vehicle_blast_test || (vehicle_blast_count>=1 && vehicle_blast_count<=2),"Vehicle blast uses the ordinary one/two starting grenades");
  unsigned vehicle_blast_detonations{};std::optional<unsigned> vehicle_last_detonation;


  check(!vehicle_collision_test || !accompanied_driver,"Vehicle collision input requires its ordinary driver view");
  std::optional<RayHit> collision_wall;std::optional<std::pair<std::uint32_t,Vec3>> collision_previous;
  float collision_peak_speed{},collision_minimum_plane_distance=1000000.f;unsigned collision_stopped_ticks{};
  bool collision_camera_turned{};

  const bool control_test=lan && std::getenv("HALO2_TU15_CONTROL_ENTRY");
  const bool script_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_DEVICE_SCRIPT_INPUT");
  const bool support_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_SUPPORT_INPUT");
  const bool conveyor_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_PLAYER_CONVEYOR_INPUT");
  unsigned conveyor_grounded_ticks{},conveyor_uncommanded_ticks{};float conveyor_grounded_travel{};
  const bool object_field_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_OBJECT_FIELD_TARGET");
  const bool prop_watch_test=scenario=="lan-movement" && (std::getenv("HALO2_TU15_PROP_WATCH_INPUT") || object_field_test);
  const unsigned object_field_ticks=object_field_test && std::getenv("HALO2_TU15_OBJECT_FIELD_TICKS")?
      std::stoul(std::getenv("HALO2_TU15_OBJECT_FIELD_TICKS")):360u;
  check(!object_field_test || (object_field_ticks>=360 && object_field_ticks<=1800),"Declared object observation must cover360..1800 normal ticks");
  bool object_field_jump{},object_field_jump_released{};
  const bool object_field_shoot=object_field_test && std::getenv("HALO2_TU15_OBJECT_FIELD_SHOOT");
  const bool object_field_grenade=object_field_test && std::getenv("HALO2_TU15_OBJECT_FIELD_GRENADE");
  bool object_field_fire_pressed{},object_field_fire_released{};unsigned object_field_fire_commands{};
  std::map<std::pair<std::uint32_t,unsigned>,Transform> object_field_first_nodes;
  std::map<std::uint32_t,float> object_field_angles;

  std::optional<PropWatchEntrance> prop_watch;
  std::optional<std::pair<float,float>> prop_pending_look;
  std::map<std::uint32_t,Vec3> prop_watch_first;
  std::map<std::uint32_t,float> prop_watch_travel,prop_watch_horizontal,prop_watch_drop;
  std::map<std::uint32_t,unsigned> prop_watch_draws;
  unsigned prop_watch_aimed{};

  check(!support_test || (!script_test && !control_test),"Select one support/device stimulus");
  std::optional<SupportEntrance> support_start;bool support_started=false;
  const bool support_board_input=std::getenv("HALO2_TU15_SUPPORT_BOARD_INPUT");
  const bool support_board_jump=std::getenv("HALO2_TU15_SUPPORT_BOARD_JUMP");
  check(!support_board_jump||support_board_input,"Support boarding jump requires its ordinary boarding input");
  std::optional<unsigned> support_board_target_node;
  if(const auto node=std::getenv("HALO2_TU15_SUPPORT_BOARD_TARGET_NODE")) {
    std::size_t used{};const std::string text=node;const auto index=std::stoul(text,&used);
    check(support_board_input&&used==text.size()&&index<256,"Invalid declared ordinary boarding target node");
    support_board_target_node=unsigned(index);
  }
  check(!support_board_jump||support_board_target_node.has_value(),"Jump boarding needs its declared authored target node");
  std::optional<unsigned> support_board_floor_node;
  if(const auto node=std::getenv("HALO2_TU15_SUPPORT_BOARD_FLOOR_NODE")) {
    std::size_t used{};const std::string text=node;const auto index=std::stoul(text,&used);
    check(support_board_input&&!support_board_jump&&support_board_target_node&&used==text.size()&&index<256,
        "Invalid declared BSP-floor boarding node");
    support_board_floor_node=unsigned(index);
  }
  const bool support_board_approach=std::getenv("HALO2_TU15_SUPPORT_BOARD_APPROACH_INPUT");
  check(!support_board_approach||(support_board_input&&!support_board_jump&&!support_board_floor_node&&support_board_target_node),
      "A surveyed boarding approach requires its ordinary target node and no alternate entry");
  bool support_board_jump_pressed{},support_board_jump_released{};
  std::optional<std::pair<float,float>> support_pending_look;
  check(!script_test || !control_test,"Select one declared device stimulus");
  bool script_started=false;std::optional<ScriptProgram> script_input;
  std::optional<ControlEntrance> control_start;
  bool control_pressed=false,control_released=false;
  std::optional<std::pair<float,float>> control_pending_look;
  const bool long_vehicle_sequence=vehicle && (vehicle_blast_test || accompanied_driver || !std::getenv("HALO2_TU15_VEHICLE_SEAT")) &&
      (vehicle_blast_test || vehicle_name=="warthog" || vehicle_name=="banshee" || vehicle_name=="scorpion" || vehicle_name=="wraith" || vehicle_name=="spectre" || vehicle_name=="spectre_secondary" || vehicle_name=="warthog_gauss");
  check(!lan || scenario=="lan-movement" || lift || vehicle || objects,"Unknown LAN physics scenario");
  check(lan||lift||vehicle||objects||scenario=="movement","Unknown multiplayer physics scenario");
  check(std::getenv("DISPLAY")||std::getenv("WAYLAND_DISPLAY"),"Visible desktop display is required");
  const std::filesystem::path output=argv[5];std::filesystem::create_directories(output);
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=std::filesystem::canonical(argv[1]);o.shared_maps=std::filesystem::canonical(argv[2]);
  const auto mcc_maps=std::filesystem::canonical(std::filesystem::path(argv[4])/"halo2/h2_maps_win64_dx11");
  check(o.shared_maps==mcc_maps && o.map.parent_path()==mcc_maps,"Physics map is outside the selected prepared MCC cache");
  {const Cache admitted(o.map,o.shared_maps);check(admitted.is_mcc(),"Live physics acceptance requires an actual MCC cache");}
  // Native TU1.5 startup and HUD resource lookup consume the imported cache.
  // No Xbox image constructor or Vista UI path is supplied at runtime.
  o.starting_weapon=vehicle_weapon_test?8:5;o.width=960;o.height=540;o.local_players=lan&&!vehicle&&!objects?1:2;o.mute=true;
  o.source_object_world=true;o.source_object_savegame_directory=output/"source-savegame";
  o.variant.values.set(2,0);o.variant.values.set(10,0); // Finite test lifecycle owns exit, not game-over.
  if(accompanied_driver) {o.variant.values.set(91,1);o.selected_teams.assign(o.local_players,0);}
  if(vehicle) {
    const auto selected=std::getenv("HALO2_TU15_VEHICLE_NAME");
    // TU1.5 15CC70: parameter78 choices1/2 select the original mulg
    // heavy references at +14/+1C. Keep the ordinary netgame factory,
    // authored placement, variant and child constructors responsible for birth.
    if(selected && std::string_view(selected)=="scorpion")o.variant.values.set(78,1);
    if(selected && std::string_view(selected)=="wraith")o.variant.values.set(78,2);
    // Actual15CB60 fixed choice5 reads multiplayer reference+2C.
    if(selected && std::string_view(selected)=="spectre")o.variant.values.set(76,5);
    if(selected && std::string_view(selected)=="spectre_secondary")o.variant.values.set(77,5);
    // Actual15CB60 choice2 retains the Warthog definition and writes the
    // authored gauss variant05000694; the model owns its turret child.
    if(selected && std::string_view(selected)=="warthog_gauss")o.variant.values.set(76,2);
    if(fixed_turret_test) {
      const unsigned classification=std::getenv("HALO2_TU15_TURRET_CLASS")?std::stoul(std::getenv("HALO2_TU15_TURRET_CLASS")):6u;
      check(classification==6 || classification==7,"Fixed turret requires authored class6/7");
      o.variant.values.set(81u+classification-6u,vehicle_name=="h_turret_ap"?1:4);
    }
  }
  Json report{{"scenario",scenario},{"reference","English TU1.5 only"},{"runtime_cache_source","MCC"},{"visible",true},
      {"live_physics_parity",false},{"stimulus","SDL keyboard events; no supplied physics results"}};
  const bool prop_test=objects && std::getenv("HALO2_TU15_PROP_GRENADE");
  const bool recreate_test=lan && prop_test && std::getenv("HALO2_TU15_SCENE_RECREATE");
  // This fixture's actual spawn collector places the first client at the
  // distant location and the second beside the prop. Assign ordinary devices
  // to those views; never move a completed body to repair the test.
  const unsigned actor_view=recreate_test?1u:0u,watcher_view=recreate_test?0u:1u;
  const bool debris_test=prop_test && std::getenv("HALO2_TU15_DYNAMIC_OBJECTS");
  std::set<std::uint32_t> debris_definitions,turret_projectile_definitions,turret_glow_definitions,turret_contrail_definitions;
  std::set<std::uint32_t> turret_damage_definitions;
  std::uint32_t turret_damage_target=null_datum;
  std::optional<Vec3> turret_damage_position;
  std::array<unsigned,2> turret_damage_draws{};
  std::map<std::uint32_t,Vec3> turret_projectile_births;
  float turret_projectile_travel{},turret_projectile_speed{};
  std::array<unsigned,2> turret_projectile_draws{},turret_glow_draws{},turret_contrail_draws{},turret_visible_contrail_draws{};
  float vehicle_weapon_blast_radius{},vehicle_response_blast_radius{};
  std::optional<ModelCollision> vehicle_blast_model;
  if(vehicle_weapon_test) {
    const Cache cache(o.map,o.shared_maps);
    const auto damage=std::find_if(cache.tags().begin(),cache.tags().end(),[](const Tag &t){return t.group=="jpt!" &&
        t.name=="objects\\weapons\\support_high\\rocket_launcher\\damage_effects\\rocket_launcher_explosion";});
    check(damage!=cache.tags().end(),"Rocket approach has no authored blast definition");
    const auto metadata=cache.load_source_tag_metadata(damage->datum);
    vehicle_weapon_blast_radius=Bytes(metadata.bytes_at(metadata.source_address(),metadata.size())).f32(4);
    check(vehicle_weapon_blast_radius>0.f,"Rocket approach needs the original positive upper blast radius");
    report["declared_rocket_outer_blast_radius"]=vehicle_weapon_blast_radius;
  }
  std::optional<std::uint32_t> prop_placement;
  if(prop_test) {
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    for(const auto &tag:cache.tags())if(tag.group=="bloc" && tag.name.find("power_module_garbage")!=std::string::npos)
      debris_definitions.insert(tag.datum);
    for(unsigned i=0;i<content.objects.size();++i) {
      const auto &object=content.objects[i];const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
      if(!tag || tag->group!="bloc" || tag->name.find("power_module")==std::string::npos)continue;
      prop_placement=i;
      const CollisionWorld terrain(content.collision);
      for(const float side:{0.f,.75f,-.75f}) {
        Spawn spawn;spawn.position=object.position+Vec3{side,1.25f,0};
        const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},
            {0,0,-1},4*content.player.height);
        check(floor.has_value(),"Declared prop fixture has no cached BSP floor");
        spawn.position.z=floor->position.z+content.player.radius;
        const auto delta=object.position-spawn.position;spawn.facing=std::atan2(delta.y,delta.x);
        o.native_spawn_inputs.push_back(spawn);
      }
      if(recreate_test) {
        const auto basis=authored_basis(object.rotation);
        const auto center=object.position+basis.rotate(object.bounding_offset)*object.scale;
        std::optional<Spawn> watcher;
        for(const float radius:{8.f,10.f,6.f}) {
          for(unsigned direction_index=0;direction_index<32 && !watcher;++direction_index) {
            const float angle=float(direction_index)*6.283185307f/32.f;
            Spawn candidate;candidate.position=object.position+Vec3{radius*std::cos(angle),radius*std::sin(angle),0};
            const auto floor=terrain.raycast(candidate.position+Vec3{0,0,2*content.player.height},
                {0,0,-1},4*content.player.height);
            if(!floor || floor->normal.z<content.player.maximum_slope_cosine)continue;
            candidate.position.z=floor->position.z+content.player.radius;
            const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
            const auto delta=center-eye;const float distance=length(delta);
            if(distance<=0 || terrain.raycast(eye,delta/distance,distance))continue;
            candidate.facing=std::atan2(delta.y,delta.x);watcher=candidate;
          }
          if(watcher)break;
        }
        check(watcher.has_value(),"Recreation observer has no visible cached-BSP floor location");
        o.native_spawn_inputs.back()=*watcher;
        report["declared_recreation_observer_position"]=xyz(watcher->position);
      }
      report["prop_placement"]=i;report["prop_definition"]=object.definition;
      report["prop_fixture_authority"]="Declared approach positions relative to authored Lockout power module and cached BSP floor, before tick zero";
      break;
    }
    check(prop_placement.has_value(),"Authored prop fixture has no power module");
  }
  if(conveyor_test) {
    check(!script_test && !support_test && !control_test && !prop_watch_test,"Select one conveyor stimulus");
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    const auto approach=conveyor_entrance(content);const CollisionWorld terrain(content.collision);std::optional<Spawn> distant;
    for(const auto &authored:cache.spawns()) {
      if(length(authored.position-approach.spawn.position)<12*content.player.height)continue;
      Spawn candidate=authored;const auto floor=terrain.raycast(candidate.position+Vec3{0,0,content.player.height},{0,0,-1},4*content.player.height);
      if(!floor || floor->normal.z<=0)continue;candidate.position.z=floor->position.z+content.player.radius;
      if(terrain.overlaps(candidate.position,content.player.height,content.player.radius))continue;
      distant=candidate;break;
    }
    check(distant.has_value(),"Player conveyor requires a distinct authored authority start");
    o.native_spawn_inputs={*distant,*distant,approach.spawn};
    report["stimulus"]="Declared clear original cached BSP conveyor surface and distant authority starts; no movement/jump/fire/grenade input or supplied physics result";
    report["declared_conveyor_start"]=xyz(approach.spawn.position);
    report["declared_conveyor_surface"]={{"bsp",approach.surface.bsp},{"record",approach.surface.record},{"surface",approach.surface.surface},{"instance",approach.surface.instance},{"plane",approach.surface.plane}};
  }
  if(prop_watch_test) {
    check(!script_test && !support_test && !control_test,"Select one declared live physics stimulus");
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    prop_watch=object_field_test?object_field_entrance(cache,content):prop_watch_entrance(cache,content);o.native_spawn_inputs={prop_watch->spawn};
    report["stimulus"]="Unchanged authored map spawner scripts and original live physics; declared BSP-grounded watcher with ordinary SDL look only";
    report["declared_prop_watch_position"]=xyz(prop_watch->spawn.position);
    report["declared_prop_watch_target"]=xyz(prop_watch->target);
    report["watched_prop_placements"]=prop_watch->placements;
    if(object_field_test) {
      report["stimulus"]="Declared BSP-grounded approach to an existing authored physical object; ordinary SDL movement/look/jump/fire/grenade inputs; original contacts, constraints and fields solve all motion";
      // Keep the authority's ordinary player out of the interaction approach.
      // All three starts are supplied before world construction; the normal
      // original spawn collector retains allocation/selection/body ownership.
      const CollisionWorld terrain(content.collision);std::optional<Spawn> distant;
      for(const auto &authored:cache.spawns()) {
        if(length(authored.position-prop_watch->target)<12*content.player.height)continue;
        Spawn candidate=authored;const auto floor=terrain.raycast(candidate.position+Vec3{0,0,content.player.height},{0,0,-1},4*content.player.height);
        if(!floor || floor->normal.z<=0)continue;candidate.position.z=floor->position.z+content.player.radius;
        if(terrain.overlaps(candidate.position,content.player.height,content.player.radius))continue;
        distant=candidate;break;
      }
      check(distant.has_value(),"Object field interaction has no distant authored authority start");
      o.native_spawn_inputs={*distant,*distant,prop_watch->spawn};
      report["declared_distant_authority_start"]=xyz(distant->position);
    }
  }
  if(control_test || script_test) {
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    control_start=control_entrance(cache,content);o.native_spawn_inputs={control_start->spawn};
    report["declared_control_approach"]=xyz(control_start->spawn.position);
    report["declared_control_target"]=xyz(control_start->target);
    report["control_definition"]=control_start->definition;
    report["control_input"]="Ordinary SDL approach/look and displayed control pickup hold; no supplied group or motion result";
    if(script_test) {
      int gate_name=-1;
      for(unsigned n=0;n<content.objects.size();++n)if(n<content.devices.size() && content.devices[n] &&
          content.devices[n]->machine && content.devices[n]->placement.position_group==0)gate_name=content.objects[n].name;
      check(gate_name>=0,"Visible device script requires the original named Zanzibar gate");
      script_input=std::move(test::DeviceScriptProgram(gate_name,60,true).data);
      report["stimulus"]="Declared dormant ScriptProgram started in the actual authority VM; ordinary SDL camera look; no supplied physics result";
      report["control_input"]="Script API sequence through Simulation and original TU1.5 device owners; no switch pickup stimulus";
    }
  }
  if(support_test) {
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    support_start=wheel_support_entrance(cache,content);
    check(!support_board_input || (support_start->narrow&&!support_start->phase),
        "Ordinary support boarding requires the unmodified flat-paddle phase0 approach");
    if(support_board_floor_node) {
      check(*support_board_floor_node<support_start->authored_nodes.size(),"Boarding floor node is outside the actual hierarchy");
      const CollisionWorld terrain(content.collision);
      const auto height=support_start->authored_nodes[*support_board_floor_node].position.z+content.player.height;
      for(auto& spawn:support_start->spawns) {
        auto origin=spawn.position;origin.z=height;
        const auto floor=terrain.raycast(origin,{0,0,-1},height-spawn.position.z+content.player.height);
        check(floor&&floor->normal.z>=content.player.maximum_slope_cosine&&floor->position.z>spawn.position.z,
            "Authored boarding level has no actual upward BSP floor");
        spawn.position=floor->position+Vec3{0,0,content.player.radius};
        check(!terrain.overlaps(spawn.position,content.player.height,content.player.radius),"Authored boarding floor overlaps BSP");
      }
      report["declared_support_boarding_floor_node"]=*support_board_floor_node;
      report["declared_support_boarding_floor_reference"]="v963: identical original Xbox/MCC BSP and PHMO; the old lower approach is behind a solid wall";
    }
    if(support_board_approach) {
      check(*support_board_target_node<support_start->authored_nodes.size(),"Boarding target escapes actual hierarchy");
      const auto target=support_start->authored_nodes[*support_board_target_node].position;
      const CollisionWorld terrain(content.collision);std::optional<Spawn> selected;
      Json survey=Json::array();
      for(float ring:{2.f,3.f,4.f})for(unsigned direction=0;direction<32&&!selected;++direction) {
        const auto angle=6.283185307f*float(direction)/32.f;
        auto origin=target+Vec3{std::cos(angle),std::sin(angle),0}*(ring*content.player.height);
        origin.z=target.z+2*content.player.height;
        const auto floor=terrain.raycast(origin,{0,0,-1},4*content.player.height);
        Json candidate{{"ring_in_player_heights",ring},{"direction",direction},{"origin",xyz(origin)}};
        if(floor)candidate["floor"]={{"position",xyz(floor->position)},{"surface",floor->source.surface},{"plane",floor->source.plane}};
        if(!floor||floor->normal.z<content.player.maximum_slope_cosine||std::abs(floor->position.z-target.z)>content.player.height){survey.push_back(candidate);continue;}
        Spawn spawn;spawn.position=floor->position+Vec3{0,0,content.player.radius};
        if(terrain.overlaps(spawn.position,content.player.height,content.player.radius)){candidate["overlaps_bsp"]=true;survey.push_back(candidate);continue;}
        const auto eye=spawn.position+Vec3{0,0,content.player.camera_height};
        const auto delta=target+Vec3{0,0,content.player.camera_height}-eye;
        const auto block=terrain.raycast(eye,normalized(delta),length(delta));
        if(block&&block->distance<length(delta)-content.player.radius){candidate["blocked_surface"]=block->source.surface;survey.push_back(candidate);continue;}
        spawn.facing=std::atan2(target.y-spawn.position.y,target.x-spawn.position.x);
        candidate["selected"]=true;survey.push_back(candidate);selected=spawn;
      }
      report["support_boarding_bsp_survey"]=survey;
      std::ofstream(output/"support-boarding-survey.json")<<report.dump(2)<<'\n';
      check(selected.has_value(),"Original BSP has no clear surveyed phase0 boarding approach");
      support_start->spawns={*selected,*selected};
    }
    o.native_spawn_inputs=support_start->spawns;
    if(support_start->narrow) {
      const CollisionWorld terrain(content.collision);std::optional<Spawn> distant;
      for(const auto &authored:cache.spawns()) {
        if(length(authored.position-support_start->spawns.front().position)<12*content.player.height)continue;
        Spawn candidate=authored;const auto floor=terrain.raycast(candidate.position+Vec3{0,0,content.player.height},{0,0,-1},4*content.player.height);
        if(!floor || floor->normal.z<=0)continue;candidate.position.z=floor->position.z+content.player.radius;
        if(terrain.overlaps(candidate.position,content.player.height,content.player.radius))continue;
        distant=candidate;break;
      }
      check(distant.has_value(),"Narrow paddle requires a distinct original authority start");
      o.native_spawn_inputs={*distant,*distant,support_start->spawns.front()};
    }
    report["stimulus"]=std::getenv("HALO2_TU15_SUPPORT_DEFINITION")?
        "Declared original PHMO body and model-node approach; actual current source device APIs pause/resume; ordinary SDL look only; no supplied contact, velocity or solved body":
        "Declared original hull0 paddle spawn inputs; original device power API pauses/resumes the actual wheel; ordinary SDL look only";
    report["declared_support_spawns"]=Json::array();for(const auto &spawn:o.native_spawn_inputs)report["declared_support_spawns"].push_back(xyz(spawn.position));
    report["declared_support_local_top"]=xyz(support_start->local_top);
    report["declared_support_node"]=support_start->node;report["declared_support_definition"]=support_start->definition;
    if(support_start->phase)report["declared_support_phase_input"]=*support_start->phase;
    report["support_input_bsp_survey"]=Json::array();
    report["support_input_player_height"]=content.player.height;report["support_input_player_radius"]=content.player.radius;
    const CollisionWorld terrain(content.collision);
    for(const auto &spawn:o.native_spawn_inputs) {
      Json row{{"spawn",xyz(spawn.position)},{"overlaps_cached_bsp",terrain.overlaps(spawn.position,content.player.height,content.player.radius)}};
      const auto floor=terrain.raycast(spawn.position+Vec3{0,0,content.player.height},{0,0,-1},4*content.player.height);
      if(floor)row["floor"]={{"position",xyz(floor->position)},{"normal",xyz(floor->normal)},
          {"bsp",floor->source.bsp},{"surface",floor->source.surface},{"plane",floor->source.plane}};
      else row["floor"]=nullptr;
      report["support_input_bsp_survey"].push_back(std::move(row));
    }
    std::ofstream(output/"support-input-survey.json")<<report.dump(2)<<'\n';
    if(std::getenv("HALO2_TU15_SUPPORT_SURVEY_ONLY"))return 0;
  }
  std::unique_ptr<Host> host;
  std::optional<VehicleEntrance> vehicle_start;
  std::optional<Vec3> lift_approach_target;
  const bool lift_jump_input=lift && std::getenv("HALO2_TU15_LIFT_JUMP_INPUT");
  const bool field_create_test=lift && std::getenv("HALO2_TU15_LIFT_CREATE_INPUT");
  float lift_jump_heights=1.f;
  if(const auto text=std::getenv("HALO2_TU15_LIFT_JUMP_HEIGHTS")) {
    std::size_t used{};lift_jump_heights=std::stof(text,&used);
    check(used==std::strlen(text) && std::isfinite(lift_jump_heights) && lift_jump_heights>0,
        "Invalid declared jump input clearance");
  }
  bool lift_jump_pressed=false,lift_jump_released=false,lift_approach_finished=false;
  std::optional<float> lift_pending_yaw_origin;
  if(lift || vehicle) {
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true,source_game_variant(o.variant));
    if(lift) {
      if(std::getenv("HALO2_TU15_LIFT_APPROACH"))lift_approach_target=Vec3{};
      const auto spawn=lift_entrance(cache,content,lift_approach_target?&*lift_approach_target:nullptr);
      o.native_spawn_inputs={spawn};
      if(lift_approach_target) {
        report["declared_lift_approach_target"]=xyz(*lift_approach_target);
        report["lift_input"]=lift_jump_input?"Ordinary SDL forward, look and jump input into raised field":
            "Ordinary SDL forward and look input from declared BSP-grounded approach; no jump";
      }
      report["authored_lift_player_height"]=content.player.height;
      if(lift_jump_input)report["declared_lift_jump_distance_heights"]=lift_jump_heights;
      if(field_create_test)report["declared_field_creation_input"]=true;
      report["declared_lower_field_floor"]=bool(std::getenv("HALO2_TU15_LIFT_LOWER_FLOOR"));
      if(const auto clearance=std::getenv("HALO2_TU15_LIFT_APPROACH_HEIGHTS"))report["declared_lift_approach_heights"]=std::stof(clearance);
      report["declared_initial_location"]=xyz(spawn.position);
      report["location_authority"]=lift_approach_target?
          "Declared approach around original phmo geometry, grounded above original cached BSP floor":
          "Selected original phmo capsule endpoint or hull center composed with its authored placement";
      report["selected_lift_definition"]=std::getenv("HALO2_TU15_LIFT_DEFINITION")?
          std::getenv("HALO2_TU15_LIFT_DEFINITION"):"scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube";
      report["selected_lift_instance"]=std::getenv("HALO2_TU15_LIFT_INSTANCE")?
          std::stoul(std::getenv("HALO2_TU15_LIFT_INSTANCE")):0u;
    } else {
      vehicle_start=vehicle_entrance(cache,content);o.native_spawn_inputs={vehicle_start->spawn};
      if(const auto row=std::getenv("HALO2_TU15_VEHICLE_EQUIPMENT_INPUT"))
        report["declared_vehicle_equipment_input"]=std::stoul(row);
      if(vehicle_collision_test) {
        collision_wall=vehicle_wall_target(*vehicle_start,content);
        report["stimulus"]="Ordinary driver entry, SDL look/steering/throttle toward an existing cached BSP wall, stop and exit; no supplied collision, force, damage or solved pose";
        report["declared_collision_wall"]={{"position",xyz(collision_wall->position)},{"normal",xyz(collision_wall->normal)},
            {"surface",collision_wall->source.surface},{"plane",collision_wall->source.plane},{"material",collision_wall->material}};
        if(const auto direction=std::getenv("HALO2_TU15_VEHICLE_WALL_DIRECTION_INPUT"))
          report["declared_collision_wall"]["survey_direction_input"]=std::stoi(direction);
      }
      report["declared_initial_location"]=xyz(vehicle_start->spawn.position);
      report["location_authority"]="Declared half-radius approach to authored vehicle entry marker, above cached BSP floor before tick zero";
      const auto selected=std::getenv("HALO2_TU15_VEHICLE_NAME");
      report["selected_vehicle"]=selected?selected:"warthog";
      report["driver_seat"]=vehicle_start->seat;report["vehicle_placement"]=vehicle_start->placement;
      report["selected_seat"]=vehicle_start->seat;report["selected_seat_flags"]=vehicle_start->flags;
      report["selected_seat_controls_vehicle"]=(vehicle_start->flags&4u)!=0;
      const auto &placement=content.objects.at(vehicle_start->placement);
      const auto &definition=content.unit_interactions->units.at(placement.definition);
      const auto nodes=pose_model_nodes_world(definition.hierarchy,vehicle_start->root);
      report["selected_vehicle_definition"]=placement.definition;
      if(vehicle_weapon_test) {
        // Scene input clearance includes original direct destruction blasts,
        // whose authored radius can exceed the incoming rocket's radius.
        const auto object=cache.load_source_tag_metadata(placement.definition);
        const Bytes object_bytes(object.bytes_at(object.source_address(),object.size()));
        const auto model=cache.load_source_tag_metadata(object_bytes.u32(0x38u));
        check(model.group()=="hlmt","Vehicle standoff requires its original model damage owner");
        vehicle_blast_model=cache.load_model_collision(model.datum());
        const Bytes head(model.bytes_at(model.source_address(),model.size()));
        const auto read=[&](std::uint32_t at,std::size_t size){return Bytes(cache.read_tag_bytes(at,size));};
        std::set<std::uint32_t> references;
        for(unsigned info=0;info<head.u32(0x60u);++info) {
          const auto damage=read(head.u32(0x64u)+info*0xf8u,0xf8u);
          for(unsigned section=0;section<damage.u32(0xbcu);++section) {
            const auto part=read(damage.u32(0xc0u)+section*0x38u,0x38u);
            for(unsigned response=0;response<part.u32(0xcu);++response) {
              const auto effect=read(part.u32(0x10u)+response*0x50u,0x50u).u32(0x18u);
              if(effect==null_datum || !references.insert(effect).second)continue;
              const auto metadata=cache.load_source_tag_metadata(effect);
              check(metadata.group()=="jpt!","Vehicle response damage reference has another source class");
              const auto radius=Bytes(metadata.bytes_at(metadata.source_address(),metadata.size())).f32(4u);
              check(std::isfinite(radius) && radius>=0,"Vehicle response blast has an invalid original radius");
              vehicle_response_blast_radius=std::max(vehicle_response_blast_radius,radius);
              report["declared_vehicle_response_blasts"].push_back({{"definition",effect},{"outer_radius",radius}});
            }
          }
        }
      }
      if(turret_damage_test) {
        for(const auto &target:cache.tags())if(target.group=="vehi") {
          const auto tag=cache.load_source_tag_metadata(target.datum);
          const Bytes value(tag.bytes_at(tag.source_address(),tag.size()));
          if(value.u16(0x1f0u)!=6u)turret_damage_definitions.insert(target.datum);
        }
        check(!turret_damage_definitions.empty(),"Human turret has no authored vehicle targets");
      }
      if(fixed_turret_test) {
        // Original unit weapons and each barrel's authored projectile ref.
        // These definitions only select observations; no body is created here.
        const auto tag=cache.load_source_tag_metadata(placement.definition);
        const Bytes unit(tag.bytes_at(tag.source_address(),tag.size()));
        const Bytes weapons(tag.bytes_at(unit.u32(0x1c4u),unit.u32(0x1c0u)*8u));
        for(unsigned n=0;n<unit.u32(0x1c0u);++n) {
          const auto weapon=cache.load_source_tag_metadata(weapons.u32(n*8u+4u));
          const Bytes value(weapon.bytes_at(weapon.source_address(),weapon.size()));
          const Bytes barrels(weapon.bytes_at(value.u32(0x2d4u),value.u32(0x2d0u)*0xecu));
          for(unsigned b=0;b<value.u32(0x2d0u);++b)if(barrels.u32(b*0xecu+0x90u)!=null_datum)
            turret_projectile_definitions.insert(barrels.u32(b*0xecu+0x90u));
        }
        check(!turret_projectile_definitions.empty(),"Fixed turret has no authored barrel projectiles");
        report["fixed_turret_authored_projectiles"]=turret_projectile_definitions;
        for(const auto projectile:turret_projectile_definitions) {
          const auto tag=cache.load_source_tag_metadata(projectile);const Bytes value(tag.bytes_at(tag.source_address(),tag.size()));
          const Bytes attachments(tag.bytes_at(value.u32(0x98u),value.u32(0x94u)*0x18u));
          for(unsigned n=0;n<value.u32(0x94u);++n)if(attachments.u32(n*0x18u)==0x4d475332u)
            turret_glow_definitions.insert(attachments.u32(n*0x18u+4u));
        }
        report["fixed_turret_authored_glows"]=turret_glow_definitions;
        for(const auto projectile:turret_projectile_definitions) {
          const auto tag=cache.load_source_tag_metadata(projectile);const Bytes value(tag.bytes_at(tag.source_address(),tag.size()));
          const Bytes attachments(tag.bytes_at(value.u32(0x98u),value.u32(0x94u)*0x18u));
          for(unsigned n=0;n<value.u32(0x94u);++n)if(attachments.u32(n*0x18u)==0x636f6e74u)
            turret_contrail_definitions.insert(attachments.u32(n*0x18u+4u));
        }
        report["fixed_turret_authored_contrails"]=turret_contrail_definitions;
      }
      if(vehicle_blast_test) {
        const auto tag=cache.query_tag({.handle=TagHandle{placement.definition}});
        check(tag && tag->name.starts_with("objects\\vehicles\\"),"Vehicle debris needs its actual selected authored tag");
        const auto end=tag->name.find('\\',std::string_view("objects\\vehicles\\").size());
        check(end!=std::string::npos,"Selected vehicle has no authored family directory");
        const auto prefix=tag->name.substr(0,end+1);
        report["authored_vehicle_debris_definitions"]=Json::array();
        for(const auto &part:cache.tags())if(part.group=="bloc" && part.name.starts_with(prefix) && part.name.find("garbage")!=std::string::npos) {
          debris_definitions.insert(part.datum);
          report["authored_vehicle_debris_definitions"].push_back({{"definition",part.datum},{"name",part.name}});
        }
        check(!debris_definitions.empty(),"Selected vehicle has no authored physical debris definitions");
      }
      report["selected_vehicle_is_child"]=placement.model_attachment.has_value();
      report["authored_seats"]=Json::array();
      for(unsigned index=0;index<definition.seats.size();++index) {
        const auto &seat=definition.seats[index];Json row{{"index",index},{"flags",seat.flags},
            {"mode",seat.mode},{"marker",seat.marker},{"entry_marker",seat.entry_marker},
            {"boarding_seat",seat.boarding_seat},{"radius",seat.radius}};
        for(const auto [name,id]:{std::pair{"seat",seat.marker},std::pair{"entry",seat.entry_marker}}) {
          const auto at=definition.markers.find(id);if(at==definition.markers.end())continue;
          const auto markers=pose_model_markers(at->second,nodes,definition.permutations,{},1);
          if(!markers.empty()) {row[std::string(name)+"_position"]=xyz(markers.front().world.position);
            row[std::string(name)+"_forward"]=xyz(markers.front().world.basis[0]);}
        }
        report["authored_seats"].push_back(row);
      }
      if(std::getenv("HALO2_TU15_VEHICLE_FIXTURE_ONLY")) {
        std::ofstream file(output/"fixture-inputs.json");file<<report.dump(2)<<'\n';
        check(bool(file),"Could not retain declared fixture inputs");return 0;
      }
    }
  }
  if(lan) {
    auto host_options=o;host_options.local_players=1;
    if(!host_options.selected_teams.empty())host_options.selected_teams.resize(1);
    host=std::make_unique<Host>(host_options,recreate_test?prop_placement:std::nullopt,std::move(script_input));
    report["initial_inventory"]=host->initial_inventory;
    o.join=host->join_address;report["udp_port"]=host->port;
    o.source_object_world=false; // Remote authority owns the solved source bank.
    o.native_spawn_inputs.clear(); // The authority alone owns declared test spawns.
    if(const auto selected=std::getenv("HALO2_TU15_CLIENT_LIGHT_CHOICE")) {
      // A declared independent client preference exercises ordinary join
      // integration; the authority keeps its original selected variant.
      const std::string text=selected;std::size_t consumed{};const auto choice=std::stoi(text,&consumed);
      check(consumed==text.size() && choice>=0 && choice<=7 && choice!=4 && choice!=6,
          "Invalid declared client light-vehicle preference");
      o.variant.values.set(76,choice);report["declared_client_light_vehicle_choice"]=choice;
    }
    if(const auto selected=std::getenv("HALO2_TU15_CLIENT_STARTING_WEAPON")) {
      const std::string text=selected;std::size_t consumed{};const auto choice=std::stoi(text,&consumed);
      check(consumed==text.size() && choice>=3 && choice<=20,"Invalid declared client starting weapon");
      o.starting_weapon=unsigned(choice);report["declared_client_starting_weapon"]=choice;
    }
    if(const auto selected=std::getenv("HALO2_TU15_CLIENT_SECONDARY_LIGHT_CHOICE")) {
      const std::string text=selected;std::size_t consumed{};const auto choice=std::stoi(text,&consumed);
      check(consumed==text.size() && choice>=0 && choice<=7 && choice!=6,"Invalid declared client secondary light preference");
      o.variant.values.set(77,choice);report["declared_client_secondary_light_choice"]=choice;
    }
    if(const auto selected=std::getenv("HALO2_TU15_CLIENT_HEAVY_CHOICE")) {
      const std::string text=selected;std::size_t consumed{};const auto choice=std::stoi(text,&consumed);
      check(consumed==text.size() && (choice==0 || choice==1 || choice==2 || choice==4),"Invalid declared client heavy preference");
      o.variant.values.set(78,choice);report["declared_client_heavy_choice"]=choice;
    }
  } else {
    o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);
    report["initial_inventory"]=inventory(*o.prepared_world);
  }
  const auto &interaction_content=lan?*host->content:o.prepared_world->content;
  std::unique_ptr<test::DestructionFiringLaneInputs> vehicle_firing_lane;
  std::optional<Vec3> vehicle_walk_input;
  std::optional<std::uint32_t> vehicle_lane_plan_tick;
  if(vehicle_weapon_test) {
    const Cache cache(o.map,o.shared_maps);
    vehicle_firing_lane=std::make_unique<test::DestructionFiringLaneInputs>(cache,interaction_content,o.bsp);
  }
  std::unique_ptr<CollisionWorld> turret_target_terrain;
  if(turret_damage_test)turret_target_terrain=std::make_unique<CollisionWorld>(interaction_content.collision);
  // Unchanged TU1.5 CB862..CB8A1: player unit+13C != NONE and
  // seat bit16 clear candidate validity. NPC riding is a separate path.
  const bool seat_invalid_for_player=vehicle && (vehicle_start->flags&0x10000u);
  check(!accompanied_driver || !seat_invalid_for_player,"Selected authored seat rejects player occupants");
  std::size_t companion_placement{};unsigned companion_seat{};
  if(accompanied_driver) {
    check(lan && !(vehicle_start->flags&4u),"Accompanied fixture requires a remote passenger or gunner");
    companion_placement=vehicle_start->placement;
    if(const auto &attachment=interaction_content.objects.at(companion_placement).model_attachment)
      companion_placement=attachment->parent;
    const auto &definition=interaction_content.unit_interactions->units.at(interaction_content.objects.at(companion_placement).definition);
    const auto at=std::find_if(definition.seats.begin(),definition.seats.end(),[](const auto &seat){return seat.flags&4u;});
    check(at!=definition.seats.end(),"Accompanied fixture has no authored driver");
    companion_seat=unsigned(at-definition.seats.begin());
    report["observer_input"]="Actual SDL virtual gamepad; approach, boarding, driving and look";
  }
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(o.width,o.height,false);
  SDL_SetWindowTitle(window,("TU1.5 physics acceptance: "+scenario).c_str());
  SDL_SysWMinfo wm{};SDL_VERSION(&wm.version);
#ifdef SDL_VIDEO_DRIVER_X11
  if(SDL_GetWindowWMInfo(window,&wm) && wm.subsystem==SDL_SYSWM_X11)
    std::cout<<"acceptance_x11_window="<<wm.info.x11.window<<std::endl;
#endif
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);focus(window);
  std::unique_ptr<ObserverController> observer;
  if(vehicle || objects) {
    observer=std::make_unique<ObserverController>();o.input_instances={-1,observer->instance};
    if(recreate_test)o.input_instances={observer->instance,-1};
    if(objects)observer->axis(SDL_CONTROLLER_AXIS_LEFTY,0.f);
    if(!accompanied_driver)report["observer_input"]="Actual SDL virtual gamepad; movement and look only";
    report["actor_view"]=actor_view;report["observer_view"]=watcher_view;
  }
  if(source_drive_ticks)report["declared_source_drive_ticks"]=source_drive_ticks;
  if(vehicle_rate_reversal_input)report["declared_vehicle_rate_reversal_input"]=
      "SDL W+D, S+A, W+D in three equal authority-tick drive intervals";
  if(takeoff_pitch_input)report["declared_takeoff_pitch_input"]=*takeoff_pitch_input;
  Json samples=Json::array(),frames=Json::array();
  Json impulses=Json::array();
  std::ofstream impulse_trace;
  if(prop_placement && !lan) {
    impulse_trace.open(output/"point-impulses.jsonl");check(bool(impulse_trace),"Could not retain actual local point impulses");
    o.prepared_world->source_objects->observe_body_impulses([&](const auto &event){
      auto row=impulse_json(event);impulse_trace<<row.dump()<<'\n';impulse_trace.flush();
      check(bool(impulse_trace),"Could not retain completed local point impulse");
      impulses.push_back(std::move(row));
    });
  }
  std::ofstream trace(output/"ticks.jsonl");check(bool(trace),"Could not create live tick trace");
  const auto window_id=SDL_GetWindowID(window);unsigned local_slot=~0u;bool started=false;
  std::uint32_t start_step{},start_authority_tick{};float initial_z{},peak_z{},fastest{},largest_crouch{},distance{};
  Vec3 initial{};unsigned jump_commands{},forward_commands{},matched_snapshots{};
  unsigned grenade_commands{};bool attached_grenade{},released_grenade{},falling_grenade{},detonated{};
  bool grenade_pressed{},grenade_released{};
  std::optional<Vec3> prop_before;float prop_displacement{};bool prop_retired{};
  struct AuthoritySample {
    unsigned slot;std::uint32_t tick,player,unit,parent;std::int16_t seat;
    Vec3 position,velocity;float crouch;
    std::vector<WorldVehiclePresentation> vehicles;
    std::vector<SourceWorldPose> poses;
    std::vector<SourceSceneRoot> scene_roots;
    std::vector<SourceDynamicObject> dynamic_objects;
    std::vector<NativeGrenadeProjectile> grenades;
    std::vector<NativeGrenadeDetonation> detonations;
    SourceDeviceWorldSnapshot devices;
    SourceContrailSnapshot contrails;
  };
  std::vector<AuthoritySample> authority_samples;
  std::uint32_t driven_vehicle=null_datum;Vec3 vehicle_initial{};float vehicle_distance{};
  std::map<std::uint32_t,std::uint32_t> expected_vehicle_definitions;
  std::array<unsigned,2> driven_vehicle_draws{};
  o.vehicle_draw_observer=[&](unsigned view,std::uint32_t object,std::uint32_t definition,
      const ObjectLightingState &,bool,unsigned,std::span<const Vertex> vertices) {
    if(!vehicle || !started || vertices.empty())return;
    const auto expected=expected_vehicle_definitions.find(object);
    if(expected!=expected_vehicle_definitions.end())
      check(definition==expected->second,"Submitted vehicle model differs from the actual authority definition");
    if(object==driven_vehicle && view<driven_vehicle_draws.size())++driven_vehicle_draws[view];
    if(turret_damage_test && object==turret_damage_target && view<2)++turret_damage_draws[view];
  };
  o.weapon_projectile_draw_observer=[&](unsigned view,std::uint32_t,std::uint32_t object,
      std::uint32_t definition,std::span<const Vertex> vertices) {
    if(fixed_turret_test && turret_projectile_definitions.contains(definition) && !vertices.empty() && view<2)
      ++turret_projectile_draws[view];
  };
  o.weapon_light_volume_observer=[&](unsigned view,std::uint32_t,const SourceLightVolumeDraw &draw) {
    if(fixed_turret_test && view<2 && turret_projectile_births.contains(draw.object) &&
        turret_glow_definitions.contains(draw.definition) && !draw.vertices.empty())++turret_glow_draws[view];
  };
  Json contrail_submissions=Json::array();
  o.weapon_contrail_observer=[&](unsigned view,std::uint32_t tick,const SourceContrailDraw &draw) {
    if(fixed_turret_test && view<2 && turret_contrail_definitions.contains(draw.definition) && !draw.vertices.empty()) {
      ++turret_contrail_draws[view];bool colored_area{};
      for(unsigned n=0;n+2<draw.vertices.size();n+=3) {
        const auto &a=draw.vertices[n],&b=draw.vertices[n+1],&c=draw.vertices[n+2];
        colored_area|=((a.color|b.color|c.color)&0xff000000u) && length(cross(b.position-a.position,c.position-a.position))>1e-6f;
      }
      if(colored_area)++turret_visible_contrail_draws[view];
      Json vertices=Json::array();
      for(const auto &v:draw.vertices)vertices.push_back({{"position",xyz(v.position)},{"uv",{v.uv.x,v.uv.y}},{"color",v.color}});
      contrail_submissions.push_back({{"view",view},{"authority_tick",tick},{"completed_frames_before_submission",frames.size()},
          {"sdl_milliseconds",SDL_GetTicks64()},{"trail",draw.datum},{"definition",draw.definition},
          {"vertices",vertices},{"viewport",draw.viewport},{"clip",draw.clip_range},
          {"colored_triangle_area",colored_area},{"constants",draw.constants},{"pixel",draw.pixel.words},
          {"sampler",std::bit_cast<std::array<std::uint32_t,14>>(draw.sampler)}});
    }
  };
  bool vehicle_entered{},vehicle_exited{};unsigned vehicle_drive_commands{};
  bool entry_released{},exit_released{},entry_approaching{};
  std::optional<unsigned> entry_press,entered_tick,drive_start,exit_press;
  std::optional<unsigned> drive_end;bool steering_started{},steering_released{},forward_released{};
  Vec3 drive_initial{};float driven_distance{},vehicle_climb{};
  Vec3 steering_forward{};float steering_angle{};unsigned steering_commands{};
  bool turret_fire_pressed{},turret_fire_released{};unsigned turret_fire_commands{};
  std::optional<unsigned> turret_fire_started;
  std::set<std::uint32_t> turret_effect_serials;
  std::map<unsigned,Vec3> turret_initial_node_forward;float turret_node_turn{};
  std::optional<Vec3> steering_camera_forward;float steering_camera_angle{};
  unsigned observer_slot=~0u,observer_back_commands{};Vec3 observer_initial{};float observer_distance{};
  bool companion_entered{},companion_button{},companion_released{};
  unsigned companion_route{},companion_forward_commands{},companion_camera_frames{};
  unsigned invalid_seat_prompts{},ordinary_rejected_seat_camera_frames{};
  std::optional<unsigned> companion_press,companion_entered_tick;
  std::uint32_t companion_handle=null_datum;
  std::array<unsigned,4> grenade_draws{};
  std::array<unsigned,4> explosion_particle_draws{};
  std::set<std::uint32_t> detonated_projectiles;
  o.effect_observer=[&](unsigned view,std::uint32_t,std::span<const ParticleDraw> draws) {
    check(view<o.local_players,"Effect submission lost its actual local view");
    for(const auto &draw:draws)if(detonated_projectiles.contains(draw.effect_owner))
      ++explosion_particle_draws[view];
  };
  o.grenade_draw_observer=[&](unsigned view,std::uint32_t,std::uint32_t,std::uint32_t,std::span<const Vertex> vertices) {
    check(view<o.local_players && !vertices.empty(),"Actual grenade draw lost its view or geometry");++grenade_draws[view];
  };
  o.frame_observer=[&](const MultiplayerFrameTiming &f) {
    if(frames.empty())std::cout<<"acceptance_first_frame=1"<<std::endl;
    frames.push_back({{"frame",f.frame},{"views",f.views},{"wall_ms",f.wall_ms},{"submitted",f.submitted}});
  };
  unsigned adopted_scene_frames{};
  unsigned adopted_dynamic_frames{};float maximum_debris_displacement{};
  std::map<std::uint32_t,Vec3> first_debris_position;
  std::map<std::uint32_t,std::array<unsigned,4>> debris_draws;
  o.source_dynamic_observer=[&](const MultiplayerState &state,const SourceDynamicObjectScene &scene) {
    for(const auto &object:state.source_dynamic_objects) {
      check(same_dynamic(object,scene.get(object.object)),"Dynamic renderer changed its received source owner/state");
      const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
          [&](const auto &p){return p.object==object.object;});
      const auto nodes=scene.nodes(object.object);
      check(object.has_world_nodes==(pose!=state.source_body_poses.end()),"Dynamic renderer lost source WORLD ownership");
      if(object.has_world_nodes)check(nodes.size()==pose->nodes.size() &&
          !std::memcmp(nodes.data(),pose->nodes.data(),nodes.size()*sizeof(Transform)),"Dynamic renderer changed completed WORLD nodes");
      if(debris_definitions.contains(object.definition)) {
        const auto [at,fresh]=first_debris_position.try_emplace(object.object,object.transform.position);(void)fresh;
        maximum_debris_displacement=std::max(maximum_debris_displacement,length(object.transform.position-at->second));
      }
    }
    ++adopted_dynamic_frames;
  };
  o.dynamic_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,std::span<const Vertex> vertices) {
    check(view<o.local_players && !vertices.empty(),"Source dynamic geometry lost its actual view/submission");
    if(debris_definitions.contains(definition))++debris_draws[owner][view];
  };
  std::set<std::uint32_t> adopted_prop_generations;
  std::array<std::map<std::uint32_t,unsigned>,4> prop_draw_generations;
  o.prop_draw_observer=[&](unsigned view,std::uint32_t handle,std::uint32_t placement,std::span<const Vertex> vertices) {
    check(view<o.local_players && !vertices.empty(),"Actual prop draw lost its local view or geometry");
    if(prop_placement && placement==*prop_placement)++prop_draw_generations[view][handle];
    if(prop_watch && prop_watch->placements.contains(placement))++prop_watch_draws[handle];
  };
  o.source_scene_observer=[&](const MultiplayerState &state,const SceneObjects &scene) {
    if(!lan)return;
    check(state.source_scene_roots_authoritative,"Replica scene observer has no source authority");
    for(const auto &root:state.source_scene_roots) {
      const auto &object=scene.get(root.object);
      if(prop_watch && (object_field_test || root.created_tick>0) && prop_watch->placements.contains(root.placement)) {
        const auto position=root.transform.position;const auto [at,fresh]=prop_watch_first.try_emplace(root.object,position);(void)fresh;
        const auto delta=position-at->second;
        prop_watch_travel[root.object]=std::max(prop_watch_travel[root.object],length(delta));
        prop_watch_horizontal[root.object]=std::max(prop_watch_horizontal[root.object],std::hypot(delta.x,delta.y));
        prop_watch_drop[root.object]=std::max(prop_watch_drop[root.object],-delta.z);
      }

      if(prop_placement && root.placement==*prop_placement)adopted_prop_generations.insert(root.object);
      check(object.placement==root.placement && object.solved_transform &&
          object.created_tick==root.created_tick &&
          !std::memcmp(&*object.solved_transform,&root.transform,sizeof(Transform)) &&
          object.visible==root.visible,"Rendered scene root differs from received source placement/frame");
      const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
          [&](const auto &p){return p.object==root.object;});
      if(object_field_test && prop_watch->placements.contains(root.placement) && pose!=state.source_body_poses.end()) {
        for(unsigned node=0;node<pose->nodes.size();++node) {
          const auto [first,fresh]=object_field_first_nodes.try_emplace({root.object,node},pose->nodes[node]);(void)fresh;
          for(unsigned axis=0;axis<3;++axis) {
            const auto cosine=std::clamp(dot(normalized(first->second.basis[axis]),normalized(pose->nodes[node].basis[axis])),-1.f,1.f);
            object_field_angles[root.object]=std::max(object_field_angles[root.object],std::acos(cosine));
          }
        }
      }
      check(object.solved_nodes.has_value(),"Rendered scene lost its source WORLD adoption");
      if(root.has_world_nodes)check(pose!=state.source_body_poses.end() &&
          object.solved_nodes->size()==pose->nodes.size() &&
          !std::memcmp(object.solved_nodes->data(),pose->nodes.data(),pose->nodes.size()*sizeof(Transform)),
          "Rendered scene hierarchy differs from received source WORLD bank");
      else check(object.solved_nodes->empty(),"Empty source hierarchy gained invented nodes");
    }
    ++adopted_scene_frames;
  };
  Json camera_frames=Json::array();unsigned seated_camera_frames{},exit_camera_frames{};
  unsigned source_exit_transition_frames{},ordinary_after_exit_frames{};
  Json body_frames=Json::array();
  struct BodySample {unsigned slot;std::uint32_t tick,unit;float fraction;Vec3 node0;};
  std::array<std::vector<BodySample>,2> body_view_samples;
  std::array<unsigned,2> seated_body_draws{},exited_body_draws{};
  std::array<unsigned,2> submitted_corpse_draws{};
  o.animation_observer=[&](unsigned view,unsigned slot,const AnimationRenderObservation &sample) {
    if(!vehicle || !started || sample.first_person)return;
    check(view<body_view_samples.size() && slot<multiplayer_capacity,"Vehicle body submission lost its view/player");
    auto &samples=body_view_samples[view];
    check(std::none_of(samples.begin(),samples.end(),[&](const auto &p){return p.unit==sample.unit;}),
        "Vehicle entry submitted a duplicate player body in one view");
    check(!sample.body_nodes.empty(),"Vehicle body submission lost its WORLD nodes");
    samples.push_back({slot,sample.tick,sample.unit,sample.fraction,sample.body_nodes.front().position});
  };
  unsigned living_recreation_observer_frames{},aimed_recreation_observer_frames{};
  float seated_camera_distance{};
  unsigned exact_source_camera_frames{},stable_source_third_person_frames{};
  std::array<float,4> submitted_horizontal_fov{};
  o.view_observer=[&](unsigned view,const Camera &,float horizontal_fov,std::int16_t) {
    check(view<submitted_horizontal_fov.size(),"Camera submission exceeds actual local views");
    submitted_horizontal_fov[view]=horizontal_fov;
  };
  o.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
    if(prop_watch && view==0)for(const auto &root:state.source_scene_roots) {
      if((!object_field_test && root.created_tick==0) || !prop_watch->placements.contains(root.placement))continue;
      const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
      auto target=root.transform.position;
      if(object_field_test)if(const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &pose){return pose.object==root.object;});pose!=state.source_body_poses.end())target=pose->sphere.center;
      if(object_field_test && std::getenv("HALO2_TU15_OBJECT_FIELD_BODY_INPUT"))target=prop_watch->target;
      if(dot(normalized(target-camera.position),forward)>.9f)++prop_watch_aimed;
    }

    if(vehicle && view<body_view_samples.size()) {
      // Body callbacks precede this callback. Validate against this frame's
      // actual snapshot, including entry/exit and replacement generations.
      for(const auto &sample:body_view_samples[view]) {
        const auto &player=state.players[sample.slot];
        check(sample.tick==state.tick,"Vehicle body submission has a different snapshot tick");
        if(sample.unit!=player.source_unit) {
          const auto corpse=std::find_if(state.corpses.begin(),state.corpses.end(),[&](const auto &c){
            return c.object==sample.unit && (c.player&65535u)==sample.slot;
          });
          check(corpse!=state.corpses.end(),"Vehicle body submission has neither current unit nor authoritative corpse identity");
          check(std::any_of(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &pose){return pose.object==corpse->object && !pose.nodes.empty();}),
              "Submitted vehicle-blast corpse has no authoritative source WORLD bank");
          ++submitted_corpse_draws[view];
          body_frames.push_back({{"view",view},{"tick",sample.tick},{"unit",sample.unit},{"corpse",true},
              {"corpse_player",corpse->player},{"death_tick",corpse->death_tick},{"fraction",sample.fraction},{"node0",xyz(sample.node0)}});
          continue;
        }
        const auto parent=std::find_if(state.world_vehicles.begin(),state.world_vehicles.end(),
            [&](const auto &v){return v.handle==player.source_parent;});
        if(parent!=state.world_vehicles.end()) {
          const auto definition=interaction_content.objects.at(parent->placement).definition;
          const auto &bounds=interaction_content.unit_interactions->units.at(definition);
          // Authored bounds allow seat/entry motion and reject an abandoned body.
          check(length(sample.node0-parent->transform.position)<=
              bounds.bound_radius*parent->transform.scale+interaction_content.player.height,
              "Seated player model remained outside its moving vehicle");
          if(sample.slot==local_slot)++seated_body_draws[view];
        } else if(sample.slot==local_slot && vehicle_exited)++exited_body_draws[view];
        body_frames.push_back({{"view",view},{"tick",sample.tick},{"unit",sample.unit},
            {"parent",player.source_parent},{"seat",player.source_seat},{"fraction",sample.fraction},
            {"node0",xyz(sample.node0)},{"player_position",xyz(player.position)},
            {"parent_position",parent==state.world_vehicles.end()?Json():xyz(parent->transform.position)}});
      }
      body_view_samples[view].clear();
    }
    if(recreate_test && started && view==watcher_view && observer_slot<state.players.size() && !state.players[observer_slot].dead) {
      const auto root=std::find_if(state.source_scene_roots.begin(),state.source_scene_roots.end(),
          [&](const auto &root){return root.placement==*prop_placement && root.object!=host->first_prop_generation;});
      if(root!=state.source_scene_roots.end()) {
        const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto &pose){return pose.object==root->object;});
        check(pose!=state.source_body_poses.end(),"Recreated prop camera check lost its actual source sphere");
        ++living_recreation_observer_frames;
        const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
        if(dot(normalized(pose->sphere.center-camera.position),forward)>.98f)++aimed_recreation_observer_frames;
      }
    }
    if(objects && started && view<2) {
      const auto slot=view==actor_view?local_slot:observer_slot;
      if(slot<state.players.size()) {
        const auto &player=state.players[slot];
        camera_frames.push_back({{"view",view},{"tick",state.tick},{"slot",slot},
            {"unit",player.source_unit},{"dead",player.dead},{"player_position",xyz(player.position)},
            {"position",xyz(camera.position)},{"forward",xyz(camera.forward)},
            {"up",xyz(camera.up)},{"field_of_view",camera.field_of_view},{"horizontal_field_of_view",camera.horizontal_field_of_view}});
      }
    }
    if(accompanied_driver && started && view==1 && observer_slot<state.players.size()) {
      const auto &player=state.players[observer_slot];
      if(player.source_parent==companion_handle && companion_handle!=null_datum && player.source_seat==int(companion_seat)) {
        check(player.source_vehicle_camera.has_value(),"Moving seat driver has no original camera");
        const auto &expected=*player.source_vehicle_camera;
        check(!std::memcmp(&camera.position,&expected.position,sizeof(Vec3)) &&
            !std::memcmp(&camera.forward,&expected.forward,sizeof(Vec3)) &&
            !std::memcmp(&camera.up,&expected.up,sizeof(Vec3)) &&
            std::bit_cast<std::uint32_t>(submitted_horizontal_fov[view])==std::bit_cast<std::uint32_t>(expected.horizontal_fov),
            "Moving seat driver submitted a different camera");
        ++companion_camera_frames;
      }
    }
    if(!vehicle || !started || view!=0 || local_slot>=state.players.size())return;
    const auto &player=state.players[local_slot];
    const auto eye=ordinary_unit_eye(player.position,interaction_content.player.camera_height,
        interaction_content.player.crouch_camera_height,player.crouch);
    const auto separation=length(camera.position-eye);
    if(seat_invalid_for_player) {
      check(player.source_parent==null_datum && !player.source_vehicle_camera,
          "Player entered an original invalid-for-player seat or received a seated camera");
      ++ordinary_rejected_seat_camera_frames;
    }
    if(player.source_parent==driven_vehicle && player.source_seat==int(vehicle_start->seat)) {
      ++seated_camera_frames;seated_camera_distance=std::max(seated_camera_distance,separation);
      if(steering_started && (!forward_released || vehicle_collision_test)) {
        if(!steering_camera_forward)steering_camera_forward=camera.forward;
        const auto &f=camera.forward,&s=*steering_camera_forward;
        steering_camera_angle=std::max(steering_camera_angle,std::abs(std::atan2(
            s.x*f.y-s.y*f.x,s.x*f.x+s.y*f.y)));
      }
      if(lan) {
        check(player.source_vehicle_camera.has_value(),"UDP seat has no actual authority vehicle camera");
        const auto &expected=*player.source_vehicle_camera;
        check(!std::memcmp(&camera.position,&expected.position,sizeof(Vec3)) &&
            !std::memcmp(&camera.forward,&expected.forward,sizeof(Vec3)) &&
            !std::memcmp(&camera.up,&expected.up,sizeof(Vec3)) &&
            std::bit_cast<std::uint32_t>(submitted_horizontal_fov[view])==
                std::bit_cast<std::uint32_t>(expected.horizontal_fov),
            "Submitted UDP seat camera differs from the actual authority vehicle view");
        const auto layout=source_view_grid(o.local_players,view,
            hud_layout_widescreen(o.local_players,std::uint64_t(o.width)*3>std::uint64_t(o.height)*4,
                o.desktop->display_settings.two_player_split));
        const auto lens=vehicle_camera_angles(expected.horizontal_fov,
            interaction_content.original_data->death_camera_constants->default_fov,
            o.desktop->display_settings.vehicle_field_of_view,layout.grid,layout.span);
        check(std::abs(camera.field_of_view-lens.projection_angle*57.295779513f)<.00001f,
            "Submitted vehicle projection discarded the TU1.5 lens or split-screen factors");
        ++exact_source_camera_frames;
        stable_source_third_person_frames+=expected.camera_mode==1;

      }
    } else if(vehicle_exited && player.source_parent==null_datum) {
      ++exit_camera_frames;
      if(player.source_vehicle_camera) {
        ++source_exit_transition_frames;
        const auto &expected=*player.source_vehicle_camera;
        check(!std::memcmp(&camera.position,&expected.position,sizeof(Vec3)) &&
            !std::memcmp(&camera.forward,&expected.forward,sizeof(Vec3)) &&
            !std::memcmp(&camera.up,&expected.up,sizeof(Vec3)) &&
            std::bit_cast<std::uint32_t>(submitted_horizontal_fov[view])==
                std::bit_cast<std::uint32_t>(expected.horizontal_fov),
            "Submitted vehicle exit transition differs from the authority observer");
      } else ++ordinary_after_exit_frames;
    }
    camera_frames.push_back({{"view",view},{"tick",state.tick},{"unit",player.source_unit},
        {"parent",player.source_parent},{"seat",player.source_seat},{"position",xyz(camera.position)},
        {"forward",xyz(camera.forward)},{"up",xyz(camera.up)},{"ordinary_eye_distance",separation},
        {"horizontal_fov",submitted_horizontal_fov[view]},
        {"received_source_vehicle_camera",player.source_vehicle_camera.has_value()}});
  };
  o.input_observer=[&](unsigned step,std::span<const Controls> commands,const MultiplayerState&s) {
    if(!started) {
      std::vector<unsigned> local_slots;
      for(unsigned n=0;n<s.players.size();++n)
        if(s.players[n].connected && (!lan || n!=host->host_slot))local_slots.push_back(n);
      if(local_slots.size()<=actor_view)return;
      if(vehicle && (s.players[local_slots[actor_view]].source_unit==null_datum ||
          s.players[local_slots[actor_view]].dead))return;
      local_slot=local_slots[actor_view];
      const auto &p=s.players[local_slot];started=true;start_step=step;start_authority_tick=s.tick;initial=p.position;
      if(support_start && std::getenv("HALO2_TU15_SUPPORT_DEFINITION"))host->support_client_ready.store(true);
      initial_z=peak_z=p.position.z;
      if(observer) {
        check(local_slots.size()>watcher_view,"Visible observer has no actual local view");
        observer_slot=local_slots[watcher_view];observer_initial=s.players[observer_slot].position;
        check(observer_slot!=~0u,"Visible observer did not join the real roster");
      }
      focus(window);
    }
    check(commands.size()>actor_view,"Actor lost its actual local input view");
    // Full source capture can stall the host while client input time advances.
    // Schedule this finite restore stimulus by completed authoritative ticks,
    // as the existing passenger/blast fixtures do; do not consume its driving
    // interval while no original physics frames have executed.
    const auto elapsed=(player_rearm_test || vehicle_rate_reversal_input || vehicle_blast_test || accompanied_driver || script_test || support_test ||
        std::getenv("HALO2_TU15_AUTHORITY_RESTORE"))?s.tick-start_authority_tick:step-start_step;const auto &p=s.players[local_slot];const auto &c=commands[actor_view];
    if(vehicle)for(const auto &v:s.world_vehicles) {
      check(v.placement<interaction_content.objects.size(),"Vehicle snapshot has no authored authority template");
      expected_vehicle_definitions[v.handle]=interaction_content.objects[v.placement].definition;
    }
    peak_z=std::max(peak_z,p.position.z);fastest=std::max(fastest,p.velocity.z);
    largest_crouch=std::max(largest_crouch,p.crouch);distance=std::max(distance,length(p.position-initial));
    jump_commands+=c.jump;forward_commands+=c.forward!=0;
    Json row{{"input_step",step},{"elapsed",elapsed},{"authority_tick",s.tick},{"slot",local_slot},
      {"player",p.source_player},{"unit",p.source_unit},{"position",xyz(p.position)},
      {"velocity",xyz(p.velocity)},{"crouch",p.crouch},{"grounded",p.native_grounded},
      {"window_focus",bool(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)},
      {"controls",{{"forward",c.forward},{"right",c.right},{"jump",c.jump},{"crouch",c.crouch},
          {"pickup",c.pickup},{"pickup_milliseconds",c.pickup_milliseconds},
          {"yaw",c.yaw},{"pitch",c.pitch}}}};
    grenade_commands+=c.grenade;row["grenade_command"]=c.grenade;
    row["dead"]=p.dead;row["health"]=p.health;row["shield"]=p.shield;
    row["source_scene_roots"]=s.source_scene_roots.size();
    row["grenade_projectiles"]=Json::array();
    if(vehicle) {
      const auto threshold=source_pickup_hold_threshold(interaction_content.pickup_hold_seconds);
      row["pickup_hold_threshold_ms"]=threshold;
      row["pickup_buttons"]=source_pickup_buttons(c.pickup,c.pickup?c.pickup_milliseconds:0,
          c.dual_wield,c.dual_wield?c.dual_wield_milliseconds:0,threshold);
      row["parent"]=p.source_parent;row["seat"]=p.source_seat;row["unit_action_word"]=p.source_action_word;
      row["action_prompt"]={{"action",p.action_prompt.target.action},
          {"index",p.action_prompt.target.index},{"object",p.action_prompt.target.object}};
      if(seat_invalid_for_player && p.action_prompt.target.object==driven_vehicle &&
          p.action_prompt.target.index==int(vehicle_start->seat) &&
          p.action_prompt.target.action>=4 && p.action_prompt.target.action<=6)++invalid_seat_prompts;
      std::optional<Vec3> vehicle_position,vehicle_forward,entry_target;
      if(turret_damage_test) {
        const WorldVehiclePresentation *closest=nullptr;float nearest=100000.f;
        row["turret_damage_candidates"]=Json::array();
        for(const auto &target:s.world_vehicles)if(turret_damage_definitions.contains(interaction_content.objects.at(target.placement).definition)) {
          if(turret_damage_target!=null_datum && target.handle!=turret_damage_target)continue;
          const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &p){return p.object==target.handle;});
          if(pose==s.source_body_poses.end())continue;
          const auto origin=vehicle_start->root.position+Vec3{0,0,interaction_content.player.camera_height};
          const auto delta=pose->sphere.center-origin;const auto distance=length(delta);
          if(distance<=pose->sphere.radius)continue;
          const auto forward=dot(normalized(delta),vehicle_start->root.basis[0]);
          const bool blocked=turret_target_terrain->raycast(origin,normalized(delta),distance-pose->sphere.radius).has_value();
          row["turret_damage_candidates"].push_back({{"object",target.handle},{"definition",interaction_content.objects.at(target.placement).definition},
              {"position",xyz(pose->sphere.center)},{"forward_dot",forward},{"range",distance},{"bsp_occluded",blocked}});
          // A declared forward60-degree input cone, not a replacement for
          // the original turret's actual aiming limits or collision result.
          if(forward<=.5f || blocked)continue;
          if(distance<nearest){nearest=distance;closest=&target;}
        }
        if(closest) {
          if(turret_damage_target==null_datum)turret_damage_target=closest->handle;
          host->turret_damage_target.store(turret_damage_target);
          const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &p){return p.object==turret_damage_target;});
          check(pose!=s.source_body_poses.end(),"Human turret target lost its actual WORLD sphere");
          turret_damage_position=pose->sphere.center;
          row["turret_damage_target"]={{"object",turret_damage_target},{"position",xyz(*turret_damage_position)}};
        }
      }
      if(fixed_turret_test) {
        if(s.source_contrails.authoritative) {
          const auto read=[&](std::uint32_t p,unsigned n) {
            const auto &slices=s.source_contrails.slices;
            const auto at=std::upper_bound(slices.begin(),slices.end(),p,[](auto address,const auto &r){return address<r.address;});
            check(at!=slices.begin(),"Contrail diagnostic lacks original source slice");const auto &r=*std::prev(at);
            check(p>=r.address && std::uint64_t(p)+n<=std::uint64_t(r.address)+r.bytes.size(),"Contrail diagnostic input extent omitted");
            return std::span<const std::byte>(r.bytes).subspan(p-r.address,n);
          };
          const auto word=[&](auto p){return Bytes(read(p,4)).u32(0);};
          const auto trails=word(0x4f7214u),points=word(0x4f7210u);
          row["contrail_source_inputs"]={{"point_high",word(points+0x38u)},{"trails",Json::array()}};
          for(unsigned n=0;n<word(trails+0x38u);++n)if(word(word(trails+0x48u)+n/32u*4u)&(1u<<(n%32u))) {
            const auto bytes=read(word(trails+0x44u)+n*72u,72);const Bytes value(bytes);
            row["contrail_source_inputs"]["trails"].push_back({{"object",(value.u16(0)<<16u)|n},
                {"definition",value.u32(4)},{"owner",value.u32(8)},{"bytes",hex_bytes(bytes)},
                {"counts",{value.u16(0x30),value.u16(0x32),value.u16(0x34),value.u16(0x36)}}});
          }
        }
        row["turret_projectiles"]=Json::array();
        for(const auto &projectile:s.weapon_projectiles)if(turret_projectile_definitions.contains(projectile.definition)) {
          const auto [at,inserted]=turret_projectile_births.try_emplace(projectile.datum,projectile.position);
          turret_projectile_travel=std::max(turret_projectile_travel,length(projectile.position-at->second));
          turret_projectile_speed=std::max(turret_projectile_speed,length(projectile.velocity));
          row["turret_projectiles"].push_back({{"object",projectile.datum},{"definition",projectile.definition},
              {"position",xyz(projectile.position)},{"velocity",xyz(projectile.velocity)},{"traveled",projectile.traveled}});
        }
      }
      for(const auto &v:s.world_vehicles)if(v.placement==vehicle_start->placement) {
        if(driven_vehicle==null_datum){driven_vehicle=v.handle;vehicle_initial=v.transform.position;}
        if(lan)host->vehicle_attack_target.store(driven_vehicle);
        check(v.handle==driven_vehicle,"Driving acceptance crossed a vehicle generation");
        vehicle_distance=std::max(vehicle_distance,length(v.transform.position-vehicle_initial));
        if(lan && vehicle_entered && (vehicle_distance>2.f || (fixed_turret_test && turret_fire_commands>0)) && !vehicle_collision_test)host->capture_world_requested.store(true);
        row["vehicle_root"]=xyz(v.transform.position);row["vehicle_forward"]=xyz(v.transform.basis[0]);row["vehicle"]=v.handle;
        vehicle_position=v.transform.position;
        vehicle_forward=v.transform.basis[0];
        if(vehicle_collision_test && vehicle_entered) {
          const auto plane_distance=std::abs(dot(v.transform.position-collision_wall->position,collision_wall->normal));
          collision_minimum_plane_distance=std::min(collision_minimum_plane_distance,plane_distance);
          if(collision_previous && collision_previous->first!=s.tick) {
            const float seconds=float(s.tick-collision_previous->first)/30.f;
            const auto delta=v.transform.position-collision_previous->second;const float speed=std::hypot(delta.x,delta.y)/seconds;
            collision_peak_speed=std::max(collision_peak_speed,speed);
            const auto &bounds=interaction_content.unit_interactions->units.at(interaction_content.objects.at(v.placement).definition);
            if(c.forward>0 && plane_distance<bounds.bound_radius+interaction_content.player.height && collision_peak_speed>1.f && speed<.5f*collision_peak_speed) {
              ++collision_stopped_ticks;if(lan)host->capture_world_requested.store(true);
            }
            row["collision_horizontal_speed"]=speed;
          }
          if(!collision_previous || collision_previous->first!=s.tick)collision_previous=std::pair{s.tick,v.transform.position};
          row["collision_plane_distance"]=plane_distance;row["collision_stopped_ticks"]=collision_stopped_ticks;
        }
        const auto &definition=interaction_content.unit_interactions->units.at(
            interaction_content.objects.at(vehicle_start->placement).definition);
        const auto &seat=definition.seats.at(vehicle_start->seat);
        const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==v.handle;});
        check(pose!=s.source_body_poses.end(),"Vehicle snapshot omitted the actual WORLD hierarchy");
        if(fixed_turret_test && vehicle_entered) {
          for(const auto &effect:s.weapon_effects)if(effect.unit==driven_vehicle && effect.source_scheduled && effect.source_presentation)
            turret_effect_serials.insert(effect.serial);
          for(unsigned n=0;n<pose->nodes.size();++n) {
            const auto current=pose->nodes[n].basis[0];
            const auto [at,inserted]=turret_initial_node_forward.try_emplace(n,current);
            if(!inserted)turret_node_turn=std::max(turret_node_turn,std::acos(std::clamp(dot(normalized(at->second),normalized(current)),-1.f,1.f)));
          }
          turret_fire_commands+=c.fire;
        }
        const auto markers=pose_model_markers(definition.markers.at(seat.entry_marker),pose->nodes,definition.permutations,{},1);
        check(!markers.empty(),"Live source vehicle has no actual driver-entry WORLD marker");entry_target=markers.front().world.position;
        const auto entry_delta=*entry_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
        const auto entry_distance=length(entry_delta);
        row["entry_distance"]=entry_distance;row["entry_radius"]=seat.radius;
        row["entry_maximum_velocity"]=seat.maximum_velocity;row["entry_velocity"]=length(p.velocity);
        row["entry_aim_limit"]=seat.aim;row["entry_facing_limit"]=seat.facing;
        if(entry_distance>0) {
          const auto toward=entry_delta*(1.f/entry_distance);
          row["entry_aim_angle"]=std::acos(std::clamp(dot(toward,direction(c.yaw,c.pitch)),-1.f,1.f));
          row["entry_facing_angle"]=3.1415927f-std::acos(std::clamp(dot(toward,markers.front().world.basis[0]),-1.f,1.f));
        }
        if(!entry_press && report["selected_vehicle"]=="banshee") {
          if(entry_distance>seat.radius*.75f) {
            if(!entry_approaching || !c.forward)key(window_id,SDL_SCANCODE_W,true);
            entry_approaching=true;
          } else if(entry_approaching) {
            key(window_id,SDL_SCANCODE_W,false);entry_approaching=false;
          }
        }
      }
      if(observer) {
        check(commands.size()==2,"Observer gamepad lost its selected local view");
        const auto &watcher=s.players[observer_slot];const auto &input=commands[1];
        if(accompanied_driver) {
          check(!input.fire && !input.grenade && !input.melee,"Companion gamepad produced an unrequested attack");
          const auto root=std::find_if(s.world_vehicles.begin(),s.world_vehicles.end(),[&](const auto &v){return v.placement==companion_placement;});
          if(root!=s.world_vehicles.end()) {
            companion_handle=root->handle;
            const auto &definition=interaction_content.unit_interactions->units.at(interaction_content.objects.at(companion_placement).definition);
            const auto &seat=definition.seats.at(companion_seat);
            const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==root->handle;});
            check(pose!=s.source_body_poses.end(),"Companion driver lost its actual WORLD hierarchy");
            const auto markers=pose_model_markers(definition.markers.at(seat.entry_marker),pose->nodes,definition.permutations,{},1);
            check(!markers.empty(),"Companion driver lost its authored entry marker");
            const auto &entry=markers.front().world;
            const bool seated=watcher.source_parent==root->handle && watcher.source_seat==int(companion_seat);
            companion_entered|=seated;
            if(seated && !companion_entered_tick)companion_entered_tick=elapsed;
            row["companion"]={{"parent",watcher.source_parent},{"seat",watcher.source_seat},{"route",companion_route},
                {"position",xyz(watcher.position)},{"pickup",input.pickup},{"pickup_ms",input.pickup_milliseconds},
                {"prompt_object",watcher.action_prompt.target.object},{"prompt_seat",watcher.action_prompt.target.index},
                {"forward",input.forward},{"yaw",input.yaw},{"pitch",input.pitch}};
            if(seated) {
              observer->axis(SDL_CONTROLLER_AXIS_LEFTY,drive_start && !forward_released?-1.f:0.f);
              companion_forward_commands+=input.forward>0;
              const auto forward=root->transform.basis[0];
              observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(std::atan2(forward.y,forward.x)-input.yaw,6.283185307f)*8.f);
              observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,input.pitch*8.f);
              observer->button(SDL_CONTROLLER_BUTTON_X,false);companion_button=false;
            } else if(!companion_entered) {
              const float clearance=definition.bound_radius+interaction_content.player.height;
              const auto initial_side=observer_initial-root->transform.position;
              const float initial_angle=std::atan2(initial_side.y,initial_side.x);
              const float entry_angle=std::atan2(entry.basis[0].y,entry.basis[0].x);
              const float arc=std::remainder(entry_angle-initial_angle,6.283185307f);
              std::array<Vec3,10> targets;
              for(unsigned n=0;n<9;++n) {
                const float angle=initial_angle+arc*float(n)/8.f;
                targets[n]=root->transform.position+Vec3{std::cos(angle)*clearance,std::sin(angle)*clearance,0};
              }
              targets.back()=entry.position+entry.basis[0]*(seat.radius*.6f);
              if(companion_route<targets.size() && std::hypot(targets[companion_route].x-watcher.position.x,targets[companion_route].y-watcher.position.y)<.3f)
                ++companion_route;
              const auto target=companion_route<targets.size()?targets[companion_route]:entry.position;
              const auto eye=watcher.position+Vec3{0,0,interaction_content.player.camera_height};
              row["companion"]["approach_target"]=xyz(target);
              const auto delta=target-eye;const auto yaw=std::atan2(delta.y,delta.x);
              const float yaw_error=std::remainder(yaw-input.yaw,6.283185307f);
              const float pitch=companion_route<targets.size()?0.f:std::atan2(delta.z,std::hypot(delta.x,delta.y));
              observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-yaw_error*8.f);
              observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-input.pitch)*8.f);
              observer->axis(SDL_CONTROLLER_AXIS_LEFTY,companion_route<targets.size() && std::abs(yaw_error)<.25f?-1.f:0.f);
              if(!companion_press && watcher.action_prompt.target.object==root->handle &&
                  watcher.action_prompt.target.index==int(companion_seat) && companion_route>=targets.size()) {
                companion_press=elapsed;companion_button=true;observer->button(SDL_CONTROLLER_BUTTON_X,true);
              }
              if(companion_button && input.pickup && input.pickup_milliseconds>=threshold) {
                observer->button(SDL_CONTROLLER_BUTTON_X,false);companion_button=false;companion_released=true;
              }
              if(companion_press && companion_released && elapsed>=*companion_press+60) {
                companion_press.reset();companion_released=false;
              }
            }
          }
        } else {
        check(!input.fire && !input.grenade && !input.melee && !input.pickup,
            "Observer gamepad produced an unrequested gameplay button");
        observer_back_commands+=input.forward<0;
        observer_distance=std::max(observer_distance,length(watcher.position-observer_initial));
        row["observer"]={{"player",watcher.source_player},{"unit",watcher.source_unit},
            {"position",xyz(watcher.position)},{"forward_command",input.forward},
            {"yaw",input.yaw},{"pitch",input.pitch},
            {"source_yaw",watcher.yaw},{"source_pitch",watcher.pitch}};
        // Render/input samples can repeat one completed authority tick.
        // Continue the same ordinary retreat until actual displacement clears
        // the vehicle, with a finite input budget; never move its body here.
        const bool retreat=elapsed<45 || (elapsed<120 && observer_distance<.75f);
        observer->axis(SDL_CONTROLLER_AXIS_LEFTY,retreat?1.f:0.f);
        if(vehicle_position) {
          const auto watch=turret_damage_test && turret_damage_position?*turret_damage_position:*vehicle_position+Vec3{0,0,interaction_content.player.camera_height};
          const auto look=watch -
              (watcher.position+Vec3{0,0,interaction_content.player.camera_height});
          const auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
          // As for the actor's SDL mouse, close this ordinary gamepad loop
          // against the processed source view. Raw PC sample angles can
          // retain a different baseline after original birth/vehicle turns.
          const auto observed_yaw=vehicle_blast_test?watcher.yaw:input.yaw;
          const auto observed_pitch=vehicle_blast_test?watcher.pitch:input.pitch;
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(yaw-observed_yaw,6.283185307f)*8.f);
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-observed_pitch)*8.f);
        }
        }
      }
      if(p.source_parent==driven_vehicle && driven_vehicle!=null_datum) {
        check(p.source_seat==int(vehicle_start->seat),"Source attached the player to a different vehicle seat");
        vehicle_entered=true;vehicle_drive_commands+=c.forward>0;
        if(!entered_tick)entered_tick=elapsed;
        if(drive_start && vehicle_position) {
          const auto delta=*vehicle_position-drive_initial;
          driven_distance=std::max(driven_distance,std::hypot(delta.x,delta.y));
          vehicle_climb=std::max(vehicle_climb,delta.z);
          if(c.right!=0)++steering_commands;
          // The mouse aim remains delivered after lateral input releases.
          // Observe the authored controller's response through the rest of
          // the driven interval, including its actual turning delay.
          if(steering_started && !forward_released && vehicle_forward) {
            const auto &f=*vehicle_forward;
            steering_angle=std::max(steering_angle,std::abs(std::atan2(
                steering_forward.x*f.y-steering_forward.y*f.x,steering_forward.x*f.x+steering_forward.y*f.y)));
          }
        }
      }else if(vehicle_entered && p.source_parent==null_datum)vehicle_exited=true;
      if(entry_press && entry_released && !vehicle_entered && elapsed>=*entry_press+45) {
        // A hold can expire after real contacts carry the player out of seat
        // range. Re-approach and retry through normal input; source admission
        // and attachment remain the only success signal.
        entry_press.reset();entry_released=false;
      }
      if(entry_target && !entry_press && elapsed%5==0 && vehicle_entry_aim_tick!=s.tick) {
        vehicle_entry_aim_tick=s.tick;
        focus(window);check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual vehicle-test mouse input");
        const auto look=*entry_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
        const float pitch=std::atan2(look.z,std::hypot(look.x,look.y));
        const float yaw=std::atan2(look.y,look.x);
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Invalid actual mouse gain");SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
        e.motion.xrel=-int(std::lround(std::remainder(yaw-p.yaw,6.283185307f)/gain));
        e.motion.yrel=-int(std::lround((pitch-p.pitch)/gain))*(settings.invert?-1:1);
        check(SDL_PushEvent(&e)==1,"SDL rejected vehicle-entry look input");
      }
      const int pickup=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Pickup)];
      check(pickup>0 && pickup<SDL_NUM_SCANCODES,"Vehicle acceptance requires a keyboard pickup binding");
      if(!entry_press && driven_vehicle!=null_datum && p.action_prompt.target.object==driven_vehicle &&
          p.action_prompt.target.index==int(vehicle_start->seat) &&
          p.action_prompt.target.action>=4 && p.action_prompt.target.action<=6) {
        if(entry_approaching) {key(window_id,SDL_SCANCODE_W,false);entry_approaching=false;}
        focus(window);entry_press=elapsed;key(window_id,SDL_Scancode(pickup),true);
      }
      if(entry_press && !entry_released && c.pickup && c.pickup_milliseconds>=threshold) {
        key(window_id,SDL_Scancode(pickup),false);entry_released=true;
      }
      if(entered_tick && vehicle_position && vehicle_forward && !drive_start && (!accompanied_driver || companion_entered) && elapsed>=*entered_tick+30) {
        drive_start=elapsed;drive_initial=*vehicle_position;
        focus(window);key(window_id,SDL_SCANCODE_W,true);
        {
          const auto &settings=o.desktop->keyboard_settings;
          const float gain=.0025f*float(settings.sensitivity)/3.f;
          check(gain>0,"Invalid actual departure mouse gain");
          SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
          const auto yaw=std::atan2(vehicle_forward->y,vehicle_forward->x);
          look.motion.xrel=-int(std::lround(std::remainder(yaw-p.yaw,6.283185307f)/gain));
          // A declared flight look is ordinary mouse input before throttle.
          // It leaves original controller/contact results and all flight
          // distance/climb gates unchanged.
          const float departure_pitch=takeoff_pitch_input.value_or(0.f);
          look.motion.yrel=-int(std::lround((departure_pitch-p.pitch)/gain))*(settings.invert?-1:1);
          check(SDL_PushEvent(&look)==1,"SDL rejected vehicle departure look input");
        }
      }
      if(drive_start) {
        if(fixed_turret_test) {
          const auto &settings=o.desktop->keyboard_settings;
          const auto fire=[&](bool down) {
            const int binding=settings.bindings[unsigned(PcAction::Fire)];
            if(binding>0 && binding<SDL_NUM_SCANCODES)key(window_id,SDL_Scancode(binding),down);
            else {
              check(binding>pc_mouse_button(0) && binding<=pc_mouse_button(5),"Turret input requires actual fire binding");
              SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;event.button.windowID=window_id;
              event.button.button=Uint8(binding-pc_mouse_button(0));event.button.state=down?SDL_PRESSED:SDL_RELEASED;
              check(SDL_PushEvent(&event)==1,"SDL rejected ordinary turret fire input");
            }
          };
          if(!turret_fire_pressed && steering_started) {fire(true);turret_fire_pressed=true;turret_fire_started=elapsed;}
          if(turret_damage_test && turret_damage_position && vehicle_position && steering_started && !forward_released && elapsed%3u==0) {
            const auto delta=*turret_damage_position-(*vehicle_position+Vec3{0,0,interaction_content.player.camera_height});
            const auto yaw=std::atan2(delta.y,delta.x),pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
            const float gain=.0025f*float(settings.sensitivity)/3.f;
            SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
            look.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
            look.motion.yrel=-int(std::lround((pitch-c.pitch)/gain))*(settings.invert?-1:1);
            if(look.motion.xrel || look.motion.yrel)check(SDL_PushEvent(&look)==1,"SDL rejected actual turret target look input");
          }
          if(turret_fire_pressed && !turret_fire_released && forward_released) {fire(false);turret_fire_released=true;}
        }
        // A real desktop focus loss clears SDL's keys. Reacquire the actual
        // window and deliver the remaining observed input budget, instead of
        // silently shortening the driving sequence while counting wall ticks.
        if(!vehicle_rate_reversal_input && !forward_released && !c.forward) {focus(window);key(window_id,SDL_SCANCODE_W,true);}
        if(vehicle_collision_test && vehicle_position && vehicle_forward && !forward_released) {
          const auto delta=collision_wall->position-*vehicle_position;
          const auto yaw=std::atan2(delta.y,delta.x);
          const auto error=std::remainder(yaw-std::atan2(vehicle_forward->y,vehicle_forward->x),6.283185307f);
          key(window_id,SDL_SCANCODE_D,error>.08f);key(window_id,SDL_SCANCODE_A,error<-.08f);
          if(elapsed%5==0) {
            const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
            SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
            look.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
            look.motion.yrel=-int(std::lround(-c.pitch/gain))*(settings.invert?-1:1);
            if(look.motion.xrel || look.motion.yrel)check(SDL_PushEvent(&look)==1,"SDL rejected ordinary wall approach look");
          }
          if(collision_stopped_ticks>=8 || vehicle_drive_commands>=600) {
            key(window_id,SDL_SCANCODE_W,false);key(window_id,SDL_SCANCODE_A,false);key(window_id,SDL_SCANCODE_D,false);
            forward_released=true;drive_end=elapsed;
            if(!collision_camera_turned) {
              steering_forward=*vehicle_forward;steering_started=true;collision_camera_turned=true;
              const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
              SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;look.motion.xrel=-int(std::lround(.6f/gain));
              check(SDL_PushEvent(&look)==1,"SDL rejected parked original vehicle camera look");
            }
          }
        }
        if(!vehicle_collision_test && (source_drive_ticks?elapsed>=*drive_start+120u:
            vehicle_drive_commands>=(long_vehicle_sequence?120u:25u)) && !steering_started) {
          check(vehicle_forward.has_value(),"Steering input has no actual vehicle WORLD forward");
          steering_forward=*vehicle_forward;
          key(window_id,SDL_SCANCODE_D,true);steering_started=true;
          {
            // Deliver an ordinary look turn as well as lateral input. The
            // actual authored controller decides how either affects motion.
            const auto &settings=o.desktop->keyboard_settings;
            const float gain=.0025f*float(settings.sensitivity)/3.f;
            check(gain>0,"Invalid actual vehicle-test mouse gain");
            SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
            e.motion.xrel=-int(std::lround(.6f/gain));
            if(report["selected_vehicle"]=="banshee")
              e.motion.yrel=-int(std::lround((.6f-c.pitch)/gain))*(settings.invert?-1:1);
            check(SDL_PushEvent(&e)==1,"SDL rejected vehicle driving look input");
          }
        }
        if(!vehicle_rate_reversal_input && !vehicle_collision_test && steering_started && !steering_released) {
          if(steering_commands>=25) {key(window_id,SDL_SCANCODE_D,false);steering_released=true;}
          else if(!c.right)key(window_id,SDL_SCANCODE_D,true);
        }
        if(!vehicle_collision_test && (source_drive_ticks?elapsed>=*drive_start+source_drive_ticks:
            vehicle_drive_commands>=(long_vehicle_sequence?300u:65u)) && !forward_released &&
            (!turret_damage_test || (turret_fire_started && elapsed>=*turret_fire_started+120u))) {
          key(window_id,SDL_SCANCODE_W,false);forward_released=true;drive_end=elapsed;
        }
        if(vehicle_rate_reversal_input) {
          const auto phase=elapsed-*drive_start;
          const bool reverse=phase>=source_drive_ticks/3u && phase<(source_drive_ticks*2u)/3u;
          const bool active=!forward_released;
          key(window_id,SDL_SCANCODE_W,active&&!reverse);
          key(window_id,SDL_SCANCODE_S,active&&reverse);
          key(window_id,SDL_SCANCODE_D,active&&!reverse);
          key(window_id,SDL_SCANCODE_A,active&&reverse);
          if(!active)steering_released=true;
        }
        if(drive_end && elapsed>=*drive_end+45 && !exit_press) {
          exit_press=elapsed;key(window_id,SDL_Scancode(pickup),true);
        }
      }
      if(exit_press && !exit_released && c.pickup && c.pickup_milliseconds>=threshold) {
        key(window_id,SDL_Scancode(pickup),false);exit_released=true;
      }
      if(vehicle_blast_test && vehicle_exited && p.source_parent==null_datum && !p.dead && vehicle_position) {
        const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==driven_vehicle;});
        check(pose!=s.source_body_poses.end(),"Vehicle blast lost its actual target sphere");
        const auto eye=p.position+Vec3{0,0,interaction_content.player.camera_height};
        auto target=pose->sphere.center;
        bool collision_aim_ready=!vehicle_weapon_test;
        std::optional<PosedCollisionHit> current_view_hit;
        bool current_view_lane_clear=!vehicle_weapon_test;
        if(vehicle_weapon_test) {
          const auto vehicle=std::ranges::find_if(s.world_vehicles,[&](const auto &v){return v.handle==driven_vehicle;});
          check(vehicle_blast_model && vehicle!=s.world_vehicles.end() && vehicle->regions && !pose->nodes.empty(),
              "Vehicle weapon input planning requires actual model collision, nodes and current source permutations");
          const auto query_at=[&](Vec3 origin,Vec3 ray){return posed_collision(*vehicle_blast_model,pose->nodes,
              vehicle->regions->selection.permutations,*interaction_content.original_data,origin,ray,2u);};
          const auto visible_target=[&](Vec3 origin)->std::optional<std::pair<Vec3,PosedCollisionHit>> {
            const auto admit=[&](Vec3 point)->std::optional<std::pair<Vec3,PosedCollisionHit>> {
              const auto ray=(point-origin)*2.f;
              if(const auto hit=query_at(origin,ray)) {
                const auto surface=origin+ray*hit->fraction;
                if(vehicle_firing_lane->clear(origin,surface))return std::pair{surface,*hit};
              }
              return {};
            };
            if(const auto hit=admit(pose->sphere.center))return hit;
            for(float z:{-.5f,0.f,.5f})for(float x:{0.f,-.5f,.5f})for(float y:{0.f,-.5f,.5f}) {
              const auto &basis=pose->nodes.front().basis;
              const auto point=pose->sphere.center+
                  (basis[0]*x+basis[1]*y+basis[2]*z)*pose->sphere.radius;
              if(const auto hit=admit(point))return hit;
            }
            return {};
          };
          const auto minimum_range=length(pose->sphere.center-*vehicle_position)+pose->sphere.radius+
              std::max(vehicle_weapon_blast_radius,vehicle_response_blast_radius)+interaction_content.player.radius;
          const auto root_range=std::hypot(p.position.x-vehicle_position->x,p.position.y-vehicle_position->y);
          const auto planned=visible_target(eye);
          collision_aim_ready=bool(planned) && root_range>=minimum_range;
          row["vehicle_bound_center_collision_hit"]=bool(query_at(eye,(pose->sphere.center-eye)*2.f));
          if(collision_aim_ready || host->vehicle_attack_dead.load())vehicle_walk_input.reset();
          else if(!vehicle_walk_input && (!vehicle_lane_plan_tick || s.tick>=*vehicle_lane_plan_tick+30u)) {
            vehicle_lane_plan_tick=s.tick;
            vehicle_walk_input=vehicle_firing_lane->vantage(p.position,*vehicle_position,minimum_range,
                [&](Vec3 origin){return bool(visible_target(origin));});
          }
          if(vehicle_walk_input) {
            target=*vehicle_walk_input+Vec3{0,0,interaction_content.player.camera_height};
            if(std::hypot(vehicle_walk_input->x-p.position.x,vehicle_walk_input->y-p.position.y)<interaction_content.player.radius)
              vehicle_walk_input.reset();
          } else if(planned) {
            target=planned->first;
            row["declared_vehicle_collision_aim"]={{"point",xyz(target)},{"region",planned->second.region},
                {"node",planned->second.node},{"surface",planned->second.surface},{"material",planned->second.global_material},
                {"scope","Read-only posed/BSP/render input planning; actual projectile impact is not supplied"}};
          }
          row["vehicle_firing_lane"]={{"clear_current_input_lane",collision_aim_ready},
              {"walking_input",bool(vehicle_walk_input)},{"target",xyz(target)},
              {"scope","Only SDL walking/look/fire inputs; original runtime owns all movement and impacts"}};
          const auto view_ray=direction(p.yaw,p.pitch)*(length(target-eye)*2.f);
          current_view_hit=query_at(eye,view_ray);
          current_view_lane_clear=current_view_hit && vehicle_firing_lane->clear(eye,eye+view_ray*current_view_hit->fraction);
          row["vehicle_firing_lane"]["clear_current_view_lane"]=current_view_lane_clear;
        }
        const auto delta=target-eye;
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        if(elapsed%5==0 && vehicle_blast_aim_tick!=s.tick) {
          vehicle_blast_aim_tick=s.tick;
          SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
          // Native18A300 consumes changes in raw PC samples. Vehicle turns
          // can reorient the processed source view independently of that raw
          // baseline, so steer by the error in the actual source view.
          look.motion.xrel=-int(std::lround(std::remainder(std::atan2(delta.y,delta.x)-p.yaw,6.283185307f)/gain));
          look.motion.yrel=-int(std::lround((std::atan2(delta.z,std::hypot(delta.x,delta.y))-p.pitch)/gain))*(settings.invert?-1:1);
          if(look.motion.xrel || look.motion.yrel)check(SDL_PushEvent(&look)==1,"SDL rejected ordinary vehicle-blast aim");
        }
        const auto range=std::hypot(p.position.x-vehicle_position->x,p.position.y-vehicle_position->y);
        if(vehicle_weapon_test) {
          const auto minimum_range=length(pose->sphere.center-*vehicle_position)+pose->sphere.radius+
              std::max(vehicle_weapon_blast_radius,vehicle_response_blast_radius)+interaction_content.player.radius;
          report["declared_weapon_minimum_root_range"]=minimum_range;
          report["weapon_standoff_inputs"]={{"actual_target_sphere_radius",pose->sphere.radius},
              {"actual_center_to_root",length(pose->sphere.center-*vehicle_position)},
              {"authored_blast_outer_radius",vehicle_weapon_blast_radius},
              {"authored_vehicle_response_outer_radius",vehicle_response_blast_radius},
              {"authored_player_radius",interaction_content.player.radius}};
          const auto aim_error=std::acos(std::clamp(dot(normalized(delta),direction(p.yaw,p.pitch)),-1.f,1.f));
          const auto aim_limit=std::asin(std::clamp(pose->sphere.radius/length(delta),0.f,1.f));
          row["vehicle_weapon_aim"]={{"source_look",{p.yaw,p.pitch}},{"raw_input",{c.yaw,c.pitch}},
              {"error",aim_error},{"target_sphere_angle",aim_limit},
              {"posed_view_collision_hit",bool(current_view_hit)},
              {"fire_input",c.fire},{"loaded",p.loaded},{"shot_tick",p.shot_tick}};
          vehicle_weapon_commands+=c.fire;vehicle_reload_commands+=c.reload;
          const auto fire=[&](bool down) {
            const int binding=settings.bindings[unsigned(PcAction::Fire)];
            if(binding>0 && binding<SDL_NUM_SCANCODES)key(window_id,SDL_Scancode(binding),down);
            else {
              check(binding>pc_mouse_button(0) && binding<=pc_mouse_button(5),"Vehicle weapon needs the actual fire binding");
              SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;event.button.windowID=window_id;
              event.button.button=Uint8(binding-pc_mouse_button(0));event.button.state=down?SDL_PRESSED:SDL_RELEASED;
              check(SDL_PushEvent(&event)==1,"SDL rejected ordinary vehicle weapon fire");
            }
          };
          key(window_id,SDL_SCANCODE_S,false);
          if(host->vehicle_attack_dead.load()) {
            if(vehicle_weapon_pressed){fire(false);vehicle_weapon_pressed=false;}
            key(window_id,SDL_SCANCODE_W,false);
          } else if(vehicle_walk_input || !collision_aim_ready) {
            if(vehicle_weapon_pressed){fire(false);vehicle_weapon_pressed=false;vehicle_last_shot=elapsed;}
            const auto walking_error=std::abs(std::remainder(std::atan2(delta.y,delta.x)-p.yaw,6.283185307f));
            key(window_id,SDL_SCANCODE_W,vehicle_walk_input && walking_error<.15f);
          } else {
            key(window_id,SDL_SCANCODE_W,false);
            if(vehicle_weapon_pressed && (unsigned(p.loaded)<vehicle_loaded_before || elapsed>=vehicle_weapon_press_tick+20)) {
              fire(false);vehicle_weapon_pressed=false;vehicle_last_shot=elapsed;
            }
            if(!vehicle_weapon_pressed && p.loaded>0 && elapsed>=vehicle_last_shot+75 &&
                collision_aim_ready && current_view_hit && current_view_lane_clear && aim_error<aim_limit) {
              vehicle_loaded_before=p.loaded;vehicle_weapon_press_tick=elapsed;fire(true);vehicle_weapon_pressed=true;
            }
            const int reload=settings.bindings[unsigned(PcAction::Reload)];
            check(reload>0 && reload<SDL_NUM_SCANCODES,"Vehicle weapon needs the actual reload binding");
            if(p.loaded==0 && p.reserve>0 && !vehicle_reload_pressed){key(window_id,SDL_Scancode(reload),true);vehicle_reload_pressed=true;}
            if(vehicle_reload_pressed && c.reload){key(window_id,SDL_Scancode(reload),false);vehicle_reload_pressed=false;}
          }
        } else {
        if(s.grenade_detonations.size()>vehicle_blast_detonations) {
          vehicle_blast_detonations=s.grenade_detonations.size();vehicle_last_detonation=elapsed;
        }
        if(vehicle_blast_detonations<vehicle_blast_count && vehicle_last_detonation && elapsed>=*vehicle_last_detonation+100 && grenade_released) {
          vehicle_blast_started.reset();grenade_pressed=false;grenade_released=false;vehicle_last_detonation.reset();
        }
        if(!vehicle_blast_started && vehicle_blast_detonations<vehicle_blast_count) {
          key(window_id,SDL_SCANCODE_S,range<4*interaction_content.player.height);
          if(range>=4*interaction_content.player.height) {
            const int grenade=settings.bindings[unsigned(PcAction::Grenade)];
            check(grenade>0 && grenade<SDL_NUM_SCANCODES,"Vehicle blast needs the actual grenade binding");
            key(window_id,SDL_Scancode(grenade),true);grenade_pressed=true;vehicle_blast_started=elapsed;
          }
        } else if(vehicle_blast_started) {
          key(window_id,SDL_SCANCODE_S,elapsed<*vehicle_blast_started+90);
          if(vehicle_blast_detonations>=vehicle_blast_count && vehicle_last_detonation && elapsed>=*vehicle_last_detonation+100)
            key(window_id,SDL_SCANCODE_W,range>3*interaction_content.player.height);
          if(grenade_pressed && !grenade_released && c.grenade) {
            key(window_id,SDL_Scancode(settings.bindings[unsigned(PcAction::Grenade)]),false);grenade_released=true;
          }
        }
        }
      }
    }
    if(objects && observer) {
      check(commands.size()==2,"Object observer lost its actual gamepad view");
      const auto &watcher=s.players[observer_slot];const auto &input=commands[watcher_view];
      check(!input.fire && !input.grenade && !input.melee && !input.pickup,
          "Object observer produced an unrequested gameplay button");
      std::optional<Vec3> target;
      for(const auto &projectile:s.grenade_projectiles)
        if(projectile.parent==null_datum){target=projectile.position;break;}
      if(!target && recreate_test) {
        const auto root=std::find_if(s.source_scene_roots.begin(),s.source_scene_roots.end(),
            [&](const auto &root){return root.placement==*prop_placement;});
        if(root!=s.source_scene_roots.end()) {
          const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),
              [&](const auto &pose){return pose.object==root->object;});
          if(pose!=s.source_body_poses.end())target=pose->sphere.center;
        }
      }
      if(!target && !s.grenade_detonations.empty())target=s.grenade_detonations.back().position;
      if(!target)target=p.position;
      const auto eye=ordinary_unit_eye(watcher.position,interaction_content.player.camera_height,
          interaction_content.player.crouch_camera_height,watcher.crouch);
      const auto delta=*target-eye;
      observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(std::atan2(delta.y,delta.x)-input.yaw,6.283185307f)*8.f);
      observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(std::atan2(delta.z,std::hypot(delta.x,delta.y))-input.pitch)*8.f);
      row["observer"]={{"slot",observer_slot},{"position",xyz(watcher.position)},{"dead",watcher.dead},
          {"yaw",input.yaw},{"pitch",input.pitch}};
    }
    row["source_action_word"]=p.source_action_word;
    if(conveyor_test) {
      const bool neutral=c.forward==0 && c.right==0 && !c.jump && !c.crouch && !c.fire && !c.grenade;
      conveyor_uncommanded_ticks+=neutral;
      if(p.native_grounded) {++conveyor_grounded_ticks;conveyor_grounded_travel=std::max(conveyor_grounded_travel,std::hypot(p.position.x-initial.x,p.position.y-initial.y));}
      row["conveyor_neutral_input"]=neutral;row["conveyor_grounded_travel"]=conveyor_grounded_travel;
      if(conveyor_grounded_ticks>30 && conveyor_grounded_travel>.25f)host->capture_world_requested.store(true);
    }
    if(object_field_test)row["weapon_observation"]={{"fire_input",c.fire},{"object",p.source_weapon},{"loaded",p.loaded},{"shot_tick",p.shot_tick},{"trigger_flags",p.weapon_trigger.input_flags},{"trigger_ticks",p.weapon_trigger.ticks},{"trigger_state",p.weapon_trigger.mode}};
    row["source_grenade_phase"]=p.grenade_action.phase();
    row["source_grenade_counter"]=p.grenade_action.counter();
    row["source_held_grenade"]=p.grenade_action.held();
    row["source_grenade_current"]=p.grenade_current;
    if(objects && elapsed%5==0) {
      const auto released=std::find_if(s.grenade_projectiles.begin(),s.grenade_projectiles.end(),
          [](const auto &projectile){return projectile.parent==null_datum;});
      if(released!=s.grenade_projectiles.end()) {
        const auto eye=ordinary_unit_eye(p.position,interaction_content.player.camera_height,
            interaction_content.player.crouch_camera_height,p.crouch);
        const auto target=released->position-eye;
        const auto &settings=o.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Invalid actual mouse gain while watching the source grenade");
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(std::remainder(std::atan2(target.y,target.x)-c.yaw,6.283185307f)/gain));
        look.motion.yrel=-int(std::lround((std::atan2(target.z,std::hypot(target.x,target.y))-c.pitch)/gain))*(settings.invert?-1:1);
        check(SDL_PushEvent(&look)==1,"SDL rejected actual look input while observing grenade flight");
      }
    }
    for(const auto &g:s.grenade_projectiles) {
      attached_grenade|=g.parent!=null_datum;released_grenade|=g.parent==null_datum;
      falling_grenade|=g.parent==null_datum && g.velocity.z<0;
      row["grenade_projectiles"].push_back({{"datum",g.datum},{"parent",g.parent},{"position",xyz(g.position)},{"velocity",xyz(g.velocity)}});
    }
    if(prop_test) {
      const auto root=std::find_if(s.source_scene_roots.begin(),s.source_scene_roots.end(),
          [&](const auto &root){return root.placement==*prop_placement;});
      if(root!=s.source_scene_roots.end()) {
        row["prop_root"]=xyz(root->transform.position);row["prop_object"]=root->object;
        if(s.grenade_detonations.empty())prop_before=root->transform.position;
        else if(prop_before)prop_displacement=std::max(prop_displacement,length(root->transform.position-*prop_before));
      } else if(prop_before)prop_retired=true;
    }
    detonated|=!s.grenade_detonations.empty();
    for(const auto &event:s.grenade_detonations) {
      detonated_projectiles.insert(event.projectile);
      row["grenade_detonations"].push_back({{"serial",event.serial},{"tick",event.tick},
          {"projectile",event.projectile},{"effect",event.effect},{"position",xyz(event.position)},
          {"forward",xyz(event.forward)},{"up",xyz(event.up)}});
    }
    if(lan) {
      // Compare after the authority worker stops: HELLO's reply can reach this
      // thread before receive() returns and records that same-clock roster.
      // Every observed sample must match; no unavailable comparison is skipped.
      authority_samples.push_back({local_slot,s.tick,p.source_player,p.source_unit,p.source_parent,p.source_seat,
          p.position,p.velocity,p.crouch,s.world_vehicles,s.source_body_poses,s.source_scene_roots,s.source_dynamic_objects,s.grenade_projectiles,s.grenade_detonations,s.devices,s.source_contrails});
      row["authority_comparison_deferred"]=true;
    }
    trace<<row.dump()<<'\n';samples.push_back(std::move(row));
    if(objects && elapsed==25) {
      focus(window);check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual object-test mouse input");
      const auto &settings=o.desktop->keyboard_settings;
      const float gain=.0025f*float(settings.sensitivity)/3.f;check(gain>0,"Invalid actual mouse gain");
      SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
      look.motion.yrel=int(std::lround((prop_test?1.5f:.8f)/gain))*(settings.invert?-1:1);
      check(SDL_PushEvent(&look)==1,"SDL rejected downward object-test look");
    }
    if(objects || object_field_grenade) {
      const int binding=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Grenade)];
      check(binding>0 && binding<SDL_NUM_SCANCODES,"Object acceptance needs the actual grenade key binding");
      if(!grenade_pressed && elapsed>=35) {
        key(window_id,SDL_Scancode(binding),true);grenade_pressed=true;
      }
      if(grenade_pressed && !grenade_released && c.grenade) {
        key(window_id,SDL_Scancode(binding),false);grenade_released=true;
      }
      if(grenade_pressed && !grenade_released && elapsed>=45 && elapsed%10==0)
        key(window_id,SDL_Scancode(binding),true);
    }
    if(lift_approach_target) {
      const auto delta=*lift_approach_target-p.position;
      const auto horizontal_squared=delta.x*delta.x+delta.y*delta.y;
      if(horizontal_squared<=interaction_content.player.radius*interaction_content.player.radius ||
          (!lift_jump_input && peak_z-initial_z>=interaction_content.player.height))lift_approach_finished=true;
      const bool approach=elapsed>=10 && !lift_approach_finished;
      key(window_id,SDL_SCANCODE_W,approach);
      if(approach) {
        const auto &settings=o.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Lift approach requires the actual mouse gain");
        const auto turn=std::remainder(std::atan2(delta.y,delta.x)-c.yaw,6.283185307179586f);
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(turn/gain));
        // Several fixed ticks can precede the next SDL event pump. Queue
        // one correction, then wait until actual input yaw consumes it.
        if(lift_pending_yaw_origin && c.yaw!=*lift_pending_yaw_origin)lift_pending_yaw_origin.reset();
        if(look.motion.xrel && !lift_pending_yaw_origin) {
          check(SDL_PushEvent(&look)==1,"SDL rejected lift approach look input");
          lift_pending_yaw_origin=c.yaw;
        }
      }
      if(lift_jump_input && !lift_jump_pressed && elapsed>=20 && p.native_grounded &&
          horizontal_squared<=lift_jump_heights*lift_jump_heights*interaction_content.player.height*interaction_content.player.height) {
        key(window_id,SDL_SCANCODE_SPACE,true);lift_jump_pressed=true;
      }
      if(lift_jump_pressed && !lift_jump_released && c.jump) {
        key(window_id,SDL_SCANCODE_SPACE,false);lift_jump_released=true;
      }
    }
    if(prop_watch) {
      Vec3 target=prop_watch->target;std::uint32_t newest{};
      for(const auto &root:s.source_scene_roots)if(prop_watch->placements.contains(root.placement) && root.created_tick>=newest) {
        target=root.transform.position;newest=root.created_tick;
        if(object_field_test)if(const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==root.object;});pose!=s.source_body_poses.end())target=pose->sphere.center;
      if(object_field_test && std::getenv("HALO2_TU15_OBJECT_FIELD_BODY_INPUT"))target=prop_watch->target;
      }
      if(object_field_test) {
        key(window_id,SDL_SCANCODE_W,!object_field_shoot && !object_field_grenade && elapsed>=20 && elapsed<100);
        key(window_id,SDL_SCANCODE_S,!object_field_shoot && !object_field_grenade && elapsed>=100 && elapsed<145);
        if(object_field_shoot) {
          const auto fire=[&](bool down) {
            const int binding=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Fire)];
            if(binding>0 && binding<SDL_NUM_SCANCODES)key(window_id,SDL_Scancode(binding),down);
            else {
              check(binding>pc_mouse_button(0) && binding<=pc_mouse_button(5),"Original interaction requires a keyboard or mouse fire binding");
              SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
              event.button.windowID=window_id;event.button.button=Uint8(binding-pc_mouse_button(0));
              event.button.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&event)==1,"SDL rejected physical object fire input");
            }
          };
          if(elapsed>=50 && !object_field_fire_pressed){fire(true);object_field_fire_pressed=true;}
          if(elapsed>=110 && object_field_fire_pressed && !object_field_fire_released){fire(false);object_field_fire_released=true;}
          object_field_fire_commands+=c.fire;
        }
        if(std::getenv("HALO2_TU15_OBJECT_FIELD_JUMP") && !object_field_jump && elapsed>=35 && p.native_grounded) {
          key(window_id,SDL_SCANCODE_SPACE,true);object_field_jump=true;
        }
        if(object_field_jump && !object_field_jump_released && c.jump) {key(window_id,SDL_SCANCODE_SPACE,false);object_field_jump_released=true;}
      }
      const auto delta=target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
      const auto desired=std::pair{std::atan2(delta.y,delta.x),object_field_grenade && std::getenv("HALO2_TU15_OBJECT_FIELD_DROP") && elapsed>=20 && elapsed<70?-1.5f:std::atan2(delta.z,std::hypot(delta.x,delta.y))};
      const auto current=std::pair{c.yaw,c.pitch};
      if(prop_pending_look && current!=*prop_pending_look)prop_pending_look.reset();
      if(elapsed>=5 && !prop_pending_look) {
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Prop watcher requires actual mouse gain");SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(std::remainder(desired.first-c.yaw,6.283185307f)/gain));
        look.motion.yrel=-int(std::lround((desired.second-c.pitch)/gain))*(settings.invert?-1:1);
        if(look.motion.xrel || look.motion.yrel){check(SDL_PushEvent(&look)==1,"SDL rejected live prop look input");prop_pending_look=current;}
      }
    }
    if(support_start) {
      auto target=support_start->center;
      const auto target_node=support_board_target_node.value_or(support_start->node);
      bool target_current=false;
      const auto root=std::find_if(s.source_scene_roots.begin(),s.source_scene_roots.end(),[&](const auto&r){return r.definition==support_start->definition;});
      if(root!=s.source_scene_roots.end())if(const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==root->object;});pose!=s.source_body_poses.end() && target_node<pose->nodes.size()) {
        target=pose->nodes[target_node].position;target_current=true;
      }
      const auto delta=target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
      const auto desired=std::pair{std::atan2(delta.y,delta.x),std::atan2(delta.z,std::hypot(delta.x,delta.y))};
      const auto current=std::pair{c.yaw,c.pitch};
      if(support_pending_look && current!=*support_pending_look)support_pending_look.reset();
      if(elapsed>=5 && !support_pending_look) {
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Support fixture requires the actual mouse gain");
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(std::remainder(desired.first-c.yaw,6.283185307f)/gain));
        look.motion.yrel=-int(std::lround((desired.second-c.pitch)/gain))*(settings.invert?-1:1);
        if(look.motion.xrel || look.motion.yrel) {check(SDL_PushEvent(&look)==1,"SDL rejected rotating support look input");support_pending_look=current;}
      }
      const auto boarding_range=std::hypot(target.x-p.position.x,target.y-p.position.y);
      if(support_board_jump&&target_current&&elapsed>=15&&!support_board_jump_pressed&&p.native_grounded) {
        key(window_id,SDL_SCANCODE_SPACE,true);support_board_jump_pressed=true;
      }
      if(support_board_jump_pressed&&!support_board_jump_released&&c.jump) {
        key(window_id,SDL_SCANCODE_SPACE,false);support_board_jump_released=true;
      }
      // v908's ordinary walk stops at the actual BSP lip. The optional
      // declared jump boards the authored shaft while the device is paused;
      // resume only after the real player lands at that input waypoint.
      if(elapsed>=30&&!support_started&&(!support_board_jump||
          (target_current&&support_board_jump_released&&p.native_grounded&&boarding_range<=interaction_content.player.radius))) {
        host->resume_support.store(true);support_started=true;
      }
      // The phase0 paddle starts below the BSP floor and extends into the
      // shaft. Follow its actual published node with ordinary SDL input until
      // the original grounded controller reports a real carrier contact.
      // Passive carry below excludes every nonneutral authority command.
      if(support_board_input)
        key(window_id,SDL_SCANCODE_W,target_current&&(support_started||((support_board_jump||support_board_floor_node)&&elapsed>=10)||(support_board_approach&&elapsed>=20))&&
            !host->support_contact_observed.load()&&boarding_range>interaction_content.player.radius);
    }
    if(control_start) {
      const auto delta=control_start->target-p.position;
      const auto look_target=(script_test || control_released)?control_start->gate:control_start->target;
      const auto look_delta=look_target-p.position;
      const auto eye=look_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
      const auto desired=std::pair{std::atan2(look_delta.y,look_delta.x),std::atan2(eye.z,std::sqrt(eye.x*eye.x+eye.y*eye.y))};
      const auto current=std::pair{c.yaw,c.pitch};
      if(control_pending_look && current!=*control_pending_look)control_pending_look.reset();
      if(elapsed>=5 && (!control_pressed || control_released) && !control_pending_look) {
        const auto &settings=o.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f;check(gain>0,"Control approach has invalid mouse gain");
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(std::remainder(desired.first-c.yaw,6.283185307179586f)/gain));
        look.motion.yrel=-int(std::lround((desired.second-c.pitch)/gain))*(settings.invert?-1:1);
        if(look.motion.xrel || look.motion.yrel) {
          check(SDL_PushEvent(&look)==1,"SDL rejected control approach look");control_pending_look=current;
        }
      }
      if(script_test && elapsed>=30 && !script_started) {
        host->start_device_script.store(true);script_started=true;
      }
      const bool shown=p.action_prompt.target.action==3 && p.action_prompt.definition==control_start->definition;
      key(window_id,SDL_SCANCODE_W,!script_test && elapsed>=10 && !control_pressed && !shown &&
          delta.x*delta.x+delta.y*delta.y>interaction_content.player.radius*interaction_content.player.radius);
      const int binding=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Pickup)];
      check(binding>0 && binding<SDL_NUM_SCANCODES,"Control test requires the actual pickup key");
      if(!script_test && shown && !control_pressed) {key(window_id,SDL_Scancode(binding),true);control_pressed=true;}
      if(control_pressed && !control_released && c.pickup_milliseconds>=
          source_pickup_hold_threshold(interaction_content.pickup_hold_seconds)+100u) {
        key(window_id,SDL_Scancode(binding),false);control_released=true;
      }
    }
    if(player_rearm_test) {
      if(rearm_last_tick!=s.tick) {
        if(rearm_first_press && rearm_previous_grounded && !p.native_grounded && p.velocity.z>0) {
          if(rearm_second_press)++rearm_second_launches;else ++rearm_held_launches;
        }
        if(rearm_first_press && !rearm_first_released && elapsed>=*rearm_first_press+3) {
          rearm_held_commands+=c.jump;rearm_missing_held_commands+=!c.jump;
        }
        if(rearm_second_launches)rearm_landed_ticks=p.native_grounded?rearm_landed_ticks+1:0;
        rearm_previous_grounded=p.native_grounded;rearm_last_tick=s.tick;
      }
      if(!rearm_first_press && elapsed>=30 && p.native_grounded) {
        key(window_id,SDL_SCANCODE_SPACE,true);rearm_first_press=elapsed;
      }
      if(rearm_first_press && !rearm_first_released && elapsed>=*rearm_first_press+180) {
        key(window_id,SDL_SCANCODE_SPACE,false);rearm_first_released=true;
      }
      if(rearm_first_press && rearm_first_released && !rearm_second_press &&
          elapsed>=*rearm_first_press+190 && p.native_grounded) {
        key(window_id,SDL_SCANCODE_SPACE,true);rearm_second_press=elapsed;
      }
      if(rearm_second_press && !rearm_second_released && elapsed>=*rearm_second_press+10) {
        key(window_id,SDL_SCANCODE_SPACE,false);rearm_second_released=true;
      }
      if(rearm_second_launches==1)host->capture_world_requested.store(true);
    }
    if(!player_rearm_test&&!lift&&!vehicle&&!objects&&!control_test&&!script_test&&!support_test&&!prop_watch_test&&!conveyor_test) switch(elapsed) {
      case 25:focus(window);key(window_id,SDL_SCANCODE_W,true);break;
      case 40:key(window_id,SDL_SCANCODE_W,false);break;
      case 50:key(window_id,SDL_SCANCODE_LCTRL,true);break;
      case 65:key(window_id,SDL_SCANCODE_LCTRL,false);break;
      case 85:focus(window);key(window_id,SDL_SCANCODE_SPACE,true);key(window_id,SDL_SCANCODE_SPACE,false);break;
    }
    if(vehicle && !vehicle_blast_test && vehicle_exited && exit_camera_frames>=40 && ordinary_after_exit_frames>0)quit();
    if(accompanied_driver && companion_entered_tick && !vehicle_entered &&
        elapsed-*companion_entered_tick>=90)quit();
    // Every support input, including a declared device phase, must observe
    // enough real authority ticks to satisfy the shared170-sample minimum.
    if(elapsed>=(player_rearm_test?450u:object_field_test?object_field_ticks:prop_watch_test?900u:support_test?240u:script_test?600u:control_test?900u:accompanied_driver?900u:vehicle_blast_test?1800u:vehicle?(long_vehicle_sequence?900u:450u):180u))quit();
  };
  // Finite local screenshots retain all earlier real input delivery. Network
  // exit uses SDL_QUIT because exact tick capture deliberately rejects LAN.
  if(!lan){o.capture_tick=vehicle?(long_vehicle_sequence?901:451):181;o.screenshot=output/"frame.ppm";}
  int result{};
  try {result=run_multiplayer(o);if(host)host->finish();}
  catch(const std::exception &error) {
    if(vehicle_blast_test && host) {
      try {host->finish();}catch(const std::exception &authority_error) {
        report["authority_error"]=authority_error.what();
      }
      report["acceptance_passed"]=false;report["error"]=error.what();
      report["original_player_birth_scene_inputs"]=host->scene_lifecycle;
      report["actual_authority_point_impulses"]=host->impulses.size();
      report["target_vitality_on_failure"]=Json::array();
      for(const auto &sample:host->vehicle_vitality_samples)
        if(sample.at("object")==driven_vehicle) {
          auto &retained=report["target_vitality_on_failure"];
          if(retained.size()==32u)retained.erase(retained.begin());
          retained.push_back(sample);
        }
      std::ofstream(output/"failure.json")<<report.dump(2)<<'\n';
    }
    throw;
  }
  trace.close();
  if(vehicle_blast_test && host)report["original_player_birth_scene_inputs"]=host->scene_lifecycle;
  if(support_test) {
    std::ofstream file(output/"authority-support-samples.json");file<<host->support_samples.dump(2)<<'\n';
    check(bool(file),"Could not retain original rotating support observations");
    // Retain compact actual controller evidence even when full unit/motion
    // banks are discarded by the managed test-storage lifecycle.
    Json compact=Json::array();
    for(const auto &row:host->support_samples)if(row.contains("support_controller"))
      compact.push_back({{"tick",row.at("tick")},{"unit",row.at("unit")},{"carrier",row.at("wheel")},
          {"position",row.at("position")},{"grounded",row.at("grounded")},{"resumed",row.at("resumed")},
          {"source_power_bits",row.at("wheel_power_bits")},{"source_phase_bits",row.at("wheel_position_bits")},
          {"delivered_controls",row.at("delivered_controls")},{"node_position",row.at("node_position")},
          {"controller",row.at("support_controller")}});
    std::ofstream controller_file(output/"authority-support-controller.json");controller_file<<compact.dump()<<'\n';
    check(bool(controller_file),"Could not retain original support controller observations");
  }
  if(script_test) {std::ofstream file(output/"authority-script-stages.json");file<<host->script_stages.dump(2)<<'\n';
    check(bool(file),"Could not retain actual visible script stages");}
  if(lan) {
    Json comparisons=Json::array();
    for(const auto &p:authority_samples) {
      const bool same=std::any_of(host->history.begin(),host->history.end(),[&](const auto &h) {
        if(h.tick!=p.tick || h.source_contrails!=p.contrails)return false;const auto &a=h.players[p.slot];
        try {test::same_device_snapshot(h.devices,p.devices);}catch(const FormatError &){return false;}
        const auto same_poses=[&] {
          if(h.source_body_poses.size()!=p.poses.size() || h.world_vehicles.size()!=p.vehicles.size())return false;
          if(h.source_scene_roots.size()!=p.scene_roots.size())return false;
          if(h.source_dynamic_objects.size()!=p.dynamic_objects.size())return false;
          for(unsigned n=0;n<p.dynamic_objects.size();++n)if(!same_dynamic(h.source_dynamic_objects[n],p.dynamic_objects[n]))return false;
          for(unsigned n=0;n<p.scene_roots.size();++n) {
            const auto &x=h.source_scene_roots[n],&y=p.scene_roots[n];
            if(x.object!=y.object || x.definition!=y.definition || x.placement!=y.placement || x.created_tick!=y.created_tick ||
                x.visible!=y.visible || x.has_world_nodes!=y.has_world_nodes ||
                x.runtime_flags!=y.runtime_flags || x.frozen!=y.frozen ||
                x.render_clusters!=y.render_clusters ||
                std::memcmp(&x.velocity,&y.velocity,sizeof(Vec3)) ||
                std::memcmp(&x.transform,&y.transform,sizeof(Transform)))return false;
          }
          for(unsigned i=0;i<p.poses.size();++i) {
            const auto &x=h.source_body_poses[i],&y=p.poses[i];
            if(x.object!=y.object || x.nodes.size()!=y.nodes.size() ||
                std::memcmp(&x.sphere,&y.sphere,sizeof(x.sphere)) ||
                std::memcmp(x.nodes.data(),y.nodes.data(),x.nodes.size()*sizeof(Transform)))return false;
          }
          for(unsigned i=0;i<p.vehicles.size();++i) {
            const auto &x=h.world_vehicles[i],&y=p.vehicles[i];
            if(x.handle!=y.handle || x.placement!=y.placement || std::memcmp(&x.transform,&y.transform,sizeof(Transform)))return false;
          }
          return true;
        };
        return a.source_player==p.player && a.source_unit==p.unit && a.source_parent==p.parent && a.source_seat==p.seat &&
            a.position.x==p.position.x && a.position.y==p.position.y && a.position.z==p.position.z &&
            a.velocity.x==p.velocity.x && a.velocity.y==p.velocity.y && a.velocity.z==p.velocity.z && a.crouch==p.crouch && same_poses() && [&] {
          if(h.grenade_detonations.size()!=p.detonations.size())return false;
          for(unsigned n=0;n<p.detonations.size();++n) {
            const auto &x=h.grenade_detonations[n],&y=p.detonations[n];
            if(x.source_locations.size()!=y.source_locations.size())return false;
            for(unsigned j=0;j<x.source_locations.size();++j)
              if(x.source_locations[j].name!=y.source_locations[j].name ||
                  std::memcmp(&x.source_locations[j].frame,&y.source_locations[j].frame,sizeof(Transform)))return false;
            if(x.serial!=y.serial || x.tick!=y.tick || x.projectile!=y.projectile ||
                x.definition!=y.definition || x.effect!=y.effect || x.player!=y.player || x.unit!=y.unit ||
                std::memcmp(&x.position,&y.position,sizeof(Vec3)) ||
                std::memcmp(&x.forward,&y.forward,sizeof(Vec3)) ||
                std::memcmp(&x.up,&y.up,sizeof(Vec3)))return false;
          }
          if(h.grenade_projectiles.size()!=p.grenades.size())return false;
          for(unsigned n=0;n<p.grenades.size();++n) {
            const auto &x=h.grenade_projectiles[n],&y=p.grenades[n];
            if(x.datum!=y.datum || x.definition!=y.definition || x.parent!=y.parent ||
                x.player!=y.player || x.unit!=y.unit || x.designator!=y.designator ||
                x.native_owner!=y.native_owner || x.type!=y.type || x.flags!=y.flags ||
                x.state!=y.state || x.age!=y.age ||
                std::memcmp(&x.position,&y.position,sizeof(Vec3)) ||
                std::memcmp(&x.velocity,&y.velocity,sizeof(Vec3)) ||
                std::memcmp(&x.forward,&y.forward,sizeof(Vec3)) ||
                std::memcmp(&x.up,&y.up,sizeof(Vec3)))return false;
            for(const auto pair:{std::pair{x.timer,y.timer},std::pair{x.timer_step,y.timer_step},
                std::pair{x.arming,y.arming},std::pair{x.arming_step,y.arming_step},std::pair{x.traveled,y.traveled}})
              if(std::bit_cast<std::uint32_t>(pair.first)!=std::bit_cast<std::uint32_t>(pair.second))return false;
          }
          return true;
        }();
      });
      comparisons.push_back({{"tick",p.tick},{"player",p.player},{"unit",p.unit},{"exact",same}});
      matched_snapshots+=same;
    }
    std::ofstream file(output/"authority-comparisons.json");file<<comparisons.dump(2)<<'\n';
    check(bool(file),"Could not retain actual authority comparisons");
    check(matched_snapshots==authority_samples.size(),"LAN snapshot disagrees with every actual same-tick authority publication");
  }
  if(lan) {std::ofstream file(output/"authority-script-status.json");file<<host->script_status.dump(2)<<'\n';
    check(bool(file),"Could not retain actual authority map script status");}
  report["runtime_exit"]=result;report["observed_input_ticks"]=samples.size();report["submitted_frames"]=frames.size();
  report["local_slot"]=local_slot;report["initial_position"]=xyz(initial);report["maximum_distance"]=distance;
  report["initial_z"]=initial_z;report["peak_z"]=peak_z;report["maximum_upward_velocity"]=fastest;
  report["maximum_crouch"]=largest_crouch;report["jump_commands"]=jump_commands;report["forward_commands"]=forward_commands;
  report["exact_lan_snapshot_comparisons"]=matched_snapshots;
  report["adopted_source_scene_frames"]=adopted_scene_frames;
  report["adopted_source_dynamic_frames"]=adopted_dynamic_frames;
  bool passed=result==0 && started && samples.size()>=170 && frames.size()>0;
  if(lan)passed&=adopted_scene_frames>0;
  if(lan && std::getenv("HALO2_TU15_REQUIRE_MAP_SCRIPTS"))passed&=host->script_status.at("diagnostics").empty();
  if(lift) {
    const auto body_height=report.at("authored_lift_player_height").get<float>();
    check(body_height>0,"Lift test requires the actual authored player height");
    std::optional<float> first_delivery_z;unsigned upward_deliveries{};
    const auto phase_path=output/"authority-phantom-phase.jsonl";
    if(std::filesystem::exists(phase_path)) {
      const auto scalar=[](const std::string &hex,unsigned offset) {
        check(hex.size()>=2u*(offset+4u),"Incomplete original phantom trace bank");
        std::uint32_t bits{};for(unsigned n=0;n<4;++n)
          bits|=std::uint32_t(std::stoul(hex.substr((offset+n)*2u,2u),nullptr,16))<<(n*8u);
        return std::bit_cast<float>(bits);
      };
      std::ifstream phase(phase_path);std::string line;
      std::map<std::pair<std::uint32_t,std::uint32_t>,Json> before;
      while(std::getline(phase,line)) {
        const auto event=Json::parse(line);const auto tick=event.at("tick").get<std::uint32_t>();
        const auto selected_unit=samples.front().at("unit").get<std::uint32_t>();
        const auto key=std::pair{tick,event.at("component").get<std::uint32_t>()};
        for(const auto &actor:event.at("players")) {
          if(actor.at("slot").get<unsigned>()!=local_slot || actor.at("object").get<std::uint32_t>()!=selected_unit || !actor.contains("motion_bytes"))continue;
          if(event.at("entering").get<bool>())before[key]=actor;
          else if(const auto prior=before.find(key);prior!=before.end()) {
            if(scalar(actor.at("motion_bytes").get<std::string>(),0x48u)>
                scalar(prior->second.at("motion_bytes").get<std::string>(),0x48u)) {
              ++upward_deliveries;
              if(!first_delivery_z)first_delivery_z=scalar(actor.at("body_bytes").get<std::string>(),0x6cu);
            }
            before.erase(prior);
          }
        }
      }
      report["actual_actor_upward_phantom_deliveries"]=upward_deliveries;
      report["phantom_comparison_window"]="Actual joined full unit generation, including source updates before its first rendered client tick";
      if(first_delivery_z)report["first_upward_phantom_delivery_z"]=*first_delivery_z;
    }
    bool lifted=peak_z-initial_z>=body_height && fastest>0;
    if(lift_approach_target) {
      lifted&=forward_commands>0;
      if(std::filesystem::exists(phase_path))lifted&=upward_deliveries>0;
    }
    if(lift_jump_input) {
      // Jump height alone cannot pass: the original force must reach this
      // actor and it must rise another authored body height after entry.
      lifted&=jump_commands>0 && upward_deliveries>0 && first_delivery_z &&
          peak_z-*first_delivery_z>=body_height;
      report["lift_jump_input"]=true;
    } else if(lift_approach_target)lifted&=jump_commands==0;
    report["lift_travel_observed"]=lifted;report["lift_height_threshold"]=body_height;
    passed&=lifted;
  }
  else if(vehicle) {
    std::ofstream(output/"player-body-frames.json")<<body_frames.dump(2)<<'\n';
    report["submitted_seated_player_bodies_by_view"]=seated_body_draws;
    report["submitted_authoritative_corpse_bodies_by_view"]=submitted_corpse_draws;
    report["submitted_exited_player_bodies_by_view"]=exited_body_draws;
    passed&=!report["initial_inventory"]["authored_vehicle_candidates"].empty() &&
      driven_vehicle!=null_datum;
    if(seat_invalid_for_player) {
      report["original_seat_invalid_for_player"]=true;
      report["invalid_player_seat_prompts"]=invalid_seat_prompts;
      report["ordinary_camera_frames_for_rejected_seat"]=ordinary_rejected_seat_camera_frames;
      report["source_seat_entered"]=vehicle_entered;
      report["source_seat_rejection_verified"]=!vehicle_entered && !invalid_seat_prompts && ordinary_rejected_seat_camera_frames>0;
      passed&=report["source_seat_rejection_verified"].get<bool>() && (!lan || matched_snapshots==samples.size());
    } else {
    passed&=seated_body_draws[0]>0 && seated_body_draws[1]>0 && exited_body_draws[1]>0;
    report["driving_and_solved_contacts_verified"]=false;
    report["source_driver_entered"]=vehicle_entered;report["source_driver_exited"]=vehicle_exited;
    report["source_seat_entered"]=vehicle_entered;report["source_seat_exited"]=vehicle_exited;
    report["source_vehicle_maximum_distance"]=vehicle_distance;report["driver_forward_commands"]=vehicle_drive_commands;
    report["source_driver_horizontal_distance"]=driven_distance;
    report["source_vehicle_climb"]=vehicle_climb;
    // The normal flight case retains its original climb/travel gates. The
    // separate BSP-wall case measures contact, stop and exit instead.
    if(!vehicle_collision_test && report["selected_vehicle"]=="banshee")passed&=vehicle_climb>2.f && driven_distance>10.f;
    report["source_driving_observed"]=vehicle_entered && vehicle_drive_commands>0 && driven_distance>1;
    report["driver_steering_commands"]=steering_commands;report["source_driver_steering_radians"]=steering_angle;
    report["observer_backward_commands"]=observer_back_commands;report["observer_maximum_distance"]=observer_distance;
    report["source_driver_controls_completed"]=vehicle_entered && vehicle_exited &&
        vehicle_drive_commands>0 && driven_distance>1 && steering_commands>0 && steering_angle>.05f;
    const bool driver=(vehicle_start->flags&4u)!=0;
    report["source_selected_seat_completed"]=vehicle_entered && vehicle_exited && vehicle_drive_commands>0 &&
        steering_commands>0;
    if(vehicle_collision_test) {
      report["collision_peak_horizontal_speed"]=collision_peak_speed;
      report["collision_minimum_plane_distance"]=collision_minimum_plane_distance;
      report["collision_stopped_under_throttle_ticks"]=collision_stopped_ticks;
      passed&=driver && vehicle_entered && vehicle_exited && driven_distance>1.f && collision_peak_speed>1.f && collision_stopped_ticks>=8;
    } else if(fixed_turret_test) {
      report["fixed_turret_fire_commands"]=turret_fire_commands;
      report["fixed_turret_original_firing_effects"]=turret_effect_serials.size();
      report["fixed_turret_actual_projectiles"]=turret_projectile_births.size();
      report["fixed_turret_actual_projectile_travel"]=turret_projectile_travel;
      report["fixed_turret_actual_projectile_speed"]=turret_projectile_speed;
      report["fixed_turret_projectile_geometry_by_view"]=turret_projectile_draws;
      report["fixed_turret_projectile_glows_by_view"]=turret_glow_draws;
      report["fixed_turret_original_contrails_by_view"]=turret_contrail_draws;
      report["fixed_turret_colored_area_contrails_by_view"]=turret_visible_contrail_draws;
      report["fixed_turret_completed_world_node_turn"]=turret_node_turn;
      report["fixed_turret_controls_completed"]=vehicle_entered && vehicle_exited && steering_commands>0 &&
          turret_fire_commands>0 && !turret_projectile_births.empty() && turret_projectile_travel>.25f &&
          turret_projectile_speed>0.f && turret_node_turn>.05f &&
          (turret_projectile_draws[0]>0 || turret_glow_draws[0]>0 || turret_visible_contrail_draws[0]>0) &&
          (turret_projectile_draws[1]>0 || turret_glow_draws[1]>0 || turret_visible_contrail_draws[1]>0);
      if(turret_damage_test) {
        float minimum=1.f;unsigned target_impulses{};
        for(const auto &sample:host->vehicle_vitality_samples)if(sample.at("object")==turret_damage_target)minimum=std::min(minimum,sample.at("body").get<float>());
        for(const auto &event:host->impulses)target_impulses+=event.at("object").get<std::uint32_t>()==turret_damage_target;
        report["fixed_turret_damage_target"]=turret_damage_target;report["fixed_turret_target_body_minimum"]=minimum;
        report["fixed_turret_target_impulses"]=target_impulses;report["fixed_turret_target_geometry_by_view"]=turret_damage_draws;
        report["fixed_turret_controls_completed"]=vehicle_entered && vehicle_exited && steering_commands>0 && turret_fire_commands>0 &&
            turret_node_turn>.05f && minimum<.999f && turret_damage_draws[0]>0 && turret_damage_draws[1]>0;
        std::ofstream(output/"authority-turret-target-vitality.json")<<host->vehicle_vitality_samples.dump(2)<<'\n';
      }
      passed&=report["fixed_turret_controls_completed"].get<bool>() && (!lan || matched_snapshots==samples.size());
    } else passed&=(driver?report["source_driver_controls_completed"].get<bool>():
        report["source_selected_seat_completed"].get<bool>()) && (!lan || matched_snapshots==samples.size());
      if(vehicle_blast_test) {
        unsigned target_impulses{};for(const auto &event:host->impulses)target_impulses+=event.at("object").get<std::uint32_t>()==driven_vehicle;
        float minimum_body=1.f;unsigned vital_samples{},dead_samples{};
        for(const auto &event:host->vehicle_vitality_samples)if(event.at("object").get<std::uint32_t>()==driven_vehicle) {
          ++vital_samples;minimum_body=std::min(minimum_body,event.at("body").get<float>());
          dead_samples+=bool(event.at("original_damage_flags").get<unsigned>()&4u);
        }
        report["vehicle_blast_grenade_commands"]=grenade_commands;report["vehicle_blast_detonated"]=detonated;
        report["vehicle_blast_attached_grenade"]=attached_grenade;
        report["vehicle_blast_released_grenade"]=released_grenade;
        report["vehicle_blast_target_impulses"]=target_impulses;report["vehicle_minimum_original_body_fraction"]=minimum_body;
        report["vehicle_original_vitality_samples"]=vital_samples;
        {std::ofstream file(output/"authority-vehicle-vitality.json");file<<host->vehicle_vitality_samples.dump(2)<<'\n';check(bool(file),"Could not retain source vehicle vitality");}
        report["vehicle_requested_blast_count"]=vehicle_blast_count;report["vehicle_observed_blast_detonations"]=vehicle_blast_detonations;
        report["vehicle_weapon_fire_commands"]=vehicle_weapon_commands;report["vehicle_weapon_reload_commands"]=vehicle_reload_commands;
        if(vehicle_weapon_test)passed&=vehicle_weapon_commands>0 && target_impulses>0 && vital_samples>0 && minimum_body<=0.f && dead_samples>0;
        else passed&=grenade_commands==vehicle_blast_count && attached_grenade && released_grenade && detonated &&
            vehicle_blast_detonations>=vehicle_blast_count && target_impulses>=vehicle_blast_count && vital_samples>0;
        report["vehicle_original_dead_flag_samples"]=dead_samples;
        report["vehicle_original_debris_objects"]=first_debris_position.size();
        report["vehicle_original_debris_maximum_displacement"]=maximum_debris_displacement;
        std::array<unsigned,2> visible_parts{};
        for(const auto &[object,draws]:debris_draws)for(unsigned view=0;view<visible_parts.size();++view)visible_parts[view]+=draws[view]>0;
        report["vehicle_original_debris_visible_parts_by_view"]=visible_parts;
        report["vehicle_original_debris_submitted_parts_by_view"]=visible_parts;
        report["vehicle_original_debris_pixel_visibility_verified"]=false;
        report["vehicle_blast_acceptance_scope"]="Original death/impulse/debris motion, LAN equality and draw submissions; pixel visibility and audio require separate verification";
        if(vehicle_blast_count==2 || vehicle_weapon_test)passed&=minimum_body<=0.f && dead_samples>0 && !first_debris_position.empty() && maximum_debris_displacement>.01f && visible_parts[0]>0 && visible_parts[1]>0;
      }
    if(accompanied_driver) {
      report["companion_driver_entered"]=companion_entered;
      report["companion_driver_forward_commands"]=companion_forward_commands;
      report["elapsed_clock"]="Actual authority ticks; input samples may repeat a completed snapshot";
      report["companion_original_camera_frames"]=companion_camera_frames;
      report["moving_occupied_vehicle_distance"]=driven_distance;
      passed&=companion_entered && companion_forward_commands>0 && companion_camera_frames>0 && driven_distance>1.f;
    } else passed&=observer_back_commands>0 && observer_distance>.5f;
    report["submitted_seated_camera_frames"]=seated_camera_frames;
    report["submitted_exact_source_camera_frames"]=exact_source_camera_frames;
    report["submitted_original_third_person_frames"]=stable_source_third_person_frames;
    // Authored collision/track output owns the distance. A fixed half-unit
    // minimum rejected a bit-exact TU1.5 turret observer at0.498986 units.
    if(lan)passed&=exact_source_camera_frames==seated_camera_frames && stable_source_third_person_frames>0;

    report["submitted_source_exit_transition_frames"]=source_exit_transition_frames;
    report["submitted_ordinary_after_exit_frames"]=ordinary_after_exit_frames;
    report["submitted_camera_distance_from_first_person_eye"]=seated_camera_distance;
    report["submitted_camera_frames_after_exit"]=exit_camera_frames;
    passed&=seated_camera_frames>0 && exit_camera_frames>0;
    if(lan)passed&=source_exit_transition_frames>0 && ordinary_after_exit_frames>0;
    report["submitted_seated_camera_look_turn_radians"]=steering_camera_angle;
    passed&=steering_camera_angle>.05f;
    report["submitted_driver_vehicle_geometry_by_view"]=driven_vehicle_draws;
    passed&=driven_vehicle_draws[0]>0 && driven_vehicle_draws[1]>0;
    report["camera_acceptance_passed"]=vehicle_entered && vehicle_exited &&
        seated_camera_frames>0 && steering_camera_angle>.05f &&
        exit_camera_frames>0 && seated_body_draws[0]>0 && seated_body_draws[1]>0 &&
        exited_body_draws[1]>0 && driven_vehicle_draws[0]>0 && driven_vehicle_draws[1]>0 &&
        (!lan || (exact_source_camera_frames==seated_camera_frames && stable_source_third_person_frames>0 &&
          matched_snapshots==samples.size() && source_exit_transition_frames>0 &&
          ordinary_after_exit_frames>0)) && (!accompanied_driver ||
          (companion_entered && companion_forward_commands>0 && companion_camera_frames>0 && driven_distance>1.f));
    }
  } else if(objects) {
    report["grenade_commands"]=grenade_commands;report["attached_grenade"]=attached_grenade;
    report["released_grenade"]=released_grenade;report["falling_grenade"]=falling_grenade;
    report["detonated"]=detonated;report["grenade_draws_by_view"]=grenade_draws;
    report["explosion_particle_draws_by_view"]=explosion_particle_draws;
    report["dynamic_prop_impulses_verified"]=false;
    if(prop_test) {
      report["source_prop_displacement_after_explosion"]=prop_displacement;
      report["source_prop_retired"]=prop_retired;
      passed&=prop_displacement>.01f || prop_retired;
    }
    passed&=grenade_commands==1 && attached_grenade && released_grenade && falling_grenade && detonated;
    for(unsigned view=0;view<o.local_players;++view)passed&=grenade_draws[view]>0 && explosion_particle_draws[view]>0;
  } else if(object_field_test) {
    unsigned angular_deliveries{};const auto path=output/"authority-phantom-phase.jsonl";
    check(std::filesystem::exists(path),"Object-field acceptance requires its actual source force phases");
    std::ifstream file(path);std::string line;std::map<std::pair<unsigned,unsigned>,Json> prior;
    while(std::getline(file,line)) {
      const auto event=Json::parse(line);const auto key=std::pair{event.at("tick").get<unsigned>(),event.at("component").get<unsigned>()};
      if(event.at("entering").get<bool>()){prior[key]=event;continue;}
      const auto first=prior.find(key);if(first==prior.end())continue;
      for(const auto &member:event.at("members")) {
        const auto object=member.at("object").get<std::uint32_t>();if(!prop_watch_first.contains(object) || !member.contains("motion_bytes"))continue;
        for(const auto &before:first->second.at("members"))if(before.at("object")==object && before.contains("motion_bytes")) {
          const auto a=before.at("motion_bytes").get<std::string>(),b=member.at("motion_bytes").get<std::string>();
          check(a.size()>=0xb8 && b.size()>=0xb8,"Incomplete actual angular motion state");
          angular_deliveries+=a.substr(0xa0,24)!=b.substr(0xa0,24);
        }
      }
      prior.erase(first);
    }
    Json targets=Json::array();unsigned rotating_drawn{};
    for(const auto &[object,angle]:object_field_angles) {
      targets.push_back({{"object",object},{"angle",angle},{"travel",prop_watch_travel[object]},{"draw_submissions",prop_watch_draws[object]}});
      rotating_drawn+=angle>.05f && prop_watch_draws[object]>0;
    }
    report["declared_object_observation_ticks"]=object_field_ticks;
    report["object_field_targets"]=targets;report["original_angular_deliveries"]=angular_deliveries;
    report["aimed_prop_camera_frames"]=prop_watch_aimed;report["declared_object_jump"]=object_field_jump;
    report["object_field_fire_commands"]=object_field_fire_commands;
    unsigned target_impulses{};
    for(const auto &impulse:host->impulses)target_impulses+=prop_watch_first.contains(impulse.at("object").get<std::uint32_t>());
    report["object_field_original_target_impulses"]=target_impulses;
    report["grenade_commands"]=grenade_commands;report["grenade_detonated"]=detonated;
    report["attached_grenade"]=attached_grenade;report["released_grenade"]=released_grenade;
    passed&=angular_deliveries>0 && rotating_drawn>0 && prop_watch_aimed>10 &&
        (object_field_grenade?(grenade_commands==1 && attached_grenade && released_grenade && detonated && target_impulses>0):
          object_field_shoot?(object_field_fire_commands>0 && target_impulses>0):forward_commands>0);
  } else if(prop_watch_test) {
    Json trajectories=Json::array();unsigned moving_drawn{},conveyor_drawn{};
    for(const auto &[object,travel]:prop_watch_travel) {
      const auto draws=prop_watch_draws[object];const auto horizontal=prop_watch_horizontal[object],drop=prop_watch_drop[object];
      trajectories.push_back({{"object",object},{"travel",travel},{"horizontal",horizontal},{"drop",drop},{"draw_submissions",draws}});
      moving_drawn+=travel>.1f && draws>0;conveyor_drawn+=horizontal>.25f && drop>.25f && draws>0;
    }
    report["actual_scripted_prop_trajectories"]=trajectories;report["moving_drawn_generations"]=moving_drawn;
    report["conveyor_drawn_generations"]=conveyor_drawn;report["aimed_prop_camera_frames"]=prop_watch_aimed;
    passed&=moving_drawn>=2 && conveyor_drawn>=2 && prop_watch_aimed>10 && forward_commands==0 && jump_commands==0 && host->script_status.at("diagnostics").empty();
  } else if(support_test) {
    const auto vector=[](const Json &v){return Vec3{v.at(0).get<float>(),v.at(1).get<float>(),v.at(2).get<float>()};};
    std::optional<Vec3> origin,local_origin;std::optional<float> initial_phase;
    unsigned static_supported{},moving_supported{},passive_source_ticks{};float carried{},wheel_motion{};
    test::OriginalSupportCarryTu15 actual_carry(interaction_content.player.radius);
    for(const auto &row:host->support_samples) {
      // Worker teardown may execute further real frames after the visible
      // client's last input. Count only the observed visible interval.
      if(row.at("tick").get<std::uint32_t>()>samples.back().at("authority_tick").get<std::uint32_t>())continue;
      if(row.at("unit").get<std::uint32_t>()!=samples.front().at("unit").get<std::uint32_t>())continue;
      if(support_board_input) {
        const auto &input=row.at("delivered_controls");
        if(input.at("forward").get<float>()!=0.f || input.at("right").get<float>()!=0.f || input.at("jump").get<bool>())continue;
        if(row.at("resumed").get<bool>())++passive_source_ticks;
      }
      const auto position=vector(row.at("position"));
      const auto phase=std::bit_cast<float>(row.at("wheel_position_bits").get<std::uint32_t>());
      const auto power=std::bit_cast<float>(row.at("wheel_power_bits").get<std::uint32_t>());
      const bool grounded=row.at("grounded").get<bool>();
      const auto &controller=row.at("support_controller"),&words=controller.at("matrix_words");
      check(words.size()==13,"Incomplete original support transform");
      const auto component=[&](unsigned n){return std::bit_cast<float>(words.at(n).get<std::uint32_t>());};
      Transform support_transform;support_transform.scale=component(0);
      for(unsigned axis=0;axis<3;++axis)support_transform.basis[axis]={component(1+axis*3),component(2+axis*3),component(3+axis*3)};
      support_transform.position={component(10),component(11),component(12)};
      actual_carry.observe({.tick=row.at("tick").get<std::uint32_t>(),.owner=controller.at("owner").get<std::uint32_t>(),
          .carrier=row.at("wheel").get<std::uint32_t>(),.body_index=controller.at("body_index").get<std::int32_t>(),
          .body_count=controller.at("body_count").get<unsigned>(),.resumed=row.at("resumed").get<bool>(),
          .grounded=grounded,.power=power,.player_position=position,.support_transform=support_transform});
      const auto &basis=row.at("node_basis");const auto node=vector(row.at("node_position"));
      const auto scale=row.at("node_scale").get<float>();check(scale>0,"Original support node has invalid scale");
      if(!row.at("resumed").get<bool>()) {
        if(grounded && power==0)++static_supported;
        origin=position;const auto delta=position-node;
        local_origin=Vec3{dot(delta,vector(basis.at(0)))/scale,dot(delta,vector(basis.at(1)))/scale,dot(delta,vector(basis.at(2)))/scale};
        initial_phase=phase;continue;
      }
      if(!origin || !local_origin || !initial_phase)continue;
      wheel_motion=std::max(wheel_motion,std::abs(phase-*initial_phase));
      const auto expected=node+(vector(basis.at(0))*local_origin->x+vector(basis.at(1))*local_origin->y+vector(basis.at(2))*local_origin->z)*scale;
      if(grounded && power==1 && length(position-expected)<.5f*interaction_content.player.height &&
          length(position-*origin)>.1f) {++moving_supported;carried=std::max(carried,length(position-*origin));}
    }
    report["support_living_observation_last_tick"]=samples.back().at("authority_tick");
    report["released_support_unit_observations"]=std::count_if(host->support_samples.begin(),host->support_samples.end(),[](const auto &row){return row.value("released_player_body",false);});
    report["paused_grounded_ticks"]=actual_carry.paused_grounded_ticks;
    report["paused_original_support_ticks"]=actual_carry.paused_actual_carrier_ticks;
    report["moving_original_support_ticks"]=actual_carry.moving_ticks;
    report["original_support_carried_distance"]=actual_carry.maximum_carried;
    report["declared_node_moving_support_ticks"]=moving_supported;
    report["declared_node_carried_distance"]=carried;
    report["original_support_controller_body_carry"]=true;
    report["original_support_relative_tolerance"]=interaction_content.player.radius;
    Json segments=Json::array();for(const auto &segment:actual_carry.segments)
      segments.push_back({{"owner",segment.owner},{"body_index",segment.body_index},
          {"first_tick",segment.first_tick},{"last_tick",segment.last_tick},{"moving_ticks",segment.moving_ticks},
          {"carried",segment.carried},{"expected_displacement",segment.expected_displacement},
          {"maximum_relative_error",segment.maximum_relative_error}});
    report["original_support_body_segments"]=segments;
    report["original_wheel_position_delta"]=wheel_motion;
    report["original_support_resume_command"]=support_started;
    report["ordinary_support_boarding_input"]=support_board_input;
    report["ordinary_support_boarding_jump"]=support_board_jump;
    if(support_board_target_node)report["ordinary_support_boarding_target_node"]=*support_board_target_node;
    if(support_board_floor_node)report["ordinary_support_boarding_floor_node"]=*support_board_floor_node;
    report["ordinary_support_boarding_surveyed_approach"]=support_board_approach;
    report["passive_neutral_authority_ticks"]=passive_source_ticks;
    if(support_board_input)report["stimulus"]="Unmodified phase0 BSP-floor approach; ordinary SDL walk/look and optional declared jump to the authored node, then passive carry counted only for neutral delivered authority commands";
    if(support_board_floor_node)report["stimulus"]="Unmodified phase0 device; declared real BSP floor below the authored level node, ordinary SDL walk/look into the shaft; carry requires actual controller and neutral authority commands";
    if(support_board_approach)report["stimulus"]="Unmodified phase0 device and clear authored BSP-floor approach selected by a declared geometry survey; ordinary SDL walk/look; carry still requires actual controller and neutral authority commands";
    passed&=support_started && actual_carry.paused_grounded_ticks>=5 && actual_carry.moving_ticks>=5 && actual_carry.maximum_carried>.25f && wheel_motion>.01f &&
        (support_board_jump?(support_board_jump_released&&jump_commands>0):jump_commands==0) &&
        (support_board_input?(host->support_contact_observed.load()&&forward_commands>0&&passive_source_ticks>=5):forward_commands==0);
  } else if(script_test) {
    report["declared_device_script_started"]=script_started;
    report["device_script_stages"]=host->script_stages;
    check(script_started && host->script_stages.size()==8,"Visible device script did not complete eight stages");
    const auto bits=[](float value){return std::bit_cast<std::uint32_t>(value);};
    for(unsigned n=0;n<8;++n) {
      const auto &row=host->script_stages[n];check(row.at("stage")==n+1,"Visible device script stage ordering differs");
      const auto &g=row.at("globals");
      if(n==0)check(g.at(1)==bits(.375f) && g.at(3)==bits(.375f),"Visible original immediate position differs");
      if(n==1)check(g.at(2)==bits(.5f),"Visible original direct power differs");
      if(n==2)check(g.at(4)==1 && g.at(3)==bits(.75f),"Visible original group admission differs");
      if(n==3)check(g.at(5)==0 && g.at(3)==bits(.75f),"Visible original one-use group differs");
      if(n==4)check(g.at(1)==bits(.25f) && g.at(3)==bits(.25f),"Visible original linked immediate group differs");
      if(n==5)check(g.at(5)==0 && g.at(3)==bits(.25f),"Visible original immediate group admission differs");
      if(n==6)check(g.at(1)==bits(1.f) && g.at(2)==bits(-.25f),"Visible original clamp/power differs");
      if(n==7)check(g.at(6)==0 && g.at(7)==0 && g.at(8)==0,"Visible original nullable device differs");
    }
    passed&=matched_snapshots>0;
  } else if(control_test) {
    float source_position{},native_position{};unsigned differing_samples{};
    std::ifstream file(output/"authority-machine-state.jsonl");check(bool(file),"Control acceptance requires actual machine state tracing");
    std::string line;while(std::getline(file,line)) {
      const auto row=Json::parse(line);for(const auto &owner:row.at("owners")) {
        if(owner.at("definition").get<std::string>()!="scenarios\\objects\\multi\\zanzibar\\house_gate\\house_gate")continue;
        const auto source=owner.at("source_motion_bits").at(0).get<std::uint32_t>();
        const auto native=owner.at("native_motion_bits").at(0).get<std::uint32_t>();
        source_position=std::max(source_position,std::bit_cast<float>(source));
        native_position=std::max(native_position,std::bit_cast<float>(native));differing_samples+=source!=native;
      }
    }
    report["control_pickup_pressed"]=control_pressed;report["control_pickup_released"]=control_released;
    report["source_gate_maximum_position"]=source_position;report["native_gate_maximum_position"]=native_position;
    report["gate_position_mismatching_samples"]=differing_samples;
    passed&=control_pressed && control_released && source_position>.99f && native_position>.99f && differing_samples==0;
  } else if(conveyor_test) {
    report["conveyor_grounded_ticks"]=conveyor_grounded_ticks;
    report["conveyor_neutral_input_ticks"]=conveyor_uncommanded_ticks;
    report["conveyor_grounded_horizontal_travel"]=conveyor_grounded_travel;
    passed&=conveyor_grounded_ticks>30 && conveyor_uncommanded_ticks==samples.size() && conveyor_grounded_travel>.25f && forward_commands==0 && jump_commands==0;
  } else if(player_rearm_test) {
    report["stimulus"]="Authored player start; ordinary SDL jump held180 source ticks, released10 ticks, pressed again10 ticks, then neutral landing. No supplied controller state, velocity, support or pose.";
    report["held_jump_launches"]=rearm_held_launches;report["rearmed_jump_launches"]=rearm_second_launches;
    report["held_jump_command_ticks"]=rearm_held_commands;report["missing_held_jump_command_ticks"]=rearm_missing_held_commands;
    report["rearmed_landed_ticks"]=rearm_landed_ticks;
    passed&=rearm_first_released && rearm_second_released && rearm_held_launches==1 && rearm_second_launches==1 &&
        rearm_held_commands>=60 && rearm_missing_held_commands==0 && rearm_landed_ticks>=10 && fastest>0 && forward_commands==0;
  } else passed&=distance>0 && fastest>0 && largest_crouch>0 && jump_commands==1 && forward_commands>0 && (!lan||matched_snapshots>0);
  if(!lan)report["final_inventory"]=inventory(*o.prepared_world);
  if(prop_placement) {
    if(lan)impulses=host->impulses;
    std::ofstream file(output/"point-impulses.json");file<<impulses.dump(2)<<'\n';
    check(bool(file),"Could not write actual source point impulses");
    report["observed_source_point_impulses"]=impulses.size();
    if(prop_test) {
      const auto decode=[](const std::string &text) {
        check(text.size()%2==0,"Source impulse record has a partial byte");
        std::vector<std::byte> bytes(text.size()/2);constexpr std::string_view digits="0123456789abcdef";
        for(unsigned n=0;n<bytes.size();++n) {
          const auto a=digits.find(text[n*2]),b=digits.find(text[n*2+1]);
          check(a!=digits.npos && b!=digits.npos,"Source impulse record lost its literal byte encoding");
          bytes[n]=std::byte(a*16+b);
        }
        return bytes;
      };
      std::set<std::uint32_t> observed_props;
      for(const auto &sample:samples)if(sample.contains("prop_object"))
        observed_props.insert(sample.at("prop_object").get<std::uint32_t>());
      unsigned delivered{};float linear_change{},angular_change{};
      const auto vector=[](const Bytes &bytes,unsigned at){return Vec3{bytes.f32(at),bytes.f32(at+4),bytes.f32(at+8)};};
      const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
      for(const auto &event:impulses) {
        if(!observed_props.contains(event.at("object").get<std::uint32_t>()))continue;
        const auto before=decode(event.at("motion_before")),after=decode(event.at("motion_after"));
        const auto force=decode(event.at("impulse")),point=decode(event.at("point"));
        const auto a=vector(Bytes(before),0x40),b=vector(Bytes(after),0x40);
        const auto c=vector(Bytes(before),0x50),d=vector(Bytes(after),0x50);
        check(finite(a)&&finite(b)&&finite(c)&&finite(d)&&finite(vector(Bytes(force),0))&&finite(vector(Bytes(point),0)),
            "Actual prop impulse contains nonfinite force/point/motion data");
        const auto linear=length(b-a),angular=length(d-c);
        if(length(vector(Bytes(force),0))>0 && linear>0 && angular>0) {
          ++delivered;linear_change=std::max(linear_change,linear);angular_change=std::max(angular_change,angular);
        }
      }
      report["verified_prop_point_impulses"]=delivered;
      report["source_prop_point_impulse_linear_change"]=linear_change;
      report["source_prop_point_impulse_angular_change"]=angular_change;
      report["dynamic_prop_impulses_verified"]=delivered>0;
      // Retirement alone cannot satisfy blast/physical-body acceptance.
      // Require the observed target's genuine nonzero off-center impulse,
      // original linear/angular response, travel and original retirement.
      passed&=delivered>0 && prop_displacement>.01f && prop_retired;
    }
  }
  if(field_create_test) {
    report["declared_field_scene_lifecycle"]=host->scene_lifecycle;passed&=!host->scene_lifecycle.empty();
  }
  if(recreate_test) {
    report["actual_source_scene_lifecycle"]=host->scene_lifecycle;
    report["adopted_prop_generations"]=adopted_prop_generations;
    report["prop_geometry_submissions_by_generation"]=prop_draw_generations;
    report["living_recreation_observer_frames"]=living_recreation_observer_frames;
    report["camera_frames_aimed_at_recreated_prop"]=aimed_recreation_observer_frames;
    passed&=living_recreation_observer_frames>0 && aimed_recreation_observer_frames>0;
    passed&=host->scene_lifecycle.size()==2 && adopted_prop_generations.contains(host->first_prop_generation);
    for(const auto &event:host->scene_lifecycle) {
      const auto handle=event.at("new").get<std::uint32_t>();
      passed&=adopted_prop_generations.contains(handle);
      for(unsigned view=0;view<o.local_players;++view)passed&=prop_draw_generations[view].contains(handle);
    }
  }
  if(debris_test) {
    report["source_debris_objects"]=first_debris_position.size();
    report["source_debris_geometry_submissions_by_view"]=debris_draws;
    report["source_debris_maximum_displacement"]=maximum_debris_displacement;
    passed&=!first_debris_position.empty() && maximum_debris_displacement>.01f;
    for(const auto &[owner,position]:first_debris_position) {
      (void)position;
      for(unsigned view=0;view<o.local_players;++view)passed&=debris_draws[owner][view]>0;
    }
  }
  if(std::getenv("HALO2_TU15_AUTHORITY_RESTORE")) {
    check(host!=nullptr,"Live source authority restoration requires the actual LAN host");
    report["source_authority_restore"]=host->scene_lifecycle;
    report["source_authority_restored"]=host->source_authority_restored;
    passed&=host->source_authority_restored;
  }
  const auto native_rates_after=source_vehicle_native_rate_activity();
  const auto native_frames=lan?host->native_frames:
      o.prepared_world->source_objects->tu15_world().native_physics_frame_activity();
  report["native_physics_frame_activity"]={{"frame_calls",native_frames.frame_calls},
      {"residual_calls",native_frames.residual_calls},{"native_only_closure",false},
      {"world_integrations",native_frames.world_integrations},{"island_integrations",native_frames.island_integrations},
      {"native_force_calls",native_frames.native_force_calls},{"native_pose_calls",native_frames.native_pose_calls},{"native_deactivation_calls",native_frames.native_deactivation_calls},
        {"native_kinematic_pose_calls",native_frames.native_kinematic_pose_calls},
        {"native_fixed_pose_calls",native_frames.native_fixed_pose_calls},
        {"native_prescribed_force_calls",native_frames.native_prescribed_force_calls},
        {"native_body_policy_calls",native_frames.native_body_policy_calls},
        {"native_island_transition_calls",native_frames.native_island_transition_calls},
        {"native_array_growth_calls",native_frames.native_array_growth_calls},
        {"native_array_reserve_calls",native_frames.native_array_reserve_calls},
        {"native_array_pool_allocations",native_frames.native_array_pool_allocations},
        {"native_array_pool_releases",native_frames.native_array_pool_releases},
        {"native_world_collision_calls",native_frames.native_world_collision_calls},
        {"native_world_collision_listener_calls",native_frames.native_world_collision_listener_calls},
        {"native_island_collision_listener_calls",native_frames.native_island_collision_listener_calls},
        {"native_constraint_calls",native_frames.native_constraint_calls},
        {"native_dynamic_accumulator_calls",native_frames.native_dynamic_accumulator_calls},
        {"native_constraint_setup_calls",native_frames.native_constraint_setup_calls},
        {"native_island_collision_calls",native_frames.native_island_collision_calls},
        {"native_island_agent_flag_clear_calls",native_frames.native_island_agent_flag_clear_calls},
        {"native_island_agent_flag_set_calls",native_frames.native_island_agent_flag_set_calls},
        {"native_island_agent_process_calls",native_frames.native_island_agent_process_calls},
        {"native_island_agent_finish_calls",native_frames.native_island_agent_finish_calls},
        {"native_island_contact_minimum_calls",native_frames.native_island_contact_minimum_calls},
        {"native_island_contact_time_calls",native_frames.native_island_contact_time_calls},
        {"native_island_contact_buffers_constructed",native_frames.native_island_contact_buffers_constructed},
        {"native_island_contact_buffers_destroyed",native_frames.native_island_contact_buffers_destroyed},
        {"native_contact_advance_calls",native_frames.native_contact_advance_calls},
        {"native_contact_advance_motion_calls",native_frames.native_contact_advance_motion_calls},
        {"native_contact_advance_process_calls",native_frames.native_contact_advance_process_calls},
        {"native_contact_advance_single_iterations",native_frames.native_contact_advance_single_iterations},
        {"native_contact_advance_paired_iterations",native_frames.native_contact_advance_paired_iterations},
        {"native_contact_advance_negative_single_restores",native_frames.native_contact_advance_negative_single_restores},
        {"native_contact_advance_negative_paired_restores",native_frames.native_contact_advance_negative_paired_restores},
        {"native_motion_commit_calls",native_frames.native_motion_commit_calls},
        {"native_motion_commit_kinds",native_frames.native_motion_commit_kinds},
        {"native_broad_phase_calls",native_frames.native_broad_phase_calls},
        {"native_shape_bounds_box_calls",native_frames.native_shape_bounds_box_calls},
        {"native_shape_bounds_convex_calls",native_frames.native_shape_bounds_convex_calls},
        {"native_shape_bounds_transformed_calls",native_frames.native_shape_bounds_transformed_calls},
        {"native_shape_bounds_forward_calls",native_frames.native_shape_bounds_forward_calls},
        {"native_shape_bounds_motion_calls",native_frames.native_shape_bounds_motion_calls},
        {"native_shape_bounds_centered_box_calls",native_frames.native_shape_bounds_centered_box_calls},
        {"native_shape_bounds_centered_radius_calls",native_frames.native_shape_bounds_centered_radius_calls},
        {"shape_bounds_residual_calls",native_frames.shape_bounds_residual_calls},
        {"shape_bounds_residual_owners",native_frames.shape_bounds_residual_owners},
        {"named_residual_owner_calls",native_frames.residual_owner_calls},
        {"native_shape_bounds_endpoint_calls",native_frames.native_shape_bounds_endpoint_calls},
        {"native_shape_bounds_compound_calls",native_frames.native_shape_bounds_compound_calls},
        {"native_shape_bounds_compound_child_calls",native_frames.native_shape_bounds_compound_child_calls},
        {"native_solver_calls",native_frames.native_solver_calls},
        {"native_solver_initializations",native_frames.native_solver_initializations},
        {"native_solver_iterations",native_frames.native_solver_iterations},
        {"native_solver_primary_rows",native_frames.native_solver_primary_rows},
        {"native_solver_secondary_rows",native_frames.native_solver_secondary_rows},
        {"native_solver_exported_rows",native_frames.native_solver_exported_rows},
        {"native_solver_custom_refusals",native_frames.native_solver_custom_refusals},
        {"native_solver_custom_calls",native_frames.native_solver_custom_calls},
        {"native_solver_clock_calls",native_frames.native_solver_clock_calls},
        {"native_solver_custom_kinds",native_frames.native_solver_custom_kinds},
        {"native_constraint_producer_axis_calls",native_frames.native_constraint_producer_axis_calls},
        {"native_constraint_producer_scaled_calls",native_frames.native_constraint_producer_scaled_calls},
        {"native_constraint_producer_base_calls",native_frames.native_constraint_producer_base_calls},
        {"native_constraint_producer_base_rows",native_frames.native_constraint_producer_base_rows},
        {"native_constraint_producer_property_lookups",native_frames.native_constraint_producer_property_lookups},
        {"native_constraint_producer_emissions",native_frames.native_constraint_producer_emissions},
        {"native_constraint_producer_completed_emissions",native_frames.native_constraint_producer_completed_emissions},
        {"native_vehicle_rate_acceleration_calls",native_frames.native_vehicle_rate_acceleration_calls},
        {"native_vehicle_rate_approach_calls",native_frames.native_vehicle_rate_approach_calls},
        {"native_vehicle_rate_cursor_calls",native_frames.native_vehicle_rate_cursor_calls},
        {"native_vehicle_rate_refusals",native_frames.native_vehicle_rate_refusals},
        {"native_schema_emitter_calls",native_frames.native_schema_emitter_calls},
        {"native_schema_emitter_completed_calls",native_frames.native_schema_emitter_completed_calls},
        {"native_schema_emitter_cursor_calls",native_frames.native_schema_emitter_cursor_calls},
        {"native_schema_emitter_refusals",native_frames.native_schema_emitter_refusals},
        {"native_schema_emitter_caller_kinds",native_frames.native_schema_emitter_caller_kinds},
        {"native_schema_emitter_refusal_kinds",native_frames.native_schema_emitter_refusal_kinds},{"world_residual_calls",native_frames.world_residual_calls},
      {"scope","Actual authority/local world frame; residual physics owners still require native conversion"}};
  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_COLLISION")) {
    report["native_world_collision_gate"]=native_frames.native_world_collision_calls>0;
    passed&=native_frames.native_world_collision_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_ISLAND_COLLISION")) {
    report["native_island_collision_gate"]=native_frames.native_island_collision_calls>0;
    passed&=native_frames.native_island_collision_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_ISLAND_AGENTS")) {
    const bool agents_passed=native_frames.native_island_agent_flag_clear_calls+
      native_frames.native_island_agent_flag_set_calls>0&&
      native_frames.native_island_agent_process_calls>0&&native_frames.native_island_agent_finish_calls>0&&
      native_frames.native_island_contact_buffers_constructed>0&&
      native_frames.native_island_contact_buffers_constructed==native_frames.native_island_contact_buffers_destroyed;
    report["native_island_agents_gate"]=agents_passed;passed&=agents_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_ADVANCE")) {
    const bool advance_passed=native_frames.native_contact_advance_calls>0&&
      native_frames.native_contact_advance_motion_calls>0&&native_frames.native_contact_advance_process_calls>0;
    report["native_contact_advance_gate"]=advance_passed;passed&=advance_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_ISLAND_CONTACT_MINIMUM")) {
    report["native_island_contact_minimum_gate"]=native_frames.native_island_contact_minimum_calls>0;
    passed&=native_frames.native_island_contact_minimum_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_ARRAY_RESERVE")) {
    report["native_array_reserve_gate"]=native_frames.native_array_reserve_calls>0;
    passed&=native_frames.native_array_reserve_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_BROAD_PHASE")) {
    report["native_broad_phase_gate"]=native_frames.native_broad_phase_calls>0;
    passed&=native_frames.native_broad_phase_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_SHAPE_BOUNDS")) {
    const bool bounds_passed=native_frames.native_shape_bounds_box_calls+
      native_frames.native_shape_bounds_centered_box_calls+native_frames.native_shape_bounds_endpoint_calls>0&&
      native_frames.shape_bounds_residual_calls==0;
    report["native_shape_bounds_gate"]=bounds_passed;passed&=bounds_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_PRODUCERS")) {
    const auto&emissions=native_frames.native_constraint_producer_completed_emissions;
    const bool producers_passed=native_frames.native_constraint_producer_axis_calls+
      native_frames.native_constraint_producer_scaled_calls>0&&native_frames.native_constraint_producer_base_calls>0&&
      native_frames.native_constraint_producer_base_rows>0&&emissions[0]+emissions[1]+emissions[2]>0;
    report["native_constraint_producers_gate"]=producers_passed;passed&=producers_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_SCHEMA_EMITTER")) {
    const auto&completed_direct=native_frames.native_constraint_producer_completed_emissions;
    const auto direct=completed_direct[0]+completed_direct[1]+completed_direct[2];
    const bool emitter_passed=native_frames.native_schema_emitter_calls+direct>0&&
      native_frames.native_schema_emitter_completed_calls==native_frames.native_schema_emitter_calls&&
      native_frames.native_schema_emitter_cursor_calls+direct>0&&native_frames.native_schema_emitter_refusals==0;
    report["native_schema_emitter_gate"]=emitter_passed;
    passed&=emitter_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_VEHICLE_RATES")) {
    const bool rates_passed=native_frames.native_vehicle_rate_acceleration_calls>0&&
      native_frames.native_vehicle_rate_approach_calls>0&&native_frames.native_vehicle_rate_cursor_calls>0&&
      native_frames.native_vehicle_rate_refusals==0;
    report["native_vehicle_rates_gate"]=rates_passed;
    passed&=rates_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_SOLVER")) {
    const bool solver_passed=native_frames.native_solver_calls>0&&native_frames.native_solver_custom_refusals==0;
    report["native_constraint_solver_gate"]=solver_passed;
    passed&=solver_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_SOLVER_ITERATIONS")) {
    const bool iterations_passed=native_frames.native_solver_iterations>0&&native_frames.native_solver_primary_rows>0;
    report["native_constraint_solver_iterations_gate"]=iterations_passed;
    passed&=iterations_passed;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_CONSTRAINT_STEP")) {
    report["native_constraint_gate"]=native_frames.native_constraint_calls>0;
    passed&=native_frames.native_constraint_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_DYNAMIC_ACCUMULATOR")) {
    report["native_dynamic_accumulator_gate"]=native_frames.native_dynamic_accumulator_calls>0;
    passed&=native_frames.native_dynamic_accumulator_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_MOTION_COMMIT")) {
    report["native_motion_commit_gate"]=native_frames.native_motion_commit_calls>0;
    passed&=native_frames.native_motion_commit_calls>0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_PHYSICS_FRAME"))passed&=native_frames.frame_calls>0;
  if(std::getenv("HALO2_REQUIRE_NATIVE_PHYSICS_INTEGRATION"))passed&=
      native_frames.world_integrations>0&&native_frames.island_integrations>0&&native_frames.native_force_calls>0;
  if(std::getenv("HALO2_REQUIRE_NATIVE_PHYSICS_POSE")) {
    report["native_physics_pose_gate"]=native_frames.native_pose_calls>0;
    passed&=native_frames.native_pose_calls>0;
  }
  report["native_lift_scalar_activity"]={{"caller_calls",native_frames.native_lift_scalar_calls},
      {"power_calls",native_frames.native_lift_power_calls},{"errno_writes",native_frames.native_lift_errno_writes},
      {"observation_refusals",native_frames.native_lift_observation_refusals},
      {"scope","Actual authority/local world; each of the four authenticated scalar consumer sites"}};
  if(std::getenv("HALO2_REQUIRE_NATIVE_LIFT_SCALAR")) {
    std::uint64_t calls{};for(const auto count:native_frames.native_lift_scalar_calls)calls+=count;
    report["native_lift_scalar_gate"]=calls>0&&native_frames.native_lift_observation_refusals==0;
    passed&=calls>0&&native_frames.native_lift_observation_refusals==0;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_PHYSICS_DEACTIVATION")) {
    report["native_physics_deactivation_gate"]=native_frames.native_deactivation_calls>0;
    passed&=native_frames.native_deactivation_calls>0;
  }
  report["native_vehicle_rate_activity"]={{"acceleration_calls",native_rates_after.acceleration_calls-native_rates_before.acceleration_calls},
      {"approach_calls",native_rates_after.approach_calls-native_rates_before.approach_calls},
      {"live_cursor_calls",native_rates_after.live_cursor_calls-native_rates_before.live_cursor_calls},
      {"scope","Process-local counters across actual host/local source worlds; no object attribution or gameplay/replay input"}};
  report["acceptance_passed"]=passed;
  report["limit"]="Live native behavior only. Full retail contact/solver, driving, dynamic-object impulses and complete TU1.5 parity remain required.";
  {std::ofstream file(output/"report.json");file<<report.dump(2)<<'\n';check(bool(file),"Could not write acceptance report");}
  {std::ofstream file(output/"frames.json");file<<frames.dump()<<'\n';}
  {std::ofstream file(output/"cameras.json");file<<camera_frames.dump(2)<<'\n';}
  {std::ofstream file(output/"contrail-submissions.json");file<<contrail_submissions.dump(2)<<'\n';}
  std::cout<<(passed?"PASS":"RED")<<" visible "<<scenario<<": ticks="<<samples.size()<<" peak_z="<<peak_z
      <<" upward_velocity="<<fastest<<" crouch="<<largest_crouch<<" LAN_comparisons="<<matched_snapshots<<'\n';
  return passed?0:2;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
