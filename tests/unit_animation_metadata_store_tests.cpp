#include "halo2/original_data.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
void check(bool passed, const char *message) {
  if (!passed)
    throw std::runtime_error(message);
}
std::uint32_t number(const std::string &s) {
  return std::stoul(s, nullptr, 16);
}
std::vector<std::byte> packet(const std::string &s) {
  check(s.size() % 2 == 0, "Incomplete literal source packet");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < s.size(); i += 2)
    out.push_back(std::byte(number(s.substr(i, 2))));
  return out;
}
template <class F> void rejects(F &&f) {
  bool stopped = false;
  try {
    f();
  } catch (const std::runtime_error &) {
    stopped = true;
  }
  check(stopped, "Missing source data was replaced by a default");
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 7, "Supply map, graph/definition fixtures and base/1.1/u5 images");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i]))
        return 77;
    rejects([] { halo2::UnitAnimationMetadataStore absent(nullptr); });
    halo2::SourceAnimationGraphMemory memory;
    std::function<const halo2::UnitAnimationDefinition &(std::uint32_t,
                                                        std::uint32_t)>
        definition;
    std::weak_ptr<const halo2::Cache> lifetime;
    std::size_t graph_count;
    {
      auto cache = std::make_shared<halo2::Cache>(argv[1]);
      lifetime = cache;
      halo2::UnitAnimationMetadataStore owned(cache);
      graph_count = owned.graph_count();
      memory = owned.graph_memory();
      definition = owned.definition_provider();
      cache.reset();
      check(!lifetime.expired(), "Metadata store did not retain original cache");
    }
    check(!lifetime.expired(), "Providers lost backing after facade destruction");
    std::ifstream graphs(argv[2]);
    unsigned graph_rows = 0, boundaries = 0, definition_rows = 0;
    for (std::string line; std::getline(graphs, line);) {
      std::istringstream row(line);
      std::string kind, datum, address, raw;
      std::size_t size;
      row >> kind;
      if (kind != "G")
        continue;
      check(bool(row >> datum >> address >> size >> raw), "Incomplete original graph row");
      const auto original = packet(raw);
      const auto at = number(address);
      check(memory.graph_address(number(datum)) == at && original.size() == size,
            "Source graph identity/address differs");
      const auto actual = memory.read(at, size);
      check(std::equal(actual.begin(), actual.end(), original.begin(), original.end()),
            "Retained full graph bytes differ from original");
      check(memory.read(at + std::uint32_t(size), 0).empty(), "Empty end span differs");
      rejects([&] { (void)memory.read(at, size + 1); });
      ++boundaries;
      ++graph_rows;
    }
    check(graph_rows == graph_count && graph_rows != 0, "Incomplete original graph coverage");
    std::ifstream definitions(argv[3]);
    std::map<std::pair<std::uint32_t, std::uint32_t>,
             const halo2::UnitAnimationDefinition *> retained;
    for (std::string line; std::getline(definitions, line);) {
      std::istringstream row(line);
      std::string kind, graph, packed, raw;
      row >> kind;
      if (kind != "D")
        continue;
      check(bool(row >> graph >> packed >> raw), "Incomplete original definition row");
      const auto key = std::pair(number(graph), number(packed));
      const auto original = packet(raw);
      const halo2::Bytes bytes(original);
      const auto &actual = definition(key.first, key.second);
      check(actual.frame_count == std::int16_t(bytes.u16(0x14)) &&
                actual.loop_frame == std::int16_t(bytes.u16(0x2e)) &&
                actual.authored_flags == std::to_integer<unsigned>(original.at(0x18)),
            "Retained definition differs from original source scalar fields");
      retained.emplace(key, &actual);
      ++definition_rows;
    }
    check(definition_rows != 0, "No original definition queries");
    for (const auto &[key, pointer] : retained)
      check(&definition(key.first, key.second) == pointer,
            "Further imports invalidated a live definition reference");
    rejects([&] { (void)memory.graph_address(0xffffffffu); });
    rejects([&] { (void)memory.read(0xffffffffu, 1); });
    rejects([&] { (void)definition(0xffffffffu, 0x0000ffffu); });
    auto base = halo2::OriginalData::load_xbe(argv[4]);
    check(!base->unit_animation_constants,
          "Unverified base-image animation words were invented");
    for (int image = 5; image <= 6; ++image) {
      const auto data = halo2::OriginalData::load_xbe(argv[image]);
      check(bool(data->unit_animation_constants), "Verified executable operands unavailable");
      const auto &c = *data->unit_animation_constants;
      check(c.one_bits == 0x3f800000u && c.zero_bits == 0 &&
                c.frame_subtractor_bits == 0x38d1b717u &&
                c.rng_scale_bits == 0x37800080u &&
                c.unsigned_conversion_correction_bits == 0x4f800000u,
            "Imported executable words differ from independent original observations");
    }
    memory = {};
    check(!lifetime.expired(), "Definition callback did not retain source backing");
    definition = {};
    check(lifetime.expired(), "Released callbacks leaked cache backing");
    std::cout << "originalGraphs=" << graph_rows << " originalDefinitions="
              << definition_rows << " stableReferences=" << retained.size()
              << " graphBounds=" << boundaries
              << " providerLifetime=passed supportedImages=2 unavailableBase=passed\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
