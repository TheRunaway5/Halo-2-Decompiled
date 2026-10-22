#include "halo2/gameplay_font_source.hpp"
#include "halo2/hud.hpp"
#include "halo2/mcc_hud_reference.hpp"
#include "halo2/mcc_sound_class_reference.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using Json = nlohmann::json;
namespace {
unsigned checks{}, negatives{};
void check(bool value, const char *message) {
  ++checks;
  if (!value)
    throw FormatError(message);
}
std::vector<std::byte> unhex(const std::string &text) {
  std::vector<std::byte> out;
  for (unsigned i = 0; i < text.size(); i += 2)
    out.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return out;
}
void put(std::span<std::byte> bytes, unsigned at, unsigned value,
         unsigned width = 4) {
  for (unsigned i = 0; i < width; ++i)
    bytes[at + i] = std::byte(value >> (8 * i));
}
Tag tag(const Json &row) {
  return {row.at("group"), row.at("name"), row.at("datum"), row.at("address"),
          row.at("size")};
}
struct Fixture {
  struct Region {
    unsigned address;
    std::vector<std::byte> bytes;
    bool bitmap{};
  };
  std::vector<Region> regions;
  std::vector<Tag> directory;
  std::map<unsigned, std::string> strings;
  unsigned owner_handle{};
  Fixture(const char *filename = "mcc_scoreboard_hud_metadata.json", unsigned index = 0) {
    std::ifstream f(std::filesystem::path(__FILE__).parent_path() /
                    "fixtures" / filename);
    const auto input = Json::parse(f);
    const auto j = input.contains("cases") ? input.at("cases").at(index) : input;
    for (const auto &r : j.at("regions"))
      regions.push_back({r.at("address"), unhex(r.at("hex")), r.value("bitmap",false)});
    for (const auto &r : j.at("directory"))
      directory.push_back(tag(r));
    for (const auto &[id, name] : j.at("strings").items())
      strings.emplace(unsigned(std::stoul(id)), name.get<std::string>());
    owner_handle = j.at("owner").at("datum");
  }
  Tag &owner() {
    return *std::find_if(
        directory.begin(), directory.end(),
        [&](const auto &t) { return t.datum == owner_handle; });
  }
  std::span<std::byte> bytes(unsigned address, std::size_t size) {
    for (auto &r : regions)
      if (address >= r.address &&
          std::uint64_t(address) - r.address + size <= r.bytes.size())
        return std::span(r.bytes).subspan(address - r.address, size);
    throw FormatError("Fixture metadata extent unavailable");
  }
  MccHudReferenceAccess access() {
    return {.read =
                [&](unsigned a, std::size_t n) {
                  return std::span<const std::byte>(bytes(a, n));
                },
            .resolve = [&](TagHandle h) -> std::optional<Tag> {
              for (const auto &t : directory)
                if (t.handle() == h)
                  return t;
              return {};
            },
            .string_id = [&](unsigned id) { return strings.at(id); },
            .directory = directory};
  }
  std::vector<std::vector<std::byte>> snapshot() const {
    std::vector<std::vector<std::byte>> out;
    for (const auto &r : regions)
      out.push_back(r.bytes);
    return out;
  }
};
void portable() {
  Fixture source;
  source.directory.emplace_back();
  const auto before = source.snapshot();
  const auto original = source.owner();
  const auto plan =
      project_mcc_scoreboard_hud_reference(original, source.access());
  check(plan && plan->status == MccGameplayReferenceStatus::Converted &&
            plan->bytes.size() == 2608 && plan->pointers.size() == 56,
        "Complete known metadata did not project");
  check(source.snapshot() == before, "Pure projection changed source bytes");
  std::set<unsigned> fields;
  for (auto [field, target] : plan->pointers) {
    check(field + 4 <= plan->bytes.size() && target < plan->bytes.size() &&
              target >= 40 && fields.insert(field).second,
          "Invalid relocation owner");
    check(Bytes(plan->bytes).u32(field) == target,
          "Stored pointer is not root-relative");
  }
  auto projected = plan->bytes;
  constexpr unsigned new_root = 0x30000000;
  for (auto [field, target] : plan->pointers)
    put(projected, field, new_root + target);
  source.regions.push_back({new_root, std::move(projected)});
  source.owner().address = new_root;
  source.owner().size = unsigned(plan->bytes.size());
  const auto post = source.snapshot();
  const auto again =
      project_mcc_scoreboard_hud_reference(source.owner(), source.access());
  check(again &&
            again->status == MccGameplayReferenceStatus::AlreadyEquivalent &&
            again->bytes.empty() && again->pointers.empty(),
        "Whole postimage not idempotent");
  check(source.snapshot() == post, "Idempotence changed a bank");
  const auto reject = [](auto mutate) {
    Fixture f;
    mutate(f);
    const auto before = f.snapshot();
    const auto r = project_mcc_scoreboard_hud_reference(f.owner(), f.access());
    check(!r || (r->status != MccGameplayReferenceStatus::Converted &&
                 r->status != MccGameplayReferenceStatus::AlreadyEquivalent &&
                 r->bytes.empty() && r->pointers.empty()),
          "Unknown/partial metadata admitted");
    check(f.snapshot() == before, "Rejected metadata was written");
    ++negatives;
  };
  reject([](Fixture &f) { put(f.bytes(f.owner().address, 40), 8, 10); });
  reject([](Fixture &f) { put(f.bytes(f.owner().address, 40), 16, 6); });
  reject([](Fixture &f) { put(f.bytes(f.owner().address, 40), 36, 0x100); });
  reject([](Fixture &f) {
    auto r = f.bytes(f.owner().address, 40);
    put(r, 12, 0xfffffff0);
  });
  reject([](Fixture &f) {
    const Bytes root(f.bytes(f.owner().address, 40));
    auto row = f.bytes(root.u32(12), 100);
    put(row, 36, Bytes(row).u32(36) ^ 0x10000u);
  });
  reject([](Fixture &f) {
    const Bytes root(f.bytes(f.owner().address, 40));
    auto row = f.bytes(root.u32(12), 100);
    put(row, 52, 19, 2);
  });
  reject([](Fixture &f) {
    auto &r = f.regions[0];
    r.bytes.push_back(std::byte{});
    ++f.owner().size;
  });
  reject([](Fixture &f) { f.strings.begin()->second = "wrong"; });
  reject([](Fixture &f) {
    f.directory.push_back(f.directory.back());
    f.directory.back().datum ^= 0x10000u;
  });
  reject([](Fixture &f) { f.regions.erase(f.regions.begin() + 1); });
  reject([](Fixture &f) {
    const Bytes root(f.bytes(f.owner().address, 40));
    auto row = f.bytes(root.u32(12), 100);
    put(row, 88, 17);
  });
  {
    auto bad = source;
    auto bytes = bad.bytes(new_root, 40);
    put(bytes, 24, 1, 2);
    const auto r =
        project_mcc_scoreboard_hud_reference(bad.owner(), bad.access());
    check(r &&
              r->status ==
                  MccGameplayReferenceStatus::AuthoredTupleUnrecognized &&
              r->bytes.empty(),
          "Partial postimage admitted");
    ++negatives;
  }
  constexpr std::array<std::pair<std::string_view, unsigned>, 15> operands{
      {{"hud_elite", 0x090005ce},
       {"hud_input1", 0x0a0005cf},
       {"hud_input2", 0x0a0005d0},
       {"hud_input3", 0x0a0005d1},
       {"hud_input4", 0x0a0005d2},
       {"hud_wp_alpha", 0x0c0005cb},
       {"hud_wp_theta", 0x0c0005cc},
       {"hud_wp_os_amt", 0x0d0005ca},
       {"hud_wp_arrow_t", 0x0e0005cd},
       {"emblem_active", 0x0d0005ff},
       {"emblem_foreground_index", 0x170005fa},
       {"emblem_background_index", 0x170005fb},
       {"emblem_toggle_foreground", 0x180005fc},
       {"emblem_foreground_map_scale_x", 0x1d0005fd},
       {"emblem_background_map_scale_x", 0x1d0005fe}}};
  for (const auto &[name, expected] : operands) {
    const auto actual = (unsigned(name.size()) << 24) | 0x1234u;
    check(normalize_mcc_hud_function_operand(actual, name) == expected,
          "Named original function operand changed");
    bool threw = false;
    try {
      normalize_mcc_hud_function_operand(expected,
                                         std::string(name.size(), 'x'));
    } catch (const FormatError &) {
      threw = true;
    }
    check(threw, "Unknown colliding HUD operand admitted");
    ++negatives;
  }
  check(normalize_mcc_hud_function_operand(0x06001234, "custom") == 0x06001234,
        "Unknown noncolliding default changed");
  check(normalize_mcc_hud_function_operand(0, "") == 0,
        "Empty default changed");
}
void portable_geometry() {
  constexpr const char *filename = "mcc_hud_geometry_metadata.json";
  for (unsigned fixture = 0; fixture < 3; ++fixture) {
    Fixture source(filename, fixture);
    const auto before = source.snapshot();
    const auto root = source.owner();
    const Bytes original(source.bytes(root.address, 40));
    const auto plan = project_mcc_hud_reference(root, source.access());
    check(plan && plan->status == MccGameplayReferenceStatus::Converted,
          "Canonical non-scoreboard HUD geometry was not projected");
    check(source.snapshot() == before, "HUD geometry projection changed source storage");
    const Bytes result(plan->bytes);
    check(result.u32(0) == original.u32(0) && result.u32(4) == original.u32(4),
          "HUD geometry projection changed its typed parent");
    for (unsigned kind = 0; kind < 3; ++kind) {
      const unsigned descriptor = kind == 0 ? 8 : kind == 1 ? 16 : 32;
      const unsigned stride = kind == 0 ? 100 : kind == 1 ? 84 : 80;
      const unsigned ep = kind == 0 ? 88 : kind == 1 ? 76 : 72;
      const auto count = original.u32(descriptor);
      check(result.u32(descriptor) == count,
            "HUD geometry clone dropped active rows or a screen bank");
      if (!count) {
        check(!result.u32(descriptor + 4), "Empty HUD bank acquired a nonnull pointer");
        continue;
      }
      for (unsigned i = 0; i < count; ++i) {
        const Bytes a(source.bytes(original.u32(descriptor + 4) + i * stride, stride));
        const auto row = result.u32(descriptor + 4) + i * stride;
        check(result.u32(row) == a.u32(0), "HUD geometry clone changed a raw StringID");
        for (unsigned field : kind == 0 ? std::vector<unsigned>{36, 44}
                             : kind == 1 ? std::vector<unsigned>{36, 40}
                                         : std::vector<unsigned>{36, 44, 52, 60})
          check(result.u32(row + field) == a.u32(field),
                "HUD geometry clone changed a typed reference or text ID");
        check(result.u32(row + ep) == a.u32(ep), "HUD geometry clone dropped effects");
        for (unsigned e = 0; e < a.u32(ep); ++e) {
          const Bytes old_effect(source.bytes(a.u32(ep + 4) + e * 104, 104));
          const auto effect = result.u32(row + ep + 4) + e * 104;
          check(result.u32(effect) == old_effect.u32(0), "HUD effect flags changed");
          for (unsigned k = 0; k < 5; ++k) {
            const auto q = 4 + k * 20;
            for (unsigned field : {q, q + 4, q + 8, q + 12})
              check(result.u32(effect + field) == old_effect.u32(field),
                    "HUD function IDs, period or size changed");
            const auto size = old_effect.u32(q + 12);
            if (size)
              check(std::ranges::equal(result.slice(result.u32(effect + q + 16), size),
                    source.bytes(old_effect.u32(q + 16), size)), "Cloned HUD function bytes changed");
            else
              check(!result.u32(effect + q + 16), "Empty HUD function acquired storage");
          }
        }
      }
    }
    auto bytes = plan->bytes;
    constexpr unsigned new_root = 0x31000000;
    for (const auto [field, target] : plan->pointers)
      put(bytes, field, new_root + target);
    // Referenced bitmaps remain typed Cache owners; nhdt rows/effects/curves
    // must all survive after every original nhdt region has been released.
    std::erase_if(source.regions,[](const auto &r){return !r.bitmap;});
    source.regions.push_back({new_root, std::move(bytes)});
    source.owner().address = new_root;
    source.owner().size = unsigned(plan->bytes.size());
    const auto after = source.snapshot();
    const auto again = project_mcc_hud_reference(source.owner(), source.access());
    check(again && again->status == MccGameplayReferenceStatus::AlreadyEquivalent &&
              again->bytes.empty() && again->pointers.empty(),
          "Complete relocated HUD geometry did not admit its measured postimage");
    check(source.snapshot() == after, "Idempotent HUD geometry changed storage");
    const auto reject = [&](auto mutate) {
      Fixture f(filename, fixture);
      mutate(f);
      const auto state = f.snapshot();
      const auto r = project_mcc_hud_reference(f.owner(), f.access());
      check(!r || (r->status == MccGameplayReferenceStatus::AuthoredTupleUnrecognized &&
                   r->bytes.empty() && r->pointers.empty()), "Foreign HUD geometry tuple was admitted");
      check(f.snapshot() == state, "Rejected HUD geometry changed storage");
      ++negatives;
    };
    reject([](Fixture &f) { auto b=f.bytes(f.owner().address,40); put(b,24,1,2); });
    reject([](Fixture &f) { auto b=f.bytes(f.owner().address,f.owner().size); b.back()^=std::byte{1}; });
    reject([](Fixture &f) { f.directory.push_back(f.owner()); f.directory.back().datum^=0x10000u; });
    reject([](Fixture &f) {
      const Bytes r(f.bytes(f.owner().address,40));
      auto b=f.bytes(r.u32(12),100); put(b,52,Bytes(b).u16(52)^1u,2);
    });
    reject([](Fixture &f) {
      const Bytes r(f.bytes(f.owner().address,40));
      auto b=f.bytes(r.u32(12),100); put(b,36,Bytes(b).u32(36)^0x10000u);
    });
    reject([](Fixture &f) { f.strings.begin()->second="invalid-name"; });
    reject([](Fixture &f) {
      const auto tag=std::find_if(f.directory.begin(),f.directory.end(),[](const auto &t){return t.group=="bitm";});
      check(tag!=f.directory.end(),"Missing bitmap guard fixture");
      const Bytes root(f.bytes(tag->address,76));
      auto frame=f.bytes(root.u32(72),168);
      put(frame,4,Bytes(frame).u16(4)^1u,2);
    });
    reject([](Fixture &f) {
      const auto tag=std::find_if(f.directory.begin(),f.directory.end(),[](const auto &t){return t.group=="bitm";});
      const Bytes root(f.bytes(tag->address,76));
      check(root.u32(60)>0,"Missing sequence guard fixture");
      const Bytes sequence(f.bytes(root.u32(64),60));
      if(sequence.u32(52)) {
        auto sprite=f.bytes(sequence.u32(56),32);
        put(sprite,24,Bytes(sprite).u32(24)^1u);
      } else {
        auto row=f.bytes(root.u32(64),60);
        put(row,34,Bytes(row).u16(34)^1u,2);
      }
    });
  }
}
MccHudReferenceAccess access(Cache &cache) {
  return {.read = [&](unsigned a,
                      std::size_t n) { return cache.read_tag_bytes(a, n); },
          .resolve = [&](TagHandle h) -> std::optional<Tag> {
            const auto *t = cache.find_tag(h);
            return t ? std::optional<Tag>{*t} : std::nullopt;
          },
          .string_id = [&](unsigned id) { return cache.string_id(id); },
          .directory = cache.tags()};
}
float value(unsigned id, float input) {
  if (id == 0x0a0005d0)
    return input;
  return 1.f;
}
Json actual(const char *donor, const char *original, std::string_view mode) {
  const bool constructors = mode == "--constructors";
  Cache mcc(donor, {}, OriginalData::native_tu15()),
      og(original, {}, OriginalData::native_tu15());
  const auto *owner = mcc.query_tag({.group = TagGroup{"nhdt"},
                                     .name = "ui\\hud\\scoreboard",
                                     .require_address = true});
  if (!owner)
    return {{"owner_absent", true}};
  const auto plan = project_mcc_scoreboard_hud_reference(*owner, access(mcc));
  check(bool(plan), "Actual source profile absent");
  const bool converted = plan->status == MccGameplayReferenceStatus::Converted;
  if (converted) {
    // This detached region models publication without touching any Cache bytes,
    // including source allocations that other HUD definitions may still borrow.
    constexpr unsigned detached_root = 0x70000000;
    auto bytes = plan->bytes;
    for (auto [field, target] : plan->pointers)
      put(bytes, field, detached_root + target);
    auto detached_owner = *owner;
    detached_owner.address = detached_root;
    detached_owner.size = unsigned(bytes.size());
    auto directory = mcc.tags();
    for (auto &entry : directory)
      if (entry.datum == detached_owner.datum)
        entry = detached_owner;
    auto detached_access = access(mcc);
    detached_access.read = [&](unsigned address, std::size_t size) {
      if (address >= detached_root &&
          std::uint64_t(address) - detached_root + size <= bytes.size())
        return std::span<const std::byte>(bytes).subspan(
            address - detached_root, size);
      return mcc.read_tag_bytes(address, size);
    };
    detached_access.resolve = [&](TagHandle handle) -> std::optional<Tag> {
      if (handle == detached_owner.handle())
        return detached_owner;
      const auto *tag = mcc.find_tag(handle);
      return tag ? std::optional<Tag>{*tag} : std::nullopt;
    };
    detached_access.directory = directory;
    const auto post =
        project_mcc_scoreboard_hud_reference(detached_owner, detached_access);
    check(post && post->status == MccGameplayReferenceStatus::AlreadyEquivalent,
          "Actual detached postimage not complete");
  } else {
    check(plan->status == MccGameplayReferenceStatus::AlreadyEquivalent,
          "Actual source not canonical");
  }
  if (mode == "--profile-only")
    return {{"converted", converted},
            {"projection_bytes", plan->bytes.size()},
            {"whole_postimage", true},
            {"cache_bytes_modified", false}};
  check(!converted,
        "Normal Cache clone hook required for actual HUD consumer checks");
  const auto *a =
      mcc.query_tag({.group = tag_groups::biped,
                     .name = "objects\\characters\\masterchief\\masterchief_mp",
                     .require_address = true});
  const auto *b =
      og.query_tag({.group = tag_groups::biped,
                    .name = "objects\\characters\\masterchief\\masterchief_mp",
                    .require_address = true});
  check(a && b, "Missing paired player biped");
  const auto mh = mcc.load_hud(a->datum, null_datum),
             oh = og.load_hud(b->datum, null_datum);
  std::set<std::pair<bool, std::string>> names;
  std::ifstream fixture(std::filesystem::path(__FILE__).parent_path() /
                        "fixtures/mcc_scoreboard_hud_metadata.json");
  auto logical = Json::parse(fixture).at("original_logical");
  for (const auto &kind : {"bitmap", "text"})
    for (const auto &row : logical[kind])
      names.emplace(std::string_view(kind) == "text",
                    row.at("name").get<std::string>());
  unsigned dynamic{};
  for (const auto &[text, name] : names) {
    const auto find = [&](const HudContent &h) -> const HudWidget & {
      const auto i =
          std::find_if(h.widgets.begin(), h.widgets.end(), [&](const auto &w) {
            return w.is_text == text && w.name == name;
          });
      check(i != h.widgets.end(), "Original scoreboard widget absent");
      return *i;
    };
    const auto &x = find(mh);
    const auto &y = find(oh);
    check(x.inputs == y.inputs && x.yes == y.yes && x.no == y.no &&
              x.cutoffs == y.cutoffs && x.anchor == y.anchor &&
              x.flags == y.flags && x.special == y.special &&
              x.sequence == y.sequence && x.fonts == y.fonts &&
              x.justification == y.justification,
          "Scoreboard original logical fields differ");
    for (unsigned i = 0; i < 3; ++i)
      check(x.offset[i].x == y.offset[i].x && x.offset[i].y == y.offset[i].y &&
                x.registration[i].x == y.registration[i].x &&
                x.registration[i].y == y.registration[i].y,
            "Original scoreboard geometry operands differ");
    check(x.effects.size() == y.effects.size(),
          "Original effect count differs");
    for (unsigned e = 0; e < x.effects.size(); ++e) {
      check(x.effects[e].flags == y.effects[e].flags,
            "Original effect flags differ");
      for (unsigned k = 0; k < 5; ++k) {
        const auto &f = x.effects[e].functions[k];
        const auto &g = y.effects[e].functions[k];
        check(f.input == g.input && f.range == g.range &&
                  f.period == g.period && f.curve == g.curve,
              "Original detached dynamic function differs");
        for (float input : {0.f, .125f, .25f, .5f, .75f, .875f, 1.f}) {
          const auto tables = mcc.shader_function_tables();
          check(std::bit_cast<unsigned>(
                    shader_function_value(f.curve, value(f.input, input),
                                          value(f.range, input), *tables)) ==
                    std::bit_cast<unsigned>(
                        shader_function_value(g.curve, value(g.input, input),
                                              value(g.range, input), *tables)),
                "Dynamic original curve output differs");
          ++dynamic;
        }
      }
    }
  }
  for (const auto &w : mh.widgets)
    check(w.name.find("text_chat_") != 0,
          "MCC-only chat widget remains vanilla");
  Json gaps = Json::array();
  unsigned constructed{};
  const auto run = [&](HudContent h, const std::string &context,
                       unsigned hand = 0) {
    try {
      Mesh mesh;
      MultiplayerHud hud(
          std::move(h), gameplay_font_directory(mcc), mesh, 1, "Slayer",
          mcc.original_data()->hud_bitmap_geometry_constants, {}, hand);
      ++constructed;
    } catch (const FormatError &e) {
      gaps.push_back({{"context", context}, {"error", e.what()}});
    }
  };
  if (constructors) {
    std::set<unsigned> bipeds, weapons, vehicle_weapons, parent_huds;
    const auto *globals = mcc.find_tag(mcc.globals_tag(), tag_groups::globals);
    const Bytes d(mcc.read_tag_bytes(globals->address + 0x138, 8));
    for (unsigned i = 0; i < d.u32(0); ++i) {
      const auto id =
          Bytes(mcc.read_tag_bytes(d.u32(4) + i * 0xbc + 0xb4, 4)).u32(0);
      if (mcc.find_tag({id}, tag_groups::biped))
        bipeds.insert(id);
    }
    for (const auto &t : mcc.tags())
      if (t.address && t.group == "weap" && t.size >= 0x2b8 &&
          Bytes(mcc.read_tag_bytes(t.address + 0x2b4, 4)).u32(0) != null_datum)
        weapons.insert(t.datum);
    for (const auto &t : mcc.tags())
      if (t.address && (t.group == "bipd" || t.group == "vehi")) {
        for (auto id : mcc.load_unit_seat_huds(t.datum))
          parent_huds.insert(id);
        const Bytes d(mcc.read_tag_bytes(t.address + 0x1c0, 8));
        if (d.u32(0) > 4096)
          throw FormatError("Vehicle fixture count overflow");
        for (unsigned n = 0; n < d.u32(0); ++n) {
          const auto id =
              Bytes(mcc.read_tag_bytes(d.u32(4) + n * 8 + 4, 4)).u32(0);
          if (id != null_datum)
            vehicle_weapons.insert(id);
        }
      }
    for (auto id : bipeds) {
      auto base = mcc.load_hud(id, null_datum);
      std::erase_if(base.widgets,
                    [](const auto &w) { return w.weapon_type == 1; });
      run(std::move(base), "unit:" + std::to_string(id));
      for (auto weapon : weapons) {
        auto h = mcc.load_hud(id, weapon);
        std::erase_if(h.widgets,
                      [](const auto &w) { return w.weapon_type != 1; });
        h.action_messages.enabled = false;
        run(std::move(h),
            "handheld:" + std::to_string(id) + ":" + std::to_string(weapon));
      }
      for (auto weapon : vehicle_weapons) {
        auto h = mcc.load_hud(id, weapon);
        std::erase_if(h.widgets,
                      [](const auto &w) { return w.weapon_type != 1; });
        for (auto &w : h.widgets)
          w.weapon_type = 4;
        h.action_messages.enabled = false;
        run(std::move(h),
            "vehicle:" + std::to_string(id) + ":" + std::to_string(weapon), 2);
      }
      for (auto parent : parent_huds)
        run(mcc.load_hud_definition(id, parent),
            "parent:" + std::to_string(id) + ":" + std::to_string(parent));
    }
  }
  return {{"converted", converted},
          {"projection_bytes", plan->bytes.size()},
          {"paired_scoreboard_widgets", names.size()},
          {"dynamic_curve_outputs", dynamic},
          {"constructors_passed", constructed},
          {"constructor_gaps", gaps},
          {"mcc_multiplayer_scale", mh.multiplayer_scale},
          {"original_multiplayer_scale", oh.multiplayer_scale},
          {"logical_bitmap_extent_or_pixel_parity_claim", false}};
}
Json actual_geometry(const char *donor, const char *original) {
  Cache mcc(donor, {}, OriginalData::native_tu15()),
      og(original, {}, OriginalData::native_tu15());
  bool class_namespace{};
  if (mcc.find_tag(mcc.scenario_tag(), tag_groups::scenario)) {
    const auto *globals = mcc.find_tag(mcc.globals_tag(), tag_groups::globals);
    check(globals, "HUD publication lost the actual globals owner");
    const Bytes block(mcc.read_tag_bytes(globals->address + 0xc0, 8));
    check(block.u32(0) == 1, "HUD publication changed the sound-globals route");
    const Bytes sound(mcc.read_tag_bytes(block.u32(4), 44));
    const TagHandle legacy{sound.u32(12)};
    const auto *classes = mcc.find_tag(legacy, TagGroup{"sncl"});
    check(classes, "HUD publication lost the actual Legacy class owner");
    const Bytes root(mcc.read_tag_bytes(classes->address, 8));
    check(root.u32(0) == 54, "HUD publication changed the Legacy class count");
    const auto before = project_mcc_sound_class_records(
        mcc.read_tag_bytes(root.u32(4), 54 * 120));
    const MccSoundRuntimeSource source{
        .tags = mcc.tags(),
        .read = [&](auto a, auto n) { return mcc.read_tag_bytes(a, n); }};
    const auto projected = project_mcc_sound_class_reference(source, legacy, before);
    check(projected.status == MccSoundClassReferenceStatus::Converted ||
              projected.status == MccSoundClassReferenceStatus::AlreadyEquivalent,
          "HUD publication moved the authenticated native sound namespace");
    class_namespace = true;
  }
  unsigned owners{}, fields{}, screens{};
  for (const auto &tag : mcc.select_tags({.group=TagGroup{"nhdt"},.require_address=true})) {
    if (tag.name=="ui\\hud\\scoreboard")
      continue;
    const auto plan=project_mcc_hud_reference(tag,access(mcc));
    if (!plan || plan->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized)
      continue;
    check(plan->status==MccGameplayReferenceStatus::AlreadyEquivalent && plan->bytes.empty(),
          "Normal Cache failed to publish its canonical HUD geometry owner");
    const auto *reference=og.query_tag({.group=TagGroup{"nhdt"},.name=tag.name,.require_address=true});
    check(reference,"Missing Original HUD geometry counterpart");
    const Bytes a(mcc.tag_payload(tag.handle())),b(og.tag_payload(reference->handle()));
    for (unsigned kind=0;kind<2;++kind) {
      const unsigned descriptor=kind==0?8:16,stride=kind==0?100:84;
      check(a.u32(descriptor)==b.u32(descriptor),"Canonical HUD geometry changed row count");
      for (unsigned i=0;i<a.u32(descriptor);++i) {
        const Bytes ar(mcc.read_tag_bytes(a.u32(descriptor+4)+i*stride,stride)),
                    br(og.read_tag_bytes(b.u32(descriptor+4)+i*stride,stride));
        check(mcc.string_id(ar.u32(0))==og.string_id(br.u32(0)),"Canonical HUD row ordinal/name changed");
        const unsigned offset=kind==0?52:64;
        for(unsigned n=0;n<6;++n) {
          check(ar.u16(offset+n*2)==br.u16(offset+n*2),"Canonical HUD offset differs from Original");
          ++fields;
        }
        if(kind==0)for(unsigned n=0;n<6;++n) {
          check(ar.u32(64+n*4)==br.u32(64+n*4),"Canonical HUD registration differs from Original");
          ++fields;
        }
      }
    }
    screens+=a.u32(32);
    ++owners;
  }
  check(owners>0,"No canonical HUD geometry owners reached in actual Cache");
  return {{"canonical_owners",owners},{"original_geometry_fields",fields},
          {"retained_screen_rows",screens},{"normal_cache_publication",true},
          {"native_sound_class_namespace",class_namespace},
          {"bitmap_font_pixel_or_runtime_render_parity_claim",false}};
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 1 && argc != 3 && argc != 4)
    return 2;
  if (argc == 4 && std::string_view(argv[3]) != "--constructors" &&
      std::string_view(argv[3]) != "--profile-only" && std::string_view(argv[3]) != "--geometry")
    return 2;
  if (argc >= 3 &&
      (!*argv[1] || !*argv[2] || !std::filesystem::is_regular_file(argv[1]) ||
       !std::filesystem::is_regular_file(argv[2])))
    return 77;
  portable();
  portable_geometry();
  Json out{{"checks", checks}, {"negative_cases", negatives}};
  if (argc >= 3)
    out["actual"] =
        argc==4 && std::string_view(argv[3])=="--geometry" ? actual_geometry(argv[1],argv[2]) : actual(argv[1], argv[2],
               argc == 4 ? std::string_view(argv[3]) : std::string_view{});
  out["checks"] = checks;
  std::cout << out.dump(2) << '\n';
  return out.contains("actual") &&
                 !out["actual"].value("constructor_gaps", Json::array()).empty()
             ? 1
             : 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
