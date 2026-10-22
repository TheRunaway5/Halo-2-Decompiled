#include "halo2/mcc_hud_shader_reference.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
namespace halo2 {
namespace {
using Json = nlohmann::json;
#include "ui/hud/mcc_hud_shader_reference.inc"
constexpr std::array<unsigned, 14> fields{4,  12, 20, 28, 36, 44,  52,
                                          60, 68, 76, 84, 92, 100, 108};
constexpr std::array<unsigned, 14> strides{12, 4, 16, 6, 2, 2,  10,
                                           20, 4, 2,  4, 4, 12, 4};
void word(std::vector<std::byte> &bytes, std::size_t at, std::uint32_t value) {
  Bytes(bytes).require(at, 4);
  for (unsigned k = 0; k < 4; ++k)
    bytes[at + k] = std::byte(value >> (8 * k));
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string out;
  for (auto b : bytes) {
    auto n = std::to_integer<unsigned>(b);
    out += digits[n >> 4];
    out += digits[n & 15];
  }
  return out;
}
std::string digest(std::span<const std::byte> bytes) {
  std::array<unsigned char, 32> out{};
  unsigned size{};
  if (EVP_Digest(bytes.data(), bytes.size(), out.data(), &size, EVP_sha256(),
                 nullptr) != 1 ||
      size != out.size())
    throw FormatError("Cannot authenticate HUD shader metadata");
  return hex({reinterpret_cast<const std::byte *>(out.data()), out.size()});
}
bool same(const Tag &a, const Tag &b) {
  return a.datum == b.datum && a.group == b.group && a.name == b.name &&
         a.address == b.address && a.size == b.size;
}
std::uint32_t add(std::uint32_t address, std::size_t size) {
  if (size > 0xffffffffu - address)
    throw FormatError("HUD shader metadata address overflow");
  return address + std::uint32_t(size);
}
class Reader {
public:
  const MccHudShaderReferenceAccess &access;
  explicit Reader(const MccHudShaderReferenceAccess &a) : access(a) {}
  void owner(const Tag &tag, bool resident = true) const {
    const auto bound = access.resolve(tag.handle());
    if (tag.handle().is_null() || tag.group.size() != 4 || tag.name.empty() ||
        !bound || !same(tag, *bound) ||
        (resident && (!tag.address || !tag.size)))
      throw FormatError("HUD shader typed owner unavailable");
    unsigned names{}, handles{};
    for (const auto &t : access.directory) {
      if (t.group == tag.group && t.name == tag.name)
        ++names;
      if (t.datum == tag.datum) {
        ++handles;
        if (!same(t, tag))
          throw FormatError("HUD shader full-salt directory mismatch");
      }
    }
    if (names != 1 || handles != 1)
      throw FormatError("HUD shader owner alias");
  }
  Tag named(std::string_view group, std::string_view name) const {
    std::optional<Tag> out;
    for (const auto &t : access.directory)
      if (t.group == group && t.name == name) {
        if (out)
          throw FormatError("HUD shader namespace alias");
        out = t;
      }
    if (!out)
      throw FormatError("HUD shader closure reference unavailable");
    owner(*out, false);
    return *out;
  }
  std::vector<std::byte> read(std::uint32_t address, std::size_t size) const {
    if (!size) {
      if (address)
        throw FormatError("Empty HUD shader block has nonnull pointer");
      return {};
    }
    if (!address || address == 0xffffffffu || size > 1024 * 1024 ||
        std::uint64_t(address) + size > 0x100000000ull)
      throw FormatError("HUD shader source extent invalid");
    const auto view = access.source_read(address, size);
    if (view.size() != size)
      throw FormatError("HUD shader source view truncated");
    return {view.begin(), view.end()};
  }
  std::string string(std::uint32_t id) const {
    auto name = access.string_id(id);
    if (name.size() != (id >> 24) || (name.empty() && id))
      throw FormatError("HUD shader function StringID lost its full name");
    return name;
  }
  void native(const Tag &tag, std::span<const std::byte> payload,
              const Json &policy) const {
    owner(tag);
    if (tag.group != policy.at("group").get<std::string>() || tag.name != policy.at("name").get<std::string>() ||
        tag.size != policy.at("bytes").get<unsigned>() || payload.size() != tag.size ||
        digest(payload) != policy.at("sha256").get<std::string>())
      throw FormatError("HUD native closure payload changed");
    const auto end = std::uint64_t(tag.address) + tag.size;
    if (end > 0x100000000ull)
      throw FormatError("HUD native closure extent overflow");
    for (const auto &t : access.directory)
      if (t.datum != tag.datum && t.address && t.size &&
          std::uint64_t(t.address) < end &&
          std::uint64_t(tag.address) < std::uint64_t(t.address) + t.size)
        throw FormatError("HUD native closure overlaps another typed owner");
    auto expected = std::vector<std::byte>(payload.begin(), payload.end());
    std::set<unsigned> fixups;
    for (const auto &p : policy.at("pointers")) {
      const auto field = p.at(0).get<unsigned>(),
                 target = p.at(1).get<unsigned>();
      if (field > expected.size() || expected.size() - field < 4 ||
          target >= expected.size() || Bytes(expected).u32(field) != target ||
          !fixups.insert(field).second)
        throw FormatError("HUD native closure pointer contract changed");
      word(expected, field, add(tag.address, target));
    }
    for (const auto &p : policy.at("references")) {
      const auto field = p.at(0).get<unsigned>();
      const auto target = named(p.at(1).at(0).get<std::string>(),
                                p.at(1).at(1).get<std::string>());
      if (!fixups.insert(field).second)
        throw FormatError("HUD native closure fixup overlap");
      word(expected, field, target.datum);
    }
    const auto actual = access.native_read(tag.address, tag.size);
    if (actual.size() != expected.size() ||
        !std::equal(actual.begin(), actual.end(), expected.begin()))
      throw FormatError(
          "HUD native published closure differs from retained payload");
  }
};
struct Snapshot {
  std::vector<std::byte> root, runtime, definition, allocation;
  std::array<std::vector<std::byte>, 14> banks;
  std::vector<std::vector<std::byte>> curves;
  Json logical;
};
Snapshot snapshot(const Tag &tag, const Reader &reader) {
  Snapshot out;
  out.allocation = reader.read(tag.address, tag.size);
  const Bytes allocation(out.allocation);
  out.root.assign(out.allocation.begin(), out.allocation.begin() + 84);
  const Bytes root(out.root);
  const auto normalize = [&](std::uint32_t address) {
    if (address >= tag.address &&
        std::uint64_t(address) - tag.address + 4 <= out.allocation.size())
      word(out.allocation, address - tag.address, 0);
  };
  auto normalized = out.root;
  for (auto q : {4u, 16u, 36u}) {
    word(normalized, q, 0);
    normalize(add(tag.address, q));
  }
  out.logical["root_hex"] = hex(normalized);
  if (root.u32(12) != 1 || root.u32(32) != 3)
    throw FormatError("HUD shader source root shape changed");
  out.runtime = reader.read(root.u32(16), 80);
  out.logical["runtime_hex"] = hex(out.runtime);
  const auto definitions = reader.read(root.u32(36), 3 * 124);
  out.logical["definitions"] = Json::array();
  for (unsigned d = 0; d < 3; ++d) {
    auto definition = std::vector<std::byte>(
        definitions.begin() + d * 124, definitions.begin() + (d + 1) * 124);
    const Bytes b(definition);
    if ((!d && b.u32(0) != root.u32(4)) ||
        (d && std::any_of(definition.begin(), definition.end(),
                          [](std::byte value) { return value != std::byte{}; })))
      throw FormatError("HUD source definition lost its exact typed template");
    auto normal = definition;
    word(normal, 0, 0);
    normalize(add(root.u32(36), d * 124));
    Json logical{{"banks", Json::array()}};
    for (unsigned k = 0; k < fields.size(); ++k) {
      const auto field = fields[k], count = b.u32(field),
                 address = b.u32(field + 4);
      if (count > 256)
        throw FormatError("Excessive HUD shader metadata bank");
      auto bank = reader.read(address, std::size_t(count) * strides[k]);
      auto nb = bank;
      Json curves = Json::array();
      word(normal, field + 4, 0);
      normalize(add(root.u32(36), d * 124 + field + 4));
      if (field == 60)
        for (unsigned i = 0; i < count; ++i) {
          const Bytes f(bank);
          const auto q = i * 20, size = f.u32(q + 12);
          if (size > 4096)
            throw FormatError("Excessive HUD shader function");
          auto curve = reader.read(f.u32(q + 16), size);
          curves.push_back({{"input", reader.string(f.u32(q))},
                            {"range", reader.string(f.u32(q + 4))},
                            {"hex", hex(curve)}});
          for (auto at : {q, q + 4, q + 16})
            word(nb, at, 0);
          normalize(add(address, q + 16));
          if (!d)
            out.curves.push_back(std::move(curve));
        }
      logical["banks"].push_back({{"field", field},
                                  {"count", count},
                                  {"stride", strides[k]},
                                  {"hex", hex(nb)},
                                  {"curves", curves}});
      if (!d)
        out.banks[k] = std::move(bank);
    }
    logical["hex"] = hex(normal);
    out.logical["definitions"].push_back(logical);
    if (!d)
      out.definition = std::move(definition);
  }
  return out;
}
MccOwnedShaderProjection build(Snapshot source) {
  // The74 matched original red/green controls prove these exact cook-layout
  // changes. Grey uses only the same admitted alpha and first RGB routes.
  std::swap_ranges(source.banks[1].begin(), source.banks[1].begin() + 4,
                   source.banks[1].begin() + 4);
  source.banks[10] = {std::byte{0}, std::byte{0}, std::byte{3}, std::byte{1},
                      std::byte{0}, std::byte{0}, std::byte{4}, std::byte{0}};
  MccOwnedShaderProjection out{std::move(source.root), {}};
  word(out.bytes, 32, 1);
  const auto append = [&](std::span<const std::byte> bytes) {
    const auto at = (out.bytes.size() + 15) & ~std::size_t(15);
    out.bytes.resize(at);
    out.bytes.insert(out.bytes.end(), bytes.begin(), bytes.end());
    return unsigned(at);
  };
  const auto pointer = [&](unsigned field, unsigned target) {
    word(out.bytes, field, target);
    out.pointers.emplace_back(field, target);
  };
  pointer(16, append(source.runtime));
  const auto definition = append(source.definition);
  pointer(36, definition);
  for (unsigned k = 0; k < fields.size(); ++k) {
    const auto field = definition + fields[k] + 4;
    if (source.banks[k].empty())
      word(out.bytes, field, 0);
    else
      pointer(field, append(source.banks[k]));
  }
  const auto overlays = Bytes(out.bytes).u32(definition + 64);
  for (unsigned i = 0; i < source.curves.size(); ++i)
    pointer(overlays + i * 20 + 16, append(source.curves[i]));
  std::set<unsigned> seen;
  for (auto [field, target] : out.pointers) {
    if (field > out.bytes.size() || out.bytes.size() - field < 4 ||
        target < 84 || target >= out.bytes.size() ||
        !seen.insert(field).second || Bytes(out.bytes).u32(field) != target)
      throw FormatError("HUD shader owned relocation invalid");
    for (auto other : seen)
      if (other != field && other < field + 4 && field < other + 4)
        throw FormatError("HUD shader owned relocation overlap");
  }
  if (out.bytes.size() != 676 || out.pointers.size() != 16)
    throw FormatError("HUD shader owned shape changed");
  return out;
}
} // namespace
std::optional<MccOwnedShaderProjection>
project_mcc_grey_widget_flash(const Tag &shader,
                              const MccHudNativeShaderClosure &native,
                              const MccHudShaderReferenceAccess &access) {
  if (shader.group != "shad" ||
      shader.name != "ui\\hud\\shaders\\widget_simple_flash_grey" ||
      shader.size != 524 || !access.source_read || !access.native_read ||
      !access.resolve || !access.string_id)
    return {};
  try {
    Reader reader(access);
    reader.owner(shader);
    reader.owner(native.shader_template);
    reader.owner(native.shader_pass);
    if (native.shader_template.group != "stem" ||
        native.shader_template.name !=
            "shaders\\shader_templates\\hud\\widget_simple_flash" ||
        native.shader_pass.group != "spas" ||
        native.shader_pass.name !=
            "shaders\\shader_passes\\hud\\widget_simple_flash")
      return {};
    const auto root = reader.read(shader.address, 84);
    if (Bytes(root).u32(0) != TagGroup{"stem"}.value ||
        Bytes(root).u32(4) != native.shader_template.datum)
      return {};
    auto source = snapshot(shader, reader);
    static const Json policy = Json::parse(hud_shader_policy);
    const auto fingerprint = digest(source.allocation);
    if (source.logical != policy.at("source") ||
        std::find(policy.at("preimages").begin(), policy.at("preimages").end(),
                  fingerprint) == policy.at("preimages").end())
      return {};
    const auto template_hash = digest(native.template_payload),
               pass_hash = digest(native.pass_payload);
    for (const auto &closure : policy.at("native_closures"))
      if (closure.at("template").at("sha256") == template_hash &&
          closure.at("pass").at("sha256") == pass_hash) {
        reader.native(native.shader_template, native.template_payload,
                      closure.at("template"));
        reader.native(native.shader_pass, native.pass_payload,
                      closure.at("pass"));
        return build(std::move(source));
      }
    return {};
  } catch (const FormatError &) {
    return {};
  }
}
} // namespace halo2
