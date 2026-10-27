#include "halo2/frontend.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/frontend_network_transition.hpp"
#include "halo2/frontend_keyboard.hpp"
#include "halo2/frontend_signin.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/frontend_lobby_roster.hpp"
#include "halo2/frontend_lobby_setup.hpp"
#include "halo2/frontend_lobby_warning.hpp"
#include "halo2/frontend_text_style.hpp"
#include "halo2/frontend_typewriter.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/controller_prompt_artwork.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "ui/mcc_frontend_string_ids.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "ui/frontend_mcc_logical_layout.hpp"
#include "ui/frontend_og_matchmaking_layout.hpp"
#include "ui/frontend_mcc_model_viewports.hpp"
#include "ui/frontend_mcc_frame_geometry.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/frontend_frame_texture.hpp"
#include "halo2/frontend_frame_sampling.hpp"
#include "halo2/frontend_exit_animation.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
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
  // A projected view contains only its finite authored screens. Navigation
  // ancestors still belong to the complete selected-cache definition table.
  const FrontendContent *navigation_content{&content};
  std::vector<RetailFont> fonts;
  std::vector<unsigned> font_materials;
  bool drawing_original_goal{};
  std::map<unsigned,unsigned> notice_materials;
  std::shared_ptr<ControllerPromptArtwork> prompt_artwork;
  PromptDevice prompt_device=PromptDevice::Xbox;
  std::map<std::uint32_t, std::vector<unsigned>> images;
  std::map<unsigned, std::array<unsigned, 2>> frame_extensions;
  std::set<unsigned> original_mcc_frames;
  unsigned white{}, background_dim{}, pcr_black{}, map_intro_black{};
  UiColor clan_background_color;
  Mesh &mesh;
  std::unique_ptr<Impl> vista;
  std::unique_ptr<Impl> online;
  std::optional<detail::mcc_logical_layout::View> logical_view;
  std::unique_ptr<Impl> logical;
  const UiScreen *original_goal_background{};
  const UiScreen *original_friends_footer{};
  const UiScreen *other_lobby{};
  std::map<unsigned,const UiScreen*> other_goal_dialogs;
  const Impl *shell_fonts{};
  Impl *pc_profile_owner{};
  std::map<const UiScene *, UiScene> model_viewports;
  bool missing_online_assets{};
  const Impl *online_roster_fonts{};
  bool online_vista_art{};
  std::map<std::string,std::vector<unsigned>> vista_art;
  std::map<const UiEmblem *, unsigned> emblem_materials;
  std::map<const UiGrid *, std::vector<std::vector<unsigned>>> grid_emblem_materials;
  std::map<const UiList *, std::vector<std::vector<unsigned>>> profile_emblem_materials;
  UiDraw output;
  FrontendCanvas canvas = frontend_canvas(640, 480);
  bool imported_mcc_canvas{};
  bool parent_projects_pc_canvas{};
  bool original_logical_layout{};
  bool full_surface_friends{};
  std::optional<UiScreen> mcc_browser;
  float browser_frame_height_delta{};
  bool wide_social_page(const MenuFrame &f) const {
    return f.screen == 26 && (f.context == 1 || (full_surface_friends && f.context == 0));
  }
  float social_width_extra() const { return canvas.surface_width / canvas.scale - 529; }
  float social_footer_offset() const { return canvas.surface_height / canvas.scale - 420; }
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
  bool original_string_matches(std::uint32_t authored,std::uint32_t original) const {
    const auto imported=detail::frontend_compiled_string_id(content,original);
    return imported && authored==*imported;
  }
  std::string original_low_localized(const UiScreen &screen,std::uint32_t original) const {
    if(detail::mcc_frontend_content(content))
      return detail::frontend_compiled_localized(content,screen,original);
    for(const auto &[id,label]:screen.strings)
      if((id&0xffffu)==original)return label;
    return {};
  }
  static bool is_profile_skin(const UiSkin &skin) {
    return skin.name == "ui\\list_skins\\profile_picker\\profile_picker" ||
           skin.name == "ui\\list_skins\\profile_picker2\\profile_picker";
  }
  void row_emblems(const UiSkin &skin,std::vector<const UiEmblem *> &instances,
                   unsigned depth=0) const {
    if(depth>16)throw FormatError("cyclic frontend skin group");
    for(const auto &emblem:skin.emblems)
      if(content.emblem_shaders.contains(emblem.shader))instances.push_back(&emblem);
    for(const auto &grid:skin.grids)
      if(content.skin_groups.contains(grid.skin))
        cells(grid,{},[&](Vec2){row_emblems(content.skin_groups.at(grid.skin),instances,depth+1);});
  }
  void reserve_grid_emblems(const UiScreen &screen) {
    // Original2B12CA gives every constructed row its own appearance bank.
    // Definition copies need their own grid identities before residency freezes.
    for(const auto &pane:screen.panes)for(const auto &grid:pane.grids) {
      if(!content.skin_groups.contains(grid.skin) || grid_emblem_materials.contains(&grid))continue;
      std::vector<const UiEmblem *> instances;
      row_emblems(content.skin_groups.at(grid.skin),instances);
      if(instances.empty())continue;
      auto &cells_materials=grid_emblem_materials[&grid];
      cells(grid,{},[&](Vec2) {
        auto &cell=cells_materials.emplace_back();
        for(const auto *emblem:instances) {
          // The source emblem already has this painter's sampled images.
          // Copy its independent constants; immutable image bytes stay shared.
          auto material=mesh.materials.at(emblem_materials.at(emblem));
          cell.push_back(unsigned(mesh.materials.size()));
          mesh.materials.push_back(std::move(material));
        }
      });
    }
  }
  Impl(const FrontendContent &c, const std::filesystem::path &directory,
       Mesh &mesh, const std::filesystem::path &edge_directory,
       const FrontendUiAssetPack &ui_assets,
       const std::filesystem::path &sampled_fonts, bool separate_online = true,
       bool original_layout = false,std::shared_ptr<ControllerPromptArtwork> artwork = {})
      : content(c), mesh(mesh), original_logical_layout(original_layout) {
    prompt_artwork=artwork?std::move(artwork):std::make_shared<ControllerPromptArtwork>(mesh,directory);
    white = unsigned(mesh.materials.size());
    mesh.materials.push_back({});
    background_dim = unsigned(mesh.materials.size());
    mesh.materials.push_back({"ui/background-dim"});
    mesh.materials.back().ui_widescreen_background = true;
    pcr_black = unsigned(mesh.materials.size());
    mesh.materials.push_back({"ui/original-pcr-intro-black"});
    mesh.materials.back().ui_full_surface = true;
    map_intro_black = unsigned(mesh.materials.size());
    mesh.materials.push_back({"ui/original-map-intro-black"});
    mesh.materials.back().ui_full_surface = true;
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
        if (image.name == "ui\\global_bitmaps\\framing_center" ||
            image.name==detail::og_matchmaking_layout::embedded_image_name("ui\\global_bitmaps\\framing_center")) {
          const auto original = images[id].back();
          if (original_logical_layout &&
              detail::stock_mcc_frame_pixels(mesh.materials.back().diffuse))
            original_mcc_frames.insert(original);
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
    // Imported MCC owns its authored twelve selectors in halo2/h2_fonts.
    // The same imported source also identifies its PC-coordinate widget bank;
    // keep that private to this painter, separate from Vista content routing.
    const bool imported_mcc_fonts = directory.filename() == "h2_fonts" &&
        directory.parent_path().filename() == "halo2";
    const auto font_source = imported_mcc_fonts
        ? std::optional{load_gameplay_font_source(directory)} : std::nullopt;
    const auto expected_fonts = original_logical_layout ? 11u : imported_mcc_fonts
        ? font_source->names.size()
        : c.source_fonts.empty() ? 11u : 12u;
    imported_mcc_canvas = imported_mcc_fonts && !original_logical_layout;
    const auto prompt_assets=font_prompt_asset_directory(directory,
        sampled_fonts.empty()?std::filesystem::path{}:sampled_fonts.parent_path());
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
      // The authenticated MCC table has twelve PC roles. This finite original
      // layout uses the existing eleven-role logical font projection instead.
      if (original_logical_layout && fonts.size() == expected_fonts)
        continue;
      if (fonts.size() >= expected_fonts)
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
      fonts.push_back(original_logical_layout
          ? load_gameplay_font(*font_source, unsigned(fonts.size()))
          : load_retail_font(font_path));
      prompt_artwork->prepare(unsigned(fonts.size()-1));
      const bool mcc=is_mcc_classic_menu_pack(sampled_fonts.parent_path());
      const auto sampled=mcc?ui_sampled_font_path(sampled_fonts.parent_path(),unsigned(fonts.size()-1),name):sampled_fonts/name;
      if(!sampled_fonts.empty() && std::filesystem::is_regular_file(sampled)) {
        if(mcc)apply_mcc_classic_font_sampling(fonts.back(),load_retail_font(sampled));
        else apply_retail_font_sampling(fonts.back(),load_retail_font(sampled));
      } else if(mcc)throw FormatError("Missing MCC Classic menu font owner");
      if(!prompt_assets.empty())
        apply_vista_prompt_sampling(fonts.back(),prompt_assets,unsigned(fonts.size()-1));
      // Original selectors may share a logical font but select different MCC
      // rasters. Their combined atlases must have distinct material owners.
      const auto key=original_logical_layout ? name+"/original-role/"+std::to_string(fonts.size()-1)
          : mcc?name+"/"+sampled.filename().string():name;
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
    if (fonts.size() != expected_fonts)
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
    // Repeated lobby grid cells construct independent type9 widgets in the
    // original. Their shader banks must survive together in one submission.
    for (const auto &[id, screen] : c.screens)
      reserve_grid_emblems(screen);
    for (const auto &[id, screen] : c.screens)
      for (const auto &pane : screen.panes)
        for (const auto &list : pane.lists) {
          if(!list.visible)continue;
          const auto &skin = c.skins.at(list.skin);
          if (!is_profile_skin(skin) && !(c.original_matchmaking_layout &&
              (screen.id==210 || screen.id==16 || screen.id==185))) continue;
          std::vector<const UiEmblem *> instances;
          row_emblems(skin, instances);
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
      const auto sampled=load_frontend_ui_assets(*c.vista,
          mcc?sampled_fonts.parent_path():std::filesystem::path{});
      vista = std::make_unique<Impl>(*c.vista, c.vista->source_fonts, mesh,
                                    std::filesystem::path{}, sampled, mcc?sampled_fonts:std::filesystem::path{},true,false,prompt_artwork);
      // This child emits the raw authored PC coordinates. Its parent owns
      // the single height/1200 projection for vertices, hits and viewports.
      vista->parent_projects_pc_canvas=true;
      vista->online_roster_fonts = this;
      for(const auto &[id,image]:c.vista->images)
        vista_art.emplace(image.name,vista->images.at(id));
      const bool same_mcc_bank=detail::mcc_frontend_content(c) &&
          detail::mcc_frontend_content(*c.vista) && c.source_fonts==c.vista->source_fonts;
      if(separate_online && !same_mcc_bank) {
        // The PC cache and imported bank share one source root. A selected
        // shell pack must not replace ONLINE/clan fonts or bitmap resources.
        const auto root=c.vista->source_fonts.parent_path().parent_path();
        std::error_code error;
        const bool selected_vista=!sampled_fonts.empty() &&
            std::filesystem::equivalent(sampled_fonts.parent_path(),root,error);
        if(!selected_vista) {
          if(std::filesystem::is_regular_file(root/"pack.tsv"))
            online=std::make_unique<Impl>(c,directory,mesh,edge_directory,
                load_frontend_ui_assets(c,root),root/"fonts",false,false,prompt_artwork);
          else missing_online_assets=true;
        }
      }
    }
    if (c.vista && detail::mcc_frontend_content(c)) {
      logical_view = detail::mcc_logical_layout::make_view(c);
      if(logical_view && !detail::og_matchmaking_layout::augment(*logical_view,c,ui_assets))
        throw FormatError("Selected MCC matchmaking definitions do not match the qualified original UI profile");
      if (logical_view)
        logical = std::make_unique<Impl>(logical_view->content, directory, mesh,
            edge_directory, logical_view->rasters, std::filesystem::path{}, false, true,prompt_artwork);
      if (logical) {
        logical->navigation_content=&c;
        logical->original_goal_background=&*logical_view->original_goal_background;
        logical->original_friends_footer=&*logical_view->original_friends_footer;
        logical->other_lobby=&*logical_view->other_lobby;
        logical->reserve_grid_emblems(*logical->other_lobby);
        for(const auto &[id,definition]:logical_view->other_goal_dialogs)logical->other_goal_dialogs.emplace(id,&definition);
        shell_fonts = logical.get();
        vista->shell_fonts = logical.get();
        vista->online_roster_fonts = logical.get();
        logical->pc_profile_owner = vista.get();
        logical->full_surface_friends = logical_view->full_surface_friends;
        vista->mcc_browser = logical_view->browser;
        vista->browser_frame_height_delta = logical_view->browser_frame_height_delta;
      }
    }
    // Reserve notice materials before the renderer freezes its resident table.
    // TexturePixels copies share immutable residency with the source atlas.
    std::vector<unsigned> notice_sources{white};
    if(!font_materials.empty())notice_sources.push_back(font_materials.front());
    const auto count=mesh.materials.size();
    for(unsigned i=0;i<count;++i) {
      const auto &name=mesh.materials[i].name;
      if(name.starts_with("controller-prompts/") && name.find("/atlases/0.dds/")!=std::string::npos && name.find("/party-notice/")==std::string::npos)
        notice_sources.push_back(i);
    }
    for(const auto id:notice_sources) {
      const auto name=mesh.materials.at(id).name+"/party-notice/"+std::to_string(id);
      const auto found=std::find_if(mesh.materials.begin(),mesh.materials.end(),[&](const auto &m){return m.name==name;});
      if(found!=mesh.materials.end())notice_materials[id]=unsigned(found-mesh.materials.begin());
      else {
        auto material=mesh.materials.at(id);material.name=name;material.ui_full_surface=true;
        notice_materials[id]=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(material));
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
  void group_bitmap(UiBitmap b, Vec2 origin, float alpha=1,
                    std::optional<UiColor> tint={}) {
    if(content.images.contains(b.datum) && !content.images.at(b.datum).images.empty())
      b.position.y+=content.images.at(b.datum).images.front().height;
    // 2B9D59/2BA733 retains the selected rank page. Passing an explicit
    // zero here also discarded nonzero roster stripe selections.
    bitmap(b,origin,alpha,-1,tint);
  }
  static constexpr std::string_view title_logo =
      "ui\\screens\\game_shell\\start_screen\\start_screen";
  float layout_scale() const {
    return original_logical_layout ||
        (content.source_fonts.empty() && !imported_mcc_canvas) ? 1.f : .4f;
  }
  void forever_wordmark(float x, float y, float width, float height, float alpha) {
    if (alpha <= 0 || width <= 0 || height <= 0) return;
    // Match the website's pale-blue, widely spaced subtitle. Use the
    // imported font atlas so branding needs no additional runtime asset bank.
    constexpr std::string_view label = "FOREVER";
    const auto &f = font(1);
    float ink_width = 0;
    int top = 0, bottom = 0;
    for (const auto c : label) {
      const auto &g = f.glyphs.at(c);
      ink_width += g.width;
      top = std::max(top, g.bearing_y);
      bottom = std::min(bottom, g.bearing_y - int(g.height));
    }
    if (top <= bottom) return;
    const float scale = width * (9.f / 365.f) / float(top - bottom);
    const float span = width * .53f;
    const float tracking = (span - ink_width * scale) / float(label.size() - 1);
    const float baseline = y + height + width * (12.f / 365.f) + top * scale;
    const UiColor color{alpha, 182.f / 255, 217.f / 255, 247.f / 255};
    float cursor = x + (width - span) * .5f;
    for (const auto c : label) {
      const auto &g = f.glyphs.at(c);
      const auto [u0, v0, u1, v1] = retail_font_sample_uvs(f, c, 0, 0, g.width, g.height);
      quad(cursor, baseline - g.bearing_y * scale,
           g.width * scale, g.height * scale, font_material(1), color, u0, v0, u1, v1);
      cursor += g.width * scale + tracking;
    }
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
    if (original_mcc_frames.contains(material)) {
      // Map the authenticated MCC frame contours to the original shell,
      // independently of text/font placement. All sampled pixels stay MCC.
      detail::mcc_frame_geometry::regions([&](float dx, float dy, float w, float h,
                                             float u0, float v0, float u1, float v1) {
        quad(x+dx*sx, y+dy*sy, w*sx, h*sy, material, color, u0, v0, u1, v1);
      });
      if (const auto edges = frame_extensions.find(material); edges != frame_extensions.end()) {
        const auto sampling = frontend_frame_sampling(x,640*sx,0,1,
            mesh.materials.at(material).diffuse.width);
        for (unsigned edge = 0; edge < edges->second.size(); ++edge) {
          const auto edge_u = edge ? sampling.right_u : sampling.left_u;
          if (!edge_u) continue;
          detail::mcc_frame_geometry::edge_regions([&](float dy, float h, float v0, float v1) {
            quad(0, y+dy*sy, 640, h*sy, edges->second[edge], color,
                 *edge_u, v0, *edge_u, v1);
          });
        }
      }
      return;
    }
    quad(x, y, width * sx, draw_height, material,
         color, sprite.left + u, sprite.top + v,
         sprite.right + u, bottom_v);
    if (image.name == title_logo)
      forever_wordmark(x, y, width * sx, draw_height, color.alpha);
    if (const auto edges = frame_extensions.find(material); edges != frame_extensions.end()) {
      const float canvas_scale = original_logical_layout ||
          (content.source_fonts.empty() && !imported_mcc_canvas) ? 1.f : .4f;
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
  unsigned font_material(int index)const {
    return font_materials.at(index>=0&&index<int(fonts.size())?unsigned(index):1u);
  }
  std::string button_key(unsigned index)const {
    if(drawing_original_goal) {
      const auto &keys=detail::og_matchmaking_layout::data().at("original").at("button_keys");
      if(const auto it=keys.find(std::to_string(index));it!=keys.end())return it->get<std::string>();
    }
    return index&&index<=content.button_keys.size()?content.button_keys[index-1]:std::string{};
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
    std::map<std::size_t,unsigned> keyboard_prompt_sources;
    if (!button && (t.flags & 8) && owner &&
        (is_vista_menu(owner->screen) ||
         (content.original_matchmaking_layout && owner->screen == 210) ||
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
    if(prompts && prompt_device==PromptDevice::KeyboardMouse) {
      std::vector<unsigned> keys;
      for(auto c:chars) {
        std::string_view name;
        switch(c) {
        case 0xe100:case 0xe10c:name="[Enter]";break;
        case 0xe101:case 0xe10d:name="[Esc]";break;
        case 0xe102:name=owner && is_vista_settings(owner->screen,253)?"[Backspace]":"[X]";break;
        case 0xe103:name="[Y]";break;
        case 0xe105:
          if(owner && owner->screen==14){name="[Tab]";break;}
          keys.push_back(c);continue;
        default:keys.push_back(c);continue;
        }
        keyboard_prompt_sources.emplace(keys.size(),c);
        for(auto letter:name)keys.push_back(unsigned(letter));
      }
      chars=std::move(keys);
    }
    if (chars.empty())
      return;
    const auto key = animation(t.animation, t.delay);
    alpha *= key.alpha;
    if (alpha <= 0)
      return;
    const auto setup = frontend_text_setup(t.flags, t.font, button,
                                           content.typewriter.scale);
    const auto &raster = drawing_original_goal ? *this : font_owner ? *font_owner : *this;
    const auto &f = raster.font(setup.font);
    const unsigned material = raster.font_material(setup.font);
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
    if (prompts && owner && owner->screen == 9 && original_string_matches(t.string,0x0e001811))
      prompt_action = FrontendPointerAction::Accept;
    auto action_for = [&](unsigned c) -> std::optional<FrontendPointerAction> {
      using Action = FrontendPointerAction;
      switch (c) {
      case 0xe100: return Action::Accept;
      case 0xe101: case 0xe10d: return Action::Back;
      case 0xe102: return owner && is_vista_settings(owner->screen, 253)
          ? Action::ClearBinding : Action::Auxiliary;
      case 0xe103: return Action::Players;
      case 0xe105: return owner && owner->screen==14 ? std::optional{Action::PriorStats} : std::nullopt;
      case 0xe10c: return owner && owner->screen == 167 ? Action::CommitText : Action::Accept;
      default: return {};
      }
    };
    auto action_at = [&](std::size_t i) -> std::optional<FrontendPointerAction> {
      if(const auto source=keyboard_prompt_sources.find(i);source!=keyboard_prompt_sources.end())
        return action_for(source->second);
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
          auto uv=retail_font_sample_uvs(f,c,ix,iy,iw,ih);
          auto glyph_material=material;
          const auto selector=setup.font>=0 && setup.font<int(raster.fonts.size())?unsigned(setup.font):1u;
          if(const auto sample=raster.prompt_artwork->sample(selector,c,prompt_device,g,ix,iy,iw,ih)) {
            uv=sample->uv;glyph_material=sample->material;
          }
          const auto [u0,v0,u1,v1]=uv;
          // 1F490 shifts the already clipped glyph for its shadow, without
          // reclipping, and submits it before the foreground. 13F700 retains
          // atlas RGB for eligible controller/private-use glyphs.
          quad(gx + scale, gy + scale, width, height, glyph_material, shadow,
               u0, v0, u1, v1);
          const auto foreground = frontend_text_pixel_color(
              frontend_text_texture_color(c) ? pixel | 0xffffffu : pixel);
          quad(gx, gy, width, height, glyph_material, foreground, u0, v0, u1, v1);
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
  void shell_text(UiText t, const std::string &value, bool uppercase,
                  const MenuFrame *frame, bool prompts, const Impl *owner) {
    const auto first_vertex = output.vertices.size(), first_hit = hits.size();
    text(t, value, {}, 1, false, uppercase, frame, nullptr, prompts, owner);
    if (!owner || !imported_mcc_canvas) return;
    // Body widgets retain their authored PC canvas. Put shared shell text
    // into that canvas before its existing single projection, so the title,
    // footer and their pointer targets end at the original logical anchors.
    const auto inverse_x = [](float x) { return 320+(x-320)/(480.f/1200.f); };
    const auto inverse_y = [](float y) { return 240+(y-240)/(480.f/1200.f); };
    for (auto i = first_vertex; i < output.vertices.size(); ++i) {
      auto &p = output.vertices[i].position;
      p.x = inverse_x(p.x); p.y = inverse_y(p.y);
    }
    for (auto i = first_hit; i < hits.size(); ++i) {
      auto &h = hits[i];
      h.left=inverse_x(h.left); h.right=inverse_x(h.right);
      h.top=inverse_y(h.top); h.bottom=inverse_y(h.bottom);
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
                     unsigned depth = 0,std::optional<Vec3> primary_override = {}) {
    if (depth > 16)
      throw FormatError("cyclic frontend skin group");
    const bool create = entry.destination == 167;
    for (auto b : skin.bitmaps) {
      // 24A09B -> 2AFEAE: Create New supplies value5e; each saved row
      // supplies its own profile appearance, independently of focus/sign-in.
      if (original_string_matches(b.placeholder,0x0e00069b) && !create)
        continue;
      if ((original_string_matches(b.placeholder,0x1400069a) && !appearance) ||
          original_string_matches(b.placeholder,0x0900069c))
        continue;
      b.animation = 0;
      b.delay = 0;
      if (content.images.contains(b.datum) &&
          !content.images.at(b.datum).images.empty())
        b.position.y += content.images.at(b.datum).images.front().height;
      if (original_string_matches(b.placeholder,0x1400069a)) {
        const auto rgb = primary_override.value_or(frontend_appearance_colors(
            *appearance, content.appearance_palette)[0]);
        bitmap(b, p, opacity, 0, UiColor{1, rgb.x, rgb.y, rgb.z});
      } else {
        bitmap(b, p, opacity);
      }
    }
    for (const auto &grid : skin.grids)
      if (content.skin_groups.contains(grid.skin))
        cells(grid, p, [&](Vec2 origin) {
          profile_group(content.skin_groups.at(grid.skin), origin, opacity, entry,
                        appearance, depth + 1,primary_override);
        });
    // 2B0080 copies a saved name into the nested field and applies the
    // original primary-color HSV conversion. Create New uses the outer field.
    if (appearance && !create)
      for (auto t : skin.texts)
        if (original_string_matches(t.string,0x0b000698)) {
          const auto rgb = primary_override?source_profile_name_color(*primary_override):
              frontend_appearance_name_color(*appearance, content.appearance_palette);
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
          if(detail::frontend_pc_compiled_matches(content,fields->texts[i].string,0x0b000696) && i==0)
            texts[i].color=pcr_color(player->name_color);
      };
      vista_pcr_bind_row(VistaPcrPane(frame.context),row,*owner.results,services,owner.floating_point,
          content.original_matchmaking_layout?VistaPcrRowStyle::original_xbox:VistaPcrRowStyle::vista);
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
          if(detail::frontend_pc_compiled_matches(content,bitmap.placeholder,0x14000698)) {
            visible=player->primary_visible;image=int(player->primary_frame);color=pcr_color(player->primary_color);
          } else if(detail::frontend_pc_compiled_matches(content,bitmap.placeholder,0x0e000699))
            visible=player->secondary_visible;
          else if(detail::frontend_pc_compiled_matches(content,bitmap.placeholder,0x0900069a)) {
            visible=player->rank_visible;image=int(player->rank_frame);
          } else throw FormatError("Unbound PC PCR player bitmap placeholder");
        } else if(bitmap.flags&2)image=row==frame.selected?0:row<frame.selected?1:2;
        if(visible)draw_bitmap(bitmap,field_origin,image,color);
      }
      if(player && player->emblem_visible && !fields->emblems.empty()) {
        if(content.original_matchmaking_layout) {
          FrontendAppearance appearance;
          static_assert(sizeof appearance==9);
          std::memcpy(&appearance,player->appearance.data(),sizeof appearance);
          if(appearance.species!=0 && appearance.species!=1)
            throw FormatError("Original PCR appearance has an invalid species");
          unsigned instance=0;
          profile_emblems(*fields,field_origin,focus.alpha,appearance,
              profile_emblem_materials.at(&list).at(row),instance);
        } else {
        // The PC spas definition and full16 appearance bytes are retained.
        // The Xbox waypoint pass has a different bytecode owner and cannot
        //render this PC emblem without a recovered PC shader implementation.
        constexpr const char *gap="Vista PCR PC emblem shader is retained but its native spas pass is unresolved";
        if(std::find(output.rendering_gaps.begin(),output.rendering_gaps.end(),gap)==output.rendering_gaps.end())
          output.rendering_gaps.emplace_back(gap);
        }
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
        FrontendLobbyRosterInput input;
        input.pane=lobby_policy->pane;input.valid_session=1;
        input.session_present=true;input.matchmaking=input.session_enum_is_six=true;
        input.tu15=true;
        input.players=lobby->party_players;input.appearance_palette=content.appearance_palette;
        roster=frontend_lobby_roster(input);
        for(unsigned i=0;i<lobby->party_speakers.size();++i) {
          auto &row=roster->at(i);
          row.speaker_visible=lobby->party_speakers[i]<=2;
          row.speaker_frame=lobby->party_speakers[i];
        }
        // Pregame25042D hides absent party slots. The matched/empty rows
        // from progress257A11/257A86 belong to screen210, not this lobby.
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
    auto *profile_owner = pc_profile_owner ? pc_profile_owner : vista.get();
    const bool pc_link = profile_owner &&
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
      if (wide_social_page(frame) && index == 2) continue;
      if (online_vista_art && frame.screen==186 && index==3 && frame.selected==3) continue;
      if (frame.screen==210 && !content.original_matchmaking_layout && content.images.contains(b->datum)) {
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
      if(lobby && lobby->party_names && lobby_policy->pane!=2 && (index==2 || index==4))continue;
      if (roster && index >= 21 && index <= 36 &&
          !(*roster)[index-21].speaker_visible) continue;
      auto image = *b;
      if (wide_social_page(frame) && index == 3)
        image.position.x += social_width_extra();
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
      if(image.matchmaking_map_preview) {
        const auto map=state.online_matchmaking_map_bitmap();
        if(!map) {
          output.rendering_gaps.emplace_back("Original matched-map preview has no qualified scenario bitmap owner");
          continue;
        }
        const auto original=detail::og_matchmaking_layout::map_preview_datum(content,*map);
        if(!original) {
          output.rendering_gaps.emplace_back("Original matched-map preview has no authenticated source raster");
          continue;
        }
        image.datum=*original;image.frame=0;
      }
      if(content.original_matchmaking_layout && frame.screen==210 &&
          state.online_matchmaking_progress_pane()==3 && index==12) {
        const auto game=state.online_matchmaking_game_icon();if(!game)continue;
        image.frame=int(*game);
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
      if (wide_social_page(frame) && (index == 0 || index == 3)) {
        // The source Y-menu body is512 high, with visible alpha to409.
        // At its authored y74 it reaches483, below the native480 canvas.
        // Finish this standalone clan panel beneath its actual footer,
        // cropping UVs with the geometry so the source art is not stretched.
        const auto &footer = pane.texts.at(1).bounds;
        const auto &metadata = pane.texts.at(2).bounds;
        clip_bottom = 240 - footer.bottom + (metadata.top - metadata.bottom) * .5f;
      }
      if ((frame.screen == 6 || frame.screen == 9) && content.images.contains(image.datum) &&
          content.images.at(image.datum).name == title_logo)
        image.position.y += 36.f / layout_scale();
      bitmap(image, parent, 1, -1, {}, clip_bottom);
      if (interactive && frame.vista_pcr && content.images.contains(image.datum) &&
          content.images.at(image.datum).name == "ui\\screens\\game_shell\\postgame_statistics\\spinners" &&
          (image.frame == 0 || image.frame == 1) && output.vertices.size() > first) {
        // The actual pane's left/right spinners own their pointer geometry.
        // Use the emitted, animated image bounds so PC/MCC projection and
        // viewport transforms apply identically to artwork and input.
        float left=output.vertices[first].position.x, right=left;
        float top=output.vertices[first].position.y, bottom=top;
        for(auto i=first;i<output.vertices.size();++i) {
          const auto &p=output.vertices[i].position;
          left=std::min(left,p.x);right=std::max(right,p.x);
          top=std::min(top,p.y);bottom=std::max(bottom,p.y);
        }
        hits.push_back({left,top,right,bottom,unsigned(image.frame),FrontendPointerAction::PcrTab});
      }
      if (network_browser && mcc_browser && index == 0)
        extend_bitmap(first, {0, browser_frame_height_delta});
      if (wide_social_page(frame)) {
        const float body_extra = canvas.surface_height / canvas.scale - 424;
        if (index == 0) extend_bitmap(first, {social_width_extra(), body_extra});
        if (index == 1) extend_bitmap(first, {social_width_extra(), 0}, 209, 4);
        if (index == 3) extend_bitmap(first, {0, body_extra});
      }
      if (frame.screen == 167)
        keyboard_widget(first, b->layer, 8,
                        animation(b->animation, b->delay).position.z);
      if (appearance_popup && output.vertices.size() > first)
        popup_widgets.push_back({first, output.vertices.size(), b->layer, 8,
                                animation(b->animation, b->delay).position.z});
    }
    if(pc_link && frame.screen==38) {
      auto &pc = *profile_owner;
      pc.output={};pc.seconds=seconds;pc.ui_clock_ms=ui_clock_ms;
      const auto &pc_pane=pc.content.screen(38).panes.at(2);
      for(const auto &b:pc_pane.bitmaps)pc.bitmap(b);
      for (std::size_t i = 1; i < pc_pane.texts.size(); ++i) {
        const auto &t = pc_pane.texts[i];
        pc.text(t, state.expand(pc.content.localized(pc.content.screen(38), t.string)),
                    {}, 1, false, false, &frame, &t);
      }
      for(auto vertex:pc.output.vertices) {
        vertex.position.x=320+(vertex.position.x-320)*.4f;
        vertex.position.y=240+(vertex.position.y-240)*.4f;
        output.vertices.push_back(vertex);
      }
    }
    for (unsigned i = 0; i < pane.scenes.size(); ++i) {
      const auto &scene = pane.scenes[i];
      // Original22E3B4 gates drawing until animation.end_time. Its offset
      // remains zero before the authored delay; submitting that default
      // position flashes the final pose before the slide starts. Model clocks
      // and constructor requests remain independent in frontend.cpp.
      if (!drawing_exit && seconds*1000.f < float(scene.delay))
        continue;
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
      const UiScene *definition = &scene;
      if (const auto viewport = detail::frontend_mcc_model_viewport(content, screen, scene)) {
        auto [entry, inserted] = model_viewports.try_emplace(&scene, scene);
        if (inserted) entry->second.viewport = *viewport;
        definition = &entry->second;
      }
      output.scenes.push_back({definition, output.vertices.size(),
                              animation(scene.animation, scene.delay).position,
                              appearance});
    }
    for (unsigned i = 0; i < pane.texts.size(); ++i) {
      // The native Xbox lobby has no PC browser/download producer. Its MCC
      // definition additionally authors this percent template; the recovered
      // PC producer hides it at the quiescent 100% state as well.
      if (frame.screen == 14 && lobby_policy && detail::mcc_frontend_content(content) &&
          detail::frontend_pc_compiled_matches(content, pane.texts[i].string, 0x15000916))
        continue;
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
          lobby->protocol, lobby->machines, lobby->input.party_privacy, lobby->input.session_enum)
          : FrontendLobbyHelp{};
      if (help.text_index == i && !help.visible) continue;
      // 24A886 hides runtime text(6,3), authored pane text1, in every case.
      // The focused name is supplied to the separate player-skin group.
      if (frame.screen == 193 && i == 1) continue;
      auto t = pane.texts[i];
      if(frame.vista_pcr) {
        //40CE6A hides the reached empty-report field whenever the retained
        //signed PC player count is positive. PC185 has no such child.
        if(detail::frontend_pc_compiled_matches(content,t.string,0x1d001a71) && std::bit_cast<std::int32_t>(frame.vista_pcr->results->player_count)>0)continue;
        const auto headings=vista_pcr_headings(VistaPcrPane(frame.context),frame.vista_pcr->results->variant);
        if(i<headings.size() && headings[i])
          t.string=detail::frontend_pc_compiled_string_id(content,*headings[i]).value_or(0xffffffffu);
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
        t.string = detail::frontend_compiled_string_id(content, t.string).value_or(0xffffffffu);
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
      if(content.original_matchmaking_layout && (frame.screen==219 || frame.screen==237) && frame.context==0 && (i==1 || i==2)) {
        auto bounded=standard_text_buffer(value);const auto capacity=i==1?15u:127u;
        if(bounded.size()>capacity)bounded.resize(capacity);
        value=pcr_utf8(bounded); //2B225A's actual16/128-unit locale buffers.
      }
      if (frame.screen == 210 && content.original_matchmaking_layout)
        value=state.online_matchmaking_original_text(state.online_matchmaking_progress_pane(),i);
      else if (frame.screen == 210 && (content.source_fonts.empty() || i == 0))
        value = state.online_matchmaking_text(frame, i);
      // TU1.5 pane3 authors no tip callouts; its text0 replaces them. The PC
      // pane2 frame hosting it keeps only its main text record.
      if (frame.screen == 210 && !content.original_matchmaking_layout && !content.source_fonts.empty() && i > 0 &&
          state.online_matchmaking_progress_pane() == 3)
        value.clear();
      if (state.online_clan_screen(frame)) value = state.online_clan_text(frame,i);
      if (wide_social_page(frame)) {
        // Native membership replaces the source player's rank columns.
        // Give the labelled count and microphone state their whole columns.
        if (frame.context == 1 && i == 2) t.flags &= ~1u;
        if (frame.context == 1 && i == 4) t.bounds.right = pane.texts.at(6).bounds.left;
        if (frame.context == 1 && i == 6) t.bounds.right = pane.texts.at(7).bounds.right;
        if (i == 1 || i >= 4) {
          t.bounds.left += social_width_extra();
          t.bounds.right += social_width_extra();
        }
        if (i == 1) {
          t.bounds.top -= social_footer_offset();
          t.bounds.bottom -= social_footer_offset();
        }
      }
      if(frame.screen==vista_menu(297) && detail::frontend_pc_compiled_low_matches(content,t.string,0x979))
        value=detail::mcc_frontend_content(content)?"Halo 2 Native\r\nMCC Classic menu assets"
            :"Halo 2 Native\r\nVista settings assets: 11081.07.04.30.0934.main";
      if(frame.screen==vista_menu(296) && i==0)value=frame.description;
      if (pc_link && i == 0)
        value = detail::frontend_pc_compiled_localized(profile_owner->content,
            profile_owner->content.screen(frame.screen), frame.screen == 19 ? 0x210019e2u : 0x220019fcu);
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
            if (detail::frontend_pc_compiled_low_matches(content,t.string,sid))
              binding = pc_binding_name(state.pc_keyboard_settings().bindings[unsigned(action)]);
        value = binding + value;
      }
      if (focused_map && i == 0) value = focused_map->description;
      if (setup && i < setup->strings.size())
        value = detail::frontend_compiled_localized(content,screen,setup->strings[i]);
      if (lobby_policy) {
        if (i == 0 && lobby_policy->status_string)
          value = detail::frontend_compiled_localized(content,screen,*lobby_policy->status_string);
        if (lobby_policy->privacy_text == i && lobby_policy->privacy_string)
          value = detail::frontend_compiled_localized(content,screen,*lobby_policy->privacy_string);
        if (lobby_policy->pane == 0 && i >= 4 && i <= 8)
          value = lobby->quick_options[i-4].value_or("");
        if (help.text_index == i && help.visible)
          value = detail::frontend_compiled_localized(content,screen,help.string_id);
      }
      if (signin) {
        if (signin->text_strings[i])
          value = detail::frontend_compiled_localized(content,screen,signin->text_strings[i]);
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
              if (original_string_matches(b.placeholder,0x1400069a)) {
                visible = row.background_visible;
                b.frame = int(row.background_frame);
                tint = UiColor{1, row.primary_color.x, row.primary_color.y, row.primary_color.z};
              } else if (original_string_matches(b.placeholder,0x0e00069b))
                visible = row.secondary_background_visible;
              else if (original_string_matches(b.placeholder,0x0900069c)) {
                visible = row.rank_visible;
                b.frame = int(row.rank_frame);
              }
              if (!visible) continue;
              group_bitmap(b,origin,1,tint);
            }
            for (auto t : group.texts)
              if (original_string_matches(t.string,0x0b000698)) {
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
                text(t, row.name_string ? detail::frontend_compiled_localized(content,screen,*row.name_string) : row.name, origin);
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
              if (original_string_matches(b.placeholder,0x1400069a)) {
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
              if (original_string_matches(t.string,0x0b000698)) {
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
                if (original_string_matches(t.string,0x0b000698)) {
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
                if (original_string_matches(b.placeholder,0x0e00069b)) {
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
    // The original category popup207 likewise authors its persistent list
    // only in pane0. MCC adds empty pane-local placeholders to later panes;
    // those must not replace the screen's primary list on a focus change.
    const auto &lists = frame.screen == 186 || frame.screen == 207
        ? screen.panes.front().lists : pane.lists;
    if (!lists.empty() && !frame.entries.empty()) {
      std::optional<UiList> title_list;
      if (frame.screen == 6) {
        // Keep every title option visible, including Exit Game, with the
        // same placement for row artwork, glyphs and pointer rectangles.
        title_list = lists.front();
        title_list->visible = std::max(title_list->visible, unsigned(frame.entries.size()));
        title_list->position.y += 18.f / layout_scale();
      }
      const auto &list = title_list ? *title_list : lists.front();
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
      if (profile_skin || (content.original_matchmaking_layout && frame.screen == 210) ||
          frame.screen == 26 || frame.screen == 49 || frame.screen == 52 ||
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
        const auto matchmaking_speaker=frame.screen==210 && content.original_matchmaking_layout
            ? state.online_matchmaking_speaker(index) : std::optional<unsigned>{};
        for (auto b : skin.bitmaps) {
          const auto social_bitmap=social_bitmap_index++;
          // The directory owns no source host/custom-map status icons.
          // Retain the PC13 selection background/end caps and column dividers.
          if (network_browser && social_bitmap >= 2 && social_bitmap <= 4) continue;
          // The queue API has no voice/presence state for these player rows.
          if ((frame.screen == 210 && !matchmaking_speaker) || (frame.screen == 26 && social_bitmap >= 2)) continue;
          b.animation = 0;
          b.delay = 0;
          // List skin anchors are bottom-left (screen bitmap anchors are
          // top-left). The original tags deliberately use different origins.
          if (content.images.contains(b.datum) &&
              !content.images.at(b.datum).images.empty())
            b.position.y += content.images.at(b.datum).images.front().height;
          const auto first_bitmap = output.vertices.size();
          bitmap(b, p, opacity,matchmaking_speaker?int(*matchmaking_speaker):
                 (b.flags & 2) ? (drawing_exit ? 1 : selected ? 0
                                  : index < frame.selected ? 1
                                                         : 2)
                               : b.frame);
          if (wide_social_page(frame) &&
              content.images.contains(b.datum) &&
              content.images.at(b.datum).images.front().width > 200)
            extend_bitmap(first_bitmap, {social_width_extra(), 0});
        }
        if (profile_skin && !(wide_social_page(frame))) {
          const auto appearance = state.profile_entry_appearance(frame, index);
          for (const auto &grid : skin.grids)
            if (content.skin_groups.contains(grid.skin))
              cells(grid, p, [&](Vec2 origin) {
                profile_group(content.skin_groups.at(grid.skin), origin,
                              opacity, frame.entries[index], appearance);
              });
        }
        if(frame.screen==210) {
          const auto kind=frame.entries[index].value;
          if(content.original_matchmaking_layout && kind!=matchmaking_matched_player_row && kind!=matchmaking_empty_player_row) {
            const auto appearance=state.online_matchmaking_player_appearance(index);
            std::optional<Vec3> primary;
            if(state.online_matchmaking_team_colors() && kind>=0 && kind<8)
              primary=frontend_lobby_team_colors[unsigned(kind)];
            for(const auto &grid:skin.grids)if(content.skin_groups.contains(grid.skin))
              cells(grid,p,[&](Vec2 origin) {
                const auto &group=content.skin_groups.at(grid.skin);
                profile_group(group,origin,opacity,frame.entries[index],appearance,0,primary);
                if(appearance) {
                  unsigned instance=0;profile_emblems(group,origin,opacity,*appearance,
                      profile_emblem_materials.at(&list).at(index),instance);
                } else for(auto t:group.texts) {
                  t.animation=t.delay=0;t.color={1,1,1,1};
                  if(primary){const auto rgb=source_profile_name_color(*primary);t.color={1,rgb.x,rgb.y,rgb.z};}
                  text(t,frame.entries[index].label,origin,opacity);
                }
              });
          }
          if(kind==matchmaking_matched_player_row || kind==matchmaking_empty_player_row)
            for(const auto &grid:skin.grids)
              if(content.skin_groups.contains(grid.skin))
                cells(grid,p,[&](Vec2 origin) {
                  const auto &group=content.skin_groups.at(grid.skin);
                  // TU1.5 257A11/257A86: group bitmap1 is player_in,
                  // page1 for a matched player and page0 for an empty slot.
                  // Item flags20 show bitmap3, the secondary background.
                  for(const auto child:{1u,3u})if(child<group.bitmaps.size()) {
                    auto b=group.bitmaps[child];
                    b.frame=child==1 && kind==matchmaking_matched_player_row?1:0;
                    b.animation=b.delay=0;
                    group_bitmap(b,origin,opacity);
                  }
                });
          if(const auto rank=state.online_matchmaking_rank(index))
            for(const auto &grid:skin.grids)
              if(content.skin_groups.contains(grid.skin))
                cells(grid,p,[&](Vec2 origin) {
                  for(auto b:content.skin_groups.at(grid.skin).bitmaps)
                    if(content.images.contains(b.datum) &&
                        content.images.at(b.datum).name=="ui\\global_bitmaps\\rank_icons_sm") {
                      // TU1.5 257BDC ->2B9EB2 ->2B9D59 supplies the
                      // selected rank page; retain this skin's own geometry.
                      b.frame=int(*rank);b.animation=b.delay=0;
                      group_bitmap(b,origin,opacity);
                    }
                });
        }
        for (unsigned j = 0; j < skin.texts.size(); ++j) {
          // 2517E2 hides outer text0 for actual players; their name belongs
          // to group0. 251703 shows it only for an anonymous matched slot.
          if(frame.screen==210 && content.original_matchmaking_layout &&
              frame.entries[index].value!=matchmaking_matched_player_row)continue;
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
          if (frame.screen == 210 && j == 0 && state.online_matchmaking_team_colors()) {
            // TU1.5 2517E2 ->2AFEAE: a team row's name follows its team
            // color once the countdown exists; team NONE keeps the default.
            const int team = frame.entries[index].value;
            if (team >= 0 && std::size_t(team) < frontend_lobby_team_colors.size() - 1) {
              const auto color = source_profile_name_color(frontend_lobby_team_colors[std::size_t(team)]);
              t.color.red = color.x; t.color.green = color.y; t.color.blue = color.z;
            }
          }
          if (wide_social_page(frame))
            t.bounds.right += social_width_extra();
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
            value=original_low_localized(screen,low);
          }
          // Vista6468BF supplies a zero list descriptor, so the original
          // 210 loader skips this authored roster. Its20-unit box cannot fit
          // the PC font's29-unit ascent. The native admitted-player roster
          // explicitly uses existing logical metrics with Vista glyph samples;
          // preserve the PC list/skin geometry and every other PC text field.
          const auto *roster_fonts = frame.screen == 210 && list.skin == 20
              ? online_roster_fonts : nullptr;
          if(content.original_matchmaking_layout && (frame.screen==219 || frame.screen==237) && frame.context==0) {
            auto bounded=standard_text_buffer(value);if(bounded.size()>15)bounded.resize(15);value=pcr_utf8(bounded);
          }
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
          if (wide_social_page(frame)) {
            // The complete visible selection bar is a mouse target, including
            // the empty area beyond the member name and role.
            for (const auto &b : skin.bitmaps)
              if ((b.flags & 2) && content.images.contains(b.datum) &&
                  !content.images.at(b.datum).images.empty()) {
                const auto &image = content.images.at(b.datum).images.front();
                l = b.position.x;
                r = l + image.width + social_width_extra();
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
        if(is_vista_settings(frame.screen,256) || (is_vista_settings(frame.screen,272) &&
            (frame.context==video_fov_context || frame.context==video_vehicle_fov_context))) {
          const auto bounds=skin_bounds(skin);
          if(content.images.contains(b.datum) && !content.images.at(b.datum).images.empty())
            b.position.y=bounds.top+float(content.images.at(b.datum).images.front().height)+4.f;
        }
        bitmap(b, {parent.x + list.position.x, parent.y + list.position.y});
      }
      if (first + count < frame.entries.size()) {
        UiBitmap b;
        b.datum = skin.arrows;
        b.position = skin.arrow_down;
        if(is_vista_settings(frame.screen,256) || (is_vista_settings(frame.screen,272) &&
            (frame.context==video_fov_context || frame.context==video_vehicle_fov_context)))
          b.position.y=skin_bounds(skin).bottom-float(count-1)*row-4.f;
        if (wide_social_page(frame))
          b.position.y -= social_footer_offset();
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
    const auto goal_owner=[&] {
      for(auto it=state.stack().rbegin();it!=state.stack().rend();++it) {
        if(detail::og_matchmaking_layout::goal_frame(*it,&state))return true;
        if(!(navigation_content->screen(state.screen_layout(*it)).flags&(1|8|16)))return false;
      }return false;
    };
    struct FontScope {bool&flag;bool previous;~FontScope(){flag=previous;}} scope{drawing_original_goal,drawing_original_goal};
    drawing_original_goal=content.original_matchmaking_layout &&
      (detail::og_matchmaking_layout::goal_frame(frame,&state)||(frame.screen==5&&goal_owner()));
    if (logical && logical_view->content.screens.contains(layout) &&
        (!detail::og_matchmaking_layout::data().at("original").at("screens").contains(std::to_string(layout)) ||
         frame.screen==14 || logical_view->other_goal_dialogs.contains(frame.screen) || detail::og_matchmaking_layout::goal_frame(frame,&state))) {
      logical->output = {};
      logical->hits.clear();
      logical->seconds = seconds;
      logical->ui_clock_ms = ui_clock_ms;
      logical->canvas = canvas;
      logical->drawing_exit = drawing_exit;
      logical->exit_elapsed_ms = exit_elapsed_ms;
      logical->online_vista_art = online_vista_art;
      logical->screen(frame, state, interactive, override_pane);
      const auto first_vertex = output.vertices.size();
      output.vertices.insert(output.vertices.end(), logical->output.vertices.begin(),
                             logical->output.vertices.end());
      hits.insert(hits.end(), logical->hits.begin(), logical->hits.end());
      for (auto scene : logical->output.scenes) {
        scene.before_vertex += first_vertex;
        output.scenes.push_back(std::move(scene));
      }
      output.rendering_gaps.insert(output.rendering_gaps.end(),
          logical->output.rendering_gaps.begin(), logical->output.rendering_gaps.end());
      return;
    }
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
        scene.vista_viewport=!vista->original_model_viewport(scene.definition);
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
    const auto mcc_first_vertex = output.vertices.size(), mcc_first_hit = hits.size(),
               mcc_first_scene = output.scenes.size();
    if (frame.screen == 167)
      keyboard_widgets.clear();
    const auto &s = other_goal_dialogs.contains(frame.screen)&&!detail::og_matchmaking_layout::goal_frame(frame,&state)?*other_goal_dialogs.at(frame.screen):
        frame.screen==14&&!state.online_party_screen(frame)&&other_lobby?*other_lobby:
        mcc_browser && layout == vista_menu(13)?*mcc_browser:
        frame.screen==5&&original_goal_background&&goal_owner()?*original_goal_background:content.screen(layout);
    const auto lobby = frame.screen == 14 ? state.lobby_view() : std::nullopt;
    const auto setup = frame.screen == 23 ? state.lobby_view() : std::nullopt;
    const bool dialog = s.flags & (1 | 8 | 16);
    const Impl *shell_owner = !dialog && !state.online_clan_screen(frame) &&
        !state.online_party_screen(frame) && !state.online_friends_screen(frame)
        ? shell_fonts : nullptr;
    if (!s.panes.empty()) {
      const auto index =
          override_pane >= 0
              ? std::min(unsigned(override_pane), unsigned(s.panes.size() - 1))
              : (frame.vista_pcr ? frame.context
                 : lobby
                     ? unsigned(frontend_lobby_pane(lobby->input.session_enum, 0))
                 : frame.screen == 210
                     ? content.original_matchmaking_layout ? state.online_matchmaking_progress_pane()
                       : content.source_fonts.empty() ? 1u : state.online_matchmaking_pane()
                 : frame.screen == 26
                     ? std::min(frame.context, unsigned(s.panes.size() - 1))
                 : (s.flags & 2)
                     ? std::min(frame.selected, unsigned(s.panes.size() - 1))
                     : 0);
      const auto first_vertex = output.vertices.size(), first_hit = hits.size();
      // TU1.5 257E88 reconstructs the progress pane on every pane change.
      // Its authored child intros restart; shared screen chrome does not.
      const auto screen_seconds = seconds;
      if(content.original_matchmaking_layout && frame.screen == 210)
        seconds=float(ui_clock_ms-frame.pane_created_clock)/1000.f;
      pane(s, s.panes.at(index), frame, state, interactive);
      seconds=screen_seconds;
      if (wide_social_page(frame)) {
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
      const auto &shell = shell_owner ? shell_owner->content : content;
      header.font = shell.header_fonts[type];
      if (drawing_exit && !s.panes.empty())
        header.animation = s.panes.front().animation; //22FDA6 pane_value
      header.bounds = shell.header_bounds[type * 2];
      header.flags = 1;
      header.color = content.color;
      const auto first = output.vertices.size();
      shell_text(header,
           (state.online_clan_screen(frame) || state.online_party_screen(frame) || state.online_friends_screen(frame) || frame.screen == 20 || frame.screen == 186 || frame.screen == 167 || frame.screen == 210 || frame.screen == 219 || frame.screen == 237) ? frame.title : state.expand(frame.title.empty() ? content.localized(s, s.header)
                                            : frame.title), true, nullptr, false, shell_owner);
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
      t.bounds = (shell_owner ? shell_owner->content : content).header_bounds[type * 2 + 1];
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
      shell_text(t, frame.vista_pcr?pcr_footer():state.expand(lobby
          ? detail::frontend_compiled_localized(content,s,frontend_lobby_presentation(lobby->input).subtitle_string)
          : setup ? detail::frontend_compiled_localized(content,s,frontend_lobby_setup_subtitle(setup->input.session_enum))
          : (frame.screen == 219 || frame.screen == 237) && frame.context == 0 && content.original_matchmaking_layout ? button_key(s.button_key)
          : frame.screen == 219 && frame.context == 0 ? "\xee\x84\x80 SELECT  \xee\x84\x81 BACK"
          : frame.screen == 237 && frame.context == 0 ? "\xee\x84\x80 SELECT  \xee\x84\x81 BACK"
          : frame.screen == 219 ? (frame.entries.size() == 1 && frame.entries.front().destination == -13
              ? "\xee\x84\x80 REFRESH  \xee\x84\x81 BACK  \xee\x84\x82 REFRESH"
              : "\xee\x84\x80 DETAILS  \xee\x84\x81 BACK  \xee\x84\x82 REFRESH")
          : frame.screen == 237 ? "\xee\x84\x81 BACK"
          : button_key(s.button_key)), false, &frame, interactive, shell_owner);
      if (frame.screen == 167)
        keyboard_widget(first, 32767, 6, 0);
    }
    if (frame.screen == 6) {
      UiText t;
      t.flags = 2;
      t.font = 1;
      t.color = content.color;
      t.bounds = content.header_bounds[1];
      text(t, detail::frontend_compiled_localized(content, s, 0x08001682), {}, 1, false, false, &frame, nullptr, interactive);
    }
    if (frame.screen == 167 && !s.panes.empty()) {
      const auto first = output.vertices.size();
      keyboard_overlay(s.panes.front(), state);
      keyboard_widget(first, 0, 0,
          animation(s.panes.front().animation).position.z);
      keyboard_order();
    }
    if (imported_mcc_canvas && !parent_projects_pc_canvas && !state.online_clan_screen(frame) &&
        !state.online_party_screen(frame) && !state.online_friends_screen(frame)) {
      // MCC retains the PC widget coordinate bank. Reuse the same authored
      // height/1200 projection as the separate PC content above, including
      // pointer rectangles and model viewports. Native 640-unit social pages
      // and every Xbox-source painter retain their existing coordinates.
      constexpr float scale = 480.f / 1200.f;
      for (auto i = mcc_first_vertex; i < output.vertices.size(); ++i) {
        auto &p = output.vertices[i].position;
        p.x = 320 + (p.x - 320) * scale;
        p.y = 240 + (p.y - 240) * scale;
      }
      for (auto i = mcc_first_hit; i < hits.size(); ++i) {
        auto &h = hits[i];
        h.left = 320 + (h.left - 320) * scale;
        h.right = 320 + (h.right - 320) * scale;
        h.top = 240 + (h.top - 240) * scale;
        h.bottom = 240 + (h.bottom - 240) * scale;
      }
      for (auto i = mcc_first_scene; i < output.scenes.size(); ++i)
        output.scenes[i].vista_viewport = !original_model_viewport(output.scenes[i].definition);
    }
    // Original2353A5 calls PCR16's v23 AFTER its sorted child draws.
    // Upper channels (including the source27 Friends footer) follow this
    // screen; the full-window black command never scales individual rows.
    if(const auto color=state.pcr_intro_color(frame)) {
      const bool projected=parent_projects_pc_canvas;
      quad(projected?-480.f:0.f,projected?-360.f:0.f,
          projected?1600.f:640.f,projected?1200.f:480.f,pcr_black,
          {float(*color>>24)/255.f,0,0,0});
    }
  }
  bool original_model_viewport(const UiScene *scene) const {
    return std::any_of(model_viewports.begin(), model_viewports.end(),
        [&](const auto &entry) { return &entry.second == scene; });
  }
  void invite_overlays(const MenuState &state) {
    // These are native service controls. Use the same authenticated project
    // prompt banks as the rest of the frontend, including PS artwork.
    const auto label=[&](const std::string &value,float x,float y,float width,float height,UiColor color=UiColor{}) {
      UiText t;t.font=0;t.color=color;t.bounds={240-y,x-320,240-y-height,x+width-320};
      text(t,value);
    };
    if(state.party_invite_picker()) {
      constexpr float left=50,top=62,width=270,height=30;
      for(unsigned tab=0;tab<2;++tab) {
        const float x=left+float(tab)*width;
        const bool selected=tab==state.party_invite_tab();
        quad(x,top,width-2,height,white,selected?UiColor{1,.12f,.32f,.48f}:UiColor{1,.025f,.09f,.15f});
        const auto icon=prompt_device==PromptDevice::KeyboardMouse?std::string(tab?"[PgDn] ":"[PgUp] "):
          std::string(tab?"\xee\x84\x85 ":"\xee\x84\x84 ");
        label(icon+(tab?"CLAN MEMBERS":"FRIENDS"),x+7,top+5,width-14,24);
        hits.push_back({x,top,x+width-2,top+height,tab,FrontendPointerAction::InviteTab});
      }
    }
    if(const auto notice=state.invite_notice())paint_invite_notice(*notice);
  }
  void paint_invite_notice(const FrontendInviteNotice &value) {
    const auto first=output.vertices.size();
    const auto *notice=&value;
    const auto label=[&](const std::string &value,float x,float y,float width,float height,UiColor color=UiColor{}) {
      UiText t;t.font=0;t.color=color;t.bounds={240-y,x-320,240-y-height,x+width-320};text(t,value);
    };
    const float right=(canvas.surface_width-canvas.left)/canvas.scale-12;
    const float x=right-300,y=-canvas.top/canvas.scale+12;
    quad(x,y,300,104,white,{.97f,.02f,.07f,.13f});
    quad(x,y,3,104,white,{1,.25f,.65f,.9f});
    label("PARTY INVITATION",x+12,y+7,276,22,{1,.6f,.85f,1});
    label(notice->name,x+12,y+30,276,24);
    const auto accept=prompt_device==PromptDevice::KeyboardMouse?std::string("[F9] "):std::string("\xee\x84\x91 ");
    const auto dismiss=prompt_device==PromptDevice::KeyboardMouse?std::string("[F10] "):std::string("\xee\x84\x90 ");
    label(notice->accepting?"Updating party...":notice->can_accept?accept+"Accept":"Leave party or match to accept",x+12,y+64,170,32,
          notice->can_accept?UiColor{}:UiColor{1,.5f,.6f,.7f});
    label(dismiss+"Dismiss",x+188,y+64,108,32);
    if(notice->can_accept)hits.push_back({x+8,y+58,x+183,y+102,0,FrontendPointerAction::InviteAccept});
    hits.push_back({x+184,y+58,right-4,y+102,0,FrontendPointerAction::InviteDismiss});
    // Only the notice uses a full-screen pass. Preserve the underlying menu's
    // canvas/scissor and its original frame-extension arithmetic.
    for(auto i=first;i<output.vertices.size();++i) {
      auto &v=output.vertices[i];
      auto found=notice_materials.find(v.material);
      if(found==notice_materials.end())throw FormatError("Party notice material was not reserved before renderer creation");
      v.material=found->second;
      if(!output.full_surface_bounds) {
        v.position.x=(canvas.left+v.position.x*canvas.scale)*640/canvas.surface_width;
        v.position.y=(canvas.top+v.position.y*canvas.scale)*480/canvas.surface_height;
      }
    }
  }
  void invite_picker(const MenuState &state) {
    output.full_surface_bounds=true;
    quad(-canvas.left/canvas.scale,-canvas.top/canvas.scale,canvas.surface_width/canvas.scale,canvas.surface_height/canvas.scale,white,{1,.02f,.055f,.1f});
    const auto label=[&](const std::string &value,float x,float y,float width,float height,UiColor color=UiColor{}) {
      UiText t;t.font=0;t.color=color;t.bounds={240-y,x-320,240-y-height,x+width-320};text(t,value);
    };
    label("INVITE TO PARTY",50,25,540,30);
    const auto &f=state.frame();
    constexpr unsigned visible=12;
    const auto first=f.selected>=visible?f.selected-visible+1:0;
    for(unsigned row=first;row<f.entries.size() && row<first+visible;++row) {
      const float y=104+float(row-first)*23;
      if(row==f.selected)quad(50,y,540,23,white,{1,.1f,.3f,.46f});
      label(f.entries[row].label,58,y+2,524,22,f.entries[row].destination==-82?UiColor{1,.45f,.55f,.65f}:UiColor{});
      hits.push_back({50,y,590,y+23,row,FrontendPointerAction::Entry});
    }
    label(f.description,50,389,540,52,{1,.6f,.75f,.85f});
    label(prompt_device==PromptDevice::KeyboardMouse?"[Enter] Invite    [Esc] Back":"\xee\x84\x80 Invite    \xee\x84\x81 Back",50,449,540,25);
  }
  void original_friends(const MenuState &state) {
    if(!logical || !logical->original_friends_footer || content.gameplay_overlay ||
        !state.online_friends_available() ||
        !detail::og_matchmaking_layout::goal_frame(state.frame(),&state))return;
    // TU1.5 23C254 (retail23586F) maintains the separate collapsed
    // channel4 owner only without an expanded/pending Y screen and outside
    // source session UI6/7. Here the admitted mainmenu goal is active, the
    // expanded native social stack is outside that goal, and this renderer
    // is submitting ordinary visible UI (not transition-suppressed UI).
    // online_matchmaking_ui is the explicit replacement-service phase
    // adapter; it does not assert ownership of original Live's raw49E8 key.
    const auto ui=state.online_matchmaking_ui();
    if(ui==MatchmakingProgressUi::starting || ui==MatchmakingProgressUi::start_match)return;
    auto &owner=*logical;owner.output={};owner.hits.clear();owner.seconds=seconds;
    owner.ui_clock_ms=ui_clock_ms;owner.canvas=canvas;
    const auto previous=owner.drawing_original_goal;owner.drawing_original_goal=true;
    MenuFrame collapsed;collapsed.screen=27;collapsed.instance=state.frame().instance;
    collapsed.pane_generation=state.frame().pane_generation;
    const auto &definition=*owner.original_friends_footer;
    owner.pane(definition,definition.panes.at(0),collapsed,state,true);
    owner.drawing_original_goal=previous;
    output.vertices.insert(output.vertices.end(),owner.output.vertices.begin(),owner.output.vertices.end());
    hits.insert(hits.end(),owner.hits.begin(),owner.hits.end());
    output.rendering_gaps.insert(output.rendering_gaps.end(),owner.output.rendering_gaps.begin(),owner.output.rendering_gaps.end());
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
    if (logical)
      std::erase_if(logical->typing_widgets, [&](const auto &entry) {
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
    const bool social_backdrop = std::any_of(state.stack().begin(), state.stack().end(),
        [&](const auto &f) { return state.online_clan_screen(f) ||
            (logical && logical->full_surface_friends && state.online_friends_screen(f)); });
    output.full_surface_bounds = social_backdrop;
    if(content.gameplay_overlay && content.vista) {
      const auto &overlay=content.vista->overlay;
      output.backdrops.push_back({0,1,{overlay.red,overlay.green,overlay.blue,overlay.alpha},true});
    }
    if (social_backdrop) {
      // An opaque color sampled from the original body replaces shell decoration
      // and the world beneath this native page, including its nested dialogs.
      quad(-canvas.left / canvas.scale, -canvas.top / canvas.scale,
           canvas.surface_width / canvas.scale, canvas.surface_height / canvas.scale,
           white, clan_background_color);
    }
    if(state.party_invite_picker()) {invite_picker(state);invite_overlays(state);return output;}
    // 2234E0 ->13870 owns this normal mainmenu world view: mode3, one
    // canonical640x480 view, neither offscreen guard. The scenario's constant
    // interpolator selects E4380218 before UI, independently of screen ID.
    // Same-clock6/14/23/206 captures confirm the title/47 shader binding.
    // Channel tint/blur belongs below, after lower UI; no initial blue quad.
    output.scene_color_filter = true;
    if (!content.gameplay_overlay && !social_backdrop && !title_popup_backdrop && frame.screen != 9 && frame.screen != 6 &&
        content.screens.contains(5)) {
      MenuFrame background;
      const bool pc_settings=content.vista && std::any_of(state.stack().begin(),state.stack().end(),
          [](const auto &f){return is_vista_menu(f.screen);});
      // The MCC original shell owns both its frame contour mapping and its
      // shared title/footer anchors. PC bodies must not select another frame.
      background.screen = !logical && (pc_settings || (online_panel && content.vista))
          ? vista_menu(5) : 5;
      screen(background, state, false);
    }
    const auto layout = state.screen_layout(frame);
    const auto effective_definition=[&](const MenuFrame&current)->const UiScreen& {
      const auto id=state.screen_layout(current);
      if(logical_view && logical_view->content.original_matchmaking_layout && detail::og_matchmaking_layout::goal_frame(current,&state) &&
          detail::og_matchmaking_layout::data().at("original").at("screens").contains(std::to_string(id)))
        return logical_view->content.screen(id);
      return content.screen(id);
    };
    const bool dialog = effective_definition(frame).flags & (1 | 8 | 16);
    if (dialog && state.stack().size() > 1) {
      auto it = state.stack().rbegin() + 1;
      while (it + 1 != state.stack().rend() &&
             (effective_definition(*it).flags & (1 | 8 | 16)))
        ++it;
      const bool choosing_profile = std::any_of(state.stack().begin(),
          state.stack().end(), [](const auto &f) { return f.screen == 10; });
      // Disc screens9/6 share the start_screen logo bitmap. The native
      // ONLINE panel replaces that lower title/main-menu UI for readability.
      const bool hide_title = online_panel && (it->screen == 9 || it->screen == 6);
      // The native directory details page keeps browser history for Back,
      // while replacing its visible content and footer.
      const bool hide_lower = hide_title || (social_backdrop && frame.screen != 26) ||
          (state.online_party_screen(frame) && frame.screen != 14) ||
          (state.online_friends_screen(frame) && frame.screen != 26) ||
          ((frame.screen == 237 || frame.screen == 210) && (it->screen == 219 || it->screen == 237 || it->screen == 186));
      // 22F916 ->1473C9 ->23029A ->234F8C: every common dialog
      // over an occupied lower channel receives the authored tint/blur,
      // unless definition flag5 suppresses it. An ID whitelist omitted
      // error/confirmation/settings dialogs and left lower text sharp.
      if (!(effective_definition(frame).flags & 32)) {
        if (!hide_lower) screen(*it, state, false);
        // The original disposes profile channel 3 before opening keyboard
        // channel 2 (23784F). Both paths therefore blur the retained title
        // once, as confirmed by 23029A and the live channel snapshots.
        // 230374/234F8C/235756: the popup's authored transition
        // controls the tint and full-resolution blur of lower-channel UI.
        // Setup23 and Map206 also occupy channel3 over retained lobby14.
        // A replacement retains the outgoing channel's complementary fade.
        const auto &definition = effective_definition(frame);
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
    original_friends(state); // Original channel4 renders after the base channel.
    invite_overlays(state);
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
void FrontendPainter::prompt_device(PromptDevice device) {
  const auto set=[&](auto &&self,Impl &item)->void {
    item.prompt_device=device;
    if(item.vista)self(self,*item.vista);
    if(item.online)self(self,*item.online);
    if(item.logical)self(self,*item.logical);
  };
  set(set,*impl_);
}
UiDraw FrontendPainter::draw(const MenuState &state, float seconds, int pane,
                             std::uint32_t ui_clock_ms, std::array<unsigned, 2> viewport) {
  impl_->canvas = frontend_canvas(viewport[0], viewport[1]);
  auto output=impl_->draw(state, seconds, pane, ui_clock_ms);
  //14A62E renders every UI channel before its channel4 map-init black.
  // Submit once at the outer draw boundary, after delegated MCC/private
  // layout children and the independent PCR16 window intro.
  if(const auto color=state.ui_map_intro_color()) {
    const float alpha=float(*color>>24)/255.f;
    const auto material=impl_->map_intro_black;
    const Vertex a{{0,0,alpha},{0,0,0},{0,0},material},
        b{{640,0,alpha},{0,0,0},{1,0},material},
        c{{0,480,alpha},{0,0,0},{0,1},material},
        d{{640,480,alpha},{0,0,0},{1,1},material};
    output.vertices.insert(output.vertices.end(),{a,b,c,b,d,c});
  }
  return output;
}
UiDraw FrontendPainter::draw_invite_notice(const FrontendInviteNotice &notice,std::array<unsigned,2> viewport,bool append) {
  impl_->canvas=frontend_canvas(viewport[0],viewport[1]);
  if(!append){impl_->output={};impl_->hits.clear();}
  impl_->paint_invite_notice(notice);return impl_->output;
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
namespace {
// TU1.5 0x194890 compiled status constants: network_loading_map,
// network_joining and network_leaving. MCC binds them by exact name.
std::uint32_t network_transition_string(FrontendNetworkTransitionStatus status) {
  switch(status) {
  case FrontendNetworkTransitionStatus::LoadingMap:return 0x1300079eu;
  case FrontendNetworkTransitionStatus::SettingUpGame:return 0x0f0007a1u;
  case FrontendNetworkTransitionStatus::LeavingGame:return 0x0f0007a3u;
  }
  throw FormatError("unsupported network transition status");
}
constexpr unsigned network_transition_screen=231;
constexpr std::array network_transition_statuses{FrontendNetworkTransitionStatus::LoadingMap,
    FrontendNetworkTransitionStatus::SettingUpGame,FrontendNetworkTransitionStatus::LeavingGame};
}
struct FrontendNetworkTransitionPainter::State {
  FrontendContent content;
  Mesh scratch;
  std::unique_ptr<FrontendPainter::Impl> impl;
  std::array<std::string,network_transition_statuses.size()> status;
  std::map<unsigned,unsigned> remap;
  std::vector<Material> bank;
  // The root pane owner for this one-pane dialog: no lists, scenes, header
  // (definition flag4) or button key. Same widget order and projection.
  std::vector<Vertex> paint(FrontendNetworkTransitionStatus value,float seconds,std::uint32_t clock) {
    auto &p=*impl;
    p.output={};p.hits.clear();p.seconds=seconds;p.ui_clock_ms=clock;
    p.drawing_exit=false;p.canvas=frontend_canvas(640,480);
    const auto &screen=content.screens.at(network_transition_screen);
    const auto &pane=screen.panes.front();
    std::vector<const UiBitmap *> ordered;
    for(const auto &b:pane.bitmaps)ordered.push_back(&b);
    float nearest=std::numeric_limits<float>::max(),farthest=-nearest;
    for(const auto *b:ordered) {
      const auto depth=p.animation(b->animation,b->delay).position.z;
      nearest=std::min(nearest,depth);farthest=std::max(farthest,depth);
    }
    if(farthest-nearest<=.001f)
      std::stable_sort(ordered.begin(),ordered.end(),[](const UiBitmap *x,const UiBitmap *y){return x->layer<y->layer;});
    // The code-owned bitmap slot has no datum; 0x258605 only zeroes its
    // scale, so bitmap() submits nothing for it, like the original widget.
    for(const auto *b:ordered)p.bitmap(*b);
    for(unsigned i=0;i<pane.texts.size();++i)
      p.text(pane.texts[i],i==0?status.at(unsigned(value)):content.localized(screen,pane.texts[i].string),
          {},1,false,false,nullptr,&pane.texts[i],false);
    if(p.imported_mcc_canvas) {
      constexpr float scale=480.f/1200.f;
      for(auto &vertex:p.output.vertices) {
        vertex.position.x=320+(vertex.position.x-320)*scale;
        vertex.position.y=240+(vertex.position.y-240)*scale;
      }
    }
    return std::move(p.output.vertices);
  }
};
FrontendNetworkTransitionPainter::FrontendNetworkTransitionPainter(const FrontendContent &source,
    const std::filesystem::path &fonts,const FrontendUiAssetPack &ui_assets,
    const std::filesystem::path &sampled_fonts):state_(std::make_unique<State>()) {
  auto &s=*state_;
  const auto found=source.screens.find(network_transition_screen);
  if(found==source.screens.end() || found->second.panes.empty() || found->second.panes.front().texts.size()<2)
    throw FormatError("Selected menu content lacks the original network transition screen");
  // A private one-screen view of the selected content. Fonts, sampled pages,
  // animations and strings are the frontend's own; no other screen is built.
  auto &c=s.content;
  c.source_fonts=source.source_fonts;c.pc_string_ids=source.pc_string_ids;
  c.screens.emplace(network_transition_screen,found->second);
  c.animations=source.animations;c.global_strings=source.global_strings;
  c.button_keys=source.button_keys;c.header_fonts=source.header_fonts;
  c.header_bounds=source.header_bounds;c.typewriter=source.typewriter;
  c.color=source.color;c.overlay=source.overlay;
  for(const auto &bitmap:found->second.panes.front().bitmaps)
    if(const auto image=source.images.find(bitmap.datum);image!=source.images.end())c.images.insert(*image);
  s.impl=std::make_unique<FrontendPainter::Impl>(c,fonts,s.scratch,std::filesystem::path{},ui_assets,sampled_fonts);
  const auto &screen=c.screens.at(network_transition_screen);
  for(const auto value:network_transition_statuses) {
    const auto id=detail::frontend_compiled_string_id(c,network_transition_string(value));
    auto &text=s.status.at(unsigned(value));
    if(id)text=c.localized(screen,*id);
    if(text.empty())throw FormatError("Network transition status string is unavailable");
  }
  // Keep only the resident owners this dialog can address: every status,
  // before and after the pane intro. Sprite pages here have no frame rate.
  std::set<unsigned> used;
  for(const auto value:network_transition_statuses)
    for(const float seconds:{0.f,60.f})
      for(const auto &vertex:s.paint(value,seconds,0))used.insert(vertex.material);
  if(used.empty())throw FormatError("Network transition dialog has no visible widgets");
  for(const auto index:used) {
    auto material=s.scratch.materials.at(index);
    material.name="network-transition/"+material.name;
    prepare_frontend_material_storage(material);
    if(material.diffuse.pixels.empty() || material.diffuse.layers!=1 || material.diffuse.depth)
      throw FormatError("Network transition material storage is unsupported: "+material.name);
    s.remap.emplace(index,unsigned(s.bank.size()));
    s.bank.push_back(std::move(material));
  }
  // The painter reads only metrics/dimensions after construction. Release
  // unused staging pixels such as controller prompt atlases.
  for(auto &material:s.scratch.materials)material.diffuse.pixels=TexturePixels{};
}
FrontendNetworkTransitionPainter::~FrontendNetworkTransitionPainter()=default;
std::span<const Material> FrontendNetworkTransitionPainter::materials() const {return state_->bank;}
std::vector<Vertex> FrontendNetworkTransitionPainter::draw(FrontendNetworkTransitionStatus status,
    float seconds,std::uint32_t ui_clock_ms) {
  auto vertices=state_->paint(status,seconds,ui_clock_ms);
  for(auto &vertex:vertices) {
    const auto found=state_->remap.find(vertex.material);
    if(found==state_->remap.end())throw FormatError("Network transition draw left its resident bank");
    vertex.material=found->second;
  }
  return vertices;
}
} // namespace halo2
