// Campaign qualification through native constructors, ai_place and declared
// mapped commands at 30 Hz. This does not prove raw physical-button epochs.
// No parent/seat, allegiance, activity, damage, hit or velocity result is
// supplied. Run only through tools/run_test.py.
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/original_data.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool v,const char *why){if(!v)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t at,unsigned bytes=4) {
  std::uint32_t v{};const auto b=m.view(at,bytes);
  for(unsigned n=0;n<bytes;++n)v|=std::to_integer<std::uint32_t>(b[n])<<(8*n);return v;
}
std::int16_t short_(const SourceRuntimeMemory &m,std::uint32_t at) {
  return std::bit_cast<std::int16_t>(std::uint16_t(word(m,at,2)));
}
float scalar(const SourceRuntimeMemory &m,std::uint32_t at){return std::bit_cast<float>(word(m,at));}
Json vector(Vec3 v){return Json::array({v.x,v.y,v.z});}
#ifdef HALO2_CAMPAIGN_TARGET_DIAGNOSTICS
// [DEBUG-campaign-target-20261026] Test-only native call observations.
// The exact native body and every service still execute once with their real
// bank/ABI. No diagnostic value is returned to native code.
thread_local Json *targeting_observation{};
std::uint32_t probe_word(const SourcePhysicsContactMemory&m,std::uint32_t at,unsigned n=4) {
  std::array<std::byte,4>b{};m.read(at,std::span(b).first(n));std::uint32_t v{};
  for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
Json probe_words(const SourcePhysicsContactMemory&m,std::uint32_t at,unsigned count) {
  Json out=Json::array();for(unsigned i=0;i<count;++i)out.push_back(probe_word(m,at+4*i));return out;
}
#endif
struct Snapshot {
  MultiplayerSourceMotion motion;
  std::uint32_t actions{},activity{},target{},selected{};
  float confidence{};int current{},requested{},ready{};
};
struct Witness {
  const SourceRuntimeMemory &memory;std::uint32_t body{};unsigned tick{};
  unsigned action29_sets{},action27_sets{},parent_stores{},seat_stores{},mode6_stores{};
  std::set<std::uint32_t> owner_pcs;Json stores=Json::array();
  void observe(const SourcePhysicsContactRecordStore &s) {
    const auto pc=s.instruction;
    if((pc>=0xed060 && pc<0xed4c0)||(pc>=0xec450 && pc<0xec780))owner_pcs.insert(pc);
    if(s.before==s.written)return;
    const auto overlaps=[&](std::uint32_t at,unsigned n) {
      return s.address<std::uint64_t(at)+n && std::uint64_t(s.address)+s.written.size()>at;
    };
    const auto component=body+std::uint32_t(short_(memory,body+0x346));
    bool selected=false;
    if(overlaps(component+4,4)) {
      const auto flags=word(memory,component+4);
      if(pc==0xed1d3 && (flags&(1u<<29)))++action29_sets;
      if(flags&(1u<<27))++action27_sets;
      selected=true;
    }
    if(overlaps(body+0x14,4)){++parent_stores;selected=true;}
    if(overlaps(body+0x1fc,2)){++seat_stores;selected=true;}
    if(overlaps(body+0x3dc,1)){mode6_stores+=word(memory,body+0x3dc,1)==6;selected=true;}
    if(selected && stores.size()<256)stores.push_back({{"tick",tick},{"pc",pc},{"address",s.address},
        {"parent",word(memory,body+0x14)},{"seat",short_(memory,body+0x1fc)},
        {"actions",word(memory,component+4)},{"physical_state",word(memory,body+0x3dc,1)},
        {"physical_clock",std::bit_cast<std::int32_t>(word(memory,body+0x3ec))}});
  }
};
}
#ifdef HALO2_CAMPAIGN_TARGET_DIAGNOSTICS
#define H2_CAMPAIGN_COMBAT_SYMBOL "_ZN5halo229source_run_combat_action_tu15ERNS_26SourcePhysicsContactMemoryEjRNS_30SourcePhysicsContactRuntimeAbiENS_33SourcePhysicsFactoryFloatingPointERKNS_35SourcePhysicsContactRuntimeServicesERKSt8functionIFvRKNS_31SourcePhysicsContactRecordStoreEEE"
extern "C" void campaign_combat_original(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&)
    __asm__("__real_" H2_CAMPAIGN_COMBAT_SYMBOL);
extern "C" void campaign_combat_observed(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&)
    __asm__("__wrap_" H2_CAMPAIGN_COMBAT_SYMBOL);
