#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <utility>

static void compare_capture(const halo2::Material &material, const std::array<char,160> &fixture) {
  if (std::memcmp(fixture.data(), "H2LIGHT3", 8)) throw halo2::FormatError("invalid lighting fixture marker");
  std::size_t offset=8;
  for (unsigned i=0;i<4;++i) {
    std::uint32_t index{};
    std::memcpy(&index,fixture.data()+offset,4); offset+=4;
    if (index<18 || index>=34 ||
        std::memcmp(material.program_constants[1][index-18].data(),fixture.data()+offset,16))
      throw halo2::FormatError("menu lighting vertex constants differ from original GPU capture: " + material.name);
    offset+=16;
  }
  for (unsigned i=0;i<18;++i) {
    std::uint32_t expected{};
    std::memcpy(&expected,fixture.data()+offset,4); offset+=4;
    // Xemu exposes interleaved C0/C1 banks, then final C0/C1.
    const auto word=i<16?10+i/2+(i%2)*8:43+i-16;
    if (material.forward_program.words[word]!=expected)
      throw halo2::FormatError("menu lighting pixel constants differ from original GPU capture: " + material.name);
  }
}

static void compare_carrier_emissive(const halo2::Cache &cache,
                                    const halo2::Material &source) {
  // Literal mainmenu.map E1E80073 -> opaque\\illum -> layer7/implementation0.
  // Its packed state range0x1000 contains these eight original writes.
  // This regression checks the authored binder; frontend submission is
  // separately checked through the captured carrier pipeline/depth history.
  constexpr std::array<std::pair<unsigned,std::uint32_t>,8> original_states{{
      {67,0x10101},{59,1},{62,1},{63,1},{74,0x8006},{64,0},{60,1},{58,0x205}}};
  if (source.source_shader_datum != 0xe1e80073u || source.lods.size() != 1 ||
      source.selected_shader_lod != 0 || source.lods[0].available_layers != (1u<<7) ||
      source.lods[0].layers.count() <= 7)
    throw halo2::FormatError("original carrier emissive shader/layer identity changed");
  const auto range = source.layers.at(source.lods[0].layers.first()+7);
  const auto selected = std::find_if(source.passes.begin(),source.passes.end(),
      [&](const auto &pass) {
        return pass.template_pass >= range.first() &&
            pass.template_pass-range.first() < range.count() && pass.implementation == 0;
      });
  if (selected == source.passes.end() ||
      selected->name != "shaders\\shader_passes\\self_illumination\\default_self_illumination_single_pass" ||
      !selected->vertex_shader.ends_with("\\stencil_emissive") ||
      halo2::decode_vertex_program(selected->vertex_code).count != 12 ||
      selected->states.size() != original_states.size())
    throw halo2::FormatError("original carrier emissive pass identity changed");
  for (unsigned i=0;i<original_states.size();++i)
    if (selected->states[i].index != original_states[i].first ||
        selected->states[i].value != original_states[i].second)
      throw halo2::FormatError("carrier emissive states differ from literal original cache");
  auto emitted = source;
  if (halo2::prepare_authored_lighting_material(cache,emitted))
    throw halo2::FormatError("emissive-only carrier material incorrectly admitted a lighting layer");
  if (!halo2::prepare_default_object_material(cache,emitted))
    throw halo2::FormatError("original carrier emissive fallback was rejected");
  if (!emitted.world_pass || !emitted.forward_pass || emitted.forward_vertex != 2 ||
      emitted.forward_pass_index != std::size_t(selected-source.passes.begin()) ||
      emitted.forward_blend != 3 || emitted.forward_raster.color_write != 0x10101 ||
      emitted.forward_raster.depth_write != 0 || emitted.forward_raster.alpha_test != 1 ||
      emitted.forward_raster.alpha_function != 0x205 ||
      emitted.forward_raster.alpha_reference != 0)
    throw halo2::FormatError("carrier emissive fallback lost original additive/alpha/no-depth-write state");
  if (emitted.vertex_program != selected->vertex_code ||
      emitted.program_constants != selected->constants ||
      emitted.forward_program.words != selected->program.words ||
      emitted.forward_program.fragment_offsets != selected->program.fragment_offsets ||
      emitted.vertex_declaration != selected->vertex_declaration ||
      !emitted.world_overlays_before.empty() || !emitted.world_overlays_after.empty())
    throw halo2::FormatError("carrier emissive fallback changed the original pass programs or duplicated its layer");
}

int main(int argc, char **argv) {
  try {
    if (argc != 3) throw halo2::FormatError("menu lighting test requires original cache and capture fixture");
    halo2::Cache cache(argv[1]);
    auto mesh = cache.load_bsp(0);
    unsigned selected = 0;
    for (auto &material : mesh.materials) {
      if (!halo2::prepare_authored_lighting_material(cache, material)) continue;
      ++selected;
      const auto &lighting = material.environment_pass ? *material.environment_pass : material;
      const auto &pass = lighting.passes.at(lighting.forward_pass_index);
      if (pass.implementation != 3 || lighting.static_lightmap_pass ||
          lighting.baked_lightmap_texture || !pass.vertex_externs.empty() ||
          !pass.pixel_externs.empty() || !lighting.forward_pass)
        throw halo2::FormatError("menu still uses a baked or live-lighting variant: " + material.name + " implementation " + std::to_string(pass.implementation));
      halo2::decode_vertex_program(lighting.vertex_program);
    }
    if (selected != 179) throw halo2::FormatError("original menu lighting material coverage changed");
    std::ifstream file(argv[2], std::ios::binary);
    std::array<char,160> fixture{};
    if (!file.read(fixture.data(), fixture.size()) ||
        std::memcmp(fixture.data(), "H2LIGHT3", 8))
      throw halo2::FormatError("invalid original lighting capture fixture");
    const auto &material = mesh.materials.at(136);
    if (material.name != "scenarios\\shaders\\ui\\mainmenu\\x03_metal_grey")
      throw halo2::FormatError("original capture material identity changed");
    compare_capture(material,fixture);
    if (!file.read(fixture.data(),fixture.size())) throw halo2::FormatError("carrier capture fixture missing");
    const auto carrier=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag) {
      return tag.group=="scen" && tag.name=="scenarios\\objects\\covenant\\military\\capital_ship\\capital_ship";
    });
    if (carrier==cache.tags().end()) throw halo2::FormatError("original carrier missing");
    auto model=cache.load_model(carrier->datum,true);
    unsigned bodies=0,emissives=0;
    for (auto &body:model.mesh.materials) {
      if (body.name.ends_with("\\capital_ship_c")) {
        compare_carrier_emissive(cache,body); ++emissives;
      }
      if (halo2::prepare_authored_lighting_material(cache,body)) {
        compare_capture(body,fixture); ++bodies;
        halo2::decode_vertex_program(body.vertex_program);
      }
    }
    if (bodies!=2) throw halo2::FormatError("original carrier lighting coverage changed");
    if (emissives!=1) throw halo2::FormatError("original carrier emissive coverage changed");
    std::cout << "179 BSP and two carrier lighting materials; bridge and carrier active constants match original draws 1335/1448 byte for byte; carrier glow retains original layer7 additive/alpha/no-depth-write state\n";
    return 0;
  } catch (const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
