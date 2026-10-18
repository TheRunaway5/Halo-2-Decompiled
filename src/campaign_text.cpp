#include "halo2/campaign_text.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
namespace halo2 {
CampaignTextContent load_campaign_text(const Cache &cache,
                                       const std::filesystem::path &map) {
  const auto tag = [&](const char *group) -> const Tag & {
    const auto *t = cache.query_tag(
        {.group = TagGroup::from_string(group), .require_address = true});
    if (!t)
      throw FormatError("Missing campaign text source tag");
    return *t;
  };
  const auto source = cache.load_source_tag_metadata(tag("matg").datum);
  const Bytes globals(source.bytes_at(source.source_address(), source.size()));
  if (!globals.u32(0x110))
    throw FormatError("Missing campaign HUD interface");
  const Bytes interface(source.bytes_at(globals.u32(0x114), 0x98));
  const auto hud = cache.load_source_tag_metadata(interface.u32(0x34));
  const Bytes h(hud.bytes_at(hud.source_address(), hud.size()));
  const auto scenario = cache.load_source_tag_metadata(tag("scnr").datum);
  const Bytes s(scenario.bytes_at(scenario.source_address(), scenario.size()));
  const auto count = globals.u32(0x190), size = globals.u32(0x194);
  if (count > 65536 || size > 16 * 1024 * 1024)
    throw FormatError("Excessive campaign locale");
  std::vector<std::byte> indices,text;
  if(cache.is_mcc()) {
    // Version13 keeps these offsets in the logical decoded file. Shared
    // globals additionally mark their shared.map locale offsets with bit31;
    // they cannot be read from the compressed local multiplayer map.
    const auto index_offset=globals.u32(0x198),text_offset=globals.u32(0x19c);
    const bool shared=(index_offset&0x80000000u)!=0;
    const auto shared_bytes=shared?read_mcc_cache(cache.shared_directory()/"shared.map"):
        std::vector<std::byte>{};
    const auto owned=shared?std::span<const std::byte>(shared_bytes):cache.source_runtime_cache_bytes();
    // Cache appends normalized metadata after the decoded source file. Locale
    // file offsets must stay in the original logical file, not those owners.
    const Bytes logical(Bytes(owned).slice(0,Bytes(owned).u32(8)));
    const auto rows=logical.slice(shared?index_offset&0x7fffffffu:index_offset,std::size_t(count)*8);
    const auto strings=logical.slice(shared?text_offset&0x7fffffffu:text_offset,size);
    indices.assign(rows.begin(),rows.end());text.assign(strings.begin(),strings.end());
  } else {
    std::ifstream file(map, std::ios::binary | std::ios::ate);
    const auto length = file.tellg();
    const auto read = [&](unsigned offset, std::size_t bytes) {
      if (length < 0 || offset > std::uint64_t(length) ||
          bytes > std::uint64_t(length) - offset)
        throw FormatError("Campaign locale outside map");
      std::vector<std::byte> result(bytes);
      file.seekg(offset);
      if (!file.read(reinterpret_cast<char *>(result.data()), bytes))
        throw FormatError("Incomplete campaign locale");
      return result;
    };
    indices=read(globals.u32(0x198),std::size_t(count)*8);
    text=read(globals.u32(0x19c),size);
  }
  const Bytes rows(indices), locale(text);
  const auto strings = [&](std::uint32_t datum) {
    UiStrings result;
    if (datum == 0xffffffffu)
      return result;
    const auto owner = cache.load_source_tag_metadata(datum);
    const Bytes u(owner.bytes_at(owner.source_address(), owner.size()));
    const unsigned first = u.u16(16), n = u.u16(18);
    if (first > count || n > count - first)
      throw FormatError("Campaign unic outside locale");
    for (unsigned i = first; i < first + n; ++i) {
      const auto at = rows.u32(i * 8 + 4);
      if (at >= size)
        throw FormatError("Invalid campaign string");
      result.emplace(rows.u32(i * 8), locale.string(at, size - at));
    }
    return result;
  };
  CampaignTextContent result;
  result.training = strings(h.u32(0x3fc));
  const auto events=h.u32(0x420);
  if(events>32)throw FormatError("Excessive HUD training events");
  if(events) {
    const Bytes rows(hud.bytes_at(h.u32(0x424),events*20));
    for(unsigned p=0;p<events*20;p+=20) {
      TrainingEventDefinition d{rows.u32(p),rows.u16(p+4),rows.u16(p+6),
          rows.u16(p+8),rows.u16(p+10),rows.f32(p+12),rows.u32(p+16)};
      if(!std::isfinite(d.delay_seconds) || d.delay_seconds<0 || d.delay_seconds>32767/30.f)
        throw FormatError("Invalid training event delay");
      result.training_events.push_back(d);
    }
  }
  result.subtitles = strings(s.u32(0x35c));
  result.title_strings = strings(s.u32(0x204));
  const auto n = s.u32(0x1f0);
  if (n > 256)
    throw FormatError("Excessive cinematic titles");
  if (n) {
    const Bytes rows(scenario.bytes_at(s.u32(0x1f4), n * 0x24));
    for (unsigned p = 0; p < n * 0x24; p += 0x24) {
      CampaignTitleDefinition title;
      title.text = rows.u32(p);
      title.color = rows.u32(p + 0x10);
      title.shadow = rows.u32(p + 0x14);
      for (unsigned i = 0; i < 4; ++i)
        title.bounds[i] = std::int16_t(rows.u16(p + 4 + i * 2));
      title.justification = std::int16_t(rows.u16(p + 0xc));
      title.font = std::int16_t(rows.u16(p + 0xe));
      title.fade_in = rows.f32(p + 0x18);
      title.up_time = rows.f32(p + 0x1c);
      title.fade_out = rows.f32(p + 0x20);
      result.titles.push_back(title);
    }
  }
  for (unsigned i = 0; i < 4; ++i) {
    const auto datum = h.u32(0x44c + i * 8);
    result.panel[i] = datum;
    const auto *bitmap =
        cache.query_tag({.handle = TagHandle{datum}, .require_address = true});
    if (!bitmap)
      throw FormatError("Missing training panel bitmap");
    result.images.emplace(
        datum, UiImage{bitmap->name, {cache.load_bitmap(datum, 0)}, {}});
  }
  return result;
}
namespace {
std::vector<unsigned> characters(const std::string &s) {
  std::vector<unsigned> out;
  for (std::size_t i = 0; i < s.size() && out.size() < 255;) {
    const auto b = std::uint8_t(s[i++]);
    unsigned c = b, n = 0;
    if ((b & 0xe0) == 0xc0) {
      c = b & 31;
      n = 1;
    } else if ((b & 0xf0) == 0xe0) {
      c = b & 15;
      n = 2;
    } else if ((b & 0xf8) == 0xf0) {
      c = b & 7;
      n = 3;
    }
    if (i + n > s.size())
      break;
    while (n--)
      c = (c << 6) | (std::uint8_t(s[i++]) & 63);
    out.push_back(c);
  }
  return out;
}
struct Line {
  std::vector<unsigned> text;
  float width{};
};
std::vector<Line> lines(const RetailFont &font, const std::string &text,
                        float maximum) {
  std::vector<Line> result(1);
  unsigned previous = 0;
  for (auto c : characters(text)) {
    if (c == '\r')
      continue;
    if (c == '\n') {
      result.emplace_back();
      previous = 0;
      continue;
    }
    const auto at = font.glyphs.find(c);
    if (at == font.glyphs.end())
      continue;
    float kern = 0;
    if (const auto k = font.kerning.find({c, previous});
        k != font.kerning.end())
      kern = k->second;
    const float advance = at->second.advance + at->second.bearing_x;
    if (result.back().width + kern + advance > maximum &&
        !result.back().text.empty()) {
      result.emplace_back();
      kern = 0;
    }
    result.back().text.push_back(c);
    result.back().width += kern + advance;
    previous = c;
  }
  return result;
}
void quad(std::vector<Vertex> &out, UiRect r, unsigned material, Vec3 color,
          float alpha, std::array<float, 4> uv = {0, 0, 1, 1},
          unsigned rotation = 0) {
  std::array<Vec2, 4> coords{
      {{uv[0], uv[1]}, {uv[2], uv[1]}, {uv[2], uv[3]}, {uv[0], uv[3]}}};
  const std::array<Vec3, 4> p{{{r.left, r.top, alpha},
                               {r.right, r.top, alpha},
                               {r.right, r.bottom, alpha},
                               {r.left, r.bottom, alpha}}};
  for (auto i : {0u, 1u, 3u, 1u, 2u, 3u})
    out.push_back({p[i], color, coords[(i + rotation) % 4], material});
}
void text(std::vector<Vertex> &out, const RetailFont &font, unsigned material,
          const std::vector<Line> &rows, float center, float top, Vec3 color,
          float alpha) {
  const float height = std::max(1, font.ascent + font.descent + font.leading);
  for (const auto &row : rows) {
    float x = center - row.width * .5f;
    unsigned previous = 0;
    for (auto c : row.text) {
      const auto &g = font.glyphs.at(c);
      if (const auto k = font.kerning.find({c, previous});
          k != font.kerning.end())
        x += k->second;
      const float l = x + g.bearing_x, t = top + font.ascent - g.bearing_y;
      if (g.width && g.height)
        quad(out, {t, l, t + g.height, l + g.width}, material, color, alpha,
             retail_font_sample_uvs(font, c, 0, 0, g.width, g.height));
      x += g.advance + g.bearing_x;
      previous = c;
    }
    top += height;
  }
}
} // namespace
CampaignTextRenderer::CampaignTextRenderer(
    CampaignTextContent content, const std::filesystem::path &directory,
    const std::filesystem::path &vista_assets, Mesh &mesh)
    : content_(std::move(content)) {
  const auto font_source=load_gameplay_font_source(directory);
  std::string name;
  const auto assets = hud_menu_asset_directory(vista_assets);
  std::vector<std::string> sampled_names;
  if (!assets.empty()) {
    std::ifstream table(assets / "fonts/font_table.txt");
    while (std::getline(table, name)) {
      if (!name.empty() && name.back() == '\r')
        name.pop_back();
      if (name.empty() || name.find_first_of("/\\:") != name.npos ||
          name == "..")
        throw FormatError("Invalid sampled campaign font path");
      sampled_names.push_back(name);
    }
    if (sampled_names.size() != 12)
      throw FormatError("Missing Vista campaign font table");
  }
  const auto font = [&](unsigned selector) {
    auto result = load_gameplay_font(font_source,selector);
    if (!assets.empty()) {
      // Select the same ordinal from the sampled platform's actual table.
      // Xbox subtitle font9 is conduit-16; Vista font9 is conduit-13.
      auto path =
          hud_sampled_font_path(assets, selector, sampled_names[selector]);
      if (!std::filesystem::is_regular_file(path)) {
        const auto lower = [](std::string text) {
          for (auto &c : text)
            if (c >= 'A' && c <= 'Z')
              c += 32;
          return text;
        };
        for (const auto &entry :
             std::filesystem::directory_iterator(assets / "fonts"))
          if (lower(entry.path().filename().string()) ==
              lower(path.filename().string())) {
            path = entry.path();
            break;
          }
      }
      if(font_source.domain==GameplayFontDomain::Original)
        apply_retail_font_sampling(result, load_retail_font(path));
      else {
        if(!is_mcc_classic_hud_pack(assets) && !is_mcc_classic_menu_pack(assets))
          throw FormatError("Custom MCC gameplay font redraw needs an explicit original-metric projection");
        require_mcc_gameplay_font_sample(font_source,selector,path);
      }
    }
    return result;
  };
  training_ = font(6);
  subtitle_ = font(9);
  const auto material = [&](std::string name, Texture texture) {
    const auto id = unsigned(mesh.materials.size());
    mesh.materials.push_back({std::move(name), std::move(texture)});
    mesh.materials.back().ui_full_surface = true;
    return id;
  };
  training_material_ = material("campaign/training/font6", training_.atlas);
  subtitle_material_ = material("campaign/subtitles/font9", subtitle_.atlas);
  for (const auto &title : content_.titles)
    if (!title_fonts_.contains(title.font)) {
      if (title.font < 0 || title.font >= 11)
        throw FormatError("Invalid cinematic title font");
      auto f = font(unsigned(title.font));
      const auto id =
          material("campaign/title/font" + std::to_string(title.font), f.atlas);
      title_fonts_.emplace(title.font, std::pair{std::move(f), id});
    }
  const auto samples =
      load_ui_asset_library(content_.images, assets, "hud.tsv");
  for (unsigned i = 0; i < 4; ++i) {
    const auto &image = content_.images.at(content_.panel[i]);
    const auto sample = samples.find(image.name);
    panel_materials_[i] =
        material(image.name, sample == samples.end() ? image.images.front()
                                                     : sample->second.front());
  }
}
std::vector<Vertex> CampaignTextRenderer::draw(
    const PlayerTrainingState &training, const CampaignSubtitle &subtitle,
    unsigned preference, unsigned width, unsigned height, float letterbox,
    const std::function<std::string(const std::string &)> &substitute,
    std::span<const CampaignTitle> titles) const {
  if (!width || !height)
    throw FormatError("Empty campaign text viewport");
  std::vector<Vertex> out;
  const auto training_id=training.show && training.text && training.text!=0xffffffffu
      ?training.text:training.automatic.text(content_.training_events);
  if (training.enabled && training_id && training_id != 0xffffffffu)
    if (const auto at = content_.training.find(training_id);
        at != content_.training.end()) {
      const auto rows = lines(
          training_, substitute ? substitute(at->second) : at->second, 544);
      float w = 0;
      for (const auto &row : rows)
        w = std::max(w, row.width);
      const float height =
          float(rows.size() * std::max(1, training_.ascent + training_.descent +
                                              training_.leading));
      const float l = 320 - w * .5f - 1, r = 320 + w * .5f + 1,
                  t = 240 - height * .5f - 5, b = 240 + height * .5f + 5;
      const float il = l + 8, ir = r - 8, it = t + 8, ib = b - 8;
      if (ir > il && ib > it) {
        quad(out, {it, il, ib, ir}, panel_materials_[3], {1, 1, 1}, 1);
        quad(out, {it - 10, il, it, ir}, panel_materials_[1], {1, 1, 1}, 1);
        quad(out, {ib, il, ib + 10, ir}, panel_materials_[1], {1, 1, 1}, 1,
             {0, 0, 1, 1}, 2);
        quad(out, {it, il - 10, ib, il}, panel_materials_[1], {1, 1, 1}, 1,
             {0, 0, 1, 1}, 1);
        quad(out, {it, ir, ib, ir + 10}, panel_materials_[1], {1, 1, 1}, 1,
             {0, 0, 1, 1}, 3);
        quad(out, {it - 10, il - 10, it, il}, panel_materials_[2], {1, 1, 1},
             1);
        quad(out, {ib, ir, ib + 10, ir + 10}, panel_materials_[2], {1, 1, 1}, 1,
             {0, 0, 1, 1}, 2);
        quad(out, {it - 10, ir, it, ir + 10}, panel_materials_[0], {1, 1, 1},
             1);
        quad(out, {ib, il - 10, ib + 10, il}, panel_materials_[0], {1, 1, 1}, 1,
             {0, 0, 1, 1}, 2);
      }
      text(out, training_, training_material_, rows, 320, 240 - height * .5f,
           {.7137255073f, .8588235378f, 1}, 1);
    }
  // Current native locale and Xbox English map language are both0. This is
  // the actual supported locale, not an inferred language from the OS.
  if (subtitle.text && campaign_subtitles_visible(preference, 0, 0))
    if (const auto at = content_.subtitles.find(subtitle.text);
        at != content_.subtitles.end()) {
      const auto rows = lines(subtitle_, at->second, 544);
      const float line =
          std::max(1, subtitle_.ascent + subtitle_.descent + subtitle_.leading);
      const float bottom = std::min(
                      444.f, 480.f - 60.f * std::clamp(letterbox, 0.f, 1.f)),
                  top = bottom - line * rows.size();
      text(out, subtitle_, subtitle_material_, rows, 321, top + 2, {0, 0, 0},
           subtitle.alpha());
      text(out, subtitle_, subtitle_material_, rows, 320, top,
           {.76078433f, .92941177f, 1}, subtitle.alpha());
    }
  for (const auto &state : titles)
    if (state.index >= 0 && std::size_t(state.index) < content_.titles.size()) {
      const auto &d = content_.titles[state.index];
      const auto at = content_.title_strings.find(d.text);
      if (at == content_.title_strings.end())
        continue;
      const auto &[font, material] = title_fonts_.at(d.font);
      const float seconds = state.ticks / 30.f;
      float alpha = 1;
      if (seconds < d.fade_in)
        alpha = d.fade_in > 0 ? seconds / d.fade_in : 1;
      else if (seconds > d.up_time)
        alpha = d.fade_out > 0 ? 1 - (seconds - d.up_time) / d.fade_out : 0;
      alpha = std::clamp(alpha, 0.f, 1.f);
      auto bounds = d.bounds;
      if (bounds[1] == bounds[3] || bounds[0] == bounds[2])
        bounds = {0, 0, 480, 640};
      auto rows = lines(font, at->second, float(bounds[3] - bounds[1]));
      const auto rgb = [](unsigned c) {
        return Vec3{float((c >> 16) & 255) / 255, float((c >> 8) & 255) / 255,
                    float(c & 255) / 255};
      };
      float top = bounds[0];
      for (const auto &row : rows) {
        const float center = d.justification == 0 ? bounds[1] + row.width * .5f
                             : d.justification == 2
                                 ? bounds[3] - row.width * .5f
                                 : (bounds[1] + bounds[3]) * .5f;
        text(out, font, material, {row}, center + 1, top + 2, rgb(d.shadow),
             alpha);
        text(out, font, material, {row}, center, top, rgb(d.color), alpha);
        top += std::max(1, font.ascent + font.descent + font.leading);
      }
    }
  // These materials draw in the full surface viewport, like the gameplay
  // HUD. Preserve the authored glyph and panel aspect on wider displays.
  const float scale=std::min(width/640.f,height/480.f),
              left=(width-640*scale)*.5f,top=(height-480*scale)*.5f;
  for(auto &vertex:out) {
    vertex.position.x=(vertex.position.x*scale+left)*640/width;
    vertex.position.y=(vertex.position.y*scale+top)*480/height;
  }
  return out;
}
} // namespace halo2
