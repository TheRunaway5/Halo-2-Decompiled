#include "halo2/custom_slayer.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_online.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <set>

namespace halo2 {
namespace {
int signed16(const Bytes &b, std::size_t p) {
  return std::bit_cast<std::int16_t>(b.u16(p));
}
UiRect rect(const Bytes &b, std::size_t p) {
  return {float(signed16(b, p)), float(signed16(b, p + 2)),
          float(signed16(b, p + 4)), float(signed16(b, p + 6))};
}
Vec2 point(const Bytes &b, std::size_t p) {
  return {float(signed16(b, p)), float(signed16(b, p + 2))};
}
UiColor color(const Bytes &b, std::size_t p) {
  UiColor c{b.f32(p), b.f32(p + 4), b.f32(p + 8), b.f32(p + 12)};
  for (const auto v : {c.alpha, c.red, c.green, c.blue})
    if (!std::isfinite(v) || v < 0 || v > 1)
      throw FormatError("invalid UI color");
  return c;
}
std::string localized_map_text(const Bytes &bytes, std::size_t p,
                               unsigned capacity) {
  bytes.require(p, std::size_t(capacity) * 2);
  std::string out;
  auto append = [&](std::uint32_t value) {
    if (value < 0x80)
      out.push_back(char(value));
    else if (value < 0x800) {
      out.push_back(char(0xc0 | (value >> 6)));
      out.push_back(char(0x80 | (value & 0x3f)));
    } else if (value < 0x10000) {
      out.push_back(char(0xe0 | (value >> 12)));
      out.push_back(char(0x80 | ((value >> 6) & 0x3f)));
      out.push_back(char(0x80 | (value & 0x3f)));
    } else {
      out.push_back(char(0xf0 | (value >> 18)));
      out.push_back(char(0x80 | ((value >> 12) & 0x3f)));
      out.push_back(char(0x80 | ((value >> 6) & 0x3f)));
      out.push_back(char(0x80 | (value & 0x3f)));
    }
  };
  for (unsigned i = 0; i < capacity; ++i) {
    std::uint32_t value = bytes.u16(p + i * 2);
    if (!value)
      return out;
    if (value >= 0xd800 && value <= 0xdbff) {
      if (++i >= capacity)
        throw FormatError("truncated UI map surrogate");
      const auto low = bytes.u16(p + i * 2);
      if (low < 0xdc00 || low > 0xdfff)
        throw FormatError("invalid UI map surrogate");
      value = 0x10000 + ((value - 0xd800) << 10) + (low - 0xdc00);
    } else if (value >= 0xdc00 && value <= 0xdfff)
      throw FormatError("invalid UI map surrogate");
    append(value);
  }
  throw FormatError("unterminated UI map text");
}

} // namespace
Texture prepare_ui_bitmap(const Bytes &raw, unsigned width, unsigned height,
                          unsigned format, unsigned flags) {
  if (flags & 16) {
    if (!width || !height || width > 4096 || height > 4096 || format > 11)
      throw FormatError("invalid linear UI bitmap");
    const unsigned pixel_size =
        format <= 2                                                  ? 1
        : (format == 3 || format == 6 || format == 8 || format == 9) ? 2
                                                                     : 4;
    const auto row = std::size_t(width) * pixel_size;
    // Retail 1365A0/12D6D0: Xbox linear textures have a 64-byte row pitch.
    const auto pitch = (row + 63) & ~std::size_t(63);
    raw.require(0, pitch * height);
    std::vector<std::byte> packed(row * height);
    for (unsigned y = 0; y < height; ++y) {
      const auto source = raw.slice(y * pitch, row);
      std::copy(source.begin(), source.end(), packed.begin() + y * row);
    }
    return prepare_texture(Bytes(packed), width, height, format, false);
  }
  auto texture = prepare_texture(raw, width, height, format, (flags & 8) != 0);
  // Pinned Xemu's Vulkan UI path filters its integer-expanded RGBA8 image.
  // Hardware BC3 interpolation differs by a byte at actual dialog texels.
  if (format == 16)
    return prepare_xemu_bc3_texture(texture);
  return texture;
}
std::string FrontendContent::localized(const UiScreen &screen,
                                       std::uint32_t id) const {
  if (const auto it = screen.strings.find(id); it != screen.strings.end())
    return frontend_online_text(it->second);
  if (const auto it = global_strings.find(id); it != global_strings.end())
    return frontend_online_text(it->second);
  return {};
}
FrontendContent Cache::load_frontend() const {
  const Bytes bytes(cache_bytes());
  FrontendContent out;
  auto tag = [&](std::uint32_t datum, const char *group) -> const Tag & {
    const auto* it = find_tag({datum});
    if (!it || it->group != group || !it->address)
      throw FormatError(std::string("missing frontend ") + group + " tag");
    return *it;
  };
  auto offset = [&](std::uint32_t datum, const char *group, std::size_t size) {
    const auto &t = tag(datum, group);
    if (t.size < size)
      throw FormatError(std::string("truncated frontend ") + group);
    return metadata_offset(t.address, size);
  };
  
  const auto globals =
      query_tag({.group=tag_groups::globals});
  if (!globals)
    throw FormatError("frontend has no map globals");
  for (const auto p : metadata_block_offsets(metadata_offset(globals->address, 0x168) + 0x160, 12, 256)) {
    const Vec3 value{bytes.f32(p), bytes.f32(p + 4), bytes.f32(p + 8)};
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
      throw FormatError("invalid frontend appearance palette color");
    out.appearance_palette.push_back(value);
  }
  const auto loc = metadata_offset(globals->address, 0x1ac) + 0x190;
  const auto count = bytes.u32(loc), size = bytes.u32(loc + 4),
             indices = bytes.u32(loc + 8), strings = bytes.u32(loc + 12);
  if (count > 65536 || size > 16 * 1024 * 1024)
    throw FormatError("excessive locale table");
  bytes.require(indices, std::size_t(count) * 8);
  const Bytes table(bytes.slice(strings, size));
  std::vector<std::pair<std::uint32_t, std::string>> locale;
  for (unsigned i = 0; i < count; ++i) {
    const auto p = indices + i * 8, s = bytes.u32(p + 4);
    if (s >= size)
      throw FormatError("locale string outside data");
    locale.emplace_back(bytes.u32(p), table.string(s, size - s));
  }
  auto string_list = [&](std::uint32_t datum) {
    UiStrings result;
    if (datum == 0xffffffffu)
      return result;
    const auto p = offset(datum, "unic", 52);
    const auto start = bytes.u16(p + 16), n = bytes.u16(p + 18);
    if (start > locale.size() || n > locale.size() - start)
      throw FormatError("UI locale range outside table");
    for (unsigned i = 0; i < n; ++i)
      result.insert(locale[start + i]);
    return result;
  };
  const auto multiplayer_globals = bytes.u32(metadata_offset(globals->address, 0x170) + 0x16c);
  if (multiplayer_globals != null_datum) {
    const auto universal = metadata_block_offsets(offset(multiplayer_globals, "mulg", 8), 0x20, 256);
    if (!universal.empty())
      out.variant_strings = string_list(bytes.u32(universal.front() + 0x1c));
  }
  std::set<std::uint32_t> requested_images;
  std::set<std::uint32_t> requested_groups;
  auto image_ref = [&](std::uint32_t datum) {
    if (datum != 0xffffffffu)
      requested_images.insert(datum);
    return datum;
  };
  // Original19C1F0/19C5F0 use the first matg+178 header. The UI's
  // current language is English; original2AFD10/40/70 select language0 here.
  const auto levels = metadata_block_offsets(metadata_offset(globals->address, 0x180) + 0x178, 24, 256);
  const auto campaign_paths = metadata_block_offsets(metadata_offset(globals->address, 0x178) + 0x170, 8, 256);
  const auto path_records = campaign_paths.empty()
      ? std::vector<std::size_t>{} : metadata_block_offsets(campaign_paths.front(), 0x108, 256);
  if (!levels.empty()) {
    for (const auto p : metadata_block_offsets(levels.front() + 8, 0xb50, 256)) {
      const auto map_id = std::bit_cast<std::int32_t>(bytes.u32(p + 4));
      if (map_id == -1)
        continue;
      UiLobbyMap map;
      map.campaign_id = std::bit_cast<std::int32_t>(bytes.u32(p));
      map.map_id = map_id;
      map.bitmap = image_ref(bytes.u32(p + 12));
      map.name = localized_map_text(bytes, p + 0x10, 32);
      map.description = localized_map_text(bytes, p + 0x250, 128);
      // Original19C970->19C270 uses the campaign/map key pair from matg+170.
      for (const auto q : path_records)
        if (std::bit_cast<std::int32_t>(bytes.u32(q)) == map.campaign_id &&
            std::bit_cast<std::int32_t>(bytes.u32(q + 4)) == map.map_id) {
          map.scenario_path = bytes.string(q + 8, 256);
          break;
        }
      out.campaign_maps.push_back(std::move(map));
    }
    for (const auto p : metadata_block_offsets(levels.front() + 16, 0xc64, 256)) {
      const auto map_id = std::bit_cast<std::int32_t>(bytes.u32(p));
      if (map_id == -1)
        continue;
      UiLobbyMap map;
      map.map_id = map_id;
      map.bitmap = image_ref(bytes.u32(p + 8));
      map.name = localized_map_text(bytes, p + 0x0c, 32);
      map.description = localized_map_text(bytes, p + 0x24c, 128);
      map.scenario_path = bytes.string(p + 0xb4c, 256);
      map.sort_order = std::bit_cast<std::int32_t>(bytes.u32(p + 0xc4c));
      map.flags = std::uint8_t(std::to_integer<unsigned>(bytes.slice(p + 0xc50, 1)[0]));
      for (unsigned i = 0; i < map.team_counts.size(); ++i)
        map.team_counts[i] = std::uint8_t(std::to_integer<unsigned>(bytes.slice(p + 0xc54 + i, 1)[0]));
      out.multiplayer_maps.push_back(std::move(map));
    }
  }
  auto text = [&](std::size_t p) {
    UiText v;
    v.flags = bytes.u32(p);
    v.animation = bytes.u16(p + 4);
    v.delay = bytes.u16(p + 6);
    v.font = signed16(bytes, p + 10);
    v.color = color(bytes, p + 12);
    v.bounds = rect(bytes, p + 28);
    v.string = bytes.u32(p + 36);
    v.layer = signed16(bytes, p + 40);
    return v;
  };
  auto bitmap = [&](std::size_t p) {
    UiBitmap v;
    v.flags = bytes.u32(p);
    v.animation = bytes.u16(p + 4);
    v.delay = bytes.u16(p + 6);
    v.blend = bytes.u16(p + 8);
    v.frame = signed16(bytes, p + 10);
    v.position = point(bytes, p + 12);
    v.scroll = {bytes.f32(p + 16), bytes.f32(p + 20)};
    v.datum = image_ref(bytes.u32(p + 28));
    v.layer = signed16(bytes, p + 32);
    v.placeholder = bytes.u32(p + 44);
    v.fps = bytes.f32(p + 36);
    v.scale = {bytes.f32(p + 48), bytes.f32(p + 52)};
    for (auto f : {v.scroll.x, v.scroll.y, v.fps, v.scale.x, v.scale.y})
      if (!std::isfinite(f))
        throw FormatError("invalid UI bitmap scalar");
    return v;
  };
  auto grid = [&](std::size_t p) {
    UiGrid v;
    v.skin = bytes.u32(p + 8);
    v.position = point(bytes, p + 12);
    v.order = std::to_integer<unsigned>(bytes.slice(p + 16, 1)[0]);
    v.count = std::to_integer<unsigned>(bytes.slice(p + 17, 1)[0]);
    v.rows = std::to_integer<unsigned>(bytes.slice(p + 18, 1)[0]);
    v.columns = std::to_integer<unsigned>(bytes.slice(p + 19, 1)[0]);
    v.step = {float(signed16(bytes, p + 22)), float(signed16(bytes, p + 20))};
    if (v.skin != 0xffffffffu)
      requested_groups.insert(v.skin);
    return v;
  };
  auto emblem = [&](std::size_t p) {
    UiEmblem v;
    v.flags = bytes.u32(p);
    v.animation = bytes.u16(p + 4);
    v.delay = bytes.u16(p + 6);
    v.layer = signed16(bytes, p + 8);
    v.page = signed16(bytes, p + 10);
    v.bitmap = image_ref(bytes.u32(p + 16));
    v.shader = bytes.u32(p + 24);
    if (v.shader != 0xffffffffu && !out.emblem_shaders.contains(v.shader))
      out.emblem_shaders.emplace(v.shader, frontend_emblem_material(*this, v.shader));
    v.bounds = rect(bytes, p + 28);
    return v;
  };
  auto track = [&](unsigned duration, std::size_t p) {
    UiTrack v;
    v.milliseconds = duration;
    if (duration > 600000)
      throw FormatError("excessive UI animation duration");
    for (auto k : metadata_block_offsets(p, 20, 1024)) {
      UiKey key{bytes.f32(k + 4),
                {bytes.f32(k + 8), bytes.f32(k + 12), bytes.f32(k + 16)}};
      for (auto f : {key.alpha, key.position.x, key.position.y, key.position.z})
        if (!std::isfinite(f))
          throw FormatError("invalid UI keyframe");
      v.keys.push_back(key);
    }
    return v;
  };
  const auto shared =
      query_tag({.group=tag_groups::user_interface_shared_globals_definition});
  if (!shared)
    throw FormatError("frontend has no shared UI globals");
  const auto g = offset(shared->datum, "wigl", 452);
  out.typewriter = {std::bit_cast<std::int16_t>(bytes.u16(g + 0x48)),
                    bytes.u16(g + 0x4a), bytes.f32(g + 0x4c)};
  if (!std::isfinite(out.typewriter.scale))
    throw FormatError("invalid frontend typewriter scale");
  out.keyboard_caret = image_ref(bytes.u32(g + 0x114));
  const auto global_data = metadata_offset(globals->address, 0x110);
  if (bytes.u32(global_data + 0x108)) {
    // 237C6F reads the first interface record's default_2d bitmap reference.
    const auto interface = metadata_offset(bytes.u32(global_data + 0x10c), 0x68);
    out.keyboard_selection = image_ref(bytes.u32(interface + 0x64));
  }
  out.global_strings = string_list(bytes.u32(g + 0x11c));
  out.color = color(bytes, g + 0x168);
  out.overlay = color(bytes, g + 0x6c);
  out.music = bytes.u32(g + 0x1bc);
  out.music_silence_ms = bytes.u32(g + 0x1c0);
  if (out.music_silence_ms > 75000)
    throw FormatError("invalid menu music silence interval");
  if (!bsps_.empty())
    if (const auto sky = load_visible_sky(0)) {
      out.scene_clear = sky->clear;
      if (out.scene_clear)
        for (auto value : {out.scene_clear->x, out.scene_clear->y, out.scene_clear->z})
          if (!std::isfinite(value) || value < 0 || value > 1)
            throw FormatError("invalid menu sky clear color");
    }
  for (auto group : metadata_block_offsets(g + 0x88, 40, 256)) {
    UiScreen strings;
    strings.strings = string_list(bytes.u32(group + 12));
    for (auto entry : metadata_block_offsets(group + 32, 24, 512)) {
      UiDialog dialog;
      dialog.screen =
          (bytes.u16(group + 4) & 1) || (bytes.u16(entry + 4) & 1) ? 240 : 7;
      auto choices = std::to_integer<unsigned>(bytes.slice(entry + 6, 1)[0]);
      if (!choices)
        choices = std::to_integer<unsigned>(bytes.slice(group + 6, 1)[0]);
      dialog.selected = choices == 2 ? 1 : 0;
      std::string *fields[]{&dialog.title, &dialog.message, &dialog.first,
                            &dialog.second};
      for (unsigned i = 0; i < 4; ++i) {
        auto id = bytes.u32(entry + 8 + i * 4);
        if (!id)
          id = bytes.u32(group + 16 + i * 4);
        *fields[i] = out.localized(strings, id);
      }
      // The original mainmenu cache contains error 0x67 twice in group 3.
      // Retain the first serialized definition so authored data can load.
      out.dialogs.try_emplace(bytes.u32(entry), std::move(dialog));
    }
  }
  // Original236299 / Vista61DD04 read thirteen datum words at +94+i*8.
  for (unsigned i = 0; i < out.sounds.size(); ++i)
    out.sounds[i] = bytes.u32(g + 0x94 + i * 8);
  for (unsigned i = 0; i < 4; ++i)
    out.header_fonts[i] = signed16(bytes, g + 0x160 + i * 2);
  for (unsigned i = 0; i < 8; ++i)
    out.header_bounds[i] = rect(bytes, g + 0x178 + i * 8);
  const auto key_strings = bytes.u32(g + 0x144);
  const auto key_list = string_list(key_strings);
  // 1496F6 selects a string ID by subtitle enum, then1A0180 searches the
  // authored range. Serialized locale order is different (and enum5's ID
  // is absent); range position must not substitute an unrelated string.
  constexpr std::array<std::uint32_t, 22> key_ids{{
      0x0f0006ce, 0x110006cf, 0x100006d0, 0x130006d1, 0x110006d2,
      0x0e0006d3, 0x180006d4, 0x090006d5, 0x080006d6, 0x1a0006d7,
      0x180006d8, 0x080006d9, 0x080006da, 0x230006db, 0x250006dc,
      0x240006dd, 0x1c0006de, 0x230006df, 0x1c0006e0, 0x1c0006e1,
      0x180006e2, 0x040006e3}};
  for (const auto id : key_ids) {
    const auto found = key_list.find(id);
    out.button_keys.push_back(found == key_list.end() ? std::string{} : found->second);
  }
  for (auto p : metadata_block_offsets(g + 0x120, 44, 256))
    out.animations.push_back(
        {track(bytes.u32(p + 4), p + 8), track(bytes.u32(p + 16), p + 20),
         track(bytes.u32(p + 28), p + 36), signed16(bytes, p + 32)});
  auto load_skin = [&](std::uint32_t datum) {
    const auto p = offset(datum, "skin", 60);
    UiSkin skin;
    skin.name = tag(datum, "skin").name;
    skin.arrows = image_ref(bytes.u32(p + 8));
    skin.arrow_up = point(bytes, p + 12);
    skin.arrow_down = point(bytes, p + 16);
    for (auto a : metadata_block_offsets(p + 20, 16, 8))
      skin.animations.push_back(track(bytes.u32(a + 4), a + 8));
    for (auto a : metadata_block_offsets(p + 28, 44))
      skin.texts.push_back(text(a));
    for (auto a : metadata_block_offsets(p + 36, 56))
      skin.bitmaps.push_back(bitmap(a));
    for (auto a : metadata_block_offsets(p + 44, 36))
      skin.emblems.push_back(emblem(a));
    for (auto a : metadata_block_offsets(p + 52, 24))
      skin.grids.push_back(grid(a));
    return skin;
  };
  for (auto ref : metadata_block_offsets(g + 0x138, 8, 256)) {
    const auto datum = bytes.u32(ref + 4);
    auto skin = load_skin(datum);
    out.skin_groups.emplace(datum, skin);
    out.skins.push_back(std::move(skin));
  }
  for (const auto &t : select_tags({.group=tag_groups::user_interface_screen_widget_definition})) {
      const auto p = offset(t.datum, "wgit", 104);
      UiScreen screen;
      screen.name = t.name;
      screen.flags = bytes.u32(p);
      screen.id = bytes.u16(p + 4);
      screen.button_key = bytes.u16(p + 6);
      screen.color = color(bytes, p + 8);
      screen.strings = string_list(bytes.u32(p + 28));
      screen.shape = bytes.u16(p + 40);
      screen.header = bytes.u32(p + 44);
      for (auto q : metadata_block_offsets(p + 32, 76, 128)) {
        UiPane pane;
        pane.animation = bytes.u16(q + 2);
        for (auto a : metadata_block_offsets(q + 4, 60)) {
          UiButton b;
          b.text = text(a);
          b.text.string = bytes.u32(a + 48);
          b.text.layer = signed16(bytes, a + 52);
          b.bitmap = image_ref(bytes.u32(a + 40));
          b.offset = point(bytes, a + 44);
          pane.buttons.push_back(b);
        }
        for (auto a : metadata_block_offsets(q + 12, 24, 16)) {
          UiList l;
          l.flags = bytes.u32(a);
          l.skin = bytes.u16(a + 4);
          l.visible = bytes.u16(a + 6);
          l.position = point(bytes, a + 8);
          l.animation = bytes.u16(a + 12);
          l.delay = bytes.u16(a + 14);
          if (l.skin >= out.skins.size() || !l.visible || l.visible > 128)
            throw FormatError("invalid UI list definition");
          for (auto item : metadata_block_offsets(a + 16, 20))
            l.labels.push_back(bytes.u32(item + 16));
          pane.lists.push_back(std::move(l));
        }
        for (auto a : metadata_block_offsets(q + 28, 44))
          pane.texts.push_back(text(a));
        for (auto a : metadata_block_offsets(q + 36, 56))
          pane.bitmaps.push_back(bitmap(a));
        for (auto a : metadata_block_offsets(q + 44, 76, 32)) {
          UiScene scene;
          scene.flags = bytes.u32(a);
          scene.animation = bytes.u16(a + 4);
          scene.delay = bytes.u16(a + 6);
          scene.scale = {bytes.f32(a + 28), bytes.f32(a + 32),
                         bytes.f32(a + 36)};
          scene.camera = {bytes.f32(a + 40), bytes.f32(a + 44),
                          bytes.f32(a + 48)};
          scene.field_of_view = bytes.f32(a + 52);
          scene.viewport = rect(bytes, a + 56);
          for (auto v :
               {scene.scale.x, scene.scale.y, scene.scale.z, scene.camera.x,
                scene.camera.y, scene.camera.z, scene.field_of_view})
            if (!std::isfinite(v))
              throw FormatError("invalid UI model scene scalar");
          for (auto n : metadata_block_offsets(a + 12, 32, 64))
            scene.objects.push_back(bytes.string(n, 32));
          for (auto n : metadata_block_offsets(a + 20, 32, 64))
            scene.lights.push_back(bytes.string(n, 32));
          pane.scenes.push_back(std::move(scene));
        }
        // Literal 22FC08: +3C is type9 (36B), +44 is groups (24B).
        for (auto a : metadata_block_offsets(q + 60, 36))
          pane.emblems.push_back(emblem(a));
        for (auto a : metadata_block_offsets(q + 68, 24))
          pane.grids.push_back(grid(a));
        screen.panes.push_back(std::move(pane));
      }
      for (auto a : metadata_block_offsets(p + 56, 8))
        screen.bitmap_resources.push_back(image_ref(bytes.u32(a + 4)));
      if (!out.screens.emplace(screen.id, std::move(screen)).second)
        throw FormatError("duplicate UI screen id");
    }
  while (!requested_groups.empty()) {
    const auto datum = *requested_groups.begin();
    requested_groups.erase(requested_groups.begin());
    if (!out.skin_groups.contains(datum))
      out.skin_groups.emplace(datum, load_skin(datum));
  }
  for (const auto &t : select_tags({.group=tag_groups::text_value_pair_definition})) {
      const auto p = offset(t.datum, "sily", 36);
      UiScreen dummy;
      dummy.strings = string_list(bytes.u32(p + 12));
      UiParameter param;
      param.parameter = bytes.u32(p);
      param.default_value = std::bit_cast<int>(bytes.u32(p + 4));
      param.title = out.localized(dummy, bytes.u32(p + 16));
      param.header = out.localized(dummy, bytes.u32(p + 20));
      param.description = out.localized(dummy, bytes.u32(p + 24));
      for (auto a : metadata_block_offsets(p + 28, 12, 256))
        param.choices.push_back({out.localized(dummy, bytes.u32(a + 8)),
                                 std::bit_cast<int>(bytes.u32(a + 4))});
      out.parameters.emplace(t.datum, std::move(param));
    }
  for (const auto &t : select_tags({.group=tag_groups::multiplayer_variant_settings_interface_definition})) {
      const auto p = offset(t.datum, "goof", 48);
      UiScreen dummy;
      dummy.strings = string_list(bytes.u32(p + 36));
      for (auto q : metadata_block_offsets(p + 24, 24, 64))
        for (auto a : metadata_block_offsets(q + 8, 8, 256)) {
          const auto datum = bytes.u32(a + 4);
          if (!out.parameters.contains(datum))
            throw FormatError("missing variant parameter");
          out.categories[bytes.u32(q)].push_back(datum);
        }
      for (auto q : metadata_block_offsets(p + 40, 20, 256)) {
        UiVariant v;
        v.name = out.localized(dummy, bytes.u32(q));
        v.engine = bytes.u32(q + 4);
        for (auto a : metadata_block_offsets(q + 8, 8, 256))
          v.values.append(bytes.u32(a), std::bit_cast<int>(bytes.u32(a + 4)));
        // Original149F88 admits authored rows only for frontend types0..6
        // and only when19D650 accepts their complete constructed buffer.
        // Invalid rows are omitted; their corrupt values are not a new mode.
        if (v.engine < 7 && source_game_variant_valid(source_game_variant(v)))
          out.variants.push_back(std::move(v));
      }
    }
  for (const auto &t : select_tags())
    if (t.group == "bitm" &&
        (t.name.starts_with("ui\\code_global_bitmaps\\") ||
         t.name.starts_with("ui\\global_bitmaps\\emblems\\")))
      image_ref(t.datum);
  for (auto datum : requested_images) {
    const auto p = offset(datum, "bitm", 76);
    UiImage image;
    image.name = tag(datum, "bitm").name;
    for (auto q : metadata_block_offsets(p + 68, 116, 256)) {
      if (bytes.u16(q + 10) != 0)
        throw FormatError("UI bitmap is not two dimensional");
      const auto raw = raw_data(bytes.u32(q + 28), bytes.u32(q + 52));
      image.images.push_back(
          prepare_ui_bitmap(Bytes(raw), bytes.u16(q + 4), bytes.u16(q + 6),
                            bytes.u16(q + 12), bytes.u16(q + 14)));
    }
    for (auto q : metadata_block_offsets(p + 60, 60, 256)) {
      std::vector<UiSprite> sequence;
      for (auto s : metadata_block_offsets(q + 52, 32, 1024)) {
        UiSprite v{bytes.u16(s),      bytes.f32(s + 8),
                   bytes.f32(s + 12), bytes.f32(s + 16),
                   bytes.f32(s + 20), {bytes.f32(s + 24), bytes.f32(s + 28)}};
        if (v.image >= image.images.size() || !std::isfinite(v.left) ||
            !std::isfinite(v.right) || !std::isfinite(v.top) ||
            !std::isfinite(v.bottom) || v.left < 0 || v.top < 0 ||
            v.right > 1 || v.bottom > 1 || v.left > v.right || v.top > v.bottom)
          throw FormatError("UI sprite outside bitmap page");
        sequence.push_back(v);
      }
      if (sequence.empty())
        for (unsigned i = 0; i < bytes.u16(q + 34); ++i) {
          const auto index = bytes.u16(q + 32) + i;
          if (index >= image.images.size())
            throw FormatError("UI sequence outside bitmap records");
          sequence.push_back({index, 0, 1, 0, 1, {}});
        }
      image.sequences.push_back(std::move(sequence));
    }
    out.images.emplace(datum, std::move(image));
  }
  out.bundled_variants=bundled_slayer_variants();
  return out;
}
} // namespace halo2
