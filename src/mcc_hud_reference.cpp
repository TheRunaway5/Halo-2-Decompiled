#include "halo2/mcc_hud_reference.hpp"
#include "halo2/mcc_hud_bitmap_geometry.hpp"
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
using Json = nlohmann::json;
struct HudPatch {
  unsigned kind, row, effect, function, offset, width, before, after;
};
#include "mcc_hud_reference.inc"
// Full allocation guards include all authored bytes, including unused padding;
// only declared pointers and typed tag handles are normalized to zero.
#include "mcc_hud_reference_hashes.inc"
#include "mcc_hud_geometry_reference.inc"
std::uint32_t add(std::uint32_t address, std::size_t size) {
  if (size > 0xffffffffu - address)
    throw FormatError("HUD metadata address overflow");
  return address + std::uint32_t(size);
}
void word(std::vector<std::byte> &bytes, std::size_t offset,
          std::uint32_t value, unsigned width = 4) {
  Bytes(bytes).require(offset, width);
  for (unsigned i = 0; i < width; ++i)
    bytes[offset + i] = std::byte(value >> (i * 8));
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  for (const auto byte : bytes) {
    const auto value = std::to_integer<unsigned>(byte);
    result += digits[value >> 4];
    result += digits[value & 15];
  }
  return result;
}
std::string digest(std::span<const std::byte> bytes) {
  std::array<unsigned char, 32> result{};
  unsigned size{};
  if (EVP_Digest(bytes.data(), bytes.size(), result.data(), &size, EVP_sha256(),
                 nullptr) != 1 ||
      size != result.size())
    throw FormatError("Cannot authenticate HUD metadata");
  return hex(
      {reinterpret_cast<const std::byte *>(result.data()), result.size()});
}
bool same(const Tag &a, const Tag &b) {
  return a.datum == b.datum && a.group == b.group && a.name == b.name &&
         a.address == b.address && a.size == b.size;
}
struct Function {
  std::vector<std::byte> bytes;
};
struct Effect {
  std::vector<std::byte> bytes;
  std::array<Function, 5> functions;
};
struct Row {
  std::vector<std::byte> bytes;
  std::vector<Effect> effects;
};
struct Snapshot {
  std::vector<std::byte> root, normalized_allocation;
  std::array<std::vector<Row>, 3> rows;
  Json logical;
};
class Reader {
  const Tag &owner_;
  const MccHudReferenceAccess &access_;
  std::map<std::uint32_t, std::string> strings_;

public:
  Reader(const Tag &owner, const MccHudReferenceAccess &access)
      : owner_(owner), access_(access) {}
  std::vector<std::byte> read(std::uint32_t address, std::size_t size) {
    if (!size) {
      if (address)
        throw FormatError("Empty HUD block has a non-null pointer");
      return {};
    }
    if (!address || address == 0xffffffffu || size > 1024 * 1024 ||
        size > 0x100000000ull - address)
      throw FormatError("Unowned HUD metadata extent");
    const auto result = access_.read(address, size);
    if (result.size() != size)
      throw FormatError("Truncated HUD metadata view");
    return {result.begin(), result.end()};
  }
  std::string string(std::uint32_t id) {
    auto found = strings_.find(id);
    if (found != strings_.end())
      return found->second;
    auto value = access_.string_id(id);
    if (value.size() != StringId{id}.length() || (value.empty() && id != 0))
      throw FormatError("HUD StringID lost its full typed name");
    strings_.emplace(id, value);
    return value;
  }
  Json reference(const Bytes &bytes, unsigned offset, TagGroup group) {
    // The complete all-ones TagReference is the authored null form in some
    // canonical screen widgets. Keep its serialized words intact.
    if (bytes.u32(offset) == 0xffffffffu && bytes.u32(offset + 4) == 0xffffffffu)
      return nullptr;
    if (bytes.u32(offset) != group.value)
      throw FormatError("HUD reference group changed");
    const TagHandle handle{bytes.u32(offset + 4)};
    if (handle.is_null())
      return nullptr;
    const auto tag = access_.resolve(handle);
    const unsigned minimum = group == tag_groups::shader   ? 84
                             : group == tag_groups::bitmap ? 76
                             : group == TagGroup{"nhdt"}   ? 40
                                                           : 16;
    if (!tag || tag->handle() != handle || tag->type() != group ||
        !tag->address || tag->size < minimum || tag->name.empty())
      throw FormatError("HUD reference has no full salted owner");
    unsigned matches{};
    for (const auto &entry : access_.directory) {
      // The physical directory also contains unoccupied slots. Compare
      // the declared group bytes without constructing a type for holes.
      if (entry.group == tag->group && entry.name == tag->name)
        ++matches;
      if (entry.handle() == handle && !same(entry, *tag))
        throw FormatError(
            "HUD reference resolver changed its directory identity");
    }
    if (matches != 1 ||
        std::none_of(access_.directory.begin(), access_.directory.end(),
                     [&](const auto &entry) { return same(entry, *tag); }))
      throw FormatError("Ambiguous HUD reference namespace");
    return tag->name;
  }
  Snapshot snapshot() {
    Snapshot out;
    out.normalized_allocation = read(owner_.address, owner_.size);
    if (out.normalized_allocation.size() < 40)
      throw FormatError("Truncated HUD root");
    out.root.assign(out.normalized_allocation.begin(),
                    out.normalized_allocation.begin() + 40);
    const auto clear_allocation = [&](std::uint32_t address) {
      if (address >= owner_.address &&
          std::uint64_t(address) - owner_.address + 4 <=
              out.normalized_allocation.size())
        word(out.normalized_allocation, address - owner_.address, 0);
    };
    const Bytes root(out.root);
    out.logical["parent"] = reference(root, 0, TagGroup{"nhdt"});
    auto normalized_root = out.root;
    for (const auto offset : {4u, 12u, 20u, 36u}) {
      word(normalized_root, offset, 0);
      clear_allocation(add(owner_.address, offset));
    }
    out.logical["normalized_root_hex"] = hex(normalized_root);
    constexpr std::array<const char *, 3> keys{"bitmap", "text", "screen"};
    for (unsigned kind = 0; kind < 3; ++kind) {
      const unsigned descriptor = kind == 0   ? 8
                                  : kind == 1 ? 16
                                              : 32,
                     stride = kind == 0   ? 100
                              : kind == 1 ? 84
                                          : 80;
      const auto count = root.u32(descriptor),
                 address = root.u32(descriptor + 4);
      if (count > 64)
        throw FormatError("Excessive HUD widget count");
      const auto rows = read(address, std::size_t(count) * stride);
      out.logical[keys[kind]] = Json::array();
      for (unsigned i = 0; i < count; ++i) {
        Row row;
        row.bytes.assign(rows.begin() + i * stride,
                         rows.begin() + (i + 1) * stride);
        auto normalized = row.bytes;
        const Bytes data(row.bytes);
        const auto row_address = add(address, i * stride);
        Json logical{{"name", string(data.u32(0))}, {"effects", Json::array()}};
        word(normalized, 0, 0);
        if (kind < 2) {
          const unsigned field = kind == 0 ? 40 : 32;
          logical["shader"] = reference(data, field, tag_groups::shader);
          word(normalized, field + 4, 0);
          clear_allocation(add(row_address, field + 4));
        }
        if (kind != 1) {
          logical["bitmap"] = reference(data, 32, tag_groups::bitmap);
          word(normalized, 36, 0);
          clear_allocation(add(row_address, 36));
        } else {
          logical["text_id"] = string(data.u32(40));
          word(normalized, 40, 0);
        }
        if (kind == 2)
          for (const auto field : {40u, 48u, 56u}) {
            logical["screen_ref" + std::to_string(field + 4)] =
                reference(data, field, TagGroup{"egor"});
            word(normalized, field + 4, 0);
            clear_allocation(add(row_address, field + 4));
          }
        const unsigned ep = kind == 0 ? 88 : kind == 1 ? 76 : 72;
        const auto ec = data.u32(ep), ea = data.u32(ep + 4);
        if (ec > 16)
          throw FormatError("Excessive HUD effect count");
        word(normalized, ep + 4, 0);
        clear_allocation(add(row_address, ep + 4));
        const auto effects = read(ea, ec * 104u);
        for (unsigned e = 0; e < ec; ++e) {
          Effect effect;
          effect.bytes.assign(effects.begin() + e * 104,
                              effects.begin() + (e + 1) * 104);
          auto ne = effect.bytes;
          const Bytes f(effect.bytes);
          Json le{{"flags", f.u32(0)}, {"functions", Json::array()}};
          for (unsigned k = 0; k < 5; ++k) {
            const auto q = 4 + k * 20, size = f.u32(q + 12),
                       pointer = f.u32(q + 16);
            if (size > 4096)
              throw FormatError("Excessive HUD function extent");
            effect.functions[k].bytes = read(pointer, size);
            le["functions"].push_back(
                {{"input", string(f.u32(q))},
                 {"range", string(f.u32(q + 4))},
                 {"hex", hex(effect.functions[k].bytes)}});
            for (auto offset : {q, q + 4, q + 16})
              word(ne, offset, 0);
            clear_allocation(add(ea, e * 104 + q + 16));
          }
          le["normalized_hex"] = hex(ne);
          logical["effects"].push_back(le);
          row.effects.push_back(std::move(effect));
        }
        logical["normalized_hex"] = hex(normalized);
        out.logical[keys[kind]].push_back(logical);
        out.rows[kind].push_back(std::move(row));
      }
    }
    return out;
  }
  bool geometry_bitmaps_qualified(const Snapshot &source) {
    std::set<std::string> names;
    for (const auto *kind : {"bitmap", "screen"})
      for (const auto &row : source.logical.at(kind))
        if (!row.at("bitmap").is_null())
          names.insert(row.at("bitmap").get<std::string>());
    for (const auto &name : names) {
      const Tag *owner{};
      for (const auto &tag : access_.directory)
        if (tag.group == "bitm" && tag.name == name) {
          if (owner)
            return false;
          owner = &tag;
        }
      if (!owner || !owner->address || owner->size < 76)
        return false;
      const auto bound = access_.resolve(owner->handle());
      if (!bound || !same(*owner, *bound))
        return false;
      const auto header = read(owner->address, 76);
      const Bytes root(header);
      const auto count = root.u32(68), sequences = root.u32(60);
      if (!count || count > 256 || sequences > 256)
        return false;
      const auto frames = read(root.u32(72), count * 168u);
      const Bytes records(frames);
      UiImage image;
      image.name = name;
      for (unsigned i = 0; i < count; ++i) {
        const auto at = i * 168u;
        if (std::to_integer<unsigned>(frames[at + 8]) != 1 || records.u16(at + 10))
          return false;
        Texture texture;
        texture.width = records.u16(at + 4);
        texture.height = records.u16(at + 6);
        texture.layers = 1;
        image.images.push_back(std::move(texture));
      }
      const auto sequence_rows = sequences ? read(root.u32(64), sequences * 60u)
                                           : std::vector<std::byte>{};
      const Bytes rows(sequence_rows);
      for (unsigned i = 0; i < sequences; ++i) {
        const auto at = i * 60u, sprites = rows.u32(at + 52);
        if (sprites > 1024)
          return false;
        std::vector<UiSprite> sequence;
        if (sprites) {
          const auto storage = read(rows.u32(at + 56), sprites * 32u);
          const Bytes data(storage);
          for (unsigned n = 0; n < sprites; ++n) {
            const auto q = n * 32u;
            if (data.u16(q) >= count)
              return false;
            sequence.push_back({data.u16(q), data.f32(q + 8), data.f32(q + 12),
                data.f32(q + 16), data.f32(q + 20), {data.f32(q + 24), data.f32(q + 28)}});
          }
        } else {
          const auto first = rows.u16(at + 32), size = rows.u16(at + 34);
          if (first > count || size > count - first)
            return false;
          for (unsigned n = 0; n < size; ++n)
            sequence.push_back({first + n, 0, 1, 0, 1, {}});
        }
        image.sequences.push_back(std::move(sequence));
      }
      // This metadata-only view never reads resources or creates GPU pixels.
      // Pixel-only mods can retain their geometry; foreign geometry is declined.
      if (!mcc_hud_bitmap_logical_extent(image, 0))
        return false;
    }
    return true;
  }
};
MccOwnedHudProjection build(Snapshot source) {
  for (const auto &patch : hud_patches) {
    auto &row = source.rows.at(patch.kind).at(patch.row);
    auto &bytes =
        patch.effect == 0xffffffffu
            ? row.bytes
            : row.effects.at(patch.effect).functions.at(patch.function).bytes;
    const Bytes view(bytes);
    const auto before =
        patch.width == 2 ? view.u16(patch.offset) : view.u32(patch.offset);
    if (before != patch.before)
      throw FormatError("HUD patch preimage changed");
    word(bytes, patch.offset, patch.after, patch.width);
  }
  MccOwnedHudProjection result;
  result.status = MccGameplayReferenceStatus::Converted;
  result.bytes = source.root;
  word(result.bytes, 8, 10);
  word(result.bytes, 16, 5);
  word(result.bytes, 32, 0);
  word(result.bytes, 36, 0);
  const auto append = [&](std::span<const std::byte> bytes) {
    const auto at = (result.bytes.size() + 3) & ~std::size_t(3);
    result.bytes.resize(at);
    result.bytes.insert(result.bytes.end(), bytes.begin(), bytes.end());
    return std::uint32_t(at);
  };
  const auto pointer = [&](std::uint32_t field, std::uint32_t target) {
    word(result.bytes, field, target);
    result.pointers.emplace_back(field, target);
  };
  std::array<std::uint32_t, 2> banks{};
  for (unsigned kind = 0; kind < 2; ++kind) {
    const unsigned count = kind == 0 ? 10 : 5, stride = kind == 0 ? 100 : 84;
    std::vector<std::byte> rows;
    for (unsigned i = 0; i < count; ++i)
      rows.insert(rows.end(), source.rows[kind][i].bytes.begin(),
                  source.rows[kind][i].bytes.end());
    banks[kind] = append(rows);
    pointer(kind == 0 ? 12 : 20, banks[kind]);
    for (unsigned i = 0; i < count; ++i)
      word(result.bytes, banks[kind] + i * stride + (kind == 0 ? 92 : 80), 0);
  }
  // These authored functions are read-only values. Intern identical complete
  // byte streams in this new owner; every consumer still gets its exact data.
  std::map<std::vector<std::byte>, std::uint32_t> functions;
  for (unsigned kind = 0; kind < 2; ++kind)
    for (unsigned i = 0; i < (kind == 0 ? 10u : 5u); ++i) {
      const auto &row = source.rows[kind][i];
      if (row.effects.empty())
        continue;
      const auto effect_address =
          std::uint32_t((result.bytes.size() + 3) & ~std::size_t(3));
      std::vector<std::byte> effects;
      for (const auto &effect : row.effects)
        effects.insert(effects.end(), effect.bytes.begin(), effect.bytes.end());
      (void)append(effects);
      pointer(banks[kind] + i * (kind == 0 ? 100 : 84) + (kind == 0 ? 92 : 80),
              effect_address);
      for (unsigned e = 0; e < row.effects.size(); ++e)
        for (unsigned k = 0; k < 5; ++k) {
          const auto &bytes = row.effects[e].functions[k].bytes;
          const auto field = effect_address + e * 104 + 4 + k * 20 + 16;
          if (bytes.empty()) {
            word(result.bytes, field, 0);
            continue;
          }
          auto found = functions.find(bytes);
          if (found == functions.end())
            found = functions.emplace(bytes, append(bytes)).first;
          pointer(field, found->second);
        }
    }
  if (result.bytes.size() > 65536)
    throw FormatError("Excessive projected HUD allocation");
  std::set<std::uint32_t> fields;
  for (const auto &[field, target] : result.pointers) {
    if (field > result.bytes.size() || result.bytes.size() - field < 4 ||
        target < 40 || target >= result.bytes.size() ||
        !fields.insert(field).second ||
        Bytes(result.bytes).u32(field) != target)
      throw FormatError("Invalid self-contained HUD relocation");
    for (auto previous : fields)
      if (previous != field && previous < field + 4 && field < previous + 4)
        throw FormatError("Overlapping HUD relocation fields");
  }
  return result;
}
MccOwnedHudProjection build_geometry(Snapshot source, const Json &profile) {
  std::set<std::tuple<unsigned, unsigned, unsigned>> patched;
  for (const auto &p : profile.at("patches")) {
    if (p.size() != 8)
      throw FormatError("Invalid HUD geometry patch");
    const auto kind = p[0].get<unsigned>(), row = p[1].get<unsigned>(),
               offset = p[4].get<unsigned>(), width = p[5].get<unsigned>();
    const bool offsets = width == 2 && offset % 2 == 0 &&
        ((kind == 0 && offset >= 52 && offset < 64) ||
         (kind == 1 && offset >= 64 && offset < 76));
    const bool registration = kind == 0 && width == 4 && offset % 4 == 0 &&
                              offset >= 64 && offset < 88;
    if ((!offsets && !registration) || p[2] != 0xffffffffu || p[3] != 0xffffffffu ||
        kind >= 2 || row >= source.rows[kind].size())
      throw FormatError("HUD geometry patch escapes its authored fields");
    for (unsigned byte = offset; byte < offset + width; ++byte)
      if (!patched.emplace(kind, row, byte).second)
        throw FormatError("Overlapping HUD geometry patches");
    auto &bytes = source.rows[kind][row].bytes;
    const Bytes before(bytes);
    const auto value = width == 2 ? before.u16(offset) : before.u32(offset);
    const auto after = p[7].get<std::uint32_t>();
    if (value != p[6].get<std::uint32_t>() ||
        (width == 2 && after > 0xffffu) ||
        (registration && !std::isfinite(std::bit_cast<float>(after))))
      throw FormatError("HUD geometry patch preimage changed");
    word(bytes, offset, after, width);
  }
  MccOwnedHudProjection result;
  result.status = MccGameplayReferenceStatus::Converted;
  result.bytes = source.root;
  const auto append = [&](std::span<const std::byte> bytes) {
    const auto at = (result.bytes.size() + 3) & ~std::size_t(3);
    result.bytes.resize(at);
    result.bytes.insert(result.bytes.end(), bytes.begin(), bytes.end());
    return std::uint32_t(at);
  };
  const auto pointer = [&](std::uint32_t field, std::uint32_t target) {
    word(result.bytes, field, target);
    result.pointers.emplace_back(field, target);
  };
  constexpr std::array<unsigned, 3> descriptor{8, 16, 32}, stride{100, 84, 80},
                                    effect_field{88, 76, 72};
  std::array<std::uint32_t, 3> banks{};
  for (unsigned kind = 0; kind < 3; ++kind) {
    const auto &rows = source.rows[kind];
    if (rows.empty()) {
      word(result.bytes, descriptor[kind] + 4, 0);
      continue;
    }
    std::vector<std::byte> bytes;
    for (const auto &row : rows)
      bytes.insert(bytes.end(), row.bytes.begin(), row.bytes.end());
    banks[kind] = append(bytes);
    pointer(descriptor[kind] + 4, banks[kind]);
    for (unsigned i = 0; i < rows.size(); ++i)
      word(result.bytes, banks[kind] + i * stride[kind] + effect_field[kind] + 4, 0);
  }
  std::map<std::vector<std::byte>, std::uint32_t> functions;
  for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned i = 0; i < source.rows[kind].size(); ++i) {
      const auto &row = source.rows[kind][i];
      if (row.effects.empty())
        continue;
      std::vector<std::byte> bytes;
      for (const auto &effect : row.effects)
        bytes.insert(bytes.end(), effect.bytes.begin(), effect.bytes.end());
      const auto address = append(bytes);
      pointer(banks[kind] + i * stride[kind] + effect_field[kind] + 4, address);
      for (unsigned e = 0; e < row.effects.size(); ++e)
        for (unsigned k = 0; k < 5; ++k) {
          const auto &function = row.effects[e].functions[k].bytes;
          const auto field = address + e * 104 + 20 + k * 20;
          if (function.empty()) {
            word(result.bytes, field, 0);
            continue;
          }
          auto found = functions.find(function);
          if (found == functions.end())
            found = functions.emplace(function, append(function)).first;
          pointer(field, found->second);
        }
    }
  if (result.bytes.size() > 65536)
    throw FormatError("Excessive projected HUD geometry allocation");
  std::set<std::uint32_t> fields;
  for (const auto &[field, target] : result.pointers) {
    if (field > result.bytes.size() || result.bytes.size() - field < 4 ||
        target < 40 || target >= result.bytes.size() || target % 4 ||
        !fields.insert(field).second || Bytes(result.bytes).u32(field) != target)
      throw FormatError("Invalid HUD geometry relocation");
    for (const auto prior : fields)
      if (prior != field && prior < field + 4 && field < prior + 4)
        throw FormatError("Overlapping HUD geometry relocation fields");
  }
  return result;
}
} // namespace
std::optional<MccOwnedHudProjection>
project_mcc_scoreboard_hud_reference(const Tag &owner,
                                     const MccHudReferenceAccess &access) {
  if (owner.group != "nhdt" || owner.name != "ui\\hud\\scoreboard")
    return {};
  MccOwnedHudProjection mismatch;
  if (!access.read || !access.resolve || !access.string_id ||
      owner.handle().is_null() || !owner.address || owner.size < 40 ||
      owner.size > 65536)
    return mismatch;
  try {
    const auto bound = access.resolve(owner.handle());
    if (!bound || !same(owner, *bound) ||
        std::count_if(access.directory.begin(), access.directory.end(),
                      [&](const auto &tag) {
                        return tag.group == owner.group &&
                               tag.name == owner.name;
                      }) != 1 ||
        std::none_of(access.directory.begin(), access.directory.end(),
                     [&](const auto &tag) { return same(tag, owner); }))
      return mismatch;
    auto snapshot = Reader(owner, access).snapshot();
    const auto fingerprint = digest(snapshot.normalized_allocation);
    if (snapshot.logical == Json::parse(hud_after) &&
        std::find(hud_postimages.begin(), hud_postimages.end(), fingerprint) !=
            hud_postimages.end()) {
      mismatch.status = MccGameplayReferenceStatus::AlreadyEquivalent;
      return mismatch;
    }
    if (snapshot.logical != Json::parse(hud_before) ||
        std::find(hud_preimages.begin(), hud_preimages.end(), fingerprint) ==
            hud_preimages.end())
      return mismatch;
    return build(std::move(snapshot));
  } catch (const FormatError &) {
    return mismatch;
  }
}
std::optional<MccOwnedHudProjection>
project_mcc_hud_reference(const Tag &owner, const MccHudReferenceAccess &access) {
  if (owner.name == "ui\\hud\\scoreboard")
    return project_mcc_scoreboard_hud_reference(owner, access);
  if (owner.group != "nhdt")
    return {};
  static const auto profiles = Json::parse(hud_geometry_profiles);
  const auto profile = std::find_if(profiles.begin(), profiles.end(), [&](const auto &p) {
    return p.at("name") == owner.name;
  });
  if (profile == profiles.end())
    return {};
  MccOwnedHudProjection mismatch;
  if (!access.read || !access.resolve || !access.string_id || owner.handle().is_null() ||
      !owner.address || owner.size < 40 || owner.size > 65536)
    return mismatch;
  try {
    const auto bound = access.resolve(owner.handle());
    if (!bound || !same(owner, *bound) ||
        std::count_if(access.directory.begin(), access.directory.end(), [&](const auto &tag) {
          return tag.group == owner.group && tag.name == owner.name;
        }) != 1 ||
        std::none_of(access.directory.begin(), access.directory.end(), [&](const auto &tag) {
          return same(tag, owner);
        }))
      return mismatch;
    Reader reader(owner, access);
    auto source = reader.snapshot();
    if (!reader.geometry_bitmaps_qualified(source))
      return mismatch;
    const auto fingerprint = digest(source.normalized_allocation);
    if (source.logical == profile->at("after") &&
        std::find(profile->at("postimages").begin(), profile->at("postimages").end(), fingerprint) !=
            profile->at("postimages").end()) {
      mismatch.status = MccGameplayReferenceStatus::AlreadyEquivalent;
      return mismatch;
    }
    if (source.logical != profile->at("before") ||
        std::find(profile->at("preimages").begin(), profile->at("preimages").end(), fingerprint) ==
            profile->at("preimages").end())
      return mismatch;
    return build_geometry(std::move(source), *profile);
  } catch (const FormatError &) {
    return mismatch;
  }
}
std::uint32_t normalize_mcc_hud_function_operand(std::uint32_t id,
                                                 std::string_view name) {
  struct Operand {
    std::string_view name;
    std::uint32_t original;
  };
  // Literal original22ACE7 / TU1.5 2316E7 comparisons. The complete84-op
  // TU1.5 CFG and102 unchanged CPU cases are pinned in the private authority.
  constexpr std::array supported{Operand{"hud_elite", 0x090005ce},
                                 Operand{"hud_input1", 0x0a0005cf},
                                 Operand{"hud_input2", 0x0a0005d0},
                                 Operand{"hud_input3", 0x0a0005d1},
                                 Operand{"hud_input4", 0x0a0005d2},
                                 Operand{"hud_wp_alpha", 0x0c0005cb},
                                 Operand{"hud_wp_theta", 0x0c0005cc},
                                 Operand{"hud_wp_os_amt", 0x0d0005ca},
                                 Operand{"hud_wp_arrow_t", 0x0e0005cd}};
  if (StringId{id}.length() != name.size() || (name.empty() && id != 0))
    throw FormatError("HUD function StringID has an invalid typed name");
  for (const auto &operand : supported)
    if (operand.name == name)
      return operand.original;
  for (const auto &operand : supported)
    if (operand.original == id)
      throw FormatError(
          "Unsupported MCC HUD function collides with an original operand");
  return id;
}
} // namespace halo2
