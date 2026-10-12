#include "halo2/native_action_prompt_hud.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const std::string &why){if(!ok)throw FormatError(why);}
std::string substitute(std::string text,const std::string &button) {
  const std::string token="\xee\x91\x88";
  for(std::size_t at=0;(at=text.find(token,at))!=std::string::npos;at+=button.size())text.replace(at,token.size(),button);
  return text;
}
}
int main(int argc,char **argv){try {
  check(argc==3,"Supply MAPS VISTA_OR_MCC_HUD");const std::filesystem::path maps=argv[1],assets=argv[2];
  if(!std::filesystem::is_directory(maps))return 77;
  unsigned map_count{},placements{},admitted{},unpowered{},activated{},changed{},restored{};
  std::vector<std::filesystem::path> paths;
  for(const auto &entry:std::filesystem::directory_iterator(maps))
    if(entry.path().extension()==".map")paths.push_back(entry.path());
  std::sort(paths.begin(),paths.end());
  for(const auto &path:paths) {
    Cache cache(path);const auto scenario=cache.query_tag({.group=tag_groups::scenario});
    if(!scenario || !scenario->address || !scenario->size)continue;
    const auto metadata=cache.load_source_tag_metadata(scenario->datum);
    const Bytes raw(metadata.bytes_at(metadata.source_address(),metadata.size()));
    if(raw.u16(0x10)!=0)continue;
    auto content=cache.load_content(0);++map_count;
    check(content.device_groups.size()==raw.u32(0xa0),"Campaign omitted its authored device groups");
    check(content.devices.size()==content.objects.size(),"Campaign device/placement bank drifted");
    std::vector<std::size_t> controls;
    for(std::size_t i=0;i<content.objects.size();++i)if(content.objects[i].type==8 && content.objects[i].definition!=null_datum) {
      check(bool(content.devices[i]) && bool(content.devices[i]->control),"Campaign control has no retained definition");
      if(content.devices[i]->control->action_string && !content.devices[i]->control_nodes.empty())controls.push_back(i);
    }
    if(controls.empty())continue;
    Mesh mesh;NativeActionPromptHud hud(cache,content.player.datum,maps/"fonts",mesh,assets);
    const auto messages=cache.load_hud(content.player.datum,null_datum);
    // Isolated action-consumer fixtures: real campaign placement, model,
    // channels, group values and text. Mission scripts/AI and placement birth
    // chronology are deliberately outside this per-control test.
    const auto objects=content.objects;content.collision.clear();content.squads.clear();content.scripts={};
    for(auto &object:content.objects)object.flags|=1u;
    for(const auto placement:controls) {
      auto &object=content.objects[placement];object=objects[placement];object.flags&=~65u;object.source=-1;
      Spawn spawn;spawn.position=object.position;Simulation simulation(content,0,spawn);
      object.flags|=1u;
      const auto owner_at=std::find_if(simulation.devices().owners().begin(),simulation.devices().owners().end(),
          [&](const auto &entry){return entry.second.placement==placement;});
      check(owner_at!=simulation.devices().owners().end(),"Campaign control fixture did not construct its real owner");
      const auto handle=owner_at->first;const auto &owner=owner_at->second;const auto &definition=*content.devices[placement];
      const auto nodes=source_control_world_nodes(definition,owner);
      const auto sphere=source_shadow_sphere(definition.control_bounds,nodes.front(),owner.root.scale);
      const auto markers=pose_model_markers(definition.control_markers,nodes,definition.control_permutations,{},1);
      const auto forward=markers.empty()?nodes[0].basis[0]:markers[0].world.basis[0];
      auto &unit=const_cast<Unit&>(simulation.player());
      unit.body.position=sphere.center+forward*.2f-Vec3{0,0,unit.definition.camera_height};unit.body.velocity={};
      unit.yaw=std::atan2(-forward.y,-forward.x);unit.pitch=std::atan2(-forward.z,std::hypot(forward.x,forward.y));
      const auto prompt=simulation.control_action_prompt();++placements;
      const auto &position=simulation.devices().position_group(owner),&power=simulation.devices().power_group(owner);
      if(!source_device_usable(&position,&power,owner.motion.flags)) {
        check(prompt.target.object==null_datum,"Unavailable campaign control advertised an action");++unpowered;continue;
      }
      check(prompt.target.action==3 && prompt.target.object==handle && prompt.definition==definition.control->datum,
          path.filename().string()+": campaign control did not select its actual target");++admitted;
      const auto text=messages.strings.find(definition.control->action_string);
      check(text!=messages.strings.end() && !text->second.empty(),"Placed campaign control has no authored localized message");
      PcKeyboardSettings keyboard;ControllerSettings controller;
      for(const bool pad:{false,true}) {
        const auto frame=hud.frame(prompt,keyboard,controller,pad,1280,800);
        check(frame.text==substitute(text->second,pad?"\xee\x84\x82":"\xee\x84\xb2" "E") && !frame.vertices.empty(),
            "Campaign message lost its source text or actual Vista button glyph");
      }
      keyboard.bindings[unsigned(PcAction::Pickup)]=SDL_SCANCODE_SPACE;
      check(hud.frame(prompt,keyboard,controller,false,1920,1080).text==substitute(text->second,"\xee\x84\xb2" "Space"),
          "Campaign prompt ignored its remapped pickup key");
      const auto root=unit.body.position;const auto yaw=unit.yaw,pitch=unit.pitch;
      unit.vitality.dead=true;check(simulation.control_action_prompt().target.object==null_datum,"Dead campaign unit retained prompt");unit.vitality.dead=false;
      unit.source_parent=0x10001;check(simulation.control_action_prompt().target.object==null_datum,"Parented campaign unit retained prompt");unit.source_parent=null_datum;
      unit.source_action_word|=0x400000u;check(simulation.control_action_prompt().target.object==null_datum,"Action-blocked campaign unit retained prompt");unit.source_action_word&=~0x400000u;
      auto &presentation=const_cast<Presentation&>(simulation.presentation());
      presentation.player_camera_enabled=false;check(simulation.control_action_prompt().target.object==null_datum,"Scripted campaign camera retained prompt");presentation.player_camera_enabled=true;
      presentation.cinematic=true;check(simulation.control_action_prompt().target.object==null_datum,"Cinematic campaign retained prompt");presentation.cinematic=false;
      unit.body.position={10000,10000,10000};check(simulation.control_action_prompt().target.object==null_datum,"Distant campaign control retained prompt");unit.body.position=root;
      const auto aim=[&]{unit.body.position=root;unit.body.velocity={};unit.yaw=yaw;unit.pitch=pitch;};
      simulation.request_checkpoint();simulation.step({0,0,yaw,pitch});aim();
      const auto before=simulation.devices().position_group(*simulation.devices().find(handle));
      auto expected=before;const auto activation=source_control_activate(*definition.control,&expected);
      const auto threshold=source_pickup_hold_threshold(content.pickup_hold_seconds);
      check(threshold>0 && threshold<65536,"Invalid campaign hold fixture threshold");
      Controls input;input.yaw=yaw;input.pitch=pitch;input.pickup=true;input.pickup_milliseconds=std::uint16_t(threshold-1);
      simulation.step(input);aim();
      check(simulation.devices().position_group(*simulation.devices().find(handle)).flags==before.flags &&
          simulation.devices().position_group(*simulation.devices().find(handle)).value==before.value,"Campaign control activated before authored hold threshold");
      input.pickup_milliseconds=std::uint16_t(threshold);simulation.step(input);aim();
      check(unit.source_player_actions.flags&0x10u,"Campaign hold did not consume its admitted primary action");++activated;
      const auto after=simulation.devices().position_group(*simulation.devices().find(handle));
      check(after.flags==expected.flags && after.value==expected.value,"Campaign held action did not apply the source control result to its actual group");
      changed+=activation.changed;
      if(activation.changed && (before.flags&1u))check(simulation.control_action_prompt().target.object==null_datum,"Used campaign one-shot control retained prompt");
      simulation.step(input);aim();
      const auto repeated=simulation.devices().position_group(*simulation.devices().find(handle));
      check(repeated.flags==after.flags && repeated.value==after.value,"Consumed campaign hold repeated an action");
      simulation.request_revert();simulation.step({});aim();
      check(simulation.control_action_prompt().target.object==handle,"Checkpoint did not restore campaign action owner");++restored;
    }
    std::cout<<"PASS "<<path.filename().string()<<": "<<controls.size()<<" modeled message controls\n";
  }
  check(map_count==15 && placements>0 && admitted>0 && activated==admitted && restored==admitted,"Campaign action coverage is incomplete");
  std::cout<<"PASS "<<map_count<<" campaign maps; "<<placements<<" control placements; "<<admitted<<" admitted, consumed and restored; "<<changed<<" changed groups; "<<unpowered<<" unavailable controls suppressed\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
