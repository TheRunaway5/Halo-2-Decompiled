#include "halo2/pstencil/material.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw FormatError(message);
}
} // namespace
int main(int argc, char **argv) try {
  require(argc == 2, "Supply current opening campaign cache");
  Cache cache(argv[1]);
  unsigned cases = 0;
  for (const auto &tag : cache.tags())
    if (tag.group == "shad" &&
        (tag.name == "objects\\characters\\masterchief\\shaders\\masterchief" ||
         tag.name == "objects\\characters\\dervish\\shaders\\dervish_armor" ||
         tag.name == "objects\\characters\\elite\\shaders\\arms")) {
      auto authored = cache.load_material(tag.datum);
      for (unsigned lod = 0; lod < authored.lods.size(); ++lod) {
        authored.selected_shader_lod = lod;
        for (unsigned n = 0; n < 3; ++n) {
          EffectLightUniformInput input;
          input.position = {1, 2, 3};
          input.forward = {1, 0, 0};
          input.up = {0, 0, 1};
          input.left = {0, 1, 0};
          input.radius = 4;
          input.diffuse = {.2f, .4f, .8f};
          input.specular = {.7f, .3f, .1f};
          input.brightness = float(1u << n);
          const auto uniforms = source_effect_light_uniforms(input);
          for (unsigned pass = 0; pass < authored.passes.size(); ++pass) {
            const auto &source = authored.passes[pass];
            if (!source.vertex_shader.ends_with(
                    "\\stencil_light_specular_tangentspace_sphere"))
              continue;
            const auto full =
                pstencil::bind_light_pass(cache, authored, pass, uniforms);
            const auto completed =
                pstencil::complete_light_pass(cache, authored, pass, uniforms);
            require(full && completed,
                    "Current sphere light pass failed to bind");
            const auto &a = *full;
            const auto &b = *completed;
            require(a.forward_program.words == b.forward_program.words &&
                        a.forward_program.fragment_offsets ==
                            b.forward_program.fragment_offsets,
                    "Image-free program differs");
            require(!std::memcmp(a.program_constants.data(),
                                 b.program_constants.data(),
                                 sizeof(a.program_constants)),
                    "Image-free material constants differ");
            require(a.forward_blend == b.forward_blend &&
                        a.forward_raster == b.forward_raster &&
                        a.texture_states == b.texture_states &&
                        a.vertex_program == b.vertex_program,
                    "Image-free static state differs");
            const auto ap = pack_vertex_program_bank(*a.vertex_bank),
                       bp = pack_vertex_program_bank(*b.vertex_bank);
            require(!std::memcmp(&ap, &bp, sizeof(ap)),
                    "Image-free completed extended vertex bank differs");
            for (const auto *image :
                 {&b.diffuse, &b.detail, &b.bump, &b.auxiliary})
              require(image->pixels.empty() && image->mips.empty(),
                      "Per-draw completion retained image payloads");
            for (const auto &images : b.animated_textures)
              require(images.empty(),
                      "Per-draw completion loaded bitmap frames");
            ++cases;
          }
        }
      }
    }
  require(cases >= 27,
          "Current Chief/Arbiter/Elite completion coverage absent");
  std::cout << cases
            << " exact full/image-free current armor light completions; no "
               "per-draw bitmap payloads\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
