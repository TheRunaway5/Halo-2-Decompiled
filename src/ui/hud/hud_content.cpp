#include "halo2/hud.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/texture.hpp"
#include "halo2/mcc_hud_reference.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>
namespace halo2 {
HudContent Cache::load_hud(std::uint32_t biped, std::uint32_t weapon) const {
  return load_hud(biped,weapon,null_datum);
}
HudContent Cache::load_hud_definition(std::uint32_t biped,std::uint32_t hud_definition)const {
  auto out=load_hud(biped,null_datum,hud_definition);
  std::erase_if(out.widgets,[](const auto &w){return w.weapon_type!=5;});
  for(auto &w:out.widgets)w.weapon_type=0;
  out.action_messages.enabled=false;out.messages.enabled=false;return out;
}
bool Cache::resident_hud_definition(std::uint32_t id)const {
  if(id==null_datum)return false;
  const auto tag=query_tag({.handle=TagHandle{id},.group=TagGroup::from_string("nhdt")});
  if(!tag)throw FormatError("HUD references an invalid nhdt tag");
  if(!tag->address || !tag->size)return false;
  if(tag->size<0x28)throw FormatError("Truncated HUD definition");
  return true;
}
std::vector<std::uint32_t> Cache::load_unit_seat_huds(std::uint32_t unit)const {
  const auto tag=query_tag({.handle=TagHandle{unit},.require_address=true});
  if(!tag || (tag->group!="bipd" && tag->group!="vehi") || tag->size<0x1d0)
    throw FormatError("Seat HUD requires an authored unit definition");
  const Bytes data(cache_bytes());std::set<std::uint32_t> ids;
  for(const auto seat:metadata_block_offsets(metadata_offset(tag->address,tag->size)+0x1c8u,0xb0))
    for(const auto row:metadata_block_offsets(seat+0x7cu,8))
      if(const auto id=data.u32(row+4);id!=null_datum)ids.insert(id);
  return {ids.begin(),ids.end()};
}
HudContent Cache::load_hud(std::uint32_t biped,std::uint32_t weapon,std::uint32_t hud_definition) const {
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
  const auto hud_operand = [&](std::uint32_t id) {
    return mcc_ ? normalize_mcc_hud_function_operand(id, string_id(id)) : id;
  };
  std::vector<std::pair<std::uint32_t, unsigned>> interfaces;
  if(hud_definition!=null_datum)interfaces.emplace_back(hud_definition,5);
  const auto unit_interfaces = metadata_block_offsets(root(biped, "bipd", 0x1a8) + 0x1a0, 8);
  if(weapon!=null_datum) {
    const auto wp = root(weapon, "weap", 0x2d0);
    interfaces.emplace_back(data.u32(wp + 0x2b4), 1);
    const auto magazines = metadata_block_offsets(wp + 0x2c0, 0x5c, 16);
    out.weapon_magazine_count=unsigned(magazines.size());
    const auto triggers=metadata_block_offsets(wp+0x2c8,0x40,16);
    out.weapon_trigger_count=unsigned(triggers.size());
    if(!triggers.empty()) {
      out.weapon_trigger_behavior=std::bit_cast<std::int16_t>(data.u16(triggers[0]+6));
      out.weapon_charging_type=std::bit_cast<std::int16_t>(data.u16(triggers[0]+0x18));
    }
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
  const auto hud_representations=metadata_block_offsets(gp+0x138u,0xbcu,16);
  for(unsigned i=0;i<hud_representations.size();++i)
    if(data.u32(hud_representations[i]+0xb4u)==biped) {out.unit_representation=i;break;}
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
  out.messages={true,real(hp+0x58),real(hp+0x5c),real(hp+0x80)};
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
  for(unsigned n=2;n<std::min(std::size_t(4),hud_representations.size());++n) {
    const auto unit=data.u32(hud_representations[n]+0xb4);
    if(unit==null_datum)continue;
    const auto size=std::int16_t(data.u16(root(unit,"bipd",0x196)+0x194));
    out.sensor.blip_sizes[n]=std::uint8_t(std::clamp(int(size),0,3));
  }
  const auto mcc_bitmap=[&](std::uint32_t id,unsigned frame) {
    const auto images=bitmap_count(id);
    if(!images || images>256 || frame>=images)
      throw FormatError("MCC HUD bitmap frame escapes its bounded image bank");
    const auto resource=load_bitmap_resource(id,frame);
    if(resource.type!=0 || resource.depth!=1 || resource.levels[0].empty())
      throw FormatError("MCC HUD bitmap requires its actual 2D texture resource");
    auto image=load_bitmap(id,frame);
    if(image.width!=resource.width || image.height!=resource.height ||
       image.layers!=1 || image.depth || image.mips.empty() ||
       image.mips[0].width!=image.width || image.mips[0].height!=image.height ||
       image.mips[0].depth!=1 || image.mips[0].offset ||
       !image.mips[0].size || image.mips[0].size>image.pixels.size())
      throw FormatError("MCC HUD bitmap differs from its decoded texture owner");
    return image;
  };
  auto sensor_bitmap=[&](unsigned offset) {
    const auto id=data.u32(interface_tags[0]+offset+4);
    if(mcc_) {
      auto image=mcc_bitmap(id,0);
      return image.format>=14 && image.format<=16?
          prepare_xemu_bc_texture(image):image;
    }
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
  std::vector<std::byte> mcc_indices,mcc_strings;
  if(mcc_) {
    // The serialized English cache offsets keep their actual local/shared
    // file route. Runtime locale pointers and loaded flags remain untouched.
    const auto location=indices>>30;
    if(location!=(strings>>30) || (location!=0 && location!=2))
      throw FormatError("MCC HUD locale has an unowned local/shared route");
    if(count)mcc_indices=raw_data(indices,count*8);
    if(size)mcc_strings=raw_data(strings,size);
  }else data.require(indices, std::size_t(count) * 8);
  const Bytes references=mcc_?Bytes(mcc_indices):data;
  const auto reference_base=mcc_?0u:indices;
  const Bytes table=mcc_?Bytes(mcc_strings):Bytes(data.slice(strings, size));
  const auto up = root(data.u32(hp + 0x3fc), "unic", 52);
  const auto start = data.u16(up + 16), length = data.u16(up + 18);
  if (start > count || length > count - start)
    throw FormatError("HUD locale range outside table");
  for (unsigned i = start; i < start + length; ++i) {
    const auto at = reference_base + i * 8, s = references.u32(at + 4);
    if (s >= size)
      throw FormatError("HUD string outside locale table");
    out.strings.emplace(references.u32(at), table.string(s, size - s));
  }
  // Action strings belong to each item/control's unic range, outside the
  // global nhdt text range. Resolve the same IDs from the current map locale.
  for(unsigned i=0;i<count;++i){
    const auto at=reference_base+i*8,s=references.u32(at+4);
    if(s>=size)throw FormatError("Action string outside locale table");
    out.strings.emplace(references.u32(at),table.string(s,size-s));
  }
  std::set<std::uint32_t> visited, requested;
  for (std::size_t i = 0; i < interfaces.size(); ++i) {
    const auto [id, type] = interfaces[i];
    if (id == null_datum || !visited.insert(id).second)
      continue;
    if (interfaces.size() > 64)
      throw FormatError("HUD inheritance cycle");
    const auto p = root(id, "nhdt", 0x28);
    interfaces.emplace_back(data.u32(p + 4), type==5?5:0);
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
          // Original numeric text dispatches through the same HUD callback;
          // ordinary localized text retains its real MCC locale StringID.
          if (mcc_ && (w.flags & 1))
            w.text = hud_operand(w.text);
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
            v.input = hud_operand(data.u32(f));
            v.range = hud_operand(data.u32(f + 4));
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
    if (w.shader != null_datum && !out.shaders.contains(w.shader)) {
      auto material = load_material(w.shader, true);
      //22AFF1/TU2319F1 special1/2 supplies the selected score player's
      //appearance.22A664 keeps the dummy bitmap's quad, but samples the
      //waypoint_emblem pass's own foreground/background image banks.
      if(!material.passes.empty() &&
          material.passes.front().name=="shaders\\shader_passes\\hud\\waypoint_emblem") {
        material=frontend_emblem_material(*this,w.shader);
        for(const auto &bitmap:material.bitmaps)requested.insert(bitmap.datum);
      }
      if(!w.effects.empty() && !material.animation.tables)
        material.animation.tables=shader_function_tables();
      if (mcc_)
        for (auto &overlay : material.animation.overlays) {
          overlay.input = hud_operand(overlay.input);
          overlay.range = hud_operand(overlay.range);
        }
      out.shaders.emplace(w.shader, std::move(material));
    }
  for (auto id : requested)
    if (id != null_datum) {
      const auto p = root(id, "bitm", 76);
      UiImage image;
      const auto t = query_tag({.handle=TagHandle{id}});
      image.name = t->name;
      if(mcc_) {
        const auto count=bitmap_count(id);
        if(count>256)throw FormatError("Excessive MCC HUD bitmap images");
        for(unsigned frame=0;frame<count;++frame) {
          auto texture=mcc_bitmap(id,frame);
          // The existing UI path uses the highest-detail image only. MCC's
          // loader already owns its packed DAT layout and mip extents; Xbox
          // linear row pitch must not be applied to that resource.
          texture.pixels.resize(texture.mips[0].size);texture.mips.resize(1);
          if(texture.format==16)texture=prepare_xemu_bc3_texture(texture);
          image.images.push_back(std::move(texture));
        }
      }else for (auto q : metadata_block_offsets(p + 68, 116, 256)) {
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
  // These decoded image writers are local to this loader and have expired.
  // Later HUD/material value copies can share the completed bytes; a caller
  // taking a mutable view still detaches through TexturePixels as before.
  for(auto& [id,image]:out.images)for(auto& texture:image.images)texture.pixels.publish();
  return out;
}
} // namespace halo2
