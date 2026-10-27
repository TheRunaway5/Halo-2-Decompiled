#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <nlohmann/json.hpp>
#include <x86intrin.h>
using namespace halo2;
struct InputWords {
 const SourcePhysicsContactMemory &source;
 explicit InputWords(const SourcePhysicsContactMemory &s):source(s){}
 std::uint32_t word(std::uint32_t at)const {std::array<std::byte,4>b;source.read(at,b);return Bytes(b).u32(0);}
};
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/source_first_person_pose_tu15.hpp"
int main(int argc,char**argv)try {
 if(argc!=4 && argc!=5)throw FormatError("MAP SHARED REPORT [declared-actions]");const bool actions=argc==5;const auto scratch=std::getenv("HALO2_TEST_SCRATCH");if(!scratch)throw FormatError("run_test owner required");
 nlohmann::json report{{"cases",nlohmann::json::array()}};
 for(unsigned rep:{2u,3u}) {
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=0;options.dedicated=true;options.mute=true;options.source_object_world=true;options.starting_weapon=5;
  options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(rep);
  MultiplayerPreparedWorld world(options);auto&match=world.match;auto&native=world.source_objects->tu15_world();auto&memory=native.memory();
  const auto weapon=world.cache->slayer_starting_weapon(5);auto profile=source_default_player_configuration();profile[0x44]=rep==3;const auto slot=match.join("Remote native FP",-1,profile);
  match.configure_replicated_first_person(world.cache->load_first_person_binding(weapon,rep));std::array<Controls,multiplayer_capacity> controls{};
  std::unique_ptr<SourceFirstPersonContextTu15> context;unsigned frames{};bool restored{},bad_restore_rejected{};unsigned rejected_private_owner_attacks{},private_texture_usage_words{};nlohmann::json texture_usage_owner;std::size_t actor_shadow_bytes{};std::string error;float error_max{};bool duplicate_rejected{},identity_rejected{};
  SourcePhysicsWorldRuntimeServices cpu;cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};cpu.timestamp_counter=[](){return __rdtsc();};
  for(unsigned tick=0;tick<110 && frames<40;++tick) {
   const auto before=match.state();const auto &old=before.players[slot];
   if(old.source_unit!=null_datum && old.secondary_weapon.datum==null_datum) {
    const auto pickup=std::find_if(before.world_weapons.begin(),before.world_weapons.end(),[&](const auto&w){return w.definition==weapon;});
    if(pickup!=before.world_weapons.end())native.set_object_position(old.source_unit,pickup->position+Vec3{-.2f,0,0});
   }
   controls[slot]={};controls[slot].dual_wield=true;controls[slot].dual_wield_milliseconds=1000;
   if(actions && frames>=20 && frames<28){controls[slot].fire=true;controls[slot].fire_secondary=true;}
   if(frames>=10){controls[slot].yaw=.4f*std::sin(frames*.1f);controls[slot].pitch=.2f;controls[slot].forward=.2f;}
   match.step(controls);const auto state=match.state();const auto &player=state.players[slot];
   if(player.secondary_weapon.datum==null_datum)continue;
   const auto caller=native.prepare_source_call();SourceRuntimeWriteSerial serial(memory);
   try {
    SourceFirstPersonContextInputTu15 input;
    input.identity={player.source_player,player.source_unit,0xffffffffu,std::int32_t(rep),{player.source_weapon,player.secondary_weapon.datum},{weapon,weapon}};
    InputWords inspect(memory);const auto rows=inspect.word(inspect.word(0x4ecbd0u)+0x44u);
    input.identity.unit_definition=inspect.word(inspect.word(rows+(player.source_unit&65535u)*12u+8u));
    const auto retained=0x60070000u+(player.source_player&65535u)*0xa0u;
    memory.read(retained,input.look);memory.read(inspect.word(0x4f3cd8u),std::as_writable_bytes(std::span(input.initial_rng)));
    if(!context){context=std::make_unique<SourceFirstPersonContextTu15>(*world.cache);context->reset(memory,caller,input);}
    if(actions && (frames==20 || frames==23 || frames==26)) {
      context->prepare_identity(memory,caller,input);
      const auto clock=inspect.word(inspect.word(0x51df54u)+8u);
      context->apply_action(memory,caller,{std::uint64_t(frames)*2u+1u,clock,player.source_unit,player.source_weapon,1,0});
      context->apply_action(memory,caller,{std::uint64_t(frames)*2u+2u,clock,player.source_unit,player.secondary_weapon.datum,19,1});
    }
    context->advance_tick(memory,caller,input);const auto published=context->state(),sampling=context->pose_inputs();
    if(frames==15) {
      const auto saved=context->checkpoint();
      auto fresh=std::make_unique<SourceFirstPersonContextTu15>(*world.cache);
      fresh->prepare_restore(memory,caller,saved);
      const auto after=fresh->state();
      if(after.hands[0].control!=published.hands[0].control || after.hands[1].control!=published.hands[1].control ||
          after.hands[0].saved_locals!=published.hands[0].saved_locals || after.hands[1].saved_locals!=published.hands[1].saved_locals)
        throw FormatError("Portable native continuation changed its retained temporal controls");
      const auto texture=inspect.word(0x4f2d34u),texture_rows=inspect.word(inspect.word(texture+0x64u)+0x44u);
      texture_usage_owner={{"allocator",texture},{"rows",texture_rows},{"bitmap",inspect.word(texture+0xb8u)},
          {"capacity",inspect.word(texture+0x90u)},{"stride",inspect.word(texture+0x94u)}};
      const auto retained_word=[](const auto &checkpoint,std::uint32_t address)->std::optional<std::uint32_t> {
        for(const auto &range:checkpoint.private_ranges)
          if(range.address<=address && std::uint64_t(address)+4u<=std::uint64_t(range.address)+range.bytes.size())
            return Bytes(range.bytes).u32(address-range.address);
        return {};
      };
      const auto restored_checkpoint=fresh->checkpoint();
      for(unsigned index=0;index<0x200u;++index) {
        const auto address=texture_rows+index*0x18u+0x14u;
        if(const auto before=retained_word(saved,address)) {
          if(retained_word(restored_checkpoint,address)!=before)
            throw FormatError("Portable native continuation changed a texture usage timestamp");
          ++private_texture_usage_words;
        }
      }
      if(!private_texture_usage_words)throw FormatError("Native FP context did not exercise texture usage ownership");
      auto corrupt=saved;corrupt.owner_roots[0]^=4u;
      try{SourceFirstPersonContextTu15 invalid(*world.cache);invalid.prepare_restore(memory,caller,corrupt);}
      catch(const FormatError&){bad_restore_rejected=true;}
      // Continuations may retain only their typed native presentation/cache
      // owners. Identical bytes from authoritative actors are still forbidden:
      // accepting them would make later reads shadow the live World.
      const auto reject_private=[&](auto corrupt) {
        bool rejected{};
        try {SourceFirstPersonContextTu15 invalid(*world.cache);invalid.prepare_restore(memory,caller,corrupt);}
        catch(const FormatError&) {rejected=true;}
        if(!rejected)throw FormatError("Native FP continuation accepted a private shared-input owner");
        if(serial.load())throw FormatError("Rejected native FP continuation changed authority input bytes");
        ++rejected_private_owner_attacks;
      };
      const auto object_pool=inspect.word(0x4ecbd0u),player_pool=inspect.word(0x4f54f4u);
      const auto player_row=inspect.word(player_pool+0x44u)+(player.source_player&65535u)*0x21cu;
      for(const auto address:std::array<std::uint32_t,11>{
          inspect.word(rows+(player.source_unit&65535u)*12u+8u)+0x100u,
          inspect.word(rows+(player.source_weapon&65535u)*12u+8u),
          inspect.word(rows+(player.secondary_weapon.datum&65535u)*12u+8u)+0x22cu,
          0x4ecbd0u,object_pool+0x38u,player_pool+0x44u,player_row+0x2cu,
          0x4f2d34u,texture+0x64u,texture_rows+0x10u,texture_rows+0x18u}) {
        auto corrupt=saved;std::array<std::byte,4> bytes;memory.read(address,bytes);
        corrupt.private_ranges.push_back({address,{bytes.begin(),bytes.end()}});
        std::sort(corrupt.private_ranges.begin(),corrupt.private_ranges.end(),
            [](const auto&a,const auto&b){return a.address<b.address;});
        reject_private(std::move(corrupt));
      }
      const auto replace_word=[&](auto &corrupt,std::uint32_t at,std::uint32_t value) {
        const auto bytes=std::as_bytes(std::span(&value,1));
        for(unsigned n=0;n<bytes.size();++n) {
          const auto found=std::find_if(corrupt.private_ranges.begin(),corrupt.private_ranges.end(),
              [&](const auto&r){return r.address<=at+n && std::uint64_t(at+n)-r.address<r.bytes.size();});
          if(found==corrupt.private_ranges.end())throw FormatError("Native FP test lacks the retained cache topology operand");
          found->bytes[at+n-found->address]=bytes[n];
        }
      };
      auto topology=saved;replace_word(topology,inspect.word(0x4f041cu)+0x44u,rows);
      reject_private(std::move(topology));
      context=std::move(fresh);restored=true;
    }
    if(frames==30) {
      const auto saved=context->checkpoint();
      for(const auto actor:{player.source_unit,player.source_weapon,player.secondary_weapon.datum}) {
        const auto row=rows+(actor&65535u)*12u,body=inspect.word(row+8u),bytes=inspect.word(row+4u)>>16u;
        if(!bytes)throw FormatError("Native actor header lacks its actual logical body extent");
        for(const auto &range:saved.private_ranges) {
          const auto begin=std::max<std::uint64_t>(body,range.address);
          const auto end=std::min<std::uint64_t>(std::uint64_t(body)+bytes,std::uint64_t(range.address)+range.bytes.size());
          if(end>begin)actor_shadow_bytes+=end-begin;
        }
      }
      if(actor_shadow_bytes)throw FormatError("Private native FP context shadowed mutable actor input bytes");
    }
    if(!frames) {
     try{context->advance_tick(memory,caller,input);}catch(const FormatError&){duplicate_rejected=true;}
     auto wrong=input;wrong.identity.player^=0x10000u;
     try{SourceFirstPersonContextTu15 invalid(*world.cache);invalid.reset(memory,caller,wrong);}catch(const FormatError&){identity_rejected=true;}
    }
    std::vector<SourceFirstPersonModelTu15> sampled;
    context->with_sampling_inputs(memory,caller,sampling,[&](const auto &inputs){
     sampled=source_sample_first_person_pose_tu15(*world.cache,inputs,caller,0,{1,2,3},{1,0,0},{0,0,1},cpu);
    });
    if(sampled.size()!=3)throw FormatError("Private native receiver modelcount="+std::to_string(sampled.size()));
    ++frames;
   }catch(const std::exception&e){error=e.what();break;}
   if(serial.load())throw FormatError("Private native context changed authority input bytes");
  }
  const auto state=context?context->state():SourceFirstPersonTemporalStateTu15{};
  report["cases"].push_back({{"representation",rep},{"texture_usage_owner",texture_usage_owner},{"private_texture_usage_words",private_texture_usage_words},{"rejected_private_owner_attacks",rejected_private_owner_attacks},{"frames",frames},{"error",error},{"actor_shadow_bytes",actor_shadow_bytes},{"portable_restore",restored},{"wrong_root_restore_rejected",bad_restore_rejected},{"declared_actions",actions},{"duplicate_tick_rejected",duplicate_rejected},{"stale_player_rejected",identity_rejected},{"source_tick",state.source_tick},{"saved_locals",{state.hands[0].saved_locals.size(),state.hands[1].saved_locals.size()}},{"flags",{Bytes(state.hands[0].control).u32(0),Bytes(state.hands[1].control).u32(0)}},{"class",{Bytes(state.hands[0].control).u32(0x7c),Bytes(state.hands[1].control).u32(0x7c)}}});
 }
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
 for(const auto &row:report["cases"])if(row["frames"]!=40 || row["error"]!="" || row["rejected_private_owner_attacks"]!=12 || row["private_texture_usage_words"]==0 || !row["portable_restore"].get<bool>() || !row["wrong_root_restore_rejected"].get<bool>() || !row["duplicate_tick_rejected"].get<bool>() || !row["stale_player_rejected"].get<bool>())return 1;
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
