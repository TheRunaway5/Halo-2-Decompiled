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
 if(argc<4 || argc>6)throw FormatError("MAP SHARED REPORT [declared-actions] [TU15_XBE]");const bool actions=argc>=5 && std::string_view(argv[4])=="declared-actions";const unsigned reference_argument=actions?5u:4u;const auto scratch=std::getenv("HALO2_TEST_SCRATCH");if(!scratch)throw FormatError("run_test owner required");
 nlohmann::json report{{"cases",nlohmann::json::array()}};
 for(unsigned rep:{2u,3u}) {
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=0;options.dedicated=true;options.mute=true;options.source_object_world=true;options.starting_weapon=5;
  if(unsigned(argc)>reference_argument)options.gameplay_xbe=argv[reference_argument];
  options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};options.source_object_savegame_directory=std::filesystem::path(scratch)/std::to_string(rep);
  MultiplayerPreparedWorld world(options);auto&match=world.match;auto&native=world.source_objects->tu15_world();auto&memory=native.memory();
  const auto weapon=world.cache->slayer_starting_weapon(5);auto profile=source_default_player_configuration();profile[0x44]=rep==3;const auto slot=match.join("Remote native FP",-1,profile);
  const auto binding=world.cache->load_first_person_binding(weapon,rep);
  match.configure_replicated_first_person(binding);std::array<Controls,multiplayer_capacity> controls{};
  // A rendered recipient has genuinely resident FP geometry. Headless match
  // ticks alone only queue it, so they cannot exercise1307C0/130980's resident
  // usage stores. Use the production1717B0→130980 resource producer before
  // the private binding; never install a token/header by hand.
  std::vector<std::uint32_t> resident_models{binding.arms_model,binding.weapon_model};
  std::sort(resident_models.begin(),resident_models.end());
  resident_models.erase(std::unique(resident_models.begin(),resident_models.end()),resident_models.end());
  unsigned resident_sections{};
  InputWords initial(memory);
  const auto make_geometry_resident=[&] {
  for(const auto model:resident_models) {
    native.preload_render_model(model);
    const auto mode=initial.word(initial.word(0x4f0414u)+(model&65535u)*16u+8u);
    const auto count=initial.word(mode+0x24u),sections=initial.word(mode+0x28u);
    for(unsigned index=0;index<count;++index) {
      const auto header=sections+index*0x5cu+0x38u;
      if(std::bit_cast<std::int32_t>(initial.word(header+8u))<=0)continue;
      if(!initial.word(header-4u) || !(initial.word(header+0x1cu)&0x00ff0000u) ||
          initial.word(header+0x20u)==null_datum)
        throw FormatError("Production FP geometry preload did not publish its real resident resource");
      ++resident_sections;
    }
  }
  if(!resident_sections)throw FormatError("Native FP usage fixture lacks an actual authored resident section");
  };
  make_geometry_resident();
  nlohmann::json prediction_inputs;
  std::uint32_t inherited_geometry_header{},inherited_lightmap_header{};
  const auto seed_native_predictions=[&] {
    const auto table=initial.word(0x4f0414u);
    for(const auto& tag:world.cache->select_tags({.group=TagGroup::from_string("mode"),.require_address=true})) {
      if(inherited_geometry_header)break;
      // A genuine unrelated vehicle asset proves global ownership rather
      // than silently reusing this actor's FP body/arms/weapon selection.
      if(tag.name.find("objects\\vehicles\\")==std::string::npos)continue;
      const auto root=initial.word(table+(tag.datum&65535u)*16u+8u);
      const auto count=initial.word(root+0x24u),first=initial.word(root+0x28u);
      for(unsigned section=0;section<count;++section) {
        const auto header=first+section*0x5cu+0x38u;
        if(std::bit_cast<std::int32_t>(initial.word(header+8u))<=0 ||
            std::bit_cast<std::int32_t>(initial.word(header+0xcu))<0 ||
            (initial.word(header+0x1cu)&0x04ff0000u))continue;
        const auto pool=initial.word(0x4f2d60u),before=initial.word(pool+0x3cu);
        auto call=native.prepare_source_call();call.gp[6]=header;native.run_actual(0x1307c0u,call);
        if(initial.word(pool+0x3cu)!=before+1u || !(initial.word(header+0x1cu)&0x04000000u))
          throw FormatError("Whole native1307C0 did not create its actual global geometry prediction");
        inherited_geometry_header=header;
        prediction_inputs["geometry"]={{"datum",tag.datum},{"name",tag.name},{"section",section},{"header",header},{"pool",pool}};
        break;
      }
    }
    if(!inherited_geometry_header)
      throw FormatError("Native prediction fixture lacks an actual cold authored foreign geometry input");
    const auto lightmap=initial.word(0x4ecc14u),bsp=initial.word(0x4ecc18u);
    if(!lightmap || !initial.word(lightmap+0x80u))throw FormatError("Native prediction fixture lacks its actual ltmp group");
    const auto group=initial.word(lightmap+0x84u),count=initial.word(group+0x50u),refs=initial.word(group+0x54u);
    if(initial.word(group+0x1cu)==null_datum || initial.word(group+4u)!=initial.word(bsp+8u))
      throw FormatError("Native prediction fixture cannot use absent/stale native lightmap lighting");
    for(unsigned instance=0;instance<count;++instance) {
      const auto bucket=initial.word(refs+instance*12u)>>16u;
      if(bucket>=initial.word(group+0x40u))continue;
      const auto header=initial.word(group+0x44u)+bucket*0x38u+0xcu;
      if(std::bit_cast<std::int32_t>(initial.word(header+8u))<=0 ||
          std::bit_cast<std::int32_t>(initial.word(header+0xcu))<0 ||
          (initial.word(header+0x1cu)&0x04ff0000u))continue;
      const auto pool=initial.word(0x4f2d60u),before=initial.word(pool+0x3cu);
      auto call=native.prepare_source_call();call.gp[2]=instance;native.run_actual(0x171750u,call);
      if(initial.word(pool+0x3cu)!=before+1u || !(initial.word(header+0x1cu)&0x04000000u))
        throw FormatError("Whole native171750 did not nominate its actual ltmp bucket prediction");
      inherited_lightmap_header=header;
      prediction_inputs["lightmap"]={{"root",lightmap},{"group",group},{"instance",instance},
          {"bucket",bucket},{"header",header},{"pool",pool},{"native_producer",0x171750u}};
      break;
    }
    if(!inherited_lightmap_header)throw FormatError("Native prediction fixture lacks a genuine cold referenced ltmp bucket");
  };
  std::unique_ptr<SourceFirstPersonContextTu15> context;unsigned frames{};bool restored{},bad_restore_rejected{};unsigned rejected_private_owner_attacks{},private_texture_usage_words{},private_geometry_usage_words{},private_bitmap_requests{};nlohmann::json texture_usage_owner,geometry_usage_owner,bitmap_prediction_owner;std::size_t actor_shadow_bytes{};std::string error;float error_max{};bool duplicate_rejected{},identity_rejected{};
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
   const auto caller=native.prepare_source_call();
   try {
    SourceFirstPersonContextInputTu15 input;
    input.identity={player.source_player,player.source_unit,0xffffffffu,std::int32_t(rep),{player.source_weapon,player.secondary_weapon.datum},{weapon,weapon}};
    InputWords inspect(memory);const auto rows=inspect.word(inspect.word(0x4ecbd0u)+0x44u);
    input.identity.unit_definition=inspect.word(inspect.word(rows+(player.source_unit&65535u)*12u+8u));
    const auto retained=0x60070000u+(player.source_player&65535u)*0xa0u;
    memory.read(retained,input.look);memory.read(inspect.word(0x4f3cd8u),std::as_writable_bytes(std::span(input.initial_rng)));
    if(!context){
      seed_native_predictions();
      std::optional<PhysicsLiveWorldTu15::AuthorityCheckpoint> before_bitmap;
      if(!world.cache->is_mcc()) {
        // Let whole12F720 create a real original image request, then retire
        // the independent World state through its complete native transaction.
        // No synthetic queue row, cache handle, ready bit or GPU load is used.
        try {before_bitmap.emplace(native.authority_checkpoint());}
        catch(const std::exception& e){throw FormatError(std::string("Original bitmap fixture checkpoint: ")+e.what());}
        bool seeded{};const auto table=inspect.word(0x4f0414u);
        for(const auto& tag:world.cache->select_tags({.group=tag_groups::bitmap,.require_address=true})) {
          if(seeded)break;
          const auto root=inspect.word(table+(tag.datum&65535u)*16u+8u);
          const auto count=inspect.word(root+0x44u),first=inspect.word(root+0x48u);
          for(unsigned image=0;image<count;++image) {
            const auto header=first+image*116u;
            if(inspect.word(header+0x28u)!=null_datum || (inspect.word(header+0xcu)&0x04000000u))continue;
            const auto pool=inspect.word(0x4f2d28u),before=inspect.word(pool+0x3cu);
            auto call=native.prepare_source_call();call.gp[0]=header;
            try {native.run_actual(0x12f720u,call);}
            catch(const std::exception& e){throw FormatError("Original bitmap fixture native12F720 image="+
                std::to_string(header)+": "+e.what());}
            if(inspect.word(pool+0x3cu)!=before+1u || !(inspect.word(header+0xcu)&0x04000000u))
              throw FormatError("Whole native12F720 did not produce its genuine original bitmap request");
            prediction_inputs["bitmap"]={{"datum",tag.datum},{"image",image},{"header",header},
                {"pool",pool},{"native_producer",0x12f720u}};
            seeded=true;break;
          }
        }
        if(!seeded)throw FormatError("Original bitmap fixture lacks an actual cold authored116 image");
      }
      {
        SourceRuntimeWriteSerial birth(memory);
        context=std::make_unique<SourceFirstPersonContextTu15>(*world.cache);
        try {context->reset(memory,caller,input);}
        catch(const std::exception& e){throw FormatError(std::string("Private FP fixture birth: ")+e.what());}
        if(birth.load())throw FormatError("Private FP construction changed its borrowed authority input");
      }
      if(before_bitmap) {
        try {
          if(!native.commit_authority(native.prepare_authority(*before_bitmap)))
            throw FormatError("Actual independent World bitmap checkpoint restoration failed");
        }catch(const std::exception& e){throw FormatError(std::string("Original bitmap fixture restore: ")+e.what());}
        before_bitmap.reset();
        prediction_inputs["bitmap"]["world_restored_after_private_birth"]=true;
      }
      for(unsigned step=0;step<8u && ((initial.word(inherited_geometry_header+0x1cu)|
          initial.word(inherited_lightmap_header+0x1cu))&0x04000000u);++step) {
        auto call=native.prepare_source_call();native.run_actual(0x130720u,call);
      }
      if((initial.word(inherited_geometry_header+0x1cu)|initial.word(inherited_lightmap_header+0x1cu))&0x04000000u)
        throw FormatError("Whole native130720 did not drain the actual World geometry prediction");
      prediction_inputs["geometry"]["world_queued_after_private_birth"]=false;
      prediction_inputs["lightmap"]["world_queued_after_private_birth"]=false;
    }
    SourceRuntimeWriteSerial serial(memory);
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
      const auto geometry=inspect.word(0x4f2d6cu),geometry_rows=inspect.word(inspect.word(geometry+0x64u)+0x44u);
      geometry_usage_owner={{"allocator",geometry},{"rows",geometry_rows},{"bitmap",inspect.word(geometry+0xb8u)},
          {"capacity",inspect.word(geometry+0x90u)},{"stride",inspect.word(geometry+0x94u)},
          {"first_usage",nullptr}};
      const auto retained_word=[](const auto &checkpoint,std::uint32_t address)->std::optional<std::uint32_t> {
        for(const auto &range:checkpoint.private_ranges)
          if(range.address<=address && std::uint64_t(address)+4u<=std::uint64_t(range.address)+range.bytes.size())
            return Bytes(range.bytes).u32(address-range.address);
        return {};
      };
      const auto retained_byte=[](const auto& checkpoint,std::uint32_t address)->std::optional<std::byte> {
        for(const auto& range:checkpoint.private_ranges)if(range.address<=address&&
            std::uint64_t(address)-range.address<range.bytes.size())return range.bytes[address-range.address];
        return {};
      };
      const auto geometry_queue=inspect.word(0x4f2d60u),geometry_queue_rows=inspect.word(geometry_queue+0x44u);
      const auto geometry_queue_bits=inspect.word(geometry_queue+0x48u);
      unsigned inherited_geometry_index{};
      for(;inherited_geometry_index<0x15eu;++inherited_geometry_index) {
        if(!(retained_word(saved,geometry_queue_bits+(inherited_geometry_index/32u)*4u).value()&
            (1u<<(inherited_geometry_index%32u))))continue;
        if(retained_word(saved,geometry_queue_rows+inherited_geometry_index*8u+4u)==inherited_geometry_header)break;
      }
      if(inherited_geometry_index==0x15eu ||
          (retained_byte(saved,inherited_geometry_header+0x1fu).value()&std::byte{4})==std::byte{})
        throw FormatError("Private native global geometry request did not survive actual World draining");
      prediction_inputs["geometry"]["private_retained_after_world_drain"]=true;
      const auto restored_checkpoint=fresh->checkpoint();
      if(retained_word(restored_checkpoint,geometry_queue_rows+inherited_geometry_index*8u)!=
          retained_word(saved,geometry_queue_rows+inherited_geometry_index*8u) ||
          retained_word(restored_checkpoint,geometry_queue_rows+inherited_geometry_index*8u+4u)!=inherited_geometry_header ||
          retained_byte(restored_checkpoint,inherited_geometry_header+0x1fu)!=retained_byte(saved,inherited_geometry_header+0x1fu))
        throw FormatError("Native continuation changed its inherited global geometry request");
      unsigned lightmap_index{};
      for(;lightmap_index<0x15eu;++lightmap_index)if(
          (retained_word(saved,geometry_queue_bits+(lightmap_index/32u)*4u).value()&(1u<<(lightmap_index%32u))) &&
          retained_word(saved,geometry_queue_rows+lightmap_index*8u+4u)==inherited_lightmap_header)break;
      if(lightmap_index==0x15eu ||
          (retained_byte(saved,inherited_lightmap_header+0x1fu).value()&std::byte{4})==std::byte{} ||
          retained_word(restored_checkpoint,geometry_queue_rows+lightmap_index*8u)!=
              retained_word(saved,geometry_queue_rows+lightmap_index*8u) ||
          retained_word(restored_checkpoint,geometry_queue_rows+lightmap_index*8u+4u)!=inherited_lightmap_header ||
          retained_byte(restored_checkpoint,inherited_lightmap_header+0x1fu)!=retained_byte(saved,inherited_lightmap_header+0x1fu))
        throw FormatError("Native continuation lost its genuine ltmp bucket after independent World drain");
      prediction_inputs["lightmap"]["private_retained_after_world_drain"]=true;
      if(!world.cache->is_mcc()) {
        const auto pool=inspect.word(0x4f2d28u);
        private_bitmap_requests=retained_word(saved,pool+0x3cu).value_or(0);
        bitmap_prediction_owner={{"pool",pool},{"rows",inspect.word(pool+0x44u)},
            {"bits",inspect.word(pool+0x48u)},{"private_requests",private_bitmap_requests},{"authored_stride",116}};
        if(!private_bitmap_requests)throw FormatError("Original native FP did not create its actual authored bitmap predictions");
      }
      for(unsigned index=0;index<0x200u;++index) {
        const auto address=texture_rows+index*0x18u+0x14u;
        if(const auto before=retained_word(saved,address)) {
          if(retained_word(restored_checkpoint,address)!=before)
            throw FormatError("Portable native continuation changed a texture usage timestamp");
          ++private_texture_usage_words;
        }
      }
      if(!private_texture_usage_words)throw FormatError("Native FP context did not exercise texture usage ownership");
      for(unsigned index=0;index<0x200u;++index) {
        const auto address=geometry_rows+index*0x18u+0x14u;
        if(const auto before=retained_word(saved,address)) {
          if(retained_word(restored_checkpoint,address)!=before)
            throw FormatError("Portable native continuation changed a geometry usage timestamp");
          if(!private_geometry_usage_words)geometry_usage_owner["first_usage"]=address;
          ++private_geometry_usage_words;
        }
      }
      if(!private_geometry_usage_words)throw FormatError("Native FP context did not exercise resident geometry usage ownership");
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
      for(const auto address:std::array<std::uint32_t,16>{
          inspect.word(rows+(player.source_unit&65535u)*12u+8u)+0x100u,
          inspect.word(rows+(player.source_weapon&65535u)*12u+8u),
          inspect.word(rows+(player.secondary_weapon.datum&65535u)*12u+8u)+0x22cu,
          0x4ecbd0u,object_pool+0x38u,player_pool+0x44u,player_row+0x2cu,
          0x4f2d34u,texture+0x64u,texture_rows+0x10u,texture_rows+0x18u,
          0x4f2d6cu,geometry+0x64u,geometry+0x94u,geometry_rows+0x10u,geometry_rows+0x18u}) {
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
      const auto replace_byte=[&](auto& corrupt,std::uint32_t at,std::byte bit) {
        for(auto& range:corrupt.private_ranges)if(range.address<=at&&std::uint64_t(at)-range.address<range.bytes.size()) {
          range.bytes[at-range.address]^=bit;return;
        }
        throw FormatError("Native FP bitmap fixture lacks its actual private queued flag");
      };
      const auto geometry_request_row=geometry_queue_rows+inherited_geometry_index*8u;
      for(const auto [at,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,6>{{
          {geometry_request_row+4u,inherited_geometry_header+1u},
          {geometry_request_row+4u,inspect.word(rows+(player.source_unit&65535u)*12u+8u)},
          {geometry_request_row,retained_word(saved,geometry_request_row).value()^0x8000u},
          {geometry_queue+0x30u,0x60000000u},
          {geometry_queue+0x3cu,retained_word(saved,geometry_queue+0x3cu).value()+1u},
          {geometry_queue_bits+40u,retained_word(saved,geometry_queue_bits+40u).value()|0x80000000u}}}) {
        auto corrupt=saved;replace_word(corrupt,at,value);reject_private(std::move(corrupt));
      }
      for(const auto bit:{std::byte{2},std::byte{4}}) {
        auto corrupt=saved;replace_byte(corrupt,inherited_geometry_header+0x1fu,bit);reject_private(std::move(corrupt));
      }
      {
        auto corrupt=saved;std::array<std::byte,1> value;memory.read(inherited_geometry_header+0x1eu,value);
        corrupt.private_ranges.push_back({inherited_geometry_header+0x1eu,{value.begin(),value.end()}});
        std::sort(corrupt.private_ranges.begin(),corrupt.private_ranges.end(),[](const auto&a,const auto&b){return a.address<b.address;});
        reject_private(std::move(corrupt));
      }
      for(const auto bit:{std::byte{2},std::byte{4}}) {
        auto corrupt=saved;replace_byte(corrupt,inherited_lightmap_header+0x1fu,bit);reject_private(std::move(corrupt));
      }
      const auto lightmap=inspect.word(0x4ecc14u),group=inspect.word(lightmap+0x84u);
      const auto scenario=world.cache->find_tag(world.cache->scenario_tag(),tag_groups::scenario);
      const Bytes scnr(world.cache->read_tag_bytes(scenario->address,scenario->size));
      const Bytes mapping(world.cache->read_tag_bytes(scnr.u32(0x214u),0x44u));
      for(const auto at:{inherited_lightmap_header+0x1eu,lightmap+0x80u,group+0x44u,group+4u,
          inspect.word(0x4f0414u)+(mapping.u32(0x1cu)&65535u)*16u+4u}) {
        auto corrupt=saved;std::array<std::byte,1> value;memory.read(at,value);
        corrupt.private_ranges.push_back({at,{value.begin(),value.end()}});
        std::sort(corrupt.private_ranges.begin(),corrupt.private_ranges.end(),[](const auto&a,const auto&b){return a.address<b.address;});
        reject_private(std::move(corrupt));
      }
      if(!world.cache->is_mcc()) {
        const auto pool=inspect.word(0x4f2d28u),rows=inspect.word(pool+0x44u),bits=inspect.word(pool+0x48u);
        unsigned index{};
        while(index<0xc8u && !(retained_word(saved,bits+(index/32u)*4u).value()&(1u<<(index%32u))))++index;
        if(index==0xc8u)throw FormatError("Original bitmap positive lacks its actual occupied row");
        const auto row=rows+index*8u,image=retained_word(saved,row+4u).value();
        bitmap_prediction_owner["image"]=image;bitmap_prediction_owner["full_datum"]=(retained_word(saved,row).value()<<16u)|index;
        for(const auto [at,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,7>{{
            {pool+0x30u,0x60000000u},{pool+0x24u,12u},{row+4u,player_row},
            {row+4u,image+1u},{row,retained_word(saved,row).value()^0x8000u},
            {pool+0x3cu,private_bitmap_requests+1u},{bits+24u,retained_word(saved,bits+24u).value()|0x80000000u}}}) {
          auto corrupt=saved;replace_word(corrupt,at,value);reject_private(std::move(corrupt));
        }
        for(const auto bit:{std::byte{2},std::byte{4}}) {
          auto corrupt=saved;replace_byte(corrupt,image+0xfu,bit);reject_private(std::move(corrupt));
        }
        for(const auto at:{image+0xeu,0x4f2d28u}) {
          auto corrupt=saved;std::array<std::byte,1> value;memory.read(at,value);
          corrupt.private_ranges.push_back({at,{value.begin(),value.end()}});
          std::sort(corrupt.private_ranges.begin(),corrupt.private_ranges.end(),[](const auto&a,const auto&b){return a.address<b.address;});
          reject_private(std::move(corrupt));
        }
      }
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
    if(serial.load())throw FormatError("Private native context changed authority input bytes");
   }catch(const std::exception&e){error=e.what();break;}
  }
  const auto state=context?context->state():SourceFirstPersonTemporalStateTu15{};
  report["cases"].push_back({{"representation",rep},{"original_bitmap_owner",!world.cache->is_mcc()},{"bitmap_prediction_owner",bitmap_prediction_owner},{"private_bitmap_requests",private_bitmap_requests},{"native_prediction_inputs",prediction_inputs},{"resident_fp_models",resident_models},{"resident_fp_sections",resident_sections},{"texture_usage_owner",texture_usage_owner},{"private_texture_usage_words",private_texture_usage_words},{"geometry_usage_owner",geometry_usage_owner},{"private_geometry_usage_words",private_geometry_usage_words},{"rejected_private_owner_attacks",rejected_private_owner_attacks},{"frames",frames},{"error",error},{"actor_shadow_bytes",actor_shadow_bytes},{"portable_restore",restored},{"wrong_root_restore_rejected",bad_restore_rejected},{"declared_actions",actions},{"duplicate_tick_rejected",duplicate_rejected},{"stale_player_rejected",identity_rejected},{"source_tick",state.source_tick},{"saved_locals",{state.hands[0].saved_locals.size(),state.hands[1].saved_locals.size()}},{"flags",{Bytes(state.hands[0].control).u32(0),Bytes(state.hands[1].control).u32(0)}},{"class",{Bytes(state.hands[0].control).u32(0x7c),Bytes(state.hands[1].control).u32(0x7c)}}});
 }
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
 for(const auto &row:report["cases"])if(row["frames"]!=40 || row["error"]!="" || row["rejected_private_owner_attacks"]!=(row["original_bitmap_owner"].get<bool>()?44u:33u) || (row["original_bitmap_owner"].get<bool>() && row["private_bitmap_requests"]==0) || row["private_texture_usage_words"]==0 || row["private_geometry_usage_words"]==0 || !row["portable_restore"].get<bool>() || !row["wrong_root_restore_rejected"].get<bool>() || !row["duplicate_tick_rejected"].get<bool>() || !row["stale_player_rejected"].get<bool>())return 1;
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
