#include "halo2/source_ai_references.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned count = 4) {
  std::uint32_t result = 0;
  for (unsigned i = 0; i < count; ++i)
    result |= std::uint32_t(bytes[offset+i]) << (8*i);
  return result;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned count = 4) {
  for (unsigned i = 0; i < count; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
// Original actor/squad/group access masks an index and reads the backing;
// it does not validate full salts or bitmap occupancy. Bound checks reject
// malformed native owner inputs without substituting another identity.
std::span<const std::uint8_t> record(const DatumPool &pool,
                                    std::uint32_t index) {
  index &= 0xffffu;
  if (index >= std::uint32_t(pool.capacity()))
    throw std::invalid_argument("AI reference index exceeds its source pool");
  return pool.elements().subspan(index*pool.stride(), pool.stride());
}
std::uint32_t next_squad(const DatumPool &pool, std::uint32_t index) {
  return std::uint32_t(std::int32_t(std::int16_t(read(record(pool, index), 0x74, 2))));
}
}

SourceAiSquadBank::SourceAiSquadBank(
    std::span<const SourceAiSquadDefinition> definitions,
    std::span<const std::int16_t> parents)
    : squads_("squad", 335, 0x98, 0), groups_("squad group", 100, 0x38, 0) {
  if (definitions.size() > 335 || parents.size() > 100)
    throw std::invalid_argument("Scenario exceeds source squad/group capacity");
  for (std::size_t i = 0; i < parents.size(); ++i) {
    auto parent = parents[i];
    unsigned visited = 0;
    while (parent != -1) {
      if (parent < 0 || std::size_t(parent) >= parents.size() ||
          ++visited > parents.size())
        throw std::invalid_argument("Scenario squad-group tree is invalid");
      parent = parents[parent];
    }
  }
  definitions_.assign(definitions.begin(), definitions.end());
  for (const auto &definition : definitions) {
    if (definition.parent_group < -1 ||
        (definition.parent_group >= 0 &&
         std::size_t(definition.parent_group) >= parents.size()))
      throw std::invalid_argument("Scenario squad names a missing parent group");
    auto bytes = squads_.lookup(squads_.allocate());
    write(bytes, 2, (definition.flags >> 8) & 3, 2);
    for (const auto offset : {0x68u, 0x70u, 0x78u, 0x80u, 0x2cu, 0x5cu})
      write(bytes, offset, none);
    for (const auto offset : {0x2au, 0x62u, 0x64u, 0x74u, 0x7eu})
      write(bytes, offset, none, 2);
    write(bytes, 0x6c, std::uint32_t(std::int32_t(definition.parent_group)));
    for (unsigned i = 0; i < 5; ++i)
      write(bytes, 0x84+i*4, none);
    write(bytes, 0x76, definition.resolved_team, 1);
  }
  for (const auto parent : parents) {
    auto bytes = groups_.lookup(groups_.allocate());
    for (const auto offset : {4u, 8u, 0xcu})
      write(bytes, offset, none);
    for (const auto offset : {0x14u, 0x24u, 0x26u, 0x2au, 0x2cu})
      write(bytes, offset, none, 2);
    write(bytes, 0x10, std::uint32_t(std::int32_t(parent)));
  }
  //2009D0 appends children and squads in ascending authored index order.
  for (unsigned i = 0; i < parents.size(); ++i) {
    if (parents[i] == -1)
      continue;
    auto parent = groups_.element_storage().subspan(parents[i]*0x38, 0x38);
    auto child = read(parent, 4);
    if (child == none)
      write(parent, 4, i);
    else {
      while (read(record(groups_, child), 0xc) != none)
        child = read(record(groups_, child), 0xc);
      write(groups_.element_storage().subspan(child*0x38, 0x38), 0xc, i);
    }
  }
  for (unsigned i = 0; i < definitions.size(); ++i) {
    const auto parent_index = definitions[i].parent_group;
    if (parent_index == -1)
      continue;
    auto parent = groups_.element_storage().subspan(parent_index*0x38, 0x38);
    auto squad = read(parent, 8);
    if (squad == none)
      write(parent, 8, i);
    else {
      while (next_squad(squads_, squad) != none)
        squad = next_squad(squads_, squad);
      write(squads_.element_storage().subspan(squad*0x98, 0x98), 0x74, i, 2);
    }
  }
}

SourceAiReferenceView::SourceAiReferenceView(SourceAiReferenceOwners owners)
    : owners_(owners) {
  if (owners.actors.capacity() != 256 || owners.actors.stride() != 0x888 ||
      owners.squads.capacity() != 335 || owners.squads.stride() != 0x98 ||
      owners.groups.capacity() != 100 || owners.groups.stride() != 0x38 ||
      owners.definitions.size() > 335)
    throw std::invalid_argument("AI reference view has the wrong source owners");
}

std::uint32_t SourceAiReferenceView::actor(std::uint32_t reference) const {
  const auto type = reference >> 30;
  if (type == 2) {
    const auto index = reference & 0xffffu;
    return (read(record(owners_.actors, index), 0, 2) << 16) | index;
  }
  if (type != 3)
    return none;
  const auto squad = (reference >> 16) & 0x3fffu;
  const auto location = std::int16_t(reference & 0xffffu);
  if (squad >= owners_.definitions.size() || location < 0 ||
      std::size_t(location) >= owners_.definitions[squad].starting_location_names.size())
    return none;
  if (!owners_.active)
    return none;
  const auto name = owners_.definitions[squad].starting_location_names[location];
  auto member = read(record(owners_.squads, squad), 0x68);
  unsigned visited = 0;
  while (member != none) {
    if (++visited > 256)
      throw std::invalid_argument("Source squad actor chain is cyclic");
    const auto bytes = record(owners_.actors, member);
    if (read(bytes, 0x38) == name)
      return member;
    member = read(bytes, 0x20);
  }
  return none;
}

void SourceAiReferenceView::append_squad_actors(
    std::uint32_t squad, std::vector<std::uint32_t> &result) const {
  if (!owners_.active)
    return;
  auto next = read(record(owners_.squads, squad), 0x68);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 256)
      throw std::invalid_argument("Source squad actor chain is cyclic");
    result.push_back(next);
    next = read(record(owners_.actors, next), 0x20);
  }
}

