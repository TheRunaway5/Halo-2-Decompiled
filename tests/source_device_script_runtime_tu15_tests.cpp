#include "helpers/source_device_script_input_tu15.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "source_device_snapshot_check.hpp"
#include <bit>
#include <set>
#include <map>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-device-script-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create private script world files");path=name;}
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
// Declared script input, not a replacement physics/group result. The actual
// ScriptVm resolves a current named device and calls Simulation's host path.
using Program=test::DeviceScriptProgram;
std::uint32_t word(const SourceRuntimeMemory &memory,auto address){return Bytes(memory.view(address,4)).u32(0);}
}
int main(int argc,char **argv)try{
  std::cout<<std::unitbuf;
  const bool wheel=argc==4 && std::string_view(argv[3])=="--wheel-power";
  const bool props=argc==4 && std::string_view(argv[3])=="--script-props";
  if((argc!=3 && !wheel && !props) || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  Files files;Cache cache(argv[2],{},OriginalData::load_xbe(argv[1]));auto content=cache.load_content(0,true);
  int gate_name=props?0:-1;
  for(unsigned n=0;!props && n<content.objects.size();++n)if(n<content.devices.size() && content.devices[n] &&
      content.devices[n]->machine && content.devices[n]->placement.position_group==0)gate_name=content.objects[n].name;
  check(gate_name>=0,"Original Zanzibar gate has no current name input");
  Program program(gate_name);if(!props)content.scripts=std::move(program.data);
  MultiplayerObjectRuntime world(argv[1],cache,0,files.path);
  UiVariant variant{"Slayer",0,{{0x59,5}}};auto rules=slayer_rules(variant);
  rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  world.configure_session(source_game_variant(variant),1,30,rules.random_seed);
  MultiplayerMatch match(content,0,cache.spawns(),rules,MultiplayerKingServices{},[]{return 100000u;},&world);
  match.configure_weapon_pickups();
  const auto borrowed=std::shared_ptr<const Cache>(&cache,[](const Cache *){});
  match.configure_grenades(borrowed,*cache.original_data()->grenade_constants);
  match.configure_melee_animation(borrowed,*cache.original_data()->melee_constants);
  check(match.join("Script authority")==0,"Original source host did not join");
  const auto fp=cache.load_first_person_binding(rules.weapon,2);match.configure_replicated_first_person(fp);
  const std::array<unsigned,1> local{0};match.configure_local_views(local,fp);
  std::map<std::uint32_t,std::unique_ptr<test::LivePhysicsEntryCaptureTu15>> entry_captures;
  std::optional<SourcePhysicsEntryObserverBinding> entry_diagnostic;
  if(props) {
    if(const auto path=std::getenv("HALO2_TU15_PROP_ENTRY_CAPTURE")) {
      entry_captures.emplace(0x355d0u,std::make_unique<test::LivePhysicsEntryCaptureTu15>(std::filesystem::path(path)/"conveyor"));
      entry_captures.emplace(0x1e1170u,std::make_unique<test::LivePhysicsEntryCaptureTu15>(std::filesystem::path(path)/"query"));
    }
    if(const auto path=std::getenv("HALO2_TU15_WORLD_ENTRY_CAPTURE"))for(const auto entry:{0x14fec0u,0xb8f40u,0x1cc8d0u})
      entry_captures.emplace(entry,std::make_unique<test::LivePhysicsEntryCaptureTu15>(std::filesystem::path(path)/std::to_string(entry)));
    if(!entry_captures.empty())entry_diagnostic.emplace(world.memory(),[&](bool entering,std::uint32_t entry,const auto &state){
      const auto found=entry_captures.find(entry);if(found==entry_captures.end())return;
      if(entry==0x14fec0u || entry==0xb8f40u || entry==0x1cc8d0u) {
        const auto clock=word(world.memory(),0x51df54u);if(word(world.memory(),clock+8u)<200u)return;
      }
      found->second->observe(entering,state,world.memory(),entry);
    });
  }
  constexpr std::uint64_t fingerprint=0x15de000332u;
  LanHost host(match,0,fingerprint,variant);
  LanClient peer("127.0.0.1:"+std::to_string(host.port()),fingerprint,1);
  std::array<Controls,16> controls{};Controls remote{};
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!peer.ready() && std::chrono::steady_clock::now()<deadline){
    peer.send(std::span(&remote,1));host.receive(controls);peer.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(peer.ready() && host.peers()==1,"Actual source script UDP admission failed");
  if(props) {
    const std::string prop_name=std::getenv("HALO2_TU15_SCRIPT_PROP_NAME")?std::getenv("HALO2_TU15_SCRIPT_PROP_NAME"):"gas_container";
    unsigned comparisons{},dynamic_observations{};std::set<std::uint32_t> moved;
    std::map<std::uint32_t,Vec3> last;
    std::map<std::uint32_t,Vec3> first;
    for(unsigned tick=0;tick<650;++tick) {
      peer.send(std::span(&remote,1));host.receive(controls);match.step(controls);host.broadcast();
      check(match.simulation().scripts().diagnostics().empty(),"Actual authored prop spawner faulted");
      const auto sent=match.state();const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);
      do{peer.receive();if(peer.state().tick>=sent.tick)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        while(std::chrono::steady_clock::now()<until);
      check(peer.state().tick==sent.tick,"Authored prop physics snapshot missed its actual UDP tick");
      test::same_device_snapshot(sent.devices,peer.state().devices);++comparisons;
      for(const auto &object:match.simulation().scene().objects()) {
        const auto &placed=content.objects.at(object.placement);
        const auto *tag=cache.find_tag(TagHandle{placed.definition});
        if(object.handle==null_datum || (object.created_tick==0 && !(placed.flags&1u)) || placed.type!=11 || (placed.flags&0x100u) ||
            !tag || tag->name.find(prop_name)==std::string::npos)continue;
        ++dynamic_observations;const auto position=world.object_transform(object.handle).position;
        const auto [prior,inserted]=first.emplace(object.handle,position);last[object.handle]=position;
        if(inserted) {
          const auto body=world.object_body(object.handle);
          check((std::to_integer<unsigned>(world.memory().view(body+0xc1u,1)[0])&1u)==0,
              "Script-created source prop was incorrectly forced frozen at birth by the automatic placement caller");
        }
        if(!inserted && length(position-prior->second)>.1f)moved.insert(object.handle);
      }
    }
    check(dynamic_observations>0 && moved.size()>=2,"Authored source prop spawners did not produce two moving physical generations");
    for(const auto &[object,position]:first)std::cout<<"prop="<<object<<" first="<<position.x<<","<<position.y<<","<<position.z<<" final="<<last[object].x<<","<<last[object].y<<","<<last[object].z<<"\n";
    std::cout<<"PASS authored "<<std::filesystem::path(argv[2]).filename().string()<<" source prop creation/dynamics, "<<moved.size()<<" moving generations, "<<comparisons<<" exact UDP device banks\n";return 0;
  }
  if(wheel) {
    const auto selected=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag){return tag.group=="mach" &&
        tag.name=="scenarios\\objects\\multi\\zanzibar\\big_wheel\\big_wheel";});
    check(selected!=cache.tags().end(),"Original wheel definition is absent");
    const auto objects=world.live_objects();const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.definition==selected->datum;});
    check(object!=objects.end(),"Original wheel body has no current source identity");
    const auto command=[&](float power){world.script_device("device_set_power",{ScriptValue{54,object->object,{}},ScriptValue::numeric(power,6)});};
    command(0);unsigned comparisons{};std::optional<std::uint32_t> stopped;float resumed{};
    for(unsigned tick=0;tick<180;++tick) {
      if(tick==90)command(1);
      peer.send(std::span(&remote,1));host.receive(controls);match.step(controls);host.broadcast();
      const auto sent=match.state();const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);
      do{peer.receive();if(peer.state().tick>=sent.tick)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        while(std::chrono::steady_clock::now()<until);
      check(peer.state().tick==sent.tick,"Original wheel snapshot was not delivered at its actual tick");
      test::same_device_snapshot(sent.devices,peer.state().devices);++comparisons;
      const auto owner=std::find_if(sent.devices.owners.begin(),sent.devices.owners.end(),[&](const auto &o){return o.definition==selected->datum;});
      check(owner!=sent.devices.owners.end(),"Original wheel lost its native device owner");
      const auto source=word(world.memory(),world.object_body(object->object)+0x140u);
      check(source==std::bit_cast<std::uint32_t>(owner->state.motion.position),"Original wheel position readback differs");
      if(tick<90) {if(!stopped)stopped=source;
        check(source==*stopped && owner->state.motion.power==0,"Original wheel did not stop under its actual device API");}
      else {check(owner->state.motion.power==1,"Original wheel did not resume under its actual device API");
        resumed=std::max(resumed,std::abs(owner->state.motion.position-std::bit_cast<float>(*stopped)));}
    }
    check(resumed>.01f,"Original wheel did not move after power resumed");
    std::cout<<"PASS original wheel device API pause/resume, "<<comparisons<<" exact UDP device banks, position delta="<<resumed<<'\n';return 0;
  }
  unsigned stages{},comparisons{};
  for(unsigned tick=0;tick<20 && stages<8;++tick){
    peer.send(std::span(&remote,1));host.receive(controls);match.step(controls);host.broadcast();
    const auto sent=match.state();
    const auto receive_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    do{peer.receive();if(peer.state().tick>=sent.tick)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
      while(std::chrono::steady_clock::now()<receive_deadline);
    check(peer.state().tick==sent.tick,"Device script snapshot was not delivered at its actual tick");
    test::same_device_snapshot(sent.devices,peer.state().devices);++comparisons;
    check(match.simulation().scripts().diagnostics().empty(),"Actual device script VM faulted");
    for(const auto &entry:sent.devices.owners){const auto body=world.object_body(entry.state.handle);
      check(std::bit_cast<std::uint32_t>(entry.state.motion.position)==word(world.memory(),body+0x140u) &&
          std::bit_cast<std::uint32_t>(entry.state.motion.power)==word(world.memory(),body+0x134u),
          "Scripted native device state differs from its actual original body");}
    const auto &globals=match.simulation().scripts().globals();const auto stage=globals[0].bits;
    if(stage==stages)continue;check(stage==stages+1,"Device script stage ordering changed");stages=stage;
    const auto bits=[](float value){return std::bit_cast<std::uint32_t>(value);};
    const auto gate=std::find_if(sent.devices.owners.begin(),sent.devices.owners.end(),[&](const auto &entry){
      return content.objects.at(entry.state.placement).name==gate_name;});
    check(gate!=sent.devices.owners.end(),"Source script lost its actual named gate generation");
    if(std::getenv("HALO2_TU15_DEVICE_SCRIPT_TRACE")) {
      const auto body=world.object_body(gate->state.handle);
      std::cout<<"stage="<<stage<<" globals=";
      for(const auto &value:globals)std::cout<<std::hex<<value.bits<<',';
      std::cout<<" source_position="<<word(world.memory(),body+0x140u)
          <<" source_power="<<word(world.memory(),body+0x134u)
          <<" group="<<std::bit_cast<std::uint32_t>(sent.devices.groups.at(0).value)
          <<" device_flags="<<word(world.memory(),body+0x12cu)
          <<" door_flags="<<word(world.memory(),body+0x1ccu)<<std::dec<<'\n';
    }

    if(stage==1 || stage==5) {
      const float expected=stage==1?.375f:.25f;
      check(word(world.memory(),world.object_body(gate->state.handle)+0x140u)==bits(expected),
          "Immediate script updated a native copy without updating its original source body");
    }
    const float target=stage<=2?.375f:stage<=4?.75f:stage<=6?.25f:1.f;
    check(std::bit_cast<std::uint32_t>(sent.devices.groups.at(0).value)==bits(target),
        "Scripted source group did not persist into the completed original world tick");

    if(stage==1)check(globals[1].bits==bits(.375f) && globals[3].bits==bits(.375f),"Immediate source position/group getter differs");
    if(stage==2)check(globals[2].bits==bits(.5f),"Source power getter differs");
    if(stage==3)check(globals[4].truth() && globals[3].bits==bits(.75f),"Normal source target did not consume its real one-use group");
    if(stage==4)check(!globals[5].truth() && globals[3].bits==bits(.75f),"Source one-use group allowed another target");
    if(stage==5)check(globals[1].bits==bits(.25f) && globals[3].bits==bits(.25f),"Original immediate group did not update linked device state");
    if(stage==6)check(!globals[5].truth() && globals[3].bits==bits(.25f),"Immediate source group reset one-use admission");
    if(stage==7)check(globals[1].bits==bits(1.f) && globals[2].bits==bits(-.25f),"Original immediate clamp or direct power write differs");
    if(stage==8)check(globals[6].bits==0 && globals[7].bits==0 && !globals[8].truth(),"Original nullable device results differ");
  }
  check(stages==8 && comparisons>=8,"Incomplete source device script/UDP sequence");
  const auto audit=match.simulation().scripts().audit();
  for(const auto *name:{"device_get_position","device_get_power","device_set_position","device_set_power",
      "device_set_position_immediate","device_group_get","device_group_set","device_group_set_immediate"})
    check(audit.builtins.contains(name),"Device VM sequence omitted an API");
  std::cout<<"PASS8 actual ScriptVm/source device stages, "<<comparisons<<" exact UDP device banks; original clamp, direct power, one-use, linked and nullable behavior\n";
  std::cout<<"LIMIT declared script input and native localhost UDP; not a retail HS evaluator table or a visible framebuffer comparison\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
