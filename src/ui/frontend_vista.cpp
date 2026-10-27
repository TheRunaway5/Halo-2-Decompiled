#include "halo2/frontend.hpp"
#include "halo2/frontend_extension_catalog.hpp"
#include "halo2/vista_lobby_metadata.hpp"
#include "halo2/vista_widget_cache.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "ui/mcc_pc_frontend_string_ids.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <set>
#include <zlib.h>

namespace halo2 {
namespace {
struct PcCache {
  std::filesystem::path source;
  std::vector<std::byte> storage;
  std::shared_ptr<const Cache> native_owner;
  Bytes bytes;
  std::size_t index{};
  bool mcc{};
  std::int64_t magic{};
  struct Tag {
    std::string group, name;
    std::size_t offset{}, size{};
    bool resident{};
  };
  mutable std::map<std::uint32_t, Tag> tags;
  explicit PcCache(const std::filesystem::path &path,bool gameplay=false) : source(path),bytes(storage) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    if (!file || size < 2048 || size > 512 * 1024 * 1024)
      throw FormatError("cannot read bounded Vista mainmenu cache");
    storage.resize(std::size_t(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(storage.data()), size))
      throw FormatError("incomplete Vista mainmenu cache");
    bytes = Bytes(storage);
    mcc=bytes.u32(4)==13;
    if(mcc&&gameplay) {
      // Gameplay directories retain unloaded references to shared.map. The
      // ordinary Cache owner performs the exact full-salt/group/name join,
      // overlap checks and bounded metadata flattening. Borrow its immutable
      // resident bytes instead of guessing another map or a pointer base.
      native_owner=std::make_shared<const Cache>(path);
      if(!native_owner->is_mcc())throw FormatError("MCC gameplay UI lost its cache owner");
      if(native_owner->source_runtime_cache_bytes().size()>512ull*1024*1024)
        throw FormatError("Excessive MCC gameplay UI cache");
      storage.clear();storage.shrink_to_fit();
      bytes=Bytes(native_owner->source_runtime_cache_bytes());index=bytes.u32(16);
      bytes.require(index,32);
      for(const auto &t:native_owner->tags()) {
        if(t.datum==null_datum)continue;
        Tag item{t.group,t.name,0,t.size};
        item.resident=t.address!=0;
        if(t.address&&t.size&&t.group!="sbsp"&&t.group!="ltmp") {
          const auto payload=native_owner->tag_payload({t.datum});
          item.offset=owned_offset(payload);item.size=payload.size();
        }
        if(!tags.emplace(t.datum,std::move(item)).second)
          throw FormatError("Duplicate MCC gameplay UI tag identity");
      }
      return;
    }
    if(mcc) {
      storage=decode_mcc_cache(storage);
      bytes=Bytes(storage);
      if(storage.size()>512*1024*1024)
        throw FormatError("excessive MCC settings cache");
    } else if (bytes.u32(0) != 0x68656164 || bytes.u32(4) != 8 ||
               bytes.u32(8) != storage.size() || bytes.u32(2044) != 0x666f6f74 ||
               bytes.string(0x12c, 32) != "11081.07.04.30.0934.main")
      throw FormatError("unsupported Vista settings cache");
    index = bytes.u32(16);
    bytes.require(index, 32);
    magic=mcc?std::int64_t(bytes.u32(720))-index:-std::int64_t(index);
    const auto count = bytes.u32(index + 24);
    if (!count || count > 65536 || count != bytes.u32(mcc?32:0x2cc))
      throw FormatError("invalid Vista tag count");
    const auto header_magic=mcc?std::int64_t(bytes.u32(index))-index-32:magic;
    const auto table = TagAddressSpace(bytes,header_magic).offset(bytes.u32(index+8),std::size_t(count)*16);
    const auto names = bytes.u32(mcc?36:0x2d0), names_size = bytes.u32(mcc?40:0x2d4);
    const auto offsets = bytes.u32(mcc?44:0x2d8);
    bytes.require(names, names_size);
    bytes.require(offsets, count * 4);
    for (unsigned i = 0; i < count; ++i) {
      const auto p = table + i * 16;
      const auto datum=bytes.u32(p+4);
      if(mcc && datum==null_datum)continue;
      if(mcc && (datum&0xffffu)!=i)
        throw FormatError("MCC settings datum disagrees with directory slot");
      const auto name = bytes.u32(offsets + i * 4);
      if (name >= names_size)
        throw FormatError("invalid Vista tag name");
      auto group = bytes.u32(p);
      std::string text;
      for (int shift = 24; shift >= 0; shift -= 8)
        text.push_back(char(group >> shift));
      Tag tag{text, bytes.string(names + name, names_size - name), 0,
              bytes.u32(p + 12)};
      if (bytes.u32(p + 8))
        tag.offset = pointer(bytes.u32(p + 8), tag.size);
      if (!tags.emplace(datum, std::move(tag)).second)
        throw FormatError("duplicate Vista tag datum");
    }
  }
  std::size_t owned_offset(std::span<const std::byte> payload) const {
    const auto owner=native_owner->source_runtime_cache_bytes();
    const auto base=reinterpret_cast<std::uintptr_t>(owner.data());
    const auto at=reinterpret_cast<std::uintptr_t>(payload.data());
    if(at<base||at-base>owner.size()||payload.size()>owner.size()-(at-base))
      throw FormatError("MCC gameplay UI metadata escaped its immutable cache owner");
    return at-base;
  }
  std::size_t pointer(std::uint32_t address, std::size_t size) const {
    if(native_owner)return owned_offset(native_owner->read_tag_bytes(address,size));
    return TagAddressSpace(bytes,magic).offset(address,size);
  }
  std::vector<std::byte> resource(std::uint32_t pointer,std::uint32_t size) const {
    if(!size || size>128*1024*1024)throw FormatError("invalid Vista UI resource size");
    if(native_owner)return native_owner->load_source_runtime_resource(pointer,size);
    // The same source file-selector encoding used by Cache::raw_data.
    const auto location=pointer>>30,offset=pointer&0x3fffffffu;
    if(!location) {
      const auto data=bytes.slice(offset,size);return {data.begin(),data.end()};
    }
    constexpr const char *names[]{"","mainmenu.map","shared.map","single_player_shared.map"};
    std::ifstream file(source.parent_path()/names[location],std::ios::binary|std::ios::ate);
    const auto length=file.tellg();
    if(length<0 || offset>std::uint64_t(length) || size>std::uint64_t(length)-offset)
      throw FormatError("Vista UI resource escapes its selected shared cache");
    std::vector<std::byte> data(size);file.seekg(offset);
    if(!file.read(reinterpret_cast<char *>(data.data()),size))throw FormatError("incomplete shared Vista UI resource");
    return data;
  }
  const Tag &tag(std::uint32_t datum, const char *group) const {
    const auto it = tags.find(datum);
    if(native_owner&&it!=tags.end()&&it->second.group==group&&
        !it->second.offset&&it->second.resident) {
      // Resolve an actually requested zero-sized alias through the Cache's
      // exact same-address/group allocation proof. Unused shader placeholders
      // are not forced to acquire a nonexistent payload during inventory.
      try {
        const auto payload=native_owner->tag_payload({datum});
        it->second.offset=owned_offset(payload);it->second.size=payload.size();
      } catch(const FormatError &error) {
        throw FormatError(std::string("MCC gameplay UI requires ")+group+" "+it->second.name+": "+error.what());
      }
    }
    if (it == tags.end() || it->second.group != group || !it->second.offset)
      throw FormatError(std::string("invalid source-local PC resource reference: ")+group+
          " datum "+std::to_string(datum));
    return it->second;
  }
  std::vector<std::size_t> block(std::size_t p, unsigned stride,
                                 unsigned limit = 1024) const {
    const auto n = bytes.u32(p);
    if (n > limit)
      throw FormatError("excessive Vista UI block at "+std::to_string(p)+
          ": "+std::to_string(n)+" records, limit "+std::to_string(limit));
    std::vector<std::size_t> result;
    if (n) {
      const auto at = pointer(bytes.u32(p + 4), std::size_t(n) * stride);
      for (unsigned i = 0; i < n; ++i)
        result.push_back(at + std::size_t(i) * stride);
    }
    return result;
  }
  int s16(std::size_t p) const {
    return std::bit_cast<std::int16_t>(bytes.u16(p));
  }
  UiRect rect(std::size_t p) const {
    return {float(s16(p)), float(s16(p + 2)), float(s16(p + 4)),
            float(s16(p + 6))};
  }
  Vec2 point(std::size_t p) const { return {float(s16(p)), float(s16(p + 2))}; }
  UiColor color(std::size_t p) const {
    UiColor c{bytes.f32(p), bytes.f32(p + 4), bytes.f32(p + 8),
              bytes.f32(p + 12)};
    for (auto v : {c.alpha, c.red, c.green, c.blue})
      if (!std::isfinite(v) || v < 0 || v > 1)
        throw FormatError("invalid Vista UI color");
    return c;
  }
};

std::shared_ptr<const VistaLobbyMetadata> lobby_metadata(const PcCache& cache,
    const std::vector<std::pair<std::uint32_t, std::string>>& locale) {
  const auto& b = cache.bytes;
  auto out = std::make_shared<VistaLobbyMetadata>();
  const auto source = [&](std::size_t p, std::size_t size) {
    const auto data = b.slice(p, size);
    return VistaLobbyMetadataBytes{p, {data.begin(), data.end()}};
  };
  const auto tag = [&](std::uint32_t datum, const char* group) {
    const auto& t = cache.tag(datum, group);
    return VistaLobbyMetadataTag{datum, t.group, t.name, source(t.offset, t.size)};
  };
  const auto signed_word = [&](std::size_t p) { return std::bit_cast<std::int32_t>(b.u32(p)); };
  const auto wide = [&](std::size_t p, unsigned count) {
    b.require(p, std::size_t(count) * 2);
    std::u16string result;
    for (unsigned i = 0; i < count; ++i) {
      const auto value = b.u16(p + i * 2);
      if (!value) break;
      result.push_back(static_cast<char16_t>(value));
    }
    return result;
  };
  out->globals = tag(b.u32(cache.index + 16), "matg");
  out->scenario = tag(b.u32(cache.index + 12), "scnr");
  const auto globals = out->globals.source.file_offset;
  out->scenario_type = cache.s16(out->scenario.source.file_offset + 0x10);
  const auto levels = cache.block(globals + 0x178, 24, 256);
  const auto path_headers = cache.block(globals + 0x170, 8, 256);
  for (const auto p : levels) out->level_headers.push_back(source(p, 24));
  for (const auto p : path_headers) out->path_headers.push_back(source(p, 8));
  if (!path_headers.empty()) for (const auto p : cache.block(path_headers.front(), 0x108, 256))
    out->paths.push_back({source(p, 0x108), signed_word(p), signed_word(p + 4), b.string(p + 8, 256)});
  const auto preview_ref = [&](std::uint32_t datum) {
    if (datum != null_datum && !out->preview_tags.contains(datum)) {
      out->preview_datums.push_back(datum);
      out->preview_tags.emplace(datum, tag(datum, "bitm"));
    }
    return datum;
  };
  if (!levels.empty()) {
    for (const auto p : cache.block(levels.front() + 8, 0xb50, 256)) {
      VistaLobbyMetadataMap map;
      map.source = source(p, 0xb50); map.campaign_id = signed_word(p); map.map_id = signed_word(p + 4);
      map.bitmap = preview_ref(b.u32(p + 12));
      map.english_name = wide(p + 0x10, 32); map.english_description = wide(p + 0x250, 128);
      if (const auto* path = out->campaign_path(map.campaign_id, map.map_id)) map.scenario_path = path->scenario_path;
      out->campaign.push_back(std::move(map));
    }
    for (const auto p : cache.block(levels.front() + 16, 0xc64, 256)) {
      VistaLobbyMetadataMap map;
      map.source = source(p, 0xc64); map.map_id = signed_word(p);
      map.bitmap = preview_ref(b.u32(p + 8));
      map.english_name = wide(p + 12, 32); map.english_description = wide(p + 0x24c, 128);
      map.scenario_path = b.string(p + 0xb4c, 256); map.sort_order = signed_word(p + 0xc4c);
      map.flags = std::to_integer<std::uint8_t>(b.slice(p + 0xc50, 1)[0]);
      for (unsigned i = 0; i < map.team_counts.size(); ++i)
        map.team_counts[i] = std::to_integer<std::uint8_t>(b.slice(p + 0xc54 + i, 1)[0]);
      out->multiplayer.push_back(std::move(map));
    }
  }
  for (const auto& [id, text] : locale)
    out->locale_entries.push_back({id, text, vista_lobby_metadata_utf16(text)});
  for (const auto& [datum, t] : cache.tags) if (t.group == "unic") {
    const auto p = t.offset;
    b.require(p, 20);
    const auto first = b.u16(p + 16), count = b.u16(p + 18);
    if (first > locale.size() || count > locale.size() - first)
      throw FormatError("invalid Vista lobby source-local locale range");
    out->locales.emplace(datum, VistaLobbyMetadataLocale{tag(datum, "unic"), first, count});
  }
  const auto require_locale = [&](std::uint32_t datum) {
    if (datum != null_datum && !out->locales.contains(datum))
      throw FormatError("missing Vista lobby source-local unic owner");
  };
  for (const auto& [datum, t] : cache.tags) if (t.group == "sily") {
    const auto p = t.offset;
    b.require(p, 36);
    VistaLobbyMetadataParameter parameter;
    parameter.owner = tag(datum, "sily");
    parameter.details = {datum, b.u32(p), b.u32(p + 12), b.u32(p + 16), {}};
    require_locale(parameter.details.locale);
    parameter.default_value = signed_word(p + 4); parameter.header = b.u32(p + 20);
    parameter.description = b.u32(p + 24);
    for (const auto q : cache.block(p + 28, 12, 256)) {
      parameter.details.choices.push_back({signed_word(q + 4), b.u32(q + 8)});
      parameter.choice_records.push_back(source(q, 12));
    }
    out->parameters.emplace(datum, std::move(parameter));
  }
  const auto interface_address = b.u32(globals + 0x114);
  if (b.u32(globals + 0x110) && interface_address != null_datum &&
      out->scenario_type >= 0 && out->scenario_type <= 2) {
    const auto interface = cache.pointer(interface_address, 0x98);
    const auto owner = b.u32(interface + (out->scenario_type == 0 ? 0x8c : out->scenario_type == 1 ? 0x94 : 0x84));
    if (owner != null_datum) {
      out->ui_globals = tag(owner, "wgtz");
      const auto p = out->ui_globals.source.file_offset;
      b.require(p, 24); out->shared_ui_globals = b.u32(p + 4);
      const auto goof = b.u32(p + 20);
      if (goof != null_datum) {
        out->game_settings = tag(goof, "goof");
        const auto q = out->game_settings.source.file_offset; b.require(q, 48);
        out->quick_settings_screen = b.u32(q + 12); out->variant_locale = b.u32(q + 36);
        require_locale(out->variant_locale);
        for (const auto r : cache.block(q + 24, 24, 64)) {
          VistaLobbyMetadataGroup group; group.source = source(r, 24); group.key = signed_word(r);
          for (const auto s : cache.block(r + 8, 8, 256)) {
            const auto datum = b.u32(s + 4);
            if (datum != null_datum && !out->parameters.contains(datum))
              throw FormatError("missing Vista lobby source-local sily owner");
            group.parameters.push_back(datum); group.reference_records.push_back(source(s, 8));
          }
          out->groups.push_back(std::move(group));
        }
        for (const auto r : cache.block(q + 40, 20, 256)) {
          VistaLobbyMetadataVariant variant; variant.source = source(r, 20);
          variant.name = b.u32(r); variant.engine = b.u32(r + 4);
          for (const auto s : cache.block(r + 8, 8, 256)) {
            variant.values.emplace_back(b.u32(s), signed_word(s + 4));
            variant.value_records.push_back(source(s, 8));
          }
          out->variants.push_back(std::move(variant));
        }
      }
    }
  }
  for (const auto& [datum, t] : cache.tags) if (t.group == "wgit") {
    const auto p = t.offset; b.require(p, 64);
    VistaLobbyMetadataScreen screen; screen.owner = tag(datum, "wgit"); screen.id = b.u16(p + 4);
    screen.details.locale = b.u32(p + 28); require_locale(screen.details.locale);
    for (const auto q : cache.block(p + 48, 12, 256)) {
      screen.value_names.push_back(b.u32(q)); screen.value_records.push_back(source(q, 12));
      std::vector<std::uint32_t> values; std::vector<VistaLobbyMetadataBytes> records;
      for (const auto r : cache.block(q + 4, 4, 1024)) {
        values.push_back(b.u32(r)); records.push_back(source(r, 4));
      }
      screen.details.values.push_back(std::move(values)); screen.string_records.push_back(std::move(records));
    }
    out->screens.emplace(datum, std::move(screen));
  }
  return out;
}
} // namespace
static std::shared_ptr<FrontendContent>
load_vista_ui(const std::filesystem::path &map, bool gameplay) {
  PcCache cache(map,gameplay);
  const auto &b = cache.bytes;
  auto out = std::make_shared<FrontendContent>();
  // The original PC source32 widget bank admits only its genuine v8 layout.
  // MCC keeps its own v13 identities and is consumed by the same native UI.
  if(!cache.mcc)
    out->vista_widget_cache = std::make_shared<const VistaWidgetCacheBank>(cache.storage);
  out->source_fonts = cache.mcc?map.parent_path().parent_path()/"h2_fonts":map.parent_path()/"fonts";
  if(cache.mcc) {
    (void)load_gameplay_font_source(out->source_fonts);
    // These compiled PC constants have a different symbol namespace. Check
    // the exact-name join against this v13 owner before using its mappings;
    // authored widget and locale IDs are retained without aliases.
    const auto count=b.u32(48),size=b.u32(56),indices=b.u32(60),strings=b.u32(52);
    if(count>1000000)throw FormatError("excessive MCC settings identifier table");
    b.require(indices,std::size_t(count)*4);b.require(strings,size);
    if(gameplay) {
      // A gameplay cache omits incidental mainmenu symbols and retains its
      // own indices. Resolve compiled PC symbols by their exact unique names;
      // do not rewrite any authored widget/locale ID or borrow another cache.
      std::map<std::string,std::optional<std::uint32_t>> names;
      for(std::uint32_t index=0;index<count;++index) {
        const auto offset=b.u32(indices+std::size_t(index)*4);
        if(offset>=size)continue; // Undefined slots cannot become bindings.
        const auto name=b.string(strings+offset,size-offset);if(name.empty())continue;
        const auto [entry,inserted]=names.emplace(name,index);
        if(!inserted)entry->second.reset();
      }
      auto bindings=std::make_shared<std::map<std::uint32_t,std::uint32_t>>();
      for(const auto &row:detail::mcc_pc_frontend_string_identifiers) {
        const auto found=names.find(std::string(row.name));if(found==names.end())continue;
        if(!found->second)throw FormatError("Ambiguous MCC gameplay settings identifier: "+std::string(row.name));
        bindings->emplace(row.original_index,(row.mcc_id&0xff000000u)|*found->second);
      }
      out->pc_string_ids=std::move(bindings);
    } else {
      for(const auto& row:detail::mcc_pc_frontend_string_identifiers) {
        const auto index=StringId{row.mcc_id}.index();
        if(index>=count)throw FormatError("MCC settings identifier outside table");
        const auto offset=b.u32(indices+std::size_t(index)*4);
        if(offset>=size || b.string(strings+offset,size-offset)!=row.name)
          throw FormatError("MCC settings identifier namespace differs from its named profile");
      }
    }
  }
  std::vector<std::pair<std::uint32_t, std::string>> locale;
  const auto &globals = cache.tag(b.u32(cache.index + 16), "matg");
  for (auto p : cache.block(globals.offset + 0x160, 12, 256)) {
    const Vec3 value{b.f32(p), b.f32(p + 4), b.f32(p + 8)};
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
      throw FormatError("invalid Vista appearance palette color");
    out->appearance_palette.push_back(value);
  }
  // PC5B4DB6 follows matg+16C to mulg, then the first universal
  // record at mulg+4. The RGB block belongs to universal+10/+14.
  const auto multiplayer = b.u32(globals.offset + 0x16c);
  if (multiplayer != null_datum) {
    const auto &mulg = cache.tag(multiplayer, "mulg");
    // Solo gameplay caches retain mulg but have an empty universal block.
    if(b.u32(mulg.offset)) {
      const auto universal = cache.pointer(b.u32(mulg.offset + 4), 0x18);
      for (auto p : cache.block(universal + 0x10, 12, 256)) {
        const Vec3 value{b.f32(p), b.f32(p + 4), b.f32(p + 8)};
        if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
          throw FormatError("invalid Vista PCR team palette color");
        out->vista_pcr_team_palette.push_back(value);
      }
    }
  }
  // PC60BB1D selects the wgtz owner through the actual scenario type
  // and matg interface[0], not the first wigl present in the cache.
  // PC60C1A9 then follows wgtz+4 and scans wigl+158 signed ranges.
  const auto &scenario = cache.tag(b.u32(cache.index + 12), "scnr");
  const auto type = cache.s16(scenario.offset + 0x10);
  std::optional<std::uint32_t> selected_ui_globals;
  const auto interface_address = b.u32(globals.offset + 0x114);
  if (b.u32(globals.offset + 0x110) && interface_address != null_datum &&
      type >= 0 && type <= 2) {
    const auto interface = cache.pointer(interface_address, 0x98);
    const auto owner = b.u32(interface + (type == 0 ? 0x8c : type == 1 ? 0x94 : 0x84));
    if (owner != null_datum) {
      const auto &wgtz = cache.tag(owner, "wgtz");
      const auto rank_owner = b.u32(wgtz.offset + 4);
      if (rank_owner != null_datum) {
        const auto &rank_wigl = cache.tag(rank_owner, "wigl");
        selected_ui_globals=rank_owner;
        out->vista_pcr_rank_owner_present = true;
        for (auto p : cache.block(rank_wigl.offset + 0x158, 4, 65536))
          out->vista_pcr_rank_ranges.push_back(
              {std::int16_t(cache.s16(p)), std::int16_t(cache.s16(p + 2))});
      }
    }
  }
  const auto loc = globals.offset + 0x190;
  const auto count = b.u32(loc), size = b.u32(loc + 4),
             indices = b.u32(loc + 8), strings = b.u32(loc + 12);
  if (count > 65536 || size > 16 * 1024 * 1024)
    throw FormatError("excessive Vista locale");
  std::vector<std::byte> owned_indices,owned_strings;
  if(cache.native_owner) {
    // Shared matg locale words are encoded source file selectors, not file
    // offsets or metadata pointers. Keep the Cache's admitted resource I/O
    // owner (including compressed MCC shared files) for both allocations.
    if(count)owned_indices=cache.resource(indices,count*8);
    if(size)owned_strings=cache.resource(strings,size);
  }
  const Bytes locale_indices=cache.native_owner?Bytes(owned_indices):Bytes(b.slice(indices,std::size_t(count)*8));
  const Bytes locale_strings=cache.native_owner?Bytes(owned_strings):Bytes(b.slice(strings,size));
  for (unsigned i = 0; i < count; ++i) {
    const auto p = std::size_t(i) * 8;
    const auto offset = locale_indices.u32(p + 4);
    if (offset >= size)
      throw FormatError("invalid Vista locale offset");
    locale.emplace_back(locale_indices.u32(p), locale_strings.string(offset, size - offset));
    out->global_strings.insert(locale.back());
  }
  auto string_list = [&](std::uint32_t datum) {
    UiStrings result;
    // Runtime-populated error dialogs have no authored locale owner.
    if(datum==null_datum)return result;
    const auto p = cache.tag(datum, "unic").offset;
    const auto first = b.u16(p + 16), n = b.u16(p + 18);
    if (first > locale.size() || n > locale.size() - first)
      throw FormatError("invalid Vista locale range");
    for (unsigned i = 0; i < n; ++i)
      result.insert(locale[first + i]);
    return result;
  };
  if(multiplayer!=null_datum) {
    const auto &mulg=cache.tag(multiplayer,"mulg");
    if(b.u32(mulg.offset)) {
      const auto universal=cache.pointer(b.u32(mulg.offset+4),0x18);
      const auto names=string_list(b.u32(universal+0xc));
      // Unchanged PC4CBF9B ordinals; resolve only the actual team-name owner.
      for(unsigned i=0;i<9;++i) {
        const auto id=detail::frontend_pc_compiled_string_id(*out,0x0100030au+i);
        if(id)if(const auto found=names.find(*id);found!=names.end())
          out->vista_team_names[i]=found->second;
      }
    }
  }
  std::set<std::uint32_t> images;
  auto image_ref = [&](std::uint32_t d) {
    if (d != null_datum) {
      cache.tag(d, "bitm");
      images.insert(d);
    }
    return d;
  };
  if (!gameplay) {
    out->vista_lobby_metadata = lobby_metadata(cache, locale);
    for (const auto datum : out->vista_lobby_metadata->preview_datums) image_ref(datum);
  }
  auto text = [&](std::size_t p) {
    UiText t;
    t.flags = b.u32(p);
    t.animation = b.u16(p + 4);
    t.delay = b.u16(p + 6);
    t.font = cache.s16(p + 10);
    t.color = cache.color(p + 12);
    t.bounds = cache.rect(p + 28);
    t.string = b.u32(p + 36);
    t.layer = cache.s16(p + 40);
    return t;
  };
  auto bitmap = [&](std::size_t p) {
    UiBitmap t;
    t.flags = b.u32(p);
    t.animation = b.u16(p + 4);
    t.delay = b.u16(p + 6);
    t.blend = b.u16(p + 8);
    t.frame = cache.s16(p + 10);
    t.position = cache.point(p + 12);
    t.scroll = {b.f32(p + 16), b.f32(p + 20)};
    t.datum = image_ref(b.u32(p + 28));
    t.layer = cache.s16(p + 32);
    t.fps = b.f32(p + 36);
    t.placeholder = b.u32(p + 44);
    t.scale = {b.f32(p + 48), b.f32(p + 52)};
    return t;
  };
  std::set<std::uint32_t> groups;
  auto grid = [&](std::size_t p) {
    UiGrid t;
    t.skin = b.u32(p + 8);
    t.position = cache.point(p + 12);
    t.order = std::to_integer<unsigned>(b.slice(p + 16, 1)[0]);
    t.count = std::to_integer<unsigned>(b.slice(p + 17, 1)[0]);
    t.rows = std::to_integer<unsigned>(b.slice(p + 18, 1)[0]);
    t.columns = std::to_integer<unsigned>(b.slice(p + 19, 1)[0]);
    t.step = {float(cache.s16(p + 22)), float(cache.s16(p + 20))};
    if (t.skin != null_datum) {
      cache.tag(t.skin, "skin");
      groups.insert(t.skin);
    }
    return t;
  };
  auto emblem = [&](std::size_t p) {
    UiEmblem t;
    t.flags = b.u32(p);
    t.animation = b.u16(p + 4);
    t.delay = b.u16(p + 6);
    t.layer = cache.s16(p + 8);
    t.page = cache.s16(p + 10);
    t.bitmap = image_ref(b.u32(p + 16));
    t.shader = b.u32(p + 24);
    t.bounds = cache.rect(p + 28);
    if (t.shader != null_datum) {
      const auto &shader = cache.tag(t.shader, "shad");
      if (shader.size < 84)
        throw FormatError("truncated source-local Vista emblem shader");
      for (auto definition : cache.block(shader.offset + 32, 124, 16)) {
        // MCC retains three platform slots with unused all-zero records.
        // These own no template/image references; every nonempty slot still
        // passes the same source-local group and bitmap dependency checks.
        if(cache.mcc && std::ranges::all_of(b.slice(definition,124),
            [](std::byte value){return value==std::byte{};}))continue;
        cache.tag(b.u32(definition), "stem");
        for (auto ref : cache.block(definition + 4, 12, 256))
          image_ref(b.u32(ref));
      }
      // Preserve this PC shader identity and its own complete image banks.
      // PC spas bytecode is not NV2A: Cache::load_material reports that pass
      // implementation as unresolved. Do not install an Xbox substitute in
      // emblem_shaders or claim that the retained definition is rendered.
    }
    return t;
  };
  auto track = [&](unsigned ms, std::size_t p) {
    UiTrack t;
    t.milliseconds = ms;
    for (auto q : cache.block(p, 20, 256))
      t.keys.push_back(
          {b.f32(q + 4), {b.f32(q + 8), b.f32(q + 12), b.f32(q + 16)}});
    return t;
  };
  if(!selected_ui_globals)
    throw FormatError("Vista settings cache lacks its scenario-selected UI globals owner");
  const auto g = cache.tag(*selected_ui_globals,"wigl").offset;
  out->music=b.u32(g+0x1bc);
  out->music_silence_ms=b.u32(g+0x1c0);
  out->color = cache.color(g + 0x168);
  out->overlay = cache.color(g + 0x6c);
  out->typewriter = {std::int16_t(cache.s16(g + 0x48)), b.u16(g + 0x4a),
                     b.f32(g + 0x4c)};
  for (unsigned i = 0; i < 4; ++i)
    out->header_fonts[i] = cache.s16(g + 0x160 + i * 2);
  for (unsigned i = 0; i < 8; ++i)
    out->header_bounds[i] = cache.rect(g + 0x178 + i * 8);
  // PC60BEBB builds this enum/string-ID table before looking up wigl+144.
  // Serialized locale order includes separate keyboard captions and cannot
  // substitute for the source subtitle enum (including missing enum5).
  constexpr std::uint32_t prompt_ids[]{
      0x0f0006cc, 0x110006cd, 0x100006ce, 0x130006cf, 0x110006d0,
      0x0e0006d1, 0x180006d2, 0x090006d3, 0x080006d4, 0x1a0006d5,
      0x180006d6, 0x080006d7, 0x080006d8, 0x230006d9, 0x250006da,
      0x240006db, 0x1c0006dc, 0x230006dd, 0x1c0006de, 0x1c0006df,
      0x180006e0, 0x040006e1, 0x16000873, 0x19000874, 0x11000975};
  const auto prompts = string_list(b.u32(g + 0x144));
  for (auto id : prompt_ids) {
    const auto mapped=detail::frontend_pc_compiled_string_id(*out,id);
    const auto found = mapped?prompts.find(*mapped):prompts.end();
    out->button_keys.push_back(found == prompts.end() ? std::string{} : found->second);
  }
  // The source's animated bitmap/text children share the PC wigl tracks.
  for (auto p : cache.block(g + 0x120, 44, 256))
    out->animations.push_back(
        {track(b.u32(p + 4), p + 8), track(b.u32(p + 16), p + 20),
         track(b.u32(p + 28), p + 36), cache.s16(p + 32)});
  const auto skin_refs = cache.block(g + 0x138, 8, 256);
  out->skins.resize(skin_refs.size());
  std::set<unsigned> skins;
  // PC14/23 and map selectors17/206 retain their own authored resources.
  // Import alone does not activate their native widget/row lifetime or change
  // the separately owned Xbox backdrop and current visible lobby route.
  const std::set<unsigned> shell_selected{5, 6, 13, 14, 16, 17, 19, 23, 38, 42, 44, 182, 185, 206, 209, 210, 242, 243, 245, 253, 256, 257, 263, 267, 268, 296, 297,
                                    258, 269, 271, 272, 273, 274,
                                    276, 285, 290, 293, 294, 295};
  const std::set<unsigned> game_selected{7,8,18,40,41,42,43,44,183,200,242,244,245,254,259,260,261,262,
      263,267,268,269,271,272,273,274,276,285,290,293,294,295};
  const auto &selected=gameplay?game_selected:shell_selected;
  for (const auto &[datum, tag] : cache.tags)
    if (tag.group == "wgit" && (tag.offset || (cache.native_owner&&tag.resident))) {
      const auto p = cache.tag(datum,"wgit").offset;
      const auto id = b.u16(p + 4);
      if (!selected.contains(id))
        continue;
      UiScreen s;
      s.name = tag.name;
      s.id = id;
      s.flags = b.u32(p);
      s.button_key = b.u16(p + 6);
      s.color = cache.color(p + 8);
      s.strings = string_list(b.u32(p + 28));
      s.shape = b.u16(p + 40);
      s.header = b.u32(p + 44);
      for (auto q : cache.block(p + 32, 76, 128)) {
        UiPane pane;
        pane.animation = b.u16(q + 2);
        for (auto a : cache.block(q + 4, 60)) {
          UiButton v;
          v.text = text(a);
          v.text.string = b.u32(a + 48);
          v.text.layer = cache.s16(a + 52);
          v.bitmap = image_ref(b.u32(a + 40));
          v.offset = cache.point(a + 44);
          v.vista_hit_top_offset = std::int16_t(cache.s16(a + 54));
          v.vista_flags = b.u32(a + 56);
          pane.buttons.push_back(v);
        }
        for (auto a : cache.block(q + 12, 24, 16)) {
          UiList l;
          l.flags = b.u32(a);
          l.skin = b.u16(a + 4);
          l.visible = b.u16(a + 6);
          l.position = cache.point(a + 8);
          l.animation = b.u16(a + 12);
          l.delay = b.u16(a + 14);
          if (l.skin >= out->skins.size() ||
              (!l.visible && (cache.mcc?b.u32(a+16)!=0:id!=297)) || l.visible > 128)
            throw FormatError("invalid Vista list");
          for (auto v : cache.block(a + 16, 20))
            l.labels.push_back(b.u32(v + 16));
          skins.insert(l.skin);
          pane.lists.push_back(std::move(l));
        }
        for (auto a : cache.block(q + 28, 44))
          pane.texts.push_back(text(a));
        for (auto a : cache.block(q + 36, 56))
          pane.bitmaps.push_back(bitmap(a));
        for (auto a : cache.block(q + 60, 36))
          pane.emblems.push_back(emblem(a));
        for (auto a : cache.block(q + 68, 24))
          pane.grids.push_back(grid(a));
        for (auto a : cache.block(q + 44, 76, 32)) {
          UiScene scene;
          scene.flags=b.u32(a);scene.animation=b.u16(a+4);scene.delay=b.u16(a+6);
          scene.scale={b.f32(a+28),b.f32(a+32),b.f32(a+36)};
          scene.camera={b.f32(a+40),b.f32(a+44),b.f32(a+48)};
          scene.field_of_view=b.f32(a+52);scene.viewport=cache.rect(a+56);
          for(auto v:{scene.scale.x,scene.scale.y,scene.scale.z,scene.camera.x,
              scene.camera.y,scene.camera.z,scene.field_of_view})
            if(!std::isfinite(v))throw FormatError("invalid Vista UI model scene scalar");
          for(auto n:cache.block(a+12,32,64))scene.objects.push_back(b.string(n,32));
          for(auto n:cache.block(a+20,32,64))scene.lights.push_back(b.string(n,32));
          pane.scenes.push_back(std::move(scene));
        }
        s.panes.push_back(std::move(pane));
      }
      for (auto a : cache.block(p + 56, 8))
        s.bitmap_resources.push_back(image_ref(b.u32(a + 4)));
      if (!out->screens.emplace(id, std::move(s)).second)
        throw FormatError("duplicate Vista settings screen");
    }
  if (out->screens.size() != selected.size())
    throw FormatError("missing Vista settings screen");
  auto load_skin = [&](std::uint32_t datum) {
    const auto &tag = cache.tag(datum, "skin");
    const auto p = tag.offset;
    UiSkin skin;
    skin.name = tag.name;
    skin.arrows = image_ref(b.u32(p + 8));
    skin.arrow_up = cache.point(p + 12);
    skin.arrow_down = cache.point(p + 16);
    for (auto q : cache.block(p + 20, 16, 8))
      skin.animations.push_back(track(b.u32(q + 4), q + 8));
    for (auto q : cache.block(p + 28, 44))
      skin.texts.push_back(text(q));
    for (auto q : cache.block(p + 36, 56))
      skin.bitmaps.push_back(bitmap(q));
    for (auto q : cache.block(p + 44, 36))
      skin.emblems.push_back(emblem(q));
    for (auto q : cache.block(p + 52, 24))
      skin.grids.push_back(grid(q));
    return skin;
  };
  for (auto index : skins) {
    const auto datum = b.u32(skin_refs[index] + 4);
    auto skin = load_skin(datum);
    out->skin_groups.emplace(datum, skin);
    out->skins[index] = std::move(skin);
  }
  while (!groups.empty()) {
    const auto datum = *groups.begin();
    groups.erase(groups.begin());
    if (!out->skin_groups.contains(datum))
      out->skin_groups.emplace(datum, load_skin(datum));
  }
  // ONLINE has a few donor banks that cannot enter the equal-density texture
  // pack (PC padding, button states and different service menu page counts).
  // Retain their actual source pages for the explicitly mapped ONLINE route.
  const std::set<std::string> online_art{
    "ui\\global_bitmaps\\fs_dialog", "ui\\list_skins\\default\\arrows",
    "ui\\screens\\game_shell\\pregame_lobby\\lobby_display",
    "ui\\screens\\game_shell\\pregame_lobby\\mp_games2",
    "ui\\screens\\game_shell\\pregame_lobby\\button",
    "ui\\screens\\game_shell\\pregame_lobby\\settings_framing",
    "ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\xbox_live_menu",
    "ui\\screens\\game_shell\\virtual_keyboard\\button_1",
    "ui\\screens\\game_shell\\virtual_keyboard\\button_2",
    "ui\\screens\\game_shell\\virtual_keyboard\\button_4",
    "ui\\screens\\game_shell\\virtual_keyboard\\button_5",
    "ui\\screens\\game_shell\\virtual_keyboard\\button_6"};
  for (const auto &[datum,tag] : cache.tags)
    if (!gameplay && tag.group=="bitm" && online_art.contains(tag.name)) image_ref(datum);
  for (auto datum : images) {
    const auto &tag = cache.tag(datum, "bitm");
    UiImage image;
    image.name = tag.name;
    for (auto p : cache.block(tag.offset + 0x44, cache.mcc?168:0x74, 256)) {
      const unsigned w = b.u16(p + 4), h = b.u16(p + 6), fmt = b.u16(p + 12),
                     mips = b.u16(p + 20);
      if (!w || !h || w > 4096 || h > 4096 || b.u16(p + 10) ||
          std::to_integer<unsigned>(b.slice(p + 8, 1)[0]) != 1 ||
          (b.u16(p + 14) & 8) ||
          mips >= unsigned(std::bit_width(std::max(w, h))))
        throw FormatError("invalid Vista UI image");
      if(cache.mcc) {
        if(p<tag.offset || p>std::uint64_t(tag.offset)+tag.size ||
            168>std::uint64_t(tag.offset)+tag.size-p || b.u32(p+84) || b.u32(p+88))
          throw FormatError("MCC settings bitmap descriptor escapes its owner or is split");
        const auto raw=read_mcc_texture_partition(map.parent_path()/"textures.dat",b.u32(p+56),b.u32(p+80));
        auto texture=prepare_texture(Bytes(raw),w,h,std::uint16_t(fmt),false,mips);
        if(fmt==16)texture=prepare_xemu_bc3_texture(texture);
        image.images.push_back(std::move(texture));
        continue;
      }
      if (fmt != 1 && fmt != 3 && fmt != 9 && fmt != 10 && fmt != 11 &&
          fmt != 14 && fmt != 15)
        throw FormatError("unsupported Vista UI format");
      std::size_t expected = 0;
      auto mw = w, mh = h;
      for (unsigned i = 0; i <= mips; ++i) {
        expected += fmt == 14 || fmt == 15
                        ? std::size_t((mw + 3) / 4) * ((mh + 3) / 4) *
                              (fmt == 14 ? 8 : 16)
                        : std::size_t(mw) * mh *
                              (fmt == 1               ? 1
                               : fmt == 3 || fmt == 9 ? 2
                                                      : 4);
        mw = std::max(1u, mw / 2);
        mh = std::max(1u, mh / 2);
      }
      if (b.u32(p + 32) || b.u32(p + 36) || b.u32(p + 56) || b.u32(p + 60))
        throw FormatError("split Vista UI image");
      const auto raw = cache.resource(b.u32(p + 28), b.u32(p + 52));
      std::vector<std::byte> expanded(expected);
      uLongf length = expanded.size();
      uLong consumed = raw.size();
      if (uncompress2(reinterpret_cast<Bytef *>(expanded.data()), &length,
                      reinterpret_cast<const Bytef *>(raw.data()),
                      &consumed) != Z_OK ||
          length != expected || consumed != raw.size())
        throw FormatError("invalid compressed Vista UI image");
      image.images.push_back(prepare_texture(Bytes(expanded), w, h,
                                             std::uint16_t(fmt), false, mips));
    }
    for (auto p : cache.block(tag.offset + 0x3c, 60, 256)) {
      std::vector<UiSprite> sequence;
      for (auto q : cache.block(p + 52, 32, 1024)) {
        UiSprite sprite{b.u16(q), b.f32(q + 8), b.f32(q + 12),
                        b.f32(q + 16), b.f32(q + 20),
                        {b.f32(q + 24), b.f32(q + 28)}};
        if (sprite.image >= image.images.size() ||
            !std::isfinite(sprite.left) || !std::isfinite(sprite.right) ||
            !std::isfinite(sprite.top) || !std::isfinite(sprite.bottom) ||
            !std::isfinite(sprite.registration.x) || !std::isfinite(sprite.registration.y) ||
            sprite.left < 0 || sprite.top < 0 || sprite.right > 1 || sprite.bottom > 1 ||
            sprite.left > sprite.right || sprite.top > sprite.bottom)
          throw FormatError("Vista UI sprite outside source bitmap page");
        sequence.push_back(sprite);
      }
      if (sequence.empty())
        for (unsigned i = 0; i < b.u16(p + 34); ++i) {
          const unsigned index = b.u16(p + 32) + i;
          if (index >= image.images.size())
            throw FormatError("Vista UI sequence outside source bitmap records");
          sequence.push_back({index, 0, 1, 0, 1, {}});
        }
      image.sequences.push_back(std::move(sequence));
    }
    out->images.emplace(datum, std::move(image));
  }
  return out;
}
std::shared_ptr<FrontendContent> load_vista_settings(const std::filesystem::path &map) {
  return load_vista_ui(map,false);
}
std::shared_ptr<FrontendContent> load_vista_pause(const std::filesystem::path &map) {
  return load_vista_ui(map,true);
}
} // namespace halo2
