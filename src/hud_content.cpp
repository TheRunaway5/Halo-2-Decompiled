#include "halo2/hud.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>
namespace halo2 {
HudContent Cache::load_hud(std::uint32_t biped, std::uint32_t weapon) const {
  HudContent out;
  const Bytes data(cache_bytes());
  auto root = [&](std::uint32_t datum, const std::string &group,
                  unsigned minimum) {
    const auto t = query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group)});
    if (!t || t->size < minimum)
      throw FormatError("Missing HUD " + group);
    return metadata_offset(t->address, t->size);
  };
  
  auto byte = [&](std::size_t p) {
    return std::to_integer<unsigned>(data.slice(p, 1)[0]);
  };
  auto real = [&](std::size_t p) {
    const auto f = data.f32(p);
    if (!std::isfinite(f))
      throw FormatError("Non-finite HUD metadata");
    return f;
  };
  auto point = [&](std::size_t p) {
    return Vec2{float(std::int16_t(data.u16(p))),
                float(std::int16_t(data.u16(p + 2)))};
  };
  std::vector<std::pair<std::uint32_t, unsigned>> interfaces;
  const auto unit_interfaces = metadata_block_offsets(root(biped, "bipd", 0x1a8) + 0x1a0, 8);
  if(weapon!=null_datum) {
    const auto wp = root(weapon, "weap", 0x2c8);
    interfaces.emplace_back(data.u32(wp + 0x2b4), 1);
    const auto magazines = metadata_block_offsets(wp + 0x2c0, 0x5c, 16);
    out.weapon_magazine_count=unsigned(magazines.size());
    if (!magazines.empty()) {
      out.clip_capacity=std::bit_cast<std::int16_t>(data.u16(magazines[0]+0xa));
      out.reserve_capacity=std::bit_cast<std::int16_t>(data.u16(magazines[0]+0xc));
    }
  }
  const auto globals =
      query_tag({.group=tag_groups::globals});
  if (!globals)
    throw FormatError("Missing HUD globals");
  const auto gp = root(globals->datum, "matg", 0x1ac);
  const auto scenario = query_tag({.group=tag_groups::scenario});
  out.score_scenario_present = scenario && scenario->address && scenario->size;
  for (const auto p : metadata_block_offsets(gp + 0x160, 12)) {
    for (unsigned n=0;n<3;++n) (void)real(p+n*4);
    out.score_appearance_colors.push_back({data.u32(p),data.u32(p+4),data.u32(p+8)});
  }
  // Campaign caches retain a size-zero mulg reference. Action messages and
  // the unit/weapon HUD belong to their actual matg/nhdt/locale owners; they
  // must not require this multiplayer-only runtime interface.
  if(scenario && scenario->address &&
      data.u16(metadata_offset(scenario->address,0x12)+0x10)==1) {
    const auto mp = root(data.u32(gp + 0x16c), "mulg",16);
    const auto universal_count = data.u32(mp);
    if (universal_count>16) throw FormatError("Excessive HUD universal block");
    // The original query follows the first record's20-byte prefix; a full
    // universal-record stride is not required or inferred here.
    if (universal_count) for (const auto p : metadata_block_offsets(metadata_offset(data.u32(mp+4),0x20)+0x10, 12)) {
      for (unsigned n=0;n<3;++n) (void)real(p+n*4);
      out.score_team_colors.push_back({data.u32(p),data.u32(p+4),data.u32(p+8)});
    }
    const auto runtime = metadata_block_offsets(mp + 8, 0x568, 16);
    if (!runtime.empty())
      interfaces.emplace_back(data.u32(runtime[0] + 0x544), 0);
  }
  if (!unit_interfaces.empty())
    interfaces.emplace_back(
        data.u32(unit_interfaces[std::min(std::size_t(1),
                                          unit_interfaces.size() - 1)] +
                 4),
        0);
  const auto interface_tags = metadata_block_offsets(gp + 0x110, 0x98, 16);
  if (interface_tags.empty())
    throw FormatError("Missing HUD interface globals");
  const auto hp = root(data.u32(interface_tags[0] + 0x34), "hudg", 0x400);
  out.action_messages.enabled=true;
  out.action_messages.color=data.u32(hp+0xa4);
  for(const auto &tag:select_tags({.group=tag_groups::weapon, .require_address=true})){
    const auto p=root(tag.datum,"weap",0xf4);
    auto &messages=out.action_messages.weapons[tag.datum];
    for(unsigned i=0;i<messages.size();++i)messages[i]=data.u32(p+0xcc+i*4);
    out.action_messages.weapon_trades[tag.datum]=data.u32(p+0xf0);
  }
  for(const auto &tag:select_tags())if((tag.group=="bipd" || tag.group=="vehi") && tag.address){
    const auto p=root(tag.datum,tag.group.c_str(),tag.group=="vehi"?0x220:0x1d0);
    auto &messages=out.action_messages.units[tag.datum];
    for(const auto seat:metadata_block_offsets(p+0x1c8, 0xb0, 64))messages.seats.push_back(data.u32(seat+0x84));
    if(tag.group=="vehi")messages.flip=data.u32(p+0x21c);
  }
  out.multiplayer_scale = real(hp + 0x17c);
  out.sensor.range=real(hp+0x290);
  out.sensor.minimum_speed=real(hp+0x294);
  const auto player_info=metadata_block_offsets(gp+0x130, 0x11c, 16);
  if(player_info.empty())throw FormatError("Missing motion-sensor player information");
  out.sensor.sneak_speed=real(player_info[0]+0x3c);
  const auto representations=metadata_block_offsets(gp+0x138, 0xbc, 16);
  for(unsigned n=2;n<std::min(std::size_t(4),representations.size());++n) {
    const auto unit=data.u32(representations[n]+0xb4);
    if(unit==null_datum)continue;
    const auto size=std::int16_t(data.u16(root(unit,"bipd",0x196)+0x194));
    out.sensor.blip_sizes[n]=std::uint8_t(std::clamp(int(size),0,3));
  }
  auto sensor_bitmap=[&](unsigned offset) {
    const auto id=data.u32(interface_tags[0]+offset+4);
    const auto rows=metadata_block_offsets(root(id,"bitm",76)+68, 116, 256);
    if(rows.empty())throw FormatError("Missing authored motion-sensor image");
    const auto q=rows[0];
    const auto raw=raw_data(data.u32(q+28),data.u32(q+52));
    const auto width=data.u16(q+4),height=data.u16(q+6),format=data.u16(q+12),flags=data.u16(q+14);
    const auto mips=retail_mip_count(width,height,1,format,flags&16,data.u16(q+20));
    return prepare_xemu_bc_texture(prepare_texture(Bytes(raw),width,height,format,flags&8,mips));
  };
  out.sensor.artwork={sensor_bitmap(0x38),sensor_bitmap(0x40),
                     sensor_bitmap(0x60),sensor_bitmap(0x68)};
  const auto lp = gp + 0x190;
  const auto count = data.u32(lp), size = data.u32(lp + 4),
             indices = data.u32(lp + 8), strings = data.u32(lp + 12);
  if (count > 65536 || size > 16 * 1024 * 1024)
    throw FormatError("Excessive HUD locale table");
  data.require(indices, std::size_t(count) * 8);
  const Bytes table(data.slice(strings, size));
  const auto up = root(data.u32(hp + 0x3fc), "unic", 52);
  const auto start = data.u16(up + 16), length = data.u16(up + 18);
  if (start > count || length > count - start)
    throw FormatError("HUD locale range outside table");
  for (unsigned i = start; i < start + length; ++i) {
    const auto at = indices + i * 8, s = data.u32(at + 4);
    if (s >= size)
      throw FormatError("HUD string outside locale table");
    out.strings.emplace(data.u32(at), table.string(s, size - s));
  }
  // Action strings belong to each item/control's unic range, outside the
  // global nhdt text range. Resolve the same IDs from the current map locale.
  for(unsigned i=0;i<count;++i){
    const auto at=indices+i*8,s=data.u32(at+4);
    if(s>=size)throw FormatError("Action string outside locale table");
    out.strings.emplace(data.u32(at),table.string(s,size-s));
  }
  std::set<std::uint32_t> visited, requested;
  for (std::size_t i = 0; i < interfaces.size(); ++i) {
    const auto [id, type] = interfaces[i];
    if (id == null_datum || !visited.insert(id).second)
      continue;
    if (interfaces.size() > 64)
      throw FormatError("HUD inheritance cycle");
    const auto p = root(id, "nhdt", 0x28);
    interfaces.emplace_back(data.u32(p + 4), 0);
    if (i == 0) {
      out.clip_cutoff = data.u16(p + 0x18);
      out.ammo_cutoff = data.u16(p + 0x1a);
    }
    for (unsigned text = 0; text < 2; ++text)
      for (auto at : metadata_block_offsets(p + (text ? 0x10 : 8), text ? 0x54 : 0x64)) {
        HudWidget w;
        w.name = string_id(data.u32(at));
        w.is_text = text;
        w.weapon_type = type;
        for (unsigned k = 0; k < 4; ++k) {
          w.inputs[k] = byte(at + 4 + k);
          w.yes[k] = data.u16(at + 8 + k * 2);
          w.no[k] = data.u16(at + 16 + k * 2);
        }
        for (unsigned k = 0; k < 3; ++k)
          w.cutoffs[k] = byte(at + 24 + k);
        w.anchor = data.u16(at + 28);
        w.flags = data.u16(at + 30);
        w.shader = data.u32(at + (text ? 0x24 : 0x2c));
        if (!text) {
          w.bitmap = data.u32(at + 0x24);
          w.special = data.u16(at + 0x60);
          requested.insert(w.bitmap);
        } else {
          w.text = data.u32(at + 0x28);
          w.justification = data.u16(at + 0x2c);
        }
        for (unsigned k = 0; k < 3; ++k) {
          w.sequence[k] =
              std::bit_cast<std::int8_t>(std::uint8_t(byte(at + 0x30 + k)));
          w.fonts[k] = w.sequence[k];
          w.offset[k] = point(at + (text ? 0x40 : 0x34) + k * 4);
          if (!text)
            w.registration[k] = {real(at + 0x40 + k * 8),
                                 real(at + 0x44 + k * 8)};
        }
        for (const auto effect : metadata_block_offsets(at + (text ? 0x4c : 0x58), 0x68, 16)) {
          HudEffect e;
          e.flags = data.u32(effect);
          for (unsigned k = 0; k < 5; ++k) {
            const auto f = effect + 4 + k * 20;
            auto &v = e.functions[k];
            v.input = data.u32(f);
            v.range = data.u32(f + 4);
            v.period = real(f + 8);
            const auto n = data.u32(f + 12);
            if (n > 4096)
              throw FormatError("Excessive HUD effect curve");
            if (n) {
              const auto bytes =
                  data.slice(metadata_offset(data.u32(f + 16), n), n);
              v.curve.assign(bytes.begin(), bytes.end());
            }
          }
          w.effects.push_back(std::move(e));
        }
        out.widgets.push_back(std::move(w));
      }
  }
  for (const auto &w : out.widgets)
    if (w.shader != null_datum && !out.shaders.contains(w.shader))
      out.shaders.emplace(w.shader, load_material(w.shader, true));
  for (auto id : requested)
    if (id != null_datum) {
      const auto p = root(id, "bitm", 76);
      UiImage image;
      const auto t = query_tag({.handle=TagHandle{id}});
      image.name = t->name;
      for (auto q : metadata_block_offsets(p + 68, 116, 256)) {
        if (data.u16(q + 10) != 0)
          throw FormatError("HUD bitmap must be 2D");
        const auto raw = raw_data(data.u32(q + 28), data.u32(q + 52));
        image.images.push_back(
            prepare_ui_bitmap(Bytes(raw), data.u16(q + 4), data.u16(q + 6),
                              data.u16(q + 12), data.u16(q + 14)));
      }
      for (auto q : metadata_block_offsets(p + 60, 60, 256)) {
        std::vector<UiSprite> sequence;
        for (auto s : metadata_block_offsets(q + 52, 32, 1024)) {
          UiSprite v{data.u16(s),  real(s + 8),  real(s + 12),
                     real(s + 16), real(s + 20), {real(s + 24), real(s + 28)}};
          if (v.image >= image.images.size() || v.left < 0 || v.right > 1 ||
              v.top < 0 || v.bottom > 1 || v.left > v.right || v.top > v.bottom)
            throw FormatError("HUD sprite outside image");
          sequence.push_back(v);
        }
        if (sequence.empty())
          for (unsigned j = 0; j < data.u16(q + 34); ++j) {
            const auto index = data.u16(q + 32) + j;
            if (index >= image.images.size())
              throw FormatError("HUD sequence outside images");
            sequence.push_back({index, 0, 1, 0, 1, {}});
          }
        image.sequences.push_back(std::move(sequence));
      }
      out.images.emplace(id, std::move(image));
    }
  return out;
}
} // namespace halo2
