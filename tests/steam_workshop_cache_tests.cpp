#include "halo2/steam_workshop.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/mcc_physics.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_render_policy.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace halo2;
void require(bool value, const char *why) { if (!value) throw FormatError(why); }
int main(int argc, char **argv) { try {
  if (argc != 2 || !std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / "mainmenu.map")) return 77;
  // When real local inputs are explicitly configured, exercise the production
  // process bridge and compatibility check against the downloaded E3 mission.
  const auto installed = scan_steam_workshop();
  bool found{};
  for (const auto &mod : installed.mods) if (mod.id == "3360515088" && mod.error.empty()) {
    found = true;
    const auto mission = std::find_if(mod.missions.begin(), mod.missions.end(),
        [](const auto &row) { return row.scenario == "earthcity_e3"; });
    require(mission != mod.missions.end(), "Installed E3 item lost its main mission");
    const auto *scratch = std::getenv("HALO2_TEST_SCRATCH");
    require(scratch, "Workshop service test requires owned scratch");
    const auto base = std::filesystem::canonical(argv[1]).parent_path().parent_path();
    const auto prepared = prepare_steam_workshop(*mission, base, std::filesystem::path(scratch) / "prepared");
    require(Cache(prepared.map, prepared.shared_maps, {}, prepared.prepared_root).preserves_mcc_authoring(),
            "Workshop owner admitted stock gameplay authoring corrections");
    {
      const Cache authored(prepared.map, prepared.shared_maps, {}, prepared.prepared_root);
      const auto globals=authored.query_tag({.group=tag_groups::globals});
      require(globals!=nullptr,"Prepared mod has no rasterizer globals");
      const Bytes global(authored.tag_payload(globals->handle()));
      const Bytes raster(authored.read_tag_bytes(global.u32(0x10c),0x108));
      const auto shaders=authored.read_tag_bytes(raster.u32(0x5c),raster.u32(0x58)*8u);
      require(shaders.size()>3*8+7,"Prepared mod lost its global decal program slot");
      for(std::size_t at=0;at<shaders.size();at+=8) {
        const auto datum=Bytes(shaders).u32(at+4);if(datum==0xffffffffu)continue;
        const auto vertex=authored.query_tag({.handle=TagHandle{datum},.group=tag_groups::vertex_shader,.require_address=true});
        require(vertex && authored.is_preserved_native_shader(*vertex),
            "Prepared mod omits a required global native vertex program");
      }
      require(authored.load_vertex_shader_code(Bytes(shaders).u32(3*8+4),0).size()==212,
          "Prepared mod lost the renderer's original global decal program");
      const auto decoded = read_mcc_cache(mission->item / "maps/earthcity_e3.map");
      const Bytes raw(decoded);
      const auto index = raw.u32(16);
      const auto magic = std::int64_t(raw.u32(720)) - index;
      const auto header_magic = std::int64_t(raw.u32(index)) - index - 32;
      const auto table = std::size_t(std::int64_t(raw.u32(index + 8)) - header_magic);
      unsigned unchanged{};
      for (const auto &tag : authored.tags())
        if (tag.group == "weap" || tag.group == "proj" || tag.group == "bipd" || tag.group == "vehi") {
          const auto row = table + std::size_t(tag.datum & 65535u) * 16;
          const auto original = raw.slice(std::size_t(std::int64_t(raw.u32(row + 8)) - magic), raw.u32(row + 12));
          auto expected = std::vector<std::byte>(original.begin(), original.end());
          if (tag.group == "bipd" || tag.group == "vehi")
            expected = normalize_mcc_unit_physics(original, raw.u32(row + 8),
                tag.group == "bipd" ? MccUnitPhysicsKind::Biped : MccUnitPhysicsKind::Vehicle);
          if (!std::ranges::equal(expected, authored.read_tag_bytes(tag.address, tag.size)))
            throw FormatError("Workshop changed authored " + tag.group + ": " + tag.name);
          ++unchanged;
        }
      require(unchanged > 50, "E3 gameplay preservation check did not cover its authored object definitions");
      std::cout << "E3 authored gameplay definitions unchanged=" << unchanged << '\n';
      unsigned projected_materials{};
      for(const auto &tag:authored.tags())if(tag.group=="shad" &&
          (tag.name=="objects\\weapons\\rifle\\smg\\shaders\\smg_rubber_grip" ||
           tag.name=="objects\\weapons\\grenade\\plasma_grenade\\shaders\\pg_decal")) {
        ++projected_materials;
        const bool transparent=tag.name.ends_with("\\pg_decal");
        const auto material=authored.load_material(tag.datum,false,false);
        if(material.passes.empty())throw FormatError("E3 material has no converted native passes: "+tag.name);
        const auto row=table+std::size_t(tag.datum&65535u)*16;
        const auto source_root=std::size_t(std::int64_t(raw.u32(row+8))-magic);
        const auto source_definition=std::size_t(std::int64_t(raw.u32(source_root+36))-magic);
        const Bytes converted_root(authored.tag_payload(tag.handle()));
        const Bytes converted(authored.read_tag_bytes(converted_root.u32(36),124));
        if(transparent) {
          auto source=tag;source.address=raw.u32(row+8);source.size=raw.u32(row+12);
          const auto *native=authored.find_tag({raw.u32(source_root+4)});
          require(native!=nullptr,"Transparent material lost its native template owner");
          const std::string payload_name="448ed36012b316f4a9de06319c3ddbcfdb6afb028e03be2b9b1a68bf518d8491.bin";
          std::ifstream file(prepared.prepared_root/"native-shaders/maps/workshop/payloads"/payload_name,std::ios::binary);
          std::vector<std::byte> payload(290);file.read(reinterpret_cast<char*>(payload.data()),payload.size());
          require(bool(file),"Transparent native template payload is absent");
          const MccShaderRead read=[&](unsigned at,std::size_t size){return raw.slice(std::size_t(std::int64_t(at)-magic),size);};
          const MccShaderFind find=[&](unsigned id)->const Tag* {return id==source.datum?&source:authored.find_tag({id});};
          const MccShaderBitmapFrame owns=[&](unsigned id,unsigned frame){return frame<authored.bitmap_count(id);};
          require(bool(project_mcc_transparent_one_alpha_env(source,*native,payload_name,payload,read,find,owns)),
              "Paired transparent tuple was not admitted through its public contract");
          const auto pixels=raw.u32(source_definition+16);
          const auto words=read(pixels,16);auto changed=std::vector<std::byte>(words.begin(),words.end());changed[0]^=std::byte{1};
          const MccShaderRead corrupt=[&](unsigned at,std::size_t size)->std::span<const std::byte> {
            if(at==pixels && size==changed.size())return changed;return read(at,size);
          };
          require(!project_mcc_transparent_one_alpha_env(source,*native,payload_name,payload,corrupt,find,owns),
              "Changed transparent authoring bypassed the exact-tuple gate");
          payload[0]^=std::byte{1};
          require(!project_mcc_transparent_one_alpha_env(source,*native,payload_name,payload,read,find,owns),
              "Changed native transparent template bypassed its fingerprint");
        }
        for(auto [field,stride]:{std::pair{4u,12u}, {12u,4u}, {20u,16u},
                                {92u,4u}, {100u,12u}, {108u,4u}}) {
          const auto count=raw.u32(source_definition+field);
          require(converted.u32(field)==count,"E3 material projection changed an authored parameter count");
          const auto before=raw.slice(std::size_t(std::int64_t(raw.u32(source_definition+field+4))-magic),count*stride);
          const auto after=authored.read_tag_bytes(converted.u32(field+4),count*stride);
          auto expected=std::vector<std::byte>(before.begin(),before.end());
          // Paired same-template original/MCC material proves this lossless
          // cooked pixel-slot permutation; values and all other banks survive.
          if(transparent && field==12)for(unsigned n=0;n<4;++n)std::swap(expected[4+n],expected[8+n]);
          require(std::ranges::equal(expected,after),"E3 material projection changed authored inputs");
        }
      }
      require(projected_materials==2,"E3 shader fixtures are absent");
    }
    require(std::find(prepared.script_requirements.begin(), prepared.script_requirements.end(),
        "rasterizer_bloom_override_silver_bullet") == prepared.script_requirements.end(),
        "E3 bloom command lost its verified native script owner");
    require(!prepared.script_requirements.empty(),
        "E3 gameplay bypassed its remaining unimplemented script features");
    require(std::find(prepared.script_requirements.begin(), prepared.script_requirements.end(),
        "object_cinematic_collision") == prepared.script_requirements.end(),
        "E3 cinematic collision command lost its verified native owner");
    require(std::find(prepared.script_requirements.begin(), prepared.script_requirements.end(),
        "ai_force_active") == prepared.script_requirements.end(),
        "E3 AI activation command lost its native owner");
    const auto reused = prepare_steam_workshop(*mission, base, std::filesystem::path(scratch) / "prepared");
    require(prepared.map == reused.map && prepared.script_requirements == reused.script_requirements,
            "Second mod launch lost its prepared owner or compatibility result");
    std::cout << "E3 native script requirements:";
    for (const auto &name : prepared.script_requirements) std::cout << ' ' << name;
    std::cout << '\n';
    const auto playground=std::find_if(mod.missions.begin(),mod.missions.end(),[](const auto &entry){
      return entry.scenario=="earthcity_playground";
    });
    if(playground!=mod.missions.end()) {
      const auto checked=prepare_steam_workshop(*playground,base,std::filesystem::path(scratch)/"prepared");
      require(std::find(checked.script_requirements.begin(),checked.script_requirements.end(),
          "unit_set_enterable_by_player")==checked.script_requirements.end(),
          "Verified unit entry command remained blocked by stale preparation diagnostics");
      require(std::find(checked.rendering_requirements.begin(),checked.rendering_requirements.end(),
          "scenarios\\shaders\\human\\urban\\glass\\window_skylight")!=checked.rendering_requirements.end(),
          "Playground preflight missed its actual campaign-rendering material blocker");
      require(checked.gameplay_unavailable_reason().find("model materials")!=std::string::npos,
          "Rendering compatibility failure was mislabeled as missing script support");
      std::cout<<"Playground model material requirements="<<checked.rendering_requirements.size()<<'\n';
    }
  }
  if (!found) return 77;
  for(const auto& mod:installed.mods)if(mod.id=="3366014540" && mod.error.empty()) {
    const auto mission=std::find_if(mod.missions.begin(),mod.missions.end(),[](const auto& row){return row.scenario=="alphamoon";});
    require(mission!=mod.missions.end(),"Installed Alpha Moon lost its authored scenario");
    const auto base=std::filesystem::canonical(argv[1]).parent_path().parent_path();
    const auto prepared=prepare_steam_workshop(*mission,base,std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"alpha-sky");
    const Cache authored(prepared.map,prepared.shared_maps,{},prepared.prepared_root);
    for(const auto leaf:{"sky_dome","moon_clouds","ring_shard"}) {
      const auto name=std::string("digsite\\scenarios\\skies\\solo\\alphamoon\\shaders\\")+leaf;
      const auto* shader=authored.query_tag({.group=TagGroup{"shad"},.name=name,.require_address=true});
      require(shader && authored.is_preserved_native_shader(*shader),"Alpha Moon sky lost its native cooked-data owner");
      Mesh sky;sky.materials.push_back(authored.load_material(shader->datum));prepare_forward_materials(authored,sky);
      const auto& material=sky.materials.front();
      require(material.forward_pass && !material.vertex_program.empty() &&
          source_material_layer(material)==std::optional<unsigned>{15},"Alpha Moon sky has no authored native layer15 pass");
      if(std::string_view(leaf)=="moon_clouds") {
        require(material.animation.overlays.size()==10,"Alpha Moon lost its authored cloud animation roles");
        require(material.animation.overlays[0].curve.size()==52 && material.animation.overlays[0].curve[0]==std::byte{3},
            "Alpha Moon custom periodic cloud function was replaced");
      }
    }
    std::cout<<"Alpha Moon three authored native sky materials and custom cloud curve passed\n";
  }
  for(const auto& mod:installed.mods)if(mod.id=="3577874470" && mod.error.empty()) {
    const auto mission=std::find_if(mod.missions.begin(),mod.missions.end(),[](const auto& row){return row.scenario=="mombasa_battle";});
    require(mission!=mod.missions.end(),"Installed Forge item lost its scenario");
    const auto base=std::filesystem::canonical(argv[1]).parent_path().parent_path();
    const auto prepared=prepare_steam_workshop(*mission,base,std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"forge");
    const Cache authored(prepared.map,prepared.shared_maps,{},prepared.prepared_root);
    const auto* warthog=authored.query_tag({.group=tag_groups::model,.name="objects\\vehicles\\warthog\\warthog"});
    require(warthog!=nullptr,"Forge lost its authored Warthog model");
    const auto owned=authored.load_source_tag_metadata(warthog->datum);
    const Bytes model(owned.bytes_at(owned.source_address(),owned.size()));
    const Bytes variants(authored.read_tag_bytes(model.u32(0x54),model.u32(0x50)*0x38u));
    bool deduplicated=false;
    for(unsigned n=0;n<model.u32(0x50);++n)if(variants.u32(n*0x38+0x1c)) {
      const auto address=variants.u32(n*0x38+0x20),size=variants.u32(n*0x38+0x1c)*0x10u;
      if(address>=owned.source_address() && std::uint64_t(address)+size<=std::uint64_t(owned.source_address())+owned.size())continue;
      deduplicated=true;
      require(authored.read_tag_bytes(address,size).size()==size,"Deduplicated child bank lost its resident owner");
      bool rejected=false;try{(void)owned.bytes_at(address,size);}catch(const std::out_of_range&){rejected=true;}
      require(rejected,"Literal tag payload silently admitted another tag's allocation");
    }
    require(deduplicated,"Forge fixture no longer exercises deduplicated model children");
    const auto content=authored.load_content(0,false);
    require(std::ranges::any_of(content.objects,[](const auto& object){return object.model_attachment.has_value();}),
        "Forge did not construct its authored vehicle attachments");
    std::cout<<"Forge deduplicated child bank and authored vehicle attachments passed\n";
  }
  std::cout << "Workshop private ownership, reuse and authored gameplay checks passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
