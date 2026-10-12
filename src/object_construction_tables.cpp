#include "halo2/object_construction_tables.hpp"
#include <stdexcept>

namespace halo2 {
std::array<std::int16_t,2> SourceObjectConstructionTables::scenario_blocks(std::int32_t type) const {
  if(!scenario_block_offsets || type<0 || std::size_t(type)>=scenario_block_offsets->size())
    throw std::out_of_range("Original scenario block offsets unavailable");
  return (*scenario_block_offsets)[std::size_t(type)];
}
std::uint16_t SourceObjectConstructionTables::placement_extent(std::int32_t type) const {
  if(!placement_extents || type<0 || std::size_t(type)>=placement_extents->size())
    throw std::out_of_range("Original scenario placement extent unavailable");
  return (*placement_extents)[std::size_t(type)];
}
std::uint16_t SourceObjectConstructionTables::base_extent(std::int16_t type) const {
  if(!base_extents || type<0 || std::size_t(type)>=base_extents->size())
    throw std::out_of_range("Original object construction extent unavailable");
  return (*base_extents)[std::size_t(type)];
}
std::span<const std::byte> SourceObjectConstructionTables::read(
    std::uint32_t address,std::size_t bytes) const {
  auto found=ranges.upper_bound(address);
  if(found==ranges.begin())
    throw std::out_of_range("Original object construction field unavailable");
  --found;
  const auto offset=std::uint64_t(address)-found->first;
  if(offset>found->second.size()||bytes>found->second.size()-offset)
    throw std::out_of_range("Original object construction field extent unavailable");
  return std::span<const std::byte>(found->second).subspan(std::size_t(offset),bytes);
}
SourceObjectConstructionMetadata source_object_construction_metadata(
    std::shared_ptr<const SourceObjectConstructionTables> tables,
    std::function<std::uint32_t(std::int16_t)> tag_group) {
  if(!tables||!tag_group)
    throw std::invalid_argument("Original construction tables and live tag-group owner required");
  return {std::move(tag_group),[tables](std::int16_t type){
      if(type<0||std::size_t(type)>=tables->type_addresses.size())
        throw std::out_of_range("Original object construction type unavailable");
      return tables->type_addresses[std::size_t(type)];
    },[tables](std::uint32_t address,std::size_t bytes){return tables->read(address,bytes);}};
}
} // namespace halo2