std::vector<std::uint32_t>
SourceAiReferenceView::group_squads(std::uint32_t root) const {
  std::vector<std::uint32_t> result;
  auto group = root;
  unsigned visited = 0;
  const auto descend = [&] {
    while (read(record(owners_.groups, group), 4) != none) {
      if (++visited > 100)
        throw std::invalid_argument("Source squad-group child chain is cyclic");
      group = read(record(owners_.groups, group), 4);
    }
  };
  descend();
  auto next = read(record(owners_.groups, group), 8);
  unsigned steps = 0;
  while (true) {
    if (++steps > 635)
      throw std::invalid_argument("Source squad-group traversal is cyclic");
    if (next != none) {
      result.push_back(next);
      next = next_squad(owners_.squads, next);
    } else {
      if (group == root)
        return result;
      const auto sibling = read(record(owners_.groups, group), 0xc);
      if (sibling == none)
        group = read(record(owners_.groups, group), 0x10);
      else {
        group = sibling;
        descend();
      }
      next = read(record(owners_.groups, group), 8);
    }
  }
}

std::vector<std::uint32_t>
SourceAiReferenceView::actors(std::uint32_t reference) const {
  std::vector<std::uint32_t> result;
  switch (reference >> 30) {
  case 0:
    append_squad_actors(reference & 0xffffu, result);
    break;
  case 1:
    for (const auto squad : group_squads(reference & 0xffffu))
      append_squad_actors(squad, result);
    break;
  default:
    if (const auto single = actor(reference); single != none)
      result.push_back(single);
    break;
  }
  return result;
}
} // namespace halo2
