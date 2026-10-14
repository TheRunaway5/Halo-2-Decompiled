#include "halo2/native_unit_interaction.hpp"
#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/prop_sandbox.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/source_game_variant.hpp"
#include <bit>
#include <functional>
#include <limits>
#include <set>
#include <sstream>

namespace halo2 {
namespace {

int i16(const Bytes &b, std::size_t p) {
  return static_cast<std::int16_t>(b.u16(p));
}
unsigned byte(const Bytes &b, std::size_t p) {
  return std::to_integer<unsigned>(b.slice(p, 1)[0]);
}
float real(const Bytes &b, std::size_t p) {
  const auto v = b.f32(p);
  if (!std::isfinite(v) || std::abs(v) > 1e7f)
    throw FormatError("invalid content float");
  return v;
}
Vec3 point(const Bytes &b, std::size_t p) {
  return {real(b, p), real(b, p + 4), real(b, p + 8)};
}
std::vector<CollisionTriangle> collision(const Bytes &b, std::size_t at,
                                         std::int64_t magic) {
  const auto surfaces = TagAddressSpace(b, magic).block(at + 0x28, 8),
             edges = TagAddressSpace(b, magic).block(at + 0x30, 12),
             vertices = TagAddressSpace(b, magic).block(at + 0x38, 16),
             planes = TagAddressSpace(b, magic).block(at + 8, 16);
  std::vector<CollisionTriangle> result;
  for (std::size_t s = 0; s < surfaces.count; ++s) {
    const auto p = surfaces.offset + s * 8;
    const auto flags = byte(b, p + 4);
    if (flags & 16)
      continue;
    const auto plane=b.u16(p);
    if((plane&0x7fff)>=planes.count)throw FormatError("collision surface plane outside block");
    const auto normal=point(b,planes.offset+(plane&0x7fff)*16)*(plane&0x8000?-1.f:1.f);
    const auto first = b.u16(p + 2);
    unsigned e = first;
    std::vector<Vec3> polygon;
    do {
      if (e >= edges.count || polygon.size() > edges.count)
        throw FormatError("invalid collision edge cycle");
      const auto edge = edges.offset + e * 12;
      const bool left = b.u16(edge + 8) == s;
      if (!left && b.u16(edge + 10) != s)
        throw FormatError("collision edge does not reference surface");
      const auto v = b.u16(edge + (left ? 0 : 2));
      if (v >= vertices.count)
        throw FormatError("collision vertex index outside block");
      polygon.push_back(point(b, vertices.offset + v * 16));
      e = b.u16(edge + (left ? 4 : 6));
    } while (e != first);
    for (std::size_t v = 1; v + 1 < polygon.size(); ++v)
      if (length(cross(polygon[v] - polygon[0], polygon[v + 1] - polygon[0])) >
          1e-8f)
        result.push_back({polygon[0], polygon[v], polygon[v + 1], b.u16(p + 6),
                          static_cast<std::uint8_t>(flags),
                          {-1,-1,static_cast<std::int32_t>(s),-1,plane},normal});
  }
  return result;
}
} // namespace
BspContent Cache::load_bsp_content(std::size_t index) const {
  if (index >= bsps_.size())
    throw FormatError("content BSP index outside scenario");
  BspContent out;
  const Bytes data(cache_bytes());
  const auto &bsp = bsps_[index];
  const Bytes meta(data.slice(bsp.file_offset, bsp.size));
  const auto magic = std::int64_t(bsp.memory_address);
  const auto collision_materials=TagAddressSpace(meta, magic).block(16+0xc, 0x14);
  const auto global_material=[&](std::uint16_t local){
    if(local==0xffff)return local;
    if(local>=collision_materials.count)throw FormatError("BSP collision material outside authored bank");
    return meta.u16(collision_materials.offset+std::size_t(local)*0x14+8);
  };
  const auto records = TagAddressSpace(meta, magic).block(16 + 0x14, 0x40);
  for (std::size_t i = 0; i < records.count; ++i) {
    auto tris = collision(meta, records.offset + i * 0x40, magic);
    for(auto& triangle:tris) {
      triangle.material=global_material(triangle.material);
      triangle.source.bsp=static_cast<std::int32_t>(index);
      triangle.source.record=static_cast<std::int32_t>(i);
    }
    out.collision.insert(out.collision.end(), tris.begin(), tris.end());
  }
  const auto navigation = TagAddressSpace(meta, magic).block(16 + 0xc4, 0x74);
  for (std::size_t n = 0; n < navigation.count; ++n) {
    const auto p = navigation.offset + n * 0x74;
    const auto sectors = TagAddressSpace(meta, magic).block(p, 8),
               links = TagAddressSpace(meta, magic).block(p + 8, 16),
               vertices = TagAddressSpace(meta, magic).block(p + 0x28, 12);
    const auto base = out.navigation.size();
    out.navigation.resize(base + sectors.count);
    for (std::size_t i = 0; i < sectors.count; ++i) {
      const auto q = sectors.offset + i * 8;
      auto &sector = out.navigation[base + i];
      sector.flags = meta.u16(q);
      if (!(sector.flags & 1) || (sector.flags & 4))
        continue;
      const auto first = std::int32_t(meta.u32(q + 4));
      if (first < 0)
        continue;
      auto edge = first;
      std::set<int> visited;
      do {
        if (edge < 0 || std::size_t(edge) >= links.count ||
            !visited.insert(edge).second)
          throw FormatError("invalid navigation sector edge cycle");
        const auto e = links.offset + std::size_t(edge) * 16;
        const bool left = i16(meta, e + 12) == int(i);
        if (!left && i16(meta, e + 14) != int(i))
          throw FormatError("navigation edge does not reference sector");
        const auto v = i16(meta, e + (left ? 0 : 2));
        if (v < 0 || std::size_t(v) >= vertices.count)
          throw FormatError("navigation vertex outside block");
        sector.polygon.push_back(
            point(meta, vertices.offset + std::size_t(v) * 12));
        edge = i16(meta, e + (left ? 8 : 10));
      } while (edge != first);
    }
    for (std::size_t i = 0; i < links.count; ++i) {
      const auto p = links.offset + i * 16;
      const auto left = i16(meta, p + 12), right = i16(meta, p + 14);
      const auto flags = meta.u16(p + 4);
      if (left < 0 || right < 0)
        continue;
      if (std::size_t(left) >= sectors.count ||
          std::size_t(right) >= sectors.count)
        throw FormatError("navigation portal sector outside block");
      // Retail26C380 admits ordinary walkable neighbors independently of
      // link bit5. Serialized Armory floor links commonly leave it clear.
      // Other sector kinds retain their existing flagged-link admission;
      // mobile and breakable sector owners are separate from this backend.
      const bool ordinary_neighbors = (meta.u16(sectors.offset + std::size_t(left) * 8) & 7) == 1 &&
                                      (meta.u16(sectors.offset + std::size_t(right) * 8) & 7) == 1;
      if (!(flags & 32) && !ordinary_neighbors)
        continue;
      const auto a = i16(meta, p), b = i16(meta, p + 2);
      if (a < 0 || b < 0 || std::size_t(a) >= vertices.count ||
          std::size_t(b) >= vertices.count)
        throw FormatError("navigation portal vertex outside block");
      const auto midpoint =
          (point(meta, vertices.offset + std::size_t(a) * 12) +
           point(meta, vertices.offset + std::size_t(b) * 12)) *
          .5f;
      out.navigation[base + left].portals.push_back(
          {base + std::size_t(right), midpoint, flags});
      out.navigation[base + right].portals.push_back(
          {base + std::size_t(left), midpoint, flags});
    }
  }
  const auto definitions = TagAddressSpace(meta, magic).block(16 + 0x138, 0xc8),
             instances = TagAddressSpace(meta, magic).block(16 + 0x140, 0x58);
  std::vector<std::vector<CollisionTriangle>> prototypes(definitions.count);
  std::vector<bool> loaded(definitions.count);
  for (std::size_t i = 0; i < instances.count; ++i) {
    const auto at = instances.offset + i * 0x58;
    const auto def = meta.u16(at + 0x34);
    if (def >= definitions.count)
      throw FormatError("invalid instance collision definition");
    if (!loaded[def]) {
      prototypes[def] =
          collision(meta, definitions.offset + def * 0xc8 + 0x70, magic);
      loaded[def] = true;
    }
    const auto scale = real(meta, at);
    std::array<float, 12> m{};
    for (std::size_t j = 0; j < m.size(); ++j)
      m[j] = real(meta, at + 4 + j * 4);
    auto transform = [&](Vec3 p) {
      return Vec3{scale * (p.x * m[0] + p.y * m[3] + p.z * m[6]) + m[9],
                  scale * (p.x * m[1] + p.y * m[4] + p.z * m[7]) + m[10],
                  scale * (p.x * m[2] + p.y * m[5] + p.z * m[8]) + m[11]};
    };
    for (auto tri : prototypes[def]) {
      tri.material=global_material(tri.material);
      tri.a = transform(tri.a);
      tri.b = transform(tri.b);
      tri.c = transform(tri.c);
      const auto n=tri.source_normal;
      tri.source_normal={n.x*m[0]+n.y*m[3]+n.z*m[6],n.x*m[1]+n.y*m[4]+n.z*m[7],n.x*m[2]+n.y*m[5]+n.z*m[8]};
      tri.source.bsp=static_cast<std::int32_t>(index);
      tri.source.record=static_cast<std::int32_t>(def);
      tri.source.instance=static_cast<std::int32_t>(i);
      out.collision.push_back(tri);
    }
    if (out.collision.size() > 5000000)
      throw FormatError("collision expansion limit exceeded");
  }
  return out;
}
std::vector<std::vector<std::byte>> Cache::load_model_resources(std::uint32_t datum) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::render_model});
  if (!tag || tag->size<0x80)
    throw FormatError("missing/truncated predicted render model");
  const Bytes data(cache_bytes());
  const auto base=metadata_offset(tag->address,tag->size);
  const auto sections=TagAddressSpace(data, metadata_magic_).block(base+0x24, 0x5c);
  std::vector<std::vector<std::byte>> out;
  for (std::size_t i=0;i<sections.count;++i) {
    const auto at=sections.offset+i*0x5c+0x38;
    out.push_back(data.u32(at+4) ? raw_data(data.u32(at),data.u32(at+4)) :
                                 std::vector<std::byte>{});
  }
  // The original model predictor also admits the first PRT geometry block.
  const auto prt=TagAddressSpace(data, metadata_magic_).block(base+0x74, 0x58);
  if (prt.count) {
    const auto at=prt.offset+0x34;
    if (data.u32(at+4)) out.push_back(raw_data(data.u32(at),data.u32(at+4)));
  }
  return out;
}
std::vector<std::vector<std::byte>> Cache::load_bsp_resources(std::size_t index) const {
  if (index>=bsps_.size()) throw FormatError("prediction BSP outside scenario");
  const auto &bsp=bsps_[index];
  const Bytes data(cache_bytes()), meta(data.slice(bsp.file_offset,bsp.size));
  const auto magic=std::int64_t(bsp.memory_address);
  std::vector<std::vector<std::byte>> out;
  for (bool definition:{false,true}) {
    const auto stride=definition ? 0xc8 : 0xb0;
    const auto entries=TagAddressSpace(meta, magic).block(16+(definition ? 0x138 : 0x9c), stride);
    for (std::size_t i=0;i<entries.count;++i) {
      const auto at=entries.offset+i*stride+0x28;
      out.push_back(meta.u32(at+4) ? raw_data(meta.u32(at),meta.u32(at+4)) :
                                   std::vector<std::byte>{});
    }
  }
  return out;
}
std::vector<PredictedResource> Cache::object_resources(std::uint32_t datum) const {
  if (datum == null_datum) return {};
  const auto *tag = find_tag({datum}, tag_groups::object);
  if (!tag || !tag->address || tag->size < 0xbc)
    throw FormatError("missing/truncated predicted object definition");
  const Bytes data(cache_bytes());
  const auto base = metadata_offset(tag->address, tag->size);
  // Retail 2A2400 selects obje+B4 and dispatches its eight-byte records in mode 2.
  const auto resources = metadata_block(base + 0xb4, 8);
  std::vector<PredictedResource> out;
  for (std::size_t i = 0; i < resources.count; ++i) {
    const auto at = resources.offset + i * 8;
    out.push_back({i16(data, at), i16(data, at + 2), data.u32(at + 4)});
  }
  return out;
}
std::vector<PredictedResource> Cache::model_bitmap_resources(std::uint32_t datum) const {
  const Bytes data(cache_bytes());
  auto root=[&](std::uint32_t id,const std::string &group,std::size_t minimum) {
    const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group)});
    if (!tag || tag->size<minimum)
      throw FormatError("missing/truncated predicted " + group);
    return metadata_offset(tag->address,tag->size);
  };
  const auto materials=TagAddressSpace(data, metadata_magic_).block(root(datum,"mode",0x68)+0x60, 0x20);
  std::set<std::uint32_t> bitmaps;
  for (std::size_t i=0;i<materials.count;++i) {
    const auto shader=data.u32(materials.offset+i*0x20+0xc);
    if (shader==null_datum) continue;
    const auto definitions=TagAddressSpace(data, metadata_magic_).block(root(shader,"shad",0x28)+0x20, 0x7c);
    if (!definitions.count) continue;
    const auto references=TagAddressSpace(data, metadata_magic_).block(definitions.offset+4, 0xc);
    for (std::size_t j=0;j<references.count;++j) {
      const auto bitmap=data.u32(references.offset+j*0xc);
      if (bitmap!=null_datum) bitmaps.insert(bitmap);
    }
  }
  std::vector<PredictedResource> out;
  for (const auto bitmap:bitmaps) {
    const auto count=metadata_block(root(bitmap,"bitm",0x4c)+0x44, mcc_?168:116).count;
    for (std::size_t i=0;i<count;++i) out.push_back({0,int(i),bitmap});
  }
  return out;
}
BitmapResource Cache::load_bitmap_resource(std::uint32_t datum, unsigned index) const {
  if(mcc_)return load_mcc_bitmap_resource(datum,index);
  if(vista_) {
    const auto [donor,original]=vista_texture_donor(datum);
    return donor->load_bitmap_resource(original,index);
  }
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::bitmap});
  if (!tag || tag->size<76)
    throw FormatError("missing/truncated predicted bitmap");
  const Bytes data(cache_bytes());
  const auto base=metadata_offset(tag->address,tag->size);
  const auto records=TagAddressSpace(data, metadata_magic_).block(base+0x44, 0x74);
  if (index>=records.count) throw FormatError("predicted bitmap index outside tag");
  const auto at=records.offset+index*0x74;
  BitmapResource out{data.u16(at+4),data.u16(at+6),data.u16(at+8),
                     data.u16(at+10),data.u16(at+12),{}};
  // Residency accepts the authored payload without requiring a display format.
  // The three cache LOD descriptors are offsets +1C/+20/+24 and sizes +34/+38/+3C.
  for (unsigned i=0;i<out.levels.size();++i) {
    const auto size=data.u32(at+0x34+i*4);
    if (size) out.levels[i]=raw_data(data.u32(at+0x1c+i*4),size);
  }
  return out;
}
std::vector<PredictedResource> Cache::structure_resources(std::size_t index,
                                                         int section,bool definition) const {
  if (index>=bsps_.size()) throw FormatError("prediction BSP outside scenario");
  const auto &bsp=bsps_[index];
  const Bytes data(cache_bytes()), meta(data.slice(bsp.file_offset,bsp.size));
  const auto magic=std::int64_t(bsp.memory_address);
  const auto entries=TagAddressSpace(meta, magic).block(16+(definition ? 0x138 : 0x9c), definition ? 0xc8 : 0xb0);
  if (section==-1) return {};
  if (section<0 || std::size_t(section)>=entries.count)
    throw FormatError("prediction structure section outside authored array");
  if (definition) return {}; // Direct geometry branch; no predicted-resource list.
  const auto resources=TagAddressSpace(meta, magic).block(entries.offset+std::size_t(section)*0xb0+0x84, 8);
  std::vector<PredictedResource> out;
  for (std::size_t i=0;i<resources.count;++i) {
    const auto p=resources.offset+i*8;
    out.push_back({i16(meta,p),i16(meta,p+2),meta.u32(p+4)});
  }
  return out;
}
LevelContent Cache::load_content(std::size_t index, bool multiplayer) const {
  return load_content(index,multiplayer,source_game_variant_default(0));
}
LevelContent Cache::load_content(std::size_t index, bool multiplayer, const SourceGameVariant &variant) const {
  if (index >= bsps_.size())
    throw FormatError("content BSP index outside scenario");
  LevelContent out;
  out.original_data = original_data();
  const Bytes data(cache_bytes());
  auto find = [&](std::uint32_t id,
                  const char *group = nullptr) -> const Tag * {
    const auto* tag = find_tag({id});
    return !tag || !tag->address || (group && tag->group != group) ? nullptr : tag;
  };
  auto root = [&](const Tag &t, std::size_t size) {
    if (t.size < size)
      throw FormatError("truncated " + t.group + " " + t.name);
    return metadata_offset(t.address, size);
  };
  auto geometry = load_bsp_content(index);
  out.collision = std::move(geometry.collision);
  out.navigation = std::move(geometry.navigation);
  out.bsp_count = bsps_.size();
  auto biped = [&](std::uint32_t id) {
    BipedDefinition d;
    const auto *t = find(id, "bipd");
    if (!t)
      return d;
    const auto p = root(*t, 0x2cc);
    d.datum = id;
    d.object_flags=data.u16(p+2);
    d.unit_flags=data.u32(p+0xbc);
    d.model = data.u32(p + 0x38);
    d.camera_field_of_view = real(data, p + 0xcc);
    if (d.camera_field_of_view < 0 || d.camera_field_of_view >= 3.141592654f)
      throw FormatError("invalid player camera field of view");
    d.bounding_offset = point(data, p + 8);
    d.bounding_radius = real(data,p+4);
    d.height = real(data, p + 0x268);
    d.crouch_height = real(data, p + 0x26c);
    d.radius = real(data, p + 0x270);
    d.camera_height = real(data, p + 0x218);
    d.crouch_camera_height = real(data, p + 0x21c);
    d.crouch_velocity = real(data, p + 0x250);
    d.jump_velocity = real(data, p + 0x1f8);
    d.landing={real(data,p+0x1fc),real(data,p+0x200),real(data,p+0x204),
        real(data,p+0x208),real(data,p+0x20c)};
    d.physics_flags = data.u32(p + 0x264);
    d.maximum_slope_cosine = real(data, p + 0x2b8);
    d.slope = {real(data, p + 0x2b0), real(data, p + 0x2b4),
               real(data, p + 0x2bc), real(data, p + 0x2c0),
               real(data, p + 0x2c4), real(data, p + 0x2c8)};
    d.team = i16(data, p + 0xc0);
    if (const auto *model = find(data.u32(p + 0x38), "hlmt")) {
      const auto info =
          TagAddressSpace(data, metadata_magic_).block(root(*model, 0x68) + 0x60, 0xf8);
      if (info.count) {
        const auto q = info.offset;
        d.vitality = {real(data, q + 0x28), real(data, q + 0x8c),
                      real(data, q + 0x98), real(data, q + 0xd0),
                      real(data, q + 0xd4)};
        d.vitality.body_delay = real(data, q + 0x30);
        d.vitality.body_max = real(data, q + 0x38);
        d.vitality.minimum_shield_stun_damage = real(data, q + 0x94);
        d.vitality.shield_damaged_threshold = real(data, q + 0xa0);
        d.vitality.shield_material = i16(data, q + 0xcc);
        d.vitality.minimum_body_stun_damage = real(data, q + 0x2c);
        d.vitality.body_material = i16(data, q + 0xce);
        d.vitality.default_section = i16(data, q + 8);
        d.vitality.damage_flags = data.u32(q);
        d.vitality.shield_damaged_first_person_shader = data.u32(q + 0x80);
        d.vitality.shield_damaged_shader = data.u32(q + 0x88);
        d.vitality.shield_damaged_effect = data.u32(q + 0xa8);
        d.vitality.shield_depleted_effect = data.u32(q + 0xb0);
        d.vitality.shield_recharging_effect = data.u32(q + 0xb8);
        d.vitality.damage_info_count = static_cast<std::int32_t>(info.count);
        d.vitality.overshield_first_person_shader = data.u32(q + 0xec);
        d.vitality.overshield_shader = data.u32(q + 0xf4);
        const auto sections = TagAddressSpace(data, metadata_magic_).block(q + 0xbc, 0x38);
        for (std::size_t i = 0; i < sections.count; ++i) {
          const auto r = sections.offset + i * 0x38;
          d.vitality.sections.push_back({data.u32(r), data.u32(r + 4),
                                         real(data, r + 8)});
        }
      }
    }
    return d;
  };
  std::function<std::uint32_t(std::uint32_t, unsigned)> effect_sound =
      [&](std::uint32_t id, unsigned depth) -> std::uint32_t {
    const auto *t = find(id);
    if (!t || depth > 8)
      return null_datum;
    if (t->group == "snd!")
      return id;
    if (t->group != "effe")
      return null_datum;
    const auto events =
        TagAddressSpace(data, metadata_magic_).block(root(*t, 0x1c) + 0x14, 0x38);
    for (std::size_t e = 0; e < events.count; ++e) {
      const auto parts =
          TagAddressSpace(data, metadata_magic_).block(events.offset + e * 0x38 + 0x18, 0x38);
      for (std::size_t n = 0; n < parts.count; ++n) {
        const auto sound =
            effect_sound(data.u32(parts.offset + n * 0x38 + 0x10), depth + 1);
        if (sound != null_datum)
          return sound;
      }
    }
    return null_datum;
  };
  for (const auto &tag : select_tags({.group=tag_groups::weapon, .require_address=true})) {
      try {
        const auto p = root(tag, 0x31c);
        WeaponDefinition w;
        w.datum = tag.datum;
        w.name = tag.name;
        w.model = data.u32(p + 0x38);
        w.bounding_offset=point(data,p+8);w.weapon_type=std::int16_t(data.u16(p+0x292));
        w.pickup_priority=std::int16_t(data.u16(p+0x290));
        w.inventory_flags = data.u32(p + 0x12c);
        w.dual_wieldable = (data.u32(p + 0x12c) & (1u << 22)) != 0;
        w.first_person_offset = point(data, p + 0x308);
        w.first_person_scope_size = {data.f32(p + 0x314), data.f32(p + 0x318)};
        w.zoom = {data.u32(p + 0x12c), std::bit_cast<std::int16_t>(data.u16(p + 0x1fe)),
                  data.f32(p + 0x200), data.f32(p + 0x204)};
        w.ready_seconds=real(data,p+0x138);w.ready_effect=data.u32(p+0x144);
        w.firing_camouflage_amount=real(data,p+0x27c);
        w.firing_camouflage_limit=real(data,p+0x280);
        const auto fp = TagAddressSpace(data, metadata_magic_).block(p + 0x2a8, 16);
        for(std::size_t i=0;i<fp.count;++i)
          w.first_person_models.push_back({data.u32(fp.offset+i*16+4),data.u32(fp.offset+i*16+12)});
        if (fp.count) {
          w.first_person_model = data.u32(fp.offset + 4);
          w.first_person_animation = data.u32(fp.offset + 12);
        }
        const auto native_triggers = TagAddressSpace(data, metadata_magic_).block(p + 0x2c8, 0x40);
        for(std::size_t index=0;index<native_triggers.count;++index) {
          const auto t=native_triggers.offset+index*0x40;
          w.triggers.push_back({data.u32(t),std::int16_t(i16(data,t+4)),std::int16_t(i16(data,t+6)),
              std::int16_t(i16(data,t+8)),std::int16_t(i16(data,t+0xa)),real(data,t+0x10),real(data,t+0x14),
              std::int16_t(i16(data,t+0x18)),std::int16_t(i16(data,t+0x1a)),real(data,t+0x1c),real(data,t+0x20),
              std::int16_t(i16(data,t+0x24)),real(data,t+0x2c),data.u32(t+0x34),data.u32(t+0x3c)});
        }
        const auto mags = TagAddressSpace(data, metadata_magic_).block(p + 0x2c0, 0x5c),
                   barrels = TagAddressSpace(data, metadata_magic_).block(p + 0x2d0, 0xec);
        if (!barrels.count) {
          // Authored melee-only weapons have no projectile barrel. Keep their
          // real FP graph and melee damage instead of dropping their definition.
          if(!(data.u32(p+0x12c)&(1u<<24)) || w.first_person_animation==null_datum)continue;
          w.damage=data.u32(p+0x190);
          if(w.damage==null_datum)w.damage=data.u32(p+0x1c0);
          w.range=real(data,p+0x1b8);w.instantaneous=true;w.burst=w.projectiles_per_shot=1;
          const auto strike=load_action(w.first_person_animation,"any","any","any","melee_strike_1");
          w.rounds_per_second=30.f/float(std::max(1u,strike.frame_count));
          if(const auto *damage=find(w.damage,"jpt!")) {
            const auto d=root(*damage,0x58);
            w.damage_min=real(data,d+0x1c);w.damage_max=real(data,d+0x24);
            w.damage_amount=DamageAmountDefinition{real(data,d+0x1c),real(data,d+0x20),real(data,d+0x24)};
            w.damage_flags=data.u32(d+0x14);w.damage_group=data.u32(d+0x50);w.damage_group_b=data.u32(d+0x54);
            w.damage_side_effect=i16(data,d+0x10);w.damage_category=i16(data,d+0x12);
            w.damage_camouflage_amount=real(data,d+0x30);
          }
          out.weapons.emplace(w.datum,std::move(w));continue;
        }
        //1021D0's ordinary input selects trigger+8, not necessarily barrel0.
        //The carbine's literal primary/secondary indices are both1.
        std::int16_t primary_barrel=0;
        if(native_triggers.count==1) {
          w.trigger_behavior=i16(data,native_triggers.offset+6);
          primary_barrel=i16(data,native_triggers.offset+8);
          // Charge/autofire NPC weapons may author only a secondary barrel.
          // Retain that actual data reference; their trigger manager remains
          // separate from the ordinary firing path admitted below.
          if(primary_barrel==-1)primary_barrel=i16(data,native_triggers.offset+0xa);
          if(primary_barrel<0 || std::size_t(primary_barrel)>=barrels.count)
            throw FormatError("Weapon primary trigger has no authored barrel");
        }
        const auto b = barrels.offset+std::size_t(primary_barrel)*0xec;
        for(std::size_t index=0;index<barrels.count;++index) {
          const auto at=barrels.offset+index*0xec;
          w.barrel_ammunition.push_back({data.u32(at),std::int16_t(i16(data,at+0x28)),
              std::int16_t(i16(data,at+0x2a)),std::int16_t(i16(data,at+0x2c)),
              {data.u32(at),real(data,at+4),real(data,at+8),real(data,at+0x20),real(data,at+0x24),
                real(data,p+0x258),std::int16_t(i16(data,at+0x1c)),std::int16_t(i16(data,at+0x1e))},
              {real(data,at+0xdc),real(data,at+0xe0),real(data,at+0x50),real(data,at+0x54),
                real(data,at+0xc4),real(data,at+0xc8),real(data,at+0x70),real(data,at+0x74),real(data,at+0x78),
                real(data,at+0x58),real(data,at+0x5c),real(data,at+0x60)},
              real(data,at+0xd4),real(data,at+0xd8),real(data,at+0xcc)});
        }
        const auto mag = i16(data, b + 0x28);
        if (mag >= 0 && std::size_t(mag) < mags.count) {
          const auto m = mags.offset + std::size_t(mag) * 0x5c;
          w.capacity = i16(data, m + 0xa);
          w.initial_rounds = i16(data, m + 6);
          w.reserve = i16(data, m + 0xc);
          w.reload_time = real(data, m + 0x10);
          w.reload_rounds = i16(data, m + 0x14);
          w.wastes_rounds = (data.u32(m) & 1) != 0;
          if(multiplayer && mags.count==1)w.magazine=WeaponMagazineDefinition{
            data.u32(m),std::int16_t(w.capacity),std::int16_t(w.reload_rounds),real(data,m+0x18),
            std::int16_t(w.reserve),std::int16_t(i16(data,m+4)),data.u32(m+0x38),data.u32(m+0x48)};
        }
        w.rounds_per_second = real(data, b + 4);
        if(multiplayer && native_triggers.count==1 && out.original_data &&
            out.original_data->image!=OriginalData::Image::Base &&
            out.original_data->unit_animation_constants &&
            (real(data,p+0x25c)==0 || real(data,p+0x154)>0) &&
            real(data,p+0x15c)==0 && (w.trigger_behavior==0 || w.trigger_behavior==1 || w.trigger_behavior==5 ||
              (w.trigger_behavior==4 && i16(data,native_triggers.offset+0xa)==primary_barrel))) {
          const auto &constants=*out.original_data->unit_animation_constants;
          w.barrel_timing=WeaponDefinition::BarrelTiming{
            {data.u32(b),real(data,b+4),real(data,b+8),real(data,b+0x20),real(data,b+0x24),
              real(data,p+0x258),std::int16_t(i16(data,b+0x1c)),std::int16_t(i16(data,b+0x1e))},
            {std::bit_cast<float>(constants.one_bits),std::bit_cast<float>(constants.frame_subtractor_bits)},
            real(data,b+0xd4),real(data,b+0xd8),primary_barrel,real(data,b+0xcc)};
          if(real(data,p+0x154)>0)w.energy=WeaponDefinition::Energy{
            {real(data,p+0x150),real(data,p+0x154),real(data,p+0x160),real(data,p+0x168),
              real(data,p+0x254),std::int16_t(i16(data,p+0x292)),data.u32(p+0x170)},
            {data.u32(p+0x2fc),data.u32(p+0x304)},real(data,b+0xa4),real(data,b+0xa8),
            std::bit_cast<float>(constants.one_bits),std::bit_cast<float>(constants.rng_scale_bits),
            real(data,p+0x25c),real(data,p+0x260),real(data,p+0x158),real(data,p+0x15c)};
        }
        w.rounds_per_shot = i16(data, b + 0x2a);
        w.minimum_rounds = i16(data, b + 0x2c);
        w.burst = i16(data, b + 0x1c);
        w.fire_recovery = real(data, b + 0x20);
        w.projectiles_per_shot = i16(data, b + 0x6a);
        w.distribution = i16(data, b + 0x68);
        w.distribution_angle = real(data, b + 0x6c);
        w.minimum_error = real(data, b + 0x70);
        // Original 1.1 102090 / 10365B and 108F81..10905C consume these
        // authored barrel fields. Values remain local asset data.
        w.firing_error = {
            real(data, b + 0xdc), real(data, b + 0xe0),
            real(data, b + 0x50), real(data, b + 0x54),
            real(data, b + 0xc4), real(data, b + 0xc8),
            real(data, b + 0x70), real(data, b + 0x74), real(data, b + 0x78),
            real(data, b + 0x58), real(data, b + 0x5c), real(data, b + 0x60)};
        w.dual_damage_scale = real(data, b + 0x64);
        w.projectile = data.u32(b + 0x90);
        w.random_firing_effects=(data.u32(b)&2)!=0;
        const auto effects = TagAddressSpace(data, metadata_magic_).block(b + 0xe4, 0x34);
        for(std::size_t index=0;index<effects.count;++index) {
          const auto effect=effects.offset+index*0x34;
          WeaponFiringEffectDefinition entry;
          entry.minimum=i16(data,effect);entry.maximum=i16(data,effect+2);
          for(unsigned selection=0;selection<3;++selection) {
            entry.effects[selection]=data.u32(effect+8+selection*8);
            entry.damage_effects[selection]=data.u32(effect+0x20+selection*8);
          }
          w.firing_effects.push_back(entry);
        }
        if (effects.count)
          w.sound = effect_sound(data.u32(effects.offset + 8), 0);
        if(multiplayer && w.weapon_type==2) {
          //Needler normal107160 selection: one nonrandom entry, no heat,
          //misfire threshold or alternate-marker dispatch. Actual tag data
          //owns the effect; the weapon name does not select it.
          if(barrels.count!=1 || data.u32(b)!=1 || effects.count!=1 ||
              i16(data,effects.offset)!=1 || i16(data,effects.offset+2)!=1 ||
              real(data,p+0x154)!=0 || real(data,p+0x25c)!=0)
            throw FormatError("Needler firing presentation requires additional original barrel owners");
          w.firing_presentation=WeaponDefinition::FiringPresentation{real(data,b+0xd4),real(data,b+0xd8)};
        }
        if (multiplayer && w.name == "objects\\weapons\\rifle\\smg\\smg") {
          const auto triggers = TagAddressSpace(data, metadata_magic_).block(p + 0x2c8, 0x40);
          const auto m = mags.offset;
          // Admit only the recovered single-SMG trigger/barrel/magazine path.
          // Unknown variants must not silently enter a generalized substitute.
          if (triggers.count != 1 || barrels.count != 1 || mags.count != 1 ||
              data.u32(triggers.offset) || i16(data, triggers.offset + 4) != 0 ||
              i16(data, triggers.offset + 6) != 0 ||
              i16(data, triggers.offset + 8) != 0 ||
              i16(data, triggers.offset + 0xa) != -1 || data.u32(b) || mag != 0 ||
              real(data, b + 4) != real(data, b + 8) ||
              real(data, b + 4) <= 0 || real(data, b + 4) > 30 ||
              real(data, b + 0x18) != 0 || i16(data, b + 0x1c) ||
              i16(data, b + 0x1e) || real(data, b + 0x20) != 0 ||
              real(data, b + 0x24) != 0 || w.rounds_per_shot != 1 ||
              w.minimum_rounds != 0 || i16(data, b + 0x34) != 1 ||
              w.projectiles_per_shot != 1 || w.distribution != 0 ||
              effects.count != 1 || i16(data, effects.offset) != 1 ||
              i16(data, effects.offset + 2) != 1 || real(data, b + 0x9c) != 0 ||
              data.u32(m) || i16(data, m + 4) || real(data, m + 0x18) != 0 ||
              data.u32(p + 0x12c) != 0x00420000 || i16(data, p + 0x292) != 0 ||
              real(data,p+0x154)!=0 || real(data,p+0x25c)!=0)
            throw FormatError("SMG asset uses an unported trigger or magazine path");
          const auto full = load_action(w.first_person_animation, "any", "any", "any", "reload_full");
          const auto empty = load_action(w.first_person_animation, "any", "any", "any", "reload_empty");
          const auto ready = load_action(w.first_person_animation, "any", "any", "any", "ready");
          const auto *own_model = find(w.model, "hlmt");
          if (!own_model) throw FormatError("Missing original SMG own model");
          const auto own_graph = data.u32(root(*own_model, 0x18) + 0x14);
          const auto own = load_named_animation(own_graph, animation_name(own_graph, "any:idle"));
          //1042E0 can only write own-state10 after a successful1D0A60.
          //The admitted stock SMG has no authored put-away action in either
          //class. Reject other assets until that own manager is represented.
          for(const auto weapon_class:{0x07000001u,0x0400054bu}) {
            bool present=false;
            try {(void)load_unit_weapon_action(own_graph,0x07000101u,weapon_class,0x07000001u,0x08000025u);present=true;}
            catch(const FormatError &e) {
              if(std::string(e.what())!="authored unit weapon action missing: "+string_id(0x08000025u))throw;
            }
            if(present)throw FormatError("SMG own put-away action requires its original manager");
          }
          if (own.frame_count != 1)
            throw FormatError("Original single-SMG own clock uses an unported animation");
          if (full.frame_event_count || empty.frame_event_count ||
              full.frame_count > 32767 || empty.frame_count > 32767)
            throw FormatError("SMG reload frame-event timing remains unported");
          w.smg = SmgDefinition{w.firing_error, w.rounds_per_second,
              real(data, b + 0xd4), real(data, b + 0xd8), real(data, b + 0xcc),
              real(data, b + 0xa0) > 0, std::int16_t(i16(data, b + 0x2e)),
              std::int16_t(w.capacity), std::int16_t(w.reload_rounds),
              std::int16_t(full.frame_count), std::int16_t(empty.frame_count),
              animation_ready_ticks(ready, 30), std::int16_t(own.frame_count)};
        }
        if(w.barrel_timing && !w.smg && effects.count>0 && effects.count<=16 &&
            !w.random_firing_effects && !(data.u32(b)&(1u<<12)) &&
            std::all_of(w.firing_effects.begin(),w.firing_effects.end(),[](const auto &effect) {
              return effect.minimum>0 && effect.minimum==effect.maximum;
            })) {
          // Same original107160 normal request contract, on the actual
          // selected barrel. Visual/part admission belongs to its consumer.
          w.firing_presentation=WeaponDefinition::FiringPresentation{real(data,b+0xd4),real(data,b+0xd8)};
        }
        if (const auto *proj = find(w.projectile, "proj")) {
          const auto q = root(*proj, (w.smg || (multiplayer && w.weapon_type==2)) ? 0x18c : 0x184);
          if (w.smg && (data.u32(q + 0xbc) ||
              real(data, q + 0xd4) != real(data, q + 0xd8) ||
              real(data, q + 0x17c) != real(data, q + 0x180) ||
              real(data, q + 0x184) != real(data, q + 0x188)))
            throw FormatError("SMG projectile uses an unported constructor or speed override");
          w.projectile_speed = real(data, q + 0x17c);
          w.projectile_gravity = real(data, q + 0x164);
          const auto responses=TagAddressSpace(data,metadata_magic_).block(q+0x19c,0x58);
          const auto response_bytes=data.slice(responses.offset,responses.count*0x58);
          w.projectile_material_responses.assign(response_bytes.begin(),response_bytes.end());
          w.projectile_flags=data.u32(q+0xbc);
          w.projectile_dry_damage_range=real(data,q+0x16c);
          if(multiplayer && w.weapon_type==2 && out.original_data &&
              out.original_data->unit_animation_constants) {
            // Closed FC2E0 dry free-Needler branch, authenticated against
            // TU1.5 FD200. Parent/platform/medium and guidance acquisition
            // remain separate live producers, not inferred from tag names.
            if(data.u32(q+0xbc)!=9 || i16(data,q+0xc0)!=2 ||
                real(data,q+0x164)!=0 || real(data,q+0x17c)!=real(data,q+0x180))
              throw FormatError("Needler projectile requires additional original constructor owners");
            const auto &constants=*out.original_data->unit_animation_constants;
            w.needler_projectile=SourceNeedlerProjectileDefinition{
                real(data,q+0x17c),real(data,q+0x184),real(data,q+0x188),
                real(data,q+0xd4),real(data,q+0xd8),real(data,q+0xcc),real(data,q+0x16c),
                std::bit_cast<float>(constants.one_bits),std::bit_cast<float>(constants.rng_scale_bits)};
          }
          w.damage = data.u32(q + 0x14c);
          w.instantaneous = (data.u32(q + 0xbc) & 32) != 0;
          w.range = real(data, q + 0xe0);
          if (const auto *damage = find(w.damage, "jpt!")) {
            const auto d = root(*damage, 0x58);
            w.damage_min = real(data, d + 0x1c);
            w.damage_max = real(data, d + 0x24);
            w.damage_amount = DamageAmountDefinition{real(data, d + 0x1c),
                real(data, d + 0x20), real(data, d + 0x24)};
            w.damage_flags = data.u32(d + 0x14);
            w.damage_group = data.u32(d + 0x50);
            w.damage_group_b = data.u32(d + 0x54);
            w.damage_side_effect = i16(data, d + 0x10);
            w.damage_category = i16(data, d + 0x12);
            w.damage_camouflage_amount=real(data,d+0x30);
          }
        }
        if(w.barrel_timing)for(unsigned representation=0;representation<2;++representation) {
          const auto binding=load_first_person_binding(w.datum,int(representation+2));
          for(unsigned dual=0;dual<2;++dual) {
            FirstPersonHandBinding hand;hand.dual_wield=dual;
            for(const auto &clip:first_person_clips(hand,binding))if(clip.activity==0x08000025u) {
              auto &timing=w.put_away_timing[representation][dual];timing.present=true;timing.frames=clip.frames;
              for(const auto &event:clip.events)if(event.type==0) {
                if(event.id==4 && !timing.event4)timing.event4=event.frame;
                if(event.id==0 && !timing.event0)timing.event0=event.frame;
              }
            }
          }
        }
        out.weapons.emplace(w.datum, std::move(w));
      } catch (const FormatError &e) {
        out.limitations.push_back(tag.name + ": " + e.what());
      }
    }
  std::set<std::uint32_t> visiting;
  std::function<CharacterDefinition(std::uint32_t)> character =
      [&](std::uint32_t id) {
        if (const auto it = out.characters.find(id); it != out.characters.end())
          return it->second;
        CharacterDefinition c;
        const auto *t = find(id, "char");
        if (!t)
          return c;
        if (!visiting.insert(id).second || visiting.size() > 32)
          throw FormatError("cyclic/deep character inheritance");
        const auto p = root(*t, 0x54);
        const auto parent = data.u32(p + 8);
        if (parent != null_datum)
          c = character(parent);
        c.datum = id;
        if (data.u32(p + 0x10) != null_datum)
          c.biped = biped(data.u32(p + 0x10));
        const auto general = TagAddressSpace(data, metadata_magic_).block(p + 0x34, 0xc);
        if (general.count)
          c.actor_type = i16(data, general.offset + 4);
        const auto vitality = TagAddressSpace(data, metadata_magic_).block(p + 0x3c, 0x70),
                   perception = TagAddressSpace(data, metadata_magic_).block(p + 0x4c, 0x34);
        if (vitality.count) {
          const auto v = vitality.offset;
          c.biped.vitality.health = real(data, v + 4);
          c.biped.vitality.shield = real(data, v + 8);
          c.biped.vitality.shield_delay = real(data, v + 0x48);
          c.biped.vitality.body_rate = real(data, v + 0x68);
          c.biped.vitality.shield_rate = real(data, v + 0x6c);
          c.biped.vitality.body_delay = real(data, v + 0x40);
          c.biped.vitality.body_max = real(data, v + 0x14);
          c.damage_fade = DamageFadeDefinition{real(data,v+0x30),real(data,v+0x34),
                                               real(data,v+0x38),real(data,v+0x3c)};
        }
        if (perception.count) {
          const auto v = perception.offset;
          c.vision_distance = real(data, v + 4);
          c.vision_angle = real(data, v + 0xc);
          c.acknowledgement = real(data, v + 0x24);
        }
        visiting.erase(id);
        out.characters[id] = c;
        return c;
      };
  for (const auto &t : select_tags({.group=tag_groups::globals, .require_address=true})) {
      const auto p = root(t, 0x158);
      const auto tables = TagAddressSpace(data, metadata_magic_).block(p + 0xd0, 8, 1),
                 materials = TagAddressSpace(data, metadata_magic_).block(p + 0x150, 0xb4);
      if (tables.count) {
        const auto groups = TagAddressSpace(data, metadata_magic_).block(tables.offset, 12);
        for (std::size_t i = 0; i < groups.count; ++i) {
          const auto g = groups.offset + i * 12;
          DamageGroup group{data.u32(g), {}};
          const auto modifiers = TagAddressSpace(data, metadata_magic_).block(g + 4, 8);
          for (std::size_t j = 0; j < modifiers.count; ++j) {
            const auto m = modifiers.offset + j * 8;
            group.modifiers.push_back({data.u32(m), real(data, m + 4)});
          }
          out.damage_groups.push_back(std::move(group));
        }
      }
      for (std::size_t i = 0; i < materials.count; ++i) {
        const auto m = materials.offset + i * 0xb4;
        out.damage_materials.push_back({data.u32(m + 0x10), data.u32(m + 0x14)});
        out.material_parents.push_back(std::int16_t(data.u16(m+8)));
      }
      const auto controls = TagAddressSpace(data, metadata_magic_).block(p + 0xf0, 0x98);
      if (controls.count) {
        out.player_screen_offset = {real(data, controls.offset + 0x18),
                                    real(data, controls.offset + 0x1c)};
        out.pickup_hold_seconds=data.f32(controls.offset+0x7c);
        out.auto_level_scale=real(data,controls.offset+0x5c);
        out.auto_level_minimum_ticks=std::int16_t(data.u16(controls.offset+0x6e));
      }
      const auto players = TagAddressSpace(data, metadata_magic_).block(p + 0x130, 0x11c),
                 representations =
                     TagAddressSpace(data, metadata_magic_).block(p + 0x138, 0xbc);
      out.player_representation_count=representations.count;
      if(multiplayer) {
        out.player_representations.resize(representations.count);
        for(std::size_t i=2;i<representations.count && i<4;++i) {
          auto &representation=out.player_representations[i];
          representation.biped=biped(data.u32(representations.offset+i*0xbc+0xb4));
          representation.hands=data.u32(representations.offset+i*0xbc+4);
        }
      }
      if (multiplayer) {
        // C9CC0/C9BE0 use the first player-information pointer without a
        // block-count guard. Preserve that source walk with host bounds.
        const auto q = metadata_offset(data.u32(p + 0x134), 0xc4);
        out.binocular_zoom = BinocularZoomDefinition{
            std::bit_cast<std::int32_t>(data.u32(q + 0xb8)),
            data.f32(q + 0xbc), data.f32(q + 0xc0)};
      }
      if (players.count) {
        const auto q = players.offset;
        out.movement = {
            real(data, q + 0x2c), real(data, q + 0x30), real(data, q + 0x34),
            real(data, q + 0x38), real(data, q + 0x3c), real(data, q + 0x40),
            real(data, q + 0x44), real(data, q + 0x48), real(data, q + 0x4c)};
        out.movement.limited_speed = real(data, q + 0x24);
      }
      for (std::size_t i = 0; i < representations.count; ++i) {
        if (multiplayer && i != 2) continue; // Retail 14F040: profile Spartan 0 -> representation 2.
        auto candidate =
            biped(data.u32(representations.offset + i * 0xbc + 0xb4));
        if (candidate.datum != null_datum) {
          out.player = candidate;
          out.first_person_hands =
              data.u32(representations.offset + i * 0xbc + 4);
          break;
        }
      }
      break;
    }
  const auto scenario =
      query_tag({.group=tag_groups::scenario, .require_address=true});
  if (!scenario)
    return out;
  const auto s = root(*scenario, multiplayer ? 0x320 : 0x240);
  const auto profiles = TagAddressSpace(data, metadata_magic_).block(s + 0xf8, 0x44);
  if (profiles.count)
    out.starting_weapon = data.u32(profiles.offset + 0x2c);
  const auto squads = TagAddressSpace(data, metadata_magic_).block(s + 0x160, 0x74),
             chars = TagAddressSpace(data, metadata_magic_).block(s + 0x178, 8),
             weapons = TagAddressSpace(data, metadata_magic_).block(s + 0x98, 0x28),
             zones = TagAddressSpace(data, metadata_magic_).block(s + 0x168, 0x38);
  auto palette = [&](TagBlockRange palette, std::size_t stride, int i) {
    return i < 0 || std::size_t(i) >= palette.count
               ? null_datum
               : data.u32(palette.offset + std::size_t(i) * stride + 4);
  };
  const auto names = TagAddressSpace(data, metadata_magic_).block(s + 0x48, 0x24, 640);
  for (std::size_t i = 0; i < names.count; ++i) {
    const auto p = names.offset + i * 0x24;
    out.object_names.push_back(
        {data.string(p, 32), i16(data, p + 0x20), i16(data, p + 0x22)});
  }
  struct PlacementBlock {
    int type;
    unsigned offset, stride;
  };
  constexpr PlacementBlock placements[]{
      {6, 0x50, 0x5c},  {0, 0x60, 0x54},  {1, 0x70, 0x54}, {3, 0x80, 0x38},
      {2, 0x90, 0x54},  {7, 0xa8, 0x48},  {8, 0xb8, 0x44}, {9, 0xc8, 0x54},
      {10, 0xd8, 0x50}, {11, 0x328, 0x4c}};
  for (const auto &kind : placements) {
    if (scenario->size < kind.offset + 16)
      continue;
    const auto placed =
        TagAddressSpace(data, metadata_magic_).block(s + kind.offset, kind.stride, 2048);
    const auto choices =
        TagAddressSpace(data, metadata_magic_).block(s + kind.offset + 8, 0x28);
    for (std::size_t i = 0; i < placed.count; ++i) {
      const auto p = placed.offset + i * kind.stride;
      ScenarioObject o;
      o.type = kind.type;
      o.placement = int(i);
      o.name = i16(data, p + 2);
      o.definition = palette(choices, 0x28, i16(data, p));
      o.flags = data.u32(p + 4);
      o.position = point(data, p + 8);
      o.rotation = point(data, p + 0x14);
      o.scale = real(data, p + 0x20);
      if (o.scale <= 0)
        o.scale = 1;
      o.manual_bsps = data.u16(p + 0x26);
      o.unique_id = data.u32(p + 0x28);
      o.origin_bsp = i16(data, p + 0x2c);
      o.source = std::int8_t(byte(data,p + 0x2f));
      o.bsp_policy = byte(data, p + 0x30);
      if(kind.type==6 || kind.type==7)for(const auto r:metadata_block_offsets(p+(kind.type==6?0x50:0x40),4))
        o.pathfinding_references.emplace_back(std::int16_t(i16(data,r)),std::int16_t(i16(data,r+2)));
      if(kind.type==6) {
        o.lightmapping_policy=i16(data,p+0x4e);
        o.valid_multiplayer_games=data.u16(p+0x5a);
      }
      if (kind.type == 0 || kind.type == 1 || kind.type == 2 ||
          kind.type == 6 || kind.type == 11)
        o.variant = data.u32(p + 0x34);
      if(kind.type==2)o.weapon=ScenarioObject::WeaponDatum{
          std::int16_t(i16(data,p+0x4c)),std::int16_t(i16(data,p+0x4e)),data.u32(p+0x50)};
      o.damageable = kind.type < 13 && ((1u << kind.type) & 0x199f);
      if (const auto *definition = find(o.definition)) {
        o.bounding_offset = point(data, root(*definition, 0x14) + 8);
        o.bounding_radius = real(data, root(*definition, 0x14) + 4);
        if (kind.type == 6) {
          const auto model_id = data.u32(root(*definition,0x3c) + 0x38);
          if (const auto *model = find(model_id,"hlmt"))
            o.damageable = data.u32(root(*model,0x68) + 0x60) > 0;
        }
      }
      if (o.name < -1 || (o.name >= 0 && std::size_t(o.name) >= names.count) ||
          o.bsp_policy > 2)
        throw FormatError("invalid scenario object placement");
      out.objects.push_back(o);
    }
  }
  out.command_frames.resize(bsps_.size());
  for(unsigned b=0;b<bsps_.size();++b) {
    const auto &bsp=bsps_[b];const Bytes meta(data.slice(bsp.file_offset,bsp.size));const auto magic=std::int64_t(bsp.memory_address);
    const auto navigation=TagAddressSpace(meta,magic).block(16+0xc4,0x74);
    if(!navigation.count)continue;
    const auto entries=TagAddressSpace(meta,magic).block(navigation.offset+0x30,0x1c);
    for(std::size_t placement=0;placement<out.objects.size();++placement)for(const auto [source_bsp,entry]:out.objects[placement].pathfinding_references) {
      // Cairo retains one reference per BSP, using NONE where this object
      // has no pathfinding entry (for example scenery57, BSPs1..5).
      if(source_bsp!=int(b) || entry==-1)continue;
      if(entry<0 || std::size_t(entry)>=entries.count)throw FormatError("Object pathfinding reference outside BSP");
      const auto p=entries.offset+std::size_t(entry)*0x1c;if(!(meta.u32(p)&1))continue;
      const auto nodes=TagAddressSpace(meta,magic).block(p+0x14,4);
      for(unsigned i=0;i<nodes.count;++i) {const auto n=nodes.offset+i*4;
        out.command_frames[b].push_back({std::int16_t(meta.u16(n)),placement,i,std::int8_t(byte(meta,n+2)),std::uint8_t(byte(meta,n+3))});
      }
    }
  }
  const auto flags = TagAddressSpace(data, metadata_magic_).block(s + 0x1e0, 0x38),
             cameras = TagAddressSpace(data, metadata_magic_).block(s + 0x1e8, 0x40),
             volumes = TagAddressSpace(data, metadata_magic_).block(s + 0x108, 0x44);
  for (std::size_t i = 0; i < flags.count; ++i) {
    const auto p = flags.offset + i * 0x38;
    out.cutscene_flags.push_back({data.string(p + 4, 32), point(data, p + 0x24),
                                  real(data, p + 0x30), real(data, p + 0x34)});
  }
  for (std::size_t i = 0; i < cameras.count; ++i) {
    const auto p = cameras.offset + i * 0x40;
    out.camera_points.push_back({data.string(p + 4, 32), point(data, p + 0x28),
                                 point(data, p + 0x34), data.u16(p),
                                 i16(data, p + 2)});
  }
  for (std::size_t i = 0; i < volumes.count; ++i) {
    const auto p = volumes.offset + i * 0x44;
    out.trigger_volumes.push_back({string_id(data.u32(p)), i16(data, p + 4),
                                   data.u32(p + 8), point(data, p + 0xc),
                                   point(data, p + 0x18), point(data, p + 0x24),
                                   point(data, p + 0x30),real(data,p+0x3c)});
  }
  if(multiplayer) {
    const auto netgame_flags=TagAddressSpace(data, metadata_magic_).block(s+0x118, 0x20);
    for(std::size_t i=0;i<netgame_flags.count;++i) {
      const auto p=netgame_flags.offset+i*0x20;
      NetgameFlag flag;flag.source_index=std::uint32_t(i);
      flag.position=point(data,p);flag.facing=real(data,p+0x0c);
      flag.type=std::int16_t(i16(data,p+0x10));
      flag.selector12=std::int16_t(i16(data,p+0x12));
      flag.channel=std::int16_t(i16(data,p+0x14));
      std::copy_n(data.slice(p,0x20).begin(),0x20,flag.source_bytes.begin());
      out.netgame_flags.push_back(flag);
    }
    const auto globals = query_tag({.group=tag_groups::globals});
    if (!globals) throw FormatError("Missing multiplayer spawn globals");
    const auto *mulg = find(data.u32(root(*globals, 0x170) + 0x16c), "mulg");
    if (!mulg) throw FormatError("Missing multiplayer runtime spawn globals");
    const auto runtime = TagAddressSpace(data, metadata_magic_).block(root(*mulg, 0x10) + 8, 0x568, 1);
    if (runtime.count != 1) throw FormatError("Unsupported multiplayer runtime spawn block");
    SpawnSettings settings;
    settings.upper = real(data, runtime.offset + 0x150);
    settings.lower = real(data, runtime.offset + 0x154);
    for (unsigned i = 0; i < settings.types.size(); ++i) {
      const auto at = runtime.offset + 0x180 + i * 0x1c;
      settings.types[i] = {real(data, at), real(data, at + 4), real(data, at + 8)};
    }
    const auto constants = TagAddressSpace(data, metadata_magic_).block(runtime.offset + 0x530, 0x160, 1);
    if (constants.count != 1) throw FormatError("Missing multiplayer spawn constants");
    out.teleporter_cooldown_seconds=real(data,constants.offset+4);
    settings.random_bias = real(data, constants.offset);
    const auto spawn_data = TagAddressSpace(data, metadata_magic_).block(s + 0x318, 0x60);
    float scenario_lower = 0, scenario_upper = 0;
    if (spawn_data.count) {
      const auto at = spawn_data.offset;
      scenario_lower = real(data, at); scenario_upper = real(data, at + 4);
      if (scenario_lower != 0) settings.lower = scenario_lower;
      if (scenario_upper != 0) settings.upper = scenario_upper;
      const auto overrides = TagAddressSpace(data, metadata_magic_).block(at + 0x48, 0x10);
      for (std::size_t i = 0; i < overrides.count; ++i) {
        const auto row = overrides.offset + i * 0x10;
        const auto type = i16(data, row);
        if (type < 0 || std::size_t(type) >= settings.types.size())
          throw FormatError("Spawn influence override outside original type table");
        settings.types[type] = {real(data, row + 4), real(data, row + 8), real(data, row + 12)};
      }
      auto zones = [&](std::size_t pointer, std::vector<SpawnInfluence> &out_zones,
                       std::vector<SourceSpawnZone> &source_zones) {
        const auto entries = TagAddressSpace(data, metadata_magic_).block(pointer, 0x30);
        for (std::size_t i = 0; i < entries.count; ++i) {
          const auto row = entries.offset + i * 0x30;
          const auto games = data.u32(row + 8);
          const auto lower = real(data, row + 0x1c), upper = real(data, row + 0x20);
          const SpawnInfluence influence{point(data, row + 0x10), real(data, row + 0x24),
              real(data, row + 0x28), std::abs(upper != 0 ? upper : settings.upper),
              std::abs(lower != 0 ? lower : settings.lower), real(data, row + 0x2c)};
          source_zones.push_back({data.u32(row + 4), games, influence});
          // Retain the existing FFA bank for older callers/fixtures. Runtime
          // team admission uses the complete authored source rows below.
          if ((!games || (games & 1)) && out_zones.size() < 128) out_zones.push_back(influence);
        }
      };
      zones(at + 0x50, settings.respawn_zones, settings.source_respawn_zones);
      zones(at + 0x58, settings.initial_zones, settings.source_initial_zones);
    }
    out.multiplayer_spawn = std::move(settings);
    const auto netgame = TagAddressSpace(data, metadata_magic_).block(s + 0x120, 0x90);
    if (netgame.count > 100)
      throw FormatError("Netgame equipment exceeds original runtime row capacity");
    for (std::size_t i = 0; i < netgame.count; ++i) {
      const auto at = netgame.offset + i * 0x90;
      NetgameEquipment entry;
      entry.flags = data.u32(at);
      for (unsigned j = 0; j < entry.modes.size(); ++j)
        entry.modes[j] = std::int16_t(i16(data, at + 4 + j * 2));
      entry.team = std::int16_t(i16(data, at + 0xc));
      entry.respawn_seconds = std::int16_t(i16(data, at + 0xe));
      entry.empty_seconds = std::int16_t(i16(data, at + 0x10));
      entry.timer_start = std::int16_t(i16(data, at + 0x12));
      entry.classification = std::to_integer<std::uint8_t>(data.slice(at + 0x14, 1)[0]);
      entry.position = point(data, at + 0x40);
      entry.rotation = point(data, at + 0x4c);
      entry.collection_group = data.u32(at + 0x58);
      entry.collection = data.u32(at + 0x5c);
      if (const auto *collection = find(entry.collection);
          collection && (collection->group == "itmc" || collection->group == "vehc")) {
        const auto base = root(*collection, 0xc);
        entry.collection_delay = std::int16_t(i16(data, base + 8));
        const auto items = TagAddressSpace(data, metadata_magic_).block(base, 0x10);
        for (std::size_t j = 0; j < items.count; ++j) {
          const auto item = items.offset + j * 0x10;
          entry.items.push_back({real(data, item), data.u32(item + 4),
              data.u32(item + 8), data.u32(item + 0xc)});
        }
      }
      out.netgame_equipment.push_back(std::move(entry));
    }
    const auto starts = TagAddressSpace(data, metadata_magic_).block(s + 0x128, 0x9c);
    for (std::size_t i = 0; i < starts.count; ++i) {
      const auto at = starts.offset + i * 0x9c;
      LevelContent::StartingEquipment row;row.flags=data.u32(at);
      bool matches = false;
      for (unsigned j = 0; j < 4; ++j) {
        const auto mode = i16(data, at + 4 + j * 2);
        row.modes[j]=std::int16_t(mode);
        matches |= mode == 2 || (mode >= 12 && mode <= 14);
      }
      out.multiplayer_starting_equipment.push_back(row);
      if (matches && !out.multiplayer_starting_equipment_flags) {
        out.multiplayer_starting_equipment_flags = data.u32(at);
      }
    }
    const auto killers=TagAddressSpace(data, metadata_magic_).block(s+0x230, 2);
    for(std::size_t i=0;i<killers.count;++i) {
      const auto index=data.u16(killers.offset+i*2);
      if(index>=out.trigger_volumes.size())throw FormatError("Kill trigger index outside authored volumes");
      out.kill_triggers.push_back(index);
    }
  }
  if (scenario->size >= 0x388) {
    const auto transitions = TagAddressSpace(data, metadata_magic_).block(s + 0x380, 8);
    for (std::size_t i=0;i<transitions.count;++i) {
      const auto p = transitions.offset + i*8;
      const auto volume = i16(data,p+4);
      if (volume < -1 || (volume >= 0 && std::size_t(volume) >= volumes.count))
        throw FormatError("BSP transition volume outside scenario");
      out.bsp_transitions.push_back({data.u32(p),volume});
    }
  }
  for (std::size_t i = 0; i < squads.count; ++i) {
    const auto p = squads.offset + i * 0x74;
    SquadDefinition squad;
    squad.name = data.string(p, 32);
    squad.flags = data.u32(p + 0x20);
    squad.team = i16(data, p + 0x24);
    if (!squad.team) {
      const auto id = palette(chars, 8, i16(data, p + 0x36));
      const auto c = character(id);
      squad.team = c.biped.team;
      constexpr int teams[]{3, 7, 3, 3, 3,  3, 1, 2, 2, 4,
                            4, 4, 5, 5, -1, 3, 7, 7, 7, 4};
      if (!squad.team && c.actor_type >= 0 && c.actor_type < 20)
        squad.team = teams[c.actor_type];
    }
    squad.count = i16(data, p + 0x2c);
    squad.zone = i16(data, p + 0x38);
    if (squad.zone >= 0 && std::size_t(squad.zone) < zones.count) {
      const auto z = zones.offset + std::size_t(squad.zone) * 0x38;
      if (data.u32(z + 0x20) & 1)
        squad.bsp = i16(data, z + 0x24);
    }
    const auto starts = TagAddressSpace(data, metadata_magic_).block(p + 0x48, 0x64);
    for (std::size_t n = 0; n < starts.count; ++n) {
      const auto q = starts.offset + n * 0x64;
      int ci = i16(data, q + 0x20), wi = i16(data, q + 0x22);
      if (ci < 0)
        ci = i16(data, p + 0x36);
      if (wi < 0)
        wi = i16(data, p + 0x3c);
      ActorPlacement a{point(data, q + 4),         real(data, q + 0x14),
                       data.u32(q + 0x1c),         palette(chars, 8, ci),
                       palette(weapons, 0x28, wi), data.string(q + 0x40, 32)};
      if (a.character != null_datum)
        character(a.character);
      squad.starts.push_back(std::move(a));
    }
    out.squads.push_back(std::move(squad));
  }
  //257D00/259EC0: scnr script-data -> 30-byte point sets ->3C-byte points.
  const auto script_data=TagAddressSpace(data,metadata_magic_).block(s+0x1d8,8,1);
  if(script_data.count)for(const auto set:metadata_block_offsets(script_data.offset,0x30)) {
    auto &points=out.command_points.emplace_back();
    for(const auto p:metadata_block_offsets(set+0x20,0x3c))points.push_back({point(data,p+0x20),std::int16_t(i16(data,p+0x2c)),std::int32_t(data.u32(p+0x30))});
  }
  for(const auto p:metadata_block_offsets(s+0x1f0,0x24)) {
    CampaignTitleDefinition title;title.text=data.u32(p);title.color=data.u32(p+0x10);title.shadow=data.u32(p+0x14);
    for(unsigned i=0;i<4;++i)title.bounds[i]=std::int16_t(i16(data,p+4+i*2));
    title.justification=std::int16_t(i16(data,p+0xc));title.font=std::int16_t(i16(data,p+0xe));
    title.fade_in=real(data,p+0x18);title.up_time=real(data,p+0x1c);title.fade_out=real(data,p+0x20);
    out.cinematic_titles.push_back(title);
  }
  // MCC Floodlab owns 37,509 expressions (Xbox: 36,670). Inspection retains
  // the donor bank within its 16-bit datum index space; this does not admit
  // those additional expressions as converted Xbox gameplay scripts.
  const auto nodes = TagAddressSpace(data, metadata_magic_).block(s + 0x238, 20, mcc_ ? 0x10000 : 0x9000),
             scripts = TagAddressSpace(data, metadata_magic_).block(s + 0x1b8, 0x28),
             globals = TagAddressSpace(data, metadata_magic_).block(s + 0x1c0, 0x28);
  const auto strings =
      TagAddressSpace(data, metadata_magic_).block(s + 0x1b0, 1, 16 * 1024 * 1024);
  if (strings.count) {
    const auto bytes = data.slice(strings.offset, strings.count);
    out.scripts.strings.assign(reinterpret_cast<const char *>(bytes.data()),
                               bytes.size());
  }
  for (std::size_t n = 0; n < nodes.count; ++n) {
    const auto p = nodes.offset + n * 20;
    out.scripts.nodes.push_back({data.u16(p), data.u16(p + 2), data.u16(p + 4),
                                 data.u16(p + 6), data.u32(p + 8),
                                 data.u32(p + 12), data.u32(p + 16)});
  }
  for (std::size_t n = 0; n < scripts.count; ++n) {
    const auto p = scripts.offset + n * 0x28;
    out.scripts.scripts.push_back({data.string(p, 32), data.u16(p + 32),
                                   data.u16(p + 34), data.u32(p + 36)});
  }
  for (std::size_t n = 0; n < globals.count; ++n) {
    const auto p = globals.offset + n * 0x28;
    out.scripts.globals.push_back(
        {data.string(p, 32), data.u16(p + 32), data.u32(p + 36)});
  }
  {
    const auto groups=TagAddressSpace(data, metadata_magic_).block(s+0xa0, 40);
    for(std::size_t i=0;i<groups.count;++i){
      const auto p=groups.offset+i*40;
      out.device_groups.push_back({real(data,p+32),std::uint16_t(data.u32(p+36)&1)});
    }
  }
  out.devices.reserve(out.objects.size());
  out.prop_sandbox.reserve(out.objects.size());
  out.static_scenery_collision.reserve(out.objects.size());
  // These factories read only the tag and deterministic variant selection.
  // Share their immutable data; scene/prop owners retain each placement's
  // transform, generation, vitality and model-region state separately.
  using DefinitionKey=std::pair<std::uint32_t,std::uint32_t>;
  std::map<DefinitionKey,std::shared_ptr<const StaticSceneryCollisionDefinition>> scenery_definitions;
  std::map<DefinitionKey,std::shared_ptr<const PropSandboxDefinition>> prop_definitions;
  for(const auto& object:out.objects){
    // Campaign scripts address the same machine/control owners as multiplayer.
    out.devices.push_back(load_source_device_world_definition(*this,object));
    const DefinitionKey key{object.definition,object.variant};
    std::shared_ptr<const StaticSceneryCollisionDefinition> scenery;
    if(object.type==6) {
      auto [at,inserted]=scenery_definitions.try_emplace(key);
      if(inserted)at->second=load_static_scenery_collision_definition(*this,object);
      scenery=at->second;
    }
    out.static_scenery_collision.push_back(std::move(scenery));
    std::shared_ptr<const PropSandboxDefinition> definition;
    if(object.type==11) {
      auto [at,inserted]=prop_definitions.try_emplace(key);
      if(inserted)at->second=load_prop_sandbox_definition(*this,object);
      definition=at->second;
    }
    out.prop_sandbox.push_back(definition);
    if(definition)for(const auto& section:definition->sections)for(const auto& response:section.responses)
      if(response.damage!=null_datum && !out.prop_area_damage.contains(response.damage)){
        try{out.prop_area_damage.emplace(response.damage,load_prop_area_damage_definition(*this,response.damage));}
        catch(const FormatError& e){out.limitations.push_back(e.what());}
      }
  }
  // 15AAD0's stock vehicle choices retain the collection's definition and variant.
  // Templates remain cold until the phased netgame row actually allocates them.
  const auto heavy=source_game_variant_get(variant,78);
  std::array<std::uint32_t,2> heavy_choices{null_datum,null_datum};
  if(multiplayer && heavy) {
    if(heavy!=1 && heavy!=2 && heavy!=4)
      throw FormatError("Random Primary Heavy Vehicle needs its original per-birth RNG/template owner");
    // Literal TU1.5 15CC70 uses matg+16C -> mulg+C -> runtime+74.
    // Choices1/2 are the authored reference rows at+14/+1C, with the
    // original default variant B00078B. No definition is inferred by name.
    const auto globals=query_tag({.group=tag_groups::globals});
    if(!globals)throw FormatError("Missing original heavy-vehicle globals");
    const auto *mulg=find(data.u32(root(*globals,0x170)+0x16c),"mulg");
    if(!mulg)throw FormatError("Missing original heavy-vehicle runtime globals");
    const auto runtime=TagAddressSpace(data,metadata_magic_).block(root(*mulg,0x10)+8,0x568,1);
    if(runtime.count!=1)throw FormatError("Missing original heavy-vehicle runtime block");
    const auto choices=TagAddressSpace(data,metadata_magic_).block(runtime.offset+0x70,8);
    if(choices.count<4)throw FormatError("Missing original heavy-vehicle choices");
    heavy_choices={data.u32(choices.offset+0x14),data.u32(choices.offset+0x1c)};
  }
  for(unsigned row_index=0;row_index<out.netgame_equipment.size();++row_index){const auto &row=out.netgame_equipment[row_index];
    if(row.classification<=0 || row.classification>=10)continue;
    std::uint32_t definition=null_datum,selected_variant{};
    if(multiplayer && row.classification==3 && heavy) {
      if(heavy==4)continue;
      definition=heavy_choices.at(unsigned(heavy-1));selected_variant=0x0b00078bu;
    } else if(row.items.size()==1 && row.items.front().group==0x76656869u) {
      definition=row.items.front().definition;selected_variant=row.items.front().variant;
    }
    if(definition==null_datum)continue;
    ScenarioObject vehicle;
    vehicle.netgame_row=int(row_index);vehicle.type=1;vehicle.placement=int(out.objects.size());vehicle.definition=definition;
    vehicle.flags=1;vehicle.variant=selected_variant;vehicle.position=row.position;vehicle.rotation=row.rotation;vehicle.source=-1;
    const auto tag=load_source_tag_metadata(definition);const Bytes bytes(tag.bytes_at(tag.source_address(),tag.size()));
    vehicle.bounding_radius=bytes.f32(4);vehicle.bounding_offset={bytes.f32(8),bytes.f32(12),bytes.f32(16)};
    out.objects.push_back(std::move(vehicle));
  }

  // Model variant child objects use their authored parent/child attachment markers.
  // Keep their cold templates resident for both automatic and script-created vehicles.
  const auto initial_placements=out.objects.size();
  for(std::size_t i=0;i<out.objects.size();++i) {
    const auto parent=out.objects[i];
    // BF0F0 rejects palette index NONE before object/model construction.
    // Preserve unused scenario rows without treating them as templates.
    if(parent.type!=1 || parent.definition==null_datum)continue;
    unsigned depth=0;for(auto a=parent.model_attachment;a;){if(++depth>8)throw FormatError("Cyclic vehicle model attachments");a=out.objects.at(a->parent).model_attachment;}
    const auto t=load_source_tag_metadata(parent.definition);const Bytes b(t.bytes_at(t.source_address(),t.size()));
    if(b.u32(0x38)==null_datum)continue;const auto model=load_source_tag_metadata(b.u32(0x38));const Bytes m(model.bytes_at(model.source_address(),model.size()));
    const auto variants=m.u32(0x50);if(!variants)continue;
    const auto name=parent.variant?parent.variant:b.u32(0x30);const Bytes vs(model.bytes_at(m.u32(0x54),variants*0x38));
    unsigned selected=0;if(name)for(unsigned v=0;v<variants;++v)if(vs.u32(v*0x38)==name){selected=v;break;}
    const auto count=vs.u32(selected*0x38+0x1c);if(!count)continue;
    const Bytes children(model.bytes_at(vs.u32(selected*0x38+0x20),count*0x10));
    for(unsigned n=0;n<count;++n){const auto at=n*0x10;if(children.u32(at+8)!=0x76656869u || children.u32(at+12)==null_datum)continue;
      ScenarioObject child;child.type=1;child.placement=int(out.objects.size());child.definition=children.u32(at+12);child.flags=1;child.source=-1;
      child.model_attachment=ScenarioObject::ModelAttachment{i,children.u32(at),children.u32(at+4)};
      child.position=parent.position;child.rotation=parent.rotation;
      const auto ct=load_source_tag_metadata(child.definition);const Bytes cb(ct.bytes_at(ct.source_address(),ct.size()));
      child.bounding_radius=cb.f32(4);child.bounding_offset={cb.f32(8),cb.f32(12),cb.f32(16)};out.objects.push_back(std::move(child));
    }
    if(out.objects.size()>initial_placements+2048)throw FormatError("Vehicle attachment capacity exceeded");
  }
  out.unit_interactions=load_native_unit_interaction_assets(*this,out);
  return out;
}
unsigned Cache::sound_pitch_range_count(std::uint32_t datum) const {
  const auto* tag=query_tag({.handle=TagHandle{datum},.group=tag_groups::sound,.require_address=true});
  if(!tag)throw FormatError("Missing sound tag");
  return std::to_integer<unsigned>(Bytes(tag_payload({datum})).slice(10,1)[0]);
}
unsigned Cache::sound_permutation_count(std::uint32_t datum,unsigned pitch) const {
  if(pitch>=sound_pitch_range_count(datum))throw FormatError("Sound pitch outside ranges");
  const auto* sound=query_tag({.handle=TagHandle{datum},.group=tag_groups::sound,.require_address=true});
  const auto* gestalt=mcc_?&mcc_sound_gestalt(datum):query_tag({.group=tag_groups::sound_cache_file_gestalt,.require_address=true});
  if(!sound || !gestalt)throw FormatError("Missing sound/gestalt tag");
  const Bytes data(cache_bytes());
  const auto so=metadata_offset(sound->address,20),go=metadata_offset(gestalt->address,mcc_?0x78:0x48);
  const auto ranges=metadata_block(go+(mcc_?0x28:0x20),12);
  const auto index=std::size_t(data.u16(so+8))+pitch;
  if(index>=ranges.count)throw FormatError("Sound pitch escapes resident gestalt");
  return data.u16(ranges.offset+index*12+10);
}
std::optional<std::vector<std::byte>> Cache::load_sound_prediction_chunk(std::uint32_t datum) const {
  if (datum == null_datum) return {};
  if (vista_ || mcc_) throw FormatError("Original sound prediction chunk requires an Xbox cache");
  const auto *sound = query_tag({.handle=TagHandle{datum}, .group=tag_groups::sound, .require_address=true});
  const auto *gestalt = query_tag({.group=tag_groups::sound_cache_file_gestalt, .require_address=true});
  if (!sound || !gestalt) throw FormatError("Missing prediction sound/gestalt tag");
  const Bytes data(cache_bytes());
  const auto s = metadata_offset(sound->address,20), g = metadata_offset(gestalt->address,72);
  if (!byte(data,s+10)) return {};
  const auto ranges = TagAddressSpace(data,metadata_magic_).block(g+32,12);
  const auto range = data.u16(s+8);
  if (range >= ranges.count) throw FormatError("Prediction sound pitch outside gestalt");
  const auto r = ranges.offset + std::size_t(range)*12;
  if (!data.u16(r+10)) return {};
  const auto permutations = TagAddressSpace(data,metadata_magic_).block(g+40,16);
  const auto permutation = data.u16(r+8);
  if (permutation >= permutations.count) throw FormatError("Prediction sound permutation outside gestalt");
  const auto chunks = TagAddressSpace(data,metadata_magic_).block(g+64,12);
  const auto chunk = data.u16(permutations.offset+std::size_t(permutation)*16+12);
  if (chunk >= chunks.count) throw FormatError("Prediction sound chunk outside gestalt");
  const auto c = chunks.offset + std::size_t(chunk)*12;
  return raw_data(data.u32(c),data.u32(c+4)&0x3fffffffu);
}
EncodedSound Cache::load_sound(std::uint32_t datum, unsigned pitch,
                               unsigned permutation) const {
  if(mcc_)return load_mcc_sound(datum,pitch,permutation);
  const Bytes data(cache_bytes());
  auto tag = query_tag({.handle=TagHandle{datum}, .group=tag_groups::sound, .require_address=true});
  auto gestalt = query_tag({.group=tag_groups::sound_cache_file_gestalt, .require_address=true});
  if (!tag || !gestalt)
    throw FormatError("missing sound/gestalt tag");
  const auto s = metadata_offset(tag->address, 20),
             g = metadata_offset(gestalt->address, 72);
  EncodedSound out;
  out.name = tag->name;
  out.sound_class = byte(data,s + 2);
  if (out.sound_class >= 54) throw FormatError("invalid sound class index");
  constexpr unsigned rates[]{22050, 44100, 32000};
  const auto rate = byte(data, s + 3), encoding = byte(data, s + 4);
  if (rate > 2 || encoding > 2)
    throw FormatError("unsupported sound rate/channel encoding");
  out.sample_rate = rates[rate];
  out.channels = encoding==2?2:encoding + 1;
  // Retail encoding 2 selects the WMA stream codec (21E410), regardless of
  // the compression byte. The resident chunks contain an ASF container.
  out.codec = encoding==2?4:byte(data, s + 5);
  const auto ranges = TagAddressSpace(data, metadata_magic_).block(g + 32, 12),
             perms = TagAddressSpace(data, metadata_magic_).block(g + 40, 16),
             chunks = TagAddressSpace(data, metadata_magic_).block(g + 64, 12);
  if (pitch >= byte(data, s + 10))
    throw FormatError("sound pitch index outside ranges");
  const auto ri = std::size_t(data.u16(s + 8)) + pitch;
  if (ri >= ranges.count)
    throw FormatError("invalid sound pitch range");
  const auto r = ranges.offset + ri * 12;
  if (permutation >= data.u16(r + 10))
    throw FormatError("sound permutation index outside range");
  const auto pi = std::size_t(data.u16(r + 8)) + permutation;
  if (pi >= perms.count)
    throw FormatError("invalid sound permutation");
  const auto p = perms.offset + pi * 16;
  out.samples = data.u32(p + 8);
  const auto first = data.u16(p + 12), count = data.u16(p + 14);
  if (std::size_t(first) + count > chunks.count)
    throw FormatError("sound chunk range outside gestalt");
  for (std::size_t n = 0; n < count; ++n) {
    const auto c = chunks.offset + (first + n) * 12;
    // Original21ABD0 retains all lower30 size bits; only the top2 flag bits
    // are removed. Resource location belongs to the separate pointer word.
    auto bytes = raw_data(data.u32(c), data.u32(c + 4) & 0x3fffffffu);
    if (out.data.size() + bytes.size() > 128 * 1024 * 1024)
      throw FormatError("sound concatenation limit exceeded");
    out.data.insert(out.data.end(), bytes.begin(), bytes.end());
  }
  return out;
}
LoopingSound Cache::load_looping_sound(std::uint32_t datum) const {
  const Bytes data(cache_bytes());
  const auto tag = query_tag({.handle=TagHandle{datum}, .group=tag_groups::sound_looping, .require_address=true});
  if (!tag)
    throw FormatError("missing looping sound tag");
  const auto p = metadata_offset(tag->address, 0x2c);
  LoopingSound out;
  out.name = tag->name;
  out.flags = data.u32(p);
  const auto tracks = TagAddressSpace(data, metadata_magic_).block(p + 0x1c, 0x58, 32);
  for (std::size_t i = 0; i < tracks.count; ++i) {
    const auto t = tracks.offset + i * 0x58;
    out.tracks.push_back(
        {data.u32(t + 4), data.u32(t + 0x18), data.u32(t + 0x20),
         data.u32(t + 0x28), data.u32(t + 0x30), data.u32(t + 0x38),
         real(data, t + 8), real(data, t + 12), real(data, t + 16)});
  }
  return out;
}
std::string LevelContent::describe() const {
  std::ostringstream s;
  s << "collision_triangles=" << collision.size()
    << " navigation_sectors=" << navigation.size()
    << " weapons=" << weapons.size() << " characters=" << characters.size()
    << " squads=" << squads.size() << " scripts=" << scripts.scripts.size()
    << " globals=" << scripts.globals.size()
    << " nodes=" << scripts.nodes.size() << " objects=" << objects.size()
    << " names=" << object_names.size()
    << " camera_flags=" << cutscene_flags.size()
    << " volumes=" << trigger_volumes.size()
    << " content_errors=" << limitations.size();
  return s.str();
}
} // namespace halo2