extern "C" void campaign_combat_observed(SourcePhysicsContactMemory&m,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices&s,const SourcePhysicsContactRecordObserver&o) {
  if(!targeting_observation||(entry!=0x1adc90&&entry!=0x1adf60)) {campaign_combat_original(m,entry,a,fp,s,o);return;}
  const auto before=a;Json observation{{"entry",entry},{"source_ecx",a.gp[1]},{"gate_pcs",Json::array()},{"external_returns",Json::array()}};
  if(entry==0x1adf60) {
    observation["actual_eye_words"]=probe_words(m,a.gp[1],3);
    observation["actual_query_arguments"]=probe_words(m,a.gp[4]+4,5);
    observation["actual_query_look_words"]=probe_words(m,probe_word(m,a.gp[4]+8),3);
  }
  auto observed=s;observed.instruction_cursor={};
  observed.instruction=[&](std::uint32_t pc) {
    constexpr std::array points{0x1adcc3u,0x1adcd7u,0x1add1au,0x1adf00u,0x1adf4cu,0x1adf50u,
      0x1adf8bu,0x1adf8eu,0x1adf9du,0x1adfa5u,0x1adfd1u,0x1adfd5u,0x1ae022u,0x1ae024u,
      0x1ae047u,0x1ae06fu,0x1ae07eu,0x1ae090u,0x1ae294u,0x1af3e0u};
    if(std::ranges::find(points,pc)!=points.end()&&observation["gate_pcs"].size()<128)observation["gate_pcs"].push_back(pc);
    if(s.instruction)s.instruction(pc);
  };
  observed.required_owner=[&](auto&memory,const auto&call,auto&state,const auto&writes) {
    need(bool(s.required_owner),"Diagnostic native call has no original required owner");
    s.required_owner(memory,call,state,writes);
    if(call.target==0x14cdb0&&observation["external_returns"].size()<16)
      observation["external_returns"].push_back({{"pc",call.instruction},{"target",call.target},{"actual_eax",state.gp[0]}});
  };
  campaign_combat_original(m,entry,a,fp,observed,o);
  observation["actual_return_al"]=a.gp[0]&255u;
  if(entry==0x1adc90&&(a.gp[0]&255u))observation["actual_parameter_words"]=probe_words(m,before.gp[6],9);
  if(entry==0x1adf60&&(a.gp[0]&255u))observation["actual_result_words"]=probe_words(m,probe_word(m,before.gp[4]+20),24);
  targeting_observation->push_back(std::move(observation));
}
#undef H2_CAMPAIGN_COMBAT_SYMBOL
#endif
int main(int argc,char **argv) {
  Json report{{"passed",false},{"scope","Native Campaign TU1.5 mapped-command qualification; no stored-flight result is assumed"},
      {"input_contract","Declared mapped 92-byte commands on the fixture's 30 Hz timeline; native player owners derive edges and interaction/reload arbitration"},
      {"raw_physical_button_epoch_parity",false}};
  try {
    std::cout<<std::unitbuf;need(argc==6,"Supply MAP MAPS native-mcc OWNED_OUTPUT MODE");
    need(std::string_view(argv[3])=="native-mcc","Campaign fixture requires compiled native TU1.5 operands");
    const auto env=std::getenv("HALO2_TEST_SCRATCH");need(env && *env,"Run Campaign fixture through owned scratch");
    const auto scratch=std::filesystem::canonical(env),output=std::filesystem::absolute(argv[4]);
    const auto relative=output.lexically_relative(scratch);
    need(!relative.empty() && *relative.begin()!="..","Campaign output must be inside its owned scratch");
    need(!std::getenv("HALO2_CAMERA_DECLARED_SEAT_API"),"Forced/declared seat API is excluded from this qualification");
    const std::string mode=argv[5];
    const bool authored_ai=mode=="campaign-sword-onfoot-control";
    const bool factory_biped=mode=="campaign-sword-native-biped-control";
    const bool onfoot=authored_ai || factory_biped;
    need(onfoot || mode=="campaign-sword-seat-control","Unknown Campaign control mode");
    report["mode"]=mode;report["stage"]="load prepared Campaign assets";
    Cache cache(argv[1],argv[2]);need(cache.is_mcc() && cache.original_data()==OriginalData::native_tu15(),
        "Campaign fixture requires prepared MCC with compiled native TU1.5 operands");
    need(cache.load_source_geometry_scenario_type()==0,"Campaign fixture requires actual source kind1 assets");
    auto content=cache.load_content(0,false);CollisionWorld terrain(content.collision);
    const auto sword=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="weap" &&
        t.name=="objects\\weapons\\melee\\energy_blade\\energy_blade";});
    need(sword!=cache.tags().end(),"Campaign has no actual Energy Sword definition");
    const auto sword_tag=cache.load_source_tag_metadata(sword->datum);
    const Bytes sword_bytes(sword_tag.bytes_at(sword_tag.source_address(),sword_tag.size()));
    need((sword_bytes.u32(0x12c)&(1u<<24))!=0,"Authored Campaign weapon does not have the original Sword predicate");
    const auto full_range=sword_bytes.f32(0x20c)*.5f;
    need(full_range>1.5f && std::isfinite(full_range),"Campaign Sword has no valid full-confidence primary range");
    report["sword"]={{"definition",sword->datum},{"flags",sword_bytes.u32(0x12c)},
        {"primary_range",sword_bytes.f32(0x20c)},{"full_confidence_range",full_range}};
    Spawn spawn;std::uint32_t ai_reference=null_datum,factory_definition=null_datum;
    Vec3 declared_target_position{},declared_target_forward{};
    unsigned squad_index{},start_index{};
    if(onfoot) {
      const auto atmosphere=cache.load_bsp_atmosphere(0);bool found=false;
      const float distance=std::min(2.5f,full_range-.25f);
      for(unsigned q=0;q<content.squads.size() && !found;++q) {
        const auto &s=content.squads[q];
        if(s.count<=0 || s.team<=2 || (s.bsp!=-1 && s.bsp!=0))continue; // Survey only; actual allegiance is checked below.
        for(unsigned k=0;k<s.starts.size() && !found;++k) {
          const auto &a=s.starts[k];if(a.reference_frame!=-1 || atmosphere.cluster_at(a.position)<0)continue;
          const auto character=content.characters.find(a.character);
          if(factory_biped && (character==content.characters.end() ||
              character->second.biped.datum==null_datum))continue;
          for(unsigned n=0;n<16 && !found;++n) {
            const auto yaw=float(n)*6.283185307179586f/16.f;const Vec3 d{std::cos(yaw),std::sin(yaw),0};
            const auto floor=terrain.raycast(a.position+d*distance+Vec3{0,0,2},{0,0,-1},4);
            if(!floor || floor->normal.z<.9f || std::abs(floor->position.z-a.position.z)>.5f)continue;
            const auto p=floor->position+Vec3{0,0,content.player.radius};
            const auto eye=p+Vec3{0,0,content.player.camera_height};const auto to=a.position+Vec3{0,0,.5f};
            if(terrain.overlaps(p,content.player.height,content.player.radius) ||
                terrain.raycast(eye,normalized(to-eye),length(to-eye)))continue;
            spawn.position=p;spawn.facing=std::atan2(to.y-eye.y,to.x-eye.x);
            squad_index=q;start_index=k;ai_reference=0xc0000000u|(q<<16)|k;
            declared_target_position=a.position;declared_target_forward=direction(a.facing);
            if(factory_biped)factory_definition=character->second.biped.datum;
            found=true;
          }
        }
      }
      need(found,"No authored Campaign actor start has a clear Sword control position in BSP0");
      report["authored_ai"]={{"squad",squad_index},{"name",content.squads[squad_index].name},
          {"start",start_index},{"reference",ai_reference},{"imported_team",content.squads[squad_index].team}};
    }
    report["stage"]="construct native Campaign world";std::filesystem::create_directories(output);
    MultiplayerObjectRuntime objects(cache,0,output/"source-storage");auto &world=objects.tu15_world();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x1234);
    auto &bank=world.memory();need(word(bank,word(bank,0x4f3218)+8)==1,"Native world did not retain Campaign kind1");
    std::uint32_t vehicle=null_datum;Vec3 entry_aim{};
    if(!onfoot) {
      report["stage"]="native Ghost constructor and authored entry marker";
      const auto tag=std::ranges::find_if(cache.tags(),[](const auto&t){return t.group=="vehi" && t.name=="objects\\vehicles\\ghost\\ghost";});
      need(tag!=cache.tags().end() && !cache.spawns().empty(),"Campaign Ghost/start unavailable");
      const auto start=cache.spawns().front();const auto floor=terrain.raycast(start.position+Vec3{0,0,32},{0,0,-1},256);
      need(floor.has_value(),"Authored Campaign start has no nearby BSP floor");
      constexpr std::uint32_t request=0x60010000;
      auto a=world.prepare_source_call(std::array{tag->datum,null_datum,0u});a.gp[6]=request;world.run_actual(0xb9960,a);
      const Vec3 position=floor->position,forward{1,0,0};
      bank.write(request+0x1c,std::as_bytes(std::span(&position,1)));bank.write(request+0x28,std::as_bytes(std::span(&forward,1)));
      vehicle=world.call_actual(0xb9b70,std::array{request});need(vehicle!=null_datum,"Native Ghost birth rejected");
      ScenarioObject metadata;metadata.type=1;metadata.definition=tag->datum;content.objects.push_back(metadata);
      content.unit_interactions=load_native_unit_interaction_assets(cache,content);
      const auto &definition=content.unit_interactions->units.at(tag->datum);const auto &seat=definition.seats.at(0);
      need(!(seat.flags&0x10000u),"Selected authored seat prohibits players");
      const auto marker=definition.markers.find(seat.entry_marker);need(marker!=definition.markers.end(),"Ghost lacks ordinary entry marker");
      const auto markers=pose_model_markers(marker->second,objects.world_nodes(vehicle),definition.permutations,{},1);
      need(!markers.empty(),"Native Ghost has no live entry marker");entry_aim=markers.front().world.position;
      spawn.position=entry_aim+markers.front().world.basis[0]*(seat.radius*.5f);
      const auto foot=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      need(foot.has_value(),"Ghost entry has no cached BSP floor");spawn.position.z=foot->position.z+content.player.radius;
      spawn.facing=std::atan2(entry_aim.y-spawn.position.y,entry_aim.x-spawn.position.x);
      report["vehicle"]={{"object",vehicle},{"definition",tag->datum},{"seat",0},{"flags",seat.flags},{"entry",vector(entry_aim)}};
    }
    report["stage"]="native player construction and Sword equipment";
    DatumPool players(bank,world.player_pool());const auto player=players.allocate_at(0);need(player!=null_datum,"Player pool exhausted");
    auto configuration=source_default_player_configuration();constexpr std::uint32_t profile=0x6005b000;
    bank.write(profile,std::as_bytes(std::span(configuration)));auto a=world.prepare_source_call();a.gp[2]=profile;world.run_actual(0x14e7b0,a);
    bank.read(profile,std::as_writable_bytes(std::span(configuration)));objects.construct_player(player,0,configuration);
    const auto unit=objects.create_biped(content.player.datum,spawn.position,direction(spawn.facing),player);need(unit!=null_datum,"Native player biped birth rejected");
    objects.bind_player(player,unit);objects.initialize_player_equipment(player,unit);objects.set_local_player(player,0);
    const auto weapon=world.equip_weapon(unit,sword->datum);need(weapon!=null_datum,"Native Sword equipment rejected");
    const auto body=world.source_object_body(unit);const auto owner=std::as_const(players).lookup(player);
    need(!owner.empty(),"Player lost its full source identity");const auto player_row=word(bank,objects.source_player_pool()+0x44)+(player&65535u)*players.stride();
    std::uint32_t target=null_datum;float initial_vitality{};
    if(onfoot) {
      if(authored_ai) {
        const auto counts=[&] {
          DatumPool actors(bank,word(bank,0x5020c8)),object_pool(bank,word(bank,0x4ecbd0));
          return Json{{"actors",actors.count()},{"objects",object_pool.count()},
            {"source_tick",word(bank,word(bank,0x51df54)+8)},
            {"ai_enabled",word(bank,word(bank,0x5020a8)+1,1)},
            {"pending_reference",word(bank,word(bank,0x5020a8)+0x364)}};
        };
        report["stage"]="native authored AI starting-location placement";
        report["placement_before"]=counts();world.place_ai(ai_reference);
        report["placement_after"]=counts();target=world.ai_object(ai_reference);
        report["placement_object_query"]=target;
        //27D190's synchronous placement path already owns B9B70/1E8410.
        // step_objects includes1D0670 once per source tick; no extra AI frame
        // or actor/result write may turn a rejected placement into this control.
        need(target!=null_datum,"Authored native ai_place produced no target unit");
      } else {
        report["stage"]="native biped constructor with authored default team";
        need(factory_definition!=null_datum,"Authored start has no resident native biped definition");
        const auto definition=cache.load_source_tag_metadata(factory_definition);
        const Bytes data(definition.bytes_at(definition.source_address(),definition.size()));
        const auto default_team=std::bit_cast<std::int16_t>(data.u16(0xc0));
        need(default_team>=0,"Native biped control requires an authored non-NONE default team");
        Json team_stores=Json::array();
        objects.observe_physics_stores([&](auto,const auto&s) {
          if(s.instruction==0xc676a || s.instruction==0xc6788)
            team_stores.push_back({{"pc",s.instruction},{"address",s.address},
              {"written_team",short_(bank,s.address)}});
        });
        try {target=objects.create_biped(factory_definition,declared_target_position,declared_target_forward);}
        catch(...) {objects.observe_physics_stores({});throw;}
        objects.observe_physics_stores({});
        need(target!=null_datum,"Native authored biped constructor rejected its actual input");
        const auto target_body=world.source_object_body(target);
        need(word(bank,target_body+0x12c)==null_datum,"Independent biped control unexpectedly acquired an AI owner");
        need(short_(bank,target_body+0x138)==default_team && std::any_of(team_stores.begin(),team_stores.end(),[&](const Json&s) {
          return s["pc"]==0xc6788 && s["address"]==target_body+0x138 && s["written_team"]==default_team;
        }),"Native biped control did not retain the original constructor's authored default team");
        report["factory_target"]={{"object",target},{"definition",factory_definition},
          {"authored_position",vector(declared_target_position)},{"authored_default_team",default_team},
          {"actor",word(bank,target_body+0x12c)},{"native_team_stores",team_stores},
          {"scope","Independent source-created biped control; not authored AI placement or AI behavior"}};
      }
      const auto target_body=world.source_object_body(target);const auto actor=word(bank,target_body+0x12c);
      if(authored_ai) {
        need(actor!=null_datum,"Campaign target has no actual AI owner");
        DatumPool actors(bank,word(bank,0x5020c8));const auto actor_row=std::as_const(actors).lookup(actor);
        need(!actor_row.empty() && Bytes(std::as_bytes(actor_row)).u32(0x18)==target,"Actual Campaign actor/unit identities differ");
      }
      const auto first=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,player_row+0xc0,1)));
      const auto second=short_(bank,target_body+0x138);need(first>=0 && second>=0,"Hostility requires actual non-NONE native teams");
      a=world.prepare_source_call();a.gp[1]=std::uint16_t(first);a.gp[6]=std::uint16_t(second);world.run_actual(0x1e7810,a);
      need((a.gp[0]&255)!=0,"Original live Campaign allegiance predicate did not classify the actor as hostile");
      if(factory_biped) {
        constexpr std::uint32_t friendly=0x6005f000;
        const std::array poison{std::byte{0xa5}};bank.write(friendly,poison);
        // Full original1AE500: candidate, source, actual source team, output.
        // Its biped path checks type/death/tag flags/team without actor+12C.
        auto predicate=world.prepare_source_call(std::array{target,unit,std::uint32_t(first),friendly});
        world.run_actual(0x1ae500,predicate);
        const auto accepted=predicate.gp[0]&255,related=word(bank,friendly,1);
        report["factory_target"]["native_candidate_gate"]={{"entry",0x1ae500},
          {"accepted",accepted},{"friendly",related},{"source_team",first},{"target_team",second}};
        need(accepted && related==0,"Original target gate did not admit the independent native biped as hostile");
      }
      const auto vital=objects.object_vitality(target);initial_vitality=vital.body+vital.shield;
      report["hostility"]={{"player",player},{"unit",unit},{"target",target},{"actor",actor},
          {"player_team",first},{"target_team",second},{"predicate",0x1e7810},{"result",a.gp[0]&255}};
    }
    // This callback only reads stores after the native instruction completed.
    Witness witness{bank,body};objects.observe_physics_stores([&](auto,const auto &s){witness.observe(s);});
    struct Stop {
      MultiplayerObjectRuntime &objects;Witness &witness;Json &report;
      ~Stop(){objects.observe_physics_stores({});try {
        report["source_owner_pcs"]=witness.owner_pcs;report["source_stores"]=witness.stores;
        report["observed_stores"]={{"action27",witness.action27_sets},{"action29",witness.action29_sets},
          {"parent",witness.parent_stores},{"seat",witness.seat_stores},{"mode6",witness.mode6_stores}};
      }catch(...) {}}
    } stop{objects,witness,report};
    const auto snapshot=[&] {
      const auto component=body+std::uint32_t(short_(bank,body+0x346));
      need(component>=body && component<body+0x8000,"Action component exceeds live-body bound");
      const auto current=std::bit_cast<std::int8_t>(std::uint8_t(word(bank,body+0x212,1)));
      return Snapshot{objects.biped_motion(unit),word(bank,component+4),word(bank,component+0x2c),word(bank,body+0x1c8),
          current>=0 && current<4?word(bank,body+0x218+4u*unsigned(current)):null_datum,scalar(bank,body+0x1e4),
          current,std::bit_cast<std::int8_t>(std::uint8_t(word(bank,body+0x216,1))),
          std::bit_cast<std::int8_t>(std::uint8_t(word(bank,player_row+0x3f,1)))};
    };
    unsigned ticks{},mode6_samples{},qualified_samples{},contacts{},entry_prompts{},pickup_presses{};
    std::uint64_t pickup_started_ms{};bool pickup_was_down{},pickup_release_observed{};
    float closest=1e9f,max_speed{};
    report["frames"]=Json::array();
    const auto step=[&](const char *phase,bool fire,bool pickup,Vec3 aim,bool birth_look=false) {
      witness.tick=ticks;std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
      const auto put=[&](unsigned at,std::uint32_t value,unsigned count=4){for(unsigned n=0;n<count;++n)command[at+n]=std::byte(value>>(8*n));};
      const auto command_ms=std::uint64_t(ticks)*1000u/30u;
      const bool source_x_before=(word(bank,player_row+0x3c,2)&1u)!=0;
      if(pickup && !pickup_was_down) {
        need(pickup_release_observed && !source_x_before,"Mapped X press requires an observed native release");
        pickup_started_ms=command_ms;pickup_release_observed=false;++pickup_presses;
      }
      const auto pickup_ms=pickup?std::uint16_t(std::min<std::uint64_t>(command_ms-pickup_started_ms,0xffffu)):std::uint16_t{};
      const auto pickup_buttons=source_pickup_buttons(pickup,pickup_ms,false,0,
          source_pickup_hold_threshold(content.pickup_hold_seconds));
      constexpr std::uint32_t eye=0x6005e000,velocity=eye+16;
      auto call=world.prepare_source_call(std::array{eye,velocity});call.gp[0]=unit;world.run_actual(0x1582f0,call);
      const Vec3 origin{scalar(bank,eye),scalar(bank,eye+4),scalar(bank,eye+8)};const auto d=aim-origin;
      need(!birth_look||(onfoot&&ticks==0&&!fire&&!pickup),"Birth-look prime must be the first neutral on-foot command");
      put(4,std::bit_cast<std::uint32_t>(birth_look?spawn.facing:std::atan2(d.y,d.x)));
      put(8,std::bit_cast<std::uint32_t>(birth_look?0.f:std::atan2(d.z,std::hypot(d.x,d.y))));
      // [DEBUG-campaign-target-20261026] Observe the same command/geometry;
      // do not reorient the view or rerun any targeting/physics owner.
      const auto desired_yaw=Bytes(command).f32(4),desired_pitch=Bytes(command).f32(8);
      const auto pre_selection=snapshot();const auto retained=0x60070000u+(player&65535u)*0xa0u;
      const auto retained_before=Json{{"unit",word(bank,retained)},{"player",word(bank,retained+0x94)},
          {"yaw",scalar(bank,retained+0x10)},{"pitch",scalar(bank,retained+0x14)},
          {"previous_raw_yaw",scalar(bank,retained+0x98)},{"previous_raw_pitch",scalar(bank,retained+0x9c)}};
      Json native_target_calls=Json::array();
      // Default mapped RT carries both held flags and its actual pressure.
      // X supplies only down/elapsed hold;1521B0/1522E0 own edges and bit28.
      put(0,fire?0x10040u:0u);put(0x14,std::bit_cast<std::uint32_t>(fire?1.f:0.f));put(0x1c,pickup_buttons,2);
      world.populate_weapon_selection(unit,command,false);objects.prepare_player_look(player,unit,command);
      const auto actual_yaw=Bytes(command).f32(4),actual_pitch=Bytes(command).f32(8);
      const auto query_selection=snapshot();
#ifdef HALO2_CAMPAIGN_TARGET_DIAGNOSTICS
      targeting_observation=(ticks<6||ticks==59)?&native_target_calls:nullptr;
      try{objects.populate_weapon_targeting(player,unit,command);}catch(...){targeting_observation=nullptr;throw;}
      targeting_observation=nullptr;
#else
      objects.populate_weapon_targeting(player,unit,command);
#endif
      Json target_diagnostic{{"tag","DEBUG-campaign-target-20261026"},{"eye",vector(origin)},{"requested_aim",vector(aim)},
          {"desired_yaw",desired_yaw},{"desired_pitch",desired_pitch},{"actual_yaw",actual_yaw},{"actual_pitch",actual_pitch},
          {"actual_look",vector(direction(actual_yaw,actual_pitch))},{"retained_before",retained_before},
          {"retained_yaw",scalar(bank,retained+0x10)},{"retained_pitch",scalar(bank,retained+0x14)},
          {"selected_before",pre_selection.selected},{"current_before",pre_selection.current},
          {"requested_before",pre_selection.requested},{"ready_before",pre_selection.ready},{"native_target_calls",native_target_calls}};
      target_diagnostic["selector_at_query"]={{"selected",query_selection.selected},{"current",query_selection.current},
          {"requested",query_selection.requested},{"ready",query_selection.ready},
          {"definition",query_selection.selected==null_datum?null_datum:objects.object_definition(query_selection.selected)}};
      if(onfoot) {
        const auto sphere=objects.object_sphere(target);const auto target_motion=objects.biped_motion(target);
        target_diagnostic["target_sphere"]={{"center",vector(sphere.center)},{"radius",sphere.radius}};
        target_diagnostic["target_position"]=vector(target_motion.position);target_diagnostic["target_velocity"]=vector(target_motion.velocity);
        target_diagnostic["source_position"]=vector(pre_selection.motion.position);
        target_diagnostic["eye_to_sphere_center_distance"]=length(sphere.center-origin);
        const auto actual=direction(actual_yaw,actual_pitch),to=sphere.center-origin;
        target_diagnostic["actual_aim_cosine_to_sphere"]=length(to)>0?dot(actual,to)/length(to):1.f;
      }
      entry_prompts+=!onfoot && Bytes(command).u16(0x28)==5 && Bytes(command).u32(0x2c)==vehicle;
      objects.deliver_command(player,unit,command);objects.step_objects();++ticks;const auto s=snapshot();
      const bool source_x_after=(word(bank,player_row+0x3c,2)&1u)!=0;
      need(source_x_after==pickup,"Native mapped X history did not consume the delivered down/release state");
      if(!pickup)pickup_release_observed=true;
      pickup_was_down=pickup;
      mode6_samples+=s.motion.physical_state==6;max_speed=std::max(max_speed,length(s.motion.velocity));
      if(onfoot)closest=std::min(closest,length(objects.object_transform(target).position-s.motion.position));
      for(const auto &impact:world.take_melee_impacts())contacts+=impact.attacker==unit;
      report["frames"].push_back({{"tick",ticks},{"phase",phase},{"command_time_ms",command_ms},
          {"fire",fire},{"rt_pressure",Bytes(command).f32(0x14)},{"pickup",pickup},{"pickup_milliseconds",pickup_ms},
          {"pickup_buttons",pickup_buttons},{"source_x_before",source_x_before},{"source_x_after",source_x_after},
          {"unit_input_flags",word(bank,body+0x148)},{"parent",s.motion.parent},
          {"selected",s.selected},{"current",s.current},{"requested",s.requested},{"ready",s.ready},
          {"target_diagnostic",std::move(target_diagnostic)},
          {"seat",s.motion.seat},{"physical_state",s.motion.physical_state},{"actions",s.actions},{"activity",s.activity},
          {"target",s.target},{"confidence",s.confidence},{"command_target",Bytes(command).u32(0x34)},
          {"command_confidence",Bytes(command).f32(0x50)},{"position",vector(s.motion.position)},{"velocity",vector(s.motion.velocity)}});
      return s;
    };
    if(onfoot) {
      // A new raw-input baseline intentionally applies no first delta. Start
      // with the declared horizontal native birth look, then aim normally.
      // This is a delivered neutral command, never a retained-look write.
      step("neutral-birth-look-prime",false,false,objects.object_sphere(target).center,true);
      const auto primed=objects.player_look(player,unit);
      need(primed.has_value(),"Neutral birth-look command did not publish an actual native look owner");
      const auto period=scalar(bank,0x44923c),expected_yaw=spawn.facing<0?spawn.facing+period:spawn.facing;
      const auto yaw_error=std::remainder(primed->x-spawn.facing,period);
      // Native FPATAN followed by the original positive-yaw wrap may round
      // once at each spill. Admit one representable float at this measured
      // input boundary; this does not relax any targeting/damage gate.
      const auto yaw_ulp=std::abs(std::nextafter(expected_yaw,std::numeric_limits<float>::infinity())-expected_yaw);
      const auto retained=0x60070000u+(player&65535u)*0xa0u;
      report["birth_look_prime"]={{"declared_yaw",spawn.facing},{"declared_pitch",0.f},
          {"processed_yaw",primed->x},{"processed_pitch",primed->y},{"wrapped_yaw_error",yaw_error},{"yaw_float_ulp",yaw_ulp},
          {"retained_yaw",scalar(bank,retained+0x10)},{"retained_pitch",scalar(bank,retained+0x14)},
          {"raw_baseline_yaw",scalar(bank,retained+0x98)},{"raw_baseline_pitch",scalar(bank,retained+0x9c)}};
      need(std::abs(yaw_error)<=yaw_ulp&&primed->y==0.f&&
          word(bank,retained+0x98)==std::bit_cast<std::uint32_t>(spawn.facing)&&word(bank,retained+0x9c)==0,
          "Processed/retained neutral prime differs from its declared genuine native birth look");
      report["stage"]="previous native hostile-target/selector admission";bool qualified=false;
      for(unsigned n=0;n<60;++n) {
        const auto aim=objects.object_sphere(target).center;const auto s=step("target-warm",false,false,aim);
        const bool accepted=s.target==target && s.confidence>=1 && s.selected==weapon && s.current==s.requested && s.ready<=0;
        qualified_samples+=accepted;if(n>=5 && accepted){qualified=true;break;}
      }
      need(qualified,"Actual previous Campaign target/selector/confidence did not qualify the Sword request");
      report["stage"]="ordinary delivered RT Sword control";const auto before=snapshot();
      report["before_rt"]={{"target",before.target},{"confidence",before.confidence},{"current",before.current},{"requested",before.requested},{"ready",before.ready}};
      step("RT",true,false,objects.object_sphere(target).center);
      for(unsigned n=0;n<40;++n)step("RT-release",false,false,objects.object_sphere(target).center);
      const auto vital=objects.object_vitality(target);report["target_vitality_before"]=initial_vitality;report["target_vitality_after"]=vital.body+vital.shield;
      need(mode6_samples && witness.action27_sets && contacts && vital.body+vital.shield<initial_vitality,
          "Campaign on-foot Sword control did not produce real native lunge/contact/damage");
    } else {
      // Establish released native history before the first declared X press.
      step("mapped-X-release-prime",false,false,entry_aim);
      report["stage"]="ordinary pickup-driven seat entry";bool entered=false;
      for(unsigned n=0;n<120;++n){const auto s=step("enter",false,true,entry_aim);if(s.motion.parent==vehicle && s.motion.seat==0){entered=true;break;}}
      need(entered && entry_prompts,"Campaign Ghost did not enter through its actual ordinary interaction prompt");
      auto controlled=world.prepare_source_call();controlled.gp[0]=unit;world.run_actual(0xcdbb0,controlled);
      report["seated_original_controlled_unit"]=controlled.gp[0];
      report["seated_target_scope"]="Control qualifies interaction only; biped reticle is not seated original-producer proof";
      for(unsigned n=0;n<8;++n)step("release-pickup",false,false,entry_aim);
      report["stage"]="ordinary pickup-driven seat exit";bool exited=false;
      for(unsigned n=0;n<120;++n){const auto s=step("exit",false,true,entry_aim);if(s.motion.parent==null_datum && s.motion.seat==-1){exited=true;break;}}
      need(exited && witness.action29_sets && witness.parent_stores>=2 && witness.seat_stores>=2,
          "Campaign ordinary exit lacks real parent/seat and ED1D3 action29 stores");
      for(unsigned n=0;n<8;++n)step("post-exit",false,false,entry_aim);
    }
    report["passed"]=true;report["stage"]="qualified native Campaign control";
    report["summary"]={{"ticks",ticks},{"mode6_samples",mode6_samples},{"contacts",contacts},{"qualified_target_samples",qualified_samples},
        {"entry_prompts",entry_prompts},{"max_speed",max_speed},{"closest_target_distance",onfoot?closest:0},
        {"pickup_presses",pickup_presses},{"action27_stores",witness.action27_sets},{"action29_sets",witness.action29_sets},
        {"parent_stores",witness.parent_stores},{"seat_stores",witness.seat_stores}};
    report["source_owner_pcs"]=witness.owner_pcs;report["source_stores"]=witness.stores;
    std::cout<<report.dump()<<'\n';return 0;
  } catch(const std::exception &e) {report["error"]=e.what();std::cout<<report.dump()<<'\n';return 1;}
}
