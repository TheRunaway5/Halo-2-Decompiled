#include "halo2/content.hpp"
#include "halo2/source_ai_references.hpp"
#include "halo2/source_ai_squad_team.hpp"
#include "halo2/tags/binary.hpp"
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool result, const char *message) {
  if (!result)
    throw std::runtime_error(message);
}
std::span<const std::byte> block_bytes(const Cache &cache, TagBlock block,
                                     std::size_t stride,
                                     std::size_t capacity = 65536) {
  const auto bytes = block.extent(stride, capacity);
  return bytes ? cache.read_tag_bytes(block.address, bytes)
               : std::span<const std::byte>{};
}
std::pair<std::uint32_t, int> general(const Cache &cache, std::uint32_t datum) {
  unsigned visited = 0;
  while (datum != null_datum) {
    check(++visited <= 32, "Character fixture inheritance is cyclic/deep");
    check(cache.find_tag({datum}, tag_groups::character) != nullptr,
          "Character fixture tag is absent");
    const Bytes data(cache.tag_payload({datum}));
    const auto block = read_tag_block_descriptor(data, 0x34);
    if (block.count) {
      const Bytes properties(block_bytes(cache, block, 0xc));
      return {properties.u32(0), std::int16_t(properties.u16(4))};
    }
    datum = data.u32(8);
  }
  return {0, -1};
}
}
int main(int argc, char **argv) try {
  check(argc == 4, "Supply MAP SHARED XBE");
  Cache cache(argv[1], argv[2], OriginalData::load_xbe(argv[3]));
  const auto content = cache.load_content(0);
  const Bytes scenario(cache.tag_payload(cache.scenario_tag()));
  const auto group_block = read_tag_block_descriptor(scenario, 0x158);
  const auto squad_block = read_tag_block_descriptor(scenario, 0x160);
  const auto palette_block = read_tag_block_descriptor(scenario, 0x178);
  const Bytes palette(block_bytes(cache, palette_block, 8));
  const Bytes groups(block_bytes(cache, group_block, 0x24, 100));
  const Bytes squads(block_bytes(cache, squad_block, 0x74, 335));
  check(content.squad_group_parents.size() == std::size_t(group_block.count) &&
            content.squads.size() == std::size_t(squad_block.count),
        "Canonical AI import dropped scenario records");
  for (unsigned i = 0; i < content.squad_group_parents.size(); ++i)
    check(content.squad_group_parents[i] == std::int16_t(groups.u16(i*0x24+0x20)),
          "Canonical AI import changed an authored group parent");
  unsigned starts = 0, characters = 0;
  for (unsigned i = 0; i < content.squads.size(); ++i) {
    const auto &squad = content.squads[i];
    check(squad.parent_group == std::int16_t(squads.u16(i*0x74+0x26)),
          "Canonical AI import changed an authored squad parent");
    const auto authored_team = std::to_integer<std::uint8_t>(squads.slice(i*0x74+0x24, 1)[0]);
    const auto palette_index = std::int16_t(squads.u16(i*0x74+0x36));
    const auto character = palette_index >= 0 && palette_index < palette_block.count
                               ? palette.u32(palette_index*8+4) : null_datum;
    std::uint8_t local_team{};
    if (character != null_datum) {
      const Bytes definition(cache.tag_payload({character}));
      auto preferred = definition.u32(0x10);
      if (preferred == null_datum)
        preferred = definition.u32(0x18);
      if (preferred != null_datum)
        local_team = std::to_integer<std::uint8_t>(cache.tag_payload({preferred})[0xc0]);
    }
    const auto type = general(cache, character).second;
    const auto expected_team = source_ai_squad_team(authored_team, local_team, std::int16_t(type));
    if (std::uint8_t(squad.team) != expected_team) {
      std::cerr << "Squad=" << i << " name=" << squad.name
                << " actual=" << squad.team << " expected=" << unsigned(expected_team)
                << " authored=" << unsigned(authored_team) << " local_unit=" << unsigned(local_team)
                << " general_type=" << type << " character=" << std::hex << character << std::dec;
      if (character != null_datum) {
        const Bytes definition(cache.tag_payload({character}));
        const auto preferred = definition.u32(0x10) != null_datum
                                   ? definition.u32(0x10) : definition.u32(0x18);
        if (const auto *tag = cache.find_tag({preferred}))
          std::cerr << " preferred=" << tag->group << ":" << tag->name;
      }
      std::cerr << '\n';
      throw std::runtime_error("Canonical AI importer supplied the wrong original squad team inputs");
    }
    const auto block = read_tag_block_descriptor(squads, i*0x74+0x48);
    const Bytes locations(block_bytes(cache, block, 0x64));
    check(squad.starts.size() == std::size_t(block.count),
          "Canonical AI import dropped starting locations");
    for (unsigned j = 0; j < squad.starts.size(); ++j) {
      check(squad.starts[j].starting_location_name == locations.u32(j*0x64),
            "Canonical AI import replaced a starting-location SID");
      ++starts;
    }
  }
  for (const auto &[datum, character] : content.characters) {
    const auto [flags, type] = general(cache, datum);
    check(character.general_flags == flags && character.actor_type == type,
          "Canonical AI import lost the first inherited general block");
    ++characters;
  }
  const auto bank = source_ai_squad_bank(content);
  check(bank.squads().count() == std::int32_t(content.squads.size()) &&
            bank.groups().count() == std::int32_t(content.squad_group_parents.size()),
        "Imported AI definitions did not construct their own source banks");
  for (unsigned i = 0; i < content.squads.size(); ++i) {
    const auto &definition = bank.definitions()[i];
    check(definition.parent_group == content.squads[i].parent_group,
          "Source AI adapter changed a parent group");
    for (unsigned j = 0; j < content.squads[i].starts.size(); ++j)
      check(definition.starting_location_names[j] == content.squads[i].starts[j].starting_location_name,
            "Source AI adapter changed a starting-location SID");
  }
  std::cout << "PASS canonical AI import: squads=" << content.squads.size()
            << " groups=" << content.squad_group_parents.size()
            << " starting_locations=" << starts << " characters=" << characters << '\n';
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
