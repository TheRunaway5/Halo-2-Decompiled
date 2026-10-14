#include "halo2/appearance.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/sky.hpp"
#include "halo2/texture.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/sampler.hpp"
#include <iostream>

namespace {
using namespace halo2;
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void retail(const std::filesystem::path &directory) {
  Cache cache(directory / "01b_spacestation.map");
  const auto sky = cache.load_visible_sky(0);
  check(sky && sky->name == "scenarios\\skies\\solo\\spacestation\\space" &&
            sky->scale == 400 && sky->movement == 1 && sky->yaw == 0,
        "Cairo BSP must use its original visible space sky and transform");
  auto sky_model = cache.load_model(sky->model);
  prepare_forward_materials(cache, sky_model.mesh);
  check(!sky_model.mesh.vertices.empty() &&
            std::all_of(
                sky_model.mesh.vertices.begin(), sky_model.mesh.vertices.end(),
                [&](const auto &v) {
                  return v.material < sky_model.mesh.materials.size() &&
                         sky_model.mesh.materials[v.material].forward_pass;
                }),
        "Cairo sky must retain its authored transparency programs");
  const Vec3 eye{10, 20, 30};
  const auto sky_pose = pose_sky(sky_model, *sky, eye);
  check(length(sky_pose.front().position -
               (pose_model(sky_model).front().position * 400 + eye)) < 1e-4f,
        "Cairo sky camera parallax/scale differs from 3EBD0");
  const auto panel_tag =
      std::find_if(cache.tags().begin(), cache.tags().end(), [](const auto &t) {
        return t.group == "shad" &&
               t.name ==
                   "scenarios\\shaders\\solo\\spacestation\\panel_mech_tile";
      });
  check(panel_tag != cache.tags().end(),
        "Cairo panel lighting fixture missing");
  const auto panel = cache.load_material(panel_tag->datum);
  const auto &panel_pass = panel.passes.front();
  check(panel_pass.texture_states.size() == 4 &&
            panel_pass.texture_states[2].min_filter == 3 &&
            panel_pass.texture_states[2].mag_filter == 3 &&
            panel_pass.texture_states[2].anisotropy == 4 &&
            panel_pass.texture_states[3].address_u == 3 &&
            panel.texture_states[2] == panel_pass.texture_states[2],
        "Cairo bump/lightmap textures lost their original sampling states");
  VkPhysicalDeviceLimits limits{};
  limits.maxSamplerLodBias=16;limits.maxSamplerAnisotropy=16;
  const auto bump_sampler=native_sampler(panel.texture_states[2],limits,true);
  const auto lightmap_sampler=native_sampler(panel.texture_states[3],limits,true);
  check(bump_sampler.info.anisotropyEnable && bump_sampler.info.maxAnisotropy==8 &&
        lightmap_sampler.info.addressModeU==VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        "native samplers must preserve Cairo bump/lightmap sampling");
  check((native_sampler(panel.texture_states[3],limits,true,true).info.flags &
            VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT)!=0 && lightmap_sampler.info.flags==0,
        "Xbox cube face filtering must be explicit and require an enabled device feature");
  auto no_mips=panel.texture_states[3];no_mips.mip_filter=0;no_mips.min_filter=1;
  check(native_sampler(no_mips,limits,true).info.maxLod==.25f,
        "mip-disabled textures must preserve the minification filter");
  check(sampler_address(5)==VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
        "retail mirror-once textures must retain reflection before clamping");
  // Retail 1BBF0/34060 take these selectors and brightness from this tag.
  std::array<std::int32_t, 8> panel_switches{};
  panel_switches[4] = panel_switches[5] = 1;
  auto expected_panel = assemble_pixel_program(
      panel_pass.definition, panel_pass.fragments, panel_pass.permutations,
      panel_pass.combiners, panel_switches);
  remap_pixel_constants(expected_panel, panel.pixel_constants,
                        panel_pass.pixel_remaps, panel_pass.fragments,
                        panel_switches);
  std::array<std::uint32_t, 256> externs{};
  externs[21] = 0x80000000;
  remap_pixel_constants(expected_panel, externs, panel_pass.pixel_externs,
                        panel_pass.fragments, panel_switches);
  check(
      panel_pass.program.words == expected_panel.words,
      "Cairo panel ignores its authored lighting variant/specular brightness");
  const auto tag =
      std::find_if(cache.tags().begin(), cache.tags().end(), [](const auto &t) {
        return t.group == "mode" &&
               t.name == "objects\\characters\\masterchief\\fp\\fp";
      });
  check(tag != cache.tags().end(),
        "retail Chief first-person render model missing");
  const auto model = cache.load_model(tag->datum);
  for (const auto &v : model.mesh.vertices) {
    check(length(v.normal) > .95f && length(v.normal) < 1.05f,
          "Chief arms lost the authored normal stream");
    check(length(v.tangent) > .95f && length(v.binormal) > .95f,
          "Chief arms lost the authored tangent basis");
  }
  const auto armor = std::find_if(
      model.mesh.materials.begin(), model.mesh.materials.end(),
      [](const auto &m) { return m.name.ends_with("\\shaders\\fp_arms"); });
  check(armor != model.mesh.materials.end(), "Chief armor material missing");
  check(armor->forward_pass, "Chief armor is still using diffuse-only fallback "
                             "instead of its color-mask program");
  const auto chief =
      std::find_if(cache.tags().begin(), cache.tags().end(), [](const auto &t) {
        return t.group == "bipd" &&
               t.name == "objects\\characters\\masterchief\\masterchief";
      });
  check(chief != cache.tags().end(), "retail Chief biped missing");
  const auto colors = cache.load_object_colors(chief->datum);
  check(length(colors[0] - Vec3{.458824f, .521569f, .313726f}) < 1e-6f,
        "Chief primary change color differs from authored tag");
  auto material = *armor;
  apply_object_colors(material, colors);
  check((material.forward_program.words[11] & 0xffffff) == 0x758550 &&
            (material.forward_program.words[12] & 0xffffff) == 0x55613a,
        "Chief color externs did not reach the original combiner slots");
  check(material.auxiliary.width == material.diffuse.width &&
            !material.auxiliary.pixels.empty(),
        "Chief color-mask texture missing");
  auto red = colors;
  red[0] = {1, 0, 0};
  apply_object_colors(material, red);
  check((material.forward_program.words[11] & 0xffffff) == 0xff0000,
        "object color rebinding did not replace the prior shader constant");
  const auto cube =
      std::find_if(cache.tags().begin(), cache.tags().end(), [](const auto &t) {
        return t.group == "bitm" &&
               t.name == "objects\\bitmaps\\reflection_maps\\masterchief_armor";
      });
  check(cube != cache.tags().end(), "Chief reflection map missing");
  const auto texture = cache.load_bitmap(cube->datum, 0);
  check(texture.layers == 6 && texture.mips.size() == 30 &&
            texture.pixels.size() == 16368,
        "retail cubemap lost a face or its resident mip chain");
  const auto lightmap_it = std::find_if(
      material.passes.begin(), material.passes.end(), [](const auto &p) {
        return p.vertex_shader.ends_with("\\lightmap_perpixel_tex_env");
      });
  check(lightmap_it != material.passes.end(),
        "Chief environment vertex program missing");
  const auto &lightmap = *lightmap_it;
  const auto program = decode_vertex_program(lightmap.vertex_code);
  check(program.count > 0, "original vertex program missing");
  Cache lockout(directory / "lockout.map");
  const auto bsp = lockout.load_bsp(0);
  const auto lm = std::find_if(
      bsp.materials.begin(), bsp.materials.end(), [](const auto &m) {
        return m.static_lightmap_pass &&
               m.passes.front().vertex_shader.ends_with(
                   "\\lightmap_perpixel_tex_detail");
      });
  check(lm != bsp.materials.end(), "retail detailed lightmap fixture missing");
  VertexProgramState state;
  state.constants[0] = {1, 0, 0, 0};
  state.constants[1] = {0, 1, 0, 0};
  state.constants[2] = {0, 0, 1, 0};
  state.constants[3] = {0, 0, 0, 1};
  state.constants[5] = {0, 1, 0, .5f};
  state.constants[6] = {0, 0, 1, 2};
  state.constants[10] = {1, 1, 1, 0};
  state.constants[18] = {2, 3, 1, 2};
  state.constants[19] = {4, 5, 6, 7};
  state.constants[20] = {8, 9, 10, 11};
  state.constants[172] = {1, 1, 0, 0};
  state.attributes[0] = {1, 2, 3, 1};
  state.attributes[1] = {2, 3, 0, 1};
  state.attributes[2] = {0, 0, 1, 1};
  state.attributes[3] = {1, 0, 0, 1};
  state.attributes[4] = {0, 1, 0, 1};
  state.attributes[5] = {0, 0, .25f, 1};
  state.attributes[6] = {.1f, .2f, 0, 1};
  execute_vertex_program(decode_vertex_program(lm->vertex_program), state);
  check(std::abs(state.outputs[4][3] - .25f) < 1e-6f &&
            std::abs(state.outputs[4][2] - 1) < 1e-6f,
        "authored vertex program lost incident intensity/direction");
  check(state.outputs[9][0] == 5 && state.outputs[9][1] == 11 &&
            state.outputs[10][0] == 26 && state.outputs[10][1] == 62 &&
            state.outputs[11][0] == 50 && state.outputs[11][1] == 110,
        "authored base transform must precede detail and bump transforms");
  check(state.outputs[12][0] == .1f && state.outputs[12][1] == .2f,
        "authored vertex program changed lightmap UVs");
  state.attributes[5] = {0, 0, 0, 1};
  execute_vertex_program(decode_vertex_program(lm->vertex_program), state);
  check(state.outputs[4] == ShaderVector({.5f, .5f, .5f, 0}),
        "zero incident lighting must remain finite through RSQ/MUL/MAD");
}
void skinning() {
  Material extern_material;
  extern_material.forward_pass=true;extern_material.forward_pass_index=0;
  extern_material.passes.resize(1);
  auto &color_pass=extern_material.passes.front();
  color_pass.program.words[10]=0x77123456;
  // Scalar/color banks reuse numeric indices; only RGB externs are colors.
  color_pass.pixel_externs={{0,3,3},{0x800,4,3}};
  apply_object_colors(extern_material,{{{0,1,.5f},{1,1,1},{1,1,1},{1,1,1}}});
  check(extern_material.forward_program.words[10]==0x7780ffbf,
        "change colors must respect extern banks and signed conversion");
  extern_material.environment_pass=std::make_shared<Material>(extern_material);
  const auto other_object=extern_material;
  apply_object_colors(extern_material,{{{1,0,0},{1,1,1},{1,1,1},{1,1,1}}});
  check(extern_material.environment_pass->forward_program.words[10]==0x77ff8080 &&
        other_object.environment_pass->forward_program.words[10]==0x7780ffbf,
        "environment color rebinding must not recolor another object sharing the material");
  VertexProgram move;
  move.count = 1;
  move.instructions[0] = {0, (1u << 21) | 0x1b, 2u << 26,
                          (15u << 12) | 2048u | 1u};
  VertexProgramState move_state;
  move_state.attributes[0] = {1, 2, 3, 1};
  execute_vertex_program(move, move_state);
  check(move_state.outputs[0] == move_state.attributes[0],
        "MOV must not read its unused B/C operand selectors");
  RenderModel model;
  ModelBone bone;
  bone.name = 1;
  bone.bind.rotation = {0, 0, .707106781f, .707106781f};
  bone.bind.translation = {10, 20, 30};
  bone.inverse = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  model.bones.push_back(bone);
  model.mesh.vertices.resize(1);
  model.skin.resize(1);
  auto &v = model.mesh.vertices[0];
  v.position = {1, 0, 0};
  v.normal = {1, 0, 0};
  v.tangent = {0, 1, 0};
  v.binormal = {0, 0, 1};
  const auto posed = pose_model(model);
  check(length(posed[0].normal - Vec3{0, 1, 0}) < 1e-6f &&
            length(posed[0].tangent - Vec3{-1, 0, 0}) < 1e-6f &&
            length(posed[0].binormal - Vec3{0, 0, 1}) < 1e-6f,
        "animated tangent basis must rotate without translation");
  std::vector<std::byte> faces(6 * 128);
  for (unsigned face = 0; face < 6; ++face) {
    const auto at = face * 128;
    faces[at] = std::byte(10 + face);
    faces[at + 1] = std::byte(20 + face);
    faces[at + 2] = std::byte(30 + face);
    faces[at + 3] = std::byte(255);
  }
  const auto cube = prepare_cubemap(Bytes(faces), 1, 11, false, 0);
  check(cube.layers == 6 && cube.mips.size() == 6, "synthetic cube lost faces");
  for (unsigned face = 0; face < 6; ++face)
    check(cube.pixels[face * 4] == std::byte(30 + face) &&
              cube.pixels[face * 4 + 2] == std::byte(10 + face),
          "cube face order or BGRA conversion wrong");
}
} // namespace
int main(int argc, char **argv) {
  try {
    skinning();
    if (argc > 1)
      retail(argv[1]);
    std::cout << "appearance input tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
