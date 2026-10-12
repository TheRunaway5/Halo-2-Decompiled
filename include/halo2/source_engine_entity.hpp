#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Identifiers come from registered engine slot39 and entity-definition slot0.
// They are not the source engine-table indices or variant display names.
struct SourceEngineEntityCallbacks {
  std::int32_t identifier{};
  std::function<void(std::uint32_t opaque,std::span<std::uint8_t> bank)> initialize;
  std::function<void(std::uint32_t& flags,std::uint32_t opaque,std::span<std::uint8_t> bank)> difference;
  std::function<bool(std::uint32_t flags,std::uint32_t opaque,std::span<const std::uint8_t> bank)> apply;
};
struct SourceEngineEntityCreation {
  std::uint32_t entity{};
  std::int16_t identifier{};
  std::uint16_t padding{};
};
static_assert(sizeof(SourceEngineEntityCreation)==8);
// Actual entity slots19/20/23: retail A45E0/A4650/A4730, TU1.5
// AE7F0/AE860/AE940. A mismatch performs no bank callback or mutation.
bool source_engine_entity_initialize(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t opaque,std::span<std::uint8_t> bank);
bool source_engine_entity_difference(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t& flags,std::uint32_t opaque,
    std::span<std::uint8_t> bank);
bool source_engine_entity_apply(const SourceEngineEntityCallbacks* selected,
    std::int32_t definition_identifier,std::uint32_t flags,std::uint32_t opaque,
    std::span<const std::uint8_t> bank);
// Slot22 retail A4880/TU1.5 AEA90 compares the creation record identifier,
// adopts its entity before optional application, and ignores apply's boolean.
// No entity-definition identifier participates in this owner. A missing engine
// may match identifier NONE with flags0; nonzero flags require a real provider.
bool source_engine_entity_create(const SourceEngineEntityCallbacks* selected,
    std::uint32_t& current_entity,const SourceEngineEntityCreation&,std::uint32_t flags,
    std::uint32_t opaque,std::span<const std::uint8_t> bank);
} // namespace halo2
