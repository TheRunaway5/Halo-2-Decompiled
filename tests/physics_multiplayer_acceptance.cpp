#include "halo2/physics_velocity_observation.hpp"
#include "helpers/source_device_script_input_tu15.hpp"
#include "helpers/live_phantom_original_capture_tu15.hpp"
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
#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_camera.hpp"
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
    a.frozen==b.frozen && a.has_world_nodes==b.has_world_nodes &&
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
};
SupportEntrance wheel_support_entrance(const Cache &cache,const LevelContent &content) {
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
  check(vehicle_name=="warthog" || vehicle_name=="ghost" || vehicle_name=="banshee" ||
      vehicle_name=="scorpion" || vehicle_name=="wraith" || vehicle_name=="spectre" || vehicle_name=="spectre_secondary" || vehicle_name=="warthog_gauss",
      "Vehicle acceptance selection has no authored Coagulation fixture");
  for(const auto &[row,placement]:assets.netgame_placements) {
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag&t){return t.datum==object.definition;});
    const auto definition_name=vehicle_name=="warthog_gauss"?"warthog":
        vehicle_name=="spectre_secondary"?"spectre":vehicle_name;
    if(tag==cache.tags().end() || tag->name.find("\\"+definition_name+"\\")==std::string::npos)continue;
    if(vehicle_name=="warthog_gauss")check(object.variant==0x05000694u,"Gauss fixture did not select the original authored variant");
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
      if(requested_seat?index!=*requested_seat:!(seat.flags&4u))continue;
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
  unsigned port{},host_slot{};
  Json initial_inventory;
  Json impulses=Json::array();
  Json scene_lifecycle=Json::array();
  Json script_stages=Json::array();
  std::atomic_bool start_device_script{};
  std::atomic_bool resume_support{};
  Json support_samples=Json::array();
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
      MultiplayerPreparedWorld world(options,std::move(script_input));
      world.match.configure_replicated_first_person(world.cache->load_first_person_binding(world.rules.weapon,2));
      std::optional<std::uint32_t> support_object;bool support_resumed=false;
      const auto support_power=[&](float value) {
        check(support_object.has_value(),"Original support command lost its current source object");
        world.source_objects->script_device("device_set_power",{ScriptValue{54,*support_object,{}},ScriptValue::numeric(value,6)});
      };
      if(std::getenv("HALO2_TU15_SUPPORT_INPUT")) {
        for(const auto &object:world.source_objects->live_objects()) {
          const auto *tag=world.cache->find_tag(TagHandle{object.definition});
          if(tag && tag->name=="scenarios\\objects\\multi\\zanzibar\\big_wheel\\big_wheel")support_object=object.object;
        }
        check(support_object.has_value(),"Original support fixture has no current wheel body");
        support_power(0);world.match.step({});
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
      if(world.source_objects && std::getenv("HALO2_TU15_PROP_GRENADE")) {
        impulse_trace.open(options.source_object_savegame_directory.parent_path()/"authority-point-impulses.jsonl");
        check(bool(impulse_trace),"Could not retain actual authority point impulses");
        world.source_objects->observe_body_impulses([this,&impulse_trace](const auto &event){
          auto row=impulse_json(event);impulse_trace<<row.dump()<<'\n';impulse_trace.flush();
          check(bool(impulse_trace),"Could not retain completed authority point impulse");
          impulses.push_back(std::move(row));
        });
      }
      auto effective_variant=options.variant;
      if(options.starting_weapon)effective_variant.values.set(89,int(options.starting_weapon));
      LanHost transport(world.match,0,multiplayer_fingerprint(options.map,world.cache->shared_directory(),0,world.rules.weapon,options.gameplay_xbe),effective_variant);
      initial_inventory=inventory(world);content=std::make_shared<LevelContent>(world.content);
      port=transport.port();host_slot=world.local_slots.front();
      started=true;startup.set_value();
      std::array<Controls,multiplayer_capacity> controls{};
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
      std::map<std::uint32_t,std::pair<unsigned,std::string>> velocity_targets;
      struct PhysicsObservationScope {
        MultiplayerObjectRuntime *runtime{};
        ~PhysicsObservationScope() {
          if(!runtime)return;
          // Detach while captured streams/targets still exist. Original
          // world teardown executes more stores after those locals retire.
          runtime->observe_body_velocity_inputs({});runtime->observe_physics_stores({});
          runtime->observe_phantom_phase({});
        }
      } physics_observation_scope{std::getenv("HALO2_TU15_PHANTOM_TRACE")?world.source_objects.get():nullptr};
      if(std::getenv("HALO2_TU15_PHANTOM_TRACE")) {
        phantom_trace.open(options.source_object_savegame_directory.parent_path()/"authority-phantom-phase.jsonl");
        check(bool(phantom_trace),"Could not retain actual source phantom phase");
        velocity_trace.open(options.source_object_savegame_directory.parent_path()/"authority-velocity-stores.jsonl");
        check(bool(velocity_trace),"Could not retain actual source velocity stores");
        world.source_objects->observe_physics_stores([&](std::uint32_t actual_instruction,const SourcePhysicsContactRecordStore &store) {
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
                  member["definition"]=tag?tag->name:"";member["type"]=std::to_integer<unsigned>(memory.view(target+0x10bu,1)[0]);
                  member["body_bytes"]=hex_bytes(memory.view(target,0x12cu));
                  const auto pool=word(0x52be5cu),stride=word(pool+0x24u);
                  if(physical!=null_datum && (physical&65535u)<word(pool+0x20u)) {
                    const auto address=word(pool+0x44u)+(physical&65535u)*stride;
                    const auto index=std::to_integer<unsigned>(row[1])<128?std::to_integer<unsigned>(row[1]):0u;
                    if(index<word(address+0x74u)) {
                      const auto rigid=word(word(address+0x70u)+index*0x60u+0x40u),motion=word(rigid+0x3cu);
                      member["motion_bytes"]=hex_bytes(memory.view(motion,512));member["motion_address"]=motion;
                      if(std::getenv("HALO2_TU15_PHANTOM_CAPTURE_ANY_MEMBER") && capture_target==world.match.player(1).source_current_object &&
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
        if(resume_support.exchange(false)) {support_power(1);support_resumed=true;}
        if(start_device_script.exchange(false)) {
          check(scripted,"Device script start has no declared input");
          world.match.run_script("device_source_sequence");
        }
        // HELLO can publish a new participant at the existing world clock,
        // before the next physics frame. Retain that actual sent-state phase.
        const auto before=world.match.state();
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
        world.match.step(controls);
        if(support_object && world.match.player(1).connected) {
          const auto &player=world.match.player(1);const auto &memory=world.source_objects->memory();
          const auto node=world.source_objects->world_nodes(*support_object);check(!node.empty(),"Original rotating support lost WORLD node0");
          const auto unit=world.source_objects->object_body(player.source_current_object);
          const auto flags=Bytes(memory.view(unit+0x348u,2)).u16(0);
          const auto state=world.match.state();
          check(state.players[1].native_grounded==((flags&1u)==0),"Rotating support grounded readback differs from original biped");
          const auto wheel_body=world.source_objects->object_body(*support_object);
          support_samples.push_back({{"tick",state.tick},{"unit",player.source_current_object},{"wheel",*support_object},
              {"resumed",support_resumed},{"position",xyz(state.players[1].position)},{"velocity",xyz(state.players[1].velocity)},
              {"grounded",state.players[1].native_grounded},{"source_support_flags",flags},
              {"wheel_position_bits",Bytes(memory.view(wheel_body+0x140u,4)).u32(0)},
              {"wheel_power_bits",Bytes(memory.view(wheel_body+0x134u,4)).u32(0)},
              {"node_scale",node[0].scale},{"node_position",xyz(node[0].position)},
              {"node_basis",Json::array({xyz(node[0].basis[0]),xyz(node[0].basis[1]),xyz(node[0].basis[2])})}});
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
        {std::lock_guard lock(mutex);history.push_back(world.match.state());}
        transport.broadcast();
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){
      try {throw;}catch(const std::exception &e){std::cerr<<"acceptance_host_error="<<e.what()<<std::endl;}
      catch(...){std::cerr<<"acceptance_host_error=non-standard exception"<<std::endl;}
      if(!started)startup.set_exception(std::current_exception());
      else {std::lock_guard lock(mutex);error=std::current_exception();}
    }});
    ready.get();
  }
  void finish(){worker.request_stop();worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) try {
  check(argc==7,"Supply MAP SHARED_MAPS TU15_XBE VISTA_UI OUTPUT_DIR movement|lift|vehicle|objects|lan-movement|lan-lift|lan-vehicle|lan-objects");
  const std::string scenario=argv[6];const bool lan=scenario.starts_with("lan-"),
      lift=scenario=="lift"||scenario=="lan-lift",vehicle=scenario=="vehicle"||scenario=="lan-vehicle",objects=scenario=="objects"||scenario=="lan-objects";
  const auto vehicle_selection=std::getenv("HALO2_TU15_VEHICLE_NAME");
  const std::string_view vehicle_name=vehicle_selection?vehicle_selection:"warthog";
  const bool accompanied_driver=vehicle && std::getenv("HALO2_TU15_SEAT_DRIVER");
  const bool control_test=lan && std::getenv("HALO2_TU15_CONTROL_ENTRY");
  const bool script_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_DEVICE_SCRIPT_INPUT");
  const bool support_test=scenario=="lan-movement" && std::getenv("HALO2_TU15_SUPPORT_INPUT");
  check(!support_test || (!script_test && !control_test),"Select one support/device stimulus");
  std::optional<SupportEntrance> support_start;bool support_started=false;
  std::optional<std::pair<float,float>> support_pending_look;
  check(!script_test || !control_test,"Select one declared device stimulus");
  bool script_started=false;std::optional<ScriptProgram> script_input;
  std::optional<ControlEntrance> control_start;
  bool control_pressed=false,control_released=false;
  std::optional<std::pair<float,float>> control_pending_look;
  const bool long_vehicle_sequence=vehicle && (accompanied_driver || !std::getenv("HALO2_TU15_VEHICLE_SEAT")) &&
      (vehicle_name=="warthog" || vehicle_name=="banshee" || vehicle_name=="scorpion" || vehicle_name=="wraith" || vehicle_name=="spectre" || vehicle_name=="spectre_secondary" || vehicle_name=="warthog_gauss");
  check(!lan || scenario=="lan-movement" || lift || vehicle || objects,"Unknown LAN physics scenario");
  check(lan||lift||vehicle||objects||scenario=="movement","Unknown multiplayer physics scenario");
  check(std::getenv("DISPLAY")||std::getenv("WAYLAND_DISPLAY"),"Visible desktop display is required");
  const std::filesystem::path output=argv[5];std::filesystem::create_directories(output);
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];o.ui_assets=argv[4];
  o.starting_weapon=5;o.width=960;o.height=540;o.local_players=lan&&!vehicle&&!objects?1:2;o.mute=true;
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
  }
  Json report{{"scenario",scenario},{"reference","English TU1.5 only"},{"visible",true},
      {"live_physics_parity",false},{"stimulus","SDL keyboard events; no supplied physics results"}};
  const bool prop_test=objects && std::getenv("HALO2_TU15_PROP_GRENADE");
  const bool recreate_test=lan && prop_test && std::getenv("HALO2_TU15_SCENE_RECREATE");
  // This fixture's actual spawn collector places the first client at the
  // distant location and the second beside the prop. Assign ordinary devices
  // to those views; never move a completed body to repair the test.
  const unsigned actor_view=recreate_test?1u:0u,watcher_view=recreate_test?0u:1u;
  const bool debris_test=prop_test && std::getenv("HALO2_TU15_DYNAMIC_OBJECTS");
  std::set<std::uint32_t> debris_definitions;
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
    support_start=wheel_support_entrance(cache,content);o.native_spawn_inputs=support_start->spawns;
    report["stimulus"]="Declared original hull0 paddle spawn inputs; original device power API pauses/resumes the actual wheel; ordinary SDL look only";
    report["declared_support_spawns"]=Json::array();for(const auto &spawn:support_start->spawns)report["declared_support_spawns"].push_back(xyz(spawn.position));
    report["declared_support_local_top"]=xyz(support_start->local_top);
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
    o.join="127.0.0.1:"+std::to_string(host->port);report["udp_port"]=host->port;
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
  };
  bool vehicle_entered{},vehicle_exited{};unsigned vehicle_drive_commands{};
  bool entry_released{},exit_released{},entry_approaching{};
  std::optional<unsigned> entry_press,entered_tick,drive_start,exit_press;
  std::optional<unsigned> drive_end;bool steering_started{},steering_released{},forward_released{};
  Vec3 drive_initial{};float driven_distance{},vehicle_climb{};
  Vec3 steering_forward{};float steering_angle{};unsigned steering_commands{};
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
  };
  o.source_scene_observer=[&](const MultiplayerState &state,const SceneObjects &scene) {
    if(!lan)return;
    check(state.source_scene_roots_authoritative,"Replica scene observer has no source authority");
    for(const auto &root:state.source_scene_roots) {
      const auto &object=scene.get(root.object);
      if(prop_placement && root.placement==*prop_placement)adopted_prop_generations.insert(root.object);
      check(object.placement==root.placement && object.solved_transform &&
          object.created_tick==root.created_tick &&
          !std::memcmp(&*object.solved_transform,&root.transform,sizeof(Transform)) &&
          object.visible==root.visible,"Rendered scene root differs from received source placement/frame");
      const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
          [&](const auto &p){return p.object==root.object;});
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
  Json body_frames=Json::array();
  struct BodySample {unsigned slot;std::uint32_t tick,unit;float fraction;Vec3 node0;};
  std::array<std::vector<BodySample>,2> body_view_samples;
  std::array<unsigned,2> seated_body_draws{},exited_body_draws{};
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
  std::array<float,4> submitted_horizontal_fov{};
  o.view_observer=[&](unsigned view,const Camera &,float horizontal_fov,std::int16_t) {
    check(view<submitted_horizontal_fov.size(),"Camera submission exceeds actual local views");
    submitted_horizontal_fov[view]=horizontal_fov;
  };
  o.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
    if(vehicle && view<body_view_samples.size()) {
      // Body callbacks precede this callback. Validate against this frame's
      // actual snapshot, including entry/exit and replacement generations.
      for(const auto &sample:body_view_samples[view]) {
        const auto &player=state.players[sample.slot];
        check(sample.unit==player.source_unit && sample.tick==state.tick,
            "Vehicle entry submitted a stale player generation or tick");
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
      if(steering_started && !forward_released) {
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
      }
    } else if(vehicle_exited && player.source_parent==null_datum) {
      ++exit_camera_frames;
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
      local_slot=local_slots[actor_view];
      const auto &p=s.players[local_slot];started=true;start_step=step;start_authority_tick=s.tick;initial=p.position;
      initial_z=peak_z=p.position.z;
      if(observer) {
        check(local_slots.size()>watcher_view,"Visible observer has no actual local view");
        observer_slot=local_slots[watcher_view];observer_initial=s.players[observer_slot].position;
        check(observer_slot!=~0u,"Visible observer did not join the real roster");
      }
      focus(window);
    }
    check(commands.size()>actor_view,"Actor lost its actual local input view");
    const auto elapsed=(accompanied_driver || script_test || support_test)?s.tick-start_authority_tick:step-start_step;const auto &p=s.players[local_slot];const auto &c=commands[actor_view];
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
      for(const auto &v:s.world_vehicles)if(v.placement==vehicle_start->placement) {
        if(driven_vehicle==null_datum){driven_vehicle=v.handle;vehicle_initial=v.transform.position;}
        check(v.handle==driven_vehicle,"Driving acceptance crossed a vehicle generation");
        vehicle_distance=std::max(vehicle_distance,length(v.transform.position-vehicle_initial));
        row["vehicle_root"]=xyz(v.transform.position);row["vehicle_forward"]=xyz(v.transform.basis[0]);row["vehicle"]=v.handle;
        vehicle_position=v.transform.position;
        vehicle_forward=v.transform.basis[0];
        const auto &definition=interaction_content.unit_interactions->units.at(
            interaction_content.objects.at(vehicle_start->placement).definition);
        const auto &seat=definition.seats.at(vehicle_start->seat);
        const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==v.handle;});
        check(pose!=s.source_body_poses.end(),"Vehicle snapshot omitted the actual WORLD hierarchy");
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
            {"yaw",input.yaw},{"pitch",input.pitch}};
        // Render/input samples can repeat one completed authority tick.
        // Continue the same ordinary retreat until actual displacement clears
        // the vehicle, with a finite input budget; never move its body here.
        const bool retreat=elapsed<45 || (elapsed<120 && observer_distance<.75f);
        observer->axis(SDL_CONTROLLER_AXIS_LEFTY,retreat?1.f:0.f);
        if(vehicle_position) {
          const auto look=*vehicle_position+Vec3{0,0,interaction_content.player.camera_height} -
              (watcher.position+Vec3{0,0,interaction_content.player.camera_height});
          const auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(yaw-input.yaw,6.283185307f)*8.f);
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-input.pitch)*8.f);
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
      if(entry_target && !entry_press && elapsed%5==0) {
        focus(window);check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual vehicle-test mouse input");
        const auto look=*entry_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
        const float pitch=std::atan2(look.z,std::hypot(look.x,look.y));
        const float yaw=std::atan2(look.y,look.x);
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Invalid actual mouse gain");SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
        e.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
        e.motion.yrel=-int(std::lround((pitch-c.pitch)/gain))*(settings.invert?-1:1);
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
          look.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
          look.motion.yrel=-int(std::lround(-c.pitch/gain))*(settings.invert?-1:1);
          check(SDL_PushEvent(&look)==1,"SDL rejected vehicle departure look input");
        }
      }
      if(drive_start) {
        // A real desktop focus loss clears SDL's keys. Reacquire the actual
        // window and deliver the remaining observed input budget, instead of
        // silently shortening the driving sequence while counting wall ticks.
        if(!forward_released && !c.forward) {focus(window);key(window_id,SDL_SCANCODE_W,true);}
        if(vehicle_drive_commands>=(long_vehicle_sequence?120u:25u) && !steering_started) {
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
        if(steering_started && !steering_released) {
          if(steering_commands>=25) {key(window_id,SDL_SCANCODE_D,false);steering_released=true;}
          else if(!c.right)key(window_id,SDL_SCANCODE_D,true);
        }
        if(vehicle_drive_commands>=(long_vehicle_sequence?300u:65u) && !forward_released) {
          key(window_id,SDL_SCANCODE_W,false);forward_released=true;drive_end=elapsed;
        }
        if(drive_end && elapsed>=*drive_end+45 && !exit_press) {
          exit_press=elapsed;key(window_id,SDL_Scancode(pickup),true);
        }
      }
      if(exit_press && !exit_released && c.pickup && c.pickup_milliseconds>=threshold) {
        key(window_id,SDL_Scancode(pickup),false);exit_released=true;
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
          p.position,p.velocity,p.crouch,s.world_vehicles,s.source_body_poses,s.source_scene_roots,s.source_dynamic_objects,s.grenade_projectiles,s.grenade_detonations,s.devices});
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
    if(objects) {
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
    if(support_start) {
      const auto delta=support_start->center-(p.position+Vec3{0,0,interaction_content.player.camera_height});
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
      if(elapsed>=30 && !support_started) {host->resume_support.store(true);support_started=true;}
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
    if(!lift&&!vehicle&&!objects&&!control_test&&!script_test&&!support_test) switch(elapsed) {
      case 25:focus(window);key(window_id,SDL_SCANCODE_W,true);break;
      case 40:key(window_id,SDL_SCANCODE_W,false);break;
      case 50:key(window_id,SDL_SCANCODE_LSHIFT,true);break;
      case 65:key(window_id,SDL_SCANCODE_LSHIFT,false);break;
      case 85:focus(window);key(window_id,SDL_SCANCODE_SPACE,true);key(window_id,SDL_SCANCODE_SPACE,false);break;
    }
    if(accompanied_driver && ((vehicle_exited && exit_camera_frames>=20) ||
        (companion_entered_tick && !vehicle_entered && elapsed-*companion_entered_tick>=90)))quit();
    if(elapsed>=(support_test?240u:script_test?600u:control_test?900u:accompanied_driver?900u:vehicle?(long_vehicle_sequence?900u:300u):180u))quit();
  };
  // Finite local screenshots retain all earlier real input delivery. Network
  // exit uses SDL_QUIT because exact tick capture deliberately rejects LAN.
  if(!lan){o.capture_tick=vehicle?(long_vehicle_sequence?901:301):181;o.screenshot=output/"frame.ppm";}
  const int result=run_multiplayer(o);if(host)host->finish();trace.close();
  if(support_test) {std::ofstream file(output/"authority-support-samples.json");file<<host->support_samples.dump(2)<<'\n';
    check(bool(file),"Could not retain original rotating support observations");}
  if(script_test) {std::ofstream file(output/"authority-script-stages.json");file<<host->script_stages.dump(2)<<'\n';
    check(bool(file),"Could not retain actual visible script stages");}
  if(lan) {
    Json comparisons=Json::array();
    for(const auto &p:authority_samples) {
      const bool same=std::any_of(host->history.begin(),host->history.end(),[&](const auto &h) {
        if(h.tick!=p.tick)return false;const auto &a=h.players[p.slot];
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
  report["runtime_exit"]=result;report["observed_input_ticks"]=samples.size();report["submitted_frames"]=frames.size();
  report["local_slot"]=local_slot;report["initial_position"]=xyz(initial);report["maximum_distance"]=distance;
  report["initial_z"]=initial_z;report["peak_z"]=peak_z;report["maximum_upward_velocity"]=fastest;
  report["maximum_crouch"]=largest_crouch;report["jump_commands"]=jump_commands;report["forward_commands"]=forward_commands;
  report["exact_lan_snapshot_comparisons"]=matched_snapshots;
  report["adopted_source_scene_frames"]=adopted_scene_frames;
  report["adopted_source_dynamic_frames"]=adopted_dynamic_frames;
  bool passed=result==0 && started && samples.size()>=170 && frames.size()>0;
  if(lan)passed&=adopted_scene_frames>0;
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
    if(report["selected_vehicle"]=="banshee")passed&=vehicle_climb>2.f && driven_distance>10.f;
    report["source_driving_observed"]=vehicle_entered && vehicle_drive_commands>0 && driven_distance>1;
    report["driver_steering_commands"]=steering_commands;report["source_driver_steering_radians"]=steering_angle;
    report["observer_backward_commands"]=observer_back_commands;report["observer_maximum_distance"]=observer_distance;
    report["source_driver_controls_completed"]=vehicle_entered && vehicle_exited &&
        vehicle_drive_commands>0 && driven_distance>1 && steering_commands>0 && steering_angle>.05f;
    const bool driver=(vehicle_start->flags&4u)!=0;
    report["source_selected_seat_completed"]=vehicle_entered && vehicle_exited && vehicle_drive_commands>0 &&
        steering_commands>0;
    passed&=(driver?report["source_driver_controls_completed"].get<bool>():
        report["source_selected_seat_completed"].get<bool>()) && (!lan || matched_snapshots==samples.size());
    if(accompanied_driver) {
      report["companion_driver_entered"]=companion_entered;
      report["companion_driver_forward_commands"]=companion_forward_commands;
      report["elapsed_clock"]="Actual authority ticks; input samples may repeat a completed snapshot";
      report["companion_original_camera_frames"]=companion_camera_frames;
      report["moving_occupied_vehicle_distance"]=driven_distance;
      passed&=companion_entered && companion_forward_commands>0 && companion_camera_frames>0 && driven_distance>1.f;
    } else passed&=observer_back_commands>0 && observer_distance>.5f;
    report["submitted_seated_camera_frames"]=seated_camera_frames;
    report["submitted_camera_distance_from_first_person_eye"]=seated_camera_distance;
    report["submitted_camera_frames_after_exit"]=exit_camera_frames;
    passed&=seated_camera_frames>0 && seated_camera_distance>.5f && exit_camera_frames>0;
    report["submitted_seated_camera_look_turn_radians"]=steering_camera_angle;
    passed&=steering_camera_angle>.05f;
    report["submitted_driver_vehicle_geometry_by_view"]=driven_vehicle_draws;
    passed&=driven_vehicle_draws[0]>0 && driven_vehicle_draws[1]>0;
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
  } else if(support_test) {
    const auto vector=[](const Json &v){return Vec3{v.at(0).get<float>(),v.at(1).get<float>(),v.at(2).get<float>()};};
    std::optional<Vec3> origin,local_origin;std::optional<float> initial_phase;
    unsigned static_supported{},moving_supported{};float carried{},wheel_motion{};
    for(const auto &row:host->support_samples) {
      if(row.at("unit").get<std::uint32_t>()!=samples.front().at("unit").get<std::uint32_t>())continue;
      const auto position=vector(row.at("position"));
      const auto phase=std::bit_cast<float>(row.at("wheel_position_bits").get<std::uint32_t>());
      const auto power=std::bit_cast<float>(row.at("wheel_power_bits").get<std::uint32_t>());
      const bool grounded=row.at("grounded").get<bool>();
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
    report["paused_original_support_ticks"]=static_supported;
    report["moving_original_support_ticks"]=moving_supported;
    report["original_support_carried_distance"]=carried;report["original_wheel_position_delta"]=wheel_motion;
    report["original_support_resume_command"]=support_started;
    passed&=support_started && static_supported>=5 && moving_supported>=5 && carried>.25f && wheel_motion>.01f && forward_commands==0 && jump_commands==0;
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
  } else passed&=distance>0 && fastest>0 && largest_crouch>0 && jump_commands==1 && forward_commands>0 && (!lan||matched_snapshots>0);
  if(!lan)report["final_inventory"]=inventory(*o.prepared_world);
  if(prop_placement) {
    if(lan)impulses=host->impulses;
    std::ofstream file(output/"point-impulses.json");file<<impulses.dump(2)<<'\n';
    check(bool(file),"Could not write actual source point impulses");
    report["observed_source_point_impulses"]=impulses.size();
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
  report["acceptance_passed"]=passed;
  report["limit"]="Live native behavior only. Full retail contact/solver, driving, dynamic-object impulses and complete TU1.5 parity remain required.";
  {std::ofstream file(output/"report.json");file<<report.dump(2)<<'\n';check(bool(file),"Could not write acceptance report");}
  {std::ofstream file(output/"frames.json");file<<frames.dump()<<'\n';}
  {std::ofstream file(output/"cameras.json");file<<camera_frames.dump(2)<<'\n';}
  std::cout<<(passed?"PASS":"RED")<<" visible "<<scenario<<": ticks="<<samples.size()<<" peak_z="<<peak_z
      <<" upward_velocity="<<fastest<<" crouch="<<largest_crouch<<" LAN_comparisons="<<matched_snapshots<<'\n';
  return passed?0:2;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
