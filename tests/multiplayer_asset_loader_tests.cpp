#include "halo2/multiplayer_world.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/hud.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include <algorithm>
#include <bit>
#include <atomic>
#include <cfenv>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
template<class F>void rejects(F action){bool failed{};try{action();}catch(const FormatError&){failed=true;}check(failed,"Invalid prepared map resource request was admitted");}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
}
int main(int argc,char**argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"lockout.map"))return 77;
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Asset worker test requires owned scratch");
  const auto maps=std::filesystem::canonical(argv[1]);
  MultiplayerOptions options;options.map=maps/"lockout.map";options.shared_maps=maps;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"no-source-world";
  std::atomic<unsigned> source_calls{};options.source_world_observer=[&](const auto&){++source_calls;};
  options.player_birth_owner=[&](auto&,auto){++source_calls;};
  MultiplayerAssetLoader loader;
  auto missing=options;missing.map=std::filesystem::path(scratch)/"missing.map";
  rejects([&]{loader.request(missing).get();});
  for(unsigned i=0;i<32;++i)(void)loader.request(missing); // Drop cancelled UI futures.
  auto invalid=options;invalid.host_port=12345;rejects([&]{loader.request(invalid);});
  std::fenv_t prior;check(std::fegetenv(&prior)==0,"Cannot capture test floating point context");
  check(std::fesetround(FE_DOWNWARD)==0,"Cannot set test rounding mode");
  auto future=loader.request(options);
  check(std::fesetround(FE_UPWARD)==0,"Cannot change caller rounding mode");
  const auto assets=future.get();
  check(source_calls==0 && !std::filesystem::exists(options.source_object_savegame_directory),"Preloading constructed or advanced a source world");
  check(assets->matches(options) && assets->fonts,"Prepared map lost its selected resource owner");
  const auto cache=assets->cache_for(options);
  rejects([&]{SourceRuntimeResourcePreparation duplicate(cache);});
  auto wrong=options;wrong.bsp=1;rejects([&]{assets->cache_for(wrong);});
  wrong=options;wrong.map=options.shared_maps/"zanzibar.map";rejects([&]{assets->cache_for(wrong);});
  wrong=options;wrong.shared_maps=options.shared_maps/"other";rejects([&]{assets->cache_for(wrong);});
  wrong=options;wrong.campaign_difficulty=0;rejects([&]{assets->cache_for(wrong);});
  check(std::fesetround(FE_DOWNWARD)==0,"Cannot restore captured rounding mode");
  Cache synchronous(options.map,options.shared_maps);
  check(!take_prepared_mcc_world_metadata(*cache,0x80061000u,0x10000000u),"Foreign metadata reservation consumed the prepared plan");
  const auto metadata=take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);
  check(bool(metadata) && !take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u),
      "Prepared metadata was missing or consumed twice");
  std::optional<MccSoundRuntimePlan> expected_sound;
  const auto delivery=MccRuntimeCacheDelivery::plan(synchronous,0x90000000u,0x60000000u,
      [&](MccSoundRuntimePlan&& value){expected_sound.emplace(std::move(value));});
  const auto prepared_image=metadata->delivery.tag_image(),expected_image=delivery.tag_image();
  check(prepared_image.regions.size()==expected_image.regions.size(),"Prepared metadata region count changed");
  for(unsigned i=0;i<prepared_image.regions.size();++i)
    check(prepared_image.regions[i].address==expected_image.regions[i].address &&
        prepared_image.regions[i].bytes==expected_image.regions[i].bytes,"Prepared canonical metadata changed a byte or logical address");
  check(expected_sound && metadata->sound.address==expected_sound->address && metadata->sound.bytes==expected_sound->bytes,
      "Prepared sound projection changed its exact native bytes");
  const auto timing=build_mcc_sound_timing_reference(synchronous,*expected_sound,
      [&](auto address,auto bytes){return delivery.runtime_address(address,bytes);});
  check(metadata->timing.bindings().size()==timing.bindings().size() && metadata->timing.gaps().size()==timing.gaps().size(),
      "Prepared sound timing changed admitted binding/gap coverage");
  for(unsigned i=0;i<timing.bindings().size();++i) {
    const auto& a=metadata->timing.bindings()[i];const auto& b=timing.bindings()[i];
    check(a.sound==b.sound && a.gestalt==b.gestalt && a.source_root==b.source_root && a.native_root==b.native_root &&
        a.source_range==b.source_range && a.native_range==b.native_range && a.source_permutation==b.source_permutation &&
        a.native_permutation==b.native_permutation && a.relative_pitch==b.relative_pitch && a.relative_permutation==b.relative_permutation &&
        a.path==b.path && a.range_name==b.range_name && a.permutation_name==b.permutation_name &&
        a.original==b.original && a.encoded_payload==b.encoded_payload && a.named_key==b.named_key,
        "Prepared sound timing changed an authenticated binding");
  }
  SourceRuntimeResources::Identity prepared_identity;
  constexpr std::array offsets{0u,0x40000000u,0x80000000u,0xc0000000u};
  std::array<std::vector<std::byte>,4> prepared_reads;
  {
    SourceRuntimeResources prepared_resources(*cache);prepared_identity=prepared_resources.identity();
    for(unsigned i=0;i<offsets.size();++i)prepared_reads[i]=prepared_resources.load(offsets[i],512);
  }
  {
    SourceRuntimeResources expected_resources(synchronous);
    check(prepared_identity==expected_resources.identity(),"Worker changed a resident/shared SHA256 identity");
    for(unsigned i=0;i<offsets.size();++i)
      check(prepared_reads[i]==expected_resources.load(offsets[i],512),"Prepared resource snapshot changed its encoded I/O bytes");
  }
  check(std::ranges::equal(cache->source_runtime_cache_bytes(),synchronous.source_runtime_cache_bytes()),"Worker changed normalized gameplay cache bytes");
  auto expected=synchronous.load_content(options.bsp,true,source_game_variant(options.variant));
  const auto mesh=assets->take_bsp_mesh(options),expected_mesh=synchronous.load_bsp(options.bsp);
  check(mesh.vertices.size()==expected_mesh.vertices.size() && mesh.materials.size()==expected_mesh.materials.size(),
      "Worker changed BSP geometry/material coverage");
  for(std::size_t i=0;i<mesh.vertices.size();++i) {
    const auto& a=mesh.vertices[i];const auto& b=expected_mesh.vertices[i];
    check(same(a.position,b.position)&&same(a.color,b.color)&&same(a.normal,b.normal)&&same(a.tangent,b.tangent)&&
        same(a.binormal,b.binormal)&&same(a.incident,b.incident)&&same(a.uv.x,b.uv.x)&&same(a.uv.y,b.uv.y)&&
        same(a.lightmap_uv.x,b.lightmap_uv.x)&&same(a.lightmap_uv.y,b.lightmap_uv.y)&&a.material==b.material&&
        a.lightmap==b.lightmap&&a.bsp_cluster==b.bsp_cluster&&same(a.prt_cluster,b.prt_cluster),
        "Worker changed BSP vertex bits or material/cluster order");
  }
  for(std::size_t i=0;i<mesh.materials.size();++i)
    check(mesh.materials[i].name==expected_mesh.materials[i].name &&
        mesh.materials[i].diffuse.pixels==expected_mesh.materials[i].diffuse.pixels &&
        mesh.materials[i].detail.pixels==expected_mesh.materials[i].detail.pixels,
        "Worker changed ordered BSP material names or texture bytes");
  for(const auto representation:source_session_player_representations(expected,2)) {
    const auto biped=multiplayer_biped(expected,representation).datum;
    std::vector<std::uint32_t> weapons{null_datum};
    for(const auto& binding:handheld_weapon_hud_bindings(synchronous,representation))weapons.push_back(binding.definition);
    for(const auto weapon:weapons) {
      const auto reference=synchronous.load_hud(biped,weapon);
      check(same(assets->hud_sensor_range(biped,weapon),reference.sensor.range),"Prepared HUD sensor range changed");
      const auto hud=assets->take_hud(biped,weapon);
      check(hud.widgets.size()==reference.widgets.size() && hud.images.size()==reference.images.size() &&
          hud.shaders.size()==reference.shaders.size(),"Prepared HUD resource coverage changed");
      for(unsigned i=0;i<hud.widgets.size();++i) {
        const auto& a=hud.widgets[i];const auto& b=reference.widgets[i];
        check(a.name==b.name && a.inputs==b.inputs && a.yes==b.yes && a.no==b.no && a.bitmap==b.bitmap &&
            a.shader==b.shader && a.fonts==b.fonts && a.sequence==b.sequence,"Prepared HUD widget order or bindings changed");
      }
      for(const auto& [id,image]:hud.images) {
        const auto& other=reference.images.at(id);check(image.images.size()==other.images.size(),"Prepared HUD bitmap coverage changed");
        for(unsigned i=0;i<image.images.size();++i) {
          check(image.images[i].pixels==other.images[i].pixels,"Prepared HUD decoded image bytes changed");
          auto copy=image.images[i];
          if(!copy.pixels.empty()){copy.pixels[0]^=std::byte{1};check(image.images[i].pixels==other.images[i].pixels,
              "Mutable HUD image copy corrupted shared immutable pixels");}
        }
      }
    }
  }
  std::optional<ScriptProgram> script;auto content=assets->take_content(options,script);
  rejects([&]{assets->take_content(options,script);});
  check(content.collision.size()==expected.collision.size() && content.weapons.size()==expected.weapons.size() &&
      content.objects.size()==expected.objects.size() && content.navigation.size()==expected.navigation.size(),"Worker changed content resource coverage");
  for(std::size_t i=0;i<content.collision.size();++i) {
    const auto& a=content.collision[i];const auto& b=expected.collision[i];
    check(same(a.a,b.a)&&same(a.b,b.b)&&same(a.c,b.c)&&same(a.source_normal,b.source_normal)&&a.material==b.material&&a.flags==b.flags,
        "Worker changed authored collision coordinates or flags");
  }
  check(std::fesetenv(&prior)==0,"Cannot restore caller floating point context");
  std::cout<<"PASS bounded cancellable asset worker: exact gameplay cache bytes, collision floats, captured rounding, no source world, failed/mismatched/consumed requests rejected\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
