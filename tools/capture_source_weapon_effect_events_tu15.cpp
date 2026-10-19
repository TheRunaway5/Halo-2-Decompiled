#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_projectile_effect_observer.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void require_mcc(const std::filesystem::path &path) {
  if(std::filesystem::is_symlink(path) || !std::filesystem::is_regular_file(path))
    throw FormatError("Weapon effects require a direct imported MCC map");
  std::ifstream file(path,std::ios::binary);std::array<unsigned char,8> header{};
  file.read(reinterpret_cast<char *>(header.data()),header.size());
  if(file.gcount()!=8 || header!=std::array<unsigned char,8>{0x64,0x61,0x65,0x68,13,0,0,0})
    throw FormatError("Weapon effects require MCC version13 map and shared caches");
}
std::filesystem::path output_directory(const char *requested) {
  const auto *owner=std::getenv("HALO2_TEST_SCRATCH");
  if(!owner || !*owner)throw FormatError("Run weapon effects through tools/run_test.py");
  const auto scratch=std::filesystem::canonical(owner);
  const auto output=std::filesystem::weakly_canonical(std::filesystem::absolute(requested));
  const auto relative=output.lexically_relative(scratch);
  if(relative.empty() || relative.is_absolute() || *relative.begin()==".." || std::filesystem::is_symlink(requested))
    throw FormatError("Weapon effects output must remain in HALO2_TEST_SCRATCH");
  std::filesystem::create_directories(output);return output;
}
}
int main(int argc,char **argv)try {
  if(argc<5 || argc>7)throw FormatError("Supply MAP SHARED --mcc OUTPUT_DIRECTORY [STARTING_WEAPON|authored-plasma-pistol] [match|charge|charge-instant|impact|unmapped|network]");
  if(std::string_view(argv[3])!="--mcc")throw FormatError("Weapon effect runtime accepts --mcc only; original executables are offline references");
  const bool network=argc==7 && std::string_view(argv[6])=="network";
  const bool unmapped=argc==7 && std::string_view(argv[6])=="unmapped";
  if(argc==7 && std::string_view(argv[6])=="charge-positive")
    throw FormatError("Retained positive charge requires an authored idle-loop actor; plasma-pistol charge is an instant jpt! effect");
  const bool instant_charge=argc==7 && std::string_view(argv[6])=="charge-instant";
  const bool charged=instant_charge || (argc==7 && std::string_view(argv[6])=="charge");
  const bool impact=argc==7 && std::string_view(argv[6])=="impact";
  const bool normal_match=network || unmapped || charged || impact || (argc==7 && std::string_view(argv[6])=="match");
  if(argc==7 && !normal_match)throw FormatError("Unknown source firing probe mode");
  require_mcc(argv[1]);
  if(std::filesystem::is_symlink(argv[2]) || !std::filesystem::is_directory(argv[2]))
    throw FormatError("Weapon effects require the imported MCC shared directory");
  const auto maps=std::filesystem::canonical(argv[2]);
  if(std::filesystem::canonical(argv[1]).parent_path()!=maps)
    throw FormatError("Weapon effect map must belong to its imported MCC shared directory");
  for(const auto *name:{"shared.map","single_player_shared.map","mainmenu.map"})require_mcc(maps/name);
  const auto output=output_directory(argv[4]);
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=maps;
  options.source_object_world=true;options.mute=true;
  options.source_object_savegame_directory=output/"source-savegames";
  const bool authored_pistol=argc>=6 && std::string_view(argv[5])=="authored-plasma-pistol";
  if(authored_pistol && !charged)throw FormatError("Authored plasma-pistol selection requires a charge mode");
  options.starting_weapon=argc>=6 && !authored_pistol?std::stoul(argv[5]):5;
  Cache admission(options.map,options.shared_maps);
  if(!admission.is_mcc())throw FormatError("Weapon effects require actual MCC metadata");
  nlohmann::json charge_metadata;
  std::uint32_t charge_damage=null_datum;
  if(charged) {
    const auto *pistol=admission.query_tag({.group=tag_groups::weapon,
        .name="objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol",.require_address=true});
    if(!pistol)throw FormatError("Imported MCC cache has no resident plasma pistol");
    if(authored_pistol) {
      unsigned choice{};
      for(unsigned candidate=3;candidate<=20;++candidate) {
        std::uint32_t definition;
        try {definition=admission.slayer_starting_weapon(candidate);}catch(const FormatError &){continue;}
        if(definition==pistol->datum) {
          if(choice)throw FormatError("Authored plasma-pistol starting choice is ambiguous");
          choice=candidate;
        }
      }
      if(!choice)throw FormatError("Imported MCC plasma pistol has no authored starting choice");
      options.starting_weapon=choice;
    }
    if(admission.slayer_starting_weapon(options.starting_weapon)!=pistol->datum)
      throw FormatError("Charge mode requires the actual authored plasma-pistol starting choice");
    if(instant_charge) {
      const auto weapon=admission.load_source_tag_metadata(pistol->datum);
      const Bytes root(weapon.bytes_at(weapon.source_address(),weapon.size()));
      if(root.u32(0x2c8)!=1)throw FormatError("Instant charge requires the actual single original pistol trigger");
      const Bytes trigger(weapon.bytes_at(root.u32(0x2cc),0x40));
      const auto effect=admission.load_source_tag_metadata(trigger.u32(0x34));
      const Bytes e(effect.bytes_at(effect.source_address(),effect.size()));
      if(effect.group()!="effe" || e.u32(0x14)!=1)
        throw FormatError("Instant charge effect has no complete original event");
      const Bytes event(effect.bytes_at(e.u32(0x18),0x38));
      if(event.u32(0x18)!=1)throw FormatError("Instant charge effect differs from its single jpt! part");
      const Bytes part(effect.bytes_at(event.u32(0x1c),0x38));charge_damage=part.u32(0x10);
      const auto *damage=admission.query_tag({.handle=TagHandle{charge_damage},.group=tag_groups::damage_effect});
      charge_metadata={{"schema","H2_MCC_INSTANT_PISTOL_CHARGE_METADATA_V1"},
          {"effect",trigger.u32(0x34)},{"flags",e.u32(0)},{"original_flags",6},
          {"loop_start",std::bit_cast<std::int16_t>(e.u16(4))},{"events",e.u32(0x14)},
          {"duration_bits",{event.u32(0x10),event.u32(0x14)}},{"parts",event.u32(0x18)},
          {"part_group",part.u32(8)},{"damage",charge_damage},
          {"particle_systems",event.u32(0x30)},{"beams",event.u32(0x20)},
          {"accelerations",event.u32(0x28)},{"runtime_cache_source","MCC"}};
      const bool equivalent=trigger.u16(6)==2 && trigger.u16(0x18)==1 && trigger.u16(0x1a)==0 &&
          e.u32(0)==6 && e.u16(4)==0xffff && event.u32(0x10)==0x3d088889u &&
          event.u32(0x14)==0x3d088889u && part.u32(8)==0x6a707421u && damage &&
          !event.u32(0x20) && !event.u32(0x28) && !event.u32(0x30);
      charge_metadata["equivalent_to_original"]=equivalent;
      std::ofstream(output/"authored-charge.json")<<charge_metadata.dump(2)<<'\n';
      if(!equivalent)throw FormatError("MCC effective pistol charging metadata differs from the authenticated original instant jpt! contract");
    }
  }
  options.session_random_seed=0x12345678;options.defer_initial_player_spawn=true;
  if(impact) {
    Cache placement(options.map,options.shared_maps);
    const auto content=placement.load_content(0,true);CollisionWorld geometry(content.collision);
    for(const auto &candidate:placement.spawns()) {
      const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
      const auto floor=geometry.raycast(eye,normalized(Vec3{1,0,-.6841368f}),3.f);
      if(floor && floor->normal.z>.9f) {
        auto spawn=candidate;spawn.facing=0;options.native_spawn_inputs={spawn};break;
      }
    }
    if(options.native_spawn_inputs.empty())throw FormatError("No authored close floor for source contact capture");
  }
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
  options.pre_player_update_owner=[&](MultiplayerMatch &m) {
    for(auto id:players){const auto *p=m.player_from_datum(id);
      if(p && !(p->source_flags&1u))m.apply_source_player_activation(id,p->source_flags,p->source_flags|1u);}
  };
  MultiplayerPreparedWorld world(options);std::array<Controls,16> neutral{};
  if(!world.source_objects || !world.cache->is_mcc())throw FormatError("MCC test did not construct its native source world");
  world.match.step(neutral);world.match.step(neutral);
  auto &objects=*world.source_objects;const auto &unit=world.match.unit(0);
  const std::array<unsigned,1> views{0};world.match.configure_local_views(views,
      world.cache->load_first_person_binding(unit.weapon.definition,2));
  objects.publish_native_observer_position(0,unit.body.position+Vec3{0,0,world.content.player.camera_height});
  auto &memory=objects.memory();const auto word=[&](auto p,unsigned n=4){const Bytes b(memory.view(p,n));return n==2?b.u16(0):b.u32(0);};
  unsigned contact_effects{};
  std::uint64_t contact_serial{};
  const auto weapon=unit.weapon.source_datum,body=objects.source_object_body(weapon);
  const auto unit_datum=unit.source_datum,player_datum=unit.source_player,weapon_definition=unit.weapon.definition;
  const auto yaw=unit.yaw;
  // Diagnostic presentation-boundary input: the real source player/body and
  // original firing continue unchanged, while no native player slot owns this unit.
  if(unmapped)const_cast<Unit &>(world.match.unit(0)).source_player=null_datum;
  std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> peer;
  std::array<Controls,16> received{};std::array<Controls,1> outgoing{};
  if(network) {
    host=std::make_unique<LanHost>(world.match,0,0x545531354658ull);
    peer=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x545531354658ull,1);
    for(unsigned n=0;n<64 && !peer->ready();++n) {
      peer->send(outgoing);host->receive(received);host->broadcast();peer->receive();
    }
    if(!peer->ready())throw FormatError("Actual source firing LAN peer did not join");
  }
  unsigned network_locations{};
  unsigned events{},accepted{},firing_events{};
  const auto &definition=world.content.weapons.at(weapon_definition);
  const auto charging_effect=charged && definition.triggers.size()==1?
      definition.triggers.front().charging_effect:null_datum;
  if(charged && charging_effect==null_datum)throw FormatError("Actual plasma pistol has no authored charging effect");
  std::uint32_t charge_instance=null_datum;
  bool charge_damage_active{},charge_retired{},charge_player_response{},charge_held{},charge_ready{},charge_released{};
  unsigned charge_allocations{},charge_damage_returns{},charge_none_returns{};
  std::uint32_t charged_projectile=null_datum;
  bool charged_projectile_observed{};
  unsigned particle_frames{};
  if(instant_charge) {
    const auto secondary=definition.triggers.front().secondary_barrel;
    if(secondary<0 || std::size_t(secondary)>=definition.barrel_views.size() ||
        !definition.barrel_views[secondary] || definition.barrel_views[secondary]->projectile==null_datum)
      throw FormatError("Instant charge lost its actual authored secondary projectile");
    charged_projectile=definition.barrel_views[secondary]->projectile;
    objects.observe_effect_damage([&](bool before,const SourceEffectDamageCallTu15 &e,const SourceRuntimeMemory &) {
      if(e.effect!=charging_effect)return;
      const DatumPool effects(memory,word(0x4f720cu));const auto record=effects.lookup(e.instance);
      if(record.empty() || Bytes(std::as_bytes(record)).u32(4)!=charging_effect || e.damage!=charge_damage)
        throw FormatError("Instant charge damage lost its actual positive allocation or authored jpt! part");
      if(before) {
        if(charge_damage_active)throw FormatError("Unexpected nested pistol charging damage");
        charge_damage_active=true;charge_instance=e.instance;++charge_allocations;
      } else {
        if(!charge_damage_active || charge_instance!=e.instance)
          throw FormatError("Instant charging damage returned through a different allocation");
        charge_damage_active=false;++charge_damage_returns;
      }
    });
    objects.observe_physics_stores([&](auto,const SourcePhysicsContactRecordStore &store) {
      // Whole157D70's actual final player-response flag publication, reached
      // inside the real charge jpt! call. This is observation, never admission.
      if(charge_damage_active && store.instruction==0x157dfdu && store.written.size()==1 &&
          (std::to_integer<unsigned>(store.written[0])&4u))charge_player_response=true;
      // Whole108DB0 stores the true108A70 return after its initial event ran.
      // The original instant effect must already be retired when NONE is stored.
      if(store.instruction==0x108e26u && store.address==body+0x214u) {
        if(store.written.size()!=4 || Bytes(store.written).u32(0)!=null_datum ||
            charge_instance==null_datum || charge_damage_active || !charge_allocations ||
            charge_damage_returns!=charge_allocations)
          throw FormatError("Instant charge never completed its actual positive damage allocation before returned NONE");
        const DatumPool effects(memory,word(0x4f720cu));
        if(!effects.lookup(charge_instance).empty())
          throw FormatError("Instant charge allocation survived its original returned NONE");
        charge_retired=true;++charge_none_returns;
      }
    });
  }
  const auto observe=[&](const SourceWeaponEffectEventTu15 &e) {
    if(e.unit!=unit_datum || e.input.weapon!=weapon)
      throw FormatError("Original firing effect lost its actual held-weapon parent");
    ++events;accepted+=e.presentation.instance!=null_datum;
    if(e.tick>=82 && e.tick<(charged?170u:98u))++firing_events;
    if(e.presentation.instance!=null_datum && !e.presentation.colors_observed)
      throw FormatError("Completed firing effect lost original colors");
    std::cout<<"Source scheduled effect tick="<<e.tick<<" effect="<<std::hex<<e.input.effect
      <<" instance="<<e.presentation.instance<<std::dec<<'\n';
  };
  if(!normal_match || charged)objects.observe_weapon_effects(observe);
  unsigned before{},after{};
  std::uint32_t journal_serial{};
  // Explicit original delivered commands, with no parallel native weapon step.
  // This tests the source publisher; it is not a complete frontend input history.
  for(unsigned tick=0;tick<(charged?240u:120u);++tick) {
    if(tick==79)before=word(body+0x22cu,2);
    if(normal_match) {
      neutral[0].yaw=yaw;neutral[0].pitch=impact?-.6f:0.f;
      neutral[0].fire=tick>=80 && tick<(charged?140u:92u);
      world.match.step(neutral);
      if(instant_charge) {
        const auto mode=std::to_integer<unsigned>(memory.view(body+0x20cu,1)[0]);
        if(tick>=80 && tick<140) {charge_held|=mode==1;charge_ready|=mode==2;}
        if(tick>=140)charge_released|=mode==0;
        for(const auto &p:world.match.state().weapon_projectiles)
          if(p.definition==charged_projectile && p.datum!=null_datum && tick>=140)
            charged_projectile_observed=true;
        const auto clock=word(memory.native_address(0x51df54u,4));
        const auto delta=std::bit_cast<float>(word(memory.native_address(clock+4u,4)));
        objects.advance_effect_frame(delta);++particle_frames;
      }
      for(const auto &e:world.match.state().grenade_detonations)if(e.serial>contact_serial){
        contact_serial=e.serial;
        if(e.source_effect_instance!=null_datum && !e.source_locations.empty())++contact_effects;
      }
      for(const auto &e:world.match.take_grenade_events())
        std::cout<<"Source projectile event kind="<<unsigned(e.kind)<<" projectile="<<std::hex<<e.projectile<<" effect="<<e.effect<<std::dec<<'\n';
      for(const auto &e:world.match.state().weapon_effects)if(!charged && e.serial>journal_serial) {
        journal_serial=e.serial;
        if(!e.source_scheduled || !e.source_presentation || !e.source_locations_observed)
          throw FormatError("Normal source firing reached a legacy or incomplete presentation path");
        SourceWeaponEffectEventTu15 event;event.tick=e.tick;event.unit=e.unit;
        event.input={e.effect,e.weapon,null_datum,e.a_bits,e.b_bits};
        event.presentation={e.source_instance,e.colors,e.source_presentation};event.velocity=e.velocity;
        observe(event);
      }
      if(charged && (tick==79 || tick==99 || tick==139 || tick==140 || tick==160 || tick==239)) {
        const auto p=world.match.state().players[0];
        std::cout<<"Charged state tick="<<tick<<" mode="
          <<std::to_integer<unsigned>(memory.view(body+0x20cu,1)[0])
          <<" timer="<<std::bit_cast<std::int16_t>(std::uint16_t(word(body+0x20eu,2)))
          <<" flags="<<std::hex<<word(body+0x210u)<<" parent="<<word(body+0x14u)
          <<std::dec<<" power="<<std::bit_cast<float>(word(body+0x184u))
          <<" dead="<<p.dead<<" health="<<p.health<<" shield="<<p.shield<<'\n';
        if(instant_charge)std::cout<<"MCC instant charge allocation="<<std::hex<<charge_instance
            <<std::dec<<" allocations="<<charge_allocations<<" damage_returns="<<charge_damage_returns
            <<" NONE_returns="<<charge_none_returns<<" player_response="<<charge_player_response
            <<" retired="<<charge_retired<<" projectile="<<charged_projectile_observed
            <<" effect_frames="<<particle_frames<<'\n';
      }
      if(network) {
        peer->send(outgoing);host->receive(received);host->broadcast();peer->receive();
        if(!peer->error().empty())throw FormatError(peer->error());
        const auto actual=world.match.state();const auto &remote=peer->state();
        if(remote.tick!=actual.tick || remote.weapon_effects.size()!=actual.weapon_effects.size())
          throw FormatError("LAN did not publish the completed actual source firing tick");
        for(unsigned n=0;n<actual.weapon_effects.size();++n) {
          const auto &a=actual.weapon_effects[n],&b=remote.weapon_effects[n];
          if(a.serial!=b.serial || a.weapon!=b.weapon || a.effect!=b.effect || a.source_instance!=b.source_instance ||
              !a.source_locations_observed || !b.source_locations_observed || a.source_locations.size()!=b.source_locations.size())
            throw FormatError("LAN lost actual source firing identity/location bank");
          for(unsigned i=0;i<a.source_locations.size();++i) {
            const auto &x=a.source_locations[i],&y=b.source_locations[i];
            if(x.name!=y.name || x.frame.scale!=y.frame.scale || x.frame.position.x!=y.frame.position.x ||
                x.frame.position.y!=y.frame.position.y || x.frame.position.z!=y.frame.position.z)
              throw FormatError("LAN changed the completed source firing location");
            for(unsigned axis=0;axis<3;++axis)if(x.frame.basis[axis].x!=y.frame.basis[axis].x ||
                x.frame.basis[axis].y!=y.frame.basis[axis].y || x.frame.basis[axis].z!=y.frame.basis[axis].z)
              throw FormatError("LAN changed the completed source firing orientation");
            ++network_locations;
          }
        }
      }
      after=word(body+0x22cu,2);
      if(world.match.state().players[0].dead) {
        std::cout<<"Original source death ended the input history at tick="<<tick<<'\n';break;
      }
      continue;
    }
    std::array<std::byte,0x5c> command;initialize_source_neutral_command(command);
    const auto put=[&](unsigned at,std::uint32_t value){for(unsigned n=0;n<4;++n)command[at+n]=std::byte(value>>(n*8));};
    const bool fire=tick>=80 && tick<92;
    put(0,fire?0x10000u:0u);put(4,std::bit_cast<std::uint32_t>(yaw));
    put(0x14,std::bit_cast<std::uint32_t>(fire?1.f:0.f));
    objects.deliver_command(player_datum,unit_datum,command);objects.step_objects();
    after=word(body+0x22cu,2);
  }
  if(!normal_match || charged)objects.observe_weapon_effects({});
  if(instant_charge){objects.observe_effect_damage({});objects.observe_physics_stores({});}
  if((definition.capacity>0 && !(after<before)) || !events || !firing_events || (!charged && !accepted))
    throw FormatError("Actual source firing did not publish its completed effect requests");
  if(charged && !(std::bit_cast<float>(word(body+0x184u))>0.f))
    throw FormatError("Original charged discharge did not debit its source battery");
  if(instant_charge && (charge_instance==null_datum || !charge_allocations ||
      charge_damage_returns!=charge_allocations || !charge_none_returns || !charge_retired ||
      !charge_player_response || !charge_held || !charge_ready || !charge_released ||
      !charged_projectile_observed || particle_frames!=240))
    throw FormatError("MCC instant charge did not complete actual allocation/damage/player-response/retirement plus natural hold/release and charged projectile");
  if(options.starting_weapon==5 && (firing_events!=before-after || accepted!=events))
    throw FormatError("SMG source firing did not publish exactly one completed effect per spent round");
  if(unmapped && (world.match.state().weapon_effects.empty() ||
      world.match.state().players[0].weapon_effect.serial))
    throw FormatError("Non-player source firing was dropped or fabricated a native player effect owner");
  if(network && !network_locations)throw FormatError("LAN source firing never delivered an actual location");
  if(world.match.simulation().shots()!=0)throw FormatError("Native adapter fired during the direct source history");
  if(impact && !contact_effects)throw FormatError("Floor-directed source projectile produced no completed contact effect");
  const nlohmann::json result={{"schema","H2_MCC_SOURCE_WEAPON_EFFECTS_V1"},{"passed",true},
      {"runtime_cache_source","MCC"},{"cache_version",13},{"original_executable_runtime_input",false},
      {"mode",argc==7?argv[6]:"direct"},{"starting_weapon",options.starting_weapon},
      {"weapon_definition",weapon_definition},{"events",events},{"accepted",accepted},
      {"firing_events",firing_events},{"rounds_before",before},{"rounds_after",after},
      {"source_battery_spent",std::bit_cast<float>(word(body+0x184u))},
      {"instant_charge_required",instant_charge},{"charge_instance",charge_instance},
      {"charge_positive_allocations",charge_allocations},{"charge_damage_returns",charge_damage_returns},
      {"charge_returned_none",charge_none_returns},{"charge_retired_before_none",charge_retired},
      {"charge_player_response",charge_player_response},{"charge_held",charge_held},
      {"charge_ready",charge_ready},{"charge_released",charge_released},
      {"charged_projectile_observed",charged_projectile_observed},{"actual_effect_frames",particle_frames},
      {"retained_idle_loop_charge_graph_accepted",false},{"charge_metadata",charge_metadata},
      {"full_weapon_parity",false},{"particle_emission_or_render_parity",false}};
  std::ofstream result_file(output/"result.json");result_file<<result.dump(2)<<'\n';
  if(!result_file)throw FormatError("Could not retain the bounded MCC weapon result");
  std::cout<<"PASS "<<(charged?"original charge publisher":normal_match?"normal match source journal":"direct source firing publisher")<<": rounds="<<before<<"->"<<after<<" events="<<events
    <<" firing="<<firing_events<<" accepted="<<accepted<<" contact_effects="<<contact_effects<<" LAN_locations="<<network_locations<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
