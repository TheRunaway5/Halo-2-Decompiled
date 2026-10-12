#include "halo2/material_effects.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
namespace {
std::array<std::uint32_t, 3>
select_material_effects(const Cache &cache, std::uint32_t foot,
                        std::int16_t fallback_material,
                        std::int16_t contact_material, unsigned action,
                        std::int16_t variant, bool visual) {
  constexpr auto none = 0xffffffffu;
  const auto* globals = cache.query_tag({.group=tag_groups::globals});
  if (!globals)
    throw FormatError("Material effects require global materials");
  const auto load = [&](std::uint32_t datum) -> const Tag & {
    const auto* tag = cache.find_tag({datum});
    if (!tag)
      throw FormatError("Material effect owner is missing");
    if (!tag->size) {
      const auto* backing = cache.query_tag({.group=tag->type(), .address=tag->address, .minimum_size=1});
      if (!backing)
        throw FormatError("Material effect alias has no owner");
      (void)cache.tag_payload(backing->handle());
      return *backing;
    }
    (void)cache.tag_payload(tag->handle());
    return *tag;
  };
  const auto read = [&](std::uint32_t pointer, std::size_t size) -> Bytes {
    const auto* owner = cache.query_tag({.contains=TagAddressRange{pointer,size}, .require_address=true});
    if (!owner)
      throw FormatError("Material effect pointer has no resident owner");
    (void)load(owner->datum);
    return Bytes(cache.read_tag_bytes(pointer,size));
  };
  const auto &global = load(globals->datum);
  const Bytes root(Bytes(cache.tag_payload(global.handle())).slice(0,0x158));
  const auto count = root.u32(0x150);
  const auto material = [&](int index) {
    if (index < 0 || unsigned(index) >= count)
      throw FormatError("Material effect index escapes global bank");
    return read(root.u32(0x154) + unsigned(index) * 0xb4, 0xb4);
  };
  const auto find = [&](std::uint32_t datum) -> std::optional<Bytes> {
    if (datum == none)
      return {};
    const auto &tag = load(datum);
    if (tag.group != "foot")
      throw FormatError("Material effect owner group differs from foot");
    const Bytes definition(Bytes(cache.tag_payload(tag.handle())).slice(0,8));
    int selected = int(!visual && action == 13 ? 14 : action);
    const unsigned bank = visual ? 0x10 : 8;
    for (;;) {
      if (selected < 0)
        return {};
      if (unsigned(selected) < definition.u32(0)) {
        const auto entry =
            read(definition.u32(4) + unsigned(selected) * 0x18, 0x18);
        if (entry.u32(bank)) {
          if (entry.u32(bank) > 65536)
            throw FormatError("Material effect row bank is excessive");
          const auto rows = read(entry.u32(bank + 4), entry.u32(bank) * 0x18);
          std::set<int> visited;
          for (int index = contact_material;
               index >= 0 && unsigned(index) < count;
               index = std::int16_t(material(index).u16(8))) {
            if (!visited.insert(index).second)
              throw FormatError("Material effect parent cycle");
            for (unsigned n = 0; n < entry.u32(bank); ++n)
              if (std::int16_t(rows.u16(n * 0x18 + 0x14)) == index)
                return Bytes(rows.slice(n * 0x18, 0x18));
          }
          return {};
        }
      }
      switch (selected) {
      case 13:
        selected = 14;
        break;
      case 16:
      case 17:
        selected = 15;
        break;
      case 19:
        selected = 16;
        break;
      case 20:
        selected = 19;
        break;
      default:
        return {};
      }
    }
  };
  auto entry = find(foot);
  std::set<int> visited;
  for (int index = fallback_material;
       !entry && index >= 0 && unsigned(index) < count;
       index = std::int16_t(material(index).u16(8))) {
    if (!visited.insert(index).second)
      throw FormatError("Material effect fallback cycle");
    entry = find(material(index).u32(0xb0));
  }
  std::array<std::uint32_t, 3> result{none, none, none};
  if (!entry)
    return result;
  result[0] = entry->u32(4);
  result[2] = entry->u32(12);
  if ((entry->u16(0x16) & 255) == 1 && variant != -1) {
    if (contact_material < 0 || unsigned(contact_material) >= count)
      throw FormatError("Material sweetener requires its contact material");
    if (variant < 0 || variant >= 7)
      throw FormatError("Material sweetener variant escapes authored bank");
    const unsigned sweetener =
        visual ? (action == 13   ? 0x94
                  : action == 14 ? 0x8c
                                 : 0x74 + unsigned(variant) * 8)
               : (action == 13 || action == 14 ? 0x54
                                               : 0x3c + unsigned(variant) * 8);
    result[1] = material(contact_material).u32(sweetener);
  }
  return result;
}
} // namespace
std::array<std::uint32_t, 3> source_material_visual_effects(
    const Cache &cache, std::uint32_t foot, std::int16_t fallback_material,
    std::int16_t contact_material, unsigned action, std::int16_t variant) {
  return select_material_effects(cache, foot, fallback_material,
                                 contact_material, action, variant, true);
}
std::array<std::uint32_t, 3> source_material_sound_effects(
    const Cache &cache, std::uint32_t foot, std::int16_t fallback_material,
    std::int16_t contact_material, unsigned action, std::int16_t variant) {
  return select_material_effects(cache, foot, fallback_material,
                                 contact_material, action, variant, false);
}
} // namespace halo2
