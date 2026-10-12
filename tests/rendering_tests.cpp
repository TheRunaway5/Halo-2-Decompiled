#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <iostream>
#include <source_location>

namespace {
void expect(bool value,
            std::source_location at = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("rendering assertion failed at line " +
                             std::to_string(at.line()));
}
template <class F> void rejects(F f) {
  bool error = false;
  try {
    f();
  } catch (const halo2::FormatError &) {
    error = true;
  }
  expect(error);
}
} // namespace
int main(int argc, char **argv) {
  try {
    using namespace halo2;
    // Fragment selection indexes permutations, while their stored enum
    // values are metadata. A final combiner consumes no regular stage.
    PixelProgram definition;
    definition.words[54] = 0x8421;
    definition.words[55] = 17;
    definition.words[56] = 29;
    std::array<ShaderFragment, 2> fragments{{{-1, {512}}, {2, {1025}}}};
    std::array<ShaderPermutation, 3> permutations{
        {{19, 0, {512}}, {23, 1, {1025}}, {27, 1, {1027}}}};
    std::array<ShaderCombiner, 5> combiners{};
    for (unsigned i = 0; i < 5; ++i)
      for (unsigned w = 0; w < 8; ++w)
        combiners[i].words[w] = i * 100 + w;
    std::array<std::int32_t, 8> switches{};
    auto program = assemble_pixel_program(definition, fragments, permutations,
                                          combiners, switches);
    expect(program.words[34] == 0 && program.words[35] == 100 &&
           program.words[53] == 0x11102);
    expect(program.words[8] == 200 && program.words[9] == 202 &&
           program.words[43] == 204);
    expect(program.words[54] == 0x8421 && program.fragment_offsets[1] == 1);
    switches[2] = 1;
    program = assemble_pixel_program(definition, fragments, permutations,
                                     combiners, switches);
    expect(program.words[8] == 400 && program.words[35] == 300);
    switches[2] = 2;
    rejects([&] {
      assemble_pixel_program(definition, fragments, permutations, combiners,
                             switches);
    });
    switches[2] = 0;
    auto invalid = fragments;
    invalid[0].permutations = {0xffff};
    rejects([&] {
      assemble_pixel_program(definition, invalid, permutations, combiners,
                             switches);
    });
    expect(assemble_pixel_program(definition, {}, permutations, combiners,
                                  switches)
               .words == definition.words);
    ShaderConstants constants;
    for (auto &bank : constants)
      for (auto &value : bank)
        value = {90, 91, 92, 93};
    const std::array<ShaderVector, 1> source{{{2, 3, 4, 5}}};
    const std::array<ShaderRemap, 2> mapping{
        {{std::uint16_t(2 | (13 << 5)), 0, 0},
         {std::uint16_t(7 | 16 | (9 << 5)), 0, 0}}};
    remap_vertex_constants(constants, source, mapping);
    expect(constants[0][2] == source[0]);
    expect(constants[1][7] == ShaderVector({2, 3, 92, 93}));
    expect(constants[0][3] == ShaderVector({90, 91, 92, 93}));
    program = assemble_pixel_program(definition, fragments, permutations,
                                     combiners, switches);
    const std::array<std::uint32_t, 1> color{{0xabcdef12}};
    const std::array<ShaderRemap, 1> pixel_mapping{
        {{std::uint16_t(1 | (1 << 6)), 4, 0}}};
    remap_pixel_constants(program, color, pixel_mapping, fragments, switches);
    expect(
        program.words[12] ==
        0x00cdef12); // Fragment 1 starts at stage 1, target stage 2, RGB only.
    std::vector<std::byte> palette(1024);
    for (unsigned i = 0; i < 256; ++i) {
      palette[i * 4] = std::byte(i);
      palette[i * 4 + 1] = std::byte(255 - i);
      palette[i * 4 + 2] = std::byte(i / 2);
      palette[i * 4 + 3] = std::byte(127);
    }
    const std::vector<std::byte> indices{
        std::byte{0}, std::byte{1}, std::byte{4}, std::byte{5},
        std::byte{2}, std::byte{3}, std::byte{6}, std::byte{7}};
    const auto texture =
        prepare_palettized_texture(Bytes(indices), 4, 2, true, palette);
    for (unsigned i = 0; i < 8; ++i)
      expect(texture.pixels[i * 4] == std::byte(i / 2) &&
             texture.pixels[i * 4 + 2] == std::byte(i) &&
             texture.pixels[i * 4 + 3] == std::byte(127));
    rejects([&] {
      prepare_palettized_texture(Bytes(indices), 4, 2, true,
                                 std::span(palette).first(1023));
    });
    rejects([&] {
      prepare_palettized_texture(Bytes(indices), 8, 2, true, palette);
    });
    if (argc > 1) {
      Cache cache(std::filesystem::path(argv[1]) / "lockout.map");
      const auto mesh = cache.load_bsp(0);
      expect(mesh.lightmaps.size() > 50);
      expect(std::all_of(mesh.vertices.begin(), mesh.vertices.end(),
                         [&](const Vertex &v) {
                           return v.lightmap <= mesh.lightmaps.size();
                         }));
      const auto material = std::find_if(
          mesh.materials.begin(), mesh.materials.end(),
          [](const Material &m) { return m.name.ends_with("trim_vents"); });
      expect(material != mesh.materials.end() &&
             material->world_pass && material->forward_pass &&
             material->static_lightmap_pass && material->baked_lightmap_texture &&
             !material->accumulation_texture && !material->environment_pass);
      const auto &source_pass=material->passes.at(material->forward_pass_index);
      const auto &lod=material->lods.front();
      const auto source_layer=material->layers.at(lod.layers.first()+3);
      expect(source_pass.template_pass>=source_layer.first() &&
             source_pass.template_pass-source_layer.first()<source_layer.count() &&
             source_pass.implementation==0 &&
             source_pass.vertex_shader.ends_with("\\lightmap_perpixel_tex_detail") &&
             material->forward_program.words==source_pass.program.words &&
             material->program_constants==source_pass.constants);
      expect(!material->detail.pixels.empty() &&
             !material->bump.pixels.empty() && material->bump.format == 11);
      expect(material->forward_transforms[0] == ShaderVector({1, 1, 0, 0}) &&
             material->forward_transforms[1] == ShaderVector({16, 16, 0, 0}) &&
             material->forward_transforms[0]==source_pass.constants[1][0] &&
             material->forward_transforms[1]==source_pass.constants[1][1]);
      for(unsigned stage=0;stage<4;++stage)
        expect(material->texture_states[stage]==source_pass.texture_states[stage]);
      expect(material->passes.size() > 20 &&
             !material->passes[0].vertex_code.empty());
      expect(std::any_of(
          mesh.vertices.begin(), mesh.vertices.end(), [](const Vertex &v) {
            return v.lightmap && v.incident.z != 0 && v.normal.z != 0;
          }));
      std::cout << "retail lightmap textures=" << mesh.lightmaps.size()
                << ", preserved pass implementations="
                << material->passes.size() << '\n';
    }
    std::cout << "combiner assembly, remapping, P8 and rendering content tests "
                 "passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
