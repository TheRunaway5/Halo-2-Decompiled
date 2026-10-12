#include "halo2/source_engine_entity.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::int32_t identifier(const SourceEngineEntityCallbacks* selected) noexcept {
  return selected ? selected->identifier : -1;
}
const SourceEngineEntityCallbacks& engine(const SourceEngineEntityCallbacks* selected) {
  if(!selected)throw std::runtime_error("Original entity callback requires a selected engine");
  return *selected;
}
}
bool source_engine_entity_initialize(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t opaque,std::span<std::uint8_t> bank) {
  if(identifier(selected)!=definition_identifier)return false;
  const auto& callback=engine(selected).initialize;
  if(!callback)throw std::runtime_error("Original entity initialization owner unavailable");
  callback(opaque,bank);return true;
}
bool source_engine_entity_difference(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t& flags,std::uint32_t opaque,std::span<std::uint8_t> bank) {
  if(identifier(selected)!=definition_identifier)return false;
  const auto& callback=engine(selected).difference;
  if(!callback)throw std::runtime_error("Original entity difference owner unavailable");
  callback(flags,opaque,bank);return true;
}
bool source_engine_entity_apply(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t flags,std::uint32_t opaque,std::span<const std::uint8_t> bank) {
  if(identifier(selected)!=definition_identifier)return false;
  const auto& callback=engine(selected).apply;
  if(!callback)throw std::runtime_error("Original entity application owner unavailable");
  return callback(flags,opaque,bank);
}
bool source_engine_entity_create(const SourceEngineEntityCallbacks* selected,
    std::uint32_t& current_entity,const SourceEngineEntityCreation& creation,std::uint32_t flags,
    std::uint32_t opaque,std::span<const std::uint8_t> bank) {
  if(identifier(selected)!=std::int32_t(creation.identifier))return false;
  if(selected && current_entity!=0xffffffffu)current_entity=0xffffffffu;
  current_entity=creation.entity;
  if(flags) {
    const auto& callback=engine(selected).apply;
    if(!callback)throw std::runtime_error("Original creation application owner unavailable");
    callback(flags,opaque,bank);
  }
  return true;
}
} // namespace halo2
