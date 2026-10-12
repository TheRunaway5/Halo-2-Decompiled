#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct SourceEngineEntryPrefix {
  std::uint32_t entity{};
  std::int16_t definition{};
  std::uint8_t creation_applied{},unknown_07{};
  std::uint32_t unknown_08{},requested_mask{};
};
struct SourceEngineEntryRetail {
  SourceEngineEntryPrefix prefix;
  std::int32_t creation_size{};
  std::uint32_t creation_address{};
  std::int32_t state_size{};
  std::uint32_t state_address{};
};
struct SourceEngineEntryTitleUpdate {
  SourceEngineEntryPrefix prefix;
  std::uint32_t secondary_pending_mask{};
  std::int32_t creation_size{};
  std::uint32_t creation_address{};
  std::int32_t state_size{};
  std::uint32_t state_address{};
};
static_assert(sizeof(SourceEngineEntryPrefix)==0x10);
static_assert(sizeof(SourceEngineEntryRetail)==0x20);
static_assert(sizeof(SourceEngineEntryTitleUpdate)==0x24);
static_assert(offsetof(SourceEngineEntryPrefix,creation_applied)==6);
static_assert(offsetof(SourceEngineEntryRetail,creation_size)==0x10);
static_assert(offsetof(SourceEngineEntryRetail,creation_address)==0x14);
static_assert(offsetof(SourceEngineEntryTitleUpdate,creation_size)==0x14);
static_assert(offsetof(SourceEngineEntryTitleUpdate,creation_address)==0x18);
static_assert(offsetof(SourceEngineEntryRetail,state_size)==0x18);
static_assert(offsetof(SourceEngineEntryTitleUpdate,state_size)==0x1c);
// Original handle block record. The update-apply owner uses its data pointer,
// not its size/type fields; the separately bound span supplies native backing.
struct SourceEngineEntryBlock {
  std::int16_t size{},type{};
  std::uint32_t data_address{};
  std::span<const std::uint8_t> data;
};
struct SourceEngineEntryDefinition {
  // Definition slot23 receives the mutable source entry, caller mask, entry
  // state_size and the newly copied live bank. Its boolean is ignored here.
  std::function<bool(std::span<std::uint8_t> entry,std::uint32_t mask,
      std::int32_t state_size,std::span<const std::uint8_t> live_bank)> apply;
};
// Whole retail8AC40/TU1.1 8B810/TU1.5 8EA70 behavior after the caller has
// indexed its entry by handle&3FF. This owner does not check entry identity,
// block size/type, mask validity or stream overflow. The bank copy precedes the
// definition callback even when its lower engine identity gate returns false.
// Source addresses remain metadata; explicit spans supply available backing.
void source_engine_entry_update_apply(SourceEngineEntryRetail&,
    std::span<const SourceEngineEntryDefinition>,std::uint32_t mask,
    const SourceEngineEntryBlock&,std::span<std::uint8_t> live_bank);
void source_engine_entry_update_apply(SourceEngineEntryTitleUpdate&,
    std::span<const SourceEngineEntryDefinition>,std::uint32_t mask,
    const SourceEngineEntryBlock&,std::span<std::uint8_t> live_bank);
} // namespace halo2
