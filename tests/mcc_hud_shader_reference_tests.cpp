#include "halo2/cache.hpp"
#include "halo2/mcc_hud_shader_reference.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json = nlohmann::json;
namespace {
unsigned checks{}, negatives{};
void check(bool b, const char *m) {
  ++checks;
  if (!b)
    throw FormatError(m);
}
std::vector<std::byte> file(const std::filesystem::path &p) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f || f.tellg() < 0)
    throw FormatError("Missing retained native shader payload");
  std::vector<std::byte> b(std::size_t(f.tellg()));
  f.seekg(0);
  if (!b.empty() && !f.read(reinterpret_cast<char *>(b.data()), b.size()))
    throw FormatError("Truncated retained native shader payload");
  return b;
}
std::vector<std::byte> unhex(const std::string &text) {
  std::vector<std::byte> out;
  for (unsigned i = 0; i < text.size(); i += 2)
    out.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return out;
}
void put(std::vector<std::byte> &b, unsigned q, unsigned v) {
  for (unsigned k = 0; k < 4; ++k)
    b.at(q + k) = std::byte(v >> (8 * k));
}
struct Fixture {
  Cache &cache;
  std::vector<Tag> directory;
  Tag source;
  MccHudNativeShaderClosure native;
  std::vector<std::byte> tp, pp, scratch;
  std::map<unsigned, std::vector<std::byte>> overrides;
  bool corrupt_native{}, wrong_sid{};
  explicit Fixture(Cache &c) : cache(c), directory(c.tags()) {
    const auto *t =
        c.query_tag({.group = TagGroup{"shad"},
                     .name = "ui\\hud\\shaders\\widget_simple_flash_grey",
                     .require_address = true});
    check(t, "Actual grey shader unavailable");
    source = *t;
    const auto *a = c.query_tag(
        {.group = TagGroup{"stem"},
         .name = "shaders\\shader_templates\\hud\\widget_simple_flash",
         .require_address = true});
    const auto *b =
        c.query_tag({.group = TagGroup{"spas"},
                     .name = "shaders\\shader_passes\\hud\\widget_simple_flash",
                     .require_address = true});
    check(a && b, "Actual retained native HUD closure unavailable");
    native.shader_template = *a;
    native.shader_pass = *b;
    const auto root = c.shared_directory().parent_path().parent_path() /
                      "native-shaders" / c.name();
    std::ifstream f(root / "library.json");
    const auto bank = Json::parse(f);
    for (const auto &r : bank.at("shader_tags")) {
      if (r["group"] == "stem" && r["name"] == a->name)
        tp = file(root / "payloads" / r["payload"].get<std::string>());
      if (r["group"] == "spas" && r["name"] == b->name)
        pp = file(root / "payloads" / r["payload"].get<std::string>());
    }
    native.template_payload = tp;
    native.pass_payload = pp;
    // The native conversion retains the authenticated MCC source owner. These
    // exact source handles/roots are fixture identities, never a pointer
    // search.
    if (source.size == 676) {
      if (source.datum == 4068692854u)
        source.address = 18072100u;
      else if (source.datum == 2260739209u)
        source.address = 30283812u;
      else
        throw FormatError("Unknown retained grey source identity");
      source.size = 524;
      for (auto &t : directory)
        if (t.datum == source.datum)
          t = source;
    }
  }
  std::vector<std::byte> &copy(unsigned address, unsigned size) {
    auto i = overrides.find(address);
    if (i == overrides.end()) {
      const auto b = cache.read_tag_bytes(address, size);
      i = overrides.emplace(address, std::vector<std::byte>(b.begin(), b.end()))
              .first;
    }
    return i->second;
  }
  MccHudShaderReferenceAccess access() {
    return {.source_read =
                [&](unsigned address, std::size_t size) {
                  const auto original = cache.read_tag_bytes(address, size);
                  scratch.assign(original.begin(), original.end());
                  for (const auto &[start, b] : overrides) {
                    const auto first = std::max<std::uint64_t>(address, start);
                    const auto last = std::min<std::uint64_t>(
                        std::uint64_t(address) + size,
                        std::uint64_t(start) + b.size());
                    if (first < last)
                      std::copy(b.begin() + (first - start),
                                b.begin() + (last - start),
                                scratch.begin() + (first - address));
                  }
                  return std::span<const std::byte>(scratch);
                },
            .native_read =
                [&](unsigned address, std::size_t size) {
                  if (corrupt_native) {
                    auto &b = copy(address, unsigned(size));
                    b[0] ^= std::byte{1};
                    return std::span<const std::byte>(b);
                  }
                  return cache.read_tag_bytes(address, size);
                },
            .resolve = [&](TagHandle h) -> std::optional<Tag> {
              for (const auto &t : directory)
                if (t.handle() == h)
                  return t;
              return {};
            },
            .string_id =
                [&](unsigned id) {
                  return wrong_sid && id ? std::string("unsupported")
                                         : cache.string_id(id);
                },
            .directory = directory};
  }
};
Json actual(const char *path) {
  Cache cache(path);
  Fixture f(cache);
  unsigned retained_regions{}, retained_bytes{};
  std::ifstream policy_file(std::filesystem::path(__FILE__).parent_path()
                               .parent_path() /
                           "tools/data/mcc-hud-shader-reference.json");
  const auto metadata = Json::parse(policy_file).at("portable_metadata_fixture");
  if (f.source.datum == metadata.at("source_tag").at("datum"))
    for (const auto &region : metadata.at("regions")) {
      const auto expected = unhex(region.at("hex"));
      const auto bytes = cache.read_tag_bytes(region.at("address"), expected.size());
      check(std::equal(bytes.begin(), bytes.end(), expected.begin()),
            "Retained raw MCC source or borrowed metadata changed");
      ++retained_regions;
      retained_bytes += unsigned(bytes.size());
    }
  const auto before = cache.read_tag_bytes(f.source.address, f.source.size);
  const std::vector<std::byte> saved(before.begin(), before.end());
  const auto result =
      project_mcc_grey_widget_flash(f.source, f.native, f.access());
  check(result && result->bytes.size() == 676 && result->pointers.size() == 16,
        "Canonical grey material did not project");
  check(std::equal(before.begin(), before.end(), saved.begin()),
        "Pure projection changed source");
  const Bytes root(result->bytes);
  const auto d = root.u32(36);
  check(root.u32(32) == 1, "Original postprocess count not restored");
  const Bytes definition(
      std::span<const std::byte>(result->bytes).subspan(d, 124));
  const auto pc = definition.u32(16);
  check(Bytes(result->bytes).u32(pc) == 0 &&
            Bytes(result->bytes).u32(pc + 4) == 0x0075baff &&
            Bytes(result->bytes).u32(pc + 8) == 0x00c0c0c0,
        "Authored grey/static constants changed");
  const auto maps = definition.u32(88);
  check(Bytes(result->bytes).u32(maps) == 0x01030000 &&
            Bytes(result->bytes).u32(maps + 4) == 0x00040000,
        "Original parameter routes changed");
  for (auto [field, target] : result->pointers)
    check(field + 4 <= result->bytes.size() && target < result->bytes.size() &&
              Bytes(result->bytes).u32(field) == target,
          "Owned pointer escaped");
  const auto *p = cache.find_tag(f.source.handle());
  bool published = p && p->size == 676;
  if (published) {
    auto expected = result->bytes;
    for (auto [field, target] : result->pointers)
      put(expected, field, p->address + target);
    const auto got = cache.read_tag_bytes(p->address, p->size);
    check(std::equal(got.begin(), got.end(), expected.begin()),
          "Actual normal Cache postimage differs");
    check(!project_mcc_grey_widget_flash(*p, f.native, f.access()),
          "Already converted owner projected twice");
  }
  const auto reject = [&](auto mutate) {
    Fixture n(cache);
    mutate(n);
    const auto r =
        project_mcc_grey_widget_flash(n.source, n.native, n.access());
    ++checks;
    if (r)
      throw FormatError("Unknown/custom HUD material admitted: case " +
                        std::to_string(negatives));
    check(std::equal(before.begin(), before.end(), saved.begin()),
          "Rejected source was changed");
    ++negatives;
  };
  reject([](Fixture &n) {
    auto &b = n.copy(n.source.address, n.source.size);
    b[8] ^= std::byte{1};
  });
  reject([](Fixture &n) {
    auto &b = n.copy(n.source.address, n.source.size);
    put(b, 32, 1);
  });
  reject([](Fixture &n) {
    const Bytes root(n.copy(n.source.address, n.source.size));
    auto &definitions = n.copy(root.u32(36), 372);
    put(definitions, 0, Bytes(definitions).u32(0) ^ 0x10000u);
  });
  reject([](Fixture &n) {
    const Bytes root(n.copy(n.source.address, n.source.size));
    put(n.copy(root.u32(36), 372), 124, 0xffffffffu);
  });
  reject([](Fixture &n) {
    auto &b = n.copy(n.source.address, n.source.size);
    const auto d = Bytes(b).u32(36);
    auto &defs = n.copy(d, 372);
    const auto pc = Bytes(defs).u32(16);
    n.copy(pc, 12)[8] ^= std::byte{1};
  });
  reject([](Fixture &n) {
    auto &b = n.copy(n.source.address, n.source.size);
    const auto d = Bytes(b).u32(36);
    auto &defs = n.copy(d, 372);
    const auto a = Bytes(defs).u32(64);
    auto &overlays = n.copy(a, 40);
    const auto curve = Bytes(overlays).u32(16);
    n.copy(curve, 20)[4] ^= std::byte{1};
  });
  reject([](Fixture &n) { n.tp[0] ^= std::byte{1}; });
  reject([](Fixture &n) { n.pp[100] ^= std::byte{1}; });
  reject([](Fixture &n) { n.corrupt_native = true; });
  reject([](Fixture &n) { n.wrong_sid = true; });
  reject([](Fixture &n) {
    n.directory.push_back(n.native.shader_template);
    n.directory.back().datum ^= 0x10000;
  });
  reject([](Fixture &n) { n.native.shader_pass.datum ^= 0x10000; });
  reject([](Fixture &n) {
    n.native.shader_pass.address = n.native.shader_template.address;
  });
  reject([](Fixture &n) { n.source.size = 523; });
  return {{"map", cache.name()},
          {"projection_bytes", result->bytes.size()},
          {"relocations", result->pointers.size()},
          {"normal_cache_published", published},
          {"complete_postimage_equal", published},
          {"source_bytes_unchanged", true},
          {"retained_raw_regions", retained_regions},
          {"retained_raw_bytes", retained_bytes}};
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 1 && argc != 2)
    return 2;
  if (argc == 2 && (!*argv[1] || !std::filesystem::is_regular_file(argv[1])))
    return 77;
  check(!project_mcc_grey_widget_flash({}, {}, {}), "Empty source admitted");
  Tag t{"shad", "custom", 1, 1, 524};
  check(!project_mcc_grey_widget_flash(t, {}, {}), "Unrelated source admitted");
  Json out;
  if (argc == 2)
    out["actual"] = actual(argv[1]);
  out["checks"] = checks;
  out["negative_cases"] = negatives;
  std::cout << out.dump(2) << '\n';
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
