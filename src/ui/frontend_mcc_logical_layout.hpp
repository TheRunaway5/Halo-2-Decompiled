#pragma once
#include "halo2/frontend.hpp"
#include "ui/mcc_frontend_string_ids.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <fstream>
#include <string_view>

namespace halo2::detail {
namespace mcc_logical_layout {
using Json = nlohmann::json;
#include "ui/frontend_mcc_logical_layout_data.inc"
inline Json point(Vec2 v) { return {v.x, v.y}; }
inline Json point(Vec3 v) { return {v.x, v.y, v.z}; }
inline Json rect(UiRect v) { return {v.top, v.left, v.bottom, v.right}; }
inline Json color(UiColor v) { return {v.alpha, v.red, v.green, v.blue}; }
inline Vec2 point2(const Json &j) {
  return {j.at(0).get<float>(), j.at(1).get<float>()};
}
inline Vec3 point3(const Json &j) {
  return {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>()};
}
inline UiRect rectangle(const Json &j) {
  return {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>(),
          j.at(3).get<float>()};
}
inline UiColor rgba(const Json &j) {
  return {j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>(),
          j.at(3).get<float>()};
}
template <class Values, class Fn> Json rows(const Values &values, Fn fn) {
  auto j = Json::array();
  for (const auto &v : values)
    j.push_back(fn(v));
  return j;
}
struct Codec {
  const FrontendContent &source;
  Json globals() const {
    return {{"header_fonts", source.header_fonts},
            {"header_bounds",
             rows(source.header_bounds, [](auto r) { return rect(r); })},
            {"color", color(source.color)},
            {"overlay", color(source.overlay)},
            {"typewriter",
             {source.typewriter.interval_ms, source.typewriter.cursor,
              source.typewriter.scale}},
            {"animations", rows(source.animations,
                                [&](const auto &a) { return animation(a); })}};
  }
  std::string image_name(std::uint32_t id) const {
    return id == null_datum ? "" : source.images.at(id).name;
  }
  std::string group_name(std::uint32_t id) const {
    return id == null_datum ? "" : source.skin_groups.at(id).name;
  }
  std::string shader_name(std::uint32_t id) const {
    return id == null_datum ? "" : source.emblem_shaders.at(id).name;
  }
  template <class Map>
  static std::uint32_t named(const Map &map, const std::string &name) {
    if (name.empty())
      return null_datum;
    std::optional<std::uint32_t> found;
    for (const auto &[id, v] : map)
      if (v.name == name) {
        if (found)
          throw FormatError("Ambiguous MCC logical layout resource");
        found = id;
      }
    if (!found)
      throw FormatError("Missing MCC logical layout resource: " + name);
    return *found;
  }
  Json text(const UiText &t) const {
    return {{"flags", t.flags},        {"animation", t.animation},
            {"delay", t.delay},        {"font", t.font},
            {"color", color(t.color)}, {"bounds", rect(t.bounds)},
            {"string", t.string},      {"layer", t.layer}};
  }
  UiText text(const Json &j) const {
    UiText t;
    t.flags = j.at("flags");
    t.animation = j.at("animation");
    t.delay = j.at("delay");
    t.font = j.at("font");
    t.color = rgba(j.at("color"));
    t.bounds = rectangle(j.at("bounds"));
    t.string = j.at("string");
    t.layer = j.at("layer");
    return t;
  }
  Json bitmap(const UiBitmap &b) const {
    return {{"flags", b.flags},
            {"animation", b.animation},
            {"delay", b.delay},
            {"blend", b.blend},
            {"frame", b.frame},
            {"position", point(b.position)},
            {"scroll", point(b.scroll)},
            {"datum", image_name(b.datum)},
            {"layer", b.layer},
            {"fps", b.fps},
            {"placeholder", b.placeholder},
            {"scale", point(b.scale)}};
  }
  UiBitmap bitmap(const Json &j) const {
    UiBitmap b;
    b.flags = j.at("flags");
    b.animation = j.at("animation");
    b.delay = j.at("delay");
    b.blend = j.at("blend");
    b.frame = j.at("frame");
    b.layer = j.at("layer");
    b.position = point2(j.at("position"));
    b.scroll = point2(j.at("scroll"));
    b.scale = point2(j.at("scale"));
    b.datum = named(source.images, j.at("datum"));
    b.placeholder = j.at("placeholder");
    b.fps = j.at("fps");
    return b;
  }
  Json track(const UiTrack &t) const {
    return {{"milliseconds", t.milliseconds},
            {"keys", rows(t.keys, [](const UiKey &k) {
               return Json{{"alpha", k.alpha}, {"position", point(k.position)}};
             })}};
  }
  UiTrack track(const Json &j) const {
    UiTrack t;
    t.milliseconds = j.at("milliseconds");
    for (const auto &k : j.at("keys"))
      t.keys.push_back({k.at("alpha"), point3(k.at("position"))});
    return t;
  }
  Json animation(const UiAnimation &a) const {
    return {{"intro", track(a.intro)},
            {"outro", track(a.outro)},
            {"ambient", track(a.ambient)},
            {"loop", a.loop}};
  }
  UiAnimation animation(const Json &j) const {
    return {track(j.at("intro")), track(j.at("outro")), track(j.at("ambient")),
            j.at("loop")};
  }
  Json grid(const UiGrid &g) const {
    return {{"skin", group_name(g.skin)}, {"position", point(g.position)},
            {"step", point(g.step)},      {"order", g.order},
            {"count", g.count},           {"rows", g.rows},
            {"columns", g.columns}};
  }
  UiGrid grid(const Json &j) const {
    UiGrid g;
    g.skin = named(source.skin_groups, j.at("skin"));
    g.position = point2(j.at("position"));
    g.step = point2(j.at("step"));
    g.order = j.at("order");
    g.count = j.at("count");
    g.rows = j.at("rows");
    g.columns = j.at("columns");
    return g;
  }
  Json emblem(const UiEmblem &e) const {
    return {{"flags", e.flags},
            {"animation", e.animation},
            {"delay", e.delay},
            {"layer", e.layer},
            {"page", e.page},
            {"bitmap", image_name(e.bitmap)},
            {"shader", shader_name(e.shader)},
            {"bounds", rect(e.bounds)}};
  }
  UiEmblem emblem(const Json &j) const {
    UiEmblem e;
    e.flags = j.at("flags");
    e.animation = j.at("animation");
    e.delay = j.at("delay");
    e.layer = j.at("layer");
    e.page = j.at("page");
    e.bitmap = named(source.images, j.at("bitmap"));
    e.shader = named(source.emblem_shaders, j.at("shader"));
    e.bounds = rectangle(j.at("bounds"));
    return e;
  }
  Json list(const UiList &l) const {
    return {{"flags", l.flags},         {"skin", l.skin},
            {"visible", l.visible},     {"position", point(l.position)},
            {"animation", l.animation}, {"delay", l.delay},
            {"labels", l.labels}};
  }
  UiList list(const Json &j) const {
    UiList l;
    l.flags = j.at("flags");
    l.skin = j.at("skin");
    l.visible = j.at("visible");
    l.position = point2(j.at("position"));
    l.animation = j.at("animation");
    l.delay = j.at("delay");
    l.labels = j.at("labels").get<std::vector<std::uint32_t>>();
    return l;
  }
  Json button(const UiButton &b) const {
    return {{"text", text(b.text)},
            {"bitmap", image_name(b.bitmap)},
            {"offset", point(b.offset)},
            {"hit", b.vista_hit_top_offset},
            {"vista_flags", b.vista_flags}};
  }
  UiButton button(const Json &j) const {
    UiButton b;
    b.text = text(j.at("text"));
    b.bitmap = named(source.images, j.at("bitmap"));
    b.offset = point2(j.at("offset"));
    b.vista_hit_top_offset = j.at("hit");
    b.vista_flags = j.at("vista_flags");
    return b;
  }
  Json scene(const UiScene &s) const {
    return {{"flags", s.flags},
            {"animation", s.animation},
            {"delay", s.delay},
            {"scale", point(s.scale)},
            {"camera", point(s.camera)},
            {"fov", s.field_of_view},
            {"viewport", rect(s.viewport)},
            {"objects", s.objects},
            {"lights", s.lights}};
  }
  UiScene scene(const Json &j) const {
    UiScene s;
    s.flags = j.at("flags");
    s.animation = j.at("animation");
    s.delay = j.at("delay");
    s.scale = point3(j.at("scale"));
    s.camera = point3(j.at("camera"));
    s.field_of_view = j.at("fov");
    s.viewport = rectangle(j.at("viewport"));
    s.objects = j.at("objects").get<std::vector<std::string>>();
    s.lights = j.at("lights").get<std::vector<std::string>>();
    return s;
  }
  Json pane(const UiPane &p) const {
    return {{"animation", p.animation},
            {"texts", rows(p.texts, [&](auto &x) { return text(x); })},
            {"bitmaps", rows(p.bitmaps, [&](auto &x) { return bitmap(x); })},
            {"buttons", rows(p.buttons, [&](auto &x) { return button(x); })},
            {"lists", rows(p.lists, [&](auto &x) { return list(x); })},
            {"scenes", rows(p.scenes, [&](auto &x) { return scene(x); })},
            {"emblems", rows(p.emblems, [&](auto &x) { return emblem(x); })},
            {"grids", rows(p.grids, [&](auto &x) { return grid(x); })}};
  }
  UiPane pane(const Json &j) const {
    UiPane p;
    p.animation = j.at("animation");
    for (auto &x : j.at("texts"))
      p.texts.push_back(text(x));
    for (auto &x : j.at("bitmaps"))
      p.bitmaps.push_back(bitmap(x));
    for (auto &x : j.at("buttons"))
      p.buttons.push_back(button(x));
    for (auto &x : j.at("lists"))
      p.lists.push_back(list(x));
    for (auto &x : j.at("scenes"))
      p.scenes.push_back(scene(x));
    for (auto &x : j.at("emblems"))
      p.emblems.push_back(emblem(x));
    for (auto &x : j.at("grids"))
      p.grids.push_back(grid(x));
    return p;
  }
  Json screen(const UiScreen &s) const {
    return {{"name", s.name},
            {"id", s.id},
            {"flags", s.flags},
            {"button_key", s.button_key},
            {"shape", s.shape},
            {"header", s.header},
            {"color", color(s.color)},
            {"bitmap_resources",
             rows(s.bitmap_resources, [&](auto id) { return image_name(id); })},
            {"panes", rows(s.panes, [&](auto &p) { return pane(p); })}};
  }
  UiScreen screen(const Json &j) const {
    UiScreen s;
    s.name = j.at("name");
    s.id = j.at("id");
    s.flags = j.at("flags");
    s.button_key = j.at("button_key");
    s.shape = j.at("shape");
    s.header = j.at("header");
    s.color = rgba(j.at("color"));
    s.strings = source.screens.at(s.id).strings;
    for (const auto &n : j.at("bitmap_resources"))
      s.bitmap_resources.push_back(named(source.images, n));
    for (auto &p : j.at("panes"))
      s.panes.push_back(pane(p));
    return s;
  }
  Json skin(const UiSkin &s) const {
    return {
        {"name", s.name},
        {"arrows", image_name(s.arrows)},
        {"arrow_up", point(s.arrow_up)},
        {"arrow_down", point(s.arrow_down)},
        {"animations", rows(s.animations, [&](auto &a) { return track(a); })},
        {"texts", rows(s.texts, [&](auto &x) { return text(x); })},
        {"bitmaps", rows(s.bitmaps, [&](auto &x) { return bitmap(x); })},
        {"emblems", rows(s.emblems, [&](auto &x) { return emblem(x); })},
        {"grids", rows(s.grids, [&](auto &x) { return grid(x); })}};
  }
  UiSkin skin(const Json &j) const {
    UiSkin s;
    s.name = j.at("name");
    s.arrows = named(source.images, j.at("arrows"));
    s.arrow_up = point2(j.at("arrow_up"));
    s.arrow_down = point2(j.at("arrow_down"));
    for (auto &a : j.at("animations"))
      s.animations.push_back(track(a));
    for (auto &x : j.at("texts"))
      s.texts.push_back(text(x));
    for (auto &x : j.at("bitmaps"))
      s.bitmaps.push_back(bitmap(x));
    for (auto &x : j.at("emblems"))
      s.emblems.push_back(emblem(x));
    for (auto &x : j.at("grids"))
      s.grids.push_back(grid(x));
    return s;
  }
  Json image(const UiImage &image) const {
    return {
        {"frames", rows(image.images,
                        [](auto &t) {
                          return Json{{"width", t.width}, {"height", t.height}};
                        })},
        {"sequences", rows(image.sequences, [](auto &seq) {
           return rows(seq, [](auto &s) {
             return Json{
                 {"image", s.image},   {"left", s.left},
                 {"right", s.right},   {"top", s.top},
                 {"bottom", s.bottom}, {"registration", point(s.registration)}};
           });
         })}};
  }
};
struct View {
  FrontendContent content;
  FrontendUiAssetPack rasters;
  bool full_surface_friends{};
  std::optional<UiScreen> browser;
  float browser_frame_height_delta{};
};
// This is a finite logical definition projection, not an alternate asset bank.
// Unknown/modified authored widgets retain their own layout. Every raster and
// localized string remains owned by the selected MCC cache.
inline std::optional<View> make_view(const FrontendContent &source) {
  if (!mcc_frontend_content(source))
    return {};
  static const Json compiled = Json::parse(mcc_logical_layout_data);
  Json imported;
  const Json *profile = &compiled;
  const auto converted = source.source_fonts.parent_path().parent_path() /
      "presentation/ui/native-layout.json";
  if (std::filesystem::exists(converted)) {
    if (std::filesystem::is_symlink(converted) ||
        !std::filesystem::is_regular_file(converted) ||
        std::filesystem::file_size(converted) > 2 * 1024 * 1024)
      throw FormatError("Invalid imported MCC native UI layout owner");
    std::ifstream file(converted);
    imported = Json::parse(file);
    if (imported.value("schema", "") != "H2_MCC_NATIVE_UI_LAYOUT_V1" ||
        imported.at("layout") != compiled)
      throw FormatError("Imported MCC native UI layout needs an importer update");
    profile = &imported.at("layout");
  }
  const auto &data = *profile;
  const auto &raw = data.at("raw");
  const auto &original = data.at("original");
  Codec codec{source};
  if (codec.globals() != raw.at("globals"))
    return {};
  try {
    for (const auto &[key, j] : raw.at("screens").items()) {
      const auto it = source.screens.find(unsigned(std::stoul(key)));
      if (it == source.screens.end() || codec.screen(it->second) != j)
        return {};
    }
    for (const auto &[key, j] : raw.at("skins").items()) {
      const auto index = unsigned(std::stoul(key));
      if (index >= source.skins.size() || codec.skin(source.skins[index]) != j)
        return {};
    }
    for (const auto &[name, j] : raw.at("groups").items())
      if (codec.skin(source.skin_groups.at(
              Codec::named(source.skin_groups, name))) != j)
        return {};
    for (const auto &[name, j] : original.at("images").items()) {
      auto expected = raw.at("images").at(name);
      for (auto &frame : expected.at("frames"))
        frame.erase("format");
      if (codec.image(source.images.at(Codec::named(source.images, name))) !=
          expected)
        return {};
    }
    // The selected map catalog also owns images not referenced by a pane.
    // Admit their exact stock dimensions independently of pixel/localization
    // replacements; an unavailable map entry need not construct its bitmap.
    for (const auto &[id, image] : source.images) {
      const auto expected = raw.at("map_images").find(image.name);
      if (expected == raw.at("map_images").end()) continue;
      auto frames = *expected;
      for (auto &frame : frames) frame.erase("format");
      if (codec.image(image).at("frames") != frames) return {};
    }
  } catch (const FormatError &) {
    return {}; // Missing/ambiguous named resources belong to a different bank.
  } catch (const std::out_of_range &) {
    return {}; // A modified resource graph cannot select this stock profile.
  }
  View view;
  view.full_surface_friends = data.at("responsive").at("friends_full_surface");
  // The native server directory borrows PC13's columns. Qualify its complete
  // source definition before adapting that table to the original shell title.
  // Custom PC definitions keep their authored geometry.
  if (source.vista && data.at("responsive").contains("browser_raw")) {
    Codec pc{*source.vista};
    const auto it = source.vista->screens.find(13);
    bool qualified = it != source.vista->screens.end() &&
        pc.screen(it->second) == data.at("responsive").at("browser_raw") &&
        source.vista->skins.size() > 5 &&
        pc.skin(source.vista->skins.at(5)) == data.at("responsive").at("browser_skin");
    for (const auto &[name, record] : data.at("responsive").at("browser_images").items()) {
      auto expected = record;
      for (auto &frame : expected.at("frames")) frame.erase("format");
      try {
        qualified &= pc.image(source.vista->images.at(
            Codec::named(source.vista->images, name))) == expected;
      } catch (const FormatError &) { qualified = false; }
    }
    if (qualified) {
      view.browser = pc.screen(data.at("responsive").at("browser_converted"));
      view.browser_frame_height_delta = data.at("responsive").at("browser_frame_height_delta");
    }
  }
  auto &out = view.content;
  out.source_fonts = source.source_fonts;
  out.global_strings = source.global_strings;
  out.button_keys = source.button_keys;
  out.appearance_palette = source.appearance_palette;
  out.emblem_shaders = source.emblem_shaders;
  out.skins.resize(source.skins.size());
  const auto &globals = original.at("globals");
  out.header_fonts = globals.at("header_fonts").get<std::array<int, 4>>();
  for (unsigned i = 0; i < out.header_bounds.size(); ++i)
    out.header_bounds[i] = rectangle(globals.at("header_bounds").at(i));
  out.color = rgba(globals.at("color"));
  out.overlay = rgba(globals.at("overlay"));
  const auto &type = globals.at("typewriter");
  out.typewriter = {type.at(0), type.at(1), type.at(2)};
  for (const auto &a : globals.at("animations"))
    out.animations.push_back(codec.animation(a));
  for (const auto &[key, j] : original.at("screens").items())
    out.screens.emplace(unsigned(std::stoul(key)), codec.screen(j));
  for (const auto &[key, j] : original.at("skins").items())
    out.skins.at(unsigned(std::stoul(key))) = codec.skin(j);
  for (const auto &[name, j] : original.at("groups").items())
    out.skin_groups.emplace(Codec::named(source.skin_groups, name),
                            codec.skin(j));
  for (const auto &[name, j] : original.at("images").items()) {
    const auto id = Codec::named(source.images, name);
    const auto &actual = source.images.at(id);
    view.rasters.emplace(name, actual.images);
    UiImage logical;
    logical.name = name;
    logical.sequences = actual.sequences;
    for (const auto &frame : j.at("frames")) {
      Texture t;
      t.width = frame.at("width");
      t.height = frame.at("height");
      t.format = frame.at("format");
      logical.images.push_back(std::move(t));
    }
    out.images.emplace(id, std::move(logical));
  }
  for (const auto &[id, actual] : source.images) {
    const auto expected = original.at("map_images").find(actual.name);
    if (expected == original.at("map_images").end()) continue;
    view.rasters.emplace(actual.name, actual.images);
    UiImage image;
    image.name = actual.name;
    image.sequences = actual.sequences;
    for (const auto &frame : *expected) {
      Texture t;
      t.width = frame.at("width");
      t.height = frame.at("height");
      t.format = frame.at("format");
      image.images.push_back(std::move(t));
    }
    out.images.emplace(id, std::move(image));
  }
  return view;
}
} // namespace mcc_logical_layout
} // namespace halo2::detail
