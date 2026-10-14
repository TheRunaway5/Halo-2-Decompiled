#include "halo2/frontend.hpp"
#include "halo2/frontend_keyboard.hpp"
#include "halo2/frontend_signin.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/frontend_lobby_roster.hpp"
#include "halo2/frontend_lobby_setup.hpp"
#include "halo2/frontend_lobby_warning.hpp"
#include "halo2/frontend_text_style.hpp"
#include "halo2/frontend_typewriter.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/frontend_frame_texture.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include "halo2/frontend_exit_animation.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
namespace halo2 {
namespace {
std::vector<unsigned> codepoints(const std::string &s, bool discard_cr = true) {
  std::vector<unsigned> result;
  for (std::size_t i = 0; i < s.size();) {
    unsigned c = static_cast<unsigned char>(s[i++]);
    unsigned n = 0;
    if (c >= 0xc0 && c < 0xe0) {
      c &= 31;
      n = 1;
    } else if (c >= 0xe0 && c < 0xf0) {
      c &= 15;
      n = 2;
    } else if (c >= 0xf0) {
      c &= 7;
      n = 3;
    }
    while (n-- && i < s.size())
      c = (c << 6) | (static_cast<unsigned char>(s[i++]) & 63);
    if (!discard_cr || c != '\r')
      result.push_back(c);
  }
  return result;
}
std::u16string standard_text_buffer(const std::string &value) {
  std::u16string result;
  for (auto c : codepoints(value, false)) {
    if (c <= 0xffff) {
      result.push_back(static_cast<char16_t>(c));
    } else {
      if (c > 0x10ffff) throw FormatError("invalid frontend text codepoint");
      c -= 0x10000;
      result.push_back(static_cast<char16_t>(0xd800 + (c >> 10)));
      result.push_back(static_cast<char16_t>(0xdc00 + (c & 0x3ff)));
    }
  }
  // Standard c_text_widget_45a5e0 owns a256-unit UTF16 buffer. Its original
  // 22F545 ->13FAC0 copy reserves the final unit for NUL.
  if (result.size() > 255) result.resize(255);
  if (const auto end = result.find(u'\0'); end != result.npos) result.resize(end);
  return result;
}
std::string pcr_utf8(std::u16string_view value) {
  std::string result;
  for (std::size_t i=0;i<value.size();++i) {
    unsigned c=value[i];
    if(c>=0xd800 && c<=0xdbff && i+1<value.size() &&
        value[i+1]>=0xdc00 && value[i+1]<=0xdfff)
      c=0x10000+((c-0xd800)<<10)+(value[++i]-0xdc00);
    if(c<0x80)result+=char(c);
    else if(c<0x800){result+=char(0xc0|(c>>6));result+=char(0x80|(c&63));}
    else if(c<0x10000){result+=char(0xe0|(c>>12));result+=char(0x80|((c>>6)&63));result+=char(0x80|(c&63));}
    else {result+=char(0xf0|(c>>18));result+=char(0x80|((c>>12)&63));result+=char(0x80|((c>>6)&63));result+=char(0x80|(c&63));}
  }
  return result;
}
UiColor pcr_color(const std::array<std::uint32_t,3>& value) {
  return {1,std::bit_cast<float>(value[0]),std::bit_cast<float>(value[1]),
            std::bit_cast<float>(value[2])};
}
UiColor appearance_namebar_pixel(UiColor color) {
  // 2B05C8..2B0635 stores each f32 channel*255 before FISTP under
  // source FPCW027F (nearest, ties even). This is not text's truncation.
  auto channel = [](float value) {
    const float scaled = value * 255.f;
    const float lower = std::floor(scaled), fraction = scaled - lower;
    auto integer = unsigned(lower);
    if (fraction > .5f || (fraction == .5f && (integer & 1))) ++integer;
    return float(integer) / 255.f;
  };
  return {channel(color.alpha), channel(color.red), channel(color.green),
          channel(color.blue)};
}
UiKey interpolate(const UiTrack &track, float milliseconds) {
  if (track.keys.empty())
    return {};
  if (track.keys.size() == 1 || !track.milliseconds)
    return track.keys.back();
  const float position =
      std::clamp(milliseconds / track.milliseconds, 0.f, 1.f) *
      float(track.keys.size() - 1);
  const auto a = std::size_t(position),
             b = std::min(a + 1, track.keys.size() - 1);
  const float t = position - float(a);
  const auto &x = track.keys[a];
  const auto &y = track.keys[b];
  return {x.alpha + (y.alpha - x.alpha) * t,
          {x.position.x + (y.position.x - x.position.x) * t,
           x.position.y + (y.position.y - x.position.y) * t,
           x.position.z + (y.position.z - x.position.z) * t}};
}
bool online_panel(const MenuState &state) {
  return std::any_of(state.stack().begin(),state.stack().end(),[&](const auto &f) {
    if(f.vista_pcr && f.vista_pcr->online)return true;
    if(state.online_clan_screen(f) || state.online_party_screen(f) || state.online_friends_screen(f))return true;
    switch(f.screen) {
    case 20: case 21: case 26: case 27: case 31: case 34: case 35: case 36:
    case 182: case 186: case 210: case 211: case 215: case 216: case 217:
    case 219: case 236: case 237:return true;
    default:return false;
    }
  });
}
} // namespace
struct FrontendPainter::Impl {
  const FrontendContent &content;
  std::vector<RetailFont> fonts;
  std::vector<unsigned> font_materials;
  std::map<std::uint32_t, std::vector<unsigned>> images;
  std::map<unsigned, std::array<unsigned, 2>> frame_extensions;
  unsigned white{}, background_dim{}, pcr_black{};
  UiColor clan_background_color;
  Mesh &mesh;
  std::unique_ptr<Impl> vista;
  std::unique_ptr<Impl> online;
  bool missing_online_assets{};
  const Impl *online_roster_fonts{};
  bool online_vista_art{};
  std::map<std::string,std::vector<unsigned>> vista_art;
  std::map<const UiEmblem *, unsigned> emblem_materials;
  std::map<const UiGrid *, std::vector<std::vector<unsigned>>> grid_emblem_materials;
  std::map<const UiList *, std::vector<std::vector<unsigned>>> profile_emblem_materials;
  UiDraw output;
  FrontendCanvas canvas = frontend_canvas(640, 480);
  float clan_width_extra() const { return canvas.surface_width / canvas.scale - 529; }
  float clan_footer_offset() const { return canvas.surface_height / canvas.scale - 420; }
  struct Hit {
    float left{}, top{}, right{}, bottom{};
    unsigned index{};
    FrontendPointerAction action = FrontendPointerAction::Entry;
  };
  std::vector<Hit> hits;
  unsigned pointer_revision{};
  struct WidgetSubmission {
    std::size_t first{}, last{};
    int layer{};
    unsigned type{};
    float depth{};
  };
  std::vector<WidgetSubmission> keyboard_widgets;
  float seconds{};
  std::uint32_t ui_clock_ms{};
  struct TypingWidget {
    FrontendTypewriterState state;
    std::u16string text;
    bool initialized{};
  };
  using TextLifetime = std::pair<std::uint64_t, std::uint64_t>;
  std::map<TextLifetime, std::map<const UiText *, TypingWidget>> typing_widgets;
  std::shared_ptr<const MenuFrame> previous_lobby_setup;
  std::map<unsigned, FrontendExitAnimation> exit_animations;
  std::uint32_t exit_elapsed_ms{};
  bool drawing_exit{};
  static bool is_profile_skin(const UiSkin &skin) {
    return skin.name == "ui\\list_skins\\profile_picker\\profile_picker" ||
           skin.name == "ui\\list_skins\\profile_picker2\\profile_picker";
  }
  Impl(const FrontendContent &c, const std::filesystem::path &directory,
       Mesh &mesh, const std::filesystem::path &edge_directory,
       const FrontendUiAssetPack &ui_assets,
       const std::filesystem::path &sampled_fonts, bool separate_online = true)
      : content(c), mesh(mesh) {
    white = unsigned(mesh.materials.size());
    mesh.materials.push_back({});
    background_dim = unsigned(mesh.materials.size());
    mesh.materials.push_back({"ui/background-dim"});
    mesh.materials.back().ui_widescreen_background = true;
    pcr_black = unsigned(mesh.materials.size());
    mesh.materials.push_back({"ui/vista-pcr-intro-black"});
    mesh.materials.back().ui_widescreen_background = true;
    for (const auto &[id, image] : c.images)
      for (std::size_t page = 0; page < image.images.size(); ++page) {
        const auto &texture = image.images[page];
        const auto replacement = ui_assets.find(image.name);
        // Only the uploaded page changes. All layout, hit bounds, sprite
        // registration, row spacing and animation continue reading c.images.
        if (replacement != ui_assets.end() &&
            replacement->second.size() != image.images.size())
          throw FormatError("incompatible frontend sampled image bank");
        images[id].push_back(unsigned(mesh.materials.size()));
        mesh.materials.push_back({image.name, replacement == ui_assets.end()
            ? texture : replacement->second[page]});
        if (page == 0 && image.name == "ui\\screens\\game_shell\\online_y_menu\\y_menu_bkd") {
          const auto &sample = mesh.materials.back().diffuse;
          const auto decoded = sample.format == 11 ? sample : prepare_xemu_bc_texture(sample);
          const auto pixel = ((decoded.height / 5) * decoded.width + decoded.width / 2) * 4;
          clan_background_color = {1,
              float(std::to_integer<unsigned>(decoded.pixels.at(pixel))) / 255,
              float(std::to_integer<unsigned>(decoded.pixels.at(pixel + 1))) / 255,
              float(std::to_integer<unsigned>(decoded.pixels.at(pixel + 2))) / 255};
        }
        // Decorative tracks may span the surface. Preserve framing_center's
        // original quad/UVs and use separate constant-U edge strips instead.
        mesh.materials.back().ui_widescreen_background =
            image.name.starts_with("ui\\global_bitmaps\\track");
        // Both source frames keep their authored center quad and use the
        // shared native side-margin route. PC sampling is calculated after
        // its height/1200 transform, just like the renderer's center clamp.
        if (image.name == "ui\\global_bitmaps\\framing_center") {
          const auto original = images[id].back();
          mesh.materials.back().ui_frame_center = true;
          std::array<unsigned, 2> extensions{};
          for (unsigned edge = 0; edge < extensions.size(); ++edge) {
            extensions[edge] = unsigned(mesh.materials.size());
            const auto filename=std::string(edge ? "right-edge" : "left-edge")+
                (image.images.size()>1 ? "-"+std::to_string(images[id].size()-1) : "")+".dds";
            mesh.materials.push_back({image.name + (edge ? "/native-right-edge" :
                                                          "/native-left-edge"),
                replacement != ui_assets.end() ? replacement->second[page] :
                frontend_frame_edge_texture(texture,edge_directory.empty() ? std::filesystem::path{} :
                                                                              edge_directory/filename)});
            mesh.materials.back().ui_frame_extension = edge ? UiFrameExtension::Right :
                                                            UiFrameExtension::Left;
          }
          frame_extensions.emplace(original, extensions);
        }
      }
    std::ifstream table(directory / "font_table.txt");
    if (!table)
      throw FormatError("missing retail font table");
    std::map<std::string, unsigned> unique;
    std::string name;
    while (std::getline(table, name)) {
      if (!name.empty() && name.back() == '\r')
        name.pop_back();
      if (name.empty())
        continue;
      if (name.find_first_of("/\\") != std::string::npos || name == "..")
        throw FormatError("invalid retail font table name");
      if (fonts.size() >= (c.source_fonts.empty() ? 11u : 12u))
        throw FormatError("excessive retail font count");
      auto font_path = directory / name;
      if (!std::filesystem::is_regular_file(font_path) && !c.source_fonts.empty()) {
        auto lower = [](std::string value) {
          for (auto &ch : value) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
          return value;
        };
        for (const auto &entry : std::filesystem::directory_iterator(directory))
          if (lower(entry.path().filename().string()) == lower(name)) font_path = entry.path();
      }
      fonts.push_back(load_retail_font(font_path));
      const bool mcc=is_mcc_classic_menu_pack(sampled_fonts.parent_path());
      const auto sampled=mcc?ui_sampled_font_path(sampled_fonts.parent_path(),unsigned(fonts.size()-1),name):sampled_fonts/name;
      if(!sampled_fonts.empty() && std::filesystem::is_regular_file(sampled)) {
        if(mcc)apply_mcc_classic_font_sampling(fonts.back(),load_retail_font(sampled));
        else apply_retail_font_sampling(fonts.back(),load_retail_font(sampled));
      } else if(mcc)throw FormatError("Missing MCC Classic menu font owner");
      if(!sampled_fonts.empty())
        apply_vista_prompt_sampling(fonts.back(),sampled_fonts.parent_path(),unsigned(fonts.size()-1));
      // Original selectors may share a logical font but select different MCC
      // rasters. Their combined atlases must have distinct material owners.
      const auto key=mcc?name+"/"+sampled.filename().string():name;
      if (!unique.contains(key)) {
        unique[key] = unsigned(mesh.materials.size());
        mesh.materials.push_back({"font/" + key, fonts.back().atlas});
        // Original 1F070 -> 35B90 font descriptor clamps both atlas axes.
        // See font-combiner-original-audit.json; the default material repeats.
        auto &sampler = mesh.materials.back().texture_states[0];
        sampler.address_u = sampler.address_v = 3;
      }
      font_materials.push_back(unique.at(key));
    }
    if (fonts.size() != (c.source_fonts.empty() ? 11u : 12u))
      throw FormatError("incomplete retail font table");
    auto sampled_emblem = [&](Material material) {
      if (ui_assets.empty()) return material;
      if (!material.ui_emblem_pass || material.passes.size() != 1)
        throw FormatError("frontend sampled emblem needs its existing 2D pass");
      const auto &pass = material.passes.front();
      Texture *stages[]{&material.diffuse, &material.detail};
      for (const auto &binding : pass.textures) {
        const auto remap = std::find_if(pass.bitmap_remaps.begin(),
            pass.bitmap_remaps.end(), [&](const auto &r) {
              return (r.destination & 255) == binding.parameter;
            });
        if (binding.external || binding.stage > 1 ||
            remap == pass.bitmap_remaps.end() ||
            remap->source >= material.bitmaps.size())
          throw FormatError("unsupported frontend sampled emblem binding");
        const auto &bitmap = material.bitmaps[remap->source];
        const auto image = content.images.find(bitmap.datum);
        if (image == content.images.end()) continue;
        const auto replacement = ui_assets.find(image->second.name);
        if (replacement == ui_assets.end()) continue;
        if (bitmap.index >= replacement->second.size())
          throw FormatError("frontend sampled emblem page outside image bank");
        *stages[binding.stage] = replacement->second[bitmap.index];
        auto &animated = material.animated_textures[binding.stage];
        if (!animated.empty()) {
          if (animated.size() != replacement->second.size())
            throw FormatError("frontend sampled emblem animation bank differs");
          animated = replacement->second;
        }
      }
      return material;
    };
    auto prepare_emblems = [&](const auto &owner) {
      for (const auto &e : owner.emblems)
        if (content.emblem_shaders.contains(e.shader)) {
          emblem_materials.emplace(&e, unsigned(mesh.materials.size()));
          mesh.materials.push_back(sampled_emblem(content.emblem_shaders.at(e.shader)));
        }
    };
    for (const auto &[id, screen] : c.screens)
      for (const auto &pane : screen.panes) prepare_emblems(pane);
    for (const auto &[id, skin] : c.skin_groups) prepare_emblems(skin);
    for (const auto &skin : c.skins) prepare_emblems(skin);
    // Original2B12CA stores appearance on each constructed row child, and
    //2B11C9 binds that child's own focus scale. Reserve distinct material
    //banks before Renderer construction, including repeated grid instances.
    auto row_emblems = [&](auto &&self, const UiSkin &skin,
                           std::vector<const UiEmblem *> &instances,
                           unsigned depth) -> void {
      if (depth > 16)
        throw FormatError("cyclic frontend skin group");
      for (const auto &e : skin.emblems)
        if (c.emblem_shaders.contains(e.shader)) instances.push_back(&e);
      for (const auto &grid : skin.grids)
        if (c.skin_groups.contains(grid.skin))
          cells(grid, {}, [&](Vec2) {
            self(self, c.skin_groups.at(grid.skin), instances, depth + 1);
          });
    };
    // Repeated lobby grid cells construct independent type9 widgets in the
    // original. Their shader banks must survive together in one submission.
    for (const auto &[id, screen] : c.screens)
      for (const auto &pane : screen.panes)
        for (const auto &grid : pane.grids) {
          if(!c.skin_groups.contains(grid.skin))continue;
          std::vector<const UiEmblem *> instances;
          row_emblems(row_emblems,c.skin_groups.at(grid.skin),instances,0);
          if(instances.empty())continue;
          auto &cells_materials=grid_emblem_materials[&grid];
          cells(grid,{},[&](Vec2) {
            auto &cell=cells_materials.emplace_back();
            for(const auto *e:instances) {
              cell.push_back(unsigned(mesh.materials.size()));
              mesh.materials.push_back(sampled_emblem(c.emblem_shaders.at(e->shader)));
            }
          });
        }
    for (const auto &[id, screen] : c.screens)
      for (const auto &pane : screen.panes)
        for (const auto &list : pane.lists) {
          if(!list.visible)continue;
          const auto &skin = c.skins.at(list.skin);
          if (!is_profile_skin(skin)) continue;
          std::vector<const UiEmblem *> instances;
          row_emblems(row_emblems, skin, instances, 0);
          auto &rows = profile_emblem_materials[&list];
          rows.resize(list.visible);
          for (auto &row : rows)
            for (const auto *e : instances) {
              row.push_back(unsigned(mesh.materials.size()));
              mesh.materials.push_back(sampled_emblem(c.emblem_shaders.at(e->shader)));
            }
        }
    if (c.vista) {
      const bool mcc=is_mcc_classic_menu_pack(sampled_fonts.parent_path());
      const auto sampled=mcc?load_frontend_ui_assets(*c.vista,sampled_fonts.parent_path()):FrontendUiAssetPack{};
      vista = std::make_unique<Impl>(*c.vista, c.vista->source_fonts, mesh,
                                    std::filesystem::path{}, sampled, mcc?sampled_fonts:std::filesystem::path{});
      vista->online_roster_fonts = this;
      for(const auto &[id,image]:c.vista->images)
        vista_art.emplace(image.name,vista->images.at(id));
      if(separate_online) {
        // The PC cache and imported bank share one source root. A selected
        // shell pack must not replace ONLINE/clan fonts or bitmap resources.
        const auto root=c.vista->source_fonts.parent_path().parent_path();
        std::error_code error;
        const bool selected_vista=!sampled_fonts.empty() &&
            std::filesystem::equivalent(sampled_fonts.parent_path(),root,error);
        if(!selected_vista) {
          if(std::filesystem::is_regular_file(root/"pack.tsv"))
            online=std::make_unique<Impl>(c,directory,mesh,edge_directory,
                load_frontend_ui_assets(c,root),root/"fonts",false);
          else missing_online_assets=true;
        }
      }
    }
  }
  UiKey animation(unsigned index, unsigned delay = 0) {
    if (!index || index > content.animations.size())
      return {};
    const auto &a = content.animations[index - 1];
    if (drawing_exit)
      return exit_animations[index].update(a.outro, exit_elapsed_ms, false);
    const float ms = seconds * 1000 - float(delay);
    if (ms < 0)
      return {0, {}};
    if (ms < float(a.intro.milliseconds))
      return interpolate(a.intro, ms);
    if (!a.ambient.keys.empty()) {
      float ambient = ms - float(a.intro.milliseconds);
      if (a.ambient.milliseconds) {
        if (a.loop == 1) {
          const float d = float(a.ambient.milliseconds);
          ambient = std::fmod(ambient, d * 2);
          if (ambient > d)
            ambient = d * 2 - ambient;
        } else
          ambient = std::fmod(ambient, float(a.ambient.milliseconds));
      }
      return interpolate(a.ambient, ambient);
    }
    return interpolate(a.intro, float(a.intro.milliseconds));
  }
  void keyboard_widget(std::size_t first, int layer, unsigned type,
                       float depth) {
    if (output.vertices.size() > first)
      keyboard_widgets.push_back(
          {first, output.vertices.size(), layer, type, depth});
  }
  void order_widgets(std::vector<WidgetSubmission> &widgets,
                     const char *unrecorded) {
    if (widgets.empty())
      return;
    const auto first = widgets.front().first;
    std::size_t count = 0;
    float nearest = std::numeric_limits<float>::max(), farthest = -nearest;
    for (const auto &widget : widgets) {
      count += widget.last - widget.first;
      nearest = std::min(nearest, widget.depth);
      farthest = std::max(farthest, widget.depth);
    }
    if (count != output.vertices.size() - first)
      throw FormatError(unrecorded);
    // The proved keyboard and appearance popup submissions share one depth
    // group. 2356D8 orders that group by layer ascending, then type descending.
    // Do not turn its nontransitive epsilon into a general depth sort.
    if (farthest - nearest > .001f)
      return;
    std::stable_sort(widgets.begin(), widgets.end(),
        [](const WidgetSubmission &left, const WidgetSubmission &right) {
          return left.layer != right.layer ? left.layer < right.layer
                                          : left.type > right.type;
        });
    std::vector<Vertex> ordered;
    ordered.reserve(count);
    for (const auto &widget : widgets)
      ordered.insert(ordered.end(), output.vertices.begin() + widget.first,
                     output.vertices.begin() + widget.last);
    std::copy(ordered.begin(), ordered.end(), output.vertices.begin() + first);
  }
  void keyboard_order() {
    order_widgets(keyboard_widgets, "unrecorded keyboard widget draw");
  }
  void quad(float x, float y, float width, float height, unsigned material,
            UiColor c = {1, 1, 1, 1}, float u0 = 0, float v0 = 0, float u1 = 1,
            float v1 = 1) {
    if (width <= 0 || height <= 0 || c.alpha <= 0)
      return;
    const Vertex a{
        {x, y, c.alpha}, {c.red, c.green, c.blue}, {u0, v0}, material},
        b{{x + width, y, c.alpha},
          {c.red, c.green, c.blue},
          {u1, v0},
          material};
    const Vertex d{
        {x, y + height, c.alpha}, {c.red, c.green, c.blue}, {u0, v1}, material},
        e{{x + width, y + height, c.alpha},
          {c.red, c.green, c.blue},
          {u1, v1},
          material};
    output.vertices.insert(output.vertices.end(), {a, b, d, b, e, d});
  }
  // Extend flat artwork between fixed-size edge slices. Text and icons never
  // pass through this operation, and the imported image remains unchanged.
  void extend_bitmap(std::size_t first, Vec2 extra, float left_cap = 8,
                     float right_cap = 8) {
    if (output.vertices.size() != first + 6 || (extra.x == 0 && extra.y == 0)) return;
    const auto a = output.vertices[first], b = output.vertices[first + 4];
    const float w = b.position.x - a.position.x, h = b.position.y - a.position.y;
    left_cap = std::min(left_cap, w * .5f);
    right_cap = std::min(right_cap, w * .5f);
    const float y_cap = std::min(8.f, h * .5f);
    const float xs[]{0, left_cap, w - right_cap, w};
    const float ys[]{0, y_cap, h - y_cap, h};
    const float dx[]{0, left_cap, w - right_cap + extra.x, w + extra.x};
    const float dy[]{0, y_cap, h - y_cap + extra.y, h + extra.y};
    output.vertices.resize(first);
    for (unsigned y = 0; y < 3; ++y)
      for (unsigned x = 0; x < 3; ++x)
        quad(a.position.x + dx[x], a.position.y + dy[y], dx[x+1] - dx[x], dy[y+1] - dy[y],
             a.material, {a.position.z, a.color.x, a.color.y, a.color.z},
             a.uv.x + (b.uv.x - a.uv.x) * xs[x] / w,
             a.uv.y + (b.uv.y - a.uv.y) * ys[y] / h,
             a.uv.x + (b.uv.x - a.uv.x) * xs[x+1] / w,
             a.uv.y + (b.uv.y - a.uv.y) * ys[y+1] / h);
  }
  void bitmap(const UiBitmap &b, Vec2 parent = {}, float alpha = 1,
              int frame = -1, std::optional<UiColor> tint = {},
              std::optional<float> clip_bottom = {}) {
    if (!images.contains(b.datum) || images.at(b.datum).empty())
      return;
    const auto &image = content.images.at(b.datum);
    const auto key = animation(b.animation, b.delay);
    unsigned page = 0;
    UiSprite sprite;
    const auto f = std::max(0, frame < 0 ? b.frame : frame);
    if (!image.sequences.empty() && !image.sequences.front().empty()) {
      const auto &sequence = image.sequences.front();
      const auto index =
          (unsigned(f) + unsigned(std::max(0.f, std::floor(seconds * b.fps)))) %
          sequence.size();
      sprite = sequence[index];
      page = sprite.image;
    } else {
      page = std::min(unsigned(f), unsigned(image.images.size() - 1));
      sprite.image = page;
    }
    const auto &texture = image.images.at(page);
    const float width = texture.width * (sprite.right - sprite.left),
                height = texture.height * (sprite.bottom - sprite.top);
    const float sx = b.scale.x ? b.scale.x : 1, sy = b.scale.y ? b.scale.y : 1;
    const float x = 320 + parent.x + b.position.x + key.position.x -
                    sprite.registration.x * width;
    const float y = 240 - parent.y - b.position.y - key.position.y -
                    sprite.registration.y * height;
    const float u = seconds * b.scroll.x, v = seconds * b.scroll.y;
    auto color = tint.value_or(UiColor{});
    color.alpha *= alpha * key.alpha;
    if (tint) color = appearance_namebar_pixel(color);
    auto material = images.at(b.datum).at(page);
    if (online_vista_art && vista_art.contains(image.name)) {
      const auto &donor= vista_art.at(image.name);
      if (image.name=="ui\\global_bitmaps\\fs_dialog" || image.name=="ui\\list_skins\\default\\arrows" ||
          image.name=="ui\\screens\\game_shell\\pregame_lobby\\lobby_display" ||
          image.name=="ui\\screens\\game_shell\\pregame_lobby\\mp_games2" ||
          image.name=="ui\\screens\\game_shell\\pregame_lobby\\button" ||
          image.name=="ui\\screens\\game_shell\\pregame_lobby\\settings_framing" ||
          image.name.starts_with("ui\\screens\\game_shell\\virtual_keyboard\\button_") ||
          (image.name=="ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\xbox_live_menu" && page<4))
        // PC63FF21/63ADB1: keyboard0 normal,1 selected/latched,2 hover.
        // The native keyboard uses0/1; retain the actual PC pages and padding.
        material=donor.at(page);
    }
    float draw_height = height * sy, bottom_v = sprite.bottom + v;
    if (clip_bottom && y + draw_height > *clip_bottom) {
      const float visible = std::max(0.f, *clip_bottom - y);
      bottom_v = sprite.top + v + (sprite.bottom - sprite.top) * visible / draw_height;
      draw_height = visible;
    }
    quad(x, y, width * sx, draw_height, material,
         color, sprite.left + u, sprite.top + v,
         sprite.right + u, bottom_v);
    if (const auto edges = frame_extensions.find(material); edges != frame_extensions.end()) {
      const float canvas_scale = content.source_fonts.empty() ? 1.f : .4f;
      const float scaled_width = width * sx * canvas_scale;
      const auto sampling=frontend_frame_sampling(320 + (x - 320) * canvas_scale,scaled_width,
          sprite.left+u,sprite.right+u,mesh.materials.at(material).diffuse.width);
      for (unsigned edge = 0; edge < edges->second.size(); ++edge) {
        const auto edge_u=edge ? sampling.right_u : sampling.left_u;
        // Share admission and the inward boundary sample with the center's
        // native extension route; the authored quad above remains unchanged.
        if (!edge_u) continue;
        quad(320 - 320 / canvas_scale, y, 640 / canvas_scale, height * sy, edges->second[edge], color,
             *edge_u, sprite.top + v, *edge_u, sprite.bottom + v);
      }
    }
  }
  void emblem(const UiEmblem &e, Vec2 parent,
               const FrontendAppearance &appearance, bool group = false,
               std::optional<unsigned> instance_material = {}) {
    // 2B12CA disables definition flag2 when the sequence override is NONE.
    // Frontend profile appearance layouts supply that exact NONE override.
    if ((e.flags & 2) || !emblem_materials.contains(&e)) return;
    const auto key = animation(e.animation + 1, e.delay);
    const auto material = instance_material.value_or(emblem_materials.at(&e));
    // 2B11C9 writes widget scale to g4E696C; its authored pixel overlay
    // applies opacity. D3D immediate v9 stays opaque white.
    update_frontend_emblem(mesh.materials.at(material), appearance,
                           content.appearance_palette, key.alpha);
    const auto &b = e.bounds;
    // 2BACBC adds abs(bottom-top) to both group-emblem Y bounds before
    // adding the grid origin, just as it places group bitmap bottom edges.
    const float group_y = group ? std::abs(b.bottom - b.top) : 0.f;
    quad(320 + parent.x + key.position.x + b.left,
          240 - parent.y - key.position.y - b.top - group_y,
          b.right - b.left, b.top - b.bottom, material);
  }
  const RetailFont &font(int index) const {
    return fonts.at(index >= 0 && index < int(fonts.size()) ? unsigned(index)
                                                            : 1u);
  }
  float measure(std::span<const unsigned> chars, int index, float scale) const {
    const auto &f = font(index);
    float width = 0;
    unsigned previous = 0;
    for (auto c : chars) {
      if (const auto it = f.glyphs.find(c); it != f.glyphs.end()) {
        // 13EF40/13F700 pass previous in EDI and current in EBX to 122570.
        const auto kern = f.kerning.find({previous, c});
        const float kerning = kern == f.kerning.end() ? 0.f : kern->second * scale;
        // 13EF40 includes the horizontal bearing in each cumulative advance.
        width = it->second.advance * scale + it->second.bearing_x * scale +
                kerning + width;
      }
      previous = c;
    }
    return width;
  }
  void text(UiText t, const std::string &value, Vec2 parent = {},
            float alpha = 1, bool button = false, bool uppercase = false,
            const MenuFrame *owner = nullptr, const UiText *definition = nullptr,
            bool prompts = false, const Impl *font_owner = nullptr) {
    auto chars = codepoints(value);
    if (!button && (t.flags & 8) && owner &&
        (is_vista_menu(owner->screen) ||
         frontend_text_widgets_recreated_on_focus(owner->screen))) {
      if (!definition) throw FormatError("typewriter text has no source widget identity");
      auto &widget = typing_widgets[{owner->instance, owner->pane_generation}][definition];
      auto copied = standard_text_buffer(value);
      if (!widget.initialized || widget.text != copied) {
        frontend_typewriter_set_text(widget.state, copied);
        if (!widget.initialized) frontend_typewriter_begin(widget.state);
        widget.text = std::move(copied);
        widget.initialized = true;
      }
      // 22E3B4 admits the widget only after its authored delay. 2538F5
      // then calls22CD48 before alpha or raster clipping can discard it.
      const auto end = owner->pane_created_clock + std::uint32_t(t.delay);
      if (end > ui_clock_ms) return;
      const auto displayed = frontend_typewriter_draw(widget.state, widget.text,
          ui_clock_ms, content.typewriter.interval_ms, content.typewriter.cursor,
          t.bounds.left != t.bounds.right && t.bounds.top != t.bounds.bottom);
      chars.assign(displayed.begin(), displayed.end());
      std::erase(chars, unsigned('\r'));
    }
    if (chars.empty())
      return;
    const auto key = animation(t.animation, t.delay);
    alpha *= key.alpha;
    if (alpha <= 0)
      return;
    const auto setup = frontend_text_setup(t.flags, t.font, button,
                                           content.typewriter.scale);
    const auto &raster = font_owner ? *font_owner : *this;
    const auto &f = raster.font(setup.font);
    const unsigned material = raster.font_materials.at(
        setup.font >= 0 && setup.font < int(raster.fonts.size()) ? unsigned(setup.font) : 1u);
    const float scale = setup.scale;
    const float left = 320 + parent.x + t.bounds.left + key.position.x,
                right = 320 + parent.x + t.bounds.right + key.position.x;
    const float top = 240 - parent.y - t.bounds.top - key.position.y,
                bottom = 240 - parent.y - t.bounds.bottom - key.position.y;
    if (right <= left || bottom <= top)
      return;
    UiColor color = t.color;
    color.alpha = alpha;
    if (t.flags & 4)
      color.alpha = frontend_lobby_warning_alpha(color.alpha, ui_clock_ms);
    color = frontend_text_color(color);
    const auto pixel = frontend_text_pixel(color);
    const auto shadow = frontend_text_pixel_color(
        setup.shadow ? pixel & 0xff000000u : 0);
    if (uppercase)
      for (auto &c : chars) {
        // 22FDA6 sets buffer flag2; 22CD48 then calls literal 13FAE0.
        if (c >= 'a' && c <= 'z') c -= 0x20;
        else if (c == 0xe9) c = 0xc9;
        else if (c == 0xf3) c = 0xd3;
      }
    std::size_t start = 0;
    unsigned line_index = 0;
    std::optional<FrontendPointerAction> prompt_action;
    // The title's authored Press Start caption is plain text (0E001811),
    // rather than a controller glyph. Keep its native target on that caption.
    if (prompts && owner && owner->screen == 9 && t.string == 0x0e001811)
      prompt_action = FrontendPointerAction::Accept;
    auto action_for = [&](unsigned c) -> std::optional<FrontendPointerAction> {
      using Action = FrontendPointerAction;
      switch (c) {
      case 0xe100: return Action::Accept;
      case 0xe101: case 0xe10d: return Action::Back;
      case 0xe102: return owner && is_vista_settings(owner->screen, 253)
          ? Action::ClearBinding : Action::Auxiliary;
      case 0xe103: return Action::Players;
      case 0xe10c: return owner && owner->screen == 167 ? Action::CommitText : Action::Accept;
      default: return {};
      }
    };
    auto action_at = [&](std::size_t i) -> std::optional<FrontendPointerAction> {
      if (auto action = action_for(chars[i])) return action;
      // These captions are authored by the native binding editor itself.
      // Associate explicit commands with their text spans, never arbitrary
      // user strings or English labels elsewhere in the original menus.
      if (owner && is_vista_settings(owner->screen, 253) && definition && value == owner->description) {
        using Action = FrontendPointerAction;
        constexpr std::pair<std::string_view, Action> captions[]{
            {"Enter:", Action::Accept}, {"X:", Action::Auxiliary},
            {"Backspace:", Action::ClearBinding}, {"Esc:", Action::Back}};
        for (const auto &[caption, action] : captions)
          if (i + caption.size() <= chars.size() &&
              std::equal(caption.begin(), caption.end(), chars.begin() + i)) return action;
      }
      return {};
    };
    const int line_height = f.ascent + f.descent + f.leading;
    while (start < chars.size()) {
      // 13F0E0 recomputes from the unscaled line index; repeated addition
      // of a scaled line height changes fractional-scale float words.
      const float baseline = top + float(f.ascent + line_height * line_index) * scale;
      if (baseline >= bottom) break;
      const auto line_start = start;
      std::size_t end = start, last_break = start;
      float width = 0;
      while (end < chars.size() && chars[end] != '\n') {
        // 13FD20 allows this ASCII word break after the preceding space.
        // Keep that space in the first line's original glyph traversal.
        if (end > start && chars[end - 1] == ' ')
          last_break = end;
        width = raster.measure(std::span(chars).subspan(start, end - start + 1),
                        setup.font, scale);
        // 13EF40 requires the right edge to be strictly greater than the
        // candidate endpoint. The description's space after "to" is exactly
        // at its 517-pixel edge and must return to the preceding word break.
        if (width >= right - left && end > start) {
          if (last_break > start)
            end = last_break;
          break;
        }
        ++end;
      }
      width =
          raster.measure(std::span(chars).subspan(start, end - start), setup.font, scale);
      float x = (t.flags & 1)   ? left
                : (t.flags & 2) ? right - width
                                : (left + right - width) * .5f;
      unsigned previous = 0;
      std::optional<Hit> prompt;
      auto submit_prompt = [&] {
        if (prompt) {
          // Preserve spaces inside a label, but keep the gap between adjacent
          // prompts inert. Vertical padding stays inside the source text box.
          prompt->top = std::max(top, prompt->top - 2);
          prompt->bottom = std::min(bottom, prompt->bottom + 2);
          hits.push_back(*prompt);
          prompt.reset();
        }
      };
      for (std::size_t i = start; i < end; ++i) {
        const auto c = chars[i];
        if (prompts)
          if (auto action = action_at(i)) {
            submit_prompt();
            prompt_action = action;
          }
        const auto it = f.glyphs.find(c);
        if (it == f.glyphs.end()) {
          previous = c;
          continue;
        }
        const auto &g = it->second;
        // 13F700 creates a fresh iterator for each line. 13F5A0 stores
        // previous_token from the old token, so its first TWO characters
        // have previous_character0; later characters retain the predecessor.
        const auto kern = f.kerning.find({i == line_start + 1 ? 0u : previous, c});
        const float kerning = kern == f.kerning.end() ? 0.f : kern->second * scale;
        const float offset = g.bearing_x * scale;
        float gx = x + offset + kerning, gy = baseline - g.bearing_y * scale;
        float source_width = float(g.width), source_height = float(g.height);
        float source_x = 0, source_y = 0;
        // 13F700 clips the unscaled ink extent at the scaled glyph origin.
        // 1F490 then truncates the source rectangle before1F3A0 scales it.
        if (gx + source_width > right) source_width = right - gx;
        if (gx < left) {
          source_x = left - gx;
          gx = left;
          source_width -= source_x;
        }
        if (gy + source_height > bottom) source_height = bottom - gy;
        if (gy < top) {
          source_y = top - gy;
          gy = top;
          source_height -= source_y;
        }
        if (source_width > 0 && source_height > 0) {
          const auto iw = static_cast<std::int32_t>(source_width),
                     ih = static_cast<std::int32_t>(source_height);
          const auto ix = static_cast<std::int16_t>(static_cast<std::int32_t>(source_x)),
                     iy = static_cast<std::int16_t>(static_cast<std::int32_t>(source_y));
          const float width = float(iw) * scale, height = float(ih) * scale;
          const auto uv=retail_font_sample_uvs(f,c,ix,iy,iw,ih);
          const auto [u0,v0,u1,v1]=uv;
          // 1F490 shifts the already clipped glyph for its shadow, without
          // reclipping, and submits it before the foreground. 13F700 retains
          // atlas RGB for eligible controller/private-use glyphs.
          quad(gx + scale, gy + scale, width, height, material, shadow,
               u0, v0, u1, v1);
          const auto foreground = frontend_text_pixel_color(
              frontend_text_texture_color(c) ? pixel | 0xffffffu : pixel);
          quad(gx, gy, width, height, material, foreground, u0, v0, u1, v1);
          if (prompts && prompt_action && foreground.alpha > 0 &&
              c != ' ' && width > 0 && height > 0) {
            if (!prompt) prompt = Hit{gx, gy, gx + width, gy + height, 0, *prompt_action};
            else {
              prompt->left = std::min(prompt->left, gx);
              prompt->top = std::min(prompt->top, gy);
              prompt->right = std::max(prompt->right, gx + width);
              prompt->bottom = std::max(prompt->bottom, gy + height);
            }
          }
        }
        x = g.advance * scale + x + offset + kerning;
        previous = c;
      }
      submit_prompt();
      ++line_index;
      start = end;
      if (start < chars.size() && (chars[start] == ' ' || chars[start] == '\n'))
        ++start;
      if (start == end && end == chars.size())
        break;
      if (start == line_start && start < chars.size())
        ++start;
    }
  }
  template<class F> void cells(const UiGrid &grid, Vec2 origin, F visit) const {
    const auto rows = std::max(1u, grid.rows), columns = std::max(1u, grid.columns);
    for (unsigned index = 0; index < grid.count; ++index) {
      const auto column = grid.order ? index / rows : index % columns;
      const auto row = grid.order ? index % rows : index / columns;
      if (column < columns && row < rows)
        visit(Vec2{origin.x + grid.position.x + float(column) * grid.step.x,
                   origin.y + grid.position.y + float(row) * grid.step.y});
    }
  }
  UiRect skin_bounds(const UiSkin &skin, unsigned depth = 0) const {
    if (depth > 16)
      throw FormatError("cyclic frontend skin group");
    UiRect bounds{-32768, 32767, 32767, -32768};
    auto extend = [&](UiRect b) {
      bounds.top = std::max(bounds.top, b.top);
      bounds.left = std::min(bounds.left, b.left);
      bounds.bottom = std::min(bounds.bottom, b.bottom);
      bounds.right = std::max(bounds.right, b.right);
    };
    for (const auto &t : skin.texts)
      extend(t.bounds);
    // 2BACBC places group bitmaps by their bottom edge; 2BAFA4 excludes
    // definition flag 1 from layout bounds, independently of runtime visibility.
    for (const auto &b : skin.bitmaps)
      if (!(b.flags & 1) && content.images.contains(b.datum) &&
          !content.images.at(b.datum).images.empty()) {
        const auto &image = content.images.at(b.datum).images.front();
        extend({b.position.y + image.height, b.position.x,
                b.position.y, b.position.x + image.width});
      }
    for (const auto &e : skin.emblems)
      if (!(e.flags & 1)) {
        const auto &b = e.bounds;
        const float height = std::abs(b.bottom - b.top);
        extend({b.top + height, b.left, b.bottom + height, b.right});
      }
    for (const auto &grid : skin.grids)
      if (content.skin_groups.contains(grid.skin)) {
        const auto b = skin_bounds(content.skin_groups.at(grid.skin), depth + 1);
        cells(grid, {}, [&](Vec2 p) {
          extend({b.top + p.y, b.left + p.x, b.bottom + p.y, b.right + p.x});
        });
      }
    if (bounds.right < bounds.left || bounds.top < bounds.bottom)
      return {};
    return bounds;
  }
  void profile_group(const UiSkin &skin, Vec2 p, float opacity,
                     const MenuEntry &entry,
                     const std::optional<FrontendAppearance> &appearance,
                     unsigned depth = 0) {
    if (depth > 16)
      throw FormatError("cyclic frontend skin group");
    const bool create = entry.destination == 167;
    for (auto b : skin.bitmaps) {
      // 24A09B -> 2AFEAE: Create New supplies value5e; each saved row
      // supplies its own profile appearance, independently of focus/sign-in.
      if (b.placeholder == 0x0e00069b && !create)
        continue;
      if ((b.placeholder == 0x1400069a && !appearance) ||
          b.placeholder == 0x0900069c)
        continue;
      b.animation = 0;
      b.delay = 0;
      if (content.images.contains(b.datum) &&
          !content.images.at(b.datum).images.empty())
        b.position.y += content.images.at(b.datum).images.front().height;
      if (b.placeholder == 0x1400069a) {
        const auto rgb = frontend_appearance_colors(
            *appearance, content.appearance_palette)[0];
        bitmap(b, p, opacity, 0, UiColor{1, rgb.x, rgb.y, rgb.z});
      } else {
        bitmap(b, p, opacity);
      }
    }
    for (const auto &grid : skin.grids)
      if (content.skin_groups.contains(grid.skin))
        cells(grid, p, [&](Vec2 origin) {
          profile_group(content.skin_groups.at(grid.skin), origin, opacity, entry,
                        appearance, depth + 1);
        });
    // 2B0080 copies a saved name into the nested field and applies the
    // original primary-color HSV conversion. Create New uses the outer field.
    if (appearance && !create)
      for (auto t : skin.texts)
        if (t.string == 0x0b000698) {
          const auto rgb = frontend_appearance_name_color(
              *appearance, content.appearance_palette);
          t.color = {1, rgb.x, rgb.y, rgb.z};
          t.animation = 0;
          t.delay = 0;
          text(t, entry.label, p, opacity);
        }
  }
  void profile_emblems(const UiSkin &skin, Vec2 p, float opacity,
                       const FrontendAppearance &appearance,
                       const std::vector<unsigned> &materials,
                       unsigned &instance, unsigned depth = 0) {
    if (depth > 16)
      throw FormatError("cyclic frontend skin group");
    for (const auto &e : skin.emblems)
      if (content.emblem_shaders.contains(e.shader)) {
        const auto material = materials.at(instance++);
        //24A09B supplies a saved appearance with NONE/NONE overrides.
        //2B12CA/2B11C9 reject flag2, while flag1 only excludes union bounds.
        if (e.flags & 2) continue;
        update_frontend_emblem(mesh.materials.at(material), appearance,
                               content.appearance_palette, opacity);
        //2BACBC group placement and24AC80→22E930 focus already supply p.
        //Do not replay this child's replaced authored11/250ms intro.
        const auto &b = e.bounds;
        const float height = std::abs(b.bottom - b.top);
        quad(320 + p.x + b.left, 240 - p.y - b.top - height,
             b.right - b.left, b.top - b.bottom, material);
      }
    for (const auto &grid : skin.grids)
      if (content.skin_groups.contains(grid.skin))
        cells(grid, p, [&](Vec2 origin) {
          profile_emblems(content.skin_groups.at(grid.skin), origin, opacity,
                          appearance, materials, instance, depth + 1);
        });
  }
  void pcr_list(const UiPane &pane,const MenuFrame &frame,bool interactive) {
    const auto &owner=*frame.vista_pcr;
    if(pane.lists.size()!=1 || frame.context>=6)
      throw FormatError("Vista PCR requires its actual single-list pane");
    const auto &list=pane.lists.front();
    const auto &skin=content.skins.at(list.skin);
    const bool teams=frame.context==0;
    const auto count=teams?owner.results->team_count:owner.results->player_count;
    const auto bounds=skin_bounds(skin);
    const auto pitch=bounds.top-bounds.bottom;
    if(!(pitch>0) || count>list.visible || frame.entries.size()!=count)
      throw FormatError("Vista PCR retained rows do not fit the actual source list");
    for(unsigned row=0;row<count;++row) {
      const auto focus=skin.animations.empty()?UiKey{}:interpolate(
          skin.animations.at(row==frame.selected?0:std::min(2u,unsigned(skin.animations.size()-1))),1000);
      const Vec2 origin{list.position.x+focus.position.x,
                        list.position.y-float(row)*pitch+focus.position.y};
      const UiSkin *fields=&skin;Vec2 field_origin=origin;
      if(!teams) {
        if(skin.grids.empty() || !content.skin_groups.contains(skin.grids.front().skin) ||
            skin.grids.front().count!=1)
          throw FormatError("Vista PCR player row has no actual group zero");
        fields=&content.skin_groups.at(skin.grids.front().skin);
        field_origin.x+=skin.grids.front().position.x;
        field_origin.y+=skin.grids.front().position.y;
      }
      struct Field {std::u16string text;bool visible=true;std::optional<UiColor> color;};
      std::vector<Field> texts(fields->texts.size());
      std::vector<Field> bitmaps(fields->bitmaps.size());
      std::optional<VistaPcrPlayerPresentation> player;
      auto services=owner.rows;
      services.grid_child=[&](unsigned index){return index==0 && !teams && !skin.grids.empty();};
      services.text_child=[&](unsigned index){return index<texts.size();};
      services.bitmap_child=[&](unsigned index){return index<bitmaps.size();};
      services.text=[&](unsigned index,std::u16string_view value){texts.at(index).text=value;};
      services.visible=[&](unsigned index,bool visible){(teams?bitmaps:texts).at(index).visible=visible;};
      services.text_color=[&](unsigned index,const auto &color){texts.at(index).color=pcr_color(color);};
      services.bitmap_color=[&](unsigned index,const auto &color){bitmaps.at(index).color=pcr_color(color);};
      services.appearance=[&](unsigned index){
        if(!owner.appearance)throw FormatError("Vista PCR has no consumed PC appearance owner");
        player=owner.appearance(index);
        for(unsigned i=0;i<fields->texts.size();++i)
          if(fields->texts[i].string==0x0b000696 && i==0)
            texts[i].color=pcr_color(player->name_color);
      };
      vista_pcr_bind_row(VistaPcrPane(frame.context),row,*owner.results,services,owner.floating_point);
      const auto draw_bitmap=[&](UiBitmap bitmap,Vec2 p,int image,std::optional<UiColor> color) {
        //61C300 changes the page; group bitmap origins retain their actual
        //bottom edge. Row focus replaces each descendant animation.
        bitmap.animation=bitmap.delay=0;
        if(content.images.contains(bitmap.datum) && !content.images.at(bitmap.datum).images.empty())
          bitmap.position.y+=content.images.at(bitmap.datum).images.front().height;
        this->bitmap(bitmap,p,focus.alpha,image,color);
      };
      // Outer PCR row bitmaps belong to the list focus owner. The zero-string
      //outer field has no assignment in40D32E..40DA4E; names live in group0.
      if(!teams)for(unsigned i=0;i<skin.bitmaps.size();++i) {
        const auto &bitmap=skin.bitmaps[i];
        if(i==0) {
          if(frame.screen==vista_menu(185))continue;
          if(!owner.voice) {
            constexpr const char *gap="Vista PCR voice bitmap requires the genuine PC XUID/voice owner";
            if(std::find(output.rendering_gaps.begin(),output.rendering_gaps.end(),gap)==output.rendering_gaps.end())
              output.rendering_gaps.emplace_back(gap);
            continue;
          }
          const auto voice=owner.voice(row);
          if(voice.visible)draw_bitmap(bitmap,origin,int(voice.frame),{});
        } else draw_bitmap(bitmap,origin,(bitmap.flags&2)?(row==frame.selected?0:row<frame.selected?1:2):bitmap.frame,{});
      }
      for(unsigned i=0;i<fields->bitmaps.size();++i) {
        const auto &bitmap=fields->bitmaps[i];bool visible=bitmaps[i].visible;
        auto color=bitmaps[i].color;int image=bitmap.frame;
        if(player) {
          switch(bitmap.placeholder) {
          case 0x14000698:visible=player->primary_visible;image=int(player->primary_frame);color=pcr_color(player->primary_color);break;
          case 0x0e000699:visible=player->secondary_visible;break;
          case 0x0900069a:visible=player->rank_visible;image=int(player->rank_frame);break;
          default:throw FormatError("Unbound Vista PCR player bitmap placeholder");
          }
        } else if(bitmap.flags&2)image=row==frame.selected?0:row<frame.selected?1:2;
        if(visible)draw_bitmap(bitmap,field_origin,image,color);
      }
      if(player && player->emblem_visible && !fields->emblems.empty()) {
        // The PC spas definition and full16 appearance bytes are retained.
        // The Xbox waypoint pass has a different bytecode owner and cannot
        //render this PC emblem without a recovered PC shader implementation.
        constexpr const char *gap="Vista PCR PC emblem shader is retained but its native spas pass is unresolved";
        if(std::find(output.rendering_gaps.begin(),output.rendering_gaps.end(),gap)==output.rendering_gaps.end())
          output.rendering_gaps.emplace_back(gap);
      }
      for(unsigned i=0;i<texts.size();++i)if(texts[i].visible) {
        auto definition=fields->texts[i];definition.animation=definition.delay=0;
        if(texts[i].color)definition.color=*texts[i].color;
        text(definition,pcr_utf8(texts[i].text),field_origin,focus.alpha);
      }
      if(interactive && focus.alpha>0)
        hits.push_back({320+origin.x+bounds.left,240-origin.y-bounds.top,
            320+origin.x+bounds.right,240-origin.y-bounds.bottom,row});
    }
  }
  void pane(const UiScreen &screen, const UiPane &pane, const MenuFrame &frame,
            const MenuState &state, bool interactive) {
    const bool network_browser = state.screen_layout(frame) == vista_menu(13);
    // 22E9C6/2538F5 read each widget's own animation offset/scale. A pane
    // starts its descendants' animations but does not transform their bounds.
    // Grid/list origins below are explicit placement, not inherited animation.
    const Vec2 parent{};
    // These four authored child panes have no model scenes. Reorder only
    // their contiguous direct bitmap/text submissions, before list rows, so
    // Scene.before_vertex and grid/list ordering are not changed.
    const bool appearance_popup = pane.scenes.empty() &&
        (frame.screen == 49 || frame.screen == 50 || frame.screen == 52 ||
         frame.screen == 233);
    std::vector<WidgetSubmission> popup_widgets;
    std::optional<FrontendSignInPresentation> signin;
    const auto lobby = frame.screen == 14 ? state.lobby_view() : std::nullopt;
    const auto setup_view = frame.screen == 23 ? state.lobby_view() : std::nullopt;
    std::optional<FrontendLobbySetupDetail> setup;
    const auto focused_map = state.focused_lobby_map(frame);
    if (setup_view)
      setup = frontend_lobby_setup_detail({
          frontend_lobby_setup_focused_semantic(setup_view->input.session_enum, int(frame.selected)),
          frontend_lobby_variant_type(setup_view->engine), int(setup_view->difficulty),
          setup_view->map != nullptr, setup_view->map != nullptr});
    std::optional<FrontendLobbyPresentation> lobby_policy;
    std::optional<FrontendLobbyRoster> roster;
    if (lobby) {
      lobby_policy = frontend_lobby_presentation(lobby->input);
      if(lobby->party_names) {
        // Accepted native membership owns names and the opted-in voice
        // channel owns speaker state. Unavailable profile/rank art stays hidden.
        roster=FrontendLobbyRoster{};
        for(unsigned i=0;i<roster->size();++i) {
          auto &row=(*roster)[i];row.text_a=27+i;row.text_b=11+i;
          if(i<lobby->party_names->size()) {
            row.visible=true;row.name=lobby->party_names->at(i);row.name_color={1,1,1};
            row.speaker_visible=lobby->party_speakers.at(i)<=2;row.speaker_frame=lobby->party_speakers.at(i);
          }
        }
      } else {
      std::vector<FrontendLobbyRosterPlayer> players;
      for(const auto& player:state.local_players())
        players.push_back({int(player.controller),player.name,player.appearance,
                           player.team,-1,0,false,false});
      FrontendLobbyRosterInput input;
      input.pane = lobby_policy->pane;
      input.valid_session = 1;
      input.session_present = true;
      input.team_mode = lobby->input.team_mode;
      input.players = players;
      input.local_teams[0] = {0, lobby->team_edit.valid, lobby->team_edit.team};
      input.appearance_palette = content.appearance_palette;
      roster = frontend_lobby_roster(input);
      }
    }
    if (frame.screen == 30) {
      std::array<FrontendSignInSlot, 4> slots{};
      for(const auto& player:state.local_players())
        slots[player.controller]={true,true,false,0,player.appearance.species};
      signin = frontend_signin_presentation(slots);
    }
    std::vector<const UiBitmap *> ordered_bitmaps;
    const bool pc_link = content.vista &&
        ((frame.screen == 19 && frame.selected == 2) ||
         (frame.screen == 38 && frame.selected == 6));
    for (const auto &b : pane.bitmaps)
      if (!pc_link) ordered_bitmaps.push_back(&b);
    // 2B01EB copies definition+20 to widget+6A. Original 2356D8 orders
    // same-depth bitmap widgets by ascending layer; the keyboard dialog
    // (layer0) must precede its authored input field (layer1). Restrict this
    // correction to one same-depth group, avoiding a nontransitive epsilon
    // comparison as a general std::sort ordering for animated depths.
    float nearest = std::numeric_limits<float>::max(), farthest = -nearest;
    for (const auto *b : ordered_bitmaps) {
      const auto depth = animation(b->animation, b->delay).position.z;
      nearest = std::min(nearest, depth);
      farthest = std::max(farthest, depth);
    }
    if (farthest - nearest <= .001f)
      std::stable_sort(ordered_bitmaps.begin(), ordered_bitmaps.end(),
          [](const UiBitmap *x, const UiBitmap *y) { return x->layer < y->layer; });
    for (const auto *b : ordered_bitmaps) {
      const auto index = std::size_t(b - pane.bitmaps.data());
      // PC13's off-canvas player placeholder has no native directory owner.
      if (network_browser && index == 20) continue;
      // PC63E9A0 hides child4 for the active local-controller mask.
      if(frame.screen==vista_menu(18) && index==4)continue;
      if (frame.screen == 26 && frame.context == 1 && index == 2) continue;
      if (online_vista_art && frame.screen==186 &&
          (index==1 || (index==3 && frame.selected==3))) continue;
      if (frame.screen==210 && content.images.contains(b->datum)) {
        const auto &name=content.images.at(b->datum).name;
        // The native queue owns status text, not the legacy pane's static
        // Zanzibar preview or an unbound quick-options variant descriptor.
        if(name=="ui\\screens\\game_shell\\settings_screen\\variant_settings\\quick_options_display" ||
           name.starts_with("ui\\global_bitmaps\\map_shots\\"))continue;
      }
      // Native social tabs have no single playlist rank or remote headset
      // state for the authored static rank/headset/session placeholders.
      if ((frame.screen == 26 && index >= 4) ||
          (frame.screen == 211 && frame.context == 41 && index >= 5)) continue;
      if (signin && index >= 3 && index <= 10 && !signin->bitmaps[index-3])
        continue;
      // Original24A886 ->22EE47 hides direct bitmap6 for any saved
      // profile, independently of whether either model species is admitted.
      // CreateNew restores this authored unknown-player preview.
      if (frame.screen == 193 && index == 6 &&
          state.focused_profile_appearance(frame))
        continue;
      if (roster && index >= 5 && index <= 20) continue;
      if(lobby && lobby->party_names && (index==2 || index==4))continue;
      if (roster && index >= 21 && index <= 36 &&
          !(*roster)[index-21].speaker_visible) continue;
      auto image = *b;
      if (frame.screen == 26 && frame.context == 1 && index == 3)
        image.position.x += clan_width_extra();
      //6192B3 uses page10 when there is no source game-type icon owner.
      if (network_browser && index == 4) image.frame = 10;
      if(online_vista_art && frame.screen==186 && index==3) {
        // Vista's actual ONLINE screen247 uses these action pages. Its stale
        // screen186 still refers to absent page7; never alias the news panel.
        // The native Discord account action has no authored PC action icon.
        constexpr unsigned pages[]{0,2,3,0,1};
        image.frame=int(pages[std::min(frame.selected,4u)]);
      }
      if (focused_map && index == 1) {
        image.datum = focused_map->bitmap;
        image.frame = 0;
      }
      if (setup && index == 4) {
        image.frame = setup->authored_frame;
        if ((setup->preview == FrontendLobbySetupPreview::Map ||
             setup->preview == FrontendLobbySetupPreview::CampaignLevel) && setup_view->map) {
          image.datum = setup_view->map->bitmap;
          image.frame = 0;
        } else if (setup->descriptor_bank && setup->descriptor_frame &&
                   unsigned(*setup->descriptor_bank) < screen.bitmap_resources.size()) {
          image.datum = screen.bitmap_resources[*setup->descriptor_bank];
          image.frame = *setup->descriptor_frame;
        }
      }
      if (lobby && index == 2 && lobby->map) image.datum = lobby->map->bitmap;
      if (lobby && index == 4 && lobby_policy->pane == 0)
        image.frame = frontend_lobby_game_icon(
            frontend_lobby_variant_type(lobby->engine).value_or(-1));
      if (lobby && index == 4 && lobby_policy->pane == 1)
        image.frame = int(lobby->difficulty);
      if (roster && index >= 21 && index <= 36)
        image.frame = int((*roster)[index-21].speaker_frame);
      if (signin && index >= 7 && index <= 10)
        image.frame = signin->controller_frames[index-7];
      const auto first = output.vertices.size();
      std::optional<float> clip_bottom;
      if (frame.screen == 26 && frame.context == 1 && (index == 0 || index == 3)) {
        // The source Y-menu body is512 high, with visible alpha to409.
        // At its authored y74 it reaches483, below the native480 canvas.
        // Finish this standalone clan panel beneath its actual footer,
        // cropping UVs with the geometry so the source art is not stretched.
        const auto &footer = pane.texts.at(1).bounds;
        const auto &metadata = pane.texts.at(2).bounds;
        clip_bottom = 240 - footer.bottom + (metadata.top - metadata.bottom) * .5f;
      }
      bitmap(image, parent, 1, -1, {}, clip_bottom);
      if (frame.screen == 26 && frame.context == 1) {
        const float body_extra = canvas.surface_height / canvas.scale - 424;
        if (index == 0) extend_bitmap(first, {clan_width_extra(), body_extra});
        if (index == 1) extend_bitmap(first, {clan_width_extra(), 0}, 209, 4);
        if (index == 3) extend_bitmap(first, {0, body_extra});
      }
      if (frame.screen == 167)
        keyboard_widget(first, b->layer, 8,
                        animation(b->animation, b->delay).position.z);
      if (appearance_popup && output.vertices.size() > first)
        popup_widgets.push_back({first, output.vertices.size(), b->layer, 8,
                                animation(b->animation, b->delay).position.z});
    }
    if(pc_link && frame.screen==38 && vista) {
      vista->output={};vista->seconds=seconds;vista->ui_clock_ms=ui_clock_ms;
      const auto &pc_pane=content.vista->screen(38).panes.at(2);
      for(const auto &b:pc_pane.bitmaps)vista->bitmap(b);
      for (std::size_t i = 1; i < pc_pane.texts.size(); ++i) {
        const auto &t = pc_pane.texts[i];
        vista->text(t, state.expand(content.vista->localized(content.vista->screen(38), t.string)),
                    {}, 1, false, false, &frame, &t);
      }
      for(auto vertex:vista->output.vertices) {
        vertex.position.x=320+(vertex.position.x-320)*.4f;
        vertex.position.y=240+(vertex.position.y-240)*.4f;
        output.vertices.push_back(vertex);
      }
    }
    for (unsigned i = 0; i < pane.scenes.size(); ++i) {
      const auto &scene = pane.scenes[i];
      if (signin && (i >= signin->models.size() || !signin->models[i]))
        continue;
      auto appearance = state.model_appearance(frame);
      if(signin)for(const auto& player:state.local_players())
        if(i%4==player.controller)appearance=player.appearance;
      const bool elite = (appearance ? appearance->species : state.appearance().species) == 1;
      if (!scene.objects.empty() && scene.objects[0].starts_with("ui_player")) {
        const auto &name = scene.objects[0];
        const bool is_elite = name.ends_with("b") || name.ends_with("d");
        if (frame.screen == 193) {
          const auto species = state.focused_profile_species(frame);
          if (!species || *species != (is_elite ? 1 : 0))
            continue;
        } else if (is_elite != elite)
          continue;
      }
      output.scenes.push_back({&scene, output.vertices.size(),
                              animation(scene.animation, scene.delay).position,
                              appearance});
    }
    for (unsigned i = 0; i < pane.texts.size(); ++i) {
      if ((frame.screen == 20 || frame.screen == 186) && i > 0) continue;
      if (pc_link && i != 0) continue;
      if (signin && (i >= signin->texts.size() || !signin->texts[i]))
        continue;
      if (lobby_policy && lobby_policy->leader_text == i &&
          lobby_policy->leader_visible == false) continue;
      if (lobby_policy && lobby_policy->privacy_text == i &&
          lobby_policy->privacy_visible == false) continue;
      if (roster && std::any_of(roster->begin(), roster->end(), [&](const auto &row) {
            return row.text_a == i || row.text_b == i;
          })) continue;
      if (lobby_policy && lobby_policy->prior_stats_text == i &&
          lobby_policy->prior_stats_visible == false) continue;
      const auto help = lobby ? frontend_lobby_help(lobby_policy->pane,
          lobby->protocol, 1, lobby->input.party_privacy, lobby->input.session_enum)
          : FrontendLobbyHelp{};
      if (help.text_index == i && !help.visible) continue;
      // 24A886 hides runtime text(6,3), authored pane text1, in every case.
      // The focused name is supplied to the separate player-skin group.
      if (frame.screen == 193 && i == 1) continue;
      auto t = pane.texts[i];
      if(frame.vista_pcr) {
        //40CE6A hides the reached empty-report field whenever the retained
        //signed PC player count is positive. PC185 has no such child.
        if(t.string==0x1d001a71 && std::bit_cast<std::int32_t>(frame.vista_pcr->results->player_count)>0)continue;
        const auto headings=vista_pcr_headings(VistaPcrPane(frame.context),frame.vista_pcr->results->variant);
        if(i<headings.size() && headings[i])t.string=*headings[i];
      }
      if (frame.screen == 193 && (i == 2 || i == 3)) {
        // Original24A886 binds runtime text(6,4)/(6,5) from focused
        // record bytes105/104. CreateNew bypasses both saved selectors.
        constexpr std::array<std::uint32_t, 4> thumbsticks{
            0x07000001, 0x080002f5, 0x060002f6, 0x0f0002f7};
        constexpr std::array<std::uint32_t, 4> buttons{
            0x07000001, 0x090003e4, 0x050003e5, 0x0b0003e6};
        const auto control = state.focused_profile_control(frame, i == 2 ? 41 : 40);
        const auto &strings = i == 2 ? thumbsticks : buttons;
        t.string = !control ? 0x07000001
                   : *control >= 0 && *control < 4 ? strings[unsigned(*control)] : 0;
      }
      if (lobby_policy && i == 0)
        t.flags = lobby_policy->status_warning ? t.flags | 4u : t.flags & ~4u;
      auto value = content.localized(screen, t.string);
      if(frame.screen==vista_menu(18)) {
        if(i==0)value=state.game_pause_objectives();
        if(!frame.context)value.clear();
      }
      if(frame.vista_leave_task && i==0)
        value=content.localized(screen,frame.vista_leave_task->status_string);
      if (frame.screen == 20 || frame.screen == 186) value = frame.description;
      if (network_browser) {
        if (i == 0) value = state.online_directory_text(frame, 3);
        if (i == 2) value = state.online_directory_text(frame, 2);
        if (i == 3) value = "Server Name";
        if (i == 9) value = state.online_directory_text(frame, 4);
      } else if (frame.screen == 219 || frame.screen == 237) value = frame.context == online_server_browser_context
          ? state.online_directory_text(frame, i) : state.online_matchmaking_text(frame, i);
      if (frame.screen == 210 && (content.source_fonts.empty() || i == 0))
        value = state.online_matchmaking_text(frame, i);
      if (state.online_clan_screen(frame)) value = state.online_clan_text(frame,i);
      if (frame.screen == 26 && frame.context == 1) {
        // Native membership replaces the source player's rank columns.
        // Give the labelled count and microphone state their whole columns.
        if (i == 2) t.flags &= ~1u;
        if (i == 4) t.bounds.right = pane.texts.at(6).bounds.left;
        if (i == 6) t.bounds.right = pane.texts.at(7).bounds.right;
        if (i == 1 || i >= 4) {
          t.bounds.left += clan_width_extra();
          t.bounds.right += clan_width_extra();
        }
        if (i == 1) {
          t.bounds.top -= clan_footer_offset();
          t.bounds.bottom -= clan_footer_offset();
        }
      }
      if(frame.screen==vista_menu(297) && (t.string&0xffff)==0x979)
        value="Halo 2 Native\r\nVista settings assets: 11081.07.04.30.0934.main";
      if(frame.screen==vista_menu(296) && i==0)value=frame.description;
      if (pc_link && i == 0)
        value = content.vista->global_strings.at(frame.screen == 19 ? 0x210019e2u : 0x220019fcu);
      if (is_vista_settings(frame.screen, 253) && i == 0) value = frame.description;
      if (is_vista_settings(frame.screen, 243) && value.starts_with(" - ")) {
        // These source captions reserve their prefix for the live binding.
        // Only Custom has admitted native bindings; preset tables are pending.
        constexpr std::pair<unsigned, PcAction> captions[]{
            {0x1b3e, PcAction::Forward}, {0x1b3f, PcAction::Back},
            {0x1b40, PcAction::Left}, {0x1b41, PcAction::Right},
            {0x1b42, PcAction::Jump}, {0x1b43, PcAction::Crouch},
            {0x1b44, PcAction::Melee}, {0x1b45, PcAction::Reload},
            {0x1b46, PcAction::SwitchWeapon}};
        std::string binding = "Unavailable";
        if (frame.selected == 3)
          for (auto [sid, action] : captions)
            if ((t.string & 0xffffu) == sid)
              binding = pc_binding_name(state.pc_keyboard_settings().bindings[unsigned(action)]);
        value = binding + value;
      }
      if (focused_map && i == 0) value = focused_map->description;
      if (setup && i < setup->strings.size())
        value = content.localized(screen, setup->strings[i]);
      if (lobby_policy) {
        if (i == 0 && lobby_policy->status_string)
          value = content.localized(screen, *lobby_policy->status_string);
        if (lobby_policy->privacy_text == i && lobby_policy->privacy_string)
          value = content.localized(screen, *lobby_policy->privacy_string);
        if (lobby_policy->pane == 0 && i >= 4 && i <= 8)
          value = lobby->quick_options[i-4].value_or("");
        if (help.text_index == i && help.visible)
          value = content.localized(screen, help.string_id);
      }
      if (signin) {
        if (signin->text_strings[i])
          value = content.localized(screen, signin->text_strings[i]);
        if(i<8 && i%2==0)for(const auto& player:state.local_players())
          if(player.controller==i/2)value=player.name;
      } else if (frame.screen == 167)
        value = i == 0 ? state.keyboard_display() : frame.description;
      else if (frame.screen == 223)
        value = frame.description;
      else if (!state.online_clan_screen(frame) && !frame.description.empty() && i == 0 &&
               (screen.flags & (1 | 8 | 16)))
        value = frame.description;
      if(state.online_party_screen(frame))value=state.online_party_text(frame,i);
      if(state.online_friends_screen(frame))value=state.online_friends_text(frame,i);
      if(state.online_couch_screen(frame))value=state.online_couch_text(frame,i);
      const auto first = output.vertices.size();
      // Queue text1 is the native B Cancel caption. Service descriptions and
      // player-supplied text remain inert even if they contain prompt glyphs.
      const bool pane_prompts = frame.screen == 210 ? content.source_fonts.empty() && i == 1
          : frame.screen != 219 && frame.screen != 237;
      text(t, (state.online_couch_screen(frame) || state.online_clan_screen(frame) || state.online_party_screen(frame) || state.online_friends_screen(frame) || frame.screen == 20 || frame.screen == 186 || frame.screen == 167 || frame.screen == 210 || frame.screen == 219 || frame.screen == 237) ? value : state.expand(value), parent, 1, false, false, &frame, &pane.texts[i],
           interactive && !(frame.screen == 167 && i == 0) && pane_prompts);
      if (frame.screen == 167)
        keyboard_widget(first, t.layer, 6,
                        animation(t.animation, t.delay).position.z);
      if (appearance_popup && output.vertices.size() > first)
        popup_widgets.push_back({first, output.vertices.size(), t.layer, 6,
                                animation(t.animation, t.delay).position.z});
    }
    order_widgets(popup_widgets, "unrecorded appearance popup widget draw");
    for (unsigned i = 0; i < pane.buttons.size(); ++i) {
      if (lobby_policy && lobby_policy->buttons &&
          (i >= lobby_policy->buttons->size() || !(*lobby_policy->buttons)[i])) continue;
      if (frame.screen == 167 &&
          (i >= frame.entries.size() || !state.keyboard_key_visible(i)))
        continue;
      const auto &b = pane.buttons[i];
      UiText t = b.text;
      const auto &bounds = t.bounds;
      const auto selected = interactive && i == frame.selected;
      // 253E24 uses wigl title RGB for focused buttons and authored RGB
      // otherwise. Widget animation supplies alpha without extra dimming.
      if (selected) {
        t.color.red = content.color.red;
        t.color.green = content.color.green;
        t.color.blue = content.color.blue;
      }
      UiBitmap image;
      image.datum = b.bitmap;
      image.position = {bounds.left + b.offset.x, bounds.top - b.offset.y};
      image.animation = t.animation;
      image.delay = t.delay;
      image.frame = selected || (frame.screen == 167 && state.keyboard_key_active(i)) ? 1 : 0;
      const auto first = output.vertices.size();
      bitmap(image, parent);
      text(t,
           state.expand((frame.screen == 167 || state.online_party_screen(frame) || is_vista_menu(frame.screen)) && i < frame.entries.size()
                            ? frame.entries[i].label
                            : content.localized(screen, t.string)),
           parent, 1, true);
      if (frame.screen == 167)
        keyboard_widget(first, t.layer, 3,
                        animation(t.animation, t.delay).position.z);
      const bool lobby_enabled = !lobby_policy || i != 0 ||
                                 lobby_policy->start_eligible.value_or(true);
      const auto key = animation(t.animation, t.delay);
      // PC special-button vtable+14 ->63FFF8 adjusts only the hit top by
      // signed source+36. PC63F85B/63F9B7 keep the draw rectangle intact.
      const float hit_top = bounds.top -
          (is_vista_menu(frame.screen) ? b.vista_hit_top_offset : 0);
      if (interactive && lobby_enabled && key.alpha > 0 && output.vertices.size() > first &&
          (frame.screen != 167 || state.keyboard_key_enabled(i)))
        hits.push_back(
            {320 + parent.x + bounds.left + key.position.x,
             240 - parent.y - hit_top - key.position.y,
             320 + parent.x + bounds.right + key.position.x,
             240 - parent.y - bounds.bottom - key.position.y, i});
    }
    if (roster) {
      unsigned row_index = 0;
      for (const auto &grid : pane.grids)
        if (content.skin_groups.contains(grid.skin)) {
          unsigned cell=0;
          cells(grid, {}, [&](Vec2 origin) {
            const auto grid_cell=cell++;
            const auto index = row_index++;
            if (index >= roster->size() || !(*roster)[index].visible) return;
            const auto &row = (*roster)[index];
            const auto &group = content.skin_groups.at(grid.skin);
            // Live original group0 layers: emblem0, primary bar10, rank11,
            // name15. The left edge of the bar overlaps the emblem quad.
            if (row.emblem_visible) {
              unsigned instance=0;
              for (const auto &e : group.emblems)
                if(content.emblem_shaders.contains(e.shader))
                  emblem(e,origin,row.appearance,true,
                      grid_emblem_materials.at(&grid).at(grid_cell).at(instance++));
            }
            for (auto b : group.bitmaps) {
              bool visible = false;
              std::optional<UiColor> tint;
              if (b.placeholder == 0x1400069a) {
                visible = row.background_visible;
                b.frame = int(row.background_frame);
                tint = UiColor{1, row.primary_color.x, row.primary_color.y, row.primary_color.z};
              } else if (b.placeholder == 0x0e00069b)
                visible = row.secondary_background_visible;
              else if (b.placeholder == 0x0900069c) {
                visible = row.rank_visible;
                b.frame = int(row.rank_frame);
              }
              if (!visible) continue;
              if (content.images.contains(b.datum) && !content.images.at(b.datum).images.empty())
                b.position.y += content.images.at(b.datum).images.front().height;
              bitmap(b, origin, 1, 0, tint);
            }
            for (auto t : group.texts)
              if (t.string == 0x0b000698) {
                t.color.red = row.name_color.x; t.color.green = row.name_color.y;
                t.color.blue = row.name_color.z;
                if(lobby->party_names) {
                  // Native service names can fill all31 characters. This
                  // authored skin extends past the640-wide foreground; keep
                  // its glyphs and shifted shadow inside the visible field.
                  const auto key=animation(t.animation,t.delay);
                  const auto style=frontend_text_setup(t.flags,t.font,false,content.typewriter.scale);
                  t.bounds.right=std::min(t.bounds.right,
                      320-origin.x-key.position.x-(style.shadow?style.scale:0));
                }
                text(t, row.name_string ? content.localized(screen, *row.name_string) : row.name, origin);
              }
          });
        }
    }
    if (frame.screen == 47)
      for (const auto &grid : pane.grids)
        if (content.skin_groups.contains(grid.skin))
          cells(grid, {}, [&](Vec2 origin) {
            const auto &group = content.skin_groups.at(grid.skin);
            for (const auto &e : group.emblems)
              emblem(e, origin, state.appearance(), true);
            for (auto b : group.bitmaps)
              if (b.placeholder == 0x1400069a) {
                // 2AFEAE flags3 shows this bitmap, selects sequence0 and
                // writes the raw primary from7F790. At equal depth/layer10,
                // 2356D8 draws the bitmap before its name text child.
                const auto color = frontend_appearance_colors(
                    state.appearance(), content.appearance_palette)[0];
                if (content.images.contains(b.datum) &&
                    !content.images.at(b.datum).images.empty())
                  b.position.y += content.images.at(b.datum).images.front().height;
                bitmap(b, origin, 1, 0, UiColor{1, color.x, color.y, color.z});
              }
            for (auto t : group.texts)
              if (t.string == 0x0b000698) {
                // 2C8D7C supplies flags3/name54E5E0 to 22F042 -> 2AFEAE.
                const auto color = frontend_appearance_name_color(
                    state.appearance(), content.appearance_palette);
                t.color.red = color.x;
                t.color.green = color.y;
                t.color.blue = color.z;
                text(t, state.profile_name(), origin);
              }
          });
    if (frame.screen == 193) {
      const auto appearance = state.focused_profile_appearance(frame);
      for (const auto &grid : pane.grids)
        if (content.skin_groups.contains(grid.skin))
          cells(grid, {}, [&](Vec2 origin) {
            const auto &group = content.skin_groups.at(grid.skin);
            if (appearance) {
              // Original24A886 ->22F042 ->2AFEAE ->2B12CA supplies the
              // focused saved appearance and NONE/NONE overrides. The
              // authored type9 flag2 gate remains in emblem().
              for (const auto &e : group.emblems)
                emblem(e, origin, *appearance, true);
              for (auto t : group.texts)
                if (t.string == 0x0b000698) {
                  // 24A886 ->22F042 ->2AFEAE supplies the focused saved
                  // name/appearance (flags3); CreateNew has no name flag.
                  const auto rgb = frontend_appearance_name_color(
                      *appearance, content.appearance_palette);
                  t.color.red = rgb.x;
                  t.color.green = rgb.y;
                  t.color.blue = rgb.z;
                  text(t, frame.entries[frame.selected].label, origin);
                }
            } else {
              // CreateNew's flags20/value5e shows only this bitmap. Keep
              // its own authored animation and the2BACBC bottom-edge
              // placement. The single-image icons tag rejects sequence2
              // in2B01EB, so bitmap() independently resolves image0.
              for (auto b : group.bitmaps)
                if (b.placeholder == 0x0e00069b) {
                  if (content.images.contains(b.datum) &&
                      !content.images.at(b.datum).images.empty())
                    b.position.y += content.images.at(b.datum).images.front().height;
                  bitmap(b, origin);
                }
            }
          });
    }
    if(frame.vista_pcr) {pcr_list(pane,frame,interactive);return;}
    // Original2310F9 supplies screen186's persistent type1 list through
    // 23047E. Focus changes rebuild only ordinary pane children (230427 /
    // 22FBA9); panes1-3 intentionally have no list definition of their own.
    const auto &lists = frame.screen == 186 ? screen.panes.front().lists : pane.lists;
    if (!lists.empty() && !frame.entries.empty()) {
      const auto &list = lists.front();
      const auto &skin = content.skins.at(list.skin);
      const bool profile_skin = is_profile_skin(skin);
      const auto profile_bounds = profile_skin ? skin_bounds(skin) : UiRect{};
      float row = 0;
      for (const auto &t : skin.texts)
        row = std::max(row, t.bounds.top - t.bounds.bottom);
      for (const auto &b : skin.bitmaps)
        if (!(b.flags & 1) && content.images.contains(b.datum) &&
            !content.images.at(b.datum).images.empty())
          row =
              std::max(row, float(content.images.at(b.datum).images[0].height));
      if (row < 4)
        row = 24;
      if (profile_skin || frame.screen == 26 || frame.screen == 49 || frame.screen == 52 ||
          frame.screen == 233) {
        // 24BC12 advances by the union produced by 2BAFA4. The actual
        // default child skin spans [28,10,6,210], hence pitch22 (not20).
        const auto bounds = profile_skin ? profile_bounds : skin_bounds(skin);
        row = bounds.top - bounds.bottom;
      }
      const bool appearance_window = frame.screen == 49 || frame.screen == 50 ||
                                     frame.screen == 52 || frame.screen == 233;
      const unsigned first = appearance_window ? frame.first_visible
                             : frame.selected >= list.visible
                                 ? frame.selected - list.visible + 1
                                 : 0;
      const unsigned count =
          std::min(list.visible, unsigned(frame.entries.size()) - first);
      const auto row_focus = [&](bool selected) {
        if (drawing_exit)
          //22E957 gives a type2 row its list-global type3 definition;
          //22E930 replaces that row's and every descendant's focus animation.
          return animation(list.animation, list.delay);
        return skin.animations.empty()
            ? UiKey{selected ? 1.f : .5f, {}}
            : interpolate(skin.animations.at(selected ? 0 :
                  std::min(2u, unsigned(skin.animations.size() - 1))), 1000);
      };
      //2356D8 orders the proved equal-depth row type9/layer0 children before
      //layer0/type8 bitmaps. Keep this prepass within the profile list; it
      //does not reorder scenes, direct pane widgets or the focused header.
      if (profile_skin)
        for (unsigned i = 0; i < count; ++i)
          if (const auto appearance = state.profile_entry_appearance(frame, first + i)) {
            const auto focus = row_focus(first + i == frame.selected);
            const Vec2 p{list.position.x + focus.position.x,
                         list.position.y - float(i) * row + focus.position.y};
            unsigned instance = 0;
            profile_emblems(skin, p, focus.alpha, *appearance,
                            profile_emblem_materials.at(&list).at(i), instance);
          }
      for (unsigned i = 0; i < count; ++i) {
        const auto index = first + i;
        const bool selected = index == frame.selected;
        Vec2 p{list.position.x, list.position.y - float(i) * row};
        const auto focus = row_focus(selected);
        p.x += focus.position.x;
        p.y += focus.position.y;
        // 24AC80 -> 22E930 replaces the animation on every row descendant.
        // Their own focus scale/offset is read by the original draw routines;
        // pane and list animation values are not multiplied or accumulated.
        const float opacity = focus.alpha;
        unsigned social_bitmap_index=0;
        for (auto b : skin.bitmaps) {
          const auto social_bitmap=social_bitmap_index++;
          // The directory owns no source host/custom-map status icons.
          // Retain the PC13 selection background/end caps and column dividers.
          if (network_browser && social_bitmap >= 2 && social_bitmap <= 4) continue;
          // The queue API has no voice/presence state for these player rows.
          if (frame.screen == 210 || (frame.screen == 26 && social_bitmap >= 2)) continue;
          b.animation = 0;
          b.delay = 0;
          // List skin anchors are bottom-left (screen bitmap anchors are
          // top-left). The original tags deliberately use different origins.
          if (content.images.contains(b.datum) &&
              !content.images.at(b.datum).images.empty())
            b.position.y += content.images.at(b.datum).images.front().height;
          const auto first_bitmap = output.vertices.size();
          bitmap(b, p, opacity,
                 (b.flags & 2) ? (drawing_exit ? 1 : selected ? 0
                                  : index < frame.selected ? 1
                                                         : 2)
                               : b.frame);
          if (frame.screen == 26 && frame.context == 1 &&
              content.images.contains(b.datum) &&
              content.images.at(b.datum).images.front().width > 200)
            extend_bitmap(first_bitmap, {clan_width_extra(), 0});
        }
        if (profile_skin && !(frame.screen == 26 && frame.context == 1)) {
          const auto appearance = state.profile_entry_appearance(frame, index);
          for (const auto &grid : skin.grids)
            if (content.skin_groups.contains(grid.skin))
              cells(grid, p, [&](Vec2 origin) {
                profile_group(content.skin_groups.at(grid.skin), origin,
                              opacity, frame.entries[index], appearance);
              });
        }
        for (unsigned j = 0; j < skin.texts.size(); ++j) {
          //Actual40A781 first hides both text children, then shows child
          //!enabled. Both children receive the same source-local row label.
          if(frame.screen==vista_menu(6) && j!=unsigned(!state.vista_mainmenu_row_enabled(unsigned(frame.entries[index].value))))continue;
          if (is_vista_settings(frame.screen, 253) &&
              ((frame.entries[index].value == -2 && j != 3) ||
               (frame.entries[index].value != -2 && j == 3))) continue;
          // 24A09B hides the outer Create New name field for a saved row.
          if (profile_skin && frame.entries[index].destination == -3)
            continue;
          auto t = skin.texts[j];
          if (frame.screen == 26 && frame.context == 1)
            t.bounds.right += clan_width_extra();
          t.animation = 0;
          t.delay = 0;
          auto value = j == 0 ? frame.entries[index].label
                             : content.localized(screen, t.string);
          if (network_browser) value = state.online_directory_column(frame, index, j);
          if(frame.screen==vista_menu(6))value=frame.entries[index].label;
          if (state.online_clan_screen(frame) && j > 0) value = state.online_clan_column(frame,index,j);
          if(state.online_party_screen(frame) && j>0)value=state.online_party_column(frame,index,j);
          if(state.online_friends_screen(frame) && j>0)value=state.online_friends_column(frame,index,j);
          if (is_vista_menu(frame.screen) && frame.screen!=vista_menu(6) && j > 0 && j != 3)
            value = state.pc_value_label(frame, index, j);
          if (is_vista_settings(frame.screen, 253) && j == 3) value = frame.entries[index].label;
          if (frame.screen == 47 && j == 1) {
            // Original 2C8BF5 binds the second child to the edited settings.
            // These source strings contain the original colored glyphs.
            const auto appearance = state.appearance();
            unsigned low = 0;
            if (index == 0 && appearance.species >= 0 && appearance.species < 2)
              low = 0x2ff + unsigned(appearance.species);
            else if (index >= 1 && index <= 4 &&
                     appearance.colors[index - 1] >= 0 &&
                     appearance.colors[index - 1] < 18)
              low = 0x30a + unsigned(appearance.colors[index - 1]);
            else if (index == 5 && appearance.foreground < 64)
              low = 0x36f + appearance.foreground;
            else if (index == 6 && appearance.background < 32)
              low = 0x3af + appearance.background;
            value.clear();
            for (const auto &[id, label] : screen.strings)
              if ((id & 0xffff) == low) {
                value = label;
                break;
              }
          }
          // Vista6468BF supplies a zero list descriptor, so the original
          // 210 loader skips this authored roster. Its20-unit box cannot fit
          // the PC font's29-unit ascent. The native admitted-player roster
          // explicitly uses existing logical metrics with Vista glyph samples;
          // preserve the PC list/skin geometry and every other PC text field.
          const auto *roster_fonts = frame.screen == 210 && list.skin == 20
              ? online_roster_fonts : nullptr;
          text(t,
               (state.online_clan_screen(frame) || state.online_party_screen(frame) || state.online_friends_screen(frame) || frame.screen == 20 || frame.screen == 186 || frame.screen == 167 || frame.screen == 210 || frame.screen == 219 || frame.screen == 237) ? value : state.expand(value),
               p, opacity, false, false, nullptr, nullptr, false, roster_fonts);
        }
        if (interactive && opacity > 0 &&
            !(is_vista_settings(frame.screen, 253) && frame.entries[index].value == -2)) {
          float l = -10, r = 240, top = row;
          if (!skin.texts.empty()) {
            const auto& bounds=skin.texts.at(network_browser ? 6 : 0).bounds;
            l = bounds.left;r = bounds.right;top = bounds.top;
          }
          if (profile_skin) {
            l = profile_bounds.left;
            r = profile_bounds.right;
            top = profile_bounds.top;
          }
          if (frame.screen == 26 && frame.context == 1) {
            // The complete visible selection bar is a mouse target, including
            // the empty area beyond the member name and role.
            for (const auto &b : skin.bitmaps)
              if ((b.flags & 2) && content.images.contains(b.datum) &&
                  !content.images.at(b.datum).images.empty()) {
                const auto &image = content.images.at(b.datum).images.front();
                l = b.position.x;
                r = l + image.width + clan_width_extra();
                top = b.position.y + image.height;
                break;
              }
          }
          hits.push_back({320 + p.x + l, 240 - p.y - top, 320 + p.x + r,
                          240 - p.y - top + row, index});
        }
      }
      if (first > 0) {
        UiBitmap b;
        b.datum = skin.arrows;
        b.position = skin.arrow_up;
        bitmap(b, {parent.x + list.position.x, parent.y + list.position.y});
      }
      if (first + count < frame.entries.size()) {
        UiBitmap b;
        b.datum = skin.arrows;
        b.position = skin.arrow_down;
        if (frame.screen == 26 && frame.context == 1)
          b.position.y -= clan_footer_offset();
        b.frame = 1;
        bitmap(b, {parent.x + list.position.x, parent.y + list.position.y});
      }
    }
  }
  void keyboard_overlay(const UiPane &pane, const MenuState &state) {
    if (pane.texts.empty() || !images.contains(content.keyboard_caret) ||
        images.at(content.keyboard_caret).empty())
      return;
    const auto &t = pane.texts.front();
    if (seconds * 1000.f < t.delay)
      return;
    const auto parent = animation(pane.animation);
    const auto key = animation(t.animation, t.delay);
    const auto setup = frontend_text_setup(t.flags, t.font, false,
                                           content.typewriter.scale);
    const auto &f = font(setup.font);
    const float scale = setup.scale;
    // 253AB4/253A73 call 22E9C6, which adds only this field's own offset.
    const float left = 320 + t.bounds.left + key.position.x;
    const float top = 240 - t.bounds.top - key.position.y;
    const float baseline = top + f.ascent * scale;
    const auto chars = codepoints(state.keyboard_display());
    if (state.keyboard_selected() && !chars.empty() &&
        images.contains(content.keyboard_selection) &&
        images.at(content.keyboard_selection).size() > 1) {
      float x = left, right = left, bottom = top;
      unsigned previous = 0;
      for (auto c : chars) {
        if (const auto it = f.glyphs.find(c); it != f.glyphs.end()) {
          const auto &g = it->second;
          const auto kern = f.kerning.find({previous, c});
          const float kerning = kern == f.kerning.end() ? 0.f : kern->second * scale;
          const float offset = g.bearing_x * scale;
          // 13E8E0 includes one pixel beyond each ink rectangle; 22D13D
          // retains projected top/left and the measured ink right/bottom.
          right = std::max(right, x + offset + kerning + g.width * scale + 1);
          bottom = std::max(bottom, baseline - g.bearing_y * scale +
                                    g.height * scale + 1);
          x = g.advance * scale + x + offset + kerning;
        }
        previous = c;
      }
      // 237C04's clamp flag is1, but 235E5E forwards the separate blend0.
      quad(left, top, right - left, bottom - top,
           images.at(content.keyboard_selection).at(1));
    }
    // 253A73 -> 22D0E3 requests the prefix line bounds, not its ink box.
    // Original 236235/137610 selects the first bitmap descriptor with frame0.
    const auto prefix = codepoints(state.keyboard_prefix());
    const float cursor = left + measure(prefix, setup.font, scale);
    const auto alpha = frontend_keyboard_caret_alpha(ui_clock_ms, parent.alpha);
    quad(cursor, top, 1, (f.ascent + f.descent) * scale,
         images.at(content.keyboard_caret).front(),
         {float(alpha) / 255, 1, 1, 1});
  }
  void screen(const MenuFrame &frame, const MenuState &state, bool interactive,
              int override_pane = -1) {
    const auto layout = state.screen_layout(frame);
    if (vista && (is_vista_menu(layout) ||
        (frame.screen == 210 && content.vista->screens.contains(210)))) {
      vista->output = {};
      vista->hits.clear();
      vista->seconds = seconds;
      vista->ui_clock_ms = ui_clock_ms;
      vista->screen(frame, state, interactive, override_pane);
      // PC65E191 ->664979 supplies height/1200 to UI raster state. The
      // native renderer scales its centered480-high canvas to the window.
      constexpr float scale = 480.f / 1200.f;
      const auto first_vertex=output.vertices.size();
      for (auto vertex : vista->output.vertices) {
        vertex.position.x = 320 + (vertex.position.x - 320) * scale;
        vertex.position.y = 240 + (vertex.position.y - 240) * scale;
        output.vertices.push_back(vertex);
      }
      // PC64090D independently projects the authored model rectangle with
      // its actual height/1200 raster scale, including screen182. Retain the
      // source record and let the model owner use the current drawable.
      for(auto scene:vista->output.scenes) {
        scene.vista_viewport=true;
        scene.before_vertex+=first_vertex;output.scenes.push_back(std::move(scene));
      }
      for (auto hit : vista->hits) {
        hit.left = 320 + (hit.left - 320) * scale;
        hit.right = 320 + (hit.right - 320) * scale;
        hit.top = 240 + (hit.top - 240) * scale;
        hit.bottom = 240 + (hit.bottom - 240) * scale;
        hits.push_back(hit);
      }
      output.rendering_gaps.insert(output.rendering_gaps.end(),
          vista->output.rendering_gaps.begin(),vista->output.rendering_gaps.end());
      return;
    }
    if (frame.screen == 167)
      keyboard_widgets.clear();
    const auto &s = content.screen(layout);
    if(frame.vista_pcr) {
      VistaPcrDrawServices draw;
      draw.elapsed=frame.vista_pcr->elapsed;
      draw.black=[&](std::uint32_t argb){quad(-480,-360,1600,1200,pcr_black,
          {float(argb>>24)/255,0,0,0});};
      draw.base_draw=[]{};
      vista_pcr_draw(frame.screen==vista_menu(185),frame.vista_pcr->creation_clock,
          draw,frame.vista_pcr->floating_point);
    }
    const auto lobby = frame.screen == 14 ? state.lobby_view() : std::nullopt;
    const auto setup = frame.screen == 23 ? state.lobby_view() : std::nullopt;
    const bool dialog = s.flags & (1 | 8 | 16);
    if (!s.panes.empty()) {
      const auto index =
          override_pane >= 0
              ? std::min(unsigned(override_pane), unsigned(s.panes.size() - 1))
              : (frame.vista_pcr ? frame.context
                 : lobby
                     ? unsigned(frontend_lobby_pane(lobby->input.session_enum, 0))
                 : frame.screen == 210
                     ? content.source_fonts.empty() ? 1u : state.online_matchmaking_pane()
                 : frame.screen == 26
                     ? std::min(frame.context, unsigned(s.panes.size() - 1))
                 : (s.flags & 2)
                     ? std::min(frame.selected, unsigned(s.panes.size() - 1))
                     : 0);
      const auto first_vertex = output.vertices.size(), first_hit = hits.size();
      pane(s, s.panes.at(index), frame, state, interactive);
      if (frame.screen == 26 && frame.context == 1) {
        // Native full-surface clan page: anchor the authored tab at the top
        // left of the surface. Keep glyph/icon scale and translate hits alike.
        const Vec2 offset{-canvas.left / canvas.scale, -canvas.top / canvas.scale - 36};
        for (auto i = first_vertex; i < output.vertices.size(); ++i) {
          output.vertices[i].position.x += offset.x;
          output.vertices[i].position.y += offset.y;
        }
        for (auto i = first_hit; i < hits.size(); ++i) {
          hits[i].left += offset.x;
          hits[i].right += offset.x;
          hits[i].top += offset.y;
          hits[i].bottom += offset.y;
        }
      }
    }
    if (!(s.flags & 4)) {
      const unsigned type = (s.flags & 1)    ? 3
                            : (s.flags & 8)  ? 2
                            : (s.flags & 16) ? 1
                                             : 0;
      UiText header;
      header.font = content.header_fonts[type];
      if (drawing_exit && !s.panes.empty())
        header.animation = s.panes.front().animation; //22FDA6 pane_value
      header.bounds = content.header_bounds[type * 2];
      header.flags = 1;
      header.color = content.color;
      const auto first = output.vertices.size();
      text(header,
           (state.online_clan_screen(frame) || state.online_party_screen(frame) || state.online_friends_screen(frame) || frame.screen == 20 || frame.screen == 186 || frame.screen == 167 || frame.screen == 210 || frame.screen == 219 || frame.screen == 237) ? frame.title : state.expand(frame.title.empty() ? content.localized(s, s.header)
                                            : frame.title), {}, 1, false, true);
      if (frame.screen == 167)
        keyboard_widget(first, 32767, 6, 0);
    }
    if (!frame.description.empty() && !is_vista_settings(frame.screen, 253) && frame.screen != 167 &&
        !((frame.screen == 20 || frame.screen == 186) && !s.panes.empty() && !s.panes.front().texts.empty()) &&
        frame.screen != 223 &&
        (!dialog || s.panes.empty() || s.panes.front().texts.empty())) {
      UiText t;
      t.font = 1;
      t.flags = 1;
      t.color = content.color;
      t.bounds =
          dialog ? UiRect{70, -130, -10, 130} : UiRect{160, -250, 95, 250};
      text(t, (state.online_clan_screen(frame) || state.online_party_screen(frame) || state.online_friends_screen(frame)) ? frame.description : state.expand(frame.description));
    }
    if (s.button_key && s.button_key <= content.button_keys.size()) {
      UiText t;
      t.flags = 2;
      t.font = 1;
      if (drawing_exit && !s.panes.empty())
        t.animation = s.panes.front().animation; //22FF53 pane_value
      // 22FF53 chooses the screen definition's subtitle color (+8).
      t.color = s.color;
      const unsigned type = (s.flags & 1)    ? 3
                            : (s.flags & 8)  ? 2
                            : (s.flags & 16) ? 1
                                             : 0;
      t.bounds = content.header_bounds[type * 2 + 1];
      const auto first = output.vertices.size();
      const auto pcr_footer=[&] {
        if(!frame.vista_pcr)return std::string{};
        if(!frame.vista_pcr->input.host || !frame.vista_pcr->input.details_available)
          throw FormatError("Vista PCR footer requires genuine source host/details owners");
        const auto host=frame.vista_pcr->input.host();
        const auto details=frame.vista_pcr->input.details_available();
        return pcr_utf8(vista_pcr_footer(VistaPcrPane(frame.context),frame.screen==vista_menu(185),
            host,details,frame.vista_pcr->rows.localized));
      };
      text(t, frame.vista_pcr?pcr_footer():state.expand(lobby
          ? content.localized(s, frontend_lobby_presentation(lobby->input).subtitle_string)
          : setup ? content.localized(s, frontend_lobby_setup_subtitle(setup->input.session_enum))
          : frame.screen == 219 && frame.context == 0 ? "\xee\x84\x80 SELECT  \xee\x84\x81 BACK  \xee\x84\x82 REFRESH"
          : frame.screen == 237 && frame.context == 0 ? "\xee\x84\x80 FIND MATCH  \xee\x84\x81 BACK"
          : frame.screen == 219 ? (frame.entries.size() == 1 && frame.entries.front().destination == -13
              ? "\xee\x84\x80 REFRESH  \xee\x84\x81 BACK  \xee\x84\x82 REFRESH"
              : "\xee\x84\x80 DETAILS  \xee\x84\x81 BACK  \xee\x84\x82 REFRESH")
          : frame.screen == 237 ? "\xee\x84\x81 BACK"
          : content.button_keys[s.button_key - 1]), {}, 1, false, false, &frame, nullptr, interactive);
      if (frame.screen == 167)
        keyboard_widget(first, 32767, 6, 0);
    }
    if (frame.screen == 6) {
      UiText t;
      t.flags = 2;
      t.font = 1;
      t.color = content.color;
      t.bounds = content.header_bounds[1];
      text(t, content.localized(s, 0x08001682), {}, 1, false, false, &frame, nullptr, interactive);
    }
    if (frame.screen == 167 && !s.panes.empty()) {
      const auto first = output.vertices.size();
      keyboard_overlay(s.panes.front(), state);
      keyboard_widget(first, 0, 0,
          animation(s.panes.front().animation).position.z);
      keyboard_order();
    }
  }
  UiDraw draw(const MenuState &state, float elapsed, int pane_override,
              std::uint32_t clock_ms) {
    const bool online_active=online_panel(state);
    if(online_active && online) {
      online->canvas=canvas;
      auto result=online->draw(state,elapsed,pane_override,clock_ms);
      hits=online->hits;pointer_revision=online->pointer_revision;
      return result;
    }
    if(online_active && missing_online_assets)
      throw FormatError("The imported Vista ONLINE UI assets are unavailable.");
    seconds = elapsed;
    ui_clock_ms = clock_ms;
    output.vertices.clear();
    output.scenes.clear();
    output.backdrops.clear();
    output.rendering_gaps.clear();
    hits.clear();
    pointer_revision = state.revision;
    const auto &frame = state.frame();
    std::set<TextLifetime> live_text_lifetimes;
    for (const auto &live : state.stack()) {
      live_text_lifetimes.emplace(live.instance, live.pane_generation);
      if (live.previous_lobby_setup)
        live_text_lifetimes.emplace(live.previous_lobby_setup->instance,
                                   live.previous_lobby_setup->pane_generation);
    }
    std::erase_if(typing_widgets, [&](const auto &entry) {
      return !live_text_lifetimes.contains(entry.first);
    });
    if (vista)
      std::erase_if(vista->typing_widgets, [&](const auto &entry) {
        return !live_text_lifetimes.contains(entry.first);
      });
    const auto elapsed_ms = std::uint32_t(std::max(0.f, std::floor(elapsed * 1000.f)));
    if (frame.previous_lobby_setup != previous_lobby_setup || elapsed_ms < exit_elapsed_ms) {
      previous_lobby_setup = frame.previous_lobby_setup;
      exit_animations.clear();
    }
    exit_elapsed_ms = elapsed_ms;
    drawing_exit = false;
    const bool title_popup_backdrop = (frame.screen == 10 || frame.screen == 167) &&
        std::any_of(state.stack().begin(), state.stack().end(),
                    [](const auto &f) { return f.screen == 9; });
    const bool online_panel = online_active;
    online_vista_art = online_panel && bool(vista);
    const bool clan_backdrop = std::any_of(state.stack().begin(), state.stack().end(),
        [&](const auto &f) { return state.online_clan_screen(f); });
    output.full_surface_bounds = clan_backdrop;
    if(content.gameplay_overlay && content.vista) {
      const auto &overlay=content.vista->overlay;
      output.backdrops.push_back({0,1,{overlay.red,overlay.green,overlay.blue,overlay.alpha},true});
    }
    if (clan_backdrop) {
      // An opaque color sampled from the original body replaces shell decoration
      // and the world beneath this native page, including its nested dialogs.
      quad(-canvas.left / canvas.scale, -canvas.top / canvas.scale,
           canvas.surface_width / canvas.scale, canvas.surface_height / canvas.scale,
           white, clan_background_color);
    }
    // 2234E0 ->13870 owns this normal mainmenu world view: mode3, one
    // canonical640x480 view, neither offscreen guard. The scenario's constant
    // interpolator selects E4380218 before UI, independently of screen ID.
    // Same-clock6/14/23/206 captures confirm the title/47 shader binding.
    // Channel tint/blur belongs below, after lower UI; no initial blue quad.
    output.scene_color_filter = true;
    if (!content.gameplay_overlay && !clan_backdrop && !title_popup_backdrop && frame.screen != 9 && frame.screen != 6 &&
        content.screens.contains(5)) {
      MenuFrame background;
      const bool pc_settings=content.vista && std::any_of(state.stack().begin(),state.stack().end(),
          [](const auto &f){return is_vista_menu(f.screen);});
      background.screen = (pc_settings || (online_panel && content.vista)) ? vista_menu(5) : 5;
      screen(background, state, false);
    }
    const auto layout = state.screen_layout(frame);
    const bool dialog = content.screen(layout).flags & (1 | 8 | 16);
    if (dialog && state.stack().size() > 1) {
      auto it = state.stack().rbegin() + 1;
      while (it + 1 != state.stack().rend() &&
             (content.screen(state.screen_layout(*it)).flags & (1 | 8 | 16)))
        ++it;
      const bool choosing_profile = std::any_of(state.stack().begin(),
          state.stack().end(), [](const auto &f) { return f.screen == 10; });
      // Disc screens9/6 share the start_screen logo bitmap. The native
      // ONLINE panel replaces that lower title/main-menu UI for readability.
      const bool hide_title = online_panel && (it->screen == 9 || it->screen == 6);
      // The native directory details page keeps browser history for Back,
      // while replacing its visible content and footer.
      const bool hide_lower = hide_title || (clan_backdrop && frame.screen != 26) ||
          (state.online_party_screen(frame) && frame.screen != 14) ||
          (state.online_friends_screen(frame) && frame.screen != 26) ||
          ((frame.screen == 237 || frame.screen == 210) && (it->screen == 219 || it->screen == 237 || it->screen == 186));
      // 22F916 ->1473C9 ->23029A ->234F8C: every common dialog
      // over an occupied lower channel receives the authored tint/blur,
      // unless definition flag5 suppresses it. An ID whitelist omitted
      // error/confirmation/settings dialogs and left lower text sharp.
      if (!(content.screen(layout).flags & 32)) {
        if (!hide_lower) screen(*it, state, false);
        // The original disposes profile channel 3 before opening keyboard
        // channel 2 (23784F). Both paths therefore blur the retained title
        // once, as confirmed by 23029A and the live channel snapshots.
        // 230374/234F8C/235756: the popup's authored transition
        // controls the tint and full-resolution blur of lower-channel UI.
        // Setup23 and Map206 also occupy channel3 over retained lobby14.
        // A replacement retains the outgoing channel's complementary fade.
        const auto &definition = content.screen(layout);
        const auto pane_animation = definition.panes.empty() ? 0 : definition.panes.front().animation;
        const auto &active_content = is_vista_menu(layout) && content.vista ? *content.vista : content;
        const auto duration = pane_animation ? active_content.animations.at(pane_animation - 1).intro.milliseconds : 0;
        const float fade = duration && !frame.retained_lobby_backdrop ? std::clamp(std::floor(elapsed * 1000.f) /
            float(duration), 0.f, 1.f) : 1.f;
        output.backdrops.push_back(UiBackdrop{output.vertices.size(),
            fade,
            {active_content.overlay.red, active_content.overlay.green, active_content.overlay.blue,
             active_content.overlay.alpha}, true});
      } else {
        if (!hide_lower && (it->screen != 9 || !choosing_profile))
          screen(*it, state, false);
        // 22F916 copies authored flag5 to v27; 23029A/234F8C then
        // submit these mode2 children without any tint or blur. The
        // parent's list remains collected beneath the authored panel.
      }
    }
    if (frame.screen == 206 && previous_lobby_setup) {
      const auto &previous_screen = content.screens.at(previous_lobby_setup->screen);
      const auto index = previous_screen.panes.front().animation;
      auto &exit = exit_animations[index];
      exit.update(content.animations.at(index - 1).outro, exit_elapsed_ms, true);
      if (!exit.queued) {
        //234F8C submits the previous page after the one shared blur, then
        //the incoming page. It never adds the previous page to navigation.
        drawing_exit = true;
        screen(*previous_lobby_setup, state, false);
        drawing_exit = false;
      }
    }
    screen(frame, state, true, pane_override);
    return output;
  }
};
FrontendPainter::FrontendPainter(const FrontendContent &c,
                                 const std::filesystem::path &fonts, Mesh &mesh,
                                 const std::filesystem::path &frame_edges,
                                 const FrontendUiAssetPack &ui_assets,
                                 const std::filesystem::path &sampled_fonts)
    : impl_(std::make_unique<Impl>(c, fonts, mesh, frame_edges, ui_assets, sampled_fonts)) {}
FrontendPainter::~FrontendPainter() = default;
UiDraw FrontendPainter::draw(const MenuState &state, float seconds, int pane,
                             std::uint32_t ui_clock_ms, std::array<unsigned, 2> viewport) {
  impl_->canvas = frontend_canvas(viewport[0], viewport[1]);
  return impl_->draw(state, seconds, pane, ui_clock_ms);
}
std::optional<unsigned> FrontendPainter::hit(float x, float y) const {
  for (const auto &h : impl_->hits)
    if (h.action == FrontendPointerAction::Entry &&
        x >= h.left && x < h.right && y >= h.top && y < h.bottom)
      return h.index;
  return {};
}
std::optional<FrontendPointerTarget> FrontendPainter::pointer_hit(float x, float y) const {
  // A source prompt can be embedded in a pane above other widgets. Give its
  // visible icon/label priority; retained lower menus never submit targets.
  for (const bool prompts : {true, false})
    for (auto it = impl_->hits.rbegin(); it != impl_->hits.rend(); ++it) {
      const auto &h = *it;
      if ((h.action != FrontendPointerAction::Entry) == prompts &&
          x >= h.left && x < h.right && y >= h.top && y < h.bottom)
        return FrontendPointerTarget{h.action, h.index, impl_->pointer_revision};
    }
  return {};
}
} // namespace halo2
