#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_projectile_effect_observer.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc<5 || argc>7)throw FormatError("Supply MAP SHARED TU15 OUTPUT_DIRECTORY [STARTING_WEAPON] [match|charge|impact|unmapped|network]");
  const bool network=argc==7 && std::string_view(argv[6])=="network";
  const bool unmapped=argc==7 && std::string_view(argv[6])=="unmapped";
  const bool charged=argc==7 && std::string_view(argv[6])=="charge";
  const bool impact=argc==7 && std::string_view(argv[6])=="impact";
  const bool normal_match=network || unmapped || charged || impact || (argc==7 && std::string_view(argv[6])=="match");
  if(argc==7 && !normal_match)throw FormatError("Unknown source firing probe mode");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  options.starting_weapon=argc>=6?std::stoul(argv[5]):5;
  options.session_random_seed=0x12345678;options.defer_initial_player_spawn=true;
  if(impact) {
    Cache placement(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
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
  world.match.step(neutral);world.match.step(neutral);
  auto &objects=*world.source_objects;const auto &unit=world.match.unit(0);
  const std::array<unsigned,1> views{0};world.match.configure_local_views(views,
      world.cache->load_first_person_binding(unit.weapon.definition,2));
  objects.publish_native_observer_position(0,unit.body.position+Vec3{0,0,world.content.player.camera_height});
  auto &memory=objects.memory();const auto word=[&](std::uint32_t p,unsigned n=4){const Bytes b(memory.view(p,n));return n==2?b.u16(0):b.u32(0);};
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
  const auto &definition=world.content.weapons.at(weapon_definition);
  if((definition.capacity>0 && !(after<before)) || !events || !firing_events || (!charged && !accepted))
    throw FormatError("Actual source firing did not publish its completed effect requests");
  if(charged && !(std::bit_cast<float>(word(body+0x184u))>0.f))
    throw FormatError("Original charged discharge did not debit its source battery");
  if(options.starting_weapon==5 && (firing_events!=before-after || accepted!=events))
    throw FormatError("SMG source firing did not publish exactly one completed effect per spent round");
  if(unmapped && (world.match.state().weapon_effects.empty() ||
      world.match.state().players[0].weapon_effect.serial))
    throw FormatError("Non-player source firing was dropped or fabricated a native player effect owner");
  if(network && !network_locations)throw FormatError("LAN source firing never delivered an actual location");
  if(world.match.simulation().shots()!=0)throw FormatError("Native adapter fired during the direct source history");
  if(impact && !contact_effects)throw FormatError("Floor-directed source projectile produced no completed contact effect");
  std::cout<<"PASS "<<(charged?"original charge publisher":normal_match?"normal match source journal":"direct source firing publisher")<<": rounds="<<before<<"->"<<after<<" events="<<events
    <<" firing="<<firing_events<<" accepted="<<accepted<<" contact_effects="<<contact_effects<<" LAN_locations="<<network_locations<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
