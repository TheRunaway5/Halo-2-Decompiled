#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <set>
#include <type_traits>

using namespace halo2;
namespace {
void require(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
std::uint32_t word(float value) { return std::bit_cast<std::uint32_t>(value); }
std::array<std::uint32_t, 33> vertex_words(const Vertex &v) {
  return {word(v.position.x), word(v.position.y), word(v.position.z),
      word(v.color.x), word(v.color.y), word(v.color.z), word(v.uv.x), word(v.uv.y),
      v.material, word(v.lightmap_uv.x), word(v.lightmap_uv.y),
      word(v.normal.x), word(v.normal.y), word(v.normal.z),
      word(v.tangent.x), word(v.tangent.y), word(v.tangent.z),
      word(v.binormal.x), word(v.binormal.y), word(v.binormal.z),
      word(v.incident.x), word(v.incident.y), word(v.incident.z), v.lightmap,
      std::bit_cast<std::uint32_t>(v.bsp_cluster), word(v.prt_cluster),
      word(v.prt_weights[0]), word(v.prt_weights[1]), word(v.prt_weights[2]),
      word(v.prt_weights[3]), v.source_section, v.source_part, v.source_stipple_level};
}
bool same_texture(const Texture &a, const Texture &b) {
  if (a.width != b.width || a.height != b.height || a.format != b.format ||
      a.layers != b.layers || a.depth != b.depth || a.pixels != b.pixels ||
      a.mips.size() != b.mips.size()) return false;
  for (std::size_t i = 0; i < a.mips.size(); ++i) {
    const auto &x = a.mips[i], &y = b.mips[i];
    if (x.width != y.width || x.height != y.height || x.depth != y.depth ||
        x.offset != y.offset || x.size != y.size) return false;
  }
  return true;
}
// Serialize explicit fields, not object padding or vector allocation state.
// Textures are checked separately against either original or approved pages.
struct Signature {
  std::vector<std::uint64_t> values;
  template<class T> requires (std::is_integral_v<T> || std::is_enum_v<T>)
  void add(const T &v) { values.push_back(static_cast<std::uint64_t>(v)); }
  void add(float v) { add(word(v)); }
  void add(const std::string &v) { add(v.size()); for (unsigned char c : v) add(c); }
  template<class T, std::size_t N> void add(const std::array<T, N> &v) {
    for (const auto &x : v) add(x);
  }
  template<class T> void add(const std::vector<T> &v) {
    add(v.size()); for (const auto &x : v) add(x);
  }
  template<class T> void add(const std::optional<T> &v) {
    add(v.has_value()); if (v) add(*v);
  }
  template<class T> void add(const std::shared_ptr<T> &v) {
    add(reinterpret_cast<std::uintptr_t>(v.get()));
  }
  template<class T, class... Rest> requires (sizeof...(Rest) > 0)
  void add(const T &v, const Rest &...rest) {
    add(v); (add(rest), ...);
  }
  void add(Vec3 v) { add(v.x, v.y, v.z); }
  void add(Vec2 v) { add(v.x, v.y); }
  void add(ShaderRange v) { add(v.packed); }
  void add(const ShaderFragment &v) { add(v.switch_index, v.permutations); }
  void add(const ShaderPermutation &v) { add(v.value, v.flags, v.combiners); }
  void add(const ShaderCombiner &v) { add(v.words); }
  void add(const ShaderRemap &v) { add(v.destination, v.component, v.source); }
  void add(const PixelProgram &v) { add(v.words, v.fragment_offsets); }
  void add(const ShaderTextureBinding &v) { add(v.parameter, v.external, v.stage, v.flags); }
  void add(const ShaderState &v) { add(v.index, v.value); }
  void add(const ShaderTextureState &v) {
    add(v.address_u, v.address_v, v.address_w, v.mag_filter, v.min_filter,
        v.mip_filter, v.lod_bias, v.max_mip, v.anisotropy, v.color_key_op,
        v.color_sign, v.alpha_kill, v.border_color, v.key_color);
  }
  void add(const ShaderBitmap &v) { add(v.datum, v.index, v.log_dimension); }
  void add(const ShaderLod &v) { add(v.layers, v.available_layers, v.projected_height); }
  void add(const SourceVertexDeclaration &v) { add(v.class_index, v.semantics); }
  void add(const MaterialPass &v) {
    add(v.name, v.vertex_shader, v.template_pass, v.implementation, v.textures,
        v.states, v.texture_states, v.bitmap_remaps, v.pixel_remaps, v.vertex_remaps,
        v.pixel_externs, v.vertex_externs, v.constants, v.program, v.definition,
        v.fragments, v.permutations, v.combiners, v.vertex_code, v.vertex_declaration);
  }
  void add(const ShaderOverlay &v) { add(v.input, v.range, v.period, v.curve); }
  void add(const ShaderOverlayReference &v) { add(v.function, v.transform); }
  void add(const ShaderAnimationImplementation &v) {
    add(v.bitmap_transforms, v.pixel_constants, v.vertex_constants);
  }
  void add(const ShaderAnimationLod &v) { add(v.available_layers, v.layers); }
  void add(const ShaderAnimation &v) {
    add(v.lods, v.layers, v.passes, v.parameters, v.implementations,
        v.overlays, v.references, v.mappings, v.tables);
  }
  void add(const ForwardRasterState &v) {
    add(v.color_write, v.depth_enable, v.depth_write, v.depth_function, v.cull,
        v.alpha_test, v.alpha_function, v.alpha_reference);
  }
  void add(const Material &m) {
    add(m.name, m.template_name, m.bitmaps, m.pixel_constants, m.vertex_constants,
        m.lods, m.layers, m.pass_implementations, m.passes, m.animation,
        m.shader_switches, m.specular_brightness, m.ambient_brightness,
        m.texture_transforms, m.lightmap_program, m.vertex_program,
        m.program_constants, m.vertex_bank, m.vertex_declaration,
        m.world_vertex_input_transform, m.texture_states, m.static_lightmap_pass,
        m.forward_pass, m.hud_pass, m.ui_full_surface, m.light_volume_pass,
        m.forward_pass_index, m.forward_vertex, m.forward_blend, m.forward_raster,
        m.forward_program, m.forward_transforms, m.forward_diffuse,
        m.forward_specular, m.texture_frames, m.environment_pass,
        m.planar_fog_pass, m.accumulation_texture, m.world_pass,
        m.per_vertex_lighting, m.baked_lightmap_texture,
        m.world_overlays_before, m.world_overlays_after, m.world_bloom_pass,
        m.world_bloom_passes, m.world_planar_passes,
        m.water_reflection_texture, m.water_refraction_texture, m.water_pass,
        m.scope_texture, m.water_definition, m.rendering_gaps,
        m.source_shader_datum, m.selected_shader_lod, m.object_render_colors,
        m.ui_emblem_pass, m.ui_widescreen_background, m.ui_frame_center,
        m.ui_frame_extension);
    for (const auto &bank : m.animated_textures) add(bank.size());
  }
};
Signature image_metadata(const FrontendContent &content) {
  Signature result;
  for (const auto &[datum, image] : content.images) {
    result.add(datum, image.name, image.images.size(), image.sequences.size());
    for (const auto &texture : image.images) {
      result.add(texture.width, texture.height, texture.format, texture.layers,
          texture.depth, texture.mips.size(), texture.pixels.size());
      for (const auto &mip : texture.mips)
        result.add(mip.width, mip.height, mip.offset, mip.size, mip.depth);
    }
    for (const auto &sequence : image.sequences) {
      result.add(sequence.size());
      for (const auto &sprite : sequence)
        result.add(sprite.image, sprite.left, sprite.right, sprite.top,
            sprite.bottom, sprite.registration);
    }
  }
  return result;
}
Texture sentinel_texture(unsigned seed) {
  Texture t{2, 2, 11, std::vector<std::byte>(16), {{2, 2, 0, 16, 1}}};
  for (unsigned i = 0; i < t.pixels.size(); ++i) t.pixels[i] = std::byte(seed + i);
  return t;
}
Mesh world_prefix(const std::string &colliding_name) {
  Mesh mesh;
  for (unsigned i = 0; i < 2; ++i) {
    Material m;
    m.name = i ? "test/world-fog-sentinel" : colliding_name;
    m.template_name = "test/original-world-template";
    m.diffuse = sentinel_texture(10 + i);
    m.detail = sentinel_texture(30 + i);
    m.bump = sentinel_texture(50 + i);
    m.auxiliary = sentinel_texture(70 + i);
    m.animated_textures[0] = {sentinel_texture(90 + i)};
    m.world_pass = true;
    m.forward_pass = true;
    m.forward_vertex = 6;
    m.source_shader_datum = 0xdecafbad;
    m.texture_states[0].address_u = 3;
    m.program_constants[0][0] = {1.25f, -2.5f, 3.75f, .125f};
    mesh.materials.push_back(std::move(m));
  }
  for (unsigned i = 0; i < 3; ++i) {
    Vertex v;
    v.position = {float(i) + .25f, -float(i) - .5f, float(i) * 7.5f};
    v.color = {.125f, .25f, .5f}; v.uv = {.375f, .625f}; v.material = i % 2;
    v.normal = {0, 0, 1}; v.bsp_cluster = -59;
    v.source_section = 52; v.source_part = 2;
    mesh.vertices.push_back(v);
  }
  mesh.minimum = {-1, -2, -3}; mesh.maximum = {1, 2, 3};
  mesh.clusters = 17; mesh.instances = 58;
  mesh.lightmaps = {sentinel_texture(110)};
  return mesh;
}
void material_metadata_equal(const Material &a, const Material &b, const std::string &where) {
  Signature x, y; x.add(a); y.add(b);
  require(x.values == y.values, where + ": shader, sampler, program, source binding or material state changed");
}
void check_world(const Mesh &mesh, const Mesh &original) {
  require(mesh.vertices.size() == original.vertices.size(), "UI pack changed world vertex count");
  for (std::size_t i = 0; i < original.vertices.size(); ++i)
    require(vertex_words(mesh.vertices[i]) == vertex_words(original.vertices[i]), "UI pack changed world vertices");
  for (std::size_t i = 0; i < original.materials.size(); ++i) {
    const auto &a = mesh.materials[i], &b = original.materials[i];
    material_metadata_equal(a, b, "world material prefix");
    require(same_texture(a.diffuse, b.diffuse) && same_texture(a.detail, b.detail) &&
        same_texture(a.bump, b.bump) && same_texture(a.auxiliary, b.auxiliary),
        "UI pack replaced a world material texture, including a colliding original UI name");
    for (unsigned stage = 0; stage < 4; ++stage) {
      require(a.animated_textures[stage].size() == b.animated_textures[stage].size(),
              "UI pack changed world animated bank size");
      for (std::size_t page = 0; page < a.animated_textures[stage].size(); ++page)
        require(same_texture(a.animated_textures[stage][page], b.animated_textures[stage][page]),
                "UI pack changed a world animated texture");
    }
  }
  Signature a, b;
  a.add(mesh.minimum, mesh.maximum, mesh.clusters, mesh.instances);
  b.add(original.minimum, original.maximum, original.clusters, original.instances);
  require(a.values == b.values && mesh.lightmaps.size() == original.lightmaps.size(),
          "UI pack changed world bounds, admission counts or lightmaps");
  for (std::size_t i = 0; i < mesh.lightmaps.size(); ++i)
    require(same_texture(mesh.lightmaps[i], original.lightmaps[i]), "UI pack changed a world lightmap");
}
struct EmblemStages { std::array<const std::vector<Texture> *, 2> pages{}; std::array<unsigned, 2> initial{}; };
EmblemStages emblem_stages(const Material &m, const FrontendContent &content,
                            const FrontendUiAssetPack &pack) {
  EmblemStages result;
  if (!m.ui_emblem_pass) return result;
  require(m.passes.size() == 1, "original 2D emblem pass scope changed");
  const auto &pass = m.passes.front();
  for (const auto &binding : pass.textures) {
    require(!binding.external && binding.stage < 2, "unsupported original 2D emblem binding");
    const auto remap = std::find_if(pass.bitmap_remaps.begin(), pass.bitmap_remaps.end(),
        [&](const auto &v) { return (v.destination & 255) == binding.parameter; });
    require(remap != pass.bitmap_remaps.end() && remap->source < m.bitmaps.size(), "missing original emblem remap");
    const auto &bitmap = m.bitmaps[remap->source];
    const auto image = content.images.find(bitmap.datum);
    if (image == content.images.end()) continue;
    const auto replacement = pack.find(image->second.name);
    if (replacement == pack.end()) continue;
    result.pages[binding.stage] = &replacement->second;
    result.initial[binding.stage] = bitmap.index;
  }
  return result;
}
void check_materials(const Mesh &stock, const Mesh &hd, std::size_t prefix,
                     const FrontendContent &content, const FrontendUiAssetPack &pack) {
  require(stock.materials.size() == hd.materials.size(), "UI pack changed material bank indices");
  std::map<std::string, unsigned> pages;
  unsigned replaced = 0, enlarged = 0, fonts = 0, framing = 0, emblem_stages_replaced = 0;
  for (std::size_t i = prefix; i < stock.materials.size(); ++i) {
    const auto &a = stock.materials[i], &b = hd.materials[i];
    material_metadata_equal(a, b, "material " + std::to_string(i) + " " + a.name);
    const auto replacement = pack.find(a.name);
    const auto emblem = emblem_stages(a, content, pack);
    const Texture *expected_diffuse = &a.diffuse, *expected_detail = &a.detail;
    if (a.ui_emblem_pass) {
      if (emblem.pages[0]) { expected_diffuse = &emblem.pages[0]->at(emblem.initial[0]); ++emblem_stages_replaced; }
      if (emblem.pages[1]) { expected_detail = &emblem.pages[1]->at(emblem.initial[1]); ++emblem_stages_replaced; }
    } else if (replacement != pack.end()) {
      expected_diffuse = &replacement->second.at(pages[a.name]++);
      ++replaced;
      enlarged += expected_diffuse->width > a.diffuse.width || expected_diffuse->height > a.diffuse.height;
    }
    require(same_texture(b.diffuse, *expected_diffuse) && same_texture(b.detail, *expected_detail),
            "uploaded UI stage differs from original/approved texture: " + a.name);
    require(same_texture(a.bump, b.bump) && same_texture(a.auxiliary, b.auxiliary),
            "UI pack changed an unrelated material stage: " + a.name);
    for (unsigned stage = 0; stage < 4; ++stage) {
      require(a.animated_textures[stage].size() == b.animated_textures[stage].size(),
              "UI pack changed animated bank cardinality: " + a.name);
      for (std::size_t page = 0; page < a.animated_textures[stage].size(); ++page) {
        const auto &expected = a.ui_emblem_pass && stage < 2 && emblem.pages[stage]
            ? emblem.pages[stage]->at(page) : a.animated_textures[stage][page];
        require(same_texture(b.animated_textures[stage][page], expected),
                "UI animated stage did not use its approved source: " + a.name);
      }
    }
    if (a.name.starts_with("font/")) { ++fonts; require(same_texture(a.diffuse, b.diffuse), "UI pack replaced an Xbox font"); }
    if (a.name.find("framing") != a.name.npos) { ++framing; require(same_texture(a.diffuse, b.diffuse), "UI pack replaced a dedicated framing texture"); }
  }
  require(replaced && enlarged && fonts && framing && emblem_stages_replaced,
          "actual Vista pack did not exercise sampled HD pages, stock fonts/framing and mapped 2D emblems");
}
}

int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::runtime_error("maps directory and exported UI pack directory required");
    const std::filesystem::path maps(argv[1]), directory(argv[2]);
    if (!std::filesystem::exists(maps / "mainmenu.map") || !std::filesystem::exists(directory)) return 77;
    Cache cache(maps / "mainmenu.map");
    const auto content = cache.load_frontend();
    const auto original_images = image_metadata(content);
    const auto pack = load_frontend_ui_assets(content, directory);
    require(!pack.empty(), "actual exported Vista UI pack is empty");
    require(image_metadata(content).values == original_images.values, "pack import changed original logical images/sequences");
    const auto prefix = world_prefix(pack.begin()->first);
    auto stock_mesh = prefix, hd_mesh = prefix;
    FrontendPainter stock(content, maps / "fonts", stock_mesh);
    FrontendPainter hd(content, maps / "fonts", hd_mesh, {}, pack);
    check_world(stock_mesh, prefix); check_world(hd_mesh, prefix);
    check_materials(stock_mesh, hd_mesh, prefix.materials.size(), content, pack);
    unsigned draws = 0;
    auto compare = [&](MenuState &state, float seconds, int pane = -1, unsigned clock = 5000) {
      const auto a = stock.draw(state, seconds, pane, clock), b = hd.draw(state, seconds, pane, clock);
      const auto where = "screen " + std::to_string(state.frame().screen) + " pane " + std::to_string(pane);
      require(a.vertices.size() == b.vertices.size(), where + ": HD changed UI vertex count");
      for (std::size_t i = 0; i < a.vertices.size(); ++i)
        require(vertex_words(a.vertices[i]) == vertex_words(b.vertices[i]),
                where + ": HD changed a UI float bit or integer/material index at vertex " + std::to_string(i));
      require(a.scene_color_filter == b.scene_color_filter && a.backdrops.size() == b.backdrops.size() &&
                  a.scenes.size() == b.scenes.size(), where + ": backdrop/model scheduling changed");
      for (std::size_t i = 0; i < a.backdrops.size(); ++i) {
        const auto &x = a.backdrops[i], &y = b.backdrops[i];
        Signature sx, sy;
        sx.add(x.before_vertex, x.fade, x.tint, x.widescreen_background);
        sy.add(y.before_vertex, y.fade, y.tint, y.widescreen_background);
        require(sx.values == sy.values, where + ": backdrop ordering/tint changed");
      }
      for (std::size_t i = 0; i < a.scenes.size(); ++i) {
        const auto &x = a.scenes[i], &y = b.scenes[i];
        Signature sx, sy; sx.add(x.before_vertex, x.animation_offset); sy.add(y.before_vertex, y.animation_offset);
        require(x.definition == y.definition && sx.values == sy.values && x.appearance == y.appearance,
                where + ": original 3D UI model definition, position or appearance changed");
      }
      for (int y = -8; y <= 488; y += 8)
        for (int x = -8; x <= 648; x += 8)
          require(stock.hit(float(x), float(y)) == hd.hit(float(x), float(y)), where + ": mouse hit target changed");
      ++draws;
    };
    // This imported-definition sweep proves geometry equality, not flow parity.
    unsigned panes = 0;
    for (const auto &[id, screen] : content.screens) {
      MenuState state(content);
      if (id != 9) state.open(id);
      for (unsigned pane = 0; pane < screen.panes.size(); ++pane) {
        compare(state, 5, int(pane));
        ++panes;
      }
    }
    // Exercise original local routes and focus mutations beyond pane override.
    MenuState settings(content);
    settings.ui_clock(100); settings.open(19);
    compare(settings, 0, -1, 100);
    settings.ui_clock(125); settings.select(1); compare(settings, .125f, -1, 125);
    settings.ui_clock(200); settings.select(0); compare(settings, .2f, -1, 200);
    MenuState menu(content);
    compare(menu, 5);
    menu.accept(); compare(menu, 5); // actual title -> profile picker
    menu.accept();
    require(menu.frame().screen == 167, "ordinary profile route missed keyboard167");
    compare(menu, 5); menu.text("Asset Test"); compare(menu, 5);
    menu.commit_text(); compare(menu, 5);
    menu.back();
    menu.open(47); compare(menu, 5);
    for (unsigned row = 0; row < menu.frame().entries.size(); ++row) {
      menu.select(row); compare(menu, 5);
    }
    menu.back();
    menu.select(2); menu.accept(); menu.accept();
    require(menu.frame().screen == 14 && content.screens.at(14).name.find("pregame_lobby") != std::string::npos,
            "ordinary Split Screen route missed original pregame lobby14");
    compare(menu, 5);
    menu.select(1); menu.accept();
    require(menu.frame().screen == 23, "ordinary lobby route missed Setup23");
    compare(menu, 5); menu.accept();
    require(menu.frame().screen == 206, "ordinary Setup route missed Map206");
    for (unsigned time : {0u, 125u, 249u, 250u, 500u})
      compare(menu, float(time) / 1000.f, -1, time);
    require(image_metadata(content).values == original_images.values, "painter changed original image dimensions/sequences");
    check_world(stock_mesh, prefix); check_world(hd_mesh, prefix);
    check_materials(stock_mesh, hd_mesh, prefix.materials.size(), content, pack);
    std::cout << panes << " authored panes and " << draws << " paired draws preserve all UI geometry/hits and original world/fonts/framing; approved sampled pages and 2D emblem banks verified\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
